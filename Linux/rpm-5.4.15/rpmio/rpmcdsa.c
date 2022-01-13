/** \ingroup rpmpgp
 * \file rpmio/rpmcdsa.c
 */

#include "system.h"
#include <rpmlog.h>

#include <rpmiotypes.h>
#define	_RPMPGP_INTERNAL
#if defined(WITH_CDSA)

#define	_RPMCDSA_INTERNAL
#include <rpmcdsa.h>
#endif

#include "debug.h"

#if defined(WITH_CDSA)

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

/*@unchecked@*/
static int _rpmcdsa_debug = 1;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmcdsa_debug || _pgp_debug < 0) \
	fprintf(stderr, "<-- %s(%p) %s\t%s\n", __FUNCTION__, (_dig), \
		((_rc) ? "OK" : "BAD"), (_dig)->pubkey_algoN); \
  }

static const char * rpmcdsaHashAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpHashTbl, (rpmuint8_t)algo);
}

static const char * rpmcdsaPubkeyAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpPubkeyTbl, (rpmuint8_t)algo);
}

static
int rpmcdsaErr(rpmcdsa cdsa, const char * msg, CSSM_RETURN rc)
        /*@*/
{
    /* XXX FIXME: Don't spew on expected failures ... */
    if (rc != CSSM_OK)
	cssmPerror(msg, rc);
    return rc;
}

static
int rpmcdsaSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmcdsa cdsa = dig->impl;
    int rc;
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = rpmcdsaPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = rpmcdsaHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

/* XXX FIXME: should this lazy free be done elsewhere? */
cdsa->digest = _free(cdsa->digest);
cdsa->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&cdsa->digest, &cdsa->digestlen, 1);

    /* Compare leading 16 bits of digest for quick check. */
rc = 0;

SPEW(0, !rc, dig);
    return rc;
}

static
int rpmcdsaSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmcdsa cdsa = dig->impl;
    int rc;
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = rpmcdsaPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = rpmcdsaHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    /* Set DSA hash. */
/* XXX FIXME: should this lazy free be done elsewhere? */
cdsa->digest = _free(cdsa->digest);
cdsa->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&cdsa->digest, &cdsa->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(cdsa->digest, sigp->signhash16, sizeof(sigp->signhash16));

SPEW(0, !rc, dig);
    return rc;
}

static
int rpmcdsaSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmcdsa cdsa = dig->impl;
    int rc = 1;		/* XXX always fail. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

/* XXX FIXME: should this lazy free be done elsewhere? */
cdsa->digest = _free(cdsa->digest);
cdsa->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&cdsa->digest, &cdsa->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(cdsa->digest, sigp->signhash16, sizeof(sigp->signhash16));

SPEW(rc, !rc, dig);
    return rc;
}

static
int rpmcdsaSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmcdsa cdsa = dig->impl;
    int rc = 1;		/* assume failure. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

/* XXX FIXME: should this lazy free be done elsewhere? */
cdsa->digest = _free(cdsa->digest);
cdsa->digestlen = 0;
    xx = rpmDigestFinal(ctx, &cdsa->digest, &cdsa->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(cdsa->digest, sigp->signhash16, sizeof(sigp->signhash16));

SPEW(rc, !rc, dig);
    return rc;
}

static int rpmcdsaErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	REFERENCE
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmcdsaStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmcdsaAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmcdsaAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmcdsaAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgcAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmcdsaVerify(pgpDig dig)
{
    rpmcdsa cdsa = dig->impl;
    int rc = 0;		/* assume failure */
int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = rpmcdsaPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = rpmcdsaHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	goto exit;
	break;
    case PGPPUBKEYALGO_RSA:
	cdsa->algid = CSSM_ALGID_RSA;
	break;
    case PGPPUBKEYALGO_DSA:
	cdsa->algid = CSSM_ALGID_DSA;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
	cdsa->algid = CSSM_ALGID_ECDSA;
	break;
    }

    cdsa->ccHandle = CSSM_INVALID_HANDLE;
    xx = rpmcdsaErr(cdsa, "CSSM_CSP_CreateSignatureContext",
		CSSM_CSP_CreateSignatureContext(cdsa->cspHandle,
				cdsa->algid,
				cdsa->AccessCred,
				&cdsa->pub_key,
				&cdsa->ccHandle));

CSSM_DATA hashData = { .Length = cdsa->digestlen, .Data = cdsa->digest };
uint32_t hashDataCount = 1;
CSSM_ALGORITHMS DigestAlgorithm	= CSSM_ALGID_SHA1;

    xx = rpmcdsaErr(cdsa, "CSSM_VerifyData",
		CSSM_VerifyData(cdsa->ccHandle,
				&hashData,
				hashDataCount,
				DigestAlgorithm,
				&cdsa->signature));

    rc = (xx == CSSM_OK);

    xx = rpmcdsaErr(cdsa, "CSSM_DeleteContext",
		CSSM_DeleteContext(cdsa->ccHandle));
    cdsa->ccHandle = CSSM_INVALID_HANDLE;

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmcdsaSign(pgpDig dig)
{
    rpmcdsa cdsa = dig->impl;
    int rc = 0;		/* assume failure */
int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = rpmcdsaPubkeyAlgo2Name(pubp->pubkey_algo);

    switch (pubp->pubkey_algo) {
    default:
	goto exit;
	break;
    case PGPPUBKEYALGO_RSA:
	cdsa->algid = CSSM_ALGID_RSA;
	break;
    case PGPPUBKEYALGO_DSA:
	cdsa->algid = CSSM_ALGID_DSA;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
	cdsa->algid = CSSM_ALGID_ECDSA;
	break;
    }

    cdsa->ccHandle = CSSM_INVALID_HANDLE;
    xx = rpmcdsaErr(cdsa, "CSSM_CSP_CreateSignatureContext",
		CSSM_CSP_CreateSignatureContext(cdsa->cspHandle,
				cdsa->algid,
				cdsa->AccessCred,
				&cdsa->sec_key,
				&cdsa->ccHandle));

CSSM_DATA hashData = { .Length = cdsa->digestlen, .Data = cdsa->digest };
uint32_t hashDataCount = 1;
CSSM_ALGORITHMS DigestAlgorithm	= CSSM_ALGID_SHA1;

    xx = rpmcdsaErr(cdsa, "CSSM_SignData",
		CSSM_SignData(cdsa->ccHandle,
				&hashData,
				hashDataCount,
				DigestAlgorithm,
				&cdsa->signature));

    rc = (xx == CSSM_OK);

fprintf(stderr, "\tsignature %p[%u]\n", cdsa->signature.Data, (unsigned)cdsa->signature.Length);

    xx = rpmcdsaErr(cdsa, "CSSM_DeleteContext",
		CSSM_DeleteContext(cdsa->ccHandle));
    cdsa->ccHandle = CSSM_INVALID_HANDLE;

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmcdsaGenerate(pgpDig dig)
{
    rpmcdsa cdsa = dig->impl;
    int rc = 0;		/* assume failure */
int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = rpmcdsaPubkeyAlgo2Name(pubp->pubkey_algo);

    memset(&cdsa->sec_key, 0, sizeof(cdsa->sec_key));
    memset(&cdsa->pub_key, 0, sizeof(cdsa->pub_key));

    switch (pubp->pubkey_algo) {
    default:
	goto exit;
	break;
    case PGPPUBKEYALGO_RSA:
if (cdsa->nbits == 0) cdsa->nbits = 1024;	/* XXX FIXME */
	cdsa->algid = CSSM_ALGID_RSA;
	break;
    case PGPPUBKEYALGO_DSA:
if (cdsa->nbits == 0) cdsa->nbits = 1024;	/* XXX FIXME */
	cdsa->algid = CSSM_ALGID_DSA;
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
if (cdsa->nbits == 0) cdsa->nbits = 256;	/* XXX FIXME */
	cdsa->algid = CSSM_ALGID_ECDSA;
	break;
    }

    cdsa->ccHandle = CSSM_INVALID_HANDLE;
    xx = rpmcdsaErr(cdsa, "CSSM_CSP_CreateKeyGenContext",
		CSSM_CSP_CreateKeyGenContext(cdsa->cspHandle,
				cdsa->algid,
				cdsa->nbits,
				cdsa->Seed,
				cdsa->Salt,
				cdsa->StartDate,
				cdsa->EndDate,
				cdsa->Params,
				&cdsa->ccHandle));

    switch (pubp->pubkey_algo) {
    case PGPPUBKEYALGO_RSA:
    default:
	break;
    case PGPPUBKEYALGO_DSA:
    case PGPPUBKEYALGO_ECDSA:		/* XXX FIXME */
    case PGPPUBKEYALGO_ELGAMAL:
	{   CSSM_DATA dummy = { 0, NULL };
	    xx = rpmcdsaErr(cdsa, "CSSM_GenerateAlgorithmParams",
		CSSM_GenerateAlgorithmParams(cdsa->ccHandle, 
				cdsa->nbits,
				&dummy));
	}
	break;
    }

    xx = rpmcdsaErr(cdsa, "CSSM_GenerateKeyPair",
		CSSM_GenerateKeyPair(cdsa->ccHandle,
				cdsa->PublicKeyUsage,
				cdsa->PublicKeyAttr,
				&cdsa->PublicKeyLabel,
				&cdsa->pub_key,
				cdsa->PrivateKeyUsage,
				cdsa->PrivateKeyAttr,
				&cdsa->PrivateKeyLabel,
				cdsa->CredAndAclEntry,
				&cdsa->sec_key));

    rc = (xx == CSSM_OK);

fprintf(stderr, "\tsec_key %p[%u]\n", cdsa->sec_key.KeyData.Data, (unsigned)cdsa->sec_key.KeyData.Length);
fprintf(stderr, "\tpub_key %p[%u]\n", cdsa->pub_key.KeyData.Data, (unsigned)cdsa->pub_key.KeyData.Length);

    xx = rpmcdsaErr(cdsa, "CSSM_DeleteContext",
		CSSM_DeleteContext(cdsa->ccHandle));
    cdsa->ccHandle = CSSM_INVALID_HANDLE;

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmcdsaMpiItem(/*@unused@*/ const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p,
		/*@unused@*/ /*@null@*/ const rpmuint8_t * pend)
	/*@*/
{
    rpmcdsa cdsa = dig->impl;
    unsigned int nb = ((pgpMpiBits(p) + 7) >> 3);
    int rc = 0;

    switch (itemno) {
    default:
assert(0);
    case 50:		/* ECDSA r */
    case 51:		/* ECDSA s */
    case 60:		/* ECDSA curve OID */
    case 61:		/* ECDSA Q */
	break;
    case 10:		/* RSA m**d */
	break;
    case 20:		/* DSA r */
	break;
    case 21:		/* DSA s */
	break;
    case 30:		/* RSA n */
	break;
    case 31:		/* RSA e */
	break;
    case 40:		/* DSA p */
	break;
    case 41:		/* DSA q */
	break;
    case 42:		/* DSA g */
	break;
    case 43:		/* DSA y */
	break;
    }
    return rc;
}

static
void rpmcdsaClean(void * impl)
	/*@modifies impl @*/
{
    rpmcdsa cdsa = impl;
    if (cdsa != NULL) {
	cdsa->nbits = 0;
	cdsa->err = 0;
	cdsa->badok = 0;
	cdsa->digest = _free(cdsa->digest);
	cdsa->digestlen = 0;

    }
}

static /*@null@*/
void * rpmcdsaFree(/*@only@*/ void * impl)
	/*@modifies impl @*/
{
    rpmcdsa cdsa = impl;
int xx;

    rpmcdsaClean(impl);

    xx = rpmcdsaErr(cdsa, "CSSM_ModuleDetach",
		CSSM_ModuleDetach(cdsa->cspHandle));
    cdsa->cspHandle = 0;

    xx = rpmcdsaErr(cdsa, "CSSM_ModuleUnload",
		CSSM_ModuleUnload(cdsa->ModuleGuid,
				cdsa->AppNotifyCallback,
				cdsa->AppNotifyCallbackCtx));
    cdsa->ModuleGuid = NULL;

    /* XXX FIXME: paired or oneshot */
    xx = rpmcdsaErr(cdsa, "CSSM_Terminate",
		CSSM_Terminate());

    impl = _free(impl);

    return NULL;
}

/*
 * Standard app-level memory functions required by CDSA.
 */
static void * appMalloc (uint32 size, void *allocRef)
{
    return( malloc(size) );
}
static void appFree (void *mem_ptr, void *allocRef)
{
    free(mem_ptr);
    return;
}
static void * appRealloc (void *ptr, uint32 size, void *allocRef)
{
    return( realloc( ptr, size ) );
}
static void * appCalloc (uint32 num, uint32 size, void *allocRef)
{
    return( calloc( num, size ) );
}
 
static CSSM_API_MEMORY_FUNCS memFuncs = {
    (CSSM_MALLOC) appMalloc,
    (CSSM_FREE) appFree,
    (CSSM_REALLOC) appRealloc,
    (CSSM_CALLOC) appCalloc,
    NULL
 };

static const CSSM_VERSION Version	= { 2, 0 };
static const CSSM_GUID CallerGuid	= { 0xFADE, 0, 0, { 1,2,3,4,5,6,7,0 } };

static
void * rpmcdsaInit(void)
	/*@*/
{
    rpmcdsa cdsa = xcalloc(1, sizeof(*cdsa));
int xx;

CSSM_PRIVILEGE_SCOPE Scope	= CSSM_PRIVILEGE_SCOPE_NONE;
CSSM_KEY_HIERARCHY KeyHierarchy	= CSSM_KEY_HIERARCHY_NONE;
CSSM_PVC_MODE PvcPolicy		= CSSM_PVC_NONE;

    /* XXX FIXME: paired or oneshot */
xx = rpmcdsaErr(cdsa, "CSSM_Init",
		CSSM_Init(&Version,
				Scope,
				&CallerGuid,
				KeyHierarchy,
				&PvcPolicy,
				cdsa->Reserved));

cdsa->ModuleGuid = &gGuidAppleCSP;
xx = rpmcdsaErr(cdsa, "CSSM_ModuleLoad",
		CSSM_ModuleLoad(cdsa->ModuleGuid,
				KeyHierarchy,
				cdsa->AppNotifyCallback,
				cdsa->AppNotifyCallbackCtx));

uint32 SubserviceID		= 0;
CSSM_SERVICE_TYPE SubServiceType= CSSM_SERVICE_CSP;
CSSM_ATTACH_FLAGS AttachFlags	= 0;

xx = rpmcdsaErr(cdsa, "CSSM_ModuleAttach",
		CSSM_ModuleAttach(cdsa->ModuleGuid,
				&Version,
				&memFuncs,
				SubserviceID,
				SubServiceType,
				AttachFlags,
				KeyHierarchy,
				cdsa->FunctionTable,
				cdsa->NumFunctionTable,
				cdsa->Reserved,
				&cdsa->cspHandle));

cdsa->PublicKeyUsage	= CSSM_KEYUSE_ENCRYPT | CSSM_KEYUSE_VERIFY;
cdsa->PublicKeyAttr	= CSSM_KEYATTR_RETURN_DATA | CSSM_KEYATTR_EXTRACTABLE;
cdsa->PublicKeyLabel.Length	= 8;
cdsa->PublicKeyLabel.Data	= (uint8 *) "tempKey";
cdsa->PrivateKeyUsage	= CSSM_KEYUSE_DECRYPT | CSSM_KEYUSE_SIGN;
cdsa->PrivateKeyAttr	= CSSM_KEYATTR_RETURN_DATA | CSSM_KEYATTR_EXTRACTABLE;
cdsa->PrivateKeyLabel.Length	= 8;
cdsa->PrivateKeyLabel.Data	= (uint8 *) "tempKey";
cdsa->CredAndAclEntry	= NULL;

    return (void *) cdsa;
}

struct pgpImplVecs_s rpmcdsaImplVecs = {
	rpmcdsaSetRSA,
	rpmcdsaSetDSA,
	rpmcdsaSetELG,
	rpmcdsaSetECDSA,

	rpmcdsaErrChk,
	rpmcdsaAvailableCipher, rpmcdsaAvailableDigest, rpmcdsaAvailablePubkey,
	rpmcdsaVerify, rpmcdsaSign, rpmcdsaGenerate,

	rpmcdsaMpiItem, rpmcdsaClean,
	rpmcdsaFree, rpmcdsaInit
};

#endif	/* WITH_CDSA */

