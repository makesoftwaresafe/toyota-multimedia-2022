#ifndef	H_RPMBAG
#define	H_RPMBAG

/** \ingroup rpmio
 * \file rpmio/rpmbag.h
 */

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmbag_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmbag_s * rpmbag;
typedef struct rpmsdb_s * rpmsdb;

#if defined(_RPMBAG_INTERNAL)

struct rpmsdb_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * _bf;
    int dbmode;
    void * _db;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/** \ingroup rpmio
 */
struct rpmbag_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
    int flags;
    size_t nsdbp;
    rpmsdb * sdbp;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMBAG_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a bag wrapper instance.
 * @param bag		bag wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmbag rpmbagUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmbag bag)
	/*@modifies bag @*/;
#define	rpmbagUnlink(_bag)	\
    ((rpmbag)rpmioUnlinkPoolItem((rpmioItem)(_bag), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a bag wrapper instance.
 * @param bag		bag wrapper
 * @return		new bag wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmbag rpmbagLink (/*@null@*/ rpmbag bag)
	/*@modifies bag @*/;
#define	rpmbagLink(_bag)	\
    ((rpmbag)rpmioLinkPoolItem((rpmioItem)(_bag), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a bag wrapper.
 * @param bag		bag wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmbag rpmbagFree(/*@killref@*/ /*@null@*/rpmbag bag)
	/*@globals fileSystem @*/
	/*@modifies bag, fileSystem @*/;
#define	rpmbagFree(_bag)	\
    ((rpmbag)rpmioFreePoolItem((rpmioItem)(_bag), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a bag wrapper.
 * @param fn		bag file
 * @param flags		bag flags
 * @return		new bag wrapper
 */
/*@newref@*/ /*@null@*/
rpmbag rpmbagNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

int rpmbagAdd(rpmbag bag, void *_db, int dbmode)
	/*@*/;

int rpmbagDel(rpmbag bag, int i)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMBAG */
