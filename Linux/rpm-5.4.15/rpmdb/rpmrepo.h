#ifndef	H_RPMREPO
#define	H_RPMREPO

/** \ingroup rpmio
 * \file rpmdb/rpmrepo.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>
#include <argv.h>
#include <mire.h>
#include <popt.h>

/** \ingroup rpmio
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmrepo_s * rpmrepo;
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmrfile_s * rpmrfile;

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmrepo_debug;

#if defined(_RPMREPO_INTERNAL)
/**
 * Repository metadata file.
 */
struct rpmrfile_s {
/*@observer@*/
    const char * type;
/*@observer@*/
    const char * xml_init;
/*@observer@*/ /*@relnull@*/
    const char * xml_qfmt;
/*@observer@*/
    const char * xml_fini;
/*@observer@*/
    const char ** sql_init;
/*@observer@*/
    const char * sql_qfmt;
#ifdef	NOTYET	/* XXX char **?!? */
/*@observer@*/
    const char ** sql_fini;
#endif
/*@observer@*/
    const char * yaml_init;
/*@observer@*/
    const char * yaml_qfmt;
/*@observer@*/
    const char * yaml_fini;
/*@observer@*/
    const char * Packages_init;
/*@observer@*/
    const char * Packages_qfmt;
/*@observer@*/
    const char * Packages_fini;
/*@observer@*/
    const char * Sources_init;
/*@observer@*/
    const char * Sources_qfmt;
/*@observer@*/
    const char * Sources_fini;
/*@relnull@*/
    FD_t fd;
#if defined(WITH_SQLITE)
    sqlite3 * sqldb;
#endif
/*@null@*/
    const char * digest;
/*@null@*/
    const char * Zdigest;
    time_t ctime;
};

/**
 * Repository.
 */
#define	_RFB(n)	((1U << (n)) | 0x40000000)

/**
 * Bit field enum for copy CLI options.
 */
typedef enum rpmrepoFlags_e {
    REPO_FLAGS_NONE		= 0,
    REPO_FLAGS_DRYRUN		= _RFB( 0), /*!<    --dryrun ... */
    REPO_FLAGS_PRETTY		= _RFB( 1), /*!< -p,--pretty ... */
    REPO_FLAGS_DATABASE		= _RFB( 2), /*!< -d,--database ... */
    REPO_FLAGS_CHECKTS		= _RFB( 3), /*!< -C,--checkts ... */
    REPO_FLAGS_SPLIT		= _RFB( 4), /*!<    --split ... */
    REPO_FLAGS_NOFOLLOW		= _RFB( 5), /*!< -S,--skip-symlinks ... */
    REPO_FLAGS_UNIQUEMDFN	= _RFB( 6), /*!<    --unique-md-filenames ... */

	/* 7-31 unused */
} rpmrepoFlags;

#define REPO_ISSET(_FLAG) ((repo->flags & ((REPO_FLAGS_##_FLAG) & ~0x40000000)) != REPO_FLAGS_NONE)

struct rpmrepo_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;

    rpmrepoFlags flags;		/*!< control bits */
    poptContext con;		/*!< parsing state */
    const char ** av;		/*!< arguments */

    int quiet;
    int verbose;
/*@null@*/
    ARGV_t exclude_patterns;
/*@relnull@*/
    miRE excludeMire;
    int nexcludes;
/*@null@*/
    ARGV_t include_patterns;
/*@relnull@*/
    miRE includeMire;
    int nincludes;
/*@null@*/
    const char * basedir;
/*@null@*/
    const char * baseurl;
#ifdef	NOTYET
/*@null@*/
    const char * groupfile;
#endif
/*@relnull@*/
    const char * outputdir;

/*@null@*/
    ARGV_t manifests;

/*@observer@*/ /*@relnull@*/
    const char * tempdir;
/*@observer@*/ /*@relnull@*/
    const char * finaldir;
/*@observer@*/ /*@relnull@*/
    const char * olddir;

    time_t mdtimestamp;

/*@null@*/
    void * _ts;
/*@null@*/
    ARGV_t pkglist;
    unsigned current;
    unsigned pkgcount;

/*@null@*/
    ARGV_t directories;
    int ftsoptions;
    uint32_t pkgalgo;
    uint32_t algo;
    int compression;
/*@observer@*/
    const char * markup;
/*@observer@*/ /*@null@*/
    const char * suffix;
/*@observer@*/
    const char * wmode;

    struct rpmrfile_s primary;
    struct rpmrfile_s filelists;
    struct rpmrfile_s other;
    struct rpmrfile_s repomd;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#endif	/* _RPMREPO_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a repo wrapper instance.
 * @param repo		repo wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmrepo rpmrepoUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmrepo repo)
	/*@modifies repo @*/;
#define	rpmrepoUnlink(_repo)	\
    ((rpmrepo)rpmioUnlinkPoolItem((rpmioItem)(_repo), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a repo wrapper instance.
 * @param repo		repo wrapper
 * @return		new repo wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmrepo rpmrepoLink (/*@null@*/ rpmrepo repo)
	/*@modifies repo @*/;
#define	rpmrepoLink(_repo)	\
    ((rpmrepo)rpmioLinkPoolItem((rpmioItem)(_repo), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a repo wrapper.
 * @param repo		repo wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmrepo rpmrepoFree(/*@killref@*/ /*@null@*/rpmrepo repo)
	/*@globals fileSystem @*/
	/*@modifies repo, fileSystem @*/;
#define	rpmrepoFree(_repo)	\
    ((rpmrepo)rpmioFreePoolItem((rpmioItem)(_repo), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a repo wrapper.
 * @param av		repo argv
 * @param flags		repo flags
 * @return		new repo wrapper
 */
/*@newref@*/ /*@null@*/
rpmrepo rpmrepoNew(char ** av, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Print an error message and exit (if requested).
 * @param lvl		error level (non-zero exits)
 * @param fmt		msg format
 */
/*@mayexit@*/
void rpmrepoError(int lvl, const char *fmt, ...)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 * Return realpath(3) canonicalized absolute path.
 * @param lpath		file path
 * @return		canonicalized absolute path
 */
/*@null@*/
const char * rpmrepoRealpath(const char * lpath)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Test for repository sanity.
 * @param repo		repository
 * @return		0 on success
 */
int rpmrepoTestSetupDirs(rpmrepo repo)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies repo, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Walk file/directory trees, looking for files with an extension.
 * @param repo		repository
 * @param roots		file/directory trees to search
 * @param ext		file extension to match (usually ".rpm")
 * @return		list of files with the extension
 */
/*@null@*/
const char ** rpmrepoGetFileList(rpmrepo repo, const char *roots[],
		const char * ext)
	/*@globals fileSystem, internalState @*/
	/*@modifies repo, fileSystem, internalState @*/;

/**
 * Check that repository time stamp is newer than any contained package.
 * @param repo		repository
 * @return		0 on success
 */
int rpmrepoCheckTimeStamps(rpmrepo repo)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Write repository metadata files.
 * @param repo		repository
 * @return		0 on success
 */
int rpmrepoDoPkgMetadata(rpmrepo repo)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies repo, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Write repository manifest.
 * @param repo		repository
 * @return		0 on success.
 */
int rpmrepoDoRepoMetadata(rpmrepo repo)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies repo, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Rename temporary repository to final paths.
 * @param repo		repository
 * @return		0 always
 */
int rpmrepoDoFinalMove(rpmrepo repo)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMREPO */
