#ifndef _H_PKGIO
#define _H_PKGIO

/** \ingroup lead
 * \file rpmdb/pkgio.h
 * Methods to handle package elements.
 */

#include <rpmio.h>	/* XXX FD_t typedef */
#include <rpmpgp.h>	/* XXX pgpDig typedef */
#include <rpmtag.h>	/* XXX Header typedef */

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Check header consistency, performing headerGetEntry() the hard way.
 *  
 * Sanity checks on the header are performed while looking for a
 * header-only digest or signature to verify the blob. If found,
 * the digest or signature is verified.
 *
 * @param dig		signature parameters container
 * @param uh		unloaded header blob
 * @param uc		no. of bytes in blob (or 0 to disable)
 * @retval *msg		verification error message (or NULL)
 * @return		RPMRC_OK on success
 */
rpmRC headerCheck(pgpDig dig, const void * uh, size_t uc,
		/*@out@*/ /*@null@*/ const char ** msg)
	/*@globals fileSystem, internalState @*/
	/*@modifies dig, *msg, fileSystem, internalState @*/;

/**
 * Return package header from file handle, verifying digests/signatures.
 * @todo The rpmts either needs to become void *, or changed to -lrpmdb scope.
 * @param ts		transaction set
 * @param fd		file handle
 * @param fn		file name
 * @retval hdrp		address of header (or NULL)
 * @return		RPMRC_OK on success
 */
rpmRC rpmReadPackageFile(rpmts ts, FD_t fd,
		const char * fn, /*@null@*/ /*@out@*/ Header * hdrp)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fd, *hdrp, fileSystem, internalState @*/;

/**
 * Return size of item in bytes.
 * @param fn		item name
 * @param ptr		item buffer
 * @return		size of item in bytes.
 */
size_t rpmpkgSizeof(const char * fn, /*@null@*/ const void * ptr)
	/*@*/;

/**
 * Write item onto file descriptor.
 * @param fn		item name
 * @param fd		file handle
 * @param ptr		item buffer
 * @retval *msg		item check failure message
 * @return		RPMRC_OK on success
 */
rpmRC rpmpkgWrite(const char * fn, FD_t fd, void * ptr, const char ** msg)
	/*@globals fileSystem, internalState @*/
	/*@modifies fd, ptr, *msg, fileSystem, internalState @*/;

/**
 * Read item from file descriptor.
 * @param fn		item name
 * @param fd		file handle
 * @retval *ptr		item buffer
 * @retval *msg		item check failure message
 * @return		RPMRC_OK on success
 */
rpmRC rpmpkgRead(const char * fn, FD_t fd, /*@null@*/ /*@out@*/ void * ptr,
		const char ** msg)
	/*@globals fileSystem, internalState @*/
	/*@modifies fd, *ptr, *msg, fileSystem, internalState @*/;

/**
 * Verify item integrity.
 * @param fn		item name
 * @param fd		file handle
 * @param ptr		item buffer
 * @retval *msg		item check failure message
 * @return		RPMRC_OK on success
 */
rpmRC rpmpkgCheck(const char * fn, FD_t fd, const void * ptr, const char ** msg)
	/*@globals fileSystem, internalState @*/
	/*@modifies ptr, *msg, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* _H_PKGIO */
