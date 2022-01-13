#ifndef RPMSQUIRREL_H
#define RPMSQUIRREL_H

/** \ingroup rpmio
 * \file rpmio/rpmsquirrel.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@refcounted@*/ struct rpmsquirrel_s * rpmsquirrel;

/*@unchecked@*/
extern int _rpmsquirrel_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmsquirrel _rpmsquirrelI;

#if defined(_RPMSQUIRREL_INTERNAL)
struct rpmsquirrel_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * I;			/* HSQUIRRELVM */
    rpmiob iob;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMSQUIRREL_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a squirrel interpreter instance.
 * @param squirrel		squirrel interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsquirrel rpmsquirrelUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsquirrel squirrel)
	/*@modifies squirrel @*/;
#define	rpmsquirrelUnlink(_squirrel)	\
    ((rpmsquirrel)rpmioUnlinkPoolItem((rpmioItem)(_squirrel), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a squirrel interpreter instance.
 * @param squirrel		squirrel interpreter
 * @return		new squirrel interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsquirrel rpmsquirrelLink (/*@null@*/ rpmsquirrel squirrel)
	/*@modifies squirrel @*/;
#define	rpmsquirrelLink(_squirrel)	\
    ((rpmsquirrel)rpmioLinkPoolItem((rpmioItem)(_squirrel), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a squirrel interpreter.
 * @param squirrel		squirrel interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsquirrel rpmsquirrelFree(/*@killref@*/ /*@null@*/rpmsquirrel squirrel)
	/*@globals fileSystem @*/
	/*@modifies squirrel, fileSystem @*/;
#define	rpmsquirrelFree(_squirrel)	\
    ((rpmsquirrel)rpmioFreePoolItem((rpmioItem)(_squirrel), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a squirrel interpreter.
 * @param av		squirrel interpreter args (or NULL)
 * @param flags		squirrel interpreter flags ((1<<31): use global interpreter)
 * @return		new squirrel interpreter
 */
/*@newref@*/ /*@null@*/
rpmsquirrel rpmsquirrelNew(/*@null@*/ char ** av, unsigned flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute squirrel from a file.
 * @param squirrel		squirrel interpreter (NULL uses global interpreter)
 * @param fn		squirrel file to run (NULL returns RPMRC_FAIL)
 * @param *resultp	squirrel exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmsquirrelRunFile(rpmsquirrel squirrel, /*@null@*/ const char * fn,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies squirrel, fileSystem, internalState @*/;

/**
 * Execute squirrel string.
 * @param squirrel		squirrel interpreter (NULL uses global interpreter)
 * @param str		squirrel string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	squirrel exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmsquirrelRun(rpmsquirrel squirrel, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies squirrel, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMSQUIRREL_H */
