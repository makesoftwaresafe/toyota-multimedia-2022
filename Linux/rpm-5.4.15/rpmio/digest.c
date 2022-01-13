/** \ingroup signature
 * \file rpmio/digest.c
 */

#include "system.h"

#include "rpmio_internal.h"

#include <rpmbc.h>

#include "crc.h"

#include "arirang.h"

#include "blake.h"
#include "blake2-rpm.h"

#include "bmw.h"

#include "chi.h"

#include "cubehash.h"

#include "echo.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "edon-r.h"

#include "fugue.h"

#include "groestl.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "hamsi.h"

#include "jh.h"

#include "keccak.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "lane.h"

#include "luffa.h"

#include "md2.h"
#include "md6.h"

#include "radiogatun.h"

#include "shabal.h"

#include "shavite3.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "simd.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "salsa10.h"
#include "salsa20.h"

#include "skein.h"

#include "tib3.h"
#undef	BitSequence
#undef	DataLength
#undef	HashReturn
#undef	hashState
#undef	Init
#undef	Update
#undef	Final
#undef	Hash

#include "tiger.h"

#include "debug.h"

/*@unchecked@*/
int _ctx_debug = 0;

#ifdef	_DIGEST_DEBUG
#define	DPRINTF(_a)	if (_ctx_debug < 0) fprintf _a
#else
#define	DPRINTF(_a)
#endif

/* Include Bob Jenkins lookup3 hash */
#define	_JLU3_jlu32l
#include "lookup3.c"

/*@access DIGEST_CTX@*/

/**
 * Digest private data.
 */
struct DIGEST_CTX_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@observer@*/
    const char * name;		/*!< Digest name. */
    size_t paramsize;		/*!< No. bytes of digest parameters. */
    size_t blocksize;		/*!< No. bytes in block of plaintext data. */
    size_t digestsize;		/*!< No. bytes of digest. */
    int (*Reset) (void * param)
	/*@modifies param @*/;	/*!< Digest initialize. */
    int (*Update) (void * param, const byte * data, size_t size)
	/*@modifies param @*/;	/*!< Digest update. */
    int (*Digest) (void * param, /*@out@*/ byte * digest)
	/*@modifies param, digest @*/;	/*!< Digest finish. */
    pgpHashAlgo hashalgo;	/*!< RFC 2440/4880 hash algorithm id. */
    rpmDigestFlags flags;	/*!< Bit(s) to control digest operation. */
/*@observer@*/ /*@null@*/
    const char * asn1;		/*!< RFC 3447 ASN1 oid string (in hex). */
    void * param;		/*!< Digest parameters. */
    void * salt;		/*!< Key salt[2*blocksize] */
};

static void ctxFini(void * _ctx)
	/*@modifies _ctx @*/
{
    DIGEST_CTX ctx = (DIGEST_CTX) _ctx;
    if (ctx->param != NULL && ctx->paramsize > 0)
	memset(ctx->param, 0, ctx->paramsize);	/* In case it's sensitive */
    ctx->param = _free(ctx->param);
    if (ctx->salt != NULL && ctx->blocksize > 0)
	memset(ctx->salt, 0, 2*ctx->paramsize);	/* In case it's sensitive */
    ctx->salt = _free(ctx->salt);
    ctx->name = NULL;
    ctx->paramsize = 0;
    ctx->blocksize = 0;
    ctx->digestsize = 0;
    ctx->Reset = NULL;
    ctx->Update = NULL;
    ctx->Digest = NULL;
    ctx->hashalgo = (pgpHashAlgo)0;
    ctx->flags = (rpmDigestFlags)0;
    ctx->asn1 = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _ctxPool;

static DIGEST_CTX ctxGetPool(rpmioPool pool)
{
    DIGEST_CTX ctx;

    if (_ctxPool == NULL) {
ANNOTATE_BENIGN_RACE(&_ctxPool, "");
	_ctxPool = rpmioNewPool("ctx", sizeof(*ctx), -1, _ctx_debug,
			NULL, NULL, ctxFini);
	pool = _ctxPool;
    }
    ctx = (DIGEST_CTX) rpmioGetPool(pool, sizeof(*ctx));
    memset(((char *)ctx)+sizeof(ctx->_item), 0, sizeof(*ctx)-sizeof(ctx->_item));
    return ctx;
}

pgpHashAlgo rpmDigestAlgo(DIGEST_CTX ctx)
{
    return (ctx != NULL ? ctx->hashalgo : PGPHASHALGO_NONE);
}

rpmDigestFlags rpmDigestF(DIGEST_CTX ctx)
{
    return (ctx != NULL ? ctx->flags : RPMDIGEST_NONE);
}

const char * rpmDigestName(DIGEST_CTX ctx)
{
    return (ctx != NULL ? ctx->name : "UNKNOWN");
}

const char * rpmDigestASN1(DIGEST_CTX ctx)
{
    return (ctx != NULL ? ctx->asn1 : NULL);
}

DIGEST_CTX
rpmDigestDup(DIGEST_CTX octx)
{
    DIGEST_CTX nctx = ctxGetPool(_ctxPool);

    nctx->name = octx->name;
    nctx->digestsize = octx->digestsize;
    nctx->blocksize = octx->blocksize;
    nctx->paramsize = octx->paramsize;
    nctx->Reset = octx->Reset;
    nctx->Update = octx->Update;
    nctx->Digest = octx->Digest;
    nctx->hashalgo = octx->hashalgo;
    nctx->flags = octx->flags;
    nctx->asn1 = octx->asn1;
    nctx->param = (octx->param != NULL && octx->paramsize > 0)
	    ? memcpy(DRD_xmalloc(nctx->paramsize), octx->param, nctx->paramsize)
	    : NULL;
    nctx->salt = (octx->salt != NULL && octx->blocksize > 0)
	    ? memcpy(DRD_xmalloc(nctx->blocksize), octx->salt, nctx->blocksize)
	    : NULL;
    return (DIGEST_CTX)rpmioLinkPoolItem((rpmioItem)nctx, __FUNCTION__, __FILE__, __LINE__);
}

static int noopReset(void * param)
{
    return 0;
}

/* XXX impedance match bytes -> bits length. */
static int md6_Update(void * param, const byte * _data, size_t _len)
{
    return md6_update((md6_state *)param, (unsigned char *) _data, (rpmuint64_t)(8 * _len));
}

DIGEST_CTX
rpmDigestInit(pgpHashAlgo hashalgo, rpmDigestFlags flags)
{
    DIGEST_CTX ctx = ctxGetPool(_ctxPool);
    int xx;

    ctx->name = "";
    ctx->paramsize = 0;
    ctx->blocksize = 64;
    ctx->digestsize = 0;
    ctx->Reset = NULL;
    ctx->Update = NULL;
    ctx->Digest = NULL;
    ctx->hashalgo = hashalgo;
    ctx->flags = flags;
    ctx->asn1 = NULL;
    ctx->param = NULL;
    ctx->salt = NULL;

    switch (hashalgo) {
    case PGPHASHALGO_MD5:
	ctx->name = "MD5";
	ctx->digestsize = 128/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(md5Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) md5Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) md5Update;
	ctx->Digest = (int (*)(void *, byte *)) md5Digest;
/*@=type@*/
	ctx->asn1 = "3020300c06082a864886f70d020505000410";
	break;
    case PGPHASHALGO_SHA1:
	ctx->name = "SHA1";
	ctx->digestsize = 160/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(sha1Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sha1Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sha1Update;
	ctx->Digest = (int (*)(void *, byte *)) sha1Digest;
/*@=type@*/
	ctx->asn1 = "3021300906052b0e03021a05000414";
	break;
    case PGPHASHALGO_RIPEMD128:
	ctx->name = "RIPEMD128";
	ctx->digestsize = 128/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(ripemd128Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) ripemd128Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) ripemd128Update;
	ctx->Digest = (int (*)(void *, byte *)) ripemd128Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_RIPEMD160:
	ctx->name = "RIPEMD160";
	ctx->digestsize = 160/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(ripemd160Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) ripemd160Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) ripemd160Update;
	ctx->Digest = (int (*)(void *, byte *)) ripemd160Digest;
/*@=type@*/
	ctx->asn1 = "3021300906052b2403020105000414";
	break;
    case PGPHASHALGO_RIPEMD256:
	ctx->name = "RIPEMD256";
	ctx->digestsize = 256/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(ripemd256Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) ripemd256Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) ripemd256Update;
	ctx->Digest = (int (*)(void *, byte *)) ripemd256Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_RIPEMD320:
	ctx->name = "RIPEMD320";
	ctx->digestsize = 320/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(ripemd320Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) ripemd320Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) ripemd320Update;
	ctx->Digest = (int (*)(void *, byte *)) ripemd320Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_SALSA10:
	ctx->name = "SALSA10";
	ctx->digestsize = 512/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(salsa10Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) salsa10Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) salsa10Update;
	ctx->Digest = (int (*)(void *, byte *)) salsa10Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_SALSA20:
	ctx->name = "SALSA20";
	ctx->digestsize = 512/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(salsa20Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) salsa20Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) salsa20Update;
	ctx->Digest = (int (*)(void *, byte *)) salsa20Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_TIGER192:
	ctx->name = "TIGER192";
	ctx->digestsize = 192/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(tigerParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) tigerReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) tigerUpdate;
	ctx->Digest = (int (*)(void *, byte *)) tigerDigest;
