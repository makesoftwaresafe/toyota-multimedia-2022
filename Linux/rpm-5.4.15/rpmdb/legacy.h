#ifndef H_LEGACY
#define H_LEGACY

/**
 * \file rpmdb/legacy.h
 *
 */

/**
 */
/*@-redecl@*/
/*@unchecked@*/
extern int _noDirTokens;
/*@=redecl@*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return digest and size of a file.
 * @param dalgo		digest algorithm to use
 * @param fn		file name
 * @retval *digest	file digest
 * @param dflags	0x1 = return ASCII 0x2 = do HMAC
 * @retval *fsizep	file size pointer (or NULL)
 * @return		0 on success, 1 on error
 */
int dodigest(int dalgo, const char * fn, /*@out@*/ unsigned char * digest,
		unsigned dflags, /*@null@*/ /*@out@*/ size_t *fsizep)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies digest, *fsizep, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_LEGACY */
