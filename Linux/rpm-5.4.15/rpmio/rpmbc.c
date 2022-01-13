/** \ingroup rpmpgp
 * \file rpmio/rpmbc.c
 */

#include "system.h"
#define	_RPMBC_INTERNAL
#define	_RPMPGP_INTERNAL
#include <rpmbc.h>
#include "debug.h"

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

/*@unchecked@*/
static int _rpmbc_debug;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmbc_debug || _pgp_debug < 0) \
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

#define	_spewMPB(_N, _MPB)	\
  { mpbarrett * mpb = &(_MPB); \
    fprintf(stderr, "    " _N " = [%4u]: ", (unsigned)mpbits(mpb->size, mpb->modl)); mpfprintln(stderr, mpb->size, mpb->modl); \
  }

#define	_spewMPN(_N, _MPN)	\
  { mpnumber * mpn = &(_MPN); \
    fprintf(stderr, "    " _N " = [%4u]: ", (unsigned)mpbits(mpn->size, mpn->data)); mpfprintln(stderr, mpn->size, mpn->data); \
  }

#ifdef	UNUSED
static void rpmbcDumpRSA(const char * msg, rpmbc bc)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    {
	_spewMPB(" n", bc->rsa_keypair.n);
	_spewMPN(" e", bc->rsa_keypair.e);
	_spewMPN(" d", bc->rsa_keypair.d);
	_spewMPB(" p", bc->rsa_keypair.p);
	_spewMPB(" q", bc->rsa_keypair.q);
	_spewMPN("dp", bc->rsa_keypair.dp);
	_spewMPN("dq", bc->rsa_keypair.dq);
	_spewMPN("qi", bc->rsa_keypair.qi);
    }

    _spewMPN(" c", bc->c);
    _spewMPN("md", bc->md);
}

static void rpmbcDumpDSA(const char * msg, rpmbc bc)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    {
	_spewMPB(" p", bc->dsa_keypair.param.p);
	_spewMPB(" q", bc->dsa_keypair.param.q);
	_spewMPN(" g", bc->dsa_keypair.param.g);
	_spewMPN(" y", bc->dsa_keypair.y);
    }

    _spewMPN(" r", bc->r);
    _spewMPN(" s", bc->s);

    _spewMPN("hm", bc->hm);

}
#endif	/* UNUSED */

static
int rpmbcSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmbc bc = (rpmbc) dig->impl;
    size_t nbits = 0;
    size_t nb = 0;
    const char * prefix = rpmDigestASN1(ctx);
    const char * s;
    uint8_t *t, *te;
    int rc = 1;		/* assume failure */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_RSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_RSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
assert(prefix != NULL);

