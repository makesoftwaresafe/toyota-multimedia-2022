#ifndef	H_RPMZ
#define	H_RPMZ

/**
 * \file rpmio/rpmz.h
 * RPM parallel compressor internals.
 */

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <argv.h>

#include <rpmsq.h>
/*@-bufferoverflowhigh -mustfreeonly@*/
#include <poptIO.h>
/*@=bufferoverflowhigh =mustfreeonly@*/
#if !defined(POPT_ARGFLAG_TOGGLE)	/* XXX compat with popt < 1.15 */
#define	POPT_ARGFLAG_TOGGLE	0
#endif

#include "rpmzlog.h"

#include "rpmzq.h"

/**
 */
typedef struct rpmz_s * rpmz;

#if defined(_RPMZ_INTERNAL)

/*@unchecked@*/
extern struct rpmz_s __rpmz;

/*@unchecked@*/
extern rpmz _rpmz;

/**
 */
struct rpmz_s {
/*@observer@*/
    const char *stdin_fn;	/*!< Display name for stdin. */
/*@observer@*/
    const char *stdout_fn;	/*!< Display name for stdout. */

/*@null@*/
    ARGV_t argv;		/*!< Files to process. */
/*@null@*/
    ARGV_t manifests;		/*!< Manifests to process. */
/*@null@*/
    const char * base_prefix;	/*!< Prefix for file manifests paths. */

    char _ifn[PATH_MAX+1];	/*!< input file name (accommodate recursion) */

#if defined(_RPMZ_INTERNAL_XZ)
    rpmuint64_t mem;		/*!< Physical memory. */
    rpmuint64_t memlimit_encoder;
    rpmuint64_t memlimit_decoder;
    rpmuint64_t memlimit_custom;

/*@relnull@*/
    rpmiob iob;			/*!< Buffer for I/O. */
    size_t nb;			/*!< Buffer size (in bytes) */

/*@null@*/
    const char * isuffix;
    enum rpmzFormat_e ifmt;
    FDIO_t idio;
    char ifmode[32];
/*@null@*/
    FD_t ifd;
    struct stat isb;

/*@null@*/
    const char * osuffix;
    enum rpmzFormat_e ofmt;
    FDIO_t odio;
    char ofmode[32];
/*@null@*/
    FD_t ofd;
    struct stat osb;

    int _auto_adjust;
    enum rpmzFormat_e _format_compress_auto;
    lzma_options_lzma _options;
    lzma_check _checksum;
    lzma_filter _filters[LZMA_FILTERS_MAX + 1];
    size_t _filters_count;
#endif	/* _RPMZ_INTERNAL_XZ */

/* ++++ rpmzq */
    struct rpmzQueue_s _zq;
/*@owned@*/ /*@relnull@*/
    rpmzQueue zq;               /*!< buffer pools. */
/* ---- rpmzq */

};

#endif	/* _RPMZ_INTERNAL */

/*==============================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_RPMZ_INTERNAL)
/**
 */
static int rpmzLoadManifests(rpmz z)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies z, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    ARGV_t manifests;
    const char * fn;
    int rc = 0;	/* assume success */

    if ((manifests = z->manifests) != NULL)	/* note rc=0 return with no files to load. */
    while ((fn = *manifests++) != NULL) {
	unsigned lineno;
	char * be = NULL;
	rpmiob iob = NULL;
	int xx = rpmiobSlurp(fn, &iob);
	char * f;
	char * fe;

	if (!(xx == 0 && iob != NULL)) {
	    fprintf(stderr, _("%s: Failed to open %s\n"), __progname, fn);
	    rc = -1;
	    goto bottom;
	}

	be = (char *)(iob->b + iob->blen);
	while (be > (char *)iob->b && (be[-1] == '\n' || be[-1] == '\r')) {
	  be--;
	  *be = '\0';
	}

	/* Parse and save manifest items. */
	lineno = 0;
	for (f = (char *)iob->b; *f; f = fe) {
	    const char * path;
	    char * g, * ge;
	    lineno++;

	    fe = f;
	    while (*fe && !(*fe == '\n' || *fe == '\r'))
		fe++;
	    g = f;
	    ge = fe;
	    while (*fe && (*fe == '\n' || *fe == '\r'))
		*fe++ = '\0';

	    while (*g && xisspace((int)*g))
		*g++ = '\0';
	    /* Skip comment lines. */
	    if (*g == '#')
		/*@innercontinue@*/ continue;

	    while (ge > g && xisspace(ge[-1]))
		*--ge = '\0';
	    /* Skip empty lines. */
	    if (ge == g)
		/*@innercontinue@*/ continue;

	    /* Prepend z->base_prefix if specified. */
	    if (z->base_prefix)
		path = rpmExpand(z->base_prefix, g, NULL);
	    else
		path = rpmExpand(g, NULL);
	    (void) argvAdd(&z->argv, path);
	    path = _free(path);
	}

bottom:
	iob = rpmiobFree(iob);
	if (rc != 0)
	    goto exit;
    }