/*@=type@*/
	ctx->asn1 = "3029300d06092b06010401da470c0205000418";
	break;
    case PGPHASHALGO_MD2:
	ctx->name = "MD2";
	ctx->digestsize = 128/8;
	ctx->blocksize = 16;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(md2Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) md2Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) md2Update;
	ctx->Digest = (int (*)(void *, byte *)) md2Digest;
/*@=type@*/
	ctx->asn1 = "3020300c06082a864886f70d020205000410";
	break;
    case PGPHASHALGO_MD4:
	ctx->name = "MD4";
	ctx->digestsize = 128/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(md4Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) md4Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) md4Update;
	ctx->Digest = (int (*)(void *, byte *)) md4Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_CRC32:
	ctx->name = "CRC32";
	ctx->digestsize = 32/8;
	ctx->blocksize = 8;
	{   sum32Param * mp = (sum32Param *) DRD_xcalloc(1, sizeof(*mp));
/*@-type @*/
	    mp->update = (rpmuint32_t (*)(rpmuint32_t, const byte *, size_t)) __crc32;
	    mp->combine = (rpmuint32_t (*)(rpmuint32_t, rpmuint32_t, size_t)) __crc32_combine;
/*@=type @*/
	    ctx->paramsize = sizeof(*mp);
	    ctx->param = mp;
	}
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sum32Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sum32Update;
	ctx->Digest = (int (*)(void *, byte *)) sum32Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_ADLER32:
	ctx->name = "ADLER32";
	ctx->digestsize = 32/8;
	ctx->blocksize = 8;
	{   sum32Param * mp = (sum32Param *) DRD_xcalloc(1, sizeof(*mp));
/*@-type @*/
	    mp->update = (rpmuint32_t (*)(rpmuint32_t, const byte *, size_t)) __adler32;
	    mp->combine = (rpmuint32_t (*)(rpmuint32_t, rpmuint32_t, size_t)) __adler32_combine;
/*@=type @*/
	    ctx->paramsize = sizeof(*mp);
	    ctx->param = mp;
	}
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sum32Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sum32Update;
	ctx->Digest = (int (*)(void *, byte *)) sum32Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_JLU32:
	ctx->name = "JLU32";
	ctx->digestsize = 32/8;
	ctx->blocksize = 8;
	{   sum32Param * mp = (sum32Param *) DRD_xcalloc(1, sizeof(*mp));
/*@-type @*/
	    mp->update = (rpmuint32_t (*)(rpmuint32_t, const byte *, size_t)) jlu32l;
/*@=type @*/
	    ctx->paramsize = sizeof(*mp);
	    ctx->param = mp;
	}
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sum32Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sum32Update;
	ctx->Digest = (int (*)(void *, byte *)) sum32Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_CRC64:
	ctx->name = "CRC64";
	ctx->digestsize = 64/8;
	ctx->blocksize = 8;
	{   sum64Param * mp = (sum64Param *) DRD_xcalloc(1, sizeof(*mp));
/*@-type@*/
	    mp->update = (rpmuint64_t (*)(rpmuint64_t, const byte *, size_t)) __crc64;
	    mp->combine = (rpmuint64_t (*)(rpmuint64_t, rpmuint64_t, size_t)) __crc64_combine;
/*@=type@*/
	    ctx->paramsize = sizeof(*mp);
	    ctx->param = mp;
	}
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sum64Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sum64Update;
	ctx->Digest = (int (*)(void *, byte *)) sum64Digest;