/* XXX FIXME: should this lazy free be done elsewhere? */
bc->digest = _free(bc->digest);
bc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&bc->digest, &bc->digestlen, 0);

    /*
     * The no. of bytes for hash + PKCS1 padding is needed.
     * Either n or c can be used as the size, but different code paths
     * populate n or c indeterminately. So try c, then n,
     * and error if the no. of bytes isn't sane.
     */
    if (bc->md.size)
	nbits = (unsigned) MP_WORDS_TO_BITS(bc->md.size);
    else if (bc->rsa_keypair.n.size)
	nbits = (unsigned) MP_WORDS_TO_BITS(bc->rsa_keypair.n.size);
    nb = (nbits + 7) >> 3;		/* XXX overkill */
    if (nb < 64/8 || nb > 65536/8)	/* XXX generous "sanity" check */
	goto exit;

    /* Add PKCS1 padding */
    t = te = (uint8_t *) xmalloc(nb);
    memset(te, 0xff, nb);
    te[0] = 0x00;
    te[1] = 0x01;
    te += nb - strlen(prefix)/2 - bc->digestlen - 1;
    *te++ = 0x00;
    /* Add digest algorithm ASN1 prefix */
    for (s = prefix; *s; s += 2)
	*te++ = (uint8_t) (nibble(s[0]) << 4) | nibble(s[1]);
    memcpy(te, bc->digest, bc->digestlen);

    mpnfree(&bc->c);	(void) mpnsetbin(&bc->c, t, nb);
    t = _free(t);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(bc->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmbcSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 1;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_DSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_DSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
bc->digest = _free(bc->digest);
bc->digestlen = 0;
    rc = rpmDigestFinal(ctx, (void **)&bc->digest, &bc->digestlen, 0);

    /* XXX Truncate to 160bits. */
    rc = mpnsetbin(&bc->hm, (byte *) bc->digest,
		(bc->digestlen > 160/8 ? 160/8 : bc->digestlen));

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(bc->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
        rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmbcSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 1;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
bc->digest = _free(bc->digest);
bc->digestlen = 0;
    rc = rpmDigestFinal(ctx, (void **)NULL, NULL, 0);

    rc = mpnsetbin(&bc->hm, (byte *) bc->digest, bc->digestlen);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(bc->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

#ifdef	NOTYET
static
int rpmbcGenerateELG(/*@unused@*/pgpDig dig)
	/*@*/
{
static const char P_2048[] = "fd12e8b7e096a28a00fb548035953cf0eba64ceb5dff0f5672d376d59c196da729f6b5586f18e6f3f1a86c73c5b15662f59439613b309e52aa257488619e5f76a7c4c3f7a426bdeac66bf88343482941413cef06256b39c62744dcb97e7b78e36ec6b885b143f6f3ad0a1cd8a5713e338916613892a264d4a47e72b583fbdaf5bce2bbb0097f7e65cbc86d684882e5bb8196d522dcacd6ad00dfbcd8d21613bdb59c485a65a58325d792272c09ad1173e12c98d865adb4c4d676ada79830c58c37c42dff8536e28f148a23f296513816d3dfed0397a3d4d6e1fa24f07e1b01643a68b4274646a3b876e810206eddacea2b9ef7636a1da5880ef654288b857ea3";
static const char P_1024[] = "e64a3deeddb723e2e4db54c2b09567d196367a86b3b302be07e43ffd7f2e016f866de5135e375bdd2fba6ea9b4299010fafa36dc6b02ba3853cceea07ee94bfe30e0cc82a69c73163be26e0c4012dfa0b2839c97d6cd71eee59a303d6177c6a6740ca63bd04c1ba084d6c369dc2fbfaeebe951d58a4824de52b580442d8cae77";

    rpmbc bc = (rpmbc) dig->impl;
    int rc = 0;		/* assume failure */
    int failures = 0;
    int xx;

    xx = dlkp_pInit(&bc->elg_keypair);
    if (xx) failures++;

#ifdef	DYING
    xx = dldp_pInit(&bc->elg_keypair.param);
    if (xx) failures++;
#endif

    switch (bc->nbits) {
#ifdef	NOTYET
    case 2048:
	mpbsethex(&bc->elg_keypair.param.p, P_2048);
	break;
    case 1024:
    case 0:
	mpbsethex(&bc->elg_keypair.param.p, P_1024);
	break;
#endif
    default:
	xx = dldp_pgonMakeSafe(&bc->elg_keypair.param, &bc->rngc, bc->nbits);
	break;
    }
#ifdef NOTYET
    if (bc->elg_keypair.param.q.size == 0) {
	mpnumber q;

	mpnfree(&q);
	/* set q to half of P */
	mpnset(&q, bc->elg_keypair.param.p.size, bc->elg_keypair.param.p.modl);
	mpdivtwo(q.size, q.data);
	mpbset(&bc->elg_keypair.param.q, q.size, q.data);
	/* set r to 2 */
	mpnsetw(&bc->elg_keypair.param.r, 2);

	/* make a generator, order n */
	xx = dldp_pgonGenerator(&bc->elg_keypair.param, &bc->rngc);

    }
#endif
    if (xx) failures++;

    xx = dldp_pPair(&bc->elg_keypair.param, &bc->rngc,
		&bc->elg_keypair.x, &bc->elg_keypair.y);
    if (xx) failures++;

    mpnfree(&bc->r);
    mpnfree(&bc->s);
    mpnfree(&bc->hm);

#ifdef	DYING
    dldp_pFree(&bc->elg_params);
#endif

    dlkp_pFree(&bc->elg_keypair);

    rc = (failures == 0);

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}
#endif	/* NOTYET */

static
int rpmbcSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 1;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    rc = rpmDigestFinal(ctx, (void **)&bc->digest, &bc->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(bc->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmbcErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	REFERENCE
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmbcStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmbcAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmbcAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmbcAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmbcVerify(pgpDig dig)
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rsavrfy(&bc->rsa_keypair.n, &bc->rsa_keypair.e, &bc->md, &bc->c);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = dsavrfy(&bc->dsa_keypair.param.p, &bc->dsa_keypair.param.q,
		&bc->dsa_keypair.param.g, &bc->hm, &bc->dsa_keypair.y,
		&bc->r, &bc->s);
	break;
#ifdef	NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = elgv1vrfy(&bc->elg_keypair.param.p, &bc->elg_keypair.param.n,
		&bc->elg_keypair.param.g, &bc->hm, &bc->elg_keypair.y,
		&bc->r, &bc->s);
	break;
#endif
    case PGPPUBKEYALGO_ECDSA:
	/* XXX fake ECDSA r & s verification */
fprintf(stderr, "warning: %s(ECDSA): skipped (unimplemented)\n", __FUNCTION__);
	rc = 1;
	break;
    }

SPEW(0, rc, dig);	/* XXX FIXME: thkp has known BAD signatures. */
    return rc;
}

static int rpmbcSign(pgpDig dig)
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	mpnfree(&bc->md);
#ifdef	SLOWER
	rc = rsapri(&bc->rsa_keypair.n, &bc->rsa_keypair.d, &bc->c, &bc->md);
#else
	/* XXX RSA w CRT is ~3x-4x faster for signing. */
	rc = rsapricrt(&bc->rsa_keypair.n, &bc->rsa_keypair.p, &bc->rsa_keypair.q,
		&bc->rsa_keypair.dp, &bc->rsa_keypair.dq, &bc->rsa_keypair.qi,
		&bc->c, &bc->md);
#endif
	break;
    case PGPPUBKEYALGO_DSA:
	mpnfree(&bc->r);
	mpnfree(&bc->s);
	rc = dsasign(&bc->dsa_keypair.param.p, &bc->dsa_keypair.param.q,
		&bc->dsa_keypair.param.g, &bc->rngc, &bc->hm,
		&bc->dsa_keypair.x, &bc->r, &bc->s);
	break;
#ifdef	NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = elgv1sign(&bc->elg_keypair.param.p, &bc->elg_keypair.param.n,
		&bc->elg_keypair.param.g, &bc->rngc, &bc->hm,
		&bc->elg_keypair.x, &bc->r, &bc->s);
	break;
#endif
    case PGPPUBKEYALGO_ECDSA:
	mpnfree(&bc->r);
	mpnfree(&bc->s);
	/* XXX fake ECDSA r & s signing */
      {	char hex[2048+1];
	char * te;

	te = hex;
	memset(te, (int)'1', 2*((bc->nbits+7)/8));
	te += 2*((bc->nbits+7)/8);
	*te = 0x0;
	mpnsethex(&bc->r, hex);

	te = hex;
	memset(te, (int)'2', 2*((bc->nbits+7)/8));
	te += 2*((bc->nbits+7)/8);
	*te = 0x0;
	mpnsethex(&bc->s, hex);
fprintf(stderr, "warning: %s(ECDSA): skipped (unimplemented)\n", __FUNCTION__);
	rc = 0;
      }	break;
    }
    rc = (rc == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static int rpmbcGenerate(pgpDig dig)
{
    rpmbc bc = (rpmbc) dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
assert(pubp->pubkey_algo);
assert(sigp->hash_algo);

assert(dig->pubkey_algoN);
assert(dig->hash_algoN);

    if (randomGeneratorContextInit(&bc->rngc, randomGeneratorDefault()))
	goto exit;

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
if (bc->nbits == 0) bc->nbits = 2048;	/* XXX FIXME */
	rsakpFree(&bc->rsa_keypair);
	if (!rsakpMake(&bc->rsa_keypair, &bc->rngc, bc->nbits))
	    rc = 1;
	break;
    case PGPPUBKEYALGO_DSA:
if (bc->nbits == 0) bc->nbits = 1024;	/* XXX FIXME */
	dlkp_pFree(&bc->dsa_keypair);
	if (!dsaparamMake(&bc->dsa_keypair.param, &bc->rngc, bc->nbits)
	 && !dldp_pPair(&bc->dsa_keypair.param, &bc->rngc, &bc->dsa_keypair.x,
		&bc->dsa_keypair.y))
	    rc = 1;
	break;
#ifdef NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmbcGenerateELG(dig);
	break;
#endif
    case PGPPUBKEYALGO_ECDSA:
	/* XXX Set the no. of bits based on the digest being used. */
	if (bc->nbits == 0)
	switch (sigp->hash_algo) {
	case PGPHASHALGO_MD5:		bc->nbits = 128;	break;
	case PGPHASHALGO_TIGER192:	bc->nbits = 192;	break;
	case PGPHASHALGO_SHA224:	bc->nbits = 224;	break;
	default:	/* XXX default */
	case PGPHASHALGO_SHA256:	bc->nbits = 256;	break;
	case PGPHASHALGO_SHA384:	bc->nbits = 384;	break;
	case PGPHASHALGO_SHA512:	bc->nbits = 521;	break;
	}
assert(bc->nbits);

	mpnfree(&bc->Q);
	/* XXX fake ECDSA Q generation */
      {	char hex[2048+1];
	char * te = hex;
	*te++ = '0';
	*te++ = '4';
	memset(te, (int)'5', 2*((bc->nbits+7)/8));
	te += 2*((bc->nbits+7)/8);
	memset(te, (int)'A', 2*((bc->nbits+7)/8));
	te += 2*((bc->nbits+7)/8);
	*te = 0x0;
	mpnsethex(&bc->Q, hex);
fprintf(stderr, "warning: %s(ECDSA): skipped (unimplemented)\n", __FUNCTION__);
	rc = 1;
      }	break;
    }
exit:
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcMpiItem(const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p, /*@null@*/ const rpmuint8_t * pend)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmbc bc = (rpmbc) dig->impl;
    unsigned int nb = (pend >= p ? (pend - p) : 0);
    unsigned int mbits = (((8 * (nb - 2)) + 0x1f) & ~0x1f);
    int rc = 0;

    switch (itemno) {
    default:
assert(0);
    case 10:		/* RSA m**d */
	rc = mpnsetbin(&bc->md, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN("md", bc->md);
	break;
    case 20:		/* DSA r */
	rc = mpnsetbin(&bc->r, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" r", bc->r);
	break;
    case 21:		/* DSA s */
	rc = mpnsetbin(&bc->s, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" s", bc->s);
	break;
    case 30:		/* RSA n */
	rc = mpbsetbin(&bc->rsa_keypair.n, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPB(" n", bc->dsa_keypair.param.n);
	break;
    case 31:		/* RSA e */
	rc = mpnsetbin(&bc->rsa_keypair.e, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" e", bc->rsa_keypair.e);
	break;
    case 40:		/* DSA p */
	rc = mpbsetbin(&bc->dsa_keypair.param.p, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPB(" p", bc->dsa_keypair.param.p);
	break;
    case 41:		/* DSA q */
	rc = mpbsetbin(&bc->dsa_keypair.param.q, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPB(" q", bc->dsa_keypair.param.q);
	break;
    case 42:		/* DSA g */
	rc = mpnsetbin(&bc->dsa_keypair.param.g, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" g", bc->dsa_keypair.param.g);
	break;
    case 43:		/* DSA y */
	rc = mpnsetbin(&bc->dsa_keypair.y, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" y", bc->dsa_keypair.y);
	break;
    case 50:		/* ECDSA r */
	rc = mpnsetbin(&bc->r, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" r", bc->r);
	break;
    case 51:		/* ECDSA s */
	rc = mpnsetbin(&bc->s, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" s", bc->s);
	break;
    case 60:	/* ECDSA curve OID */
	{   const char * s = xstrdup(pgpHexStr(p, nb));
	    if (!strcasecmp(s, "2a8648ce3d030101"))
		bc->nbits = 192;
	    else if (!strcasecmp(s, "2b81040021"))
		bc->nbits = 224;
	    else if (!strcasecmp(s, "2a8648ce3d030107"))
		bc->nbits = 256;
	    else if (!strcasecmp(s, "2b81040022"))
		bc->nbits = 384;
	    else if (!strcasecmp(s, "2b81040023"))
		bc->nbits = 521;
	    else
		bc->nbits = 256;	/* XXX FIXME default? */
	    s = _free(s);
	}
assert(bc->nbits > 0);
	break;
    case 61:	/* ECDSA Q */
	mbits = pgpMpiBits(p);
	nb = pgpMpiLen(p);
	rc = mpnsetbin(&bc->Q, p+2, nb-2);
if (_pgp_debug && _pgp_print)
_spewMPN(" Q", bc->Q);
	break;
    }
    return rc;
}

/*@-mustmod@*/
static
void rpmbcClean(void * impl)
	/*@modifies impl @*/
{
    rpmbc bc = (rpmbc) impl;
    if (bc != NULL) {
        bc->in_fips_mode = 0;
        bc->nbits = 0;
        bc->qbits = 0;
        bc->badok = 0;
        bc->err = 0;

	bc->digest = _free(bc->digest);
	bc->digestlen = 0;

	randomGeneratorContextFree(&bc->rngc);

	rsakpFree(&bc->rsa_keypair);

	dlkp_pFree(&bc->dsa_keypair);

	dlkp_pFree(&bc->elg_keypair);
#ifdef	NOTYET
dldp_pFree(&bc->elg_params);
#endif

	mpnfree(&bc->r);
	mpnfree(&bc->s);
	mpnfree(&bc->hm);

	mpnfree(&bc->c);
	mpnfree(&bc->md);

	mpnfree(&bc->Q);
    }
}
/*@=mustmod@*/

static /*@null@*/
void * rpmbcFree(/*@only@*/ void * impl)
	/*@modifies impl @*/
{
    rpmbcClean(impl);
    impl = _free(impl);
    return NULL;
}

static
void * rpmbcInit(void)
	/*@*/
{
    rpmbc bc = (rpmbc) xcalloc(1, sizeof(*bc));
    return (void *) bc;
}

struct pgpImplVecs_s rpmbcImplVecs = {
	"BeeCrypt 4.2.1",	/* XXX FIXME: add version string to beecrypt */
	rpmbcSetRSA,
	rpmbcSetDSA,
	rpmbcSetELG,
	rpmbcSetECDSA,

	rpmbcErrChk,
	rpmbcAvailableCipher, rpmbcAvailableDigest, rpmbcAvailablePubkey,
	rpmbcVerify, rpmbcSign, rpmbcGenerate,

	rpmbcMpiItem, rpmbcClean,
	rpmbcFree, rpmbcInit
};

int rpmbcExportPubkey(pgpDig dig)
{
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt = now;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    rpmbc bc = (rpmbc) dig->impl;
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
	bn = mpbits(bc->rsa_keypair.n.size, bc->rsa_keypair.n.modl);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->rsa_keypair.n.modl, bc->rsa_keypair.n.size);
	be += bn/8;

	bn = mpbits(bc->rsa_keypair.e.size, bc->rsa_keypair.e.data);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->rsa_keypair.e.data, bc->rsa_keypair.e.size);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_DSA:
	bn = mpbits(bc->dsa_keypair.param.p.size, bc->dsa_keypair.param.p.modl);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->dsa_keypair.param.p.modl, bc->dsa_keypair.param.p.size);
	be += bn/8;

	bn = mpbits(bc->dsa_keypair.param.q.size, bc->dsa_keypair.param.q.modl);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->dsa_keypair.param.q.modl, bc->dsa_keypair.param.q.size);
	be += bn/8;

	bn = mpbits(bc->dsa_keypair.param.g.size, bc->dsa_keypair.param.g.data);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->dsa_keypair.param.g.data, bc->dsa_keypair.param.g.size);
	be += bn/8;

	bn = mpbits(bc->dsa_keypair.y.size, bc->dsa_keypair.y.data);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->dsa_keypair.y.data, bc->dsa_keypair.y.size);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_ECDSA:
	/* OID */
	{   const char * s;
	    size_t ns;
	    size_t i;
	    switch (bc->nbits) {
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
	bn = mpbits(bc->Q.size, bc->Q.data);
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	xx = i2osp(be, bn/8, bc->Q.data, bc->Q.size);
	be += bn/8;
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

int rpmbcExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
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
    rpmbc bc = (rpmbc) dig->impl;
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

    be += 2;				/* skip hashed length */
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
    h = (uint8_t *) bc->digest;
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
	bn = mpbits(bc->md.size, bc->md.data);
	bn += 7;	bn &= ~7;
	*be++ = (bn >> 8);
	*be++ = (bn     );
	xx = i2osp(be, bn/8, bc->md.data, bc->md.size);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_DSA:
	bn = mpbits(bc->r.size, bc->r.data);
	bn += 7;	bn &= ~7;
	*be++ = (bn >> 8);
	*be++ = (bn     );
	xx = i2osp(be, bn/8, bc->r.data, bc->r.size);
	be += bn/8;

	bn = mpbits(bc->s.size, bc->s.data);
	bn += 7;	bn &= ~7;
	*be++ = (bn >> 8);
	*be++ = (bn     );
	xx = i2osp(be, bn/8, bc->s.data, bc->s.size);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_ECDSA:
	bn = mpbits(bc->r.size, bc->r.data);
	bn += 7;	bn &= ~7;
	*be++ = (bn >> 8);
	*be++ = (bn     );
	xx = i2osp(be, bn/8, bc->r.data, bc->r.size);
	be += bn/8;

	bn = mpbits(bc->s.size, bc->s.data);
	bn += 7;	bn &= ~7;
	*be++ = (bn >> 8);
	*be++ = (bn     );
	xx = i2osp(be, bn/8, bc->s.data, bc->s.size);
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
    return 0;

}
