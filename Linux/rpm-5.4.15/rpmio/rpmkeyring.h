#ifndef _RPMKEYRING_H
#define _RPMKEYRING_H

/** \ingroup rpmkeyring
 * \file rpmio/rpmkeyring.h
 */

/** \ingroup rpmkeyring
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmPubkey_s * rpmPubkey;

/** \ingroup rpmkeyring
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmKeyring_s * rpmKeyring;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmkeyring
 * Create a new, empty keyring
 * @return		new keyring handle
 */
rpmKeyring rpmKeyringNew(void)
	/*@*/;

/** \ingroup rpmkeyring
 * Free keyring and the keys within it
 * @return		NULL always
 */
/*@null@*/
rpmKeyring rpmKeyringFree(/*@killref@*/ rpmKeyring keyring)
	/*@modifies keyring @*/;

/** \ingroup rpmkeyring
 * Add a public key to keyring.
 * @param keyring	keyring handle
 * @param key		pubkey handle
 * @return		0 on success, -1 on error, 1 if key already present
 */
int rpmKeyringAddKey(rpmKeyring keyring, rpmPubkey key)
	/*@modifies keyring, key @*/;

/** \ingroup rpmkeyring
 * Perform keyring lookup for a key matching a signature
 * @param keyring	keyring handle
 * @param sig		OpenPGP packet container of signature
 * @return		RPMRC_OK if found, RPMRC_NOKEY otherwise
 */
rpmRC rpmKeyringLookup(rpmKeyring keyring, pgpDig sig)
	/*@globals fileSystem, internalState @*/
	/*@modifies sig, fileSystem, internalState @*/;

/** \ingroup rpmkeyring
 * Reference a keyring.
 * @param keyring	keyring handle
 * @return		new keyring reference
 */
/*@newref@*/
rpmKeyring rpmKeyringLink(/*@returned@*/ rpmKeyring keyring)
	/*@modifies keyring @*/;

/** \ingroup rpmkeyring
 * Unreference a keyring.
 * @param keyring	keyring handle
 * @return		NULL always
 */
/*@null@*/
rpmKeyring rpmKeyringUnlink(/*@killref@*/ rpmKeyring keyring)
	/*@modifies keyring @*/;

/** \ingroup rpmkeyring
 * Create a new rpmPubkey from OpenPGP packet
 * @param pkt		OpenPGP packet data
 * @param pktlen	Data length
 * @return		new pubkey handle
 */
rpmPubkey rpmPubkeyNew(const rpmuint8_t *pkt, size_t pktlen)
	/*@*/;

/** \ingroup rpmkeyring
 * Create a new rpmPubkey from ASCII-armored pubkey file
 * @param filename	Path to pubkey file
 * @return		new pubkey handle
 */
rpmPubkey rpmPubkeyRead(const char *filename)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/** \ingroup rpmkeyring
 * Free a pubkey.
 * @param key		Pubkey to free
 * @return		NULL always
 */
/*@null@*/
rpmPubkey rpmPubkeyFree(/*@killref@*/ rpmPubkey key)
	/*@modifies key @*/;

/** \ingroup rpmkeyring
 * Reference a pubkey.
 * @param key		Pubkey
 * @return		new pubkey reference
 */
/*@newref@*/
rpmPubkey rpmPubkeyLink(/*@returned@*/ rpmPubkey key)
	/*@modifies key @*/;

/** \ingroup rpmkeyring
 * Unreference a pubkey.
 * @param key		Pubkey
 * @return		NULL always
 */
/*@null@*/
rpmPubkey rpmPubkeyUnlink(/*@killref@*/ rpmPubkey key)
	/*@modifies key @*/;

#ifdef __cplusplus
}
#endif

#endif /* _RPMKEYDB_H */