exit:
    return rc;
}

/**
 */
static rpmRC rpmzParseEnv(/*@unused@*/ rpmz z, /*@null@*/ const char * envvar,
		struct poptOption optionsTable[])
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    static char whitespace[] = " \f\n\r\t\v,";
    static char _envvar[] = "RPMZ";
    char * s = getenv((envvar ? envvar : _envvar));
    ARGV_t av = NULL;
    poptContext optCon = NULL;
    rpmRC rc = RPMRC_OK;
    int ac;
    int xx;

    if (s == NULL)
	goto exit;

    /* XXX todo: argvSplit() assumes single separator between args. */
/*@-nullstate@*/
    xx = argvSplit(&av, s, whitespace);
/*@=nullstate@*/
    ac = argvCount(av);
    if (ac < 1)
	goto exit;

    optCon = poptGetContext(__progname, ac, (const char **)av,
		optionsTable, POPT_CONTEXT_KEEP_FIRST);

    /* Process all options, whine if unknown. */
    while ((xx = poptGetNextOpt(optCon)) > 0) {
	const char * optArg = poptGetOptArg(optCon);
/*@-dependenttrans -modobserver -observertrans @*/
	optArg = _free(optArg);
/*@=dependenttrans =modobserver =observertrans @*/
	switch (xx) {
	default:
/*@-nullpass@*/
	    fprintf(stderr, _("%s: option table misconfigured (%d)\n"),
		__progname, xx);
/*@=nullpass@*/
	    rc = RPMRC_FAIL;
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}
    }

    if (xx < -1) {
/*@-nullpass@*/
	fprintf(stderr, "%s: %s: %s\n", __progname,
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		poptStrerror(xx));
/*@=nullpass@*/
	rc = RPMRC_FAIL;
    } else	/* Check no args were in the envvar. */
    if (argvCount(poptGetArgs(optCon))) {
	fprintf(stderr, "%s: cannot provide files in %s envvar\n",
		 __progname, (envvar ? envvar : _envvar));
	rc = RPMRC_FAIL;
    }

exit:
    if (optCon)
	optCon = poptFreeContext(optCon);
    av = argvFree(av);
    return rc;
}
#endif	/* _RPMZ_INTERNAL */

#if defined(_RPMZ_INTERNAL_XZ)
/**
 */
static rpmRC rpmzParseArgv0(rpmz z, const char * argv0)
	/*@globals fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;
    const char * s = strrchr(argv0, '/');
    const char * name = (s ? (s + 1) : argv0);
    rpmRC rc = RPMRC_OK;

#if defined(WITH_XZ)
    if (strstr(name, "xz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_XZ;
	zq->format = RPMZ_FORMAT_XZ;	/* XXX eliminate */
    } else
    if (strstr(name, "lz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_LZMA;
	zq->format = RPMZ_FORMAT_LZMA;	/* XXX eliminate */
    } else
#endif	/* WITH_XZ */
#if defined(WITH_BZIP2)
    if (strstr(name, "bz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_BZIP2;
	zq->format = RPMZ_FORMAT_BZIP2;	/* XXX eliminate */
    } else
#endif	/* WITH_BZIP2 */
#if defined(WITH_ZLIB)
    if (strstr(name, "gz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_GZIP;
	zq->format = RPMZ_FORMAT_GZIP;	/* XXX eliminate */
    } else
    if (strstr(name, "zlib") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_ZLIB;
	zq->format = RPMZ_FORMAT_ZLIB;	/* XXX eliminate */
    } else
	/* XXX watchout for "bzip2" matching */
    if (strstr(name, "zip") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_ZIP2;
	zq->format = RPMZ_FORMAT_ZIP2;	/* XXX eliminate */
    } else
#endif	/* WITH_ZLIB */
    {
	z->_format_compress_auto = RPMZ_FORMAT_AUTO;
	zq->format = RPMZ_FORMAT_AUTO;	/* XXX eliminate */
    }

    if (strstr(name, "cat") != NULL) {
	zq->mode = RPMZ_MODE_DECOMPRESS;
	zq->flags |= RPMZ_FLAGS_STDOUT;
    } else if (strstr(name, "un") != NULL) {
	zq->mode = RPMZ_MODE_DECOMPRESS;
    }

    return rc;
}
#endif	/* _RPMZ_INTERNAL_XZ */

#ifdef __cplusplus
}
#endif

#endif
