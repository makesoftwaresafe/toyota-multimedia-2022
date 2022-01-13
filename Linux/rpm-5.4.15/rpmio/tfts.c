#include "system.h"

#include <fts.h>
#include <rpmdir.h>
#include <rpmdav.h>
#include <poptIO.h>

#include <magic.h>

#include "debug.h"

/**
 */
typedef struct rpmfts_s * rpmfts;

/**
 */
struct rpmfts_s {
    FTS * t;
    FTSENT * p;
    struct stat sb;

    ARGV_t paths;
    int ftsoptions;

    int ndirs;
    int nfiles;

    int mireMode;
    int mireTag;
    const char * mirePattern;
    miRE mire;
    int mireNExecs;
    int mireNMatches;
    int mireNFails;

    const char * mgFile;
    int mgFlags;
    rpmmg mg;
    int mgNFiles;
    int mgNMatches;
    int mgNFails;
};

/*@unchecked@*/
static struct rpmfts_s __rpmfts = {
    .mireMode	= RPMMIRE_REGEX,
    .mgFlags	= MAGIC_CHECK | MAGIC_MIME,
};

/*@unchecked@*/
static rpmfts _rpmfts = &__rpmfts;

static struct e_s {
    unsigned int total;
    unsigned int retries;
    unsigned int timedout;
    unsigned int again;
    unsigned int unknown;
} e;

#ifdef	DYING
extern int _dav_nooptions;
#endif

static int indent = 2;

static const char * ftsInfoStrings[] = {
    "UNKNOWN",
    "D",
    "DC",
    "DEFAULT",
    "DNR",
    "DOT",
    "DP",
    "ERR",
    "F",
    "INIT",
    "NS",
    "NSOK",
    "SL",
    "SLNONE",
    "W",
};

static const char * ftsInfoStr(int fts_info)
{
    if (!(fts_info >= 1 && fts_info <= 14))
	fts_info = 0;
    return ftsInfoStrings[ fts_info ];
}

static int ftsPrint(rpmfts fts)
{
    int xx;

    if (rpmIsDebug())
	fprintf(stderr, "FTS_%s\t%*s%s\n", ftsInfoStr(fts->p->fts_info),
		indent * (fts->p->fts_level < 0 ? 0 : fts->p->fts_level), "",
		(fts->p->fts_level > 0 ? fts->p->fts_name : fts->p->fts_accpath));

    switch (fts->p->fts_info) {
    case FTS_D:		/* preorder directory */
	fts->ndirs++;
	if (rpmIsVerbose() && !rpmIsDebug()) {
	    size_t nb = strlen(fts->p->fts_path);
	    /* Add trailing '/' if not present. */
	    fprintf(stderr, "%s%s\n", fts->p->fts_path,
		(fts->p->fts_path[nb-1] != '/' ? "/" : ""));
	}
	break;
    case FTS_DP:	/* postorder directory */
	break;
    case FTS_F:		/* regular file */
	fts->nfiles++;
	if (rpmIsVerbose() && !rpmIsDebug())
	    fprintf(stderr, "%s\n", fts->p->fts_path);
	if (fts->mire) {
	    fts->mireNExecs++;
	    xx = mireRegexec(fts->mire, fts->p->fts_path, 0);
	    if (xx >= 0) {
		fprintf(stdout, " mire:\t%*s%s\n",
			indent * (fts->p->fts_level < 0
				? 0 : fts->p->fts_level), "",
			fts->p->fts_path);
		fflush(stdout);
		fts->mireNMatches++;
	    } else {
		fts->mireNFails++;
	    }
	}
	if (fts->mg) {
	    const char * s = rpmmgFile(fts->mg, fts->p->fts_accpath);
	    fts->mgNFiles++;
	    if (s != NULL && *s != '\0') {
		fprintf(stdout, "magic:\t%*s%s\n",
			indent * (fts->p->fts_level < 0
				? 0 : fts->p->fts_level), "",
			s);
		fflush(stdout);
		fts->mgNMatches++;
	    } else {
		fts->mgNFails++;
	    }
	    s = _free(s);
	}
	break;
    case FTS_NS:	/* stat(2) failed */
    case FTS_DNR:	/* unreadable directory */
    case FTS_ERR:	/* error; errno is set */
	/* XXX Attempt a retry on network errors. */
	if (fts->p->fts_number == 0) {
	    (void) Fts_set(fts->t, fts->p, FTS_AGAIN);
	    fts->p->fts_number++;
	    e.retries++;
	    break;
	}
	fprintf(stderr, "error: %s: %s\n", fts->p->fts_accpath,
		strerror(fts->p->fts_errno));
	e.total++;
	switch (fts->p->fts_errno) {
	case ETIMEDOUT:		e.timedout++;	/*@innerbreak@*/ break;
	case EAGAIN:		e.again++;	/*@innerbreak@*/ break;
	default:		e.unknown++;	/*@innerbreak@*/ break;
	}
	break;
    case FTS_DC:	/* directory that causes cycles */
    case FTS_DEFAULT:	/* none of the above */
    case FTS_DOT:	/* dot or dot-dot */
    case FTS_INIT:	/* initialized only */
    case FTS_NSOK:	/* no stat(2) requested */
    case FTS_SL:	/* symbolic link */
    case FTS_SLNONE:	/* symbolic link without target */
    case FTS_W:		/* whiteout object */
    default:
	break;
    }

    return 0;
}

static int ftsWalk(rpmfts fts)
{
    rpmop op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
    int rc = 1;
    int xx;

    fts->ndirs = 0;
    fts->nfiles = 0;
    xx = rpmswEnter(op, 0);
    fts->t = Fts_open((char *const *)fts->paths, fts->ftsoptions, NULL);
    if (fts->t == NULL)
	goto exit;
    while((fts->p = Fts_read(fts->t)) != NULL)
	xx = ftsPrint(fts);
    rc = Fts_close(fts->t);

exit:
    xx = rpmswExit(op, fts->ndirs);

fprintf(stderr, "===== (%d/%d) dirs/files, errors(%u)/retries(%u) timedout(%u)/again(%u)/unknown(%u) in:\n", fts->ndirs, fts->nfiles, e.total, e.retries, e.timedout, e.again, e.unknown);
    argvPrint(NULL, fts->paths, NULL);
    if (_rpmsw_stats)
	rpmswPrint("fts:", op, NULL);

    return rc;
}


/**
 * Use absolute paths since Chdir(2) is problematic with remote URI's.
 */
static int ftsAbsPaths(rpmfts fts)
{
    struct stat * st = &fts->sb;
    char fullpath[MAXPATHLEN];
    int i;

    if (fts->paths)
    for (i = 0; fts->paths[i] != NULL; i++) {
	const char * rpath;
	const char * lpath = NULL;
	int ut = urlPath(fts->paths[i], &lpath);
	size_t nb = (size_t)(lpath - fts->paths[i]);
	int isdir = (lpath[strlen(lpath)-1] == '/');
	
	/* Convert to absolute/clean/malloc'd path. */
	if (lpath[0] != '/') {
	    /* XXX GLIBC: realpath(path, NULL) return malloc'd */
	    rpath = Realpath(lpath, NULL);
	    if (rpath == NULL)
		rpath = Realpath(lpath, fullpath);
assert(rpath != NULL);
	    lpath = rpmGetPath(rpath, NULL);
	    if (rpath != fullpath)	/* XXX GLIBC extension malloc's */
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
	    strncpy(fullpath, fts->paths[i], nb);
	    fullpath[nb] = '\0';
	    rpath = rpmGenPath(fullpath, lpath, NULL);
	    lpath = _free(lpath);
	    /*@switchbreak@*/ break;
	}

	/* Add a trailing '/' on directories. */
	lpath = (isdir || (!Stat(rpath, st) && S_ISDIR(st->st_mode))
		? "/" : NULL);
	fts->paths[i] = _free(fts->paths[i]);
	fts->paths[i] = rpmExpand(rpath, lpath, NULL);
	rpath = _free(rpath);
    }

    return 0;
}

static struct poptOption optionsTable[] = {
 { "pattern", '\0', POPT_ARG_STRING,	&__rpmfts.mirePattern, 0,
	NULL, NULL },
 { "magic", '\0', POPT_ARG_STRING,	&__rpmfts.mgFile, 0,
	NULL, NULL },

#ifdef	DYING
 { "options", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_dav_nooptions, 0,
	N_("always send http OPTIONS"), NULL},
 { "nooptions", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_dav_nooptions, -1,
	N_("use cached http OPTIONS"), NULL},
#endif

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
	N_("Options for Fts(3):"),
	NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    rpmfts fts = _rpmfts;
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    int ac = 0;
    ARGV_t dav;
    const char * dn;
    int rc;
    int xx;

    /* Process options. */

    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
    fts->ftsoptions = rpmioFtsOpts;

    if (__debug) {
_av_debug = -1;
_dav_debug = -1;
_ftp_debug = -1;
_fts_debug = -1;
_url_debug = -1;
_rpmio_debug = -1;
_rpmmg_debug = 1;
_mire_debug = 1;
    }

    dav = poptGetArgs(optCon);
    ac = argvCount(dav);
    if (ac < 1) {
	poptPrintUsage(optCon, stderr, 0);
	rc = 1;
	goto exit;
    }

    if (fts->mirePattern) {
	fts->mire = mireNew(fts->mireMode, fts->mireTag);
	if ((rc = mireRegcomp(fts->mire, fts->mirePattern)) != 0)
	    goto exit;
    }

    if (fts->mgFile) {
	fts->mg = rpmmgNew(fts->mgFile, fts->mgFlags);
    }

    /* XXX Add pesky trailing '/' to http:// URI's */
    while ((dn = *dav++) != NULL)
	xx = argvAdd(&fts->paths, dn);
    xx = ftsAbsPaths(fts);

    rc = ftsWalk(fts);

exit:
    fts->mg = rpmmgFree(fts->mg);
    fts->mire = mireFree(fts->mire);

    fts->paths = argvFree(fts->paths);

    optCon = rpmioFini(optCon);

    return rc;
}
