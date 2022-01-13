#ifndef	H_RPMMG
#define	H_RPMMG

/** \ingroup rpmio
 * \file rpmio/rpmmg.h
 */

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmmg_debug;

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmmg_s * rpmmg;

#if defined(_RPMMG_INTERNAL)
/** \ingroup rpmio
 */
struct rpmmg_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
    int flags;
/*@relnull@*/
    struct magic_set * ms;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMMG_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a magic wrapper instance.
 * @param mg		magic wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmmg rpmmgUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmmg mg)
	/*@modifies mg @*/;
#define	rpmmgUnlink(_mg)	\
    ((rpmmg)rpmioUnlinkPoolItem((rpmioItem)(_mg), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a magic wrapper instance.
 * @param mg		magic wrapper
 * @return		new magic wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmmg rpmmgLink (/*@null@*/ rpmmg mg)
	/*@modifies mg @*/;
#define	rpmmgLink(_mg)	\
    ((rpmmg)rpmioLinkPoolItem((rpmioItem)(_mg), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a magic wrapper.
 * @param mg		magic wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmmg rpmmgFree(/*@killref@*/ /*@null@*/rpmmg mg)
	/*@globals fileSystem @*/
	/*@modifies mg, fileSystem @*/;
#define	rpmmgFree(_mg)	\
    ((rpmmg)rpmioFreePoolItem((rpmioItem)(_mg), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a magic wrapper.
 * @param fn		magic file
 * @param flags		magic flags
 * @return		new magic wrapper
 */
/*@newref@*/ /*@null@*/
rpmmg rpmmgNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return magic string for a file.
 * @param mg		magic wrapper
 * @param fn		file path
 * @return		file magic string
 */
/*@only@*/
const char * rpmmgFile(rpmmg mg, const char *fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies mg, fileSystem, internalState @*/;

/**
 * Return magic string for a buffer.
 * @param mg		magic wrapper
 * @param b		buffer
 * @param nb		no. bytes in buffer
 * @return		buffer magic string
 */
/*@only@*/
const char * rpmmgBuffer(rpmmg mg, const char * b, size_t nb)
	/*@globals fileSystem, internalState @*/
	/*@modifies mg, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMMG */
