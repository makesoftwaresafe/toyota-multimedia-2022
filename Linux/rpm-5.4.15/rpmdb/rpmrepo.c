/** \ingroup rpmio
 * \file rpmdb/rpmrepo.c
 */

#include "system.h"

#if defined(WITH_DBSQL)
#include <dbsql.h>
#elif defined(WITH_SQLITE)
#include <sqlite3.h>
#ifdef	__LCLINT__
/*@-incondefs -redecl @*/
extern const char *sqlite3_errmsg(sqlite3 *db)
	/*@*/;
extern int sqlite3_open(
  const char *filename,		   /* Database filename (UTF-8) */
  /*@out@*/ sqlite3 **ppDb	   /* OUT: SQLite db handle */
)
	/*@modifies *ppDb @*/;
extern int sqlite3_exec(
  sqlite3 *db,			   /* An open database */
  const char *sql,		   /* SQL to be evaluted */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *,			   /* 1st argument to callback */
  /*@out@*/ char **errmsg	   /* Error msg written here */
)
	/*@modifies db, *errmsg @*/;
extern int sqlite3_prepare(
  sqlite3 *db,			   /* Database handle */
  const char *zSql,		   /* SQL statement, UTF-8 encoded */
  int nByte,			   /* Maximum length of zSql in bytes. */
  /*@out@*/ sqlite3_stmt **ppStmt, /* OUT: Statement handle */
  /*@out@*/ const char **pzTail	   /* OUT: Pointer to unused portion of zSql */
)
	/*@modifies *ppStmt, *pzTail @*/;
extern int sqlite3_reset(sqlite3_stmt *pStmt)
	/*@modifies pStmt @*/;
extern int sqlite3_step(sqlite3_stmt *pStmt)
	/*@modifies pStmt @*/;
extern int sqlite3_finalize(/*@only@*/ sqlite3_stmt *pStmt)
	/*@modifies pStmt @*/;
extern int sqlite3_close(sqlite3 * db)
	/*@modifies db @*/;
/*@=incondefs =redecl @*/
#endif	/* __LCLINT__ */
#endif	/* WITH_SQLITE */

#include <rpmio_internal.h>	/* XXX fdInitDigest() et al */
#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>
#include <poptIO.h>

#define	_RPMREPO_INTERNAL
#include <rpmrepo.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>
#include <rpmts.h>

#include "debug.h"

/*@unchecked@*/
int _rpmrepo_debug = 0;

#define REPODBG(_l) if (_rpmrepo_debug) fprintf _l

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static const char primary_xml_init[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<metadata xmlns=\"http://linux.duke.edu/metadata/common\" xmlns:rpm=\"http://linux.duke.edu/metadata/rpm\" packages=\"0\">\n";
/*@unchecked@*/ /*@observer@*/
static const char primary_xml_fini[] = "</metadata>\n";

/*@unchecked@*/ /*@observer@*/
static const char filelists_xml_init[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<filelists xmlns=\"http://linux.duke.edu/metadata/filelists\" packages=\"0\">\n";
/*@unchecked@*/ /*@observer@*/
static const char filelists_xml_fini[] = "</filelists>\n";

/*@unchecked@*/ /*@observer@*/
static const char other_xml_init[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<otherdata xmlns=\"http://linux.duke.edu/metadata/other\" packages=\"0\">\n";
/*@unchecked@*/ /*@observer@*/
static const char other_xml_fini[] = "</otherdata>\n";

/*@unchecked@*/ /*@observer@*/
static const char repomd_xml_init[] = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<repomd xmlns=\"http://linux.duke.edu/metadata/repo\">\n";
/*@unchecked@*/ /*@observer@*/
static const char repomd_xml_fini[] = "</repomd>\n";

/* XXX todo: wire up popt aliases and bury the --queryformat glop externally. */
/*@unchecked@*/ /*@observer@*/
static const char primary_xml_qfmt[] =
#include "yum_primary_xml"
;

/*@unchecked@*/ /*@observer@*/
static const char filelists_xml_qfmt[] =
#include "yum_filelists_xml"
;

/*@unchecked@*/ /*@observer@*/
static const char other_xml_qfmt[] =
#include "yum_other_xml"
;

/*@unchecked@*/ /*@observer@*/
static const char primary_yaml_qfmt[] =
#include "wnh_primary_yaml"
;

/*@unchecked@*/ /*@observer@*/
static const char filelists_yaml_qfmt[] =
#include "wnh_filelists_yaml"
;

/*@unchecked@*/ /*@observer@*/
static const char other_yaml_qfmt[] =
#include "wnh_other_yaml"
;

/*@unchecked@*/ /*@observer@*/
static const char Packages_qfmt[] =
#include "deb_Packages"
;

/*@unchecked@*/ /*@observer@*/
static const char Sources_qfmt[] =
#include "deb_Sources"
;

/*@-nullassign@*/
/*@unchecked@*/ /*@observer@*/
static const char *primary_sql_init[] = {
"PRAGMA synchronous = \"OFF\";",
"pragma locking_mode = \"EXCLUSIVE\";",
"CREATE TABLE conflicts (  pkgKey INTEGER,  name TEXT,  flags TEXT,  epoch TEXT,  version TEXT,  release TEXT );",
"CREATE TABLE db_info (dbversion INTEGER,  checksum TEXT);",
"CREATE TABLE files (  pkgKey INTEGER,  name TEXT,  type TEXT );",
"CREATE TABLE obsoletes (  pkgKey INTEGER,  name TEXT,  flags TEXT,  epoch TEXT,  version TEXT,  release TEXT );",
"CREATE TABLE packages (  pkgKey INTEGER PRIMARY KEY,  pkgId TEXT,  name TEXT,  arch TEXT,  version TEXT,  epoch TEXT,  release TEXT,  summary TEXT,  description TEXT,  url TEXT,  time_file INTEGER,  time_build INTEGER,  rpm_license TEXT,  rpm_vendor TEXT,  rpm_group TEXT,  rpm_buildhost TEXT,  rpm_sourcerpm TEXT,  rpm_header_start INTEGER,  rpm_header_end INTEGER,  rpm_packager TEXT,  size_package INTEGER,  size_installed INTEGER,  size_archive INTEGER,  location_href TEXT,  location_base TEXT,  checksum_type TEXT);",
"CREATE TABLE provides (  pkgKey INTEGER,  name TEXT,  flags TEXT,  epoch TEXT,  version TEXT,  release TEXT );",
"CREATE TABLE requires (  pkgKey INTEGER,  name TEXT,  flags TEXT,  epoch TEXT,  version TEXT,  release TEXT  );",
"CREATE INDEX filenames ON files (name);",
"CREATE INDEX packageId ON packages (pkgId);",
"CREATE INDEX packagename ON packages (name);",
"CREATE INDEX pkgconflicts on conflicts (pkgKey);",
"CREATE INDEX pkgobsoletes on obsoletes (pkgKey);",
"CREATE INDEX pkgprovides on provides (pkgKey);",
"CREATE INDEX pkgrequires on requires (pkgKey);",
"CREATE INDEX providesname ON provides (name);",
"CREATE INDEX requiresname ON requires (name);",
"CREATE TRIGGER removals AFTER DELETE ON packages\
\n    BEGIN\n\
\n    DELETE FROM files WHERE pkgKey = old.pkgKey;\
\n    DELETE FROM requires WHERE pkgKey = old.pkgKey;\
\n    DELETE FROM provides WHERE pkgKey = old.pkgKey;\
\n    DELETE FROM conflicts WHERE pkgKey = old.pkgKey;\
\n    DELETE FROM obsoletes WHERE pkgKey = old.pkgKey;\
\n    END;",
"INSERT into db_info values (9, 'direct_create');",
    NULL
};
/*XXX todo: DBVERSION needs to be set */

/*@unchecked@*/ /*@observer@*/
static const char *filelists_sql_init[] = {
"PRAGMA synchronous = \"OFF\";",
"pragma locking_mode = \"EXCLUSIVE\";",
"CREATE TABLE db_info (dbversion INTEGER, checksum TEXT);",
"CREATE TABLE filelist (  pkgKey INTEGER,  name TEXT,  type TEXT );",
"CREATE TABLE packages (  pkgKey INTEGER PRIMARY KEY,  pkgId TEXT);",
"CREATE INDEX filelistnames ON filelist (name);",
"CREATE INDEX keyfile ON filelist (pkgKey);",
"CREATE INDEX pkgId ON packages (pkgId);",
"CREATE TRIGGER remove_filelist AFTER DELETE ON packages\
\n    BEGIN\
\n    DELETE FROM filelist WHERE pkgKey = old.pkgKey;\
\n    END;",
"INSERT into db_info values (9, 'direct_create');",
    NULL
};
/*XXX todo: DBVERSION needs to be set */

/*@unchecked@*/ /*@observer@*/
static const char *other_sql_init[] = {
"PRAGMA synchronous = \"OFF\";",
"pragma locking_mode = \"EXCLUSIVE\";",
"CREATE TABLE changelog (  pkgKey INTEGER,  author TEXT,  date INTEGER,  changelog TEXT);",
"CREATE TABLE db_info (dbversion INTEGER, checksum TEXT);",
"CREATE TABLE packages (  pkgKey INTEGER PRIMARY KEY,  pkgId TEXT);",
"CREATE INDEX keychange ON changelog (pkgKey);",
"CREATE INDEX pkgId ON packages (pkgId);",
"CREATE TRIGGER remove_changelogs AFTER DELETE ON packages\
\n    BEGIN\
\n    DELETE FROM changelog WHERE pkgKey = old.pkgKey;\
\n    END;",
"INSERT into db_info values (9, 'direct_create');",
    NULL
};
/*XXX todo: DBVERSION needs to be set */
/*@=nullassign@*/

/* packages   1 pkgKey INTEGER PRIMARY KEY */
/* packages   2 pkgId TEXT */
/* packages   3 name TEXT */
/* packages   4 arch TEXT */
/* packages   5 version TEXT */
/* packages   6 epoch TEXT */
/* packages   7 release TEXT */
/* packages   8 summary TEXT */
/* packages   9 description TEXT */
/* packages  10 url TEXT */
/* packages  11 time_file INTEGER */
/* packages  12 time_build INTEGER */
/* packages  13 rpm_license TEXT */
/* packages  14 rpm_vendor TEXT */
/* packages  15 rpm_group TEXT */
/* packages  16 rpm_buildhost TEXT */
/* packages  17 rpm_sourcerpm TEXT */
/* packages  18 rpm_header_start INTEGER */
/* packages  19 rpm_header_end INTEGER */
/* packages  20 rpm_packager TEXT */
/* packages  21 size_package INTEGER */
/* packages  22 size_installed INTEGER */
/* packages  23 size_archive INTEGER */
/* packages  24 location_href TEXT */
/* packages  25 location_base TEXT */
/* packages  26 checksum_type TEXT */
/* obsoletes  1 pkgKey INTEGER */
/* obsoletes  2 name TEXT */
/* obsoletes  3 flags TEXT */
/* obsoletes  4 epoch TEXT */
/* obsoletes  5 version TEXT */
/* obsoletes  6 release TEXT */
/* provides   1 pkgKey INTEGER */
/* provides   2 name TEXT */
/* provides   3 flags TEXT */
/* provides   4 epoch TEXT */
/* provides   5 version TEXT */
/* provides   6 release TEXT */
/* conflicts  1 pkgKey INTEGER */
/* conflicts  2 name TEXT */
/* conflicts  3 flags TEXT */
/* conflicts  4 epoch TEXT */
/* conflicts  5 version TEXT */
/* conflicts  6 release TEXT */
/* requires   1 pkgKey INTEGER */
/* requires   2 name TEXT */
/* requires   3 flags TEXT */
/* requires   4 epoch TEXT */
/* requires   5 version TEXT */
/* requires   6 release TEXT */
/* files      1 pkgKey INTEGER */
/* files      2 name TEXT */
/* files      3 type TEXT */

/*@unchecked@*/ /*@observer@*/
static const char primary_sql_qfmt[] =
#include "yum_primary_sqlite"
;

/* packages  1 pkgKey INTEGER PRIMARY KEY */
/* packages  2 pkgId TEXT */
/* filelist  1 pkgKey INTEGER */
/* filelist  2 name TEXT */
/* filelist  3 type TEXT */

/*@unchecked@*/ /*@observer@*/
static const char filelists_sql_qfmt[] =
#include "yum_filelists_sqlite"
;

/* packages  1 pkgKey INTEGER PRIMARY KEY */
/* packages  2 pkgId TEXT */
/* changelog 1 pkgKey INTEGER */
/* changelog 2 author TEXT */
/* changelog 3 date INTEGER */
/* changelog 4 changelog TEXT */

/*@unchecked@*/ /*@observer@*/
static const char other_sql_qfmt[] =
#include "yum_other_sqlite"
;

/* XXX static when ready. */
/*@-fullinitblock@*/
/*@unchecked@*/
static struct rpmrepo_s __repo = {
    .flags	= REPO_FLAGS_PRETTY,

    .tempdir	= ".repodata",
    .finaldir	= "repodata",
    .olddir	= ".olddata",
    .markup	= ".xml",
    .pkgalgo	= PGPHASHALGO_SHA1,
    .algo	= PGPHASHALGO_SHA1,
    .primary	= {
	.type	= "primary",
	.xml_init= primary_xml_init,
	.xml_qfmt= primary_xml_qfmt,
	.xml_fini= primary_xml_fini,
	.sql_init= primary_sql_init,
	.sql_qfmt= primary_sql_qfmt,
#ifdef	NOTYET	/* XXX char **?!? */
	.sql_fini= NULL,
#endif
	.yaml_init= NULL,
	.yaml_qfmt= primary_yaml_qfmt,
	.yaml_fini= NULL,
	.Packages_init= NULL,
	.Packages_qfmt= NULL,
	.Packages_fini= NULL,
	.Sources_init= NULL,
	.Sources_qfmt= NULL,
	.Sources_fini= NULL
    },
    .filelists	= {
	.type	= "filelists",
	.xml_init= filelists_xml_init,
	.xml_qfmt= filelists_xml_qfmt,
	.xml_fini= filelists_xml_fini,
	.sql_init= filelists_sql_init,
	.sql_qfmt= filelists_sql_qfmt,
#ifdef	NOTYET	/* XXX char **?!? */
	.sql_fini= NULL,
#endif
	.yaml_init= NULL,
	.yaml_qfmt= filelists_yaml_qfmt,
	.yaml_fini= NULL,
	.Packages_init= NULL,
	.Packages_qfmt= NULL,
	.Packages_fini= NULL,
	.Sources_init= NULL,
	.Sources_qfmt= NULL,
	.Sources_fini= NULL
    },
    .other	= {
	.type	= "other",
	.xml_init= other_xml_init,
	.xml_qfmt= other_xml_qfmt,
	.xml_fini= other_xml_fini,
	.sql_init= other_sql_init,
	.sql_qfmt= other_sql_qfmt,
#ifdef	NOTYET	/* XXX char **?!? */
	.sql_fini= NULL,
#endif
	.yaml_init= NULL,
	.yaml_qfmt= other_yaml_qfmt,
	.yaml_fini= NULL,
	.Packages_init= NULL,
	.Packages_qfmt= NULL,
	.Packages_fini= NULL,
	.Sources_init= NULL,
	.Sources_qfmt= NULL,
	.Sources_fini= NULL
    },
    .repomd	= {
	.type	= "repomd",
	.xml_init= repomd_xml_init,
	.xml_qfmt= NULL,
	.xml_fini= repomd_xml_fini,
	.sql_init= NULL,
	.sql_qfmt= NULL,
#ifdef	NOTYET	/* XXX char **?!? */
	.sql_fini= NULL,
#endif
	.yaml_init= NULL,
	.yaml_qfmt= NULL,
	.yaml_fini= NULL,
	.Packages_init= NULL,
	.Packages_qfmt= Packages_qfmt,
	.Packages_fini= NULL,
	.Sources_init= NULL,
	.Sources_qfmt= Sources_qfmt,
	.Sources_fini= NULL
    }
};
/*@=fullinitblock@*/

/*@unchecked@*/
static rpmrepo _repo = &__repo;

/*==============================================================*/

/**
 * Return stat(2) for a file.
 * @retval st		stat(2) buffer
 * @return		0 on success
 */
static int rpmioExists(const char * fn, /*@out@*/ struct stat * st)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies st, fileSystem, internalState @*/
{
    return (Stat(fn, st) == 0);
}

/**
 * Return stat(2) creation time of a file.
 * @param fn		file path
 * @return		st_ctime
 */
static time_t rpmioCtime(const char * fn)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    struct stat sb;
    time_t stctime = 0;

    if (rpmioExists(fn, &sb))
	stctime = sb.st_ctime;
    return stctime;
}

/*==============================================================*/

void
rpmrepoError(int lvl, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    (void) fflush(NULL);
    (void) fprintf(stderr, "%s: ", __progname);
    (void) vfprintf(stderr, fmt, ap);
    va_end (ap);
    (void) fprintf(stderr, "\n");
    if (lvl)
	exit(EXIT_FAILURE);
}

/**
 * Return /repository/directory/component.markup.compression path.
 * @param repo		repository
 * @param dir		directory
 * @param type		file
 * @param compress	file
 * @return		repository file path
 */
static const char * rpmrepoGetPath(rpmrepo repo, const char * dir,
		const char * type, int compress)
	/*@globals h_errno, rpmGlobalMacroContext, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/
{
    return rpmGetPath(repo->outputdir, "/", dir, "/", type,
		(repo->markup != NULL ? repo->markup : ""),
		(repo->suffix != NULL && compress ? repo->suffix : ""), NULL);
}

/**
 * Display progress.
 * @param repo		repository 
 * @param item		repository item (usually a file path)
 * @param current	current iteration index
 * @param total		maximum iteration index
 */
static void rpmrepoProgress(/*@unused@*/ rpmrepo repo,
		/*@null@*/ const char * item, int current, int total)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    static size_t ncols = 80 - 1;	/* XXX TIOCGWINSIZ */
    const char * bn = (item != NULL ? strrchr(item, '/') : NULL);
    size_t nb;

    if (bn != NULL)
	bn++;
    else
	bn = item;
    nb = fprintf(stdout, "\r%s: %d/%d", __progname, current, total);
    if (bn != NULL)
	nb += fprintf(stdout, " - %s", bn);
    nb--;
    if (nb < ncols)
	fprintf(stdout, "%*s", (int)(ncols - nb), "");
    ncols = nb;
    (void) fflush(stdout);
}

/**
 * Create directory path.
 * @param repo		repository 
 * @param dn		directory path
 * @return		0 on success
 */
static int rpmrepoMkdir(rpmrepo repo, const char * dn)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * dnurl = rpmGetPath(repo->outputdir, "/", dn, NULL);
/*@-mods@*/
    int ut = urlPath(dnurl, &dn);
/*@=mods@*/
    int rc = 0;;

    /* XXX todo: rpmioMkpath doesn't grok URI's */
    if (ut == URL_IS_UNKNOWN)
	rc = rpmioMkpath(dn, 0755, (uid_t)-1, (gid_t)-1);
    else
	rc = (Mkdir(dnurl, 0755) == 0 || errno == EEXIST ? 0 : -1);
    if (rc)
	rpmrepoError(0, _("Cannot create/verify %s: %s"), dnurl, strerror(errno));
    dnurl = _free(dnurl);
    return rc;
}

