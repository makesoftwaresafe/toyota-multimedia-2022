#ifndef	H_RPMSVN
#define	H_RPMSVN

/** \ingroup rpmio
 * \file rpmio/rpmsvn.h
 */

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmsvn_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmsvn_s * rpmsvn;

#if defined(_RPMSVN_INTERNAL)
/** \ingroup rpmio
 */
struct rpmsvn_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;

#if defined(HAVE_APR_1_APR_H)	/* XXX FIXME: use void * instead */
    apr_allocator_t * allocator;
    apr_pool_t * pool;
#else
    void * allocator;
    void * pool;
#endif
#if defined(HAVE_SUBVERSION_1_SVN_CLIENT_H) /* XXX FIXME: use void * instead */
    svn_repos_t * repos;
    svn_fs_txn_t * txn;
    svn_fs_root_t * txn_root;
#else
    void * repos;
    void * txn;
    void * txn_root;
#endif

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMSVN_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a svn wrapper instance.
 * @param svn		svn wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsvn rpmsvnUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsvn svn)
	/*@modifies svn @*/;
#define	rpmsvnUnlink(_svn)	\
    ((rpmsvn)rpmioUnlinkPoolItem((rpmioItem)(_svn), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a svn wrapper instance.
 * @param svn		svn wrapper
 * @return		new svn wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsvn rpmsvnLink (/*@null@*/ rpmsvn svn)
	/*@modifies svn @*/;
#define	rpmsvnLink(_svn)	\
    ((rpmsvn)rpmioLinkPoolItem((rpmioItem)(_svn), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a svn wrapper.
 * @param svn		svn wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsvn rpmsvnFree(/*@killref@*/ /*@null@*/rpmsvn svn)
	/*@globals fileSystem @*/
	/*@modifies svn, fileSystem @*/;
#define	rpmsvnFree(_svn)	\
    ((rpmsvn)rpmioFreePoolItem((rpmioItem)(_svn), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a svn wrapper.
 * @param fn		svn file
 * @param flags		svn flags
 * @return		new svn wrapper
 */
/*@newref@*/ /*@null@*/
rpmsvn rpmsvnNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMSVN */
