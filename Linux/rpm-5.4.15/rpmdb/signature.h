#ifndef H_SIGNATURE
#define	H_SIGNATURE

/** \ingroup signature
 * \file rpmdb/signature.h
 * Generate and verify rpm package signatures.
 */

#include <rpmtag.h>	/* XXX Header typedef */

/** \ingroup signature
 * Identify PGP versions.
 * @note Greater than 0 is a valid PGP version.
 */
typedef enum pgpVersion_e {
    PGP_NOTDETECTED	= -1,
    PGP_UNKNOWN		= 0,
    PGP_2		= 2,
    PGP_5		= 5
} pgpVersion;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return file handle for a temporaray file.
 * A unique temporaray file path will be generated using
 *	rpmGenPath(prefix, "%{_tmppath}/", "rpm-tmp.XXXXX")
 * where "XXXXXX" is filled in using rand(3). The file is opened, and
 * the link count and (dev,ino) location are verified after opening.
 * The file name and the open file handle are returned.
 *
 * @param prefix	leading part of temp file path
 * @retval *fnptr	temp file name (or NULL)
 * @retval *fdptr	temp file handle
 * @return		0 on success
 */
int rpmTempFile(/*@null@*/ const char * prefix,
		/*@null@*/ /*@out@*/ const char ** fnptr,
		/*@out@*/ void * fdptr)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *fnptr, *fdptr, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup signature
 * Generate signature(s) from a header+payload file, save in signature header.
 * @param sigh		signature header
 * @param file		header+payload file name
 * @param sigTag	type of signature(s) to add
 * @param passPhrase	private key pass phrase
 * @return		0 on success, -1 on failure
 */
int rpmAddSignature(Header sigh, const char * file,
		    rpmSigTag sigTag, /*@null@*/ const char * passPhrase)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies sigh, sigTag, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Check for valid pass phrase by invoking a helper.
 * @param passPhrase	pass phrase
 * @return		0 on valid, 1 on invalid
 */
int rpmCheckPassPhrase(const char * passPhrase)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup signature
 * Verify a signature from a package.
 *
 * @param _dig		container
 * @retval result	detailed text result of signature verification
 * @return		result of signature verification
 */
rpmRC rpmVerifySignature(void * _dig, /*@out@*/ char * result)
	/*@globals internalState @*/
	/*@modifies _dig, *result, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_SIGNATURE */
