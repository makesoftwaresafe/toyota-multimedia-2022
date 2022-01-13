#ifndef RPMPYTHON_H
#define RPMPYTHON_H

/** \ingroup rpmio
 * \file rpmio/rpmpython.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmpython_s * rpmpython;

/*@unchecked@*/
extern int _rpmpython_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmpython _rpmpythonI;

#if defined(_RPMPYTHON_INTERNAL)
struct rpmpython_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * I;			/* (unused) */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMPYTHON_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a python interpreter instance.
 * @param python	python interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmpython rpmpythonUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmpython python)
	/*@modifies python @*/;
#define	rpmpythonUnlink(_python)	\
    ((rpmpython)rpmioUnlinkPoolItem((rpmioItem)(_python), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a python interpreter instance.
 * @param python	python interpreter
 * @return		new python interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmpython rpmpythonLink (/*@null@*/ rpmpython python)
	/*@modifies python @*/;
#define	rpmpythonLink(_python)	\
    ((rpmpython)rpmioLinkPoolItem((rpmioItem)(_python), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a python interpreter.
 * @param python	python interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmpython rpmpythonFree(/*@killref@*/ /*@null@*/rpmpython python)
	/*@globals fileSystem @*/
	/*@modifies python, fileSystem @*/;
#define	rpmpythonFree(_python)	\
    ((rpmpython)rpmioFreePoolItem((rpmioItem)(_python), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a python interpreter.
 * @param av		python interpreter args (or NULL)
 * @param flags		python interpreter flags ((1<<31): use global interpreter)
 * @return		new python interpreter
 */
/*@newref@*/ /*@null@*/
rpmpython rpmpythonNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute python from a file.
 * @param python	python interpreter (NULL uses global interpreter)
 * @param fn		python file to run (NULL returns RPMRC_FAIL)
 * @param *resultp	python exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmpythonRunFile(rpmpython python, /*@null@*/ const char * fn,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies python, fileSystem, internalState @*/;

/**
 * Execute python string.
 * @param python	python interpreter (NULL uses global interpreter)
 * @param str		python string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	python exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmpythonRun(rpmpython python, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies python, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMPYTHON_H */