const char * rpmrepoRealpath(const char * lpath)
{
    /* XXX GLIBC: realpath(path, NULL) return malloc'd */
    const char *rpath = Realpath(lpath, NULL);
    if (rpath == NULL) {
	char fullpath[MAXPATHLEN];
	rpath = Realpath(lpath, fullpath);
	if (rpath != NULL)
	    rpath = xstrdup(rpath);
    }
    return rpath;
}

/*==============================================================*/

int rpmrepoTestSetupDirs(rpmrepo repo)
{
    const char ** directories = repo->directories;
    struct stat sb, *st = &sb;
    const char * dn;
    const char * fn;
    int rc = 0;

    /* XXX todo: check repo->pkglist existence? */

    if (directories != NULL)
    while ((dn = *directories++) != NULL) {
	if (!rpmioExists(dn, st) || !S_ISDIR(st->st_mode)) {
	    rpmrepoError(0, _("Directory %s must exist"), dn);
	    rc = 1;
	}
    }

    /* XXX todo create outputdir if it doesn't exist? */
    if (!rpmioExists(repo->outputdir, st)) {
	rpmrepoError(0, _("Directory %s does not exist."), repo->outputdir);
	rc = 1;
    }
    if (Access(repo->outputdir, W_OK)) {
	rpmrepoError(0, _("Directory %s must be writable."), repo->outputdir);
	rc = 1;
    }

    if (rpmrepoMkdir(repo, repo->tempdir)
     || rpmrepoMkdir(repo, repo->finaldir))
	rc = 1;

    dn = rpmGetPath(repo->outputdir, "/", repo->olddir, NULL);
    if (rpmioExists(dn, st)) {
	rpmrepoError(0, _("Old data directory exists, please remove: %s"), dn);
	rc = 1;
    }
    dn = _free(dn);

  { /*@observer@*/
    static const char * dirs[] = { ".repodata", "repodata", NULL };
    /*@observer@*/
    static const char * types[] =
	{ "primary", "filelists", "other", "repomd", NULL };
    const char ** dirp, ** typep;
    for (dirp = dirs; *dirp != NULL; dirp++) {
	for (typep = types; *typep != NULL; typep++) {
	    fn = rpmrepoGetPath(repo, *dirp, *typep, strcmp(*typep, "repomd"));
	    if (rpmioExists(fn, st)) {
		if (Access(fn, W_OK)) {
		    rpmrepoError(0, _("Path must be writable: %s"), fn);
		    rc = 1;
		} else
		if (REPO_ISSET(CHECKTS) && st->st_ctime > repo->mdtimestamp)
		    repo->mdtimestamp = st->st_ctime;
	    }
	    fn = _free(fn);
	}
    }
  }

#ifdef	NOTYET		/* XXX repo->package_dir needs to go away. */
    if (repo->groupfile != NULL) {
	if (REPO_ISSET(SPLIT) || repo->groupfile[0] != '/') {
	    fn = rpmGetPath(repo->package_dir, "/", repo->groupfile, NULL);
	    repo->groupfile = _free(repo->groupfile);
	    repo->groupfile = fn;
	    fn = NULL;
	}
	if (!rpmioExists(repo->groupfile, st)) {
	    rpmrepoError(0, _("groupfile %s cannot be found."), repo->groupfile);
	    rc = 1;
	}
    }
#endif
    return rc;
}

/**
 * Check file name for a suffix.
 * @param fn            file name
 * @param suffix        suffix
 * @return              1 if file name ends with suffix
 */
static int chkSuffix(const char * fn, const char * suffix)
        /*@*/
{
    size_t flen = strlen(fn);
    size_t slen = strlen(suffix);
    return (flen > slen && !strcmp(fn + flen - slen, suffix));
}

const char ** rpmrepoGetFileList(rpmrepo repo, const char *roots[],
		const char * ext)
{
    const char ** pkglist = NULL;
    FTS * t;
    FTSENT * p;
    int xx;

    if ((t = Fts_open((char *const *)roots, repo->ftsoptions, NULL)) == NULL)
	rpmrepoError(1, _("Fts_open: %s"), strerror(errno));

    while ((p = Fts_read(t)) != NULL) {
#ifdef	NOTYET
	const char * fts_name = p->fts_name;
	size_t fts_namelen = p->fts_namelen;

	/* XXX fts(3) (and Fts(3)) have fts_name = "" with pesky trailing '/' */
	if (p->fts_level == 0 && fts_namelen == 0) {
            fts_name = ".";
            fts_namelen = sizeof(".") - 1;
	}
#endif

	/* Should this element be excluded/included? */
	/* XXX todo: apply globs to fts_path rather than fts_name? */
/*@-onlytrans@*/
	if (mireApply(repo->excludeMire, repo->nexcludes, p->fts_name, 0, -1) >= 0)
	    continue;
	if (mireApply(repo->includeMire, repo->nincludes, p->fts_name, 0, +1) < 0)
	    continue;
/*@=onlytrans@*/

	switch (p->fts_info) {
	case FTS_D:
	case FTS_DP:
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case FTS_SL:
	    if (REPO_ISSET(NOFOLLOW))
		continue;
	    /* XXX todo: fuss with symlinks */
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case FTS_F:
	    /* Is this a *.rpm file? */
	    if (chkSuffix(p->fts_name, ext))
		xx = argvAdd(&pkglist, p->fts_path);
	    /*@switchbreak@*/ break;
	}
    }

    (void) Fts_close(t);

if (_rpmrepo_debug)
argvPrint("pkglist", pkglist, NULL);

    return pkglist;
}

int rpmrepoCheckTimeStamps(rpmrepo repo)
{
    int rc = 0;

    if (REPO_ISSET(CHECKTS)) {
	const char ** pkg;

	if (repo->pkglist != NULL)
	for (pkg = repo->pkglist; *pkg != NULL ; pkg++) {
	    struct stat sb, *st = &sb;
	    if (!rpmioExists(*pkg, st)) {
		rpmrepoError(0, _("cannot get to file: %s"), *pkg);
		rc = 1;
	    } else if (st->st_ctime > repo->mdtimestamp)
		rc = 1;
	}
    } else
	rc = 1;

    return rc;
}

/**
 * Write to a repository metadata file.
 * @param rfile		repository metadata file
 * @param spew		contents
 * @return		0 on success
 */
