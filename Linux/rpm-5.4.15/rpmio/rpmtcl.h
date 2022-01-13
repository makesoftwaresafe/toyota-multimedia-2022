#ifndef RPMTCL_H
#define RPMTCL_H

/** \ingroup rpmio
 * \file rpmio/rpmtcl.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmtcl_s * rpmtcl;

/*@unchecked@*/
extern int _rpmtcl_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmtcl _rpmtclI;

#if defined(_RPMTCL_INTERNAL)
struct rpmtcl_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * I;			/* Tcl_Interp */
    void * tclout;		/* Tcl_Channel */
    rpmiob iob;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMTCL_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a tcl interpreter instance.
 * @param tcl		tcl interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmtcl rpmtclUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmtcl tcl)
	/*@modifies tcl @*/;
#define	rpmtclUnlink(_tcl)	\
    ((rpmtcl)rpmioUnlinkPoolItem((rpmioItem)(_tcl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a tcl interpreter instance.
 * @param tcl		tcl interpreter
 * @return		new tcl interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmtcl rpmtclLink (/*@null@*/ rpmtcl tcl)
	/*@modifies tcl @*/;
#define	rpmtclLink(_tcl)	\
    ((rpmtcl)rpmioLinkPoolItem((rpmioItem)(_tcl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a tcl interpreter.
 * @param tcl		tcl interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmtcl rpmtclFree(/*@killref@*/ /*@null@*/rpmtcl tcl)
	/*@globals fileSystem @*/
	/*@modifies tcl, fileSystem @*/;
#define	rpmtclFree(_tcl)	\
    ((rpmtcl)rpmioFreePoolItem((rpmioItem)(_tcl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a tcl interpreter.
 * @param av		tcl interpreter args (or NULL)
 * @param flags		tcl interpreter flags ((1<<31): use global interpreter)
 * @return		new tcl interpreter
 */
/*@newref@*/ /*@null@*/
rpmtcl rpmtclNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute tcl from a file.
 * @param tcl		tcl interpreter (NULL uses global interpreter)
 * @param fn		tcl file to run (NULL returns RPMRC_FAIL)
 * @param *resultp	tcl exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmtclRunFile(rpmtcl tcl, /*@null@*/ const char * fn,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies tcl, fileSystem, internalState @*/;

/**
 * Execute tcl string.
 * @param tcl		tcl interpreter (NULL uses global interpreter)
 * @param str		tcl string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	tcl exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmtclRun(rpmtcl tcl, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies tcl, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMTCL_H */