/*@=type@*/
	break;
    case PGPHASHALGO_SHA224:
	ctx->name = "SHA224";
	ctx->digestsize = 224/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(sha224Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sha224Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sha224Update;
	ctx->Digest = (int (*)(void *, byte *)) sha224Digest;
/*@=type@*/
	ctx->asn1 = "302d300d06096086480165030402040500041C";
	break;
    case PGPHASHALGO_SHA256:
	ctx->name = "SHA256";
	ctx->digestsize = 256/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(sha256Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sha256Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sha256Update;
	ctx->Digest = (int (*)(void *, byte *)) sha256Digest;
/*@=type@*/
	ctx->asn1 = "3031300d060960864801650304020105000420";
	break;
    case PGPHASHALGO_SHA384:
	ctx->name = "SHA384";
	ctx->digestsize = 384/8;
	ctx->blocksize = 128;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(sha384Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sha384Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sha384Update;
	ctx->Digest = (int (*)(void *, byte *)) sha384Digest;
/*@=type@*/
	ctx->asn1 = "3041300d060960864801650304020205000430";
	break;
    case PGPHASHALGO_SHA512:
	ctx->name = "SHA512";
	ctx->digestsize = 512/8;
	ctx->blocksize = 128;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(sha512Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
/*@-type@*/
	ctx->Reset = (int (*)(void *)) sha512Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) sha512Update;
	ctx->Digest = (int (*)(void *, byte *)) sha512Digest;
/*@=type@*/
	ctx->asn1 = "3051300d060960864801650304020305000440";
	break;
    case PGPHASHALGO_SKEIN_224: ctx->digestsize = 224/8; goto skein256;
    case PGPHASHALGO_SKEIN_256: ctx->digestsize = 256/8; goto skein256;
skein256:
	ctx->name = "SKEIN256";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(Skein_256_Ctxt_t);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) Skein_256_Init((Skein_256_Ctxt_t *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) Skein_256_Update;
	ctx->Digest = (int (*)(void *, byte *)) Skein_256_Final;
	break;
    case PGPHASHALGO_SKEIN_384: ctx->digestsize = 384/8; goto skein512;
    case PGPHASHALGO_SKEIN_512: ctx->digestsize = 512/8; goto skein512;
skein512:
	ctx->name = "SKEIN512";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(Skein_512_Ctxt_t);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) Skein_512_Init((Skein_512_Ctxt_t *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) Skein_512_Update;
	ctx->Digest = (int (*)(void *, byte *)) Skein_512_Final;
	break;
    case PGPHASHALGO_SKEIN_1024:
	ctx->name = "SKEIN1024";
	ctx->digestsize = 1024/8;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(Skein1024_Ctxt_t);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) Skein1024_Init((Skein1024_Ctxt_t *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) Skein1024_Update;
	ctx->Digest = (int (*)(void *, byte *)) Skein1024_Final;
	break;
    case PGPHASHALGO_ARIRANG_224: ctx->digestsize = 224/8; goto arirang;
    case PGPHASHALGO_ARIRANG_256: ctx->digestsize = 256/8; goto arirang;
    case PGPHASHALGO_ARIRANG_384: ctx->digestsize = 384/8; goto arirang;
    case PGPHASHALGO_ARIRANG_512: ctx->digestsize = 512/8; goto arirang;
arirang:
	ctx->name = "ARIRANG";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(arirangParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) arirangInit((arirangParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) arirangReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) arirangUpdate;
	ctx->Digest = (int (*)(void *, byte *)) arirangDigest;
	break;
    case PGPHASHALGO_BLAKE_224: ctx->digestsize = 224/8; goto blake;
    case PGPHASHALGO_BLAKE_256: ctx->digestsize = 256/8; goto blake;
    case PGPHASHALGO_BLAKE_384: ctx->digestsize = 384/8; goto blake;
    case PGPHASHALGO_BLAKE_512: ctx->digestsize = 512/8; goto blake;
blake:
	ctx->name = "BLAKE";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(blakeParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) blakeInit((blakeParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) blakeReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) blakeUpdate;
	ctx->Digest = (int (*)(void *, byte *)) blakeDigest;
	break;
    case PGPHASHALGO_BLAKE2B:
	ctx->name = "BLAKE2B";
	ctx->blocksize = 8 * BLAKE2B_BLOCKBYTES;
	ctx->digestsize = BLAKE2B_OUTBYTES;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(blake2bParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) blake2bInit((blake2bParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) blake2bReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) blake2bUpdate;
	ctx->Digest = (int (*)(void *, byte *)) blake2bDigest;
	break;
    case PGPHASHALGO_BLAKE2BP:
	ctx->name = "BLAKE2BP";
	ctx->blocksize = 8 * BLAKE2B_BLOCKBYTES;
	ctx->digestsize = BLAKE2B_OUTBYTES;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(blake2bpParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) blake2bpInit((blake2bpParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) blake2bpReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) blake2bpUpdate;
	ctx->Digest = (int (*)(void *, byte *)) blake2bpDigest;
	break;
    case PGPHASHALGO_BLAKE2S:
	ctx->name = "BLAKE2S";
	ctx->blocksize = 8 * BLAKE2S_BLOCKBYTES;
	ctx->digestsize = BLAKE2S_OUTBYTES;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(blake2sParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) blake2sInit((blake2sParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) blake2sReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) blake2sUpdate;
	ctx->Digest = (int (*)(void *, byte *)) blake2sDigest;
	break;
    case PGPHASHALGO_BLAKE2SP:
	ctx->name = "BLAKE2SP";
	ctx->blocksize = 8 * BLAKE2S_BLOCKBYTES;
	ctx->digestsize = BLAKE2S_OUTBYTES;
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(blake2spParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) blake2spInit((blake2spParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) blake2spReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) blake2spUpdate;
	ctx->Digest = (int (*)(void *, byte *)) blake2spDigest;
	break;
    case PGPHASHALGO_BMW_224: ctx->digestsize = 224/8; goto bmw;
    case PGPHASHALGO_BMW_256: ctx->digestsize = 256/8; goto bmw;
    case PGPHASHALGO_BMW_384: ctx->digestsize = 384/8; goto bmw;
    case PGPHASHALGO_BMW_512: ctx->digestsize = 512/8; goto bmw;
bmw:
	ctx->name = "BMW";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(bmwParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) bmwInit((bmwParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) bmwReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) bmwUpdate;
	ctx->Digest = (int (*)(void *, byte *)) bmwDigest;
	break;
    case PGPHASHALGO_CHI_224: ctx->digestsize = 224/8; goto chi;
    case PGPHASHALGO_CHI_256: ctx->digestsize = 256/8; goto chi;
    case PGPHASHALGO_CHI_384: ctx->digestsize = 384/8; goto chi;
    case PGPHASHALGO_CHI_512: ctx->digestsize = 512/8; goto chi;
chi:
	ctx->name = "CHI";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(chiParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) chiInit((chiParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) chiReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) chiUpdate;
	ctx->Digest = (int (*)(void *, byte *)) chiDigest;
	break;
    case PGPHASHALGO_CUBEHASH_224: ctx->digestsize = 224/8; goto cubehash;
    case PGPHASHALGO_CUBEHASH_256: ctx->digestsize = 256/8; goto cubehash;
    case PGPHASHALGO_CUBEHASH_384: ctx->digestsize = 384/8; goto cubehash;
    case PGPHASHALGO_CUBEHASH_512: ctx->digestsize = 512/8; goto cubehash;
cubehash:
	ctx->name = "CUBEHASH";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(cubehashParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) cubehashInit((cubehashParam *)ctx->param, (int)(8 * ctx->digestsize),
				(int)((ctx->flags >> 8) & 0xff),
				(int)((ctx->flags     ) & 0xff));
	ctx->Reset = (int (*)(void *)) cubehashReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) cubehashUpdate;
	ctx->Digest = (int (*)(void *, byte *)) cubehashDigest;
	break;
    case PGPHASHALGO_ECHO_224: ctx->digestsize = 224/8; goto echo;
    case PGPHASHALGO_ECHO_256: ctx->digestsize = 256/8; goto echo;
    case PGPHASHALGO_ECHO_384: ctx->digestsize = 384/8; goto echo;
    case PGPHASHALGO_ECHO_512: ctx->digestsize = 512/8; goto echo;
echo:
	ctx->name = "ECHO";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(echo_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) echo_Init((echo_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _echo_Update;
	ctx->Digest = (int (*)(void *, byte *)) echo_Final;
	break;
    case PGPHASHALGO_EDONR_224: ctx->digestsize = 224/8; goto edonr;
    case PGPHASHALGO_EDONR_256: ctx->digestsize = 256/8; goto edonr;
    case PGPHASHALGO_EDONR_384: ctx->digestsize = 384/8; goto edonr;
    case PGPHASHALGO_EDONR_512: ctx->digestsize = 512/8; goto edonr;
edonr:
	ctx->name = "EDON-R";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(edonr_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) edonr_Init((edonr_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) edonr_Update;
	ctx->Digest = (int (*)(void *, byte *)) edonr_Final;
	break;
    case PGPHASHALGO_FUGUE_224: ctx->digestsize = 224/8; goto fugue;
    case PGPHASHALGO_FUGUE_256: ctx->digestsize = 256/8; goto fugue;
    case PGPHASHALGO_FUGUE_384: ctx->digestsize = 384/8; goto fugue;
    case PGPHASHALGO_FUGUE_512: ctx->digestsize = 512/8; goto fugue;
fugue:
	ctx->name = "FUGUE";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(fugueParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) fugueInit((fugueParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) fugueReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) fugueUpdate;
	ctx->Digest = (int (*)(void *, byte *)) fugueDigest;
	break;
    case PGPHASHALGO_GROESTL_224: ctx->digestsize = 224/8; goto groestl;
    case PGPHASHALGO_GROESTL_256: ctx->digestsize = 256/8; goto groestl;
    case PGPHASHALGO_GROESTL_384: ctx->digestsize = 384/8; goto groestl;
    case PGPHASHALGO_GROESTL_512: ctx->digestsize = 512/8; goto groestl;
groestl:
	ctx->name = "GROESTL";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(groestl_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) groestl_Init((groestl_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _groestl_Update;
	ctx->Digest = (int (*)(void *, byte *)) groestl_Final;
	break;
    case PGPHASHALGO_HAMSI_224: ctx->digestsize = 224/8; goto hamsi;
    case PGPHASHALGO_HAMSI_256: ctx->digestsize = 256/8; goto hamsi;
    case PGPHASHALGO_HAMSI_384: ctx->digestsize = 384/8; goto hamsi;
    case PGPHASHALGO_HAMSI_512: ctx->digestsize = 512/8; goto hamsi;
hamsi:
	ctx->name = "HAMSI";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(hamsiParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) hamsiInit((hamsiParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) hamsiReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) hamsiUpdate;
	ctx->Digest = (int (*)(void *, byte *)) hamsiDigest;
	break;
    case PGPHASHALGO_JH_224: ctx->digestsize = 224/8; goto jh;
    case PGPHASHALGO_JH_256: ctx->digestsize = 256/8; goto jh;
    case PGPHASHALGO_JH_384: ctx->digestsize = 384/8; goto jh;
    case PGPHASHALGO_JH_512: ctx->digestsize = 512/8; goto jh;
jh:
	ctx->name = "JH";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(jhParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) jhInit((jhParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) jhReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) jhUpdate;
	ctx->Digest = (int (*)(void *, byte *)) jhDigest;
	break;
    case PGPHASHALGO_KECCAK_224: ctx->digestsize = 224/8; goto keccak;
    case PGPHASHALGO_KECCAK_256: ctx->digestsize = 256/8; goto keccak;
    case PGPHASHALGO_KECCAK_384: ctx->digestsize = 384/8; goto keccak;
    case PGPHASHALGO_KECCAK_512: ctx->digestsize = 512/8; goto keccak;
keccak:
	ctx->name = "KECCAK";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(keccak_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) keccak_Init((keccak_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _keccak_Update;
	ctx->Digest = (int (*)(void *, byte *)) keccak_Final;
	break;
    case PGPHASHALGO_LANE_224: ctx->digestsize = 224/8; goto lane;
    case PGPHASHALGO_LANE_256: ctx->digestsize = 256/8; goto lane;
    case PGPHASHALGO_LANE_384: ctx->digestsize = 384/8; goto lane;
    case PGPHASHALGO_LANE_512: ctx->digestsize = 512/8; goto lane;
lane:
	ctx->name = "LANE";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(laneParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) laneInit((laneParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) laneReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) laneUpdate;
	ctx->Digest = (int (*)(void *, byte *)) laneDigest;
	break;
    case PGPHASHALGO_LUFFA_224: ctx->digestsize = 224/8; goto luffa;
    case PGPHASHALGO_LUFFA_256: ctx->digestsize = 256/8; goto luffa;
    case PGPHASHALGO_LUFFA_384: ctx->digestsize = 384/8; goto luffa;
    case PGPHASHALGO_LUFFA_512: ctx->digestsize = 512/8; goto luffa;
luffa:
	ctx->name = "LUFFA";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(luffaParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) luffaInit((luffaParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) luffaReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) luffaUpdate;
	ctx->Digest = (int (*)(void *, byte *)) luffaDigest;
	break;
    case PGPHASHALGO_MD6_224: ctx->digestsize = 224/8; goto md6;
    case PGPHASHALGO_MD6_256: ctx->digestsize = 256/8; goto md6;
    case PGPHASHALGO_MD6_384: ctx->digestsize = 384/8; goto md6;
    case PGPHASHALGO_MD6_512: ctx->digestsize = 512/8; goto md6;
md6:
	ctx->name = "MD6";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(md6_state);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	{   int d = (8 * ctx->digestsize);	/* no. of bits in digest */
	    int L = md6_default_L;		/* no. of parallel passes */
	    unsigned char *K = NULL;		/* key */
	    int keylen = 0;			/* key length (bytes) */
	    int r = md6_default_r(d, keylen);	/* no. of rounds */

	    if (ctx->flags != 0) {
		r = ((ctx->flags >> 8) & 0xffff);
		L = ((ctx->flags     ) & 0xff);
		if (r <= 0 || r > 255) r = md6_default_r(d, keylen);
	    }
	    (void) md6_full_init((md6_state *)ctx->param,
			d, K, keylen, L, r);
	}
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) md6_Update;
	ctx->Digest = (int (*)(void *, byte *)) md6_final;
	break;

    case PGPHASHALGO_RG32_256: ctx->digestsize = 256/8;
	ctx->name = "RG32";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(rg32Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) rg32Init((rg32Param *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) rg32Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) rg32Update;
	ctx->Digest = (int (*)(void *, byte *)) rg32Digest;
	break;
    case PGPHASHALGO_RG64_256: ctx->digestsize = 256/8;
	ctx->name = "RG64";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(rg64Param);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) rg64Init((rg64Param *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) rg64Reset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) rg64Update;
	ctx->Digest = (int (*)(void *, byte *)) rg64Digest;
	break;

#ifdef	NOTYET
    case PGPHASHALGO_SHABAL_192: ctx->digestsize = 192/8; goto shabal;
#endif
    case PGPHASHALGO_SHABAL_224: ctx->digestsize = 224/8; goto shabal;
    case PGPHASHALGO_SHABAL_256: ctx->digestsize = 256/8; goto shabal;
    case PGPHASHALGO_SHABAL_384: ctx->digestsize = 384/8; goto shabal;
    case PGPHASHALGO_SHABAL_512: ctx->digestsize = 512/8; goto shabal;
shabal:
	ctx->name = "SHABAL";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(shabalParam);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) shabalInit((shabalParam *)ctx->param, (int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) shabalReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) shabalUpdate;
	ctx->Digest = (int (*)(void *, byte *)) shabalDigest;
	break;
    case PGPHASHALGO_SHAVITE3_224: ctx->digestsize = 224/8; goto shavite3;
    case PGPHASHALGO_SHAVITE3_256: ctx->digestsize = 256/8; goto shavite3;
    case PGPHASHALGO_SHAVITE3_384: ctx->digestsize = 384/8; goto shavite3;
    case PGPHASHALGO_SHAVITE3_512: ctx->digestsize = 512/8; goto shavite3;
shavite3:
	ctx->name = "SHAVITE3";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(shavite3_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) shavite3_Init((shavite3_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _shavite3_Update;
	ctx->Digest = (int (*)(void *, byte *)) shavite3_Final;
	break;
    case PGPHASHALGO_SIMD_224: ctx->digestsize = 224/8; goto simd;
    case PGPHASHALGO_SIMD_256: ctx->digestsize = 256/8; goto simd;
    case PGPHASHALGO_SIMD_384: ctx->digestsize = 384/8; goto simd;
    case PGPHASHALGO_SIMD_512: ctx->digestsize = 512/8; goto simd;
simd:
	ctx->name = "SIMD";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(simd_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) simd_Init((simd_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _simd_Update;
	ctx->Digest = (int (*)(void *, byte *)) simd_Final;
	break;
    case PGPHASHALGO_TIB3_224: ctx->digestsize = 224/8; goto tib3;
    case PGPHASHALGO_TIB3_256: ctx->digestsize = 256/8; goto tib3;
    case PGPHASHALGO_TIB3_384: ctx->digestsize = 384/8; goto tib3;
    case PGPHASHALGO_TIB3_512: ctx->digestsize = 512/8; goto tib3;
tib3:
	ctx->name = "TIB3";
/*@-sizeoftype@*/ /* FIX: union, not void pointer */
	ctx->paramsize = sizeof(tib3_hashState);
/*@=sizeoftype@*/
	ctx->param = DRD_xcalloc(1, ctx->paramsize);
	(void) tib3_Init((tib3_hashState *)ctx->param,
				(int)(8 * ctx->digestsize));
	ctx->Reset = (int (*)(void *)) noopReset;
	ctx->Update = (int (*)(void *, const byte *, size_t)) _tib3_Update;
	ctx->Digest = (int (*)(void *, byte *)) tib3_Final;
	break;
    case PGPHASHALGO_HAVAL_5_160:
    default:
	(void)rpmioFreePoolItem((rpmioItem)ctx, __FUNCTION__, __FILE__, __LINE__);
	return NULL;
	/*@notreached@*/ break;
    }

    xx = (*ctx->Reset) (ctx->param);

DPRINTF((stderr, "==> ctx %p ==== Init(%s, %x) param %p\n", ctx, ctx->name, flags, ctx->param));
    return (DIGEST_CTX)rpmioLinkPoolItem((rpmioItem)ctx, __FUNCTION__, __FILE__, __LINE__);
}

/*@-mustmod@*/ /* LCL: ctx->param may be modified, but ctx is abstract @*/
int
rpmDigestUpdate(DIGEST_CTX ctx, const void * data, size_t len)
{
    if (ctx == NULL)
	return -1;

DPRINTF((stderr, "==> ctx %p ==== Update(%s,%p[%u]) param %p\n", ctx, ctx->name, data, (unsigned)len, ctx->param));
    return (*ctx->Update) (ctx->param, (byte *)data, len);
}
/*@=mustmod@*/

#define	HMAC_IPAD	0x36
#define	HMAC_OPAD	0x5c

int
rpmDigestFinal(DIGEST_CTX ctx, void * datap, size_t *lenp, int asAscii)
{
    byte * digest;
    char * t;

    if (ctx == NULL)
	return -1;
    digest = (byte *) DRD_xmalloc(ctx->digestsize);

DPRINTF((stderr, "==> ctx %p ==== Final(%s,%p,%p,%d) param %p digest %p[%u]\n", ctx, ctx->name, datap, lenp, asAscii, ctx->param, digest, (unsigned)ctx->digestsize));
/*@-noeffectuncon@*/ /* FIX: check rc */
    (void) (*ctx->Digest) (ctx->param, digest);
/*@=noeffectuncon@*/

    /* If keyed HMAC, re-hash with key material. */
    if (ctx->salt != NULL) {
	DIGEST_CTX kctx = rpmDigestInit(ctx->hashalgo, RPMDIGEST_NONE);
	byte * salt = (byte *) ctx->salt;
	byte * kdigest = NULL;
	size_t kdigestlen = 0;
	unsigned i;
	for (i = 0; i < ctx->blocksize; i++)
	    salt[i] ^= HMAC_OPAD;
	rpmDigestUpdate(kctx, ctx->salt, ctx->blocksize);
	ctx->salt = _free(ctx->salt);
	rpmDigestUpdate(kctx, digest, ctx->digestsize);
	(void) rpmDigestFinal(kctx, &kdigest, &kdigestlen, 0);
	memcpy(digest, kdigest, kdigestlen);
	kdigest = _free(kdigest);
    }

    /* Return final digest. */
    if (!asAscii) {
	if (lenp) *lenp = ctx->digestsize;
	if (datap) {
	    *(byte **)datap = digest;
	    digest = NULL;
	}
    } else {
	if (lenp) *lenp = (2*ctx->digestsize);
	if (datap) {
	    const byte * s = (const byte *) digest;
	    static const char hex[] = "0123456789abcdef";
	    size_t i;

	    *(char **)datap = t = (char *) DRD_xmalloc((2*ctx->digestsize) + 1);
	    for (i = 0 ; i < ctx->digestsize; i++) {
		*t++ = hex[ (unsigned)((*s >> 4) & 0x0f) ];
		*t++ = hex[ (unsigned)((*s++   ) & 0x0f) ];
	    }
	    *t = '\0';
	}
    }
    if (digest) {
	memset(digest, 0, ctx->digestsize);	/* In case it's sensitive */
	free(digest);
    }
    (void)rpmioFreePoolItem((rpmioItem)ctx, __FUNCTION__, __FILE__, __LINE__);
    return 0;
}

int
rpmHmacInit(DIGEST_CTX ctx, const void * key, size_t keylen)
{
    int rc = 0;

    if (ctx == NULL)
	return -1;
    if (key != NULL) {
	byte * salt = (byte *) DRD_xcalloc(1, ctx->blocksize);
	unsigned i;
	if (keylen == 0) keylen = strlen((char *)key);
	ctx->salt = salt;
DPRINTF((stderr, "==> ctx %p ==== HMAC(%s,%p[%u])\n", ctx, ctx->name, key, (unsigned)keylen));
	if (keylen > ctx->blocksize) {
	    /* If key is larger than digestlen, then hash the material. */
	    DIGEST_CTX kctx = rpmDigestInit(ctx->hashalgo, RPMDIGEST_NONE);
	    byte * kdigest = NULL;
	    size_t kdigestlen = 0;
	    rpmDigestUpdate(kctx, key, keylen);
	    (void) rpmDigestFinal(kctx, &kdigest, &kdigestlen, 0);
	    memcpy(ctx->salt, kdigest, kdigestlen);
	    kdigest = _free(kdigest);
	} else
	    memcpy(ctx->salt, key, keylen);

	salt = (byte *)ctx->salt;
	for (i = 0; i < ctx->blocksize; i++)
	    salt[i] ^= HMAC_IPAD;
	rpmDigestUpdate(ctx, ctx->salt, ctx->blocksize);
	for (i = 0; i < ctx->blocksize; i++)
	    salt[i] ^= HMAC_IPAD;
    }
    return rc;
}