static int rpmrfileXMLWrite(rpmrfile rfile, const char * spew)
	/*@globals fileSystem @*/
	/*@modifies rfile, fileSystem @*/
{
    size_t nspew = (spew != NULL ? strlen(spew) : 0);
/*@-nullpass@*/	/* XXX spew != NULL @*/
    size_t nb = (nspew > 0 ? Fwrite(spew, 1, nspew, rfile->fd) : 0);
/*@=nullpass@*/
    int rc = 0;
    if (nspew != nb) {
	rpmrepoError(0, _("Fwrite failed: expected write %u != %u bytes: %s\n"),
		(unsigned)nspew, (unsigned)nb, Fstrerror(rfile->fd));
	rc = 1;
    }
    spew = _free(spew);
    return rc;
}

/**
 * Close an I/O stream, accumulating uncompress/digest statistics.
 * @param repo		repository
 * @param fd		I/O stream
 * @return		0 on success
 */
static int rpmrepoFclose(rpmrepo repo, FD_t fd)
	/*@modifies repo, fd @*/
{
    int rc = 0;

    if (fd != NULL) {
#ifdef	NOTYET
	rpmts ts = repo->_ts;
	if (ts != NULL) {
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_UNCOMPRESS),
			fdstat_op(fd, FDSTAT_READ));
	    (void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DIGEST),
			fdstat_op(fd, FDSTAT_DIGEST));
	}
#endif
	rc = Fclose(fd);
    }
    return rc;
}

/**
 * Open a repository metadata file.
 * @param repo		repository
 * @param rfile		repository metadata file
 * @return		0 on success
 */
static int rpmrepoOpenMDFile(const rpmrepo repo, rpmrfile rfile)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies rfile, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * spew = rfile->xml_init;
    size_t nspew = strlen(spew);
    const char * fn = rpmrepoGetPath(repo, repo->tempdir, rfile->type, 1);
    const char * tail;
    size_t nb;
    int rc = 0;

    rfile->fd = Fopen(fn, repo->wmode);
assert(rfile->fd != NULL);

    if (repo->algo != PGPHASHALGO_NONE)
	fdInitDigest(rfile->fd, repo->algo, 0);

    if ((tail = strstr(spew, " packages=\"0\">\n")) != NULL)
	nspew -= strlen(tail);

    nb = Fwrite(spew, 1, nspew, rfile->fd);

    if (tail != NULL) {
	char buf[64];
	size_t tnb = snprintf(buf, sizeof(buf), " packages=\"%d\">\n",
				repo->pkgcount);
	nspew += tnb;
	nb += Fwrite(buf, 1, tnb, rfile->fd);
    }
    if (nspew != nb) {
	rpmrepoError(0, _("Fwrite failed: expected write %u != %u bytes: %s\n"),
		(unsigned)nspew, (unsigned)nb, Fstrerror(rfile->fd));
	rc = 1;
    }

    fn = _free(fn);

#if defined(WITH_SQLITE)
    if (REPO_ISSET(DATABASE)) {
	const char ** stmt;
	int xx;
	fn = rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
		rfile->type, ".sqlite", NULL);
	if ((xx = sqlite3_open(fn, &rfile->sqldb)) != SQLITE_OK)
	    rpmrepoError(1, "sqlite3_open(%s): %s", fn, sqlite3_errmsg(rfile->sqldb));
	for (stmt = rfile->sql_init; *stmt != NULL; stmt++) {
	    char * msg;
	    xx = sqlite3_exec(rfile->sqldb, *stmt, NULL, NULL, &msg);
	    if (xx != SQLITE_OK)
		rpmrepoError(1, "sqlite3_exec(%s, \"%s\"): %s\n", fn, *stmt,
			(msg != NULL ? msg : "failed"));
	}
	fn = _free(fn);
    }
#endif

    return rc;
}

#if defined(WITH_SQLITE)
/**
 * Check sqlite3 return, displaying error messages.
 * @param rfile		repository metadata file
 * @param msg		message
 * @param rc		return code
 * @return		SQLITE_OK on success
 */
static int rpmrfileSQL(rpmrfile rfile, const char * msg, int rc)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (rc != SQLITE_OK || _rpmrepo_debug)
	rpmrepoError(0, "sqlite3_%s(%s): %s", msg, rfile->type,
		sqlite3_errmsg(rfile->sqldb));
    return rc;
}

/**
 * Execute a compiled SQL command.
 * @param rfile		repository metadata file
 * @param stmt		SQL statement
 * @return		SQLITE_OK on success
 */
static int rpmrfileSQLStep(rpmrfile rfile, sqlite3_stmt * stmt)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int loop = 1;
    int rc = 0;
    int xx;

/*@-infloops@*/
    while (loop) {
	rc = sqlite3_step(stmt);
	switch (rc) {
	default:
	    rc = rpmrfileSQL(rfile, "step", rc);
	    /*@fallthrough@*/
	case SQLITE_DONE:
	    loop = 0;
	    /*@switchbreak@*/ break;
	}
    }
/*@=infloops@*/

    xx = rpmrfileSQL(rfile, "reset",
	sqlite3_reset(stmt));

    return rc;
}

/**
 * Run a sqlite3 command.
 * @param rfile		repository metadata file
 * @param cmd		sqlite3 command to run
 * @return		0 always
 */
static int rpmrfileSQLWrite(rpmrfile rfile, const char * cmd)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    sqlite3_stmt * stmt;
    const char * tail;
    int xx;

    xx = rpmrfileSQL(rfile, "prepare",
	sqlite3_prepare(rfile->sqldb, cmd, (int)strlen(cmd), &stmt, &tail));

    xx = rpmrfileSQL(rfile, "reset",
	sqlite3_reset(stmt));

    xx = rpmrfileSQLStep(rfile, stmt);

    xx = rpmrfileSQL(rfile, "finalize",
	sqlite3_finalize(stmt));

    cmd = _free(cmd);

    return 0;
}
#endif	/* WITH_SQLITE */

/**
 * Compute digest of a file.
 * @return		0 on success
 */
static int rpmrepoRfileDigest(const rpmrepo repo, rpmrfile rfile,
		const char ** digestp)
	/*@modifies *digestp @*/
{
    static int asAscii = 1;
    struct stat sb, *st = &sb;
    const char * fn = rpmrepoGetPath(repo, repo->tempdir, rfile->type, 1);
    const char * path = NULL;
    int ut = urlPath(fn, &path);
    FD_t fd = NULL;
    int rc = 1;
    int xx;

    memset(st, 0, sizeof(*st));
    if (!rpmioExists(fn, st))
	goto exit;
    fd = Fopen(fn, "r.ufdio");
    if (fd == NULL || Ferror(fd))
	goto exit;

    switch (ut) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
#if defined(HAVE_MMAP)
    {   void * mapped = (void *)-1;

	if (st->st_size > 0)
	    mapped = mmap(NULL, st->st_size, PROT_READ, MAP_SHARED, Fileno(fd), 0);
	if (mapped != (void *)-1) {
#ifdef	NOTYET
	    rpmts ts = repo->_ts;
	    rpmop op = rpmtsOp(ts, RPMTS_OP_DIGEST);
	    rpmtime_t tstamp = rpmswEnter(op, 0);
#endif
	    DIGEST_CTX ctx = rpmDigestInit(repo->algo, RPMDIGEST_NONE);
	    xx = rpmDigestUpdate(ctx, mapped, st->st_size);
	    xx = rpmDigestFinal(ctx, digestp, NULL, asAscii);
#ifdef	NOTYET
	    tstamp = rpmswExit(op, st->st_size);
#endif
	    xx = munmap(mapped, st->st_size);
	    break;
	}
    }	/*@fallthrough@*/
#endif
    default:
    {	char buf[64 * BUFSIZ];
	size_t nb;
	size_t fsize = 0;

	fdInitDigest(fd, repo->algo, 0);
	while ((nb = Fread(buf, sizeof(buf[0]), sizeof(buf), fd)) > 0)
	    fsize += nb;
	if (Ferror(fd))
	    goto exit;
	fdFiniDigest(fd, repo->algo, digestp, NULL, asAscii);
    }	break;
    }

    rc = 0;

exit:
    if (fd)
	xx = rpmrepoFclose(repo, fd);
    fn = _free(fn);
    return rc;
}

/**
 * Close a repository metadata file.
 * @param repo		repository
 * @param rfile		repository metadata file
 * @return		0 on success
 */
static int rpmrepoCloseMDFile(const rpmrepo repo, rpmrfile rfile)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies rfile, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    static int asAscii = 1;
    char * xmlfn = xstrdup(fdGetOPath(rfile->fd));
    int rc = 0;

    if (!repo->quiet)
	rpmrepoError(0, _("Saving %s metadata"), basename(xmlfn));

    if (rpmrfileXMLWrite(rfile, xstrdup(rfile->xml_fini)))
	rc = 1;

    if (repo->algo > 0)
	fdFiniDigest(rfile->fd, repo->algo, &rfile->digest, NULL, asAscii);
    else
	rfile->digest = xstrdup("");

    (void) rpmrepoFclose(repo, rfile->fd);
    rfile->fd = NULL;

    /* Compute the (usually compressed) ouput file digest too. */
    rfile->Zdigest = NULL;
    (void) rpmrepoRfileDigest(repo, rfile, &rfile->Zdigest);

