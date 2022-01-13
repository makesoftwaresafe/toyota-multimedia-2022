/** \ingroup rpmpgp
 * \file rpmio/rpmltc.c
 */

#include "system.h"
#include <rpmlog.h>

#include <rpmiotypes.h>

#define	_RPMPGP_INTERNAL
#if defined(WITH_TOMCRYPT)
#define	_RPMLTC_INTERNAL
#include <rpmltc.h>
#endif

#include "debug.h"

#if defined(WITH_TOMCRYPT)

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

/*@unchecked@*/
static int _rpmltc_debug;

/*@unchecked@*/
static prng_state prng;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmltc_debug || _pgp_debug < 0) \
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

static
int rpmltcErr(rpmltc ltc, const char * msg, int rc)
        /*@*/
{
    /* XXX FIXME: Don't spew on expected failures ... */
    if (rc != CRYPT_OK) {
        fprintf (stderr, "rpmltc: %s rc(%d) %s\n",
                msg, rc, error_to_string(rc));
    }
    return rc;
}

#ifdef	DYING
#define	_spewBN(_N, _BN)	\
  { mp_int * bn = _BN; \
    int nt = 0; \
    int err = (bn ? mp_radix_size(bn, 16, &nt) : 0); \
    char * t = (nt ? xmalloc(nt) : ""); \
    if (nt) \
	err = mp_toradix(bn, t, 16); \
    fprintf(stderr, "\t" _N ": %s\n", t); \
    if (nt) \
	t = _free(t); \
  }

static void rpmltcDumpRSA(const char * msg, rpmltc ltc)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    {
	_spewBN(" n", ltc->rsa.N);
	_spewBN(" e", ltc->rsa.e);
	_spewBN(" d", ltc->rsa.d);
	_spewBN(" p", ltc->rsa.p);
	_spewBN(" q", ltc->rsa.q);
	_spewBN("dp", ltc->rsa.dP);
	_spewBN("dq", ltc->rsa.dQ);
	_spewBN("qi", ltc->rsa.qP);
    }

    _spewBN(" c", ltc->c);
}

static void rpmltcDumpDSA(const char * msg, rpmltc ltc)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    {
	_spewBN(" p", ltc->dsa.p);
	_spewBN(" q", ltc->dsa.q);
	_spewBN(" g", ltc->dsa.g);
	_spewBN(" x", ltc->dsa.x);
	_spewBN(" y", ltc->dsa.y);
    }

    _spewBN(" r", ltc->r);
    _spewBN(" s", ltc->s);

}

static void rpmltcDumpECDSA(const char * msg, rpmltc ltc)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    {
	const ltc_ecc_set_type * dp = ltc->ecdsa.idx >= 0
		? ltc_ecc_sets + ltc->ecdsa.idx : NULL;
	if (dp == NULL)
	    dp = ltc->ecdsa.dp;

	if (dp) {
	    fprintf(stderr, "\tsize: %d\n", dp->size);
	    fprintf(stderr, "\tname: %s\n", dp->name);
	    fprintf(stderr, "\t p: %s\n", dp->prime);
	    fprintf(stderr, "\t n: %s\n", dp->order);
	    fprintf(stderr, "\t b: %s\n", dp->B);
	    fprintf(stderr, "\tGx: %s\n", dp->Gx);
	    fprintf(stderr, "\tGy: %s\n", dp->Gy);
	}

	_spewBN("Qx", ltc->ecdsa.pubkey.x);
	_spewBN("Qy", ltc->ecdsa.pubkey.y);

	_spewBN(" d", ltc->ecdsa.k);

    }

    _spewBN(" r", ltc->r);
    _spewBN(" s", ltc->s);
}
#endif	/* DYING */

#undef	_spewBN

/*==============================================================*/
static int getHashIdx(unsigned hash_algo)
{
    const char * hashN = NULL;

    switch (hash_algo) {
    case PGPHASHALGO_MD2:	hashN = "md2";          break;
    case PGPHASHALGO_MD4:	hashN = "md4";          break;
    case PGPHASHALGO_MD5:	hashN = "md5";          break;
    case PGPHASHALGO_SHA1:	hashN = "sha1";         break;
    case PGPHASHALGO_SHA224:	hashN = "sha224";       break;
    case PGPHASHALGO_SHA256:	hashN = "sha256";       break;
    case PGPHASHALGO_SHA384:	hashN = "sha384";       break;
    case PGPHASHALGO_SHA512:	hashN = "sha512";       break;
    case PGPHASHALGO_RIPEMD128:	hashN = "rmd128";	break;
    case PGPHASHALGO_RIPEMD160:	hashN = "rmd160";	break;
    case PGPHASHALGO_RIPEMD256:	hashN = "rmd256";	break;
    case PGPHASHALGO_RIPEMD320:	hashN = "rmd320";	break;
    case PGPHASHALGO_TIGER192:	hashN = "tiger";	break;
#ifdef	NOTYET
    case PGPHASHALGO_WHIRLPOOL:	hashN = "whirlpool";	break;
#endif
    case PGPHASHALGO_HAVAL_5_160: /*@fallthrough@*/
    default:
        break;
    }
    return (hashN ? find_hash(hashN) : -1);
}
/*==============================================================*/

#define	_initBN(_t) \
  { if (_t == NULL) _t = xmalloc(sizeof(mp_int)); \
    xx = mp_init(_t); \
  }

static
int rpmltcSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmltc ltc = dig->impl;
    int rc = 1;		/* assume failure */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_RSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_RSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

ltc->digest = _free(ltc->digest);
ltc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ltc->digest, &ltc->digestlen, 0);

    ltc->hashIdx = getHashIdx(sigp->hash_algo);
    if (ltc->hashIdx < 0)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ltc->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmltcSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmltc ltc = dig->impl;
    int rc = 1;		/* assume failure */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_DSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_DSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

ltc->digest = _free(ltc->digest);
ltc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ltc->digest, &ltc->digestlen, 0);

    ltc->hashIdx = getHashIdx(sigp->hash_algo);
    if (ltc->hashIdx < 0)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ltc->digest, sigp->signhash16, sizeof(sigp->signhash16));
    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmltcSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmltc ltc = dig->impl;
    int rc = 1;		/* assume failure. */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

ltc->digest = _free(ltc->digest);
ltc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ltc->digest, &ltc->digestlen, 0);

    ltc->hashIdx = getHashIdx(sigp->hash_algo);
    if (ltc->hashIdx < 0)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ltc->digest, sigp->signhash16, sizeof(sigp->signhash16));
    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

    rc = 1;	/* XXX FIXME: always fail (unimplemented). */

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmltcSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmltc ltc = dig->impl;
    int rc = 1;		/* assume failure. */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

/* XXX FIXME: should this lazy free be done elsewhere? */
ltc->digest = _free(ltc->digest);
ltc->digestlen = 0;
    xx = rpmDigestFinal(ctx, &ltc->digest, &ltc->digestlen, 0);

    ltc->hashIdx = getHashIdx(sigp->hash_algo);
    if (ltc->hashIdx < 0)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(ltc->digest, sigp->signhash16, sizeof(sigp->signhash16));
    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmltcErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	REFERENCE
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmltcStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmltcAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmltcAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmltcAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmltcVerify(pgpDig dig)
{
    rpmltc ltc = dig->impl;
    int rc = 0;		/* assume failure */
    unsigned char sig[4096];	/* XXX big enuf */
    unsigned long siglen = sizeof(sig);
    unsigned int dlen;			/* XXX DSA */
    int _padding = LTC_LTC_PKCS_1_V1_5;	/* XXX RSA */
    unsigned long saltlen = 0;		/* XXX RSA */
    unsigned nz;			/* XXX RSA */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(ltc->digest && ltc->digestlen > 0);

    /* XXX &rc is where valid is returned: return code ususally CRYPT_OK */
    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
assert(ltc->hashIdx >= 0);
	siglen = ltc->nbits/8;
	nz = siglen - mp_unsigned_bin_size(ltc->c);
	if (nz)		/* XXX resurrect leading zero bytes. */
	    memset(sig, 0, nz);
	xx = mp_to_unsigned_bin(ltc->c, sig+nz);
	xx = rpmltcErr(ltc, "rsa_verify_hash_ex",
		rsa_verify_hash_ex(sig, siglen,
			ltc->digest, ltc->digestlen,
			_padding, ltc->hashIdx, saltlen, &rc, &ltc->rsa));
	break;
    case PGPPUBKEYALGO_DSA:
assert(ltc->r && ltc->s);
assert(ltc->qbits);
	/* XXX Truncate to qbits (if necessary) */
	dlen = (ltc->digestlen > ltc->qbits/8 ? ltc->qbits/8 : ltc->digestlen);
	xx = rpmltcErr(ltc, "dsa_verify_hash_raw",
		dsa_verify_hash_raw(ltc->r, ltc->s,
			ltc->digest, dlen, &rc, &ltc->dsa));
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmltcVerifyELG(dig);
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
assert(ltc->r && ltc->s);
	xx = der_encode_sequence_multi(sig, &siglen,
			LTC_ASN1_INTEGER, 1UL, ltc->r,
			LTC_ASN1_INTEGER, 1UL, ltc->s,
			LTC_ASN1_EOL, 0UL, NULL);
	xx = rpmltcErr(ltc, "ecc_verify_hash",
		ecc_verify_hash(sig, siglen,
			ltc->digest, ltc->digestlen, &rc, &ltc->ecdsa));
	break;
    }

SPEW(!rc, rc, dig);
    return rc;
}

static int rpmltcSign(pgpDig dig)
{
    rpmltc ltc = dig->impl;
    int rc = 0;		/* assume failure */
    unsigned char sig[2048];
    unsigned long siglen = sizeof(sig);
    unsigned int dlen;			/* XXX DSA */
    int _padding = LTC_LTC_PKCS_1_V1_5;	/* XXX RSA */
    unsigned long saltlen = 0	;	/* XXX RSA */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);

assert(ltc->digest && ltc->digestlen > 0);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
assert(ltc->hashIdx >= 0);
	rc = rpmltcErr(ltc, "rsa_sign_hash_ex",
		rsa_sign_hash_ex(ltc->digest, ltc->digestlen, sig, &siglen,
			_padding, &prng, ltc->prngIdx,
			ltc->hashIdx, saltlen, &ltc->rsa));
	if (rc == CRYPT_OK) {
	    _initBN(ltc->c);
	    xx = mp_read_unsigned_bin(ltc->c, sig, siglen);
	}
	break;
    case PGPPUBKEYALGO_DSA:
assert(ltc->qbits);
	_initBN(ltc->r);
	_initBN(ltc->s);
	/* XXX Truncate to qbits (if necessary) */
	dlen = (ltc->digestlen > ltc->qbits/8 ? ltc->qbits/8 : ltc->digestlen);
	rc = rpmltcErr(ltc, "dsa_sign_hash_raw",
		dsa_sign_hash_raw(ltc->digest, dlen, ltc->r, ltc->s,
			&prng, ltc->prngIdx, &ltc->dsa));
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmltcSignELG(dig);
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmltcErr(ltc, "ecc_sign_hash",
		ecc_sign_hash(ltc->digest, ltc->digestlen, sig, &siglen,
			&prng, ltc->prngIdx, &ltc->ecdsa));
	if (rc == CRYPT_OK) {
	    _initBN(ltc->r);
	    _initBN(ltc->s);
	    xx = der_decode_sequence_multi(sig, siglen,
				LTC_ASN1_INTEGER, 1UL, ltc->r,
				LTC_ASN1_INTEGER, 1UL, ltc->s,
				LTC_ASN1_EOL, 0UL, NULL);
	}
	break;
    }

    rc = (rc == CRYPT_OK);

SPEW(!rc, rc, dig);
    return rc;
}

