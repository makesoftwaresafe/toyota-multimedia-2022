#ifndef RPMPERL_H
#define RPMPERL_H

/** \ingroup rpmio
 * \file rpmio/rpmperl.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmperl_s * rpmperl;

/*@unchecked@*/
extern int _rpmperl_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmperl _rpmperlI;

#if defined(_RPMPERL_INTERNAL)
struct rpmperl_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * I;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMPERL_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a perl interpreter instance.
 * @param perl		perl interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmperl rpmperlUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmperl perl)
	/*@modifies perl @*/;
#define	rpmperlUnlink(_perl)	\
    ((rpmperl)rpmioUnlinkPoolItem((rpmioItem)(_perl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a perl interpreter instance.
 * @param perl		perl interpreter
 * @return		new perl interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmperl rpmperlLink (/*@null@*/ rpmperl perl)
	/*@modifies perl @*/;
#define	rpmperlLink(_perl)	\
    ((rpmperl)rpmioLinkPoolItem((rpmioItem)(_perl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a perl interpreter.
 * @param perl		perl interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmperl rpmperlFree(/*@killref@*/ /*@null@*/rpmperl perl)
	/*@globals fileSystem @*/
	/*@modifies perl, fileSystem @*/;
#define	rpmperlFree(_perl)	\
    ((rpmperl)rpmioFreePoolItem((rpmioItem)(_perl), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a perl interpreter.
 * @param av		perl interpreter args (or NULL)
 * @param flags		perl interpreter flags ((1<<31): use global interpreter)
 * @return		new perl interpreter
 */
/*@newref@*/ /*@null@*/
rpmperl rpmperlNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute perl string.
 * @param perl		perl interpreter (NULL uses global interpreter)
 * @param str		perl string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	perl exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmperlRun(rpmperl perl, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies perl, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMPERL_H */