#if defined(WITH_SQLITE)
    if (REPO_ISSET(DATABASE) && rfile->sqldb != NULL) {
	const char *dbfn = rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
		rfile->type, ".sqlite", NULL);
	int xx;
	if ((xx = sqlite3_close(rfile->sqldb)) != SQLITE_OK)
	    rpmrepoError(1, "sqlite3_close(%s): %s", dbfn, sqlite3_errmsg(rfile->sqldb));
	rfile->sqldb = NULL;
	dbfn = _free(dbfn);
    }
#endif

    rfile->ctime = rpmioCtime(xmlfn);
    xmlfn = _free(xmlfn);

    return rc;
}

/**
 */
static /*@observer@*/ /*@null@*/ const char *
algo2tagname(uint32_t algo)
	/*@*/
{
    const char * tagname = NULL;

    switch (algo) {
    case PGPHASHALGO_NONE:	tagname = "none";	break;
    case PGPHASHALGO_MD5:	tagname = "md5";	break;
    /* XXX todo: should be "sha1" */
    case PGPHASHALGO_SHA1:	tagname = "sha";	break;
    case PGPHASHALGO_RIPEMD160:	tagname = "rmd160";	break;
    case PGPHASHALGO_MD2:	tagname = "md2";	break;
    case PGPHASHALGO_TIGER192:	tagname = "tiger192";	break;
    case PGPHASHALGO_HAVAL_5_160: tagname = "haval160";	break;
    case PGPHASHALGO_SHA256:	tagname = "sha256";	break;
    case PGPHASHALGO_SHA384:	tagname = "sha384";	break;
    case PGPHASHALGO_SHA512:	tagname = "sha512";	break;
    case PGPHASHALGO_MD4:	tagname = "md4";	break;
    case PGPHASHALGO_RIPEMD128:	tagname = "rmd128";	break;
    case PGPHASHALGO_CRC32:	tagname = "crc32";	break;
    case PGPHASHALGO_ADLER32:	tagname = "adler32";	break;
    case PGPHASHALGO_CRC64:	tagname = "crc64";	break;
    case PGPHASHALGO_JLU32:	tagname = "jlu32";	break;
    case PGPHASHALGO_SHA224:	tagname = "sha224";	break;
    case PGPHASHALGO_RIPEMD256:	tagname = "rmd256";	break;
    case PGPHASHALGO_RIPEMD320:	tagname = "rmd320";	break;
    case PGPHASHALGO_SALSA10:	tagname = "salsa10";	break;
    case PGPHASHALGO_SALSA20:	tagname = "salsa20";	break;
    default:			tagname = NULL;		break;
    }
    return tagname;
}

/**
 * Return a repository metadata file item.
 * @param repo		repository
 * @param rfile		repository file
 * @return		repository metadata file item
 */
static const char * rpmrepoMDExpand(rpmrepo repo, rpmrfile rfile)
	/*@globals h_errno, rpmGlobalMacroContext, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/
{
    const char * spewalgo = algo2tagname(repo->algo);
    char spewtime[64];

    (void) snprintf(spewtime, sizeof(spewtime), "%u", (unsigned)rfile->ctime);
    return rpmExpand("\
  <data type=\"", rfile->type, "\">\n\
    <checksum type=\"", spewalgo, "\">", rfile->Zdigest, "</checksum>\n\
    <timestamp>", spewtime, "</timestamp>\n\
    <open-checksum type=\"",spewalgo,"\">", rfile->digest, "</open-checksum>\n\
    <location href=\"", repo->finaldir, "/", rfile->type, (repo->markup != NULL ? repo->markup : ""), (repo->suffix != NULL ? repo->suffix : ""), "\"/>\n\
  </data>\n", NULL);
}

int rpmrepoDoRepoMetadata(rpmrepo repo)
{
    rpmrfile rfile = &repo->repomd;
    const char * fn = rpmrepoGetPath(repo, repo->tempdir, rfile->type, 0);
    int rc = 0;

    if ((rfile->fd = Fopen(fn, "w.ufdio")) != NULL) {	/* no compression */
	if (rpmrfileXMLWrite(rfile, xstrdup(rfile->xml_init))
	 || rpmrfileXMLWrite(rfile, rpmrepoMDExpand(repo, &repo->other))
	 || rpmrfileXMLWrite(rfile, rpmrepoMDExpand(repo, &repo->filelists))
	 || rpmrfileXMLWrite(rfile, rpmrepoMDExpand(repo, &repo->primary))
	 || rpmrfileXMLWrite(rfile, xstrdup(rfile->xml_fini)))
	    rc = 1;
	(void) rpmrepoFclose(repo, rfile->fd);
	rfile->fd = NULL;
    }

    fn = _free(fn);
    if (rc) return rc;

#ifdef	NOTYET
    def doRepoMetadata(self):
        """wrapper to generate the repomd.xml file that stores the info on the other files"""
    const char * repopath =
		rpmGetPath(repo->outputdir, "/", repo->tempdir, NULL);
        repodoc = libxml2.newDoc("1.0")
        reporoot = repodoc.newChild(None, "repomd", None)
        repons = reporoot.newNs("http://linux.duke.edu/metadata/repo", None)
        reporoot.setNs(repons)
        repopath = rpmGetPath(repo->outputdir, "/", repo->tempdir, NULL);
        fn = rpmrepoGetPath(repo, repo->tempdir, repo->repomd.type, 1);

        repoid = "garbageid";

        if (REPO_ISSET(DATABASE)) {
            if (!repo->quiet) rpmrepoError(0, _("Generating sqlite DBs"));
            try:
                dbversion = str(sqlitecachec.DBVERSION)
            except AttributeError:
                dbversion = "9"
            rp = sqlitecachec.RepodataParserSqlite(repopath, repoid, None)
	}

  { static const char * types[] =
	{ "primary", "filelists", "other", NULL };
    const char ** typep;
	for (typep = types; *typep != NULL; typep++) {
	    complete_path = rpmrepoGetPath(repo, repo->tempdir, *typep, 1);

            zfo = _gzipOpen(complete_path)
            uncsum = misc.checksum(algo2tagname(repo->algo), zfo)
            zfo.close()
            csum = misc.checksum(algo2tagname(repo->algo), complete_path)
	    (void) rpmioExists(complete_path, st)
            timestamp = os.stat(complete_path)[8]

            db_csums = {}
            db_compressed_sums = {}

            if (REPO_ISSET(repo)) {
                if (repo->verbose) {
		    time_t now = time(NULL);
                    rpmrepoError(0, _("Starting %s db creation: %s"),
			*typep, ctime(&now));
		}

                if (!strcmp(*typep, "primary"))
                    rp.getPrimary(complete_path, csum)
                else if (!strcmp(*typep, "filelists"));
                    rp.getFilelists(complete_path, csum)
                else if (!strcmp(*typep, "other"))
                    rp.getOtherdata(complete_path, csum)

		{   const char *  tmp_result_path =
			rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
				*typep, ".xml.gz.sqlite", NULL);
		    const char * resultpath =
			rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
				*typep, ".sqlite", NULL);

		    /* rename from silly name to not silly name */
		    xx = Rename(tmp_result_path, resultpath);
		    tmp_result_path = _free(tmp_result_path);
		    result_compressed =
			rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
				*typep, ".sqlite.bz2", NULL);
		    db_csums[*typep] = misc.checksum(algo2tagname(repo->algo), resultpath)

		    /* compress the files */
		    bzipFile(resultpath, result_compressed)
		    /* csum the compressed file */
		    db_compressed_sums[*typep] = misc.checksum(algo2tagname(repo->algo), result_compressed)
		    /* remove the uncompressed file */
		    xx = Unlink(resultpath);
		    resultpath = _free(resultpath);
		}

                if (REPO_ISSET(UNIQUEMDFN)) {
                    const char * csum_result_compressed =
			rpmGetPath(repo->outputdir, "/", repo->tempdir, "/",
				db_compressed_sums[*typep], "-", *typep, ".sqlite.bz2", NULL);
                    xx = Rename(result_compressed, csum_result_compressed);
		    result_compressed = _free(result_compressed);
		    result_compressed = csum_result_compressed;
		}

                /* timestamp the compressed file */
		(void) rpmioExists(result_compressed, st)
                db_timestamp = os.stat(result_compressed)[8]

                /* add this data as a section to the repomdxml */
                db_data_type = rpmExpand(*typep, "_db", NULL);
                data = reporoot.newChild(None, 'data', None)
                data.newProp('type', db_data_type)
                location = data.newChild(None, 'location', None)
                if (repo->baseurl != NULL) {
                    location.newProp('xml:base', repo->baseurl)
		}

                location.newProp('href', rpmGetPath(repo->finaldir, "/", *typep, ".sqlite.bz2", NULL));
                checksum = data.newChild(None, 'checksum', db_compressed_sums[*typep])
                checksum.newProp('type', algo2tagname(repo->algo))
                db_tstamp = data.newChild(None, 'timestamp', str(db_timestamp))
                unchecksum = data.newChild(None, 'open-checksum', db_csums[*typep])
                unchecksum.newProp('type', algo2tagname(repo->algo))
                database_version = data.newChild(None, 'database_version', dbversion)
                if (repo->verbose) {
		   time_t now = time(NULL);
                   rpmrepoError(0, _("Ending %s db creation: %s"),
			*typep, ctime(&now));
		}
	    }

            data = reporoot.newChild(None, 'data', None)
            data.newProp('type', *typep)

            checksum = data.newChild(None, 'checksum', csum)
            checksum.newProp('type', algo2tagname(repo->algo))
            timestamp = data.newChild(None, 'timestamp', str(timestamp))
            unchecksum = data.newChild(None, 'open-checksum', uncsum)
            unchecksum.newProp('type', algo2tagname(repo->algo))
            location = data.newChild(None, 'location', None)
            if (repo->baseurl != NULL)
                location.newProp('xml:base', repo->baseurl)
            if (REPO_ISSET(UNIQUEMDFN)) {
                orig_file = rpmrepoGetPath(repo, repo->tempdir, *typep, strcmp(*typep, "repomd"));
                res_file = rpmExpand(csum, "-", *typep,
			(repo->markup ? repo->markup : ""),
			(repo->suffix && strcmp(*typep, "repomd") ? repo->suffix : ""), NULL);
                dest_file = rpmGetPath(repo->outputdir, "/", repo->tempdir, "/", res_file, NULL);
                xx = Rename(orig_file, dest_file);

            } else
                res_file = rpmExpand(*typep,
			(repo->markup ? repo->markup : ""),
			(repo->suffix && strcmp(*typep, "repomd") ? repo->suffix : ""), NULL);

            location.newProp('href', rpmGetPath(repo->finaldir, "/", res_file, NULL));
	}
  }

        if (!repo->quiet && REPO_ISSET(DATABASE))
	    rpmrepoError(0, _("Sqlite DBs complete"));

        if (repo->groupfile != NULL) {
            self.addArbitraryMetadata(repo->groupfile, 'group_gz', reporoot)
            self.addArbitraryMetadata(repo->groupfile, 'group', reporoot, compress=False)
	}

        /* save it down */
        try:
            repodoc.saveFormatFileEnc(fn, 'UTF-8', 1)
        except:
            rpmrepoError(0, _("Error saving temp file for %s%s%s: %s"),
		rfile->type,
		(repo->markup ? repo->markup : ""),
		(repo->suffix && strcmp(*typep, "repomd") ? repo->suffix : ""),
		fn);
            rpmrepoError(1, _("Could not save temp file: %s"), fn);

        del repodoc
