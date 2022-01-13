/** \ingroup rpmio
 * \file rpmio/cphpt.c
 */

#include "system.h"

#include "rpmio_internal.h"

#include <rpmbc.h>

#include "debug.h"

#define	_CIPHER_DEBUG	-1
/*@unchecked@*/
int _cph_debug = _CIPHER_DEBUG;

#ifdef  _CIPHER_DEBUG
#define DPRINTF(_a)     if (_cph_debug < 0) fprintf _a
#else
#define DPRINTF(_a)
#endif

/*@access CIPHER_CTX@*/

/**
 * Cipher private data.
 */
struct CIPHER_CTX_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@observer@*/
    const char * name;		/*!< Cipher name. */
    size_t paramsize;		/*!< No. bytes of digest parameters. */
    size_t blocksize;		/*!< No. bytes in block of plaintext data. */
    size_t keybitsmin;		/*!< Minimum no. of key bits. */
    size_t keybitsmax;		/*!< Maximum no. of key bits. */
    size_t keybitsinc;		/*!< Allowed increment in key bits. */
    int (*Setup) (void *, const byte *, size_t, int);
    int (*SetIV) (void *, const byte *);
    int (*SetCTR) (void *, const byte *, size_t);
    uint32_t * (*Feedback) (void *);
    int (*RAW) (void *, uint32_t *, const uint32_t *);
    int (*ECB) (void *, uint32_t *, const uint32_t *, unsigned int);
    int (*CBC) (void *, uint32_t *, const uint32_t *, unsigned int);
    int (*CTR) (void *, uint32_t *, const uint32_t *, unsigned int);

    pgpSymkeyAlgo symkeyalgo;	/*!< RFC 2440/4880 symkey algorithm id. */
    rpmCipherFlags flags;	/*!< Bit(s) to control cipher operation. */
    void * param;               /*!< Cipher parameters. */
};

/* XXX stub in a getter */
void * _cphParam(/*@null@*/ CIPHER_CTX cph)
{
    return (cph ? cph->param : NULL);
}

