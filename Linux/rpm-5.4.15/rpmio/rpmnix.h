#ifndef RPMNIX_H
#define RPMNIX_H

/** \ingroup rpmio
 * \file rpmio/rpmnix.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>
#include <argv.h>
#include <popt.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmnix_s * rpmnix;

/*@unchecked@*/
extern int _rpmnix_debug;

/*@unchecked@*/
extern rpmnix _rpmnixI;

#if defined(_RPMNIX_INTERNAL)

#define F_ISSET(_nix, _FLAG) ((_nix)->flags & (RPMNIX_FLAGS_##_FLAG))

/**
 * Interpreter flags.
 */
enum rpmnixFlags_e {
    RPMNIX_FLAGS_NONE		= 0,

    /* nix-build */
    RPMNIX_FLAGS_ADDDRVLINK	= (1 <<  0),	/*    --add-drv-link */
    RPMNIX_FLAGS_NOOUTLINK	= (1 <<  1),	/*  -o,--no-out-link */
    RPMNIX_FLAGS_DRYRUN		= (1 <<  2),	/*     --dry-run */

    /* nix-collect-garbage */
    RPMNIX_FLAGS_DELETEOLD	= (1 <<  3),	/* -d,--delete-old */

    /* nix-copy-closure */
    RPMNIX_FLAGS_SIGN		= (1 <<  4),	/*    --sign */
    RPMNIX_FLAGS_GZIP		= (1 <<  5),	/*    --gzip */

    /* nix-install-package */
    RPMNIX_FLAGS_INTERACTIVE	= (1 <<  6),	/*    --non-interactive */

    /* nix-pull */
    RPMNIX_FLAGS_SKIPWRONGSTORE	= (1 <<  7),	/*    --skip-wrong-store */

    /* nix-push */
    RPMNIX_FLAGS_COPY		= (1 <<  8),	/*    --copy */

    /* nix-instantiate */
    RPMNIX_FLAGS_EVALONLY	= (1 <<  9),	/*    --eval-only */
    RPMNIX_FLAGS_PARSEONLY	= (1 << 10),	/*    --parse-only */
    RPMNIX_FLAGS_ADDROOT	= (1 << 11),	/*    --add-root */
    RPMNIX_FLAGS_INDIRECT	= (1 << 12),	/*    --indirect */
    RPMNIX_FLAGS_XML		= (1 << 13),	/*    --xml */
    RPMNIX_FLAGS_STRICT		= (1 << 14),	/*    --strict */
    RPMNIX_FLAGS_SHOWTRACE	= (1 << 15),	/*    --show-trace */

    /* nix-hash */
    RPMNIX_FLAGS_FLAT		= (1 << 16),	/*    --flat */
    RPMNIX_FLAGS_BASE32		= (1 << 17),	/*    --base32 */
    RPMNIX_FLAGS_TRUNCATE	= (1 << 18),	/*    --truncate */
    RPMNIX_FLAGS_TOBASE16	= (1 << 19),	/*    --tobase16 */
    RPMNIX_FLAGS_TOBASE32	= (1 << 20),	/*    --tobase32 */

};

enum rpmnixQF_e {
    RPMNIX_QF_OUTPUTS		= (1 <<  1),	/*    --outputs */
    RPMNIX_QF_REQUISITES	= (1 <<  2),	/*    --requisites */
    RPMNIX_QF_REFERENCES	= (1 <<  3),	/*    --references */
    RPMNIX_QF_REFERRERS		= (1 <<  4),	/*    --referrers */
    RPMNIX_QF_REFERRERS_CLOSURE	= (1 <<  5),	/*    --referrers-closure */
    RPMNIX_QF_TREE		= (1 <<  6),	/*    --tree */
    RPMNIX_QF_GRAPH		= (1 <<  7),	/*    --graph */
    RPMNIX_QF_HASH		= (1 <<  8),	/*    --hash */
    RPMNIX_QF_ROOTS		= (1 <<  9),	/*    --roots */
    RPMNIX_QF_USE_OUTPUT	= (1 << 10),	/*    --use-output */
    RPMNIX_QF_FORCE_REALISE	= (1 << 11),	/*    --force-realise */
    RPMNIX_QF_INCLUDE_OUTPUTS	= (1 << 12),	/*    --include-outputs */
};

enum rpmnixGC_e {
    RPMNIX_GC_PRINT_ROOTS	= (1 <<  1),	/*    --print-roots */
    RPMNIX_GC_PRINT_LIVE	= (1 <<  2),	/*    --print-live */
    RPMNIX_GC_PRINT_DEAD	= (1 <<  3),	/*    --print-dead */
    RPMNIX_GC_DELETE		= (1 <<  4),	/*    --delete */
};

struct rpmnix_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    uint32_t flags;		/*!< control bits */
    poptContext con;		/*!< parsing state */
    const char ** av;		/*!< arguments */
#ifdef	NOTYET
    void * I;
#endif

    /* common environment */
/*@observer@*/
    const char * tmpDir;	/* TMPDIR */
/*@observer@*/
    const char * homeDir;	/* HOME */
/*@observer@*/
    const char * binDir;	/* NIX_BIN_DIR */
/*@observer@*/
    const char * dataDir;	/* NIX_DATA_DIR */
/*@observer@*/
    const char * libexecDir;	/* NIX_LIBEXEC_DIR */

    const char * storeDir;	/* NIX_STORE_DIR */
    const char * stateDir;	/* NIX_STATE_DIR */

    /* common options */
    int op;
    const char * url;