#endif

    return rc;
}

int rpmrepoDoFinalMove(rpmrepo repo)
{
    char * output_final_dir =
		rpmGetPath(repo->outputdir, "/", repo->finaldir, NULL);
    char * output_old_dir =
		rpmGetPath(repo->outputdir, "/", repo->olddir, NULL);
    struct stat sb, *st = &sb;
    int xx;

    if (rpmioExists(output_final_dir, st)) {
	if ((xx = Rename(output_final_dir, output_old_dir)) != 0)
	    rpmrepoError(1, _("Error moving final %s to old dir %s"),
			output_final_dir, output_old_dir);
    }

    {	const char * output_temp_dir =
		rpmGetPath(repo->outputdir, "/", repo->tempdir, NULL);
	if ((xx = Rename(output_temp_dir, output_final_dir)) != 0) {
	    xx = Rename(output_old_dir, output_final_dir);
	    rpmrepoError(1, _("Error moving final metadata into place"));
	}
	output_temp_dir = _free(output_temp_dir);
    }

    /* Merge old tree into new, unlink/rename as needed. */
  { 
    char *const _av[] = { output_old_dir, NULL };
    int _ftsoptions = FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV;
    int (*_compar)(const FTSENT **, const FTSENT **) = NULL;
    FTS * t = Fts_open(_av, _ftsoptions, _compar);
    FTSENT * p = NULL;

    if (t != NULL)
    while ((p = Fts_read(t)) != NULL) {
	const char * opath = p->fts_accpath;
	const char * ofn = p->fts_path;
	const char * obn = p->fts_name;
	const char * nfn = NULL;
	switch (p->fts_info) {
	default:
	    break;
	case FTS_DP:
	    /* Remove empty directories on post-traversal visit. */
	    if ((xx = Rmdir(opath)) != 0)
		rpmrepoError(1, _("Could not remove old metadata directory: %s: %s"),
				ofn, strerror(errno));
	    break;
	case FTS_F:
	    /* Remove all non-toplevel files. */
	    if (p->fts_level > 0) {
		if ((xx = Unlink(opath)) != 0)
		    rpmrepoError(1, _("Could not remove old metadata file: %s: %s"),
				ofn, strerror(errno));
		break;
	    }

	    /* Remove/rename toplevel files, dependent on target existence. */
	    nfn = rpmGetPath(output_final_dir, "/", obn, NULL);
	    if (rpmioExists(nfn, st)) {
		if ((xx = Unlink(opath)) != 0)
		    rpmrepoError(1, _("Could not remove old metadata file: %s: %s"),
				ofn, strerror(errno));
	    } else {
		if ((xx = Rename(opath, nfn)) != 0)
		    rpmrepoError(1, _("Could not restore old non-metadata file: %s -> %s: %s"),
				ofn, nfn, strerror(errno));
	    }
	    nfn = _free(nfn);
	    break;
	case FTS_SL:
	case FTS_SLNONE:
	    /* Remove all symlinks. */
	    if ((xx = Unlink(opath)) != 0)
		rpmrepoError(1, _("Could not remove old metadata symlink: %s: %s"),
				ofn, strerror(errno));
	    break;
	}
    }
    if (t != NULL)
        Fts_close(t);
  }

    output_old_dir = _free(output_old_dir);
    output_final_dir = _free(output_final_dir);

    return 0;
}

/*==============================================================*/

/**
 * Read a header from a repository package file, computing package file digest.
 * @param repo		repository
 * @param path		package file path
 * @return		header (NULL on error)
 */
static Header rpmrepoReadHeader(rpmrepo repo, const char * path)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies repo, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    /* XXX todo: read the payload and collect the blessed file digest. */
    FD_t fd = Fopen(path, "r.ufdio");
    Header h = NULL;

    if (fd != NULL) {
	rpmts ts = repo->_ts;
	uint32_t algo = repo->pkgalgo;
	rpmRC rpmrc;

	if (algo != PGPHASHALGO_NONE)
	    fdInitDigest(fd, algo, 0);

	/* XXX what if path needs expansion? */
	rpmrc = rpmReadPackageFile(ts, fd, path, &h);
	if (algo != PGPHASHALGO_NONE) {
	    char buffer[32 * BUFSIZ];
	    size_t nb = sizeof(buffer);
	    size_t nr;
	    while ((nr = Fread(buffer, sizeof(buffer[0]), nb, fd)) == nb)
		{};
	    if (Ferror(fd)) {
		fprintf(stderr, _("%s: Fread(%s) failed: %s\n"),
			__progname, path, Fstrerror(fd));
		rpmrc = RPMRC_FAIL;
	    } else {
		static int asAscii = 1;
		const char *digest = NULL;
		fdFiniDigest(fd, algo, &digest, NULL, asAscii);
		(void) headerSetDigest(h, digest);
		digest = _free(digest);
	    }
	}

	(void) Fclose(fd);

	switch (rpmrc) {
	case RPMRC_NOTFOUND:
	case RPMRC_FAIL:
	default:
	    (void) headerFree(h);
	    h = NULL;
	    break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    if (repo->baseurl)
		(void) headerSetBaseURL(h, repo->baseurl);
	    (void) headerSetInstance(h, (uint32_t)repo->current+1);
	    break;
	}
    }
    return h;
}

/**
 * Return header query.
 * @param h		header
 * @param qfmt		query format
 * @return		query format result
 */