static void cphFini(void * _cph)
	/*@modifies _cph @*/
{
    CIPHER_CTX cph = (CIPHER_CTX) _cph;
    if (cph->param != NULL && cph->paramsize > 0)
	memset(cph->param, 0, cph->paramsize);	/* In case it's sensitive */
    cph->param = _free(cph->param);
    cph->name = NULL;
    cph->paramsize = 0;
    cph->blocksize = 0;
    cph->keybitsmin = 0;
    cph->keybitsmax = 0;
    cph->keybitsinc = 0;
    cph->Setup = NULL;
    cph->SetIV = NULL;
    cph->SetCTR = NULL;
    cph->Feedback = NULL;
    cph->RAW = NULL;
    cph->ECB = NULL;
    cph->CBC = NULL;
    cph->CTR = NULL;

    cph->symkeyalgo = (pgpSymkeyAlgo)0;
    cph->flags = (rpmCipherFlags)0;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _cphPool;

static CIPHER_CTX cphGetPool(rpmioPool pool)
{
    CIPHER_CTX cph;

    if (_cphPool == NULL) {
ANNOTATE_BENIGN_RACE(&_cphPool, "");
	_cphPool = rpmioNewPool("cph", sizeof(*cph), -1, _cph_debug,
			NULL, NULL, cphFini);
	pool = _cphPool;
    }
    cph = (CIPHER_CTX) rpmioGetPool(pool, sizeof(*cph));
    memset(((char *)cph)+sizeof(cph->_item), 0, sizeof(*cph)-sizeof(cph->_item));
    return cph;
}

#ifdef	REFERENCE
base64             Base 64
 bf-cbc             Blowfish in CBC mode
 bf                 Alias for bf-cbc
 bf-cfb             Blowfish in CFB mode
 bf-ecb             Blowfish in ECB mode
 bf-ofb             Blowfish in OFB mode
 cast-cbc           CAST in CBC mode
 cast               Alias for cast-cbc
 cast5-cbc          CAST5 in CBC mode
 cast5-cfb          CAST5 in CFB mode
 cast5-ecb          CAST5 in ECB mode
 cast5-ofb          CAST5 in OFB mode
 des-cbc            DES in CBC mode
 des                Alias for des-cbc
 des-cfb            DES in CBC mode
 des-ofb            DES in OFB mode
 des-ecb            DES in ECB mode
 des-ede-cbc        Two key triple DES EDE in CBC mode
 des-ede            Two key triple DES EDE in ECB mode
 des-ede-cfb        Two key triple DES EDE in CFB mode
 des-ede-ofb        Two key triple DES EDE in OFB mode
 des-ede3-cbc       Three key triple DES EDE in CBC mode
 des-ede3           Three key triple DES EDE in ECB mode
 des3               Alias for des-ede3-cbc
 des-ede3-cfb       Three key triple DES EDE CFB mode
 des-ede3-ofb       Three key triple DES EDE in OFB mode
 desx               DESX algorithm.
 gost89             GOST 28147-89 in CFB mode (provided by ccgost engine)
 gost89-cnt        `GOST 28147-89 in CNT mode (provided by ccgost engine)
 idea-cbc           IDEA algorithm in CBC mode
 idea               same as idea-cbc
 idea-cfb           IDEA in CFB mode
 idea-ecb           IDEA in ECB mode
 idea-ofb           IDEA in OFB mode
 rc2-cbc            128 bit RC2 in CBC mode
 rc2                Alias for rc2-cbc
 rc2-cfb            128 bit RC2 in CFB mode
 rc2-ecb            128 bit RC2 in ECB mode
 rc2-ofb            128 bit RC2 in OFB mode
 rc2-64-cbc         64 bit RC2 in CBC mode
 rc2-40-cbc         40 bit RC2 in CBC mode
 rc4                128 bit RC4
 rc4-64             64 bit RC4
 rc4-40             40 bit RC4
 rc5-cbc            RC5 cipher in CBC mode
 rc5                Alias for rc5-cbc
 rc5-cfb            RC5 cipher in CFB mode
 rc5-ecb            RC5 cipher in ECB mode
 rc5-ofb            RC5 cipher in OFB mode
 aes-[128|192|256]-cbc  128/192/256 bit AES in CBC mode
 aes-[128|192|256]      Alias for aes-[128|192|256]-cbc
 aes-[128|192|256]-cfb  128/192/256 bit AES in 128 bit CFB mode
 aes-[128|192|256]-cfb1 128/192/256 bit AES in 1 bit CFB mode
 aes-[128|192|256]-cfb8 128/192/256 bit AES in 8 bit CFB mode
 aes-[128|192|256]-ecb  128/192/256 bit AES in ECB mode
 aes-[128|192|256]-ofb  128/192/256 bit AES in OFB mode
#endif

CIPHER_CTX
rpmCipherInit(pgpSymkeyAlgo symkeyalgo, rpmCipherFlags flags)
{
    CIPHER_CTX cph = cphGetPool(_cphPool);

    cph->symkeyalgo = symkeyalgo;
    cph->flags = flags;

    switch (symkeyalgo) {
    case PGPSYMKEYALGO_DES:
	cph->name = "des";
	cph->paramsize = 64/8;
	cph->param = DRD_xcalloc(1, cph->paramsize);
	cph->blocksize = 0;
	cph->keybitsmin = 0;
	cph->keybitsmax = 0;
	cph->keybitsinc = 0;
	cph->Setup = NULL;
	cph->SetIV = NULL;
	cph->SetCTR = NULL;
	cph->Feedback = NULL;
	break;
    case PGPSYMKEYALGO_PLAINTEXT:
    case PGPSYMKEYALGO_IDEA:
    case PGPSYMKEYALGO_TRIPLE_DES:
    case PGPSYMKEYALGO_CAST5:
    case PGPSYMKEYALGO_BLOWFISH:
    case PGPSYMKEYALGO_SAFER:
    case PGPSYMKEYALGO_DES_SK:
    case PGPSYMKEYALGO_AES_128:
    case PGPSYMKEYALGO_AES_192:
    case PGPSYMKEYALGO_AES_256:
    case PGPSYMKEYALGO_TWOFISH:
    case PGPSYMKEYALGO_CAMELLIA_128:
    case PGPSYMKEYALGO_CAMELLIA_192:
    case PGPSYMKEYALGO_CAMELLIA_256:
    case PGPSYMKEYALGO_NOENCRYPT:
    default:
	break;
    }

DPRINTF((stderr, "==> cph %p ==== Init(%s, %x) param %p\n", cph, cph->name, flags, cph->param));

    return (CIPHER_CTX)rpmioLinkPoolItem((rpmioItem)cph, __FUNCTION__, __FILE__, __LINE__);
}

int rpmCipherNext(/*@null@*/ CIPHER_CTX cph, void * data, size_t len)
{
    if (cph == NULL)
        return -1;

DPRINTF((stderr, "==> cph %p ==== Next(%s,%p[%u]) param %p\n", cph, cph->name, data, (unsigned)len, cph->param));

    return 0;
}

int rpmCipherFinal(/*@only@*/ /*@null@*/ CIPHER_CTX cph)
{
DPRINTF((stderr, "==> cph %p ==== Final(%s) param %p\n", cph, cph->name, cph->param));
    (void)rpmioFreePoolItem((rpmioItem)cph, __FUNCTION__, __FILE__, __LINE__);
    return 0;
}
