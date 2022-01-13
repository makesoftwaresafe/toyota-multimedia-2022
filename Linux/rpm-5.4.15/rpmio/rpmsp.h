#ifndef	H_RPMSP
#define	H_RPMSP

/** \ingroup rpmio
 * \file rpmio/rpmsp.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@refcounted@*/ struct rpmsp_s * rpmsp;

/*@unchecked@*/
extern int _rpmsp_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmsp _rpmspI;

#if defined(_RPMSP_INTERNAL)
/** \ingroup rpmio
 */
struct rpmsp_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
    unsigned int flags;
    struct sepol_handle * I;		/*!< sepol_handle_t */
    struct sepol_policydb * DB;		/*!< sepol_policydb_t */
    struct sepol_policy_file * F;	/*!< sepol_policy_file_t */
    struct sepol_context * C;		/*!< sepol_context_t */
    struct sepol_module_package * P;	/*!< sepol_module_package_t */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMSP_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a sepol wrapper instance.
 * @param sp		sepol wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsp rpmspUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsp sp)
	/*@modifies sp @*/;
#define	rpmspUnlink(_sp)	\
    ((rpmsp)rpmioUnlinkPoolItem((rpmioItem)(_sp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a sepol wrapper instance.
 * @param sp		sepol wrapper
 * @return		new sepol wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsp rpmspLink (/*@null@*/ rpmsp sp)
	/*@modifies sp @*/;
#define	rpmspLink(_sp)	\
    ((rpmsp)rpmioLinkPoolItem((rpmioItem)(_sp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a sepol wrapper.
 * @param sp		sepol wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsp rpmspFree(/*@killref@*/ /*@null@*/rpmsp sp)
	/*@globals fileSystem @*/
	/*@modifies sp, fileSystem @*/;
#define	rpmspFree(_sp)	\
    ((rpmsp)rpmioFreePoolItem((rpmioItem)(_sp), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a sepol wrapper.
 * @param fn		sepol file (unused).
 * @param flags		sepol flags
 * @return		new sepol wrapper
 */
/*@newref@*/ /*@null@*/
rpmsp rpmspNew(/*@null@*/ const char * fn, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMSP */
