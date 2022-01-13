#ifndef RPMFICL_H
#define RPMFICL_H

/** \ingroup rpmio
 * \file rpmio/rpmficl.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmficl_s * rpmficl;

/*@unchecked@*/
extern int _rpmficl_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmficl _rpmficlI;

#if defined(_RPMFICL_INTERNAL)
struct rpmficl_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * sys;			/* ficlSystem */
    void * vm;			/* ficlVm */
    rpmiob iob;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMFICL_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a ficl interpreter instance.
 * @param ficl		ficl interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmficl rpmficlUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmficl ficl)
	/*@modifies ficl @*/;
#define	rpmficlUnlink(_ficl)	\
    ((rpmficl)rpmioUnlinkPoolItem((rpmioItem)(_ficl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a ficl interpreter instance.
 * @param ficl		ficl interpreter
 * @return		new ficl interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmficl rpmficlLink (/*@null@*/ rpmficl ficl)
	/*@modifies ficl @*/;
#define	rpmficlLink(_ficl)	\
    ((rpmficl)rpmioLinkPoolItem((rpmioItem)(_ficl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a ficl interpreter.
 * @param ficl		ficl interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmficl rpmficlFree(/*@killref@*/ /*@null@*/rpmficl ficl)
	/*@globals fileSystem @*/
	/*@modifies ficl, fileSystem @*/;
#define	rpmficlFree(_ficl)	\
    ((rpmficl)rpmioFreePoolItem((rpmioItem)(_ficl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a ficl interpreter.
 * @param av		ficl interpreter args (or NULL)
 * @param flags		ficl interpreter flags ((1<<31) == use global interpreter)
 * @return		new ficl interpreter
 */
/*@newref@*/ /*@null@*/
rpmficl rpmficlNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute ficl from a file.
 * @param ficl		ficl interpreter (NULL uses global interpreter)
 * @param fn		ficl file to run (NULL returns RPMRC_FAIL)
 * @param *resultp	ficl exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmficlRunFile(rpmficl ficl, /*@null@*/ const char * fn,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies ficl, fileSystem, internalState @*/;

/**
 * Execute ficl string.
 * @param ficl		ficl interpreter (NULL uses global interpreter)
 * @param str		ficl string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	ficl exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmficlRun(rpmficl ficl, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies ficl, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMFICL_H */