/*@observer@*/
    const char * hashAlgo;
    const char * hashAlgoName;	/* XXX nix-hash specific option value for now. */

    const char * tmpPath;
    const char * manifestsPath;	/* NIX_MANIFESTS_DIR */
    const char * rootsPath;	/* XXX NIX_ROOTS_DIR? */

    const char ** storePaths;

    const char ** narFiles;
    const char ** localPaths;
    const char ** patches;

    /* nix-build */
    int verbose;
    const char * outLink;
    const char * drvLink;
    const char ** instArgs;
    const char ** buildArgs;
    const char ** exprs;

    /* nix-channel */
    const char * channelsList;
    const char * channelCache;
    const char * nixDefExpr;
    const char ** channels;

    /* nix-collect-garbage */
    const char * profilesPath;	/* XXX NIX_PROFILES_DIR? */

    /* nix-copy-closure */
    const char * sshHost;
    const char ** allStorePaths;
    const char ** missing;

    /* nix-install-package */
    const char ** profiles;

    /* nix-instantiate */
    const char * attr;		/* XXX POPT_ARG_ARGV instead? */
#ifdef	NOTYET
    const char * logType;
#endif

    /* nix-prefetch-url */
    int quiet;			/* QUIET */
    int print;			/* PRINT_PATHS */
    const char * nixPkgs;
    const char * downloadCache;
    const char * expHash;
    const char * hashFormat;
/*@only@*/
    const char * hash;
/*@only@*/
    const char * finalPath;
/*@only@*/
    const char * tmpFile;
/*@only@*/
    const char * name;
    const char * cacheFlags;
    const char * cachedHashFN;
    const char * cachedTimestampFN;
    const char * urlHash;

    /* nix-push */
    const char * localArchivesDir;
    const char * localManifestFile;
    const char * targetArchivesUrl;
    const char * archivesPutURL;
    const char * archivesGetURL;
    const char * manifestPutURL;
    const char ** storeExprs;
    const char ** narArchives;
    const char ** narPaths;
    const char * curl;
    const char * manifest;
    const char * nixExpr;

    /* nix-store */
    int jobs;
    enum rpmnixQF_e qf;
    enum rpmnixGC_e gc;

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/*@unchecked@*/
extern struct rpmnix_s _nix;

enum {
    NIX_CHANNEL_ADD = 1,
    NIX_CHANNEL_REMOVE,
    NIX_CHANNEL_LIST,
    NIX_CHANNEL_UPDATE,
};

enum {
    NIX_COPY_CLOSURE_FROM_HOST = 1,
    NIX_COPY_CLOSURE_TO_HOST,
};

#endif /* _RPMNIX_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_RPMNIX_INTERNAL)
#define NIXDBG(_l) if (_rpmnix_debug) fprintf _l
/*@null@*/
static inline char * _freeCmd(/*@only@*/ const char * cmd)
	/*@*/
{   
NIXDBG((stderr, "\t%s\n", cmd));
    cmd = _free(cmd);   
    return NULL;
}
#endif

/**
 * Unreference a nix interpreter instance.
 * @param nix		nix interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmnix rpmnixUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmnix nix)
	/*@modifies nix @*/;
#define	rpmnixUnlink(_nix)	\
    ((rpmnix)rpmioUnlinkPoolItem((rpmioItem)(_nix), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a nix interpreter instance.
 * @param nix		nix interpreter
 * @return		new nix interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmnix rpmnixLink (/*@null@*/ rpmnix nix)
	/*@modifies nix @*/;
#define	rpmnixLink(_nix)	\
    ((rpmnix)rpmioLinkPoolItem((rpmioItem)(_nix), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a nix interpreter.
 * @param nix		nix interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmnix rpmnixFree(/*@killref@*/ /*@null@*/rpmnix nix)
	/*@globals fileSystem @*/
	/*@modifies nix, fileSystem @*/;
#define	rpmnixFree(_nix)	\
    ((rpmnix)rpmioFreePoolItem((rpmioItem)(_nix), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a nix interpreter.
 * @param av		nix interpreter args (or NULL)
 * @param flags		nix interpreter flags
 * @param _tbl		POPT options table for option/arg parsing
 * @return		new nix interpreter
 */
/*@newref@*/ /*@null@*/
rpmnix rpmnixNew(/*@null@*/ char ** av, uint32_t flags, void * _tbl)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return arguments from a nix interpreter.
 * @param nix		nix interpreter
 * @retval *argcp	no. of arguments
 * @return		nix interpreter args
 */
/*@null@*/
const char ** rpmnixArgv(/*@null@*/ rpmnix nix, /*@null@*/ int * argcp)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Build from a set of nix expressions.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixBuild(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixBuildOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixChannel(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixChannelOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixCollectGarbage(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixCollectGarbageOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixCopyClosure(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixCopyClosureOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixEnv(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL_NOTYET)
/*@unchecked@*/
extern struct poptOption _rpmnixEnvOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixHash(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixHashOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixInstallPackage(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixInstallPackageOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixInstantiate(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixInstantiateOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixPrefetchURL(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixPrefetchUrlOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixPull(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixPullOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixPush(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixPushOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixStore(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL_NOTYET)
/*@unchecked@*/
extern struct poptOption _rpmnixStoreOptions[];
#endif

/**
 * FIXME.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixWorker(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL_NOTYET)
/*@unchecked@*/
extern struct poptOption _rpmnixWorkerOptions[];
#endif

/**
 * Print rpmnix object arguments.
 * @param nix		nix interpreter
 * @return		0 on success
 */
int rpmnixEcho(/*@null@*/ rpmnix nix)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#if defined(_RPMNIX_INTERNAL)
/*@unchecked@*/
extern struct poptOption _rpmnixEchoOptions[];
#endif

#ifdef __cplusplus
}
#endif

#endif /* RPMNIX_H */