static int rpmltcGenerate(pgpDig dig)
{
    rpmltc ltc = dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
assert(pubp->pubkey_algo);
assert(sigp->hash_algo);

assert(dig->pubkey_algoN);
assert(dig->hash_algoN);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
      {
static long _e = 0x10001;	/* XXX FIXME */
if (ltc->nbits == 0) ltc->nbits = 2048;	/* XXX FIXME */
	rc = rpmltcErr(ltc, "rsa_make_key",
		rsa_make_key(&prng, ltc->prngIdx,
			ltc->nbits/8, _e, &ltc->rsa));
	rc = (rc == CRYPT_OK);
      }	break;
    case PGPPUBKEYALGO_DSA:
      {
int xx;
int _group_size;
int _modulus_size;

	/* XXX Set the no. of qbits based on the digest being used. */
	if (ltc->qbits == 0)
	switch (sigp->hash_algo) {
	default:	/* XXX default */
	case PGPHASHALGO_SHA1:		ltc->qbits = 160;	break;
	case PGPHASHALGO_SHA224:	ltc->qbits = 224;	break;
	case PGPHASHALGO_SHA256:	ltc->qbits = 256;	break;
	case PGPHASHALGO_SHA384:	ltc->qbits = 384;	break;
	case PGPHASHALGO_SHA512:	ltc->qbits = 512;	break;
	}
assert(ltc->qbits);

	/* XXX Set the no. of nbits for non-truncated digest in use. */
	if (ltc->nbits == 0)
	switch (ltc->qbits) {
	default:	/* XXX default */
	case 160:	ltc->nbits = 1024;	break;
	case 224:	ltc->nbits = 2048;	break;
#ifdef	PAINFUL
	case 256:	ltc->nbits = 3072;	break;
	case 384:	ltc->nbits = 7680;	break;
	case 512:	ltc->nbits = 15360;	break;
#else
	case 256:	ltc->nbits = 2048;	break;
	case 384:	ltc->nbits = 2048;	ltc->qbits = 256;	break;
	case 512:	ltc->nbits = 2048;	ltc->qbits = 256;	break;
#endif
	}
assert(ltc->nbits);

_group_size = ltc->qbits/8;
_modulus_size = ltc->nbits/8;

	xx = rpmltcErr(ltc, "dsa_make_key",
		dsa_make_key(&prng, ltc->prngIdx,
			_group_size, _modulus_size, &ltc->dsa));

	/* XXX &rc is where valid is returned: return code ususally CRYPT_OK */
	xx = rpmltcErr(ltc, "dsa_verify_key",
		dsa_verify_key(&ltc->dsa, &rc));
      }	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmltcGenerateELG(dig);
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
	/* XXX Set the no. of bits based on the digest being used. */
	if (ltc->nbits == 0)
	switch (sigp->hash_algo) {
	case PGPHASHALGO_MD5:		ltc->nbits = 128;	break;
	case PGPHASHALGO_TIGER192:	ltc->nbits = 192;	break;
	case PGPHASHALGO_SHA224:	ltc->nbits = 224;	break;
	default:	/* XXX default */
	case PGPHASHALGO_SHA256:	ltc->nbits = 256;	break;
	case PGPHASHALGO_SHA384:	ltc->nbits = 384;	break;
	case PGPHASHALGO_SHA512:	ltc->nbits = 521;	break;
	}
assert(ltc->nbits);

	/* XXX use ecc_make_key_ex() instead? */
	rc = rpmltcErr(ltc, "ecc_make_key",
		ecc_make_key(&prng, ltc->prngIdx,
			ltc->nbits/8, &ltc->ecdsa));
	rc = (rc == CRYPT_OK);
	break;
    }

SPEW(!rc, rc, dig);
    return rc;
}

#define	_loadBN(_t, _s, _ns) \
  { int xx; \
    _initBN(_t); \
    xx = mp_read_unsigned_bin(_t, _s, _ns); \
  }

static
int rpmltcMpiItem(/*@unused@*/ const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p,
		/*@unused@*/ /*@null@*/ const rpmuint8_t * pend)
	/*@*/
{
    rpmltc ltc = dig->impl;
    unsigned int  nb = (pend >= p ? (pend - p) : 0);
    unsigned int mbits = (((8 * (nb - 2)) + 0x1f) & ~0x1f);
    int rc = 0;

    switch (itemno) {
    default:
assert(0);
	break;
    case 10:	/* RSA m**d */
	ltc->nbits = mbits;
	_loadBN(ltc->c, p+2, nb-2);
	break;
    case 20:	/* DSA r */
	ltc->qbits = mbits;
	_loadBN(ltc->r, p+2, nb-2);
	break;
    case 21:	/* DSA s */
assert(mbits == ltc->qbits);
	_loadBN(ltc->s, p+2, nb-2);
	break;
    case 30:	/* RSA n */
	ltc->nbits = mbits;
	_loadBN(ltc->rsa.N, p+2, nb-2);
	break;
    case 31:	/* RSA e */
	_loadBN(ltc->rsa.e, p+2, nb-2);
	break;
    case 40:	/* DSA p */
	ltc->nbits = mbits;
	_loadBN(ltc->dsa.p, p+2, nb-2);
	break;
    case 41:	/* DSA q */
	ltc->qbits = mbits;
	_loadBN(ltc->dsa.q, p+2, nb-2);
	break;
    case 42:	/* DSA g */
assert(mbits == ltc->nbits);
	_loadBN(ltc->dsa.g, p+2, nb-2);
	break;
    case 43:	/* DSA y */
assert(mbits == ltc->nbits);
	_loadBN(ltc->dsa.y, p+2, nb-2);
	break;
    case 50:	/* ECDSA r */
	ltc->qbits = mbits;
	_loadBN(ltc->r, p+2, nb-2);
	break;
    case 51:	/* ECDSA s */
assert(mbits == ltc->qbits);
	_loadBN(ltc->s, p+2, nb-2);
	break;
    case 60:	/* ECDSA curve OID */
	{   const char * s = xstrdup(pgpHexStr(p, nb));
	    if (!strcasecmp(s, "2a8648ce3d030101"))
		ltc->nbits = 192;
	    else if (!strcasecmp(s, "2b81040021"))
		ltc->nbits = 224;
	    else if (!strcasecmp(s, "2a8648ce3d030107"))
		ltc->nbits = 256;
	    else if (!strcasecmp(s, "2b81040022"))
		ltc->nbits = 384;
	    else if (!strcasecmp(s, "2b81040023"))
		ltc->nbits = 521;
	    else
		ltc->nbits = 256;	/* XXX FIXME default? */
	    s = _free(s);
	}
assert(ltc->nbits > 0);
	break;
    case 61:	/* ECDSA Q */
	mbits = pgpMpiBits(p);
	nb = pgpMpiLen(p);
	rc = ecc_ansi_x963_import(p+2, nb-2, &ltc->ecdsa);
assert(rc == CRYPT_OK);
	break;
    }

    return rc;
}

#undef	_loadBN

#define	_freeBN(_t) \
    if (_t) { \
	(void) mp_clear(_t); \
	_t = _free(_t); \
    }

static
void rpmltcClean(void * impl)
	/*@modifies impl @*/
{
    rpmltc ltc = impl;
    if (ltc != NULL) {
	ltc->nbits = 0;
	ltc->qbits = 0;
	ltc->err = 0;
	ltc->badok = 0;
	ltc->digest = _free(ltc->digest);
	ltc->digestlen = 0;

	/* XXX rsa_free(&ltc->rsa); */
	_freeBN(ltc->rsa.N);
	_freeBN(ltc->rsa.e);
	_freeBN(ltc->rsa.d);
	_freeBN(ltc->rsa.p);
	_freeBN(ltc->rsa.q);
	_freeBN(ltc->rsa.dP);
	_freeBN(ltc->rsa.dQ);
	_freeBN(ltc->rsa.qP);
	memset(&ltc->rsa, 0, sizeof(ltc->rsa));

	_freeBN(ltc->c);

	/* XXX dsa_free(&ltc->dsa); */
	_freeBN(ltc->dsa.p);
	_freeBN(ltc->dsa.q);
	_freeBN(ltc->dsa.g);
	_freeBN(ltc->dsa.x);
	_freeBN(ltc->dsa.y);
	memset(&ltc->dsa, 0, sizeof(ltc->dsa));

	_freeBN(ltc->r);
	_freeBN(ltc->s);

	ecc_free(&ltc->ecdsa);
	memset(&ltc->ecdsa, 0, sizeof(ltc->ecdsa));

    }
}

#undef	_freeBN

static /*@null@*/
void * rpmltcFree(/*@only@*/ void * impl)
	/*@modifies impl @*/
{
    rpmltcClean(impl);
    impl = _free(impl);
    return NULL;
}

static void reg_algs(rpmltc ltc)
{
  int err;
#ifdef LTC_RIJNDAEL
  register_cipher (&aes_desc);
#endif
#ifdef LTC_BLOWFISH
  register_cipher (&blowfish_desc);
#endif
#ifdef LTC_XTEA
  register_cipher (&xtea_desc);
#endif
#ifdef LTC_RC5
  register_cipher (&rc5_desc);
#endif
#ifdef LTC_RC6
  register_cipher (&rc6_desc);
#endif
#ifdef LTC_SAFERP
  register_cipher (&saferp_desc);
#endif
#ifdef LTC_TWOFISH
  register_cipher (&twofish_desc);
#endif
#ifdef LTC_SAFER
  register_cipher (&safer_k64_desc);
  register_cipher (&safer_sk64_desc);
  register_cipher (&safer_k128_desc);
  register_cipher (&safer_sk128_desc);
#endif
#ifdef LTC_RC2
  register_cipher (&rc2_desc);
#endif
#ifdef LTC_DES
  register_cipher (&des_desc);
  register_cipher (&des3_desc);
#endif
#ifdef LTC_CAST5
  register_cipher (&cast5_desc);
#endif
#ifdef LTC_NOEKEON
  register_cipher (&noekeon_desc);
#endif
#ifdef LTC_SKIPJACK
  register_cipher (&skipjack_desc);
#endif
#ifdef LTC_KHAZAD
  register_cipher (&khazad_desc);
#endif
#ifdef LTC_ANUBIS
  register_cipher (&anubis_desc);
#endif
#ifdef LTC_KSEED
  register_cipher (&kseed_desc);
#endif
#ifdef LTC_KASUMI
  register_cipher (&kasumi_desc);
#endif
#ifdef LTC_MULTI2
  register_cipher (&multi2_desc);
#endif

#ifdef LTC_TIGER
  register_hash (&tiger_desc);
#endif
#ifdef LTC_MD2
  register_hash (&md2_desc);
#endif
#ifdef LTC_MD4
  register_hash (&md4_desc);
#endif
#ifdef LTC_MD5
  register_hash (&md5_desc);
#endif
#ifdef LTC_SHA1
  register_hash (&sha1_desc);
#endif
#ifdef LTC_SHA224
  register_hash (&sha224_desc);
#endif
#ifdef LTC_SHA256
  register_hash (&sha256_desc);
#endif
#ifdef LTC_SHA384
  register_hash (&sha384_desc);
#endif
#ifdef LTC_SHA512
  register_hash (&sha512_desc);
#endif
#ifdef LTC_RIPEMD128
  register_hash (&rmd128_desc);
#endif
#ifdef LTC_RIPEMD160
  register_hash (&rmd160_desc);
#endif
#ifdef LTC_RIPEMD256
  register_hash (&rmd256_desc);
#endif
#ifdef LTC_RIPEMD320
  register_hash (&rmd320_desc);
#endif
#ifdef LTC_WHIRLPOOL
  register_hash (&whirlpool_desc);
#endif
#ifdef LTC_CHC_HASH
  register_hash(&chc_desc);
  err = rpmltcErr(ltc, "chc_register",
		chc_register(register_cipher(&aes_desc)));
  if (err != CRYPT_OK)
     exit(EXIT_FAILURE);
#endif

register_prng(&yarrow_desc);
#ifdef LTC_FORTUNA
register_prng(&fortuna_desc);
#endif
#ifdef LTC_RC4
register_prng(&rc4_desc);
#endif
#ifdef LTC_SOBER128
register_prng(&sober128_desc);
#endif

#ifdef LTC_FORTUNA
  err = rpmltcErr(ltc, "rng_make_prng",
		rng_make_prng(128, find_prng("fortuna"), &prng, NULL));
#else
  err = rpmltcErr(ltc, "rng_make_prng",
		rng_make_prng(128, find_prng("yarrow"), &prng, NULL));
#endif
  if (err != CRYPT_OK)
     exit(EXIT_FAILURE);
   
}
static
void * rpmltcInit(void)
	/*@*/
{
    static int oneshot;
    rpmltc ltc = xcalloc(1, sizeof(*ltc));
    if (!oneshot) {
	reg_algs(ltc);
#if defined(LTM_DESC)
	ltc_mp = ltm_desc;	/* XXX tfm_desc */
#endif
	oneshot++;
    }

#ifdef LTC_FORTUNA
    ltc->prngIdx = find_prng("fortuna");
#else
    ltc->prngIdx = find_prng("yarrow");
#endif

    return (void *) ltc;
}

struct pgpImplVecs_s rpmltcImplVecs = {
	"TomCrypt " SCRYPT,
	rpmltcSetRSA,
	rpmltcSetDSA,
	rpmltcSetELG,
	rpmltcSetECDSA,

	rpmltcErrChk,
	rpmltcAvailableCipher, rpmltcAvailableDigest, rpmltcAvailablePubkey,
	rpmltcVerify, rpmltcSign, rpmltcGenerate,

	rpmltcMpiItem, rpmltcClean,
	rpmltcFree, rpmltcInit
};

int rpmltcExportPubkey(pgpDig dig)
{
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt = now;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    rpmltc ltc = (rpmltc) dig->impl;
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

    switch (pubp->pubkey_algo) {
    default:
assert(0);
        break;
    case PGPPUBKEYALGO_RSA:
	bn = mp_count_bits(ltc->rsa.N);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->rsa.N, be);
	be += bn/8;

	bn = mp_count_bits(ltc->rsa.e);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->rsa.e, be);
	be += bn/8;
        break;
    case PGPPUBKEYALGO_DSA:
	bn = mp_count_bits(ltc->dsa.p);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->dsa.p, be);
	be += bn/8;

	bn = mp_count_bits(ltc->dsa.q);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->dsa.q, be);
	be += bn/8;

	bn = mp_count_bits(ltc->dsa.g);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->dsa.g, be);
	be += bn/8;

	bn = mp_count_bits(ltc->dsa.y);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->dsa.y, be);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_ECDSA:
	/* OID */
	{   const char * s;
	    size_t ns;
	    size_t i;
	    switch (ltc->nbits) {
	    case 192:	s = "2a8648ce3d030101";	break;
	    case 224:	s = "2b81040021";	break;
	    default:	/* XXX FIXME: default? */
	    case 256:	s = "2a8648ce3d030107";	break;
	    case 384:	s = "2b81040022";	break;
	    case 512:	/* XXX sanity */
	    case 521:	s = "2b81040023";	break;
	    }
	    ns = strlen(s);
	    *be++ = ns/2;
	    for (i = 0; i < ns; i += 2)
		*be++ = (nibble(s[i]) << 4) | (nibble(s[i+1]));
	}
	/* Q */
	{   unsigned long nQ = (sizeof(pkt) - (be+2 - pkt));
	    xx = ecc_ansi_x963_export(&ltc->ecdsa, be+2, &nQ);
assert(xx == CRYPT_OK);
	    bn = 8 * nQ;
	/* XXX uncompressed {x,y} starts with 0x04 (i.e. 5 leading zero bits) */
	    bn -= 5;
	    *be++ = (bn >> 8);	*be++ = (bn     );
	    bn += 7; bn &= ~7;
	    be += bn/8;
	}
	break;
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

int rpmltcExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
{
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    uint8_t * h;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmltc ltc = (rpmltc) dig->impl;
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
	xx = pgpImplSetRSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
        break;
    case PGPPUBKEYALGO_DSA:
	xx = pgpImplSetDSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
	break;
    case PGPPUBKEYALGO_ECDSA:
	xx = pgpImplSetECDSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
	break;
    }
    h = (uint8_t *) ltc->digest;
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
	bn = mp_count_bits(ltc->c);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->c, be);
	be += bn/8;
        break;
    case PGPPUBKEYALGO_DSA:
	bn = mp_count_bits(ltc->r);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->r, be);
	be += bn/8;

	bn = mp_count_bits(ltc->s);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->s, be);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_ECDSA:
	bn = mp_count_bits(ltc->r);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->r, be);
	be += bn/8;

	bn = mp_count_bits(ltc->s);
	*be++ = (bn >> 8);
	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	xx = mp_to_unsigned_bin(ltc->s, be);
	be += bn/8;
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

#endif	/* WITH_TOMCRYPT */
