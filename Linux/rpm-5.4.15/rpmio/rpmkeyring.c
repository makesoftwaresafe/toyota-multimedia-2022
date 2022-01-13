#include "system.h"

#include "rpmio_internal.h"
#include "rpmkeyring.h"

#include "debug.h"

/*@access pgpDig @*/
/*@access pgpDigParams @*/

struct rpmPubkey_s {
    rpmuint8_t *pkt;
    size_t pktlen;
    pgpKeyID_t keyid;
/*@refs@*/
    int nrefs;
};

struct rpmKeyring_s {
/*@relnull@*/
    rpmPubkey *keys;
    size_t numkeys;
/*@refs@*/
    int nrefs;
};

static int keyidcmp(const void *k1, const void *k2)
{
    const struct rpmPubkey_s *key1 = *(const struct rpmPubkey_s **) k1;
    const struct rpmPubkey_s *key2 = *(const struct rpmPubkey_s **) k2;
    
    return memcmp(key1->keyid, key2->keyid, sizeof(key1->keyid));
}

rpmKeyring rpmKeyringNew(void)
{
    rpmKeyring keyring = xcalloc(1, sizeof(*keyring));
    keyring->keys = NULL;
    keyring->numkeys = 0;
    keyring->nrefs = 0;
    return rpmKeyringLink(keyring);
}

rpmKeyring rpmKeyringFree(rpmKeyring keyring)
{
    if (keyring == NULL)
	return NULL;

    if (keyring->nrefs > 1)
	return rpmKeyringUnlink(keyring);

    if (keyring->keys) {
	int i;
/*@-unqualifiedtrans @*/
	for (i = 0; i < (int)keyring->numkeys; i++)
	    keyring->keys[i] = rpmPubkeyFree(keyring->keys[i]);
/*@=unqualifiedtrans @*/
	keyring->keys = _free(keyring->keys);
    }
    keyring = _free(keyring);
    return NULL;
}

static rpmPubkey rpmKeyringFindKeyid(rpmKeyring keyring, rpmPubkey key)
	/*@*/
{
    rpmPubkey *found = NULL;
    found = bsearch(&key, keyring->keys, keyring->numkeys, sizeof(*keyring->keys), keyidcmp);
    return found ? *found : NULL;
}

int rpmKeyringAddKey(rpmKeyring keyring, rpmPubkey key)
{
    if (keyring == NULL || key == NULL)
	return -1;

    /* check if we already have this key */
    if (rpmKeyringFindKeyid(keyring, key))
	return 1;
    
    keyring->keys = xrealloc(keyring->keys, (keyring->numkeys + 1) * sizeof(*keyring->keys));
/*@-assignexpose -newreftrans @*/
    keyring->keys[keyring->numkeys] = rpmPubkeyLink(key);
/*@=assignexpose =newreftrans @*/
    keyring->numkeys++;
    qsort(keyring->keys, keyring->numkeys, sizeof(*keyring->keys), keyidcmp);

    return 0;
}

rpmKeyring rpmKeyringLink(rpmKeyring keyring)
{
    if (keyring)
	keyring->nrefs++;
/*@-nullret@*/
    return keyring;
/*@=nullret@*/
}

rpmKeyring rpmKeyringUnlink(rpmKeyring keyring)
{
    if (keyring)
	keyring->nrefs--;
    return NULL;
}

rpmPubkey rpmPubkeyRead(const char *filename)
{
    rpmuint8_t *pkt = NULL;
    size_t pktlen;
    rpmPubkey key = NULL;

/*@-globs@*/
    if (pgpReadPkts(filename, &pkt, &pktlen) <= 0)
	goto exit;
/*@=globs@*/
    key = rpmPubkeyNew(pkt, pktlen);
    pkt = _free(pkt);

exit:
    return key;
}

rpmPubkey rpmPubkeyNew(const rpmuint8_t *pkt, size_t pktlen)
{
    rpmPubkey key = NULL;
    
    if (pkt == NULL || pktlen == 0)
	goto exit;

    key = xcalloc(1, sizeof(*key));
    (void) pgpPubkeyFingerprint(pkt, pktlen, key->keyid);
    key->pkt = xmalloc(pktlen);
    key->pktlen = pktlen;
    key->nrefs = 0;
    memcpy(key->pkt, pkt, pktlen);

exit:
    return rpmPubkeyLink(key);
}

rpmPubkey rpmPubkeyFree(rpmPubkey key)
{
    if (key == NULL)
	return NULL;

    if (key->nrefs > 1)
	return rpmPubkeyUnlink(key);

    key->pkt = _free(key->pkt);
    key = _free(key);
    return NULL;
}

rpmPubkey rpmPubkeyLink(rpmPubkey key)
{
    if (key)
	key->nrefs++;
/*@-nullret@*/
    return key;
/*@=nullret@*/
}

rpmPubkey rpmPubkeyUnlink(rpmPubkey key)
{
    if (key)
	key->nrefs--;
    return NULL;
}

rpmRC rpmKeyringLookup(rpmKeyring keyring, pgpDig sig)
{
    rpmRC res = RPMRC_NOKEY;

    if (keyring && sig) {
	pgpDigParams sigp = &sig->signature;
	pgpDigParams pubp = &sig->pubkey;
	struct rpmPubkey_s needle, *key;
	needle.pkt = NULL;
	needle.pktlen = 0;
	memcpy(needle.keyid, sigp->signid, sizeof(needle.keyid));

	if ((key = rpmKeyringFindKeyid(keyring, &needle))) {
	    /* Retrieve parameters from pubkey packet(s) */
	    (void) pgpPrtPkts(key->pkt, key->pktlen, sig, 0);
	    /* Do the parameters match the signature? */
	    if (sigp->pubkey_algo == pubp->pubkey_algo &&
		memcmp(sigp->signid, pubp->signid, sizeof(sigp->signid)) == 0) {
		res = RPMRC_OK;
	    }
	}
    }

    return res;
}
