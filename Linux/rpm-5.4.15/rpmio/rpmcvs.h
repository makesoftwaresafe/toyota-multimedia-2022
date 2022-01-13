#ifndef	H_RPMCVS
#define	H_RPMCVS

/** \ingroup rpmio
 * \file rpmio/rpmcvs.h
 */

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmcvs_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmcvs_s * rpmcvs;

#if defined(_RPMCVS_INTERNAL)

/** \ingroup rpmio
 */
struct rpmcvs_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMCVS_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a cvs wrapper instance.
 * @param cvs		cvs wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmcvs rpmcvsUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmcvs cvs)
	/*@modifies cvs @*/;
#define	rpmcvsUnlink(_cvs)	\
    ((rpmcvs)rpmioUnlinkPoolItem((rpmioItem)(_cvs), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a cvs wrapper instance.
 * @param cvs		cvs wrapper
 * @return		new cvs wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmcvs rpmcvsLink (/*@null@*/ rpmcvs cvs)
	/*@modifies cvs @*/;
#define	rpmcvsLink(_cvs)	\
    ((rpmcvs)rpmioLinkPoolItem((rpmioItem)(_cvs), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a cvs wrapper.
 * @param cvs		cvs wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmcvs rpmcvsFree(/*@killref@*/ /*@null@*/rpmcvs cvs)
	/*@globals fileSystem @*/
	/*@modifies cvs, fileSystem @*/;
#define	rpmcvsFree(_cvs)	\
    ((rpmcvs)rpmioFreePoolItem((rpmioItem)(_cvs), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a cvs wrapper.
 * @param fn		cvs file
 * @param flags		cvs flags
 * @return		new cvs wrapper
 */
/*@newref@*/ /*@null@*/
rpmcvs rpmcvsNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMCVS */
