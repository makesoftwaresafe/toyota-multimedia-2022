/** \ingroup rpmpgp
 * \file rpmio/rpmssl.c
 */

#include "system.h"
#include <rpmlog.h>

#include <rpmiotypes.h>
#define	_RPMPGP_INTERNAL
#if defined(WITH_SSL)

#if defined(__LCLINT__) && !defined(__i386__)
#define	__i386__
#endif

#define	_RPMSSL_INTERNAL
#include <rpmssl.h>
#endif

#include "debug.h"

#if defined(WITH_SSL)

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

/*@unchecked@*/
static int _rpmssl_debug;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmssl_debug || _pgp_debug < 0) \
	fprintf(stderr, "<-- %s(%p) %s\t%s/%s\n", __FUNCTION__, (_dig), \
		((_rc) ? "OK" : "BAD"), (_dig)->pubkey_algoN, (_dig)->hash_algoN); \
  }

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static
unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char) (c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return (unsigned char) '\0';
}

static unsigned char * rpmsslBN2bin(const char * msg, const BIGNUM * s, size_t maxn)
{
    unsigned char * t = (unsigned char *) xcalloc(1, maxn);
/*@-modunconnomods@*/
    size_t nt = BN_bn2bin(s, t);
/*@=modunconnomods@*/

    if (nt < maxn) {
	size_t pad = (maxn - nt);
	memmove(t+pad, t, nt);
	memset(t, 0, pad);
    }
    return t;
}

/*==============================================================*/
static const EVP_MD * mapHash(unsigned hash_algo)
{
    const EVP_MD * md = NULL;

    switch (hash_algo) {
#ifndef OPENSSL_NO_MD2
    case PGPHASHALGO_MD2:	md = EVP_md2();		break;
#endif
#ifndef OPENSSL_NO_MD4
    case PGPHASHALGO_MD4:	md = EVP_md4();		break;
#endif
#ifndef OPENSSL_NO_MD5
    case PGPHASHALGO_MD5:	md = EVP_md5();		break;
#endif
#ifndef OPENSSL_NO_RIPEMD
    case PGPHASHALGO_RIPEMD160:	md = EVP_ripemd160();	break;
#endif
#ifndef OPENSSL_NO_SHA
    case PGPHASHALGO_SHA1:	md = EVP_sha1();	break;
#endif
#ifndef OPENSSL_NO_SHA256
    case PGPHASHALGO_SHA224:	md = EVP_sha224();	break;
    case PGPHASHALGO_SHA256:	md = EVP_sha256();	break;
#endif
#ifndef OPENSSL_NO_SHA512
    case PGPHASHALGO_SHA384:	md = EVP_sha384();	break;
    case PGPHASHALGO_SHA512:	md = EVP_sha512();	break;
#endif
    /* Anything not enabled above will just fall through... */
    case PGPHASHALGO_TIGER192:	/*@fallthrough@*/
    case PGPHASHALGO_HAVAL_5_160:	/*@fallthrough@*/
    default:
	md = EVP_md_null();	/* XXX FIXME */
	break;
    }
    return md;
}
/*==============================================================*/

static
int rpmsslSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmssl ssl = (rpmssl) dig->impl;
    unsigned int nb = 0;
    const char * prefix = rpmDigestASN1(ctx);
    const char * s;
    uint8_t *t, *te;
    int rc = 1;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_RSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_RSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
assert(prefix != NULL);
ssl->md = mapHash(sigp->hash_algo);	/* XXX unused with RSA */

/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    rc = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 0);

    /* Find the size of the RSA keys */
assert(ssl->pkey);
    {	RSA * rsa = EVP_PKEY_get0(ssl->pkey);
assert(rsa);
	nb = RSA_size(rsa);
    }

    /* Add PKCS1 padding */
    t = te = (uint8_t *) xmalloc(nb);
    memset(te, 0xff, nb);
    te[0] = 0x00;
    te[1] = 0x01;
    te += nb - strlen(prefix)/2 - ssl->digestlen - 1;
    *te++ = 0x00;
    /* Add digest algorithm ASN1 prefix */
    for (s = prefix; *s; s += 2)
	*te++ = (rpmuint8_t) (nibble(s[0]) << 4) | nibble(s[1]);
    memcpy(te, ssl->digest, ssl->digestlen);

    /* Set RSA hash. */
/*@-moduncon -noeffectuncon @*/
/* XXX FIXME: should this lazy free be done elsewhere? */
    if (ssl->hm)
	BN_free(ssl->hm);
    ssl->hm = NULL;
    ssl->hm = BN_bin2bn(t, nb, NULL);
/*@=moduncon =noeffectuncon @*/
    t = _free(t);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ssl->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmsslSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmssl ssl = (rpmssl) dig->impl;
    int rc;
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_DSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_DSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
ssl->md = mapHash(sigp->hash_algo);

    /* Set DSA hash. */
/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ssl->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmsslSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmssl ssl = (rpmssl) dig->impl;
    int rc = 1;		/* XXX always fail. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ssl->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmsslSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmssl ssl = (rpmssl) dig->impl;
    int rc = 1;		/* assume failure. */
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
ssl->md = mapHash(sigp->hash_algo);

/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    rc = rpmDigestFinal(ctx, &ssl->digest, &ssl->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ssl->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmsslErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	NOTYET
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmsslStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmsslAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgsslvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif	/* _RPMGC_INTERNAL */
    return rc;
}

static int rpmsslAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgsslvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif	/* _RPMGC_INTERNAL */
    return rc;
}

static int rpmsslAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmsslAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif	/* _RPMGC_INTERNAL */
    return rc;
}

static int rpmsslVerify(pgpDig dig)
{
    rpmssl ssl = dig->impl;
    unsigned char * t = ssl->digest;
    size_t nt = ssl->digestlen;
    unsigned char * hm = NULL;		/* XXX for RSA */
    EVP_PKEY_CTX *ctx = NULL;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

assert(ssl->sig != NULL && ssl->siglen > 0);

    if ((ctx = EVP_PKEY_CTX_new(ssl->pkey, NULL)) == NULL
     || EVP_PKEY_verify_init(ctx) != 1)
	goto exit;

    switch (pubp->pubkey_algo) {	/* refactor pubkey_algo to dig? */
    default:
	goto exit;
	break;
    case PGPPUBKEYALGO_RSA:
      {	RSA * rsa = EVP_PKEY_get0(ssl->pkey);
	size_t maxn = RSA_size(rsa);
	size_t i;

assert(ssl->hm);
	/* XXX Find end of PKCS1 padding */
	hm = rpmsslBN2bin("hm", ssl->hm, maxn);
	for (i = 2; i < maxn; i++) {
	    if (hm[i] == 0xff)
		continue;
	    i++;
	    break;
	}
	t = hm+i;
	nt = maxn - i;

	/* XXX EVP_PKEY_verify(w/o md) -> RSA_public_decrypt */
	if (!EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING))
	    goto exit;

    }	break;
    case PGPPUBKEYALGO_DSA:
	if (!EVP_PKEY_CTX_set_signature_md(ctx, ssl->md))
	    goto exit;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmsslVerifyELG(dig);
#endif
	    goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
#if !defined(OPENSSL_NO_ECDSA)
	if (!EVP_PKEY_CTX_set_signature_md(ctx, ssl->md))
#endif
	    goto exit;
	break;
    }

    if (EVP_PKEY_verify(ctx, ssl->sig, ssl->siglen, t, nt) == 1)
	rc = 1;

exit:
    if (hm)
	hm = _free(hm);
    if (ctx)
	EVP_PKEY_CTX_free(ctx);
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmsslSign(pgpDig dig)
{
    rpmssl ssl = dig->impl;
    unsigned char * t = ssl->digest;
    size_t nt = ssl->digestlen;
    unsigned char * hm = NULL;		/* XXX for RSA */
    EVP_PKEY_CTX *ctx = NULL;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->sig = _free(ssl->sig);
ssl->siglen = 0;

    if ((ctx = EVP_PKEY_CTX_new(ssl->pkey, NULL)) == NULL
     || EVP_PKEY_sign_init(ctx) != 1)
	goto exit;

    switch (pubp->pubkey_algo) {	/* refactor pubkey_algo to dig? */
    default:
	goto exit;
	break;
    case PGPPUBKEYALGO_RSA:
      {	RSA * rsa = EVP_PKEY_get0(ssl->pkey);
	size_t maxn = RSA_size(rsa);
	size_t i;

assert(ssl->hm);
	/* XXX Find end of PKCS1 padding */
	hm = rpmsslBN2bin("hm", ssl->hm, maxn);
	for (i = 2; i < maxn; i++) {
	    if (hm[i] == 0xff)
		continue;
	    i++;
	    break;
	}
	t = hm+i;
	nt = maxn - i;

	/* XXX EVP_PKEY_sign(w/o md) -> RSA_private_encrypt */
	if (!EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING))
	    goto exit;

      }	break;
    case PGPPUBKEYALGO_DSA:
	if (!EVP_PKEY_CTX_set_signature_md(ctx, ssl->md))
	    goto exit;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmsslSignELG(dig);
#endif
	goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
#if !defined(OPENSSL_NO_ECDSA)
	if (!EVP_PKEY_CTX_set_signature_md(ctx, ssl->md))
	    goto exit;
      {	EC_KEY * ec = EVP_PKEY_get0(ssl->pkey);
	int xx;

	/* XXX Restore the copy of the private key (which is going AWOL). */
	if (EC_KEY_get0_private_key(ec) == NULL && ssl->priv != NULL) {
	    xx = EC_KEY_set_private_key(ec, ssl->priv);
assert(xx == 1);
	}

      }
#endif
	break;
    }

    if (EVP_PKEY_sign(ctx, NULL, &ssl->siglen, t, nt) == 1
     && (ssl->sig = xmalloc(ssl->siglen)) != NULL
     && EVP_PKEY_sign(ctx, ssl->sig, &ssl->siglen, t, nt) == 1)
	rc = 1;

exit:
    if (rc != 1) {
	ssl->sig = _free(ssl->sig);
	ssl->siglen = 0;
    }
    if (hm)
	hm = _free(hm);
    if (ctx)
	EVP_PKEY_CTX_free(ctx);
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmsslGenerate(pgpDig dig)
{
    rpmssl ssl = dig->impl;
    EVP_PKEY_CTX * ctx = NULL;
    EVP_PKEY * param = NULL;
    BIGNUM * bn = NULL;
    static unsigned long _e = 0x10001;		/* XXX RSA */
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
assert(pubp->pubkey_algo);
assert(sigp->hash_algo);

assert(dig->pubkey_algoN);
assert(dig->hash_algoN);

assert(ssl->pkey == NULL);

    switch (pubp->pubkey_algo) {	/* refactor pubkey_algo to dig? */
    default:
	break;
    case PGPPUBKEYALGO_RSA:
if (ssl->nbits == 0) ssl->nbits = 2048;	/* XXX FIXME */
assert(ssl->nbits);
	if ((ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL)) != NULL
	 && EVP_PKEY_keygen_init(ctx) == 1
	 && (bn = BN_new()) != NULL
	 && BN_set_word(bn, _e) == 1
	 && EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, bn) == 1
	 && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, ssl->nbits) == 1
	 && EVP_PKEY_keygen(ctx, &ssl->pkey) == 1)
	    rc = 1;
	break;
    case PGPPUBKEYALGO_DSA:
	/* XXX Set the no. of qbits based on the digest being used. */
	if (ssl->qbits == 0)
	switch (sigp->hash_algo) {
	default:	/* XXX default */
	case PGPHASHALGO_SHA1:		ssl->qbits = 160;	break;
	case PGPHASHALGO_SHA224:	ssl->qbits = 224;	break;
	case PGPHASHALGO_SHA256:	ssl->qbits = 256;	break;
#ifdef	PAINFUL	/* XXX openssl-1.0.1e-16 permits only {160,224,256} */
	case PGPHASHALGO_SHA384:	ssl->qbits = 384;	break;
	case PGPHASHALGO_SHA512:	ssl->qbits = 512;	break;
#else
	case PGPHASHALGO_SHA384:	ssl->qbits = 256;	break;
	case PGPHASHALGO_SHA512:	ssl->qbits = 256;	break;
#endif
	}
assert(ssl->qbits);

	/* XXX Set the no. of nbits for non-truncated digest in use. */
	if (ssl->nbits == 0)
	switch (ssl->qbits) {
	default:	/* XXX default */
	case 160:	ssl->nbits = 1024;	break;
	case 224:	ssl->nbits = 2048;	break;
#ifdef	PAINFUL
	case 256:	ssl->nbits = 3072;	break;
	case 384:	ssl->nbits = 7680;	break;
	case 512:	ssl->nbits = 15360;	break;
#else
	case 256:	ssl->nbits = 2048;	break;
	case 384:	ssl->nbits = 2048;	ssl->qbits = 256;	break;
	case 512:	ssl->nbits = 2048;	ssl->qbits = 256;	break;
#endif
	}
assert(ssl->nbits);

	if ((ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DSA, NULL)) == NULL
	 || EVP_PKEY_paramgen_init(ctx) != 1
	 || EVP_PKEY_CTX_set_dsa_paramgen_bits(ctx, ssl->nbits) != 1
	 || EVP_PKEY_CTX_ctrl(ctx, EVP_PKEY_DSA, EVP_PKEY_OP_PARAMGEN,
		EVP_PKEY_CTRL_DSA_PARAMGEN_Q_BITS, ssl->qbits, NULL) != 1
	 || EVP_PKEY_paramgen(ctx, &param) != 1)
	    goto exit;
	EVP_PKEY_CTX_free(ctx);
	if ((ctx = EVP_PKEY_CTX_new(param, NULL)) != NULL
	 && EVP_PKEY_keygen_init(ctx) == 1
	 && EVP_PKEY_keygen(ctx, &ssl->pkey) == 1)
	    rc = 1;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmsslGenerateELG(dig);
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
#if !defined(OPENSSL_NO_ECDSA)
	/* XXX Set the no. of bits based on the digest being used. */
	if (ssl->nbits == 0)
	switch (sigp->hash_algo) {
	case PGPHASHALGO_MD5:		ssl->nbits = 128;	break;
	case PGPHASHALGO_TIGER192:	ssl->nbits = 192;	break;
	case PGPHASHALGO_SHA224:	ssl->nbits = 224;	break;
	default:	/* XXX default */
	case PGPHASHALGO_SHA256:	ssl->nbits = 256;	break;
	case PGPHASHALGO_SHA384:	ssl->nbits = 384;	break;
	case PGPHASHALGO_SHA512:	ssl->nbits = 521;	break;
	}
assert(ssl->nbits);

	/* XXX Choose the curve parameters from the no. of digest bits. */
	if (ssl->curveN == NULL)	/* XXX FIXME */
	switch (ssl->nbits) {	/* XXX only NIST prime curves for now */
	default:	goto exit;	/*@notreached@*/ break;
	case 192:
	    ssl->curveN = xstrdup("nistp192"); ssl->nid = 711;
	    break;
	case 224:
	    ssl->curveN = xstrdup("nistp224"); ssl->nid = NID_secp224r1;
	    break;
	case 256:
	    ssl->curveN = xstrdup("nistp256"); ssl->nid = NID_X9_62_prime256v1;
	    break;
	case 384:
	    ssl->curveN = xstrdup("nistp384"); ssl->nid = NID_secp384r1;
	    break;
	case 512:	/* XXX sanity */
	case 521:
	    ssl->curveN = xstrdup("nistp521"); ssl->nid = NID_secp521r1;
	    break;
	}
assert(ssl->curveN);

	if ((ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL)) == NULL
	 || EVP_PKEY_paramgen_init(ctx) != 1
	 || EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, ssl->nid) != 1
	 || EVP_PKEY_paramgen(ctx, &param) != 1)
	    goto exit;
	EVP_PKEY_CTX_free(ctx);
	if ((ctx = EVP_PKEY_CTX_new(param, NULL)) != NULL
	 && EVP_PKEY_keygen_init(ctx) == 1
	 && EVP_PKEY_keygen(ctx, &ssl->pkey) == 1)
	    rc = 1;
	/* XXX Save a copy of the private key (which is going AWOL). */
      {
	EC_KEY * ec = EVP_PKEY_get0(ssl->pkey);
	if (ssl->priv)
	    BN_free(ssl->priv);
	ssl->priv = NULL;
	ssl->priv = BN_dup(EC_KEY_get0_private_key(ec));
      }
#endif
	break;
    }

exit:
    if (rc == 1) {
	/* XXX OpenSSL BUG(iirc): ensure that the asn1 flag is set. */
	if (EVP_PKEY_type(ssl->pkey->type) == EVP_PKEY_EC) {
	    EC_KEY * ec = EVP_PKEY_get1_EC_KEY(ssl->pkey);
	    EC_KEY_set_asn1_flag(ec, OPENSSL_EC_NAMED_CURVE);
	    EC_KEY_free(ec);
	}
    } else {
	if (ssl->pkey)
	    EVP_PKEY_free(ssl->pkey);
	ssl->pkey = NULL;
    }
    if (param)
	EVP_PKEY_free(param);
    if (ctx)
	EVP_PKEY_CTX_free(ctx);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmsslMpiItem(/*@unused@*/ const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p,
		/*@unused@*/ /*@null@*/ const rpmuint8_t * pend)
	/*@*/
{
    rpmssl ssl = (rpmssl) dig->impl;
    unsigned int nb = (pend >= p ? (pend - p) : 0);
    unsigned int mbits = (((8 * (nb - 2)) + 0x1f) & ~0x1f);
    unsigned char * q;
    unsigned int nz;
    int rc = 0;
    int xx;

/*@-moduncon@*/
    switch (itemno) {
    default:
assert(0);
	break;
    case 10:		/* RSA m**d */
assert(ssl->sig == NULL);
	ssl->nbits = mbits;
	ssl->siglen = mbits/8;
	ssl->sig = xmalloc(ssl->siglen);
	nz = ssl->siglen - (nb - 2);
	if (nz)		/* XXX resurrect leading zero bytes. */
	    memset(ssl->sig, 0, nz);
	memcpy(ssl->sig+nz, p+2, nb-2);
	break;
    case 20:		/* DSA r */
assert(ssl->dsasig == NULL);
	ssl->qbits = mbits;
	ssl->dsasig = DSA_SIG_new();
	ssl->dsasig->r = BN_bin2bn(p+2, nb-2, ssl->dsasig->r);
	break;
    case 21:		/* DSA s */
assert(ssl->dsasig != NULL);
assert(mbits == ssl->qbits);
	ssl->dsasig->s = BN_bin2bn(p+2, nb-2, ssl->dsasig->s);
	ssl->siglen = i2d_DSA_SIG(ssl->dsasig, NULL);
	ssl->sig = xmalloc(ssl->siglen);
	q = ssl->sig;
	xx = i2d_DSA_SIG(ssl->dsasig, &q);
assert(xx == (int)ssl->siglen);
	DSA_SIG_free(ssl->dsasig);
	ssl->dsasig = NULL;
	break;
    case 30:		/* RSA n */
assert(ssl->rsa == NULL);
	ssl->nbits = mbits;
	ssl->rsa = RSA_new();
	ssl->rsa->n = BN_bin2bn(p+2, nb-2, ssl->rsa->n);
	break;
    case 31:		/* RSA e */
assert(ssl->rsa != NULL);
	ssl->rsa->e = BN_bin2bn(p+2, nb-2, ssl->rsa->e);
assert(ssl->pkey == NULL);
	ssl->pkey = EVP_PKEY_new();
	xx = EVP_PKEY_assign_RSA(ssl->pkey, ssl->rsa);
assert(xx);
	ssl->rsa = NULL;
	break;
    case 40:		/* DSA p */
assert(ssl->dsa == NULL);
	ssl->nbits = mbits;
	ssl->dsa = DSA_new();
	ssl->dsa->p = BN_bin2bn(p+2, nb-2, ssl->dsa->p);
	break;
    case 41:		/* DSA q */
assert(ssl->dsa != NULL);
	ssl->qbits = mbits;
	ssl->dsa->q = BN_bin2bn(p+2, nb-2, ssl->dsa->q);
	break;
    case 42:		/* DSA g */
assert(ssl->dsa != NULL);
assert(mbits == ssl->nbits);
	ssl->dsa->g = BN_bin2bn(p+2, nb-2, ssl->dsa->g);
	break;
    case 43:		/* DSA y */
assert(ssl->dsa != NULL);
assert(mbits == ssl->nbits);
	ssl->dsa->pub_key = BN_bin2bn(p+2, nb-2, ssl->dsa->pub_key);
assert(ssl->pkey == NULL);
	ssl->pkey = EVP_PKEY_new();
	xx = EVP_PKEY_assign_DSA(ssl->pkey, ssl->dsa);
assert(xx);
	ssl->dsa = NULL;
	break;
    case 50:		/* ECDSA r */
assert(ssl->ecdsasig == NULL);
	ssl->qbits = mbits;
	ssl->ecdsasig = ECDSA_SIG_new();
	ssl->ecdsasig->r = BN_bin2bn(p+2, nb-2, ssl->ecdsasig->r);
	break;
    case 51:		/* ECDSA s */
#if !defined(OPENSSL_NO_ECDSA)
assert(ssl->ecdsasig != NULL);
assert(mbits == ssl->qbits);
	ssl->ecdsasig->s = BN_bin2bn(p+2, nb-2, ssl->ecdsasig->s);
	ssl->siglen = i2d_ECDSA_SIG(ssl->ecdsasig, NULL);
	ssl->sig = xmalloc(ssl->siglen);
	q = ssl->sig;
	xx = i2d_ECDSA_SIG(ssl->ecdsasig, &q);
	ECDSA_SIG_free(ssl->ecdsasig);
	ssl->ecdsasig = NULL;
#endif	/* !OPENSSL_NO_ECDSA */
	break;
    case 60:		/* ECDSA curve OID */
#if !defined(OPENSSL_NO_ECDSA)
	ssl->nid = 0;
    {	size_t nc = EC_get_builtin_curves(NULL, 100);
	EC_builtin_curve * c = alloca(nc * sizeof(*c));
	size_t i;
	(void) EC_get_builtin_curves(c, nc);
	for (i = 0; i < nc; i++) {
	    ASN1_OBJECT * o = OBJ_nid2obj(c[i].nid);
	    if (nb != (unsigned)o->length)
		continue;
	    if (memcmp(p, o->data, nb))
		continue;
	    ssl->curveN = xstrdup(o->sn);
	    ssl->nid = c[i].nid;
	    break;
	}
	switch (ssl->nid) {
	case NID_X9_62_prime192v1:
	    ssl->nbits = 192;
	    break;
	case NID_secp224r1:
	    ssl->nbits = 224;
	    break;
	default:		/* XXX default to NIST P-256 */
	    ssl->curveN = _free(ssl->curveN);
	    ssl->curveN = xstrdup("nistp256");
	    ssl->nid = NID_X9_62_prime256v1;
	    ssl->nbits = 256;	/* coverity #1214083 */
	    break;
	case NID_X9_62_prime256v1:
	    ssl->nbits = 256;
	    break;
	case NID_secp384r1:
	    ssl->nbits = 384;
	    break;
	case NID_secp521r1:
	    ssl->nbits = 521;
	    break;
	}
    }
#else
fprintf(stderr, "      OID[%4u]: %s\n", nb, pgpHexStr(p, nb));
	rc = 1;
#endif	/* !OPENSSL_NO_ECDSA */
	break;
    case 61:		/* ECDSA Q */
	mbits = pgpMpiBits(p);
        nb = pgpMpiLen(p);
#if !defined(OPENSSL_NO_ECDSA)
assert(ssl->nid);
      {	EC_KEY * ec = EC_KEY_new_by_curve_name(ssl->nid);
	const unsigned char *q;

assert(ec);
	q = p+2;
	ec = o2i_ECPublicKey(&ec, &q, nb-2);
assert(ec);

if (ssl->pkey) {
    if (ssl->pkey)
	EVP_PKEY_free(ssl->pkey);
    ssl->pkey = NULL;
}
assert(ssl->pkey == NULL);
	ssl->pkey = EVP_PKEY_new();
	xx = EVP_PKEY_assign_EC_KEY(ssl->pkey, ec);
assert(xx);
#else
fprintf(stderr, "        Q[%4u]: %s\n", mbits, pgpHexStr(p+2, nb-2));
	rc = 1;
#endif	/* !OPENSSL_NO_ECDSA */
      }	break;
    }
/*@=moduncon@*/
    return rc;
}

/*@-mustmod@*/
static
void rpmsslClean(void * impl)
	/*@modifies impl @*/
{
    rpmssl ssl = (rpmssl) impl;
/*@-moduncon@*/
    if (ssl != NULL) {
	ssl->nbits = 0;
	ssl->qbits = 0;
	ssl->err = 0;
	ssl->badok = 0;
	ssl->digest = _free(ssl->digest);
	ssl->digestlen = 0;

	ssl->sig = _free(ssl->sig);
	ssl->siglen = 0;

	if (ssl->dsa)
	    DSA_free(ssl->dsa);
	ssl->dsa = NULL;
	if (ssl->dsasig)
	    DSA_SIG_free(ssl->dsasig);
	ssl->dsasig = NULL;

	if (ssl->rsa)
	    RSA_free(ssl->rsa);
	ssl->rsa = NULL;
	if (ssl->hm)
	    BN_free(ssl->hm);
	ssl->hm = NULL;

	ssl->curveN = _free(ssl->curveN);
	ssl->nid = 0;
	if (ssl->ecdsasig)
	    ECDSA_SIG_free(ssl->ecdsasig);
	ssl->ecdsasig = NULL;
	if (ssl->priv)
	    BN_free(ssl->priv);
	ssl->priv = NULL;

	if (ssl->pkey)
	    EVP_PKEY_free(ssl->pkey);
	ssl->pkey = NULL;
	ssl->md = NULL;

    }
/*@=moduncon@*/
}
/*@=mustmod@*/

/*@unchecked@*/
static int rpmssl_initialized;

static /*@null@*/
void * rpmsslFree(/*@only@*/ void * impl)
	/*@modifies impl @*/
{
    rpmsslClean(impl);

    if (--rpmssl_initialized == 0) {

	CONF_modules_unload(1);
	OBJ_cleanup();
	EVP_cleanup();
	ENGINE_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_thread_state(NULL);
	ERR_free_strings();
	COMP_zlib_cleanup();

    }

    impl = _free(impl);

    return NULL;
}

#ifdef	REFERENCE
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/bn.h>
# include <openssl/md2.h>
# include <openssl/rc4.h>
# include <openssl/des.h>
# include <openssl/idea.h>
# include <openssl/blowfish.h>
#include <openssl/engine.h>
#endif

static const char *rpmsslEngines(char *te)
{
    char *t = te;
    ENGINE *e;

    for (e = ENGINE_get_first(); e != NULL; e = ENGINE_get_next(e))
	te = stpcpy(stpcpy(te, " "), ENGINE_get_id(e));
    *te = '\0';

    return t;
}

static void rpmsslVersionLog(void)
{
    static int oneshot = 0;
    int msglvl = RPMLOG_DEBUG;
    char b[8192];

    if (oneshot++)
	return;

    rpmlog(msglvl, "---------- openssl %s configuration:\n",
	   SSLeay_version(SSLEAY_VERSION));

#ifdef	REDUNDANT
    if (SSLeay() == SSLEAY_VERSION_NUMBER)
	rpmlog(msglvl, "%s\n", SSLeay_version(SSLEAY_VERSION));
    else
	rpmlog(msglvl, "%s (Library: %s)\n",
	       OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));
#endif

    rpmlog(msglvl, "  %s\n", SSLeay_version(SSLEAY_BUILT_ON));
    rpmlog(msglvl, "  %s\n", SSLeay_version(SSLEAY_PLATFORM));
    rpmlog(msglvl, "   options: %s\n", BN_options());
    rpmlog(msglvl, "  %s\n", SSLeay_version(SSLEAY_CFLAGS));
    rpmlog(msglvl, "%s\n", SSLeay_version(SSLEAY_DIR));
    rpmlog(msglvl, "   engines:%s\n", rpmsslEngines(b));
    rpmlog(msglvl, "      FIPS: %s\n",
	(FIPS_mode() ? "enabled" : "disabled"));

#ifdef REFERENCE
{   ASN1_OBJECT * o = OBJ_nid2obj(ssl->nid);
    fprintf(stderr, "   sn: %s\n", o->sn);
    fprintf(stderr, "   ln: %s\n", o->ln);
    fprintf(stderr, "  nid: %d\n", o->nid);
    fprintf(stderr, " data: %p[%u] %s\n", o->data, o->length, pgpHexStr(o->data, o->length));
    fprintf(stderr, "flags: %08X\n", o->flags);
}
#endif

    {	size_t nc = EC_get_builtin_curves(NULL, 100);
	EC_builtin_curve * c = alloca(nc * sizeof(*c));
	size_t i;
	(void) EC_get_builtin_curves(c, nc);
	for (i = 0; i < nc; i++) {
	    ASN1_OBJECT * o = OBJ_nid2obj(c[i].nid);

	    if (i == 0)
		rpmlog(msglvl, " EC curves:\n");
	    rpmlog(msglvl,	"   %s\n", c[i].comment);
	    rpmlog(msglvl,	"   %12s%5d %s\t%s\n",
		o->sn, c[i].nid, pgpHexStr(o->data, o->length), o->ln);
	}
    }

    rpmlog(msglvl, "----------\n");
}

static
void * rpmsslInit(void)
	/*@*/
{
    rpmssl ssl = (rpmssl) xcalloc(1, sizeof(*ssl));

    if (rpmssl_initialized++ == 0) {
	int xx;

/*@-moduncon@*/
#ifdef	NOTYET
	CRYPTO_malloc_init();
#endif
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
	ENGINE_load_builtin_engines();

	xx = FIPS_mode_set(1);

/*@=moduncon@*/

	rpmsslVersionLog();

    }

    return (void *) ssl;
}

struct pgpImplVecs_s rpmsslImplVecs = {
	OPENSSL_VERSION_TEXT,
	rpmsslSetRSA,
	rpmsslSetDSA,
	rpmsslSetELG,
	rpmsslSetECDSA,

	rpmsslErrChk,
	rpmsslAvailableCipher, rpmsslAvailableDigest, rpmsslAvailablePubkey,
	rpmsslVerify, rpmsslSign, rpmsslGenerate,

	rpmsslMpiItem, rpmsslClean,
	rpmsslFree, rpmsslInit
};

int rpmsslExportPubkey(pgpDig dig)
{
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt = now;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    rpmssl ssl = (rpmssl) dig->impl;
    int rc = 0;		/* assume failure */
    int xx;

    *be++ = 0x80 | (PGPTAG_PUBLIC_KEY << 2) | 0x01;
    be += 2;

    *be++ = 0x04;
    *be++ = (bt >> 24);
    *be++ = (bt >> 16);
    *be++ = (bt >>  8);
    *be++ = (bt      );
    *be++ = pubp->pubkey_algo;

assert(ssl->pkey);
    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
      {
	RSA * rsa = EVP_PKEY_get0(ssl->pkey);

	bn = BN_num_bits(rsa->n);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(rsa->n, be);
	be += bn/8;

	bn = BN_num_bits(rsa->e);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(rsa->e, be);
	be += bn/8;
      }	break;
    case PGPPUBKEYALGO_DSA:
      {
	DSA * dsa = EVP_PKEY_get0(ssl->pkey);

	bn = BN_num_bits(dsa->p);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(dsa->p, be);
	be += bn/8;

	bn = BN_num_bits(dsa->q);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(dsa->q, be);
	be += bn/8;

	bn = BN_num_bits(dsa->g);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(dsa->g, be);
	be += bn/8;

	bn = BN_num_bits(dsa->pub_key);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = BN_bn2bin(dsa->pub_key, be);
	be += bn/8;
      }	break;
    case PGPPUBKEYALGO_ECDSA:
      {
	EC_KEY * ec = EVP_PKEY_get0(ssl->pkey);
	ASN1_OBJECT * o = OBJ_nid2obj(ssl->nid);
	unsigned char *q;

	/* OID */
	*be++ = o->length;
	memcpy(be, o->data, o->length);
	be += o->length;

	/* Q */
	/* XXX uncompressed {x,y} starts with 0x04 (i.e. 5 leading zero bits) */
	q = be+2;
	bn = 8 * i2o_ECPublicKey(ec, &q) - 5;
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	be += bn/8;
assert(be == q);

      }	break;
    }

    pktlen = (be - pkt);
    bn = pktlen - 3;
    pkt[1] = (bn >> 8);
    pkt[2] = (bn     );

    xx = pgpPubkeyFingerprint(pkt, pktlen, pubp->signid);

    dig->pub = memcpy(xmalloc(pktlen), pkt, pktlen);
    dig->publen = pktlen;
    rc = 1;

SPEW(!rc, rc, dig);
    return rc;
}

int rpmsslExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
{
    const unsigned char * q;
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    uint8_t * h;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmssl ssl = (rpmssl) dig->impl;
    int rc = 0;		/* assume failure */
    int xx;

    sigp->tag = PGPTAG_SIGNATURE;
    *be++ = 0x80 | (sigp->tag << 2) | 0x01;
    be += 2;				/* pktlen */

    sigp->hash = be;
    *be++ = sigp->version = 0x04;		/* version */
    *be++ = sigp->sigtype = PGPSIGTYPE_BINARY;	/* sigtype */
    *be++ = sigp->pubkey_algo = pubp->pubkey_algo;	/* pubkey_algo */
    *be++ = sigp->hash_algo;		/* hash_algo */

    be += 2;				/* skip hashd length */
    h = (uint8_t *) be;

    *be++ = 1 + 4;			/* signature creation time */
    *be++ = PGPSUBTYPE_SIG_CREATE_TIME;
    bt = now;
    *be++ = sigp->time[0] = (bt >> 24);
    *be++ = sigp->time[1] = (bt >> 16);
    *be++ = sigp->time[2] = (bt >>  8);
    *be++ = sigp->time[3] = (bt      );

    *be++ = 1 + 4;			/* signature expiration time */
    *be++ = PGPSUBTYPE_SIG_EXPIRE_TIME;
    bt = 30 * 24 * 60 * 60;		/* XXX 30 days from creation */
    *be++ = sigp->expire[0] = (bt >> 24);
    *be++ = sigp->expire[1] = (bt >> 16);
    *be++ = sigp->expire[2] = (bt >>  8);
    *be++ = sigp->expire[3] = (bt      );

/* key expiration time (only on a self-signature) */

    *be++ = 1 + 1;			/* exportable certification */
    *be++ = PGPSUBTYPE_EXPORTABLE_CERT;
    *be++ = 0;

    *be++ = 1 + 1;			/* revocable */
    *be++ = PGPSUBTYPE_REVOCABLE;
    *be++ = 0;

/* notation data */

    sigp->hashlen = (be - h);		/* set hashed length */
    h[-2] = (sigp->hashlen >> 8);
    h[-1] = (sigp->hashlen     );
    sigp->hashlen += sizeof(struct pgpPktSigV4_s);

    if (sigp->hash != NULL)
	xx = rpmDigestUpdate(ctx, sigp->hash, sigp->hashlen);

    if (sigp->version == (rpmuint8_t) 4) {
	uint8_t trailer[6];
	trailer[0] = sigp->version;
	trailer[1] = (rpmuint8_t)0xff;
	trailer[2] = (sigp->hashlen >> 24);
	trailer[3] = (sigp->hashlen >> 16);
	trailer[4] = (sigp->hashlen >>  8);
	trailer[5] = (sigp->hashlen      );
	xx = rpmDigestUpdate(ctx, trailer, sizeof(trailer));
    }

    sigp->signhash16[0] = 0x00;
    sigp->signhash16[1] = 0x00;
    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
	xx = pgpImplSetRSA(ctx, dig, sigp); /* XXX signhash16 check fails */
	break;
    case PGPPUBKEYALGO_DSA:
	xx = pgpImplSetDSA(ctx, dig, sigp); /* XXX signhash16 check fails */
	break;
    case PGPPUBKEYALGO_ECDSA:
	xx = pgpImplSetECDSA(ctx, dig, sigp); /* XXX signhash16 check fails */
	break;
    }
    h = (uint8_t *) ssl->digest;
    sigp->signhash16[0] = h[0];
    sigp->signhash16[1] = h[1];

    /* XXX pgpImplVec forces "--usecrypto foo" to also be used */
    xx = pgpImplSign(dig);
assert(xx == 1);

    be += 2;				/* skip unhashed length. */
    h = be;

    *be++ = 1 + 8;			/* issuer key ID */
    *be++ = PGPSUBTYPE_ISSUER_KEYID;
    *be++ = pubp->signid[0];
    *be++ = pubp->signid[1];
    *be++ = pubp->signid[2];
    *be++ = pubp->signid[3];
    *be++ = pubp->signid[4];
    *be++ = pubp->signid[5];
    *be++ = pubp->signid[6];
    *be++ = pubp->signid[7];

    bt = (be - h);			/* set unhashed length */
    h[-2] = (bt >> 8);
    h[-1] = (bt     );

    *be++ = sigp->signhash16[0];	/* signhash16 */
    *be++ = sigp->signhash16[1];

    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
      {	BIGNUM * md = BN_bin2bn(ssl->sig, ssl->siglen, BN_new());
	bn = BN_num_bits(md);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = BN_bn2bin(md, be);
	be += bn/8;
	BN_free(md);
      }	break;
    case PGPPUBKEYALGO_DSA:
assert(ssl->dsasig == NULL);
	q = ssl->sig;
	ssl->dsasig = d2i_DSA_SIG(NULL, &q, ssl->siglen);

	bn = BN_num_bits(ssl->dsasig->r);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = BN_bn2bin(ssl->dsasig->r, be);
	be += bn/8;

	bn = BN_num_bits(ssl->dsasig->s);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = BN_bn2bin(ssl->dsasig->s, be);
	be += bn/8;

	DSA_SIG_free(ssl->dsasig);
	ssl->dsasig = NULL;
	break;
    case PGPPUBKEYALGO_ECDSA:
assert(ssl->ecdsasig == NULL);
	q = ssl->sig;
	ssl->ecdsasig = d2i_ECDSA_SIG(NULL, &q, ssl->siglen);

	bn = BN_num_bits(ssl->ecdsasig->r);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = BN_bn2bin(ssl->ecdsasig->r, be);
	be += bn/8;

	bn = BN_num_bits(ssl->ecdsasig->s);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = BN_bn2bin(ssl->ecdsasig->s, be);
	be += bn/8;

	ECDSA_SIG_free(ssl->ecdsasig);
	ssl->ecdsasig = NULL;
	break;
    }

    pktlen = (be - pkt);		/* packet length */
    bn = pktlen - 3;
    pkt[1] = (bn >> 8);
    pkt[2] = (bn     );

    dig->sig = memcpy(xmalloc(pktlen), pkt, pktlen);
    dig->siglen = pktlen;
    rc = 1;

SPEW(!rc, rc, dig);
    return rc;

}

#endif	/* WITH_SSL */
