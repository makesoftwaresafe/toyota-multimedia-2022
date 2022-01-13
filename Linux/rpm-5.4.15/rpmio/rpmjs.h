#ifndef RPMJS_H
#define RPMJS_H

/** \ingroup rpmio
 * \file rpmio/rpmjs.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmjs_s * rpmjs;

/*@unchecked@*/
extern int _rpmjs_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmjs _rpmjsI;

/*@unchecked@*/
extern uint32_t _rpmjs_options;

/*@unchecked@*/
extern int _rpmjs_zeal;

#if defined(_RPMJS_INTERNAL)
/**
 * Interpreter flags.
 */
enum rpmjsFlags_e {
    RPMJS_FLAGS_NONE		= 0,
    RPMJS_FLAGS_STRICT		= (1<< 0),	/* JSOPTION_STRICT */
    RPMJS_FLAGS_WERROR		= (1<< 1),	/* JSOPTION_WERROR */
    RPMJS_FLAGS_VAROBJFIX	= (1<< 2),	/* JSOPTION_VAROBJFIX */
    RPMJS_FLAGS_PRIVATE_IS_NSISUPPORTS = (1<< 3), /* JSOPTION_PRIVATE_IS_NSISUPPORTS */
    RPMJS_FLAGS_COMPILE_N_GO	= (1<< 4),	/* JSOPTION_COMPILE_N_GO */
    RPMJS_FLAGS_ATLINE		= (1<< 5),	/* JSOPTION_ATLINE */
    RPMJS_FLAGS_XML		= (1<< 6),	/* JSOPTION_XML */
	/* bit 7 unused */
    RPMJS_FLAGS_DONT_REPORT_UNCAUGHT = (1<< 8),	/* JSOPTION_DONT_REPORT_UNCAUGHT */
    RPMJS_FLAGS_RELIMIT		= (1<< 9),	/* JSOPTION_RELIMIT */
    RPMJS_FLAGS_ANONFUNFIX	= (1<<10),	/* JSOPTION_ANONFUNFIX */
    RPMJS_FLAGS_JIT		= (1<<11),	/* JSOPTION_JIT */
    RPMJS_FLAGS_NO_SCRIPT_RVAL	= (1<<12),	/* JSOPTION_NO_SCRIPT_RVAL */
    RPMJS_FLAGS_UNROOTED_GLOBAL	= (1<<13),	/* JSOPTION_UNROOTED_GLOBAL */
	/* bits 14-15 unused */
    RPMJS_FLAGS_NOEXEC		= (1<<16),	/*!< -n */
    RPMJS_FLAGS_SKIPSHEBANG	= (1<<17),	/*!< -F */
    RPMJS_FLAGS_LOADRC		= (1<<18),	/*!< -R */
    RPMJS_FLAGS_NOUTF8		= (1<<19),	/*!< -U */
    RPMJS_FLAGS_NOCACHE		= (1<<20),	/*!< -C */
    RPMJS_FLAGS_NOWARN		= (1<<21),	/*!< -W */
    RPMJS_FLAGS_ALLOW		= (1<<22),	/*!< -a */
	/* bits 23-30 unused */
    RPMJS_FLAGS_GLOBAL		= (1<<31),
};

struct rpmjs_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    uint32_t flags;		/*!< JSOPTION_FOO in 0xffff bits */
    void * I;			/*!< JS interpreter {rt, cx, globalObj} */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/*@unchecked@*/
struct rpmjs_s _rpmjs;

#endif /* _RPMJS_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a js interpreter instance.
 * @param js		js interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmjs rpmjsUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmjs js)
	/*@modifies js @*/;
#define	rpmjsUnlink(_js)	\
    ((rpmjs)rpmioUnlinkPoolItem((rpmioItem)(_js), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a js interpreter instance.
 * @param js		js interpreter
 * @return		new js interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmjs rpmjsLink (/*@null@*/ rpmjs js)
	/*@modifies js @*/;
#define	rpmjsLink(_js)	\
    ((rpmjs)rpmioLinkPoolItem((rpmioItem)(_js), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a js interpreter.
 * @param js		js interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmjs rpmjsFree(/*@killref@*/ /*@null@*/rpmjs js)
	/*@globals fileSystem @*/
	/*@modifies js, fileSystem @*/;
#define	rpmjsFree(_js)	\
    ((rpmjs)rpmioFreePoolItem((rpmioItem)(_js), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a js interpreter.
 * @param av		js interpreter args (or NULL)
 * @param flags		js interpreter flags ((1<<31): use global interpreter)
 * @return		new js interpreter
 */
/*@newref@*/ /*@null@*/
rpmjs rpmjsNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute js from a file.
 * @param js		js interpreter (NULL uses global interpreter)
 * @param fn		js file to run (NULL returns RPMRC_FAIL)
 * @param Iargv		js script argv
 * @param *resultp	js exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmjsRunFile(rpmjs js, /*@null@*/ const char * fn,
		/*@null@*/ char *const * Iargv,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies js, fileSystem, internalState @*/;

/**
 * Execute js string.
 * @param js		js interpreter (NULL uses global interpreter)
 * @param str		js string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	js exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmjsRun(rpmjs js, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies js, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMJS_H */
