#ifndef _H_RPMKU_
#define	_H_RPMKU_

/** \ingroup rpmio
 * \file rpmio/rpmku.h
 */

/**
 * Keyutils keyring to use.
 */
/*@unchecked@*/
extern rpmint32_t _kuKeyring;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lookup pubkey in keyutils keyring.
 * @param sigp		signature packet
 * @retval *iobp	pubkey I/O buffer
 * @return		RPMRC_OK on success
 */
rpmRC rpmkuFindPubkey(pgpDigParams sigp, /*@out@*/ rpmiob * iobp)
	/*@globals fileSystem @*/
	/*@modifies *iobp, fileSystem @*/;

/**
 * Store pubkey in keyutils keyring.
 * @param sigp		signature packet
 * @param iob		pubkey I/O buffer
 * @return		RPMRC_OK on success
 */
rpmRC rpmkuStorePubkey(pgpDigParams sigp, /*@only@*/ rpmiob iob)
	/*@globals fileSystem @*/
	/*@modifies iob, fileSystem @*/;

/**
 * Return pass phrase from keyutils keyring.
 * @param passPhrase	pass phrase
 * @return		(malloc'd) pass phrase
 */
/*@null@*/
const char * rpmkuPassPhrase(const char * passPhrase)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif /* _H_RPMKU_ */