static const char * rfileHeaderSprintf(Header h, const char * qfmt)
	/*@globals fileSystem @*/
	/*@modifies h, fileSystem @*/
{
    const char * msg = NULL;
    const char * s = headerSprintf(h, qfmt, NULL, NULL, &msg);
    if (s == NULL)
	rpmrepoError(1, _("headerSprintf(%s): %s"), qfmt, msg);
assert(s != NULL);
    return s;
}

#if defined(WITH_SQLITE)
/**
 * Return header query, with "XXX" replaced by rpmdb header instance.
 * @param h		header
 * @param qfmt		query format
 * @return		query format result
 */
static const char * rfileHeaderSprintfHack(Header h, const char * qfmt)
	/*@globals fileSystem @*/
	/*@modifies h, fileSystem @*/
{
    static const char mark[] = "'XXX'";
    static size_t nmark = sizeof("'XXX'") - 1;
    const char * msg = NULL;
    char * s = (char *) headerSprintf(h, qfmt, NULL, NULL, &msg);
    char * f, * fe;
    int nsubs = 0;

    if (s == NULL)
	rpmrepoError(1, _("headerSprintf(%s): %s"), qfmt, msg);
assert(s != NULL);

    /* XXX Find & replace 'XXX' with '%{DBINSTANCE}' the hard way. */
/*@-nullptrarith@*/
    for (f = s; *f != '\0' && (fe = strstr(f, "'XXX'")) != NULL; fe += nmark, f = fe)
	nsubs++;
/*@=nullptrarith@*/

    if (nsubs > 0) {
	char instance[64];
	int xx = snprintf(instance, sizeof(instance), "'%u'",
		(uint32_t) headerGetInstance(h));
	size_t tlen = strlen(s) + nsubs * ((int)strlen(instance) - (int)nmark);
	char * t = xmalloc(tlen + 1);
	char * te = t;

	xx = xx;
/*@-nullptrarith@*/
	for (f = s; *f != '\0' && (fe = strstr(f, mark)) != NULL; fe += nmark, f = fe) {
	    *fe = '\0';
	    te = stpcpy( stpcpy(te, f), instance);
	}
/*@=nullptrarith@*/
	if (*f != '\0')
	    te = stpcpy(te, f);
	s = _free(s);
	s = t;
    }
 
    return s;
}
#endif

/**
 * Export a single package's metadata to repository metadata file(s).
 * @param repo		repository
 * @param rfile		repository metadata file
 * @param h		header
 * @return		0 on success
 */
static int rpmrepoWriteMDFile(rpmrepo repo, rpmrfile rfile, Header h)
	/*@globals fileSystem @*/
	/*@modifies rfile, h, fileSystem @*/
{
    int rc = 0;

    if (rfile->xml_qfmt != NULL) {
	if (rpmrfileXMLWrite(rfile, rfileHeaderSprintf(h, rfile->xml_qfmt)))
	    rc = 1;
    }

#if defined(WITH_SQLITE)
    if (REPO_ISSET(DATABASE)) {
	if (rpmrfileSQLWrite(rfile, rfileHeaderSprintfHack(h, rfile->sql_qfmt)))
	    rc = 1;
    }
#endif

    return rc;
}

/**
 * Export all package metadata to repository metadata file(s).
 * @param repo		repository
 * @return		0 on success
 */
static int repoWriteMetadataDocs(rpmrepo repo)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies repo, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char ** pkglist = repo->pkglist;
    const char * pkg;
    int rc = 0;

    if (pkglist)
    while ((pkg = *pkglist++) != NULL) {
	Header h = rpmrepoReadHeader(repo, pkg);

	repo->current++;

	/* XXX repoReadHeader() displays error. Continuing is foolish */
	if (h == NULL) {
	    rc = 1;
	    break;
	}

#ifdef	REFERENCE
	/* XXX todo: rpmGetPath(mydir, "/", filematrix[mydir], NULL); */
	reldir = (pkgpath != NULL ? pkgpath : rpmGetPath(repo->basedir, "/", repo->directories[0], NULL));
	self.primaryfile.write(po.do_primary_xml_dump(reldir, baseurl=repo->baseurl))
	self.flfile.write(po.do_filelists_xml_dump())
	self.otherfile.write(po.do_other_xml_dump())
#endif

	if (rpmrepoWriteMDFile(repo, &repo->primary, h)
	 || rpmrepoWriteMDFile(repo, &repo->filelists, h)
	 || rpmrepoWriteMDFile(repo, &repo->other, h))
	    rc = 1;

	(void) headerFree(h);
	h = NULL;
	if (rc) break;

	if (!repo->quiet) {
	    if (repo->verbose)
		rpmrepoError(0, "%d/%d - %s", repo->current, repo->pkgcount, pkg);
	    else
		rpmrepoProgress(repo, pkg, repo->current, repo->pkgcount);
	}
    }
    return rc;
}

int rpmrepoDoPkgMetadata(rpmrepo repo)
{
    int rc = 0;

    repo->current = 0;

#ifdef	REFERENCE
    def _getFragmentUrl(self, url, fragment):
        import urlparse
        urlparse.uses_fragment.append('media')
        if not url:
            return url
        (scheme, netloc, path, query, fragid) = urlparse.urlsplit(url)
        return urlparse.urlunsplit((scheme, netloc, path, query, str(fragment)))

    def doPkgMetadata(self):
        """all the heavy lifting for the package metadata"""
        if (argvCount(repo->directories) == 1) {
            MetaDataGenerator.doPkgMetadata(self)
            return
	}

    ARGV_t roots = NULL;
    filematrix = {}
    for mydir in repo->directories {
	if (mydir[0] == '/')
	    thisdir = xstrdup(mydir);
	else if (mydir[0] == '.' && mydir[1] == '.' && mydir[2] == '/')
	    thisdir = Realpath(mydir, NULL);
	else
	    thisdir = rpmGetPath(repo->basedir, "/", mydir, NULL);

	xx = argvAdd(&roots, thisdir);
	thisdir = _free(thisdir);

	filematrix[mydir] = rpmrepoGetFileList(repo, roots, '.rpm')
	self.trimRpms(filematrix[mydir])
	repo->pkgcount = argvCount(filematrix[mydir]);
	roots = argvFree(roots);
    }

    mediano = 1;
    repo->baseurl = self._getFragmentUrl(repo->baseurl, mediano)
#endif

    if (rpmrepoOpenMDFile(repo, &repo->primary)
     || rpmrepoOpenMDFile(repo, &repo->filelists)
     || rpmrepoOpenMDFile(repo, &repo->other))
	rc = 1;
    if (rc) return rc;

#ifdef	REFERENCE
    for mydir in repo->directories {
	repo->baseurl = self._getFragmentUrl(repo->baseurl, mediano)
	/* XXX todo: rpmGetPath(mydir, "/", filematrix[mydir], NULL); */
	if (repoWriteMetadataDocs(repo, filematrix[mydir]))
	    rc = 1;
	mediano++;
    }
    repo->baseurl = self._getFragmentUrl(repo->baseurl, 1)
#endif

    if (repoWriteMetadataDocs(repo))
	rc = 1;

    if (!repo->quiet)
	fprintf(stderr, "\n");
    if (rpmrepoCloseMDFile(repo, &repo->primary)
     || rpmrepoCloseMDFile(repo, &repo->filelists)
     || rpmrepoCloseMDFile(repo, &repo->other))
	rc = 1;

    return rc;
}

/*==============================================================*/

/*@unchecked@*/
static int compression = -1;

