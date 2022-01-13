#ifndef	H_RPMSX
#define	H_RPMSX

/** \ingroup rpmio
 * \file rpmio/rpmsx.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmsx_s * rpmsx;

/*@unchecked@*/
extern int _rpmsx_debug;

/*@unchecked@*/
extern rpmsx _rpmsxI;

#if defined(_RPMSX_INTERNAL)
/** \ingroup rpmio
 */
struct rpmsx_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;		/*!< File context path (NULL disables). */
    unsigned int flags;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMSX_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a SELinux wrapper instance.
 * @param sx		SELinux wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsx rpmsxUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsx sx)
	/*@modifies sx @*/;
#define	rpmsxUnlink(_sx)	\
    ((rpmsx)rpmioUnlinkPoolItem((rpmioItem)(_sx), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a SELinux wrapper instance.
 * @param sx		SELinux wrapper
 * @return		new SELinux wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsx rpmsxLink (/*@null@*/ rpmsx sx)
	/*@modifies sx @*/;
#define	rpmsxLink(_sx)	\
    ((rpmsx)rpmioLinkPoolItem((rpmioItem)(_sx), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a SELinux wrapper.
 * @param sx		SELinux wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsx rpmsxFree(/*@killref@*/ /*@null@*/rpmsx sx)
	/*@globals fileSystem @*/
	/*@modifies sx, fileSystem @*/;
#define	rpmsxFree(_sx)	\
    ((rpmsx)rpmioFreePoolItem((rpmioItem)(_sx), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a SELinux wrapper.
 * @param fn		SELinux file (NULL uses selinux_file_context_path()).
 * @param flags		SELinux set_matchpathcon_flags() flags
 * @return		new SELinux wrapper
 */
/*@newref@*/ /*@null@*/
rpmsx rpmsxNew(/*@null@*/ const char * fn, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return SELinux enabled state.
 * @param sx		SELinux wrapper (NULL uses active context)
 * @return		SELinux enabled state
 */
int rpmsxEnabled(/*@null@*/ rpmsx sx)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return security context for a file.
 * @param sx		SELinux wrapper (NULL uses active context)
 * @param fn		file path
 * @param mode		file mode
 * @return		file security context
 */
/*@only@*/
const char * rpmsxMatch(/*@null@*/ rpmsx sx, const char * fn, mode_t mode)
	/*@globals fileSystem, internalState @*/
	/*@modifies sx, fileSystem, internalState @*/;

/**
 * Get security context for a file.
 * @param sx		SELinux wrapper (NULL uses active context)
 * @param fn		file path
 * @return		file security context (NULL on error/disabled)
 */
/*@null@*/
const char * rpmsxGetfilecon(/*@null@*/ rpmsx sx, const char *fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@null@*/
const char * rpmsxLgetfilecon(/*@null@*/ rpmsx sx, const char *fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Set security context for a file.
 * @param sx		SELinux wrapper (NULL uses active context)
 * @param fn		file path
 * @param mode		file mode
 * @param scon		file security context (NULL calls matchpathcon())
 * @return		0 on success
 */
int rpmsxSetfilecon(/*@null@*/ rpmsx sx, const char *fn, mode_t mode,
		/*@null@*/ const char * scon) 
	/*@globals fileSystem, internalState @*/
	/*@modifies sx, fileSystem, internalState @*/;
int rpmsxLsetfilecon(/*@null@*/ rpmsx sx, const char *fn, mode_t mode,
		/*@null@*/ const char * scon) 
	/*@globals fileSystem, internalState @*/
	/*@modifies sx, fileSystem, internalState @*/;

/**
 * Execute a package scriptlet within SELinux context.
 * @param sx		SELinux wrapper
 * @param verified	Scriptlet came from signature verified header? (unused)
 * @param argv		scriptlet helper
 * @return		0 on success
 */
int rpmsxExec(rpmsx sx, int verified, const char ** argv)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMSX */
