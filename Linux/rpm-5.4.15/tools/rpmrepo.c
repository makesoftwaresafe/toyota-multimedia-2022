/**
 * \file tools/rpmrepo.c
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
#include <rpmdir.h>
#include <fts.h>
#include <poptIO.h>

#define _RPMREPO_INTERNAL
#include <rpmrepo.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>
#include <rpmts.h>

#include "debug.h"

#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

extern poptContext
rpmcliFini(/*@only@*/ /*@null@*/ poptContext optCon);

/*==============================================================*/

int
main(int argc, char *argv[])
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmrepo repo;
    int rc = 1;		/* assume failure. */
    int xx;

#if !defined(__LCLINT__)	/* XXX force "rpmrepo" name. */
    __progname = "rpmrepo";
#endif
    repo = rpmrepoNew(argv, 0);
    if (repo == NULL)
	goto exit;

if (_rpmrepo_debug || REPO_ISSET(DRYRUN))
argvPrint("repo->directories", repo->directories, NULL);

#ifdef	NOTYET
    if (repo->basedir == NULL)
	repo->basedir = xstrdup(repo->directories[0]);
#endif

    if (repo->outputdir == NULL) {
	if (repo->directories != NULL && repo->directories[0] != NULL)
	    repo->outputdir = xstrdup(repo->directories[0]);
	else {
	    repo->outputdir = rpmrepoRealpath(".");
	    if (repo->outputdir == NULL)
		rpmrepoError(1, _("Realpath(%s): %s"), ".", strerror(errno));
	}
    }

    if (REPO_ISSET(SPLIT) && REPO_ISSET(CHECKTS))
	rpmrepoError(1, _("--split and --checkts options are mutually exclusive"));

#ifdef	NOTYET
    /* Add manifest(s) contents to rpm list. */
    if (repo->manifests != NULL) {
	const char ** av = repo->manifests;
	const char * fn;
	/* Load the rpm list from manifest(s). */
	while ((fn = *av++) != NULL) {
	    /* XXX todo: parse paths from files. */
	    /* XXX todo: convert to absolute paths. */
	    /* XXX todo: check for existence. */
	    xx = argvAdd(&repo->pkglist, fn);
	}
    }
#endif

    /* Set up mire patterns (no error returns with globs, easy pie). */
    if (mireLoadPatterns(RPMMIRE_GLOB, 0, repo->exclude_patterns, NULL,
                &repo->excludeMire, &repo->nexcludes))
	rpmrepoError(1, _("Error loading exclude glob patterns."));
    if (mireLoadPatterns(RPMMIRE_GLOB, 0, repo->include_patterns, NULL,
                &repo->includeMire, &repo->nincludes))
	rpmrepoError(1, _("Error loading include glob patterns."));

    /* Load the rpm list from a multi-rooted directory traversal. */
    if (repo->directories != NULL) {
	ARGV_t pkglist = rpmrepoGetFileList(repo, repo->directories, ".rpm");
	xx = argvAppend(&repo->pkglist, pkglist);
	pkglist = argvFree(pkglist);
    }

    /* XXX todo: check for duplicates in repo->pkglist? */
    xx = argvSort(repo->pkglist, NULL);

if (_rpmrepo_debug || REPO_ISSET(DRYRUN))
argvPrint("repo->pkglist", repo->pkglist, NULL);

    repo->pkgcount = argvCount(repo->pkglist);

    /* XXX enable --stats using transaction set. */
    {	rpmts ts = repo->_ts;
	rpmVSFlags vsflags;
	_rpmts_stats = _rpmsw_stats;
	repo->_ts = ts = rpmtsCreate();

	vsflags = (rpmVSFlags) 0; /* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
    /* XXX todo wire up usual rpm CLI options. hotwire --nosignature for now */
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    	VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif
	(void) rpmtsSetVSFlags(ts, vsflags);
    }

    rc = rpmrepoTestSetupDirs(repo);
	
    if (rc || REPO_ISSET(DRYRUN))
	goto exit;

    if (!REPO_ISSET(SPLIT)) {
	rc = rpmrepoCheckTimeStamps(repo);
	if (rc == 0) {
	    fprintf(stdout, _("repo is up to date\n"));
	    goto exit;
	}
    }

    if ((rc = rpmrepoDoPkgMetadata(repo)) != 0)
	goto exit;
    if ((rc = rpmrepoDoRepoMetadata(repo)) != 0)
	goto exit;
    if ((rc = rpmrepoDoFinalMove(repo)) != 0)
	goto exit;

exit:
    {	rpmts ts = repo->_ts;
	(void) rpmtsFree(ts); 
	repo->_ts = NULL;
    }

    repo = rpmrepoFree(repo);

    (void) rpmcliFini(NULL);

    return rc;
}