/*@unchecked@*/ /*@observer@*/
static struct poptOption repoCompressionPoptTable[] = {
 { "uncompressed", '\0', POPT_ARG_VAL,		&compression, 0,
	N_("don't compress"), NULL },
 { "gzip", 'Z', POPT_ARG_VAL,			&compression, 1,
	N_("use gzip compression"), NULL },
 { "bzip2", '\0', POPT_ARG_VAL,			&compression, 2,
	N_("use bzip2 compression"), NULL },
 { "lzma", '\0', POPT_ARG_VAL,			&compression, 3,
	N_("use lzma compression"), NULL },
 { "xz", '\0', POPT_ARG_VAL,			&compression, 4,
	N_("use xz compression"), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption _rpmrepoOptions[] = {

 { "quiet", 'q', POPT_ARG_VAL,			&__repo.quiet, 0,
	N_("output nothing except for serious errors"), NULL },
 { "verbose", 'v', 0,				NULL, (int)'v',
	N_("output more debugging info."), NULL },
 { "dryrun", '\0', POPT_BIT_SET,	&__repo.flags, REPO_FLAGS_DRYRUN,
	N_("sanity check arguments, don't create metadata"), NULL },
 { "excludes", 'x', POPT_ARG_ARGV,		&__repo.exclude_patterns, 0,
	N_("glob PATTERN(s) to exclude"), N_("PATTERN") },
 { "includes", 'i', POPT_ARG_ARGV,		&__repo.include_patterns, 0,
	N_("glob PATTERN(s) to include"), N_("PATTERN") },
 { "basedir", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN,	&__repo.basedir, 0,
	N_("top level directory"), N_("DIR") },
 { "baseurl", 'u', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN,	&__repo.baseurl, 0,
	N_("baseurl to append on all files"), N_("BASEURL") },
#ifdef	NOTYET
 { "groupfile", 'g', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN,	&__repo.groupfile, 0,
	N_("path to groupfile to include in metadata"), N_("FILE") },
#endif
 { "pretty", 'p', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,		&__repo.flags, REPO_FLAGS_PRETTY,
	N_("make sure all xml generated is formatted"), NULL },
 { "checkts", 'C', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&__repo.flags, REPO_FLAGS_CHECKTS,
	N_("check timestamps on files vs the metadata to see if we need to update"), NULL },
 { "database", 'd', POPT_BIT_SET,		&__repo.flags, REPO_FLAGS_DATABASE,
	N_("create sqlite3 database files"), NULL },
 { "split", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,		&__repo.flags, REPO_FLAGS_SPLIT,
	N_("generate split media"), NULL },
 { "pkglist", 'l', POPT_ARG_ARGV|POPT_ARGFLAG_DOC_HIDDEN,	&__repo.manifests, 0,
	N_("use only the files listed in this file from the directory specified"), N_("FILE") },
 { "outputdir", 'o', POPT_ARG_STRING,		&__repo.outputdir, 0,
	N_("<dir> = optional directory to output to"), N_("DIR") },
 { "skip-symlinks", 'S', POPT_BIT_SET,		&__repo.flags, REPO_FLAGS_NOFOLLOW,
	N_("ignore symlinks of packages"), NULL },
 { "unique-md-filenames", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &__repo.flags, REPO_FLAGS_UNIQUEMDFN,
	N_("include the file's checksum in the filename, helps with proxies"), NULL },

  POPT_TABLEEND

};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmrepoOptionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, _rpmrepoOptions, 0,
	N_("Repository options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
	N_("Fts(3) traversal options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, repoCompressionPoptTable, 0,
	N_("Available compressions:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioDigestPoptTable, 0,
	N_("Available digests:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

static int rpmrepoInitPopt(rpmrepo repo, char ** av)
	/*@modifies repo @*/
{
    void *use =  repo->_item.use;
    void *pool = repo->_item.pool;
    int ac = argvCount((ARGV_t)av);
    poptContext con = rpmioInit(ac, av, rpmrepoOptionsTable);
    int rc = 0;		/* XXX assume success */
    int xx;
    int i;

    *repo = *_repo;	/* structure assignment */
    repo->_item.use = use;
    repo->_item.pool = pool;

    repo->con = con;

    /* XXX Impedanace match against poptIO common code. */
    if (rpmIsVerbose())
	repo->verbose++;
    if (rpmIsDebug())
	repo->verbose++;

    repo->ftsoptions = (rpmioFtsOpts ? rpmioFtsOpts : FTS_PHYSICAL);
    switch (repo->ftsoptions & (FTS_LOGICAL|FTS_PHYSICAL)) {
    case (FTS_LOGICAL|FTS_PHYSICAL):
	rpmrepoError(1, "FTS_LOGICAL and FTS_PYSICAL are mutually exclusive");
        /*@notreached@*/ break;
    case 0:
        repo->ftsoptions |= FTS_PHYSICAL;
        break;
    }

    repo->algo = (rpmioDigestHashAlgo >= 0
		? (rpmioDigestHashAlgo & 0xff)  : PGPHASHALGO_SHA1);

    repo->compression = (compression >= 0 ? compression : 1);
    switch (repo->compression) {
    case 0:
	repo->suffix = NULL;
	repo->wmode = "w.ufdio";
	break;
    default:
	/*@fallthrough@*/
    case 1:
	repo->suffix = ".gz";
	repo->wmode = "w9.gzdio";
	break;
    case 2:
	repo->suffix = ".bz2";
	repo->wmode = "w9.bzdio";
	break;
    case 3:
	repo->suffix = ".lzma";
	repo->wmode = "w.lzdio";
	break;
    case 4:
	repo->suffix = ".xz";
	repo->wmode = "w.xzdio";
	break;
    }

    repo->av = poptGetArgs(repo->con);

    if (repo->av == NULL || repo->av[0] == NULL)
	rpmrepoError(1, _("Must specify path(s) to index."));

    if (repo->av != NULL)
    for (i = 0; repo->av[i] != NULL; i++) {
	char fullpath[MAXPATHLEN];
	struct stat sb;
	const char * rpath;
	const char * lpath = NULL;
	int ut = urlPath(repo->av[i], &lpath);
	size_t nb = (size_t)(lpath - repo->av[i]);
	int isdir = (lpath[strlen(lpath)-1] == '/');
	
	/* Convert to absolute/clean/malloc'd path. */
	if (lpath[0] != '/') {
	    if ((rpath = rpmrepoRealpath(lpath)) == NULL)
		rpmrepoError(1, _("Realpath(%s): %s"), lpath, strerror(errno));
	    lpath = rpmGetPath(rpath, NULL);
	    rpath = _free(rpath);
	} else
	    lpath = rpmGetPath(lpath, NULL);

	/* Reattach the URI to the absolute/clean path. */
	/* XXX todo: rpmGenPath was confused by file:///path/file URI's. */
	switch (ut) {
	case URL_IS_DASH:
	case URL_IS_UNKNOWN:
	    rpath = lpath;
	    lpath = NULL;
	    /*@switchbreak@*/ break;
	default:
assert(nb < sizeof(fullpath));
	    strncpy(fullpath, repo->av[i], nb);
	    fullpath[nb] = '\0';
	    rpath = rpmGenPath(fullpath, lpath, NULL);
	    lpath = _free(lpath);
	    /*@switchbreak@*/ break;
	}

	/* Add a trailing '/' on directories. */
	lpath = (isdir || (!Stat(rpath, &sb) && S_ISDIR(sb.st_mode))
		? "/" : NULL);
	if (lpath != NULL) {
	    lpath = rpmExpand(rpath, lpath, NULL);
	    xx = argvAdd(&repo->directories, lpath);
	    lpath = _free(lpath);
	} else {
	    xx = argvAdd(&repo->pkglist, rpath);
	}
	rpath = _free(rpath);
    }

    return rc;
}

static void rpmrepoFini(void * _repo)
	/*@globals fileSystem @*/
	/*@modifies *_repo, fileSystem @*/
{
    rpmrepo repo = _repo;

    repo->primary.digest = _free(repo->primary.digest);
    repo->primary.Zdigest = _free(repo->primary.Zdigest);
    repo->filelists.digest = _free(repo->filelists.digest);
    repo->filelists.Zdigest = _free(repo->filelists.Zdigest);
    repo->other.digest = _free(repo->other.digest);
    repo->other.Zdigest = _free(repo->other.Zdigest);
    repo->repomd.digest = _free(repo->repomd.digest);
    repo->repomd.Zdigest = _free(repo->repomd.Zdigest);
    repo->outputdir = _free(repo->outputdir);
    repo->pkglist = argvFree(repo->pkglist);
    repo->directories = argvFree(repo->directories);
    repo->manifests = argvFree(repo->manifests);
/*@-onlytrans -refcounttrans @*/
    repo->excludeMire = mireFreeAll(repo->excludeMire, repo->nexcludes);
    repo->includeMire = mireFreeAll(repo->includeMire, repo->nincludes);
/*@=onlytrans =refcounttrans @*/
    repo->exclude_patterns = argvFree(repo->exclude_patterns);
    repo->include_patterns = argvFree(repo->include_patterns);

    repo->con = poptFreeContext(repo->con);

}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmrepoPool = NULL;

static rpmrepo rpmrepoGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmrepoPool, fileSystem @*/
	/*@modifies pool, _rpmrepoPool, fileSystem @*/
{
    rpmrepo repo;

    if (_rpmrepoPool == NULL) {
	_rpmrepoPool = rpmioNewPool("repo", sizeof(*repo), -1, _rpmrepo_debug,
			NULL, NULL, rpmrepoFini);
	pool = _rpmrepoPool;
    }
    repo = (rpmrepo) rpmioGetPool(pool, sizeof(*repo));
    memset(((char *)repo)+sizeof(repo->_item), 0, sizeof(*repo)-sizeof(repo->_item));
    return repo;
}

rpmrepo rpmrepoNew(char ** av, int flags)
{
    rpmrepo repo = rpmrepoGetPool(_rpmrepoPool);
    int xx;

    xx = rpmrepoInitPopt(repo, av);

    return rpmrepoLink(repo);
}
