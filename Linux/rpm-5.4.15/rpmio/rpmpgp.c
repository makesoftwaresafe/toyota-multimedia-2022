/** \ingroup rpmio signature
 * \file rpmio/rpmpgp.c
 * Routines to handle RFC-2440 detached signatures.
 */

#include "system.h"

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>

#include <rpmio.h>
#include <rpmmacro.h>

#define	_RPMPGP_INTERNAL
#include <rpmbc.h>	/* XXX still needs base64 goop */
#if defined(WITH_GCRYPT)
#include <rpmgc.h>
#endif
#if defined(WITH_NSS)
#include <rpmnss.h>
#endif
#if defined(WITH_OPENSSL)
#include <rpmssl.h>
#endif
#if defined(WITH_CDSA)
#include <rpmcdsa.h>
#endif
#if defined(WITH_TOMCRYPT)
#include <rpmltc.h>
#endif
#include "debug.h"

/*@access pgpDig @*/
/*@access pgpDigParams @*/
/*@access pgpPkt @*/
/*@access rpmiob @*/

#ifdef __cplusplus
GENpair(rpmuint8_t **)
#endif	/* __cplusplus */
/*@unchecked@*/
int _pgp_debug;

/*@unchecked@*/
int _pgp_print;

/*@unchecked@*/
int _pgp_error_count;

/*@unchecked@*/
pgpImplVecs_t * pgpImplVecs =
    /* explicit selection (order DOES NOT matter here) */
#if defined(USE_CRYPTO_BEECRYPT) && defined(WITH_BEECRYPT)
	&rpmbcImplVecs;
#elif defined(USE_CRYPTO_GCRYPT) && defined(WITH_GCRYPT)
	&rpmgcImplVecs;
#elif defined(USE_CRYPTO_NSS) && defined(WITH_NSS)
	&rpmnssImplVecs;
#elif defined(USE_CRYPTO_OPENSSL) && defined(WITH_SSL)
	&rpmsslImplVecs;
#elif defined(USE_CRYPTO_TOMCRYPT) && defined(WITH_TOMCRYPT)
	&rpmltcImplVecs;
#elif defined(USE_CRYPTO_CDSA) && defined(WITH_CDSA)
	&rpmcdsaImplVecs;
    /* implict selection (order DOES matter) */
#elif defined(WITH_BEECRYPT)
	&rpmbcImplVecs;
#elif defined(WITH_GCRYPT)
	&rpmgcImplVecs;
#elif defined(WITH_NSS)
	&rpmnssImplVecs;
#elif defined(WITH_SSL)
	&rpmsslImplVecs;
#elif defined(WITH_TOMCRYPT)
	&rpmltcImplVecs;
#elif defined(WITH_CDSA)
	&rpmcdsaImplVecs;
#else
#error INTERNAL ERROR: no suitable Cryptography library available
#endif

/*@unchecked@*/ /*@refcounted@*/ /*@relnull@*/
static pgpDig _dig = NULL;

/*@unchecked@*/ /*@null@*/
static pgpDigParams _digp = NULL;

#ifdef	DYING
struct pgpPkt_s {
    pgpTag tag;
    unsigned int pktlen;
    const rpmuint8_t * h;
    unsigned int hlen;
};
#endif

struct pgpValTbl_s pgpSigTypeTbl[] = {
    { PGPSIGTYPE_BINARY,	"BINARY" },
    { PGPSIGTYPE_TEXT,		"TEXT" },
    { PGPSIGTYPE_STANDALONE,	"STANDALONE" },
    { PGPSIGTYPE_GENERIC_CERT,	"GENERIC" },
    { PGPSIGTYPE_PERSONA_CERT,	"PERSONA" },
    { PGPSIGTYPE_CASUAL_CERT,	"CASUAL" },
    { PGPSIGTYPE_POSITIVE_CERT,	"POSITIVE" },
    { PGPSIGTYPE_SUBKEY_BINDING,"SUBKEY_BIND" },
    { PGPSIGTYPE_KEY_BINDING,	"KEY_BIND" },
    { PGPSIGTYPE_SIGNED_KEY,	"KEY" },
    { PGPSIGTYPE_KEY_REVOKE,	"KEY_REVOKE" },
    { PGPSIGTYPE_SUBKEY_REVOKE,	"SUBKEY_REVOKE" },
    { PGPSIGTYPE_CERT_REVOKE,	"CERT_REVOKE" },
    { PGPSIGTYPE_TIMESTAMP,	"TIMESTAMP" },
    { PGPSIGTYPE_CONFIRM,	"CONFIRM" },
    { -1,			"SIG_UNKNOWN" },
};

struct pgpValTbl_s pgpPubkeyTbl[] = {
    { PGPPUBKEYALGO_RSA,	"RSA" },
    { PGPPUBKEYALGO_RSA_E,	"RSA_E" },
    { PGPPUBKEYALGO_RSA_S,	"RSA_S" },
    { PGPPUBKEYALGO_ELGAMAL_E,	"ELG_E" },
    { PGPPUBKEYALGO_DSA,	"DSA" },
    { PGPPUBKEYALGO_ECDH,	"ECDH" },
    { PGPPUBKEYALGO_ECDSA,	"ECDSA" },
    { PGPPUBKEYALGO_ELGAMAL,	"ELG" },
    { PGPPUBKEYALGO_EDDSA,	"EDDSA" },
    { -1,			"KEY_UNKNOWN" },
};

struct pgpValTbl_s pgpSymkeyTbl[] = {
    { PGPSYMKEYALGO_PLAINTEXT,	"Plaintext" },
    { PGPSYMKEYALGO_IDEA,	"IDEA" },
    { PGPSYMKEYALGO_TRIPLE_DES,	"3DES" },
    { PGPSYMKEYALGO_CAST5,	"CAST5" },
    { PGPSYMKEYALGO_BLOWFISH,	"BLOWFISH" },
    { PGPSYMKEYALGO_SAFER,	"SAFER" },
    { PGPSYMKEYALGO_DES_SK,	"DES/SK" },
    { PGPSYMKEYALGO_AES_128,	"AES(128-bit key)" },
    { PGPSYMKEYALGO_AES_192,	"AES(192-bit key)" },
    { PGPSYMKEYALGO_AES_256,	"AES(256-bit key)" },
    { PGPSYMKEYALGO_TWOFISH,	"TWOFISH(256-bit key)" },
    { PGPSYMKEYALGO_CAMELLIA_128, "CAMELLIA(128-bit key)" },
    { PGPSYMKEYALGO_CAMELLIA_192, "CAMELLIA(192-bit key)" },
    { PGPSYMKEYALGO_CAMELLIA_256, "CAMELLIA(256-bit key)" },
    { PGPSYMKEYALGO_NOENCRYPT,	"no encryption" },
    { -1,			"SYM_UNKNOWN" },
};

struct pgpValTbl_s pgpCompressionTbl[] = {
    { PGPCOMPRESSALGO_NONE,	"Uncompressed" },
    { PGPCOMPRESSALGO_ZIP,	"ZIP" },
    { PGPCOMPRESSALGO_ZLIB, 	"ZLIB" },
    { PGPCOMPRESSALGO_BZIP2, 	"BZIP2" },
    { -1,			"Unknown compression algorithm" },
};

struct pgpValTbl_s pgpHashTbl[] = {
    { PGPHASHALGO_MD5,		"MD5" },
    { PGPHASHALGO_SHA1,		"SHA1" },
    { PGPHASHALGO_RIPEMD160,	"RIPEMD160" },
    { PGPHASHALGO_MD2,		"MD2" },
    { PGPHASHALGO_TIGER192,	"TIGER192" },
    { PGPHASHALGO_HAVAL_5_160,	"HAVAL-5-160" },
    { PGPHASHALGO_SHA224,	"SHA224" },
    { PGPHASHALGO_SHA256,	"SHA256" },
    { PGPHASHALGO_SHA384,	"SHA384" },
    { PGPHASHALGO_SHA512,	"SHA512" },
    { -1,			"MD_UNKNOWN" },
};

/*@-exportlocal -exportheadervar@*/
/*@observer@*/ /*@unchecked@*/
struct pgpValTbl_s pgpKeyServerPrefsTbl[] = {
    { 0x80,			"No-modify" },
    { -1,			"Unknown key server preference" },
};
/*@=exportlocal =exportheadervar@*/

struct pgpValTbl_s pgpSubTypeTbl[] = {
    { PGPSUBTYPE_SIG_CREATE_TIME,"created" },
    { PGPSUBTYPE_SIG_EXPIRE_TIME,"expires" },
    { PGPSUBTYPE_EXPORTABLE_CERT,"exportable" },
    { PGPSUBTYPE_TRUST_SIG,	"trust signature" },
    { PGPSUBTYPE_REGEX,		"regular expression" },
    { PGPSUBTYPE_REVOCABLE,	"revocable" },
    { PGPSUBTYPE_KEY_EXPIRE_TIME,"key expiration time" },
    { PGPSUBTYPE_ARR,		"additional recipient request" },
    { PGPSUBTYPE_PREFER_SYMKEY,	"preferred symmetric algorithms" },
    { PGPSUBTYPE_REVOKE_KEY,	"revocation key" },
    { PGPSUBTYPE_ISSUER_KEYID,	"issuer" },
    { PGPSUBTYPE_NOTATION,	"notation data" },
    { PGPSUBTYPE_PREFER_HASH,	"preferred hash algorithms" },
    { PGPSUBTYPE_PREFER_COMPRESS,"preferred compression algorithms" },
    { PGPSUBTYPE_KEYSERVER_PREFERS,"key server preferences" },
    { PGPSUBTYPE_PREFER_KEYSERVER,"preferred key server" },
    { PGPSUBTYPE_PRIMARY_USERID,"primary user id" },
    { PGPSUBTYPE_POLICY_URL,	"policy URL" },
    { PGPSUBTYPE_KEY_FLAGS,	"key flags" },
    { PGPSUBTYPE_SIGNER_USERID,	"signer's user id" },
    { PGPSUBTYPE_REVOKE_REASON,	"reason for revocation" },
    { PGPSUBTYPE_FEATURES,	"features" },
    { PGPSUBTYPE_SIG_TARGET,	"signature target" },
    { PGPSUBTYPE_EMBEDDED_SIG,	"embedded signature" },

    { PGPSUBTYPE_INTERNAL_100,	"internal subpkt type 100" },
    { PGPSUBTYPE_INTERNAL_101,	"internal subpkt type 101" },
    { PGPSUBTYPE_INTERNAL_102,	"internal subpkt type 102" },
    { PGPSUBTYPE_INTERNAL_103,	"internal subpkt type 103" },
    { PGPSUBTYPE_INTERNAL_104,	"internal subpkt type 104" },
    { PGPSUBTYPE_INTERNAL_105,	"internal subpkt type 105" },
    { PGPSUBTYPE_INTERNAL_106,	"internal subpkt type 106" },
    { PGPSUBTYPE_INTERNAL_107,	"internal subpkt type 107" },
    { PGPSUBTYPE_INTERNAL_108,	"internal subpkt type 108" },
    { PGPSUBTYPE_INTERNAL_109,	"internal subpkt type 109" },
    { PGPSUBTYPE_INTERNAL_110,	"internal subpkt type 110" },
    { -1,			"Unknown signature subkey type" },
};

struct pgpValTbl_s pgpTagTbl[] = {
    { PGPTAG_PUBLIC_SESSION_KEY,"Public-Key Encrypted Session Key" },
    { PGPTAG_SIGNATURE,		"Signature" },
    { PGPTAG_SYMMETRIC_SESSION_KEY,"Symmetric-Key Encrypted Session Key" },
    { PGPTAG_ONEPASS_SIGNATURE,	"One-Pass Signature" },
    { PGPTAG_SECRET_KEY,	"Secret Key" },
    { PGPTAG_PUBLIC_KEY,	"Public Key" },
    { PGPTAG_SECRET_SUBKEY,	"Secret Subkey" },
    { PGPTAG_COMPRESSED_DATA,	"Compressed Data" },
    { PGPTAG_SYMMETRIC_DATA,	"Symmetrically Encrypted Data" },
    { PGPTAG_MARKER,		"Marker" },
    { PGPTAG_LITERAL_DATA,	"Literal Data" },
    { PGPTAG_TRUST,		"Trust" },
    { PGPTAG_USER_ID,		"User ID" },
    { PGPTAG_PUBLIC_SUBKEY,	"Public Subkey" },
    { PGPTAG_COMMENT_OLD,	"Comment (from OpenPGP draft)" },
    { PGPTAG_PHOTOID,		"PGP's photo ID" },
    { PGPTAG_ENCRYPTED_MDC,	"Integrity protected encrypted data" },
    { PGPTAG_MDC,		"Manipulaion detection code packet" },
    { PGPTAG_PRIVATE_60,	"Private #60" },
    { PGPTAG_COMMENT,		"Comment" },
    { PGPTAG_PRIVATE_62,	"Private #62" },
    { PGPTAG_CONTROL,		"Control (GPG)" },
    { -1,			"TAG_UNKNOWN" },
};

struct pgpValTbl_s pgpArmorTbl[] = {
    { PGPARMOR_MESSAGE,		"MESSAGE" },
    { PGPARMOR_PUBKEY,		"PUBLIC KEY BLOCK" },
    { PGPARMOR_SIGNATURE,	"SIGNATURE" },
    { PGPARMOR_SIGNED_MESSAGE,	"SIGNED MESSAGE" },
    { PGPARMOR_FILE,		"ARMORED FILE" },
    { PGPARMOR_PRIVKEY,		"PRIVATE KEY BLOCK" },
    { PGPARMOR_SECKEY,		"SECRET KEY BLOCK" },
    { -1,			"Unknown armor block" }
};

struct pgpValTbl_s pgpArmorKeyTbl[] = {
    { PGPARMORKEY_VERSION,	"Version: " },
    { PGPARMORKEY_COMMENT,	"Comment: " },
    { PGPARMORKEY_MESSAGEID,	"MessageID: " },
    { PGPARMORKEY_HASH,		"Hash: " },
    { PGPARMORKEY_CHARSET,	"Charset: " },
    { -1,			"Unknown armor key" }
};

static void pgpPrtNL(void)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (!_pgp_print) return;
    fprintf(stderr, "\n");
}

static void pgpPrtInt(const char *pre, int i)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (!_pgp_print) return;
    if (pre && *pre)
	fprintf(stderr, "%s", pre);
    fprintf(stderr, " %d", i);
}

static void pgpPrtStr(const char *pre, const char *s)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (!_pgp_print) return;
    if (pre && *pre)
	fprintf(stderr, "%s", pre);
    fprintf(stderr, " %s", s);
}

static void pgpPrtHex(const char *pre, const rpmuint8_t * p, size_t plen)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (!_pgp_print) return;
    if (pre && *pre)
	fprintf(stderr, "%s", pre);
    fprintf(stderr, " %s", pgpHexStr(p, plen));
}

void pgpPrtVal(const char * pre, pgpValTbl vs, rpmuint8_t val)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (!_pgp_print) return;
    if (pre && *pre)
	fprintf(stderr, "%s", pre);
    fprintf(stderr, "%s(%u)", pgpValStr(vs, val), (unsigned)val);
}

int pgpPrtSubType(const rpmuint8_t * h, size_t hlen, pgpSigType sigtype)
{
    const rpmuint8_t * p = h;
    unsigned plen;
    unsigned i;

    while (hlen > 0) {
	i = pgpLen(p, &plen);
	p += i;
	hlen -= i;

	pgpPrtVal("    ", pgpSubTypeTbl, (p[0]&(~PGPSUBTYPE_CRITICAL)));
	if ((p[0] & PGPSUBTYPE_CRITICAL) != (rpmuint8_t)0)
	    if (_pgp_print)
		fprintf(stderr, " *CRITICAL*");
	switch (*p) {
	case PGPSUBTYPE_PREFER_SYMKEY:	/* preferred symmetric algorithms */
	    for (i = 1; i < plen; i++)
		pgpPrtVal(" ", pgpSymkeyTbl, p[i]);
	    /*@switchbreak@*/ break;
	case PGPSUBTYPE_PREFER_HASH:	/* preferred hash algorithms */
	    for (i = 1; i < plen; i++)
		pgpPrtVal(" ", pgpHashTbl, p[i]);
	    /*@switchbreak@*/ break;
	case PGPSUBTYPE_PREFER_COMPRESS:/* preferred compression algorithms */
	    for (i = 1; i < plen; i++)
		pgpPrtVal(" ", pgpCompressionTbl, p[i]);
	    /*@switchbreak@*/ break;
	case PGPSUBTYPE_KEYSERVER_PREFERS:/* key server preferences */
	    for (i = 1; i < plen; i++)
		pgpPrtVal(" ", pgpKeyServerPrefsTbl, p[i]);
	    /*@switchbreak@*/ break;
	case PGPSUBTYPE_SIG_CREATE_TIME:
/*@-mods -mayaliasunique @*/
	    if (_digp && !(_digp->saved & PGPDIG_SAVED_TIME) &&
		(sigtype == PGPSIGTYPE_POSITIVE_CERT || sigtype == PGPSIGTYPE_BINARY || sigtype == PGPSIGTYPE_TEXT || sigtype == PGPSIGTYPE_STANDALONE))
	    {
		_digp->saved |= PGPDIG_SAVED_TIME;
		memcpy(_digp->time, p+1, sizeof(_digp->time));
	    }
/*@=mods =mayaliasunique @*/
	    if ((plen - 1) == 4) {
		time_t t = pgpGrab(p+1, plen-1);
		if (_pgp_print)
		   fprintf(stderr, " %-24.24s(0x%08x)", ctime(&t), (unsigned)t);
	    } else
		pgpPrtHex("", p+1, plen-1);
	    /*@switchbreak@*/ break;
	case PGPSUBTYPE_SIG_EXPIRE_TIME:
	case PGPSUBTYPE_KEY_EXPIRE_TIME:	/* XXX only on self-signature */
	    if ((plen - 1) == 4) {
		time_t t = pgpGrab(p+1, plen-1);
		if (t == 0) {
		    if (_pgp_print)
			fprintf(stderr, " never(0x%08x)", (unsigned)t);
		} else if (_digp && _digp->saved & PGPDIG_SAVED_TIME) {
		    t += pgpGrab(_digp->time, sizeof(_digp->time));
		    if (_pgp_print)
			fprintf(stderr, " %-24.24s(0x%08x)", ctime(&t), (unsigned)t);
		} else {
		    if (_pgp_print)
			fprintf(stderr, " creation+secs(0x%08x)", (unsigned)t);
		}
	    } else
		pgpPrtHex("", p+1, plen-1);
	    /*@switchbreak@*/ break;

	case PGPSUBTYPE_ISSUER_KEYID:	/* issuer key ID */
/*@-mods -mayaliasunique @*/
	    if (_digp && !(_digp->saved & PGPDIG_SAVED_ID) &&
		(sigtype == PGPSIGTYPE_POSITIVE_CERT || sigtype == PGPSIGTYPE_BINARY || sigtype == PGPSIGTYPE_TEXT || sigtype == PGPSIGTYPE_STANDALONE))
	    {
		_digp->saved |= PGPDIG_SAVED_ID;
		memcpy(_digp->signid, p+1, sizeof(_digp->signid));
	    }
/*@=mods =mayaliasunique @*/
	    /*@fallthrough@*/
	case PGPSUBTYPE_EXPORTABLE_CERT:
	case PGPSUBTYPE_TRUST_SIG:
	case PGPSUBTYPE_REGEX:
	case PGPSUBTYPE_REVOCABLE:
	case PGPSUBTYPE_ARR:
	case PGPSUBTYPE_REVOKE_KEY:
	case PGPSUBTYPE_NOTATION:
	case PGPSUBTYPE_PREFER_KEYSERVER:
	case PGPSUBTYPE_PRIMARY_USERID:
	case PGPSUBTYPE_POLICY_URL:
	case PGPSUBTYPE_KEY_FLAGS:
	case PGPSUBTYPE_SIGNER_USERID:
	case PGPSUBTYPE_REVOKE_REASON:
	case PGPSUBTYPE_FEATURES:
	case PGPSUBTYPE_SIG_TARGET:
	case PGPSUBTYPE_EMBEDDED_SIG:
	case PGPSUBTYPE_INTERNAL_100:
	case PGPSUBTYPE_INTERNAL_101:
	case PGPSUBTYPE_INTERNAL_102:
	case PGPSUBTYPE_INTERNAL_103:
	case PGPSUBTYPE_INTERNAL_104:
	case PGPSUBTYPE_INTERNAL_105:
	case PGPSUBTYPE_INTERNAL_106:
	case PGPSUBTYPE_INTERNAL_107:
	case PGPSUBTYPE_INTERNAL_108:
	case PGPSUBTYPE_INTERNAL_109:
	case PGPSUBTYPE_INTERNAL_110:
	default:
	    pgpPrtHex("", p+1, plen-1);
	    /*@switchbreak@*/ break;
	}
	pgpPrtNL();
	p += plen;
	hlen -= plen;
    }
    return 0;
}

/*@-varuse =readonlytrans -nullassign @*/
/*@observer@*/ /*@unchecked@*/
static const char * pgpSigRSA[] = {
    " m**d =",
    NULL,
};

/*@observer@*/ /*@unchecked@*/
static const char * pgpSigDSA[] = {
    "    r =",
    "    s =",
    NULL,
};

/*@observer@*/ /*@unchecked@*/
static const char * pgpSigECDSA[] = {
    "    r =",
    "    s =",
    NULL,
};
/*@=varuse =readonlytrans =nullassign @*/

int pgpPrtSigParams(pgpDig dig, const pgpPkt pp, pgpPubkeyAlgo pubkey_algo,
		pgpSigType sigtype, const rpmuint8_t * p)
{
    const rpmuint8_t * pend = pp->u.h + pp->hlen;
    int xx;
    int i;

    for (i = 0; p < pend; i++, p += pgpMpiLen(p)) {
	if (pubkey_algo == PGPPUBKEYALGO_RSA) {
	    if (i >= 1) break;
	    if (dig &&
	(dig != _dig || sigtype == PGPSIGTYPE_BINARY || sigtype == PGPSIGTYPE_TEXT))
	    {
		xx = 0;
		switch (i) {
		case 0:		/* m**d */
		    xx = pgpImplMpiItem(pgpSigRSA[i], dig, 10+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    xx = 1;
		    /*@switchbreak@*/ break;
		}
		if (xx) return xx;
	    }
	    pgpPrtStr("", pgpSigRSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_DSA) {
	    if (i >= 2) break;
	    if (dig &&
	(dig != _dig || sigtype == PGPSIGTYPE_BINARY || sigtype == PGPSIGTYPE_TEXT))
	    {
		xx = 0;
		switch (i) {
		case 0:		/* r */
		    xx = pgpImplMpiItem(pgpSigDSA[i], dig, 20+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 1:		/* s */
		    xx = pgpImplMpiItem(pgpSigDSA[i], dig, 20+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    xx = 1;
		    /*@switchbreak@*/ break;
		}
		if (xx) return xx;
	    }
	    pgpPrtStr("", pgpSigDSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_ECDSA) {
	    if (i >= 2) break;
	    if (dig &&
	(dig != _dig || sigtype == PGPSIGTYPE_BINARY || sigtype == PGPSIGTYPE_TEXT))
	    {
		xx = 0;
		switch (i) {
		case 0:		/* r */
		    xx = pgpImplMpiItem(pgpSigECDSA[i], dig, 50+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 1:		/* s */
		    xx = pgpImplMpiItem(pgpSigECDSA[i], dig, 50+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    xx = 1;
		    /*@switchbreak@*/ break;
		}
		if (xx) return xx;
	    }
	    pgpPrtStr("", pgpSigECDSA[i]);
	} else {
	    if (_pgp_print)
		fprintf(stderr, "%7d", i);
	}
	pgpPrtStr("", pgpMpiStr(p));
	pgpPrtNL();
    }

    return 0;
}

int pgpPrtSig(const pgpPkt pp)
	/*@globals _digp @*/
	/*@modifies *_digp @*/
{
    rpmuint8_t version = pp->u.h[0];
    rpmuint8_t * p;
    unsigned plen;
    int rc;

    switch (version) {
    case 3:
    {   pgpPktSigV3 v = (pgpPktSigV3)pp->u.h;
	time_t t;

	if (v->hashlen != (rpmuint8_t)5)
	    return 1;

	pgpPrtVal("V3 ", pgpTagTbl, (rpmuint8_t)pp->tag);
	pgpPrtVal(" ", pgpPubkeyTbl, v->pubkey_algo);
	pgpPrtVal(" ", pgpHashTbl, v->hash_algo);
	pgpPrtVal(" ", pgpSigTypeTbl, v->sigtype);
	pgpPrtNL();
	t = pgpGrab(v->time, sizeof(v->time));
	if (_pgp_print)
	    fprintf(stderr, " %-24.24s(0x%08x)", ctime(&t), (unsigned)t);
	pgpPrtNL();
	pgpPrtHex(" signer keyid", v->signid, sizeof(v->signid));
	plen = pgpGrab(v->signhash16, sizeof(v->signhash16));
	pgpPrtHex(" signhash16", v->signhash16, sizeof(v->signhash16));
	pgpPrtNL();

	if (_digp && _digp->pubkey_algo == (rpmuint8_t)0) {
	    _digp->version = v->version;
	    _digp->hashlen = (size_t) v->hashlen;
	    _digp->sigtype = v->sigtype;
	    _digp->hash = &v->sigtype;
	    memcpy(_digp->time, v->time, sizeof(_digp->time));
	    memcpy(_digp->signid, v->signid, sizeof(_digp->signid));
	    _digp->pubkey_algo = v->pubkey_algo;
	    _digp->hash_algo = v->hash_algo;
	    memcpy(_digp->signhash16, v->signhash16, sizeof(_digp->signhash16));
	}

	p = ((rpmuint8_t *)v) + sizeof(*v);
	rc = pgpPrtSigParams(_dig, pp, (pgpPubkeyAlgo)v->pubkey_algo,
			(pgpSigType)v->sigtype, p);
    }	break;
    case 4:
    {   pgpPktSigV4 v = (pgpPktSigV4)pp->u.h;

	pgpPrtVal("V4 ", pgpTagTbl, (rpmuint8_t)pp->tag);
	pgpPrtVal(" ", pgpPubkeyTbl, v->pubkey_algo);
	pgpPrtVal(" ", pgpHashTbl, v->hash_algo);
	pgpPrtVal(" ", pgpSigTypeTbl, v->sigtype);
	pgpPrtNL();

	p = &v->hashlen[0];
	plen = pgpGrab(v->hashlen, sizeof(v->hashlen));
	p += sizeof(v->hashlen);

	if ((p + plen) > (pp->u.h + pp->hlen))
	    return 1;

if (_pgp_debug && _pgp_print)
fprintf(stderr, "   hash[%u] -- %s\n", plen, pgpHexStr(p, plen));
	if (_digp && _digp->pubkey_algo == (rpmuint8_t)0) {
	    _digp->hashlen = sizeof(*v) + plen;
	    _digp->hash = pp->u.h;
	}
	(void) pgpPrtSubType(p, plen, (pgpSigType)v->sigtype);
	p += plen;

	plen = pgpGrab(p, 2);
	p += 2;

	if ((p + plen) > (pp->u.h + pp->hlen))
	    return 1;

if (_pgp_debug && _pgp_print)
fprintf(stderr, " unhash[%u] -- %s\n", plen, pgpHexStr(p, plen));
	(void) pgpPrtSubType(p, plen, (pgpSigType)v->sigtype);
	p += plen;

	plen = pgpGrab(p,2);
	pgpPrtHex(" signhash16", p, 2);
	pgpPrtNL();

	if (_digp && _digp->pubkey_algo == (rpmuint8_t)0) {
	    _digp->version = v->version;
	    _digp->sigtype = v->sigtype;
	    _digp->pubkey_algo = v->pubkey_algo;
	    _digp->hash_algo = v->hash_algo;
	    memcpy(_digp->signhash16, p, sizeof(_digp->signhash16));
	}

	p += 2;
	if (p > (pp->u.h + pp->hlen))
	    return 1;

	rc = pgpPrtSigParams(_dig, pp, (pgpPubkeyAlgo)v->pubkey_algo,
			(pgpSigType)v->sigtype, p);
    }	break;
    default:
	rc = 1;
	break;
    }
    return rc;
}

/*@-varuse =readonlytrans -nullassign @*/
/*@observer@*/ /*@unchecked@*/
static const char * pgpPublicRSA[] = {
    "    n =",
    "    e =",
    NULL,
};

#ifdef NOTYET
/*@observer@*/ /*@unchecked@*/
static const char * pgpSecretRSA[] = {
    "    d =",
    "    p =",
    "    q =",
    "    u =",
    NULL,
};
#endif

/*@observer@*/ /*@unchecked@*/
static const char * pgpPublicDSA[] = {
    "    p =",
    "    q =",
    "    g =",
    "    y =",
    NULL,
};

#ifdef	NOTYET
/*@observer@*/ /*@unchecked@*/
static const char * pgpSecretDSA[] = {
    "    x =",
    NULL,
};
#endif

/*@observer@*/ /*@unchecked@*/
static const char * pgpPublicECDSA[] = {
    "    Q =",
    NULL,
};

#ifdef	NOTYET
/*@observer@*/ /*@unchecked@*/
static const char * pgpSecretECDSA[] = {
    "    d =",
    NULL,
};
#endif

/*@observer@*/ /*@unchecked@*/
static const char * pgpPublicECDH[] = {
    "    Q =",
    NULL,
};

#ifdef	NOTYET
/*@observer@*/ /*@unchecked@*/
static const char * pgpSecretECDH[] = {
    "    d =",
    NULL,
};
#endif

/*@observer@*/ /*@unchecked@*/
static const char * pgpPublicELGAMAL[] = {
    "    p =",
    "    g =",
    "    y =",
    NULL,
};

#ifdef	NOTYET
/*@observer@*/ /*@unchecked@*/
static const char * pgpSecretELGAMAL[] = {
    "    x =",
    NULL,
};
#endif
/*@=varuse =readonlytrans =nullassign @*/

const rpmuint8_t * pgpPrtPubkeyParams(pgpDig dig, const pgpPkt pp,
		pgpPubkeyAlgo pubkey_algo, /*@returned@*/ const rpmuint8_t * p)
{
    const rpmuint8_t * pend = pp->u.h + pp->hlen;
    int i;

    for (i = 0; p < pend; i++, p += pgpMpiLen(p)) {
	if (pubkey_algo == PGPPUBKEYALGO_RSA) {
	    if (i >= 2) break;
	    if (dig) {
		switch (i) {
		case 0:		/* n */
		    (void) pgpImplMpiItem(pgpPublicRSA[i], dig, 30+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 1:		/* e */
		    (void) pgpImplMpiItem(pgpPublicRSA[i], dig, 30+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    /*@switchbreak@*/ break;
		}
	    }
	    pgpPrtStr("", pgpPublicRSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_DSA) {
	    if (i >= 4) break;
	    if (dig) {
		switch (i) {
		case 0:		/* p */
		    (void) pgpImplMpiItem(pgpPublicDSA[i], dig, 40+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 1:		/* q */
		    (void) pgpImplMpiItem(pgpPublicDSA[i], dig, 40+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 2:		/* g */
		    (void) pgpImplMpiItem(pgpPublicDSA[i], dig, 40+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		case 3:		/* y */
		    (void) pgpImplMpiItem(pgpPublicDSA[i], dig, 40+i, p, p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    /*@switchbreak@*/ break;
		}
	    }
	    pgpPrtStr("", pgpPublicDSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_ECDSA) {
	    if (i >= 1) break;
	    if (dig) {
		switch (i) {
		case 0:		/* curve_oid & Q */
		    (void) pgpImplMpiItem(pgpPublicECDSA[i], dig, 60, p+1, p+1+p[0]);
		    (void) pgpImplMpiItem(pgpPublicECDSA[i], dig, 61, p+1+p[0], p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    /*@switchbreak@*/ break;
		}
	    }
	    if (i == 0) {
		pgpPrtHex("Curve = [ OID]:", p+1, p[0]);
		pgpPrtNL();
		p += p[0] + 1;
	        pgpPrtStr(pgpPublicECDSA[i], pgpMpiStr(p));
		pgpPrtNL();
		break;	/* XXX FIXME: early exit */
	    }
	    pgpPrtStr("", pgpPublicECDSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_ECDH) {
	    if (i >= 1) break;
	    if (dig) {
		switch (i) {
		case 0:		/* curve_oid & Q */
		    (void) pgpImplMpiItem(pgpPublicECDH[i], dig, 60, p+1, p+1+p[0]);
		    (void) pgpImplMpiItem(pgpPublicECDH[i], dig, 61, p+1+p[0], p+pgpMpiLen(p));
		    /*@switchbreak@*/ break;
		default:
		    /*@switchbreak@*/ break;
		}
	    }
	    if (i == 0) {
		pgpPrtHex("Curve = [ OID]:", p+1, p[0]);
		p += p[0] + 1;
		pgpPrtNL();
	        pgpPrtStr(pgpPublicECDH[i], pgpMpiStr(p));
		p += pgpMpiLen(p);
		pgpPrtHex("    KDF params:", p+1, p[0]);
		p += p[0] + 1;
		pgpPrtNL();
		break;	/* XXX FIXME: early exit */
	    }
	} else if (pubkey_algo == PGPPUBKEYALGO_ELGAMAL_E) {
	    if (i >= 3) break;
	    pgpPrtStr("", pgpPublicELGAMAL[i]);
	} else {
	    if (_pgp_print)
		fprintf(stderr, "%7d", i);
	}
	pgpPrtStr("", pgpMpiStr(p));
	pgpPrtNL();
    }

    return p;
}

static const rpmuint8_t * pgpPrtSeckeyParams(const pgpPkt pp,
		/*@unused@*/ rpmuint8_t pubkey_algo,
		/*@returned@*/ const rpmuint8_t *p)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int i;

    switch (*p) {
    case 0:
	pgpPrtVal(" ", pgpSymkeyTbl, *p);
	break;
    case 255:
	p++;
	pgpPrtVal(" ", pgpSymkeyTbl, *p);
	switch (p[1]) {
	case 0x00:
	    pgpPrtVal(" simple ", pgpHashTbl, p[2]);
	    p += 2;
	    /*@innerbreak@*/ break;
	case 0x01:
	    pgpPrtVal(" salted ", pgpHashTbl, p[2]);
	    pgpPrtHex("", p+3, 8);
	    p += 10;
	    /*@innerbreak@*/ break;
	case 0x03:
	    pgpPrtVal(" iterated/salted ", pgpHashTbl, p[2]);
	    i = (16 + ((unsigned)p[11] & 0xf)) << (((unsigned)p[11] >> 4U) + 6);
	    pgpPrtHex("", p+3, 8);
	    pgpPrtInt(" iter", i);
	    p += 11;
	    /*@innerbreak@*/ break;
	}
	break;
    default:
	pgpPrtVal(" ", pgpSymkeyTbl, *p);
	pgpPrtHex(" IV", p+1, 8);
	p += 8;
	break;
    }
    pgpPrtNL();

    p++;

#ifdef	NOTYET	/* XXX encrypted MPI's need to be handled. */
    for (i = 0; p < &pp->u.h[pp->hlen]; i++, p += pgpMpiLen(p)) {
	if (pubkey_algo == PGPPUBKEYALGO_RSA) {
	    if (pgpSecretRSA[i] == NULL) break;
	    pgpPrtStr("", pgpSecretRSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_DSA) {
	    if (pgpSecretDSA[i] == NULL) break;
	    pgpPrtStr("", pgpSecretDSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_ECDSA) {
	    if (pgpSecretECDSA[i] == NULL) break;
	    pgpPrtStr("", pgpSecretECDSA[i]);
	} else if (pubkey_algo == PGPPUBKEYALGO_ELGAMAL_E) {
	    if (pgpSecretELGAMAL[i] == NULL) break;
	    pgpPrtStr("", pgpSecretELGAMAL[i]);
	} else {
	    if (_pgp_print)
		fprintf(stderr, "%7d", i);
	}
	pgpPrtStr("", pgpMpiStr(p));
	pgpPrtNL();
    }
#else
    pgpPrtHex(" secret", p, (pp->hlen - (p - pp->u.h) - 2));
    pgpPrtNL();
    p += (pp->hlen - (p - pp->u.h) - 2);
#endif
    pgpPrtHex(" checksum", p, 2);
    pgpPrtNL();

    return p;
}

int pgpPrtKey(const pgpPkt pp)
	/*@globals _digp @*/
	/*@modifies *_digp @*/
{
    rpmuint8_t version = pp->u.h[0];
    const rpmuint8_t * p;
    unsigned plen;
    time_t t;
    int rc;

    switch (version) {
    case 3:
    {   pgpPktKeyV3 v = (pgpPktKeyV3)pp->u.h;
	pgpPrtVal("V3 ", pgpTagTbl, (rpmuint8_t)pp->tag);
	pgpPrtVal(" ", pgpPubkeyTbl, v->pubkey_algo);
	t = pgpGrab(v->time, sizeof(v->time));
	if (_pgp_print)
	    fprintf(stderr, " %-24.24s(0x%08x)", ctime(&t), (unsigned)t);
	plen = pgpGrab(v->valid, sizeof(v->valid));
	if (plen != 0)
	    fprintf(stderr, " valid %u days", plen);
	pgpPrtNL();

	if (_digp && _digp->tag == (rpmuint8_t)pp->tag) {
	    _digp->version = v->version;
	    memcpy(_digp->time, v->time, sizeof(_digp->time));
	    _digp->pubkey_algo = v->pubkey_algo;
	}

	p = ((rpmuint8_t *)v) + sizeof(*v);
	p = pgpPrtPubkeyParams(_dig, pp, (pgpPubkeyAlgo)v->pubkey_algo, p);
	rc = 0;
    }	break;
    case 4:
    {   pgpPktKeyV4 v = (pgpPktKeyV4)pp->u.h;
	pgpPrtVal("V4 ", pgpTagTbl, (rpmuint8_t)pp->tag);
	pgpPrtVal(" ", pgpPubkeyTbl, v->pubkey_algo);
	t = pgpGrab(v->time, sizeof(v->time));
	if (_pgp_print)
	    fprintf(stderr, " %-24.24s(0x%08x)", ctime(&t), (unsigned)t);
	pgpPrtNL();

	if (_digp && _digp->tag == (rpmuint8_t)pp->tag) {
	    _digp->version = v->version;
	    memcpy(_digp->time, v->time, sizeof(_digp->time));
	    _digp->pubkey_algo = v->pubkey_algo;
	}

	p = ((rpmuint8_t *)v) + sizeof(*v);
	p = pgpPrtPubkeyParams(_dig, pp, (pgpPubkeyAlgo)v->pubkey_algo, p);
	if (!(pp->tag == PGPTAG_PUBLIC_KEY || pp->tag == PGPTAG_PUBLIC_SUBKEY))
	    p = pgpPrtSeckeyParams(pp, v->pubkey_algo, p);
	rc = 0;
    }	break;
    default:
	rc = 1;
	break;
    }
    return rc;
}

int pgpPrtUserID(const pgpPkt pp)
	/*@globals _digp @*/
	/*@modifies *_digp @*/
{
    pgpPrtVal("", pgpTagTbl, (rpmuint8_t)pp->tag);
    if (_pgp_print)
	fprintf(stderr, " \"%.*s\"", (int)pp->hlen, (const char *)pp->u.h);
    pgpPrtNL();
    if (_digp) {
	char * t = (char *) memcpy(xmalloc(pp->hlen+1), pp->u.h, pp->hlen);
	t[pp->hlen] = '\0';
	_digp->userid = _free(_digp->userid);
	_digp->userid = t;
    }
    return 0;
}

int pgpPrtComment(const pgpPkt pp)
{
    const rpmuint8_t * h = pp->u.h;
    int i = pp->hlen;

    pgpPrtVal("", pgpTagTbl, (rpmuint8_t)pp->tag);
    if (_pgp_print)
	fprintf(stderr, " ");
    while (i > 0) {
	int j;
	if (*h >= (rpmuint8_t)' ' && *h <= (rpmuint8_t)'z') {
	    j = 0;
	    while (j < i && h[j] != (rpmuint8_t)'\0')
		j++;
	    while (j < i && h[j] == (rpmuint8_t)'\0')
		j++;
	    if (_pgp_print && j)
		fprintf(stderr, "%.*s", (int)strlen((const char *)h), (const char *)h);
	} else {
	    pgpPrtHex("", h, i);
	    j = i;
	}
	i -= j;
	h += j;
    }
    pgpPrtNL();
    return 0;
}

int pgpPktLen(const rpmuint8_t *pkt, size_t pleft, pgpPkt pp)
{
    unsigned int val = (unsigned int)*pkt;
    unsigned int plen;

    memset(pp, 0, sizeof(*pp));
    /* XXX can't deal with these. */
    if (!(val & 0x80))
	return -1;

    if (val & 0x40) {
	pp->tag = (pgpTag) (val & 0x3f);
	plen = pgpLen(pkt+1, &pp->hlen);
    } else {
	pp->tag = (pgpTag) ((val >> 2) & 0xf);
	plen = (1 << (val & 0x3));
	pp->hlen = pgpGrab(pkt+1, plen);
    }

    pp->pktlen = 1 + plen + pp->hlen;
    if (pleft > 0 && pp->pktlen > (unsigned)pleft)
	return -1;

/*@-assignexpose -temptrans @*/
    pp->u.h = pkt + 1 + plen;
/*@=assignexpose =temptrans @*/

    return pp->pktlen;
}

int pgpPubkeyFingerprint(const rpmuint8_t * pkt, size_t pktlen, rpmuint8_t * keyid)
{
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    int rc = pgpPktLen(pkt, pktlen, pp);

    if (!(pp->tag == PGPTAG_PUBLIC_KEY || pp->tag == PGPTAG_PUBLIC_SUBKEY))
	return -1;

    /* Choose the correct keyid. */
    switch (pp->u.h[0]) {
    default:	return -1;
    case 3:
      {	pgpPktKeyV3 v = (pgpPktKeyV3) (pp->u.h);
	const rpmuint8_t *se = (rpmuint8_t *)(v + 1);
	switch (v->pubkey_algo) {
	default:	return -1;
	case PGPPUBKEYALGO_RSA:
	    se += pgpMpiLen(se);
	    memmove(keyid, (se-8), 8);
	    /*@innerbreak@*/ break;
	}
      } break;
    case 4:
      {	rpmuint8_t * d = NULL;
	size_t dlen = 0;

	switch (*pkt) {
	default: assert(0);
	case 0x99: case 0x98: case 0xb9: case 0xb8: break;
	}

	{   DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	    rpmuint8_t goop[3];
	    goop[0] = 0x99;
	    goop[1] = (pp->hlen >> 8) & 0xff;
	    goop[2] = (pp->hlen     ) & 0xff;
	    (void) rpmDigestUpdate(ctx, goop, 3);
	    (void) rpmDigestUpdate(ctx, pp->u.h, pp->hlen);
	    (void) rpmDigestFinal(ctx, &d, &dlen, 0);
	}
	memmove(keyid, (d + (dlen-8)), 8);
	d = _free(d);
      }	break;
    }
    rc = 0;
    return rc;
}

int pgpExtractPubkeyFingerprint(const char * b64pkt, rpmuint8_t * keyid)
{
    const rpmuint8_t * pkt;
    size_t pktlen;

    if (b64decode(b64pkt, (void **)&pkt, &pktlen))
	return -1;	/* on error */
    (void) pgpPubkeyFingerprint(pkt, (unsigned int)pktlen, keyid);
    pkt = _free(pkt);
    return 8;	/* no. of bytes of pubkey signid */
}

int pgpPrtPkt(const rpmuint8_t * pkt, size_t pleft)
{
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    int rc = pgpPktLen(pkt, pleft, pp);

    if (rc < 0)
	return rc;

    switch (pp->tag) {
    case PGPTAG_SIGNATURE:
	rc = pgpPrtSig(pp);
	break;
    case PGPTAG_PUBLIC_KEY:
	/* Get the public key fingerprint. */
	if (_digp) {
/*@-mods@*/
	    if (!pgpPubkeyFingerprint(pkt, pp->pktlen, _digp->signid))
		_digp->saved |= PGPDIG_SAVED_ID;
	    else
		memset(_digp->signid, 0, sizeof(_digp->signid));
/*@=mods@*/
	}
	/*@fallthrough@*/
    case PGPTAG_PUBLIC_SUBKEY:
	rc = pgpPrtKey(pp);
	break;
    case PGPTAG_SECRET_KEY:
    case PGPTAG_SECRET_SUBKEY:
	rc = pgpPrtKey(pp);
	break;
    case PGPTAG_USER_ID:
	rc = pgpPrtUserID(pp);
	break;
    case PGPTAG_COMMENT:
    case PGPTAG_COMMENT_OLD:
	rc = pgpPrtComment(pp);
	break;

    case PGPTAG_RESERVED:
    case PGPTAG_PUBLIC_SESSION_KEY:
    case PGPTAG_SYMMETRIC_SESSION_KEY:
    case PGPTAG_COMPRESSED_DATA:
    case PGPTAG_SYMMETRIC_DATA:
    case PGPTAG_MARKER:
    case PGPTAG_LITERAL_DATA:
    case PGPTAG_TRUST:
    case PGPTAG_PHOTOID:
    case PGPTAG_ENCRYPTED_MDC:
    case PGPTAG_MDC:
    case PGPTAG_PRIVATE_60:
    case PGPTAG_PRIVATE_62:
    case PGPTAG_CONTROL:
    default:
	pgpPrtVal("", pgpTagTbl, (rpmuint8_t)pp->tag);
	pgpPrtHex("", pp->u.h, pp->hlen);
	pgpPrtNL();
	rc = 0;
	break;
    }

    return (rc ? -1 : (int)pp->pktlen);
}

/*@unchecked@*/
pgpVSFlags pgpDigVSFlags;

void pgpDigClean(pgpDig dig)
{
    if (dig != NULL) {
	dig->signature.userid = _free(dig->signature.userid);
	dig->pubkey.userid = _free(dig->pubkey.userid);
	dig->build_sign = _free(dig->build_sign);
	dig->pubkey_algoN = NULL;
	dig->hash_algoN = NULL;
	memset(&dig->dops, 0, sizeof(dig->dops));
	memset(&dig->sops, 0, sizeof(dig->sops));
	dig->ppkts = _free(dig->ppkts);
	dig->npkts = 0;
	dig->signature.hash = NULL;
	dig->signature.hashlen = 0;
	dig->pubkey.hash = NULL;
	dig->pubkey.hashlen = 0;

	memset(&dig->signature, 0, sizeof(dig->signature));
	memset(&dig->pubkey, 0, sizeof(dig->pubkey));

	dig->md5 = _free(dig->md5);
	dig->md5len = 0;
	dig->sha1 = _free(dig->sha1);
	dig->sha1len = 0;

	pgpImplClean(dig->impl);

    }
/*@-nullstate@*/
    return;
/*@=nullstate@*/
}

const char * pgpHashAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpHashTbl, (rpmuint8_t)algo);
}

const char * pgpPubkeyAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpPubkeyTbl, (rpmuint8_t)algo);
}

rpmuint8_t pgpHashName2Algo(const char * name)
{
    return pgpStrVal(pgpHashTbl, name);
}

rpmuint8_t pgpPubkeyName2Algo(const char * name)
{
    return pgpStrVal(pgpPubkeyTbl, name);
}

int pgpDigSetAlgos(pgpDig dig)
{
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
    char * t, * te;
    int rc = 0;

    t = rpmExpand("%{?_build_sign}", NULL);
    if (!(t && *t)) {
	t = _free(t);		/* coverity #1214094 */
	t = xstrdup("DSA");
    }
    dig->build_sign = t;

    if ((te = strrchr(t, '/')) != NULL)
	*te++ = '\0';
    else
	te = (!strcasecmp(t, "ECDSA") ? "SHA256" : "SHA1");

    dig->pubkey_algoN = t;
    dig->hash_algoN = te;

    if ((pubp->pubkey_algo = pgpPubkeyName2Algo(dig->pubkey_algoN)) == 0) {
	pubp->pubkey_algo = PGPPUBKEYALGO_DSA;
	sigp->hash_algo = PGPHASHALGO_SHA1;
    } else
    if ((sigp->hash_algo = pgpHashName2Algo(dig->hash_algoN)) == 0)
	sigp->hash_algo = PGPHASHALGO_SHA1;

    sigp->pubkey_algo = pubp->pubkey_algo;
    pubp->hash_algo = sigp->hash_algo;

    return rc;
}

static void pgpDigFini(void * __dig)
	/*@globals fileSystem, internalState @*/
	/*@modifies __dig, fileSystem, internalState @*/
{
    pgpDig dig = (pgpDig) __dig;

    dig->sig = _free(dig->sig);
    dig->siglen = 0;
    dig->pub = _free(dig->pub);
    dig->publen = 0;

    /* XXX there's a recursion here ... release and reacquire the lock */
#ifndef	BUGGY
    yarnRelease(dig->_item.use);
#endif
    /* Dump the signature/pubkey data. */
    pgpDigClean(dig);
#ifndef	BUGGY
    yarnPossess(dig->_item.use);
#endif

    if (dig->hrsa != NULL)
	(void) rpmDigestFinal(dig->hrsa, NULL, NULL, 0);
    dig->hrsa = NULL;

    if (dig->hdsa != NULL)
	(void) rpmDigestFinal(dig->hdsa, NULL, NULL, 0);
    dig->hdsa = NULL;

    if (dig->hecdsa != NULL)
	(void) rpmDigestFinal(dig->hecdsa, NULL, NULL, 0);
    dig->hecdsa = NULL;

    if (dig->hsha != NULL)
	(void) rpmDigestFinal(dig->hsha, NULL, NULL, 0);
    dig->hsha = NULL;

#ifdef	NOTYET
    if (dig->hdrmd5ctx != NULL)
	(void) rpmDigestFinal(dig->hdrmd5ctx, NULL, NULL, 0);
    dig->hdrmd5ctx = NULL;
#endif

    if (dig->md5ctx != NULL)
	(void) rpmDigestFinal(dig->md5ctx, NULL, NULL, 0);
    dig->md5ctx = NULL;

    dig->impl = pgpImplFree(dig->impl);

}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _digPool;

static pgpDig digGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _digPool, fileSystem @*/
	/*@modifies pool, _digPool, fileSystem @*/
{
    pgpDig dig;

    if (_digPool == NULL) {
	_digPool = rpmioNewPool("dig", sizeof(*dig), -1, _pgp_debug,
			NULL, NULL, pgpDigFini);
	pool = _digPool;
    }
    
    dig = (pgpDig) rpmioGetPool(pool, sizeof(*dig));
    memset(((char *)dig)+sizeof(dig->_item), 0, sizeof(*dig)-sizeof(dig->_item));
    return dig;
}

pgpDig pgpDigNew(pgpVSFlags vsflags, pgpPubkeyAlgo pubkey_algo)
{
    pgpDig dig = pgpDigLink( digGetPool(_digPool) );
    pgpDigParams pubp = pgpGetPubkey(dig);
    pgpDigParams sigp = pgpGetSignature(dig);
    int xx;

    /* XXX FIXME: always set default flags, ignore the arg. */
    dig->vsflags = (vsflags != RPMVSF_DEFAULT ? vsflags : pgpDigVSFlags);
    dig->impl = pgpImplInit();

    /* XXX FIXME: always set default pubkey_algo, ignore the arg. */
    pubp->pubkey_algo = pubkey_algo;
    sigp->pubkey_algo = pubkey_algo;

    if (pubp->pubkey_algo) {
	xx = pgpDigSetAlgos(dig);
	xx = pgpImplGenerate(dig);
assert(xx == 1);
	xx = pgpExportPubkey(dig);
    }
    return dig;
}

int pgpExportPubkey(pgpDig dig)
{
    int rc = 0;		/* assume failure */

#if defined(WITH_BEECRYPT)
    if (pgpImplVecs == &rpmbcImplVecs)
	rc = rpmbcExportPubkey(dig);
#endif
#if defined(WITH_SSL)
    if (pgpImplVecs == &rpmsslImplVecs)
	rc = rpmsslExportPubkey(dig);
#endif
#if defined(WITH_NSS)
    if (pgpImplVecs == &rpmnssImplVecs)
	rc = rpmnssExportPubkey(dig);
#endif
#if defined(WITH_GCRYPT)
    if (pgpImplVecs == &rpmgcImplVecs)
	rc = rpmgcExportPubkey(dig);
#endif
#if defined(WITH_TOMCRYPT)
    if (pgpImplVecs == &rpmltcImplVecs)
	rc = rpmltcExportPubkey(dig);
#endif
    return rc;
}

int pgpExportSignature(pgpDig dig, DIGEST_CTX ctx)
{
    int rc = 0;		/* assume failure */

#if defined(WITH_BEECRYPT)
    if (pgpImplVecs == &rpmbcImplVecs)
	rc = rpmbcExportSignature(dig, ctx);
#endif
#if defined(WITH_SSL)
    if (pgpImplVecs == &rpmsslImplVecs)
	rc = rpmsslExportSignature(dig, ctx);
#endif
#if defined(WITH_NSS)
    if (pgpImplVecs == &rpmnssImplVecs)
	rc = rpmnssExportSignature(dig, ctx);
#endif
#if defined(WITH_GCRYPT)
    if (pgpImplVecs == &rpmgcImplVecs)
	rc = rpmgcExportSignature(dig, ctx);
#endif
#if defined(WITH_TOMCRYPT)
    if (pgpImplVecs == &rpmltcImplVecs)
	rc = rpmltcExportSignature(dig, ctx);
#endif
    return rc;
}

pgpDigParams pgpGetSignature(pgpDig dig)
{
    return (dig ? &dig->signature : NULL);
}

pgpDigParams pgpGetPubkey(pgpDig dig)
{
    return (dig ? &dig->pubkey : NULL);
}

rpmuint32_t pgpGetSigtag(pgpDig dig)
{
    return (dig ? dig->sigtag : 0);
}

rpmuint32_t pgpGetSigtype(pgpDig dig)
{
    return (dig ? dig->sigtype : 0);
}

const void * pgpGetSig(pgpDig dig)
{
    return (dig ? dig->sig : NULL);
}

rpmuint32_t pgpGetSiglen(pgpDig dig)
{
    return (dig ? dig->siglen : 0);
}

int pgpSetSig(pgpDig dig,
	rpmuint32_t sigtag, rpmuint32_t sigtype, const void * sig, rpmuint32_t siglen)
{
    if (dig != NULL) {
	dig->sigtag = sigtag;
	dig->sigtype = (sig ? sigtype : 0);
/*@-assignexpose -kepttrans@*/
	dig->sig = sig;
/*@=assignexpose =kepttrans@*/
	dig->siglen = siglen;
    }
    return 0;
}

void * pgpStatsAccumulator(pgpDig dig, int opx)
{
    void * sw = NULL;
    switch (opx) {
    case 10:	/* RPMTS_OP_DIGEST */
	sw = &dig->dops;
	break;
    case 11:	/* RPMTS_OP_SIGNATURE */
	sw = &dig->sops;
	break;
    }
    return sw;
}

int pgpSetFindPubkey(pgpDig dig,
		int (*findPubkey) (void *ts, /*@null@*/ void *dig), void * _ts)
{
    if (dig) {
/*@-assignexpose@*/
	dig->findPubkey = findPubkey;
/*@=assignexpose@*/
/*@-dependenttrans@*/
	dig->_ts = _ts;
/*@=dependenttrans@*/
    }
    return 0;
}

int pgpFindPubkey(pgpDig dig)
{
    int rc = 1;	/* XXX RPMRC_NOTFOUND */
    if (dig && dig->findPubkey && dig->_ts)
	rc = (*dig->findPubkey) (dig->_ts, dig);
    return rc;
}

int pgpGrabPkts(const rpmuint8_t * pkts, size_t pktlen,
		/*@out@*/ rpmuint8_t *** pppkts, /*@out@*/ int * pnpkts)
	/*@modifies *pppkts, *pnpkts @*/
{
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    const rpmuint8_t * p;
    size_t pleft;
    size_t len;
    int npkts = 0;
    rpmuint8_t ** ppkts;

    for (p = pkts, pleft = pktlen; p < (pkts + pktlen); p += len, pleft -= len) {
	if (pgpPktLen(p, pleft, pp) < 0)
	    return -1;
	len = pp->pktlen;
	npkts++;
    }
    if (npkts <= 0)
	return -2;

    ppkts = (rpmuint8_t **) xcalloc(npkts+1, sizeof(*ppkts));

    npkts = 0;
    for (p = pkts, pleft = pktlen; p < (pkts + pktlen); p += len, pleft -= len) {
  
	if (pgpPktLen(p, pleft, pp) < 0) {
	    if (ppkts) ppkts = _free(ppkts);
	    return -1;
	}
	len = pp->pktlen;
	ppkts[npkts++] = (rpmuint8_t *) p;
    }
    ppkts[npkts] = NULL;

    if (pppkts != NULL)
	*pppkts = ppkts;
   else
	ppkts = _free(ppkts);

    if (pnpkts != NULL)
	*pnpkts = npkts;

    return 0;
}

/*@-globstate -incondefs -nullderef @*/	/* _dig annotations are not correct. */
int pgpPrtPkts(const rpmuint8_t * pkts, size_t pktlen, pgpDig dig, int printing)
	/*@globals _dig, _digp, _pgp_print @*/
	/*@modifies _dig, _digp, *_digp, _pgp_print @*/
{
    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
    unsigned int val = (unsigned int)*pkts;
    size_t pleft;
    int len;
    rpmuint8_t ** ppkts = NULL;
    int npkts;
    int i;

    _pgp_print = printing;
    _dig = pgpDigLink(dig);
    if (dig != NULL && (val & 0x80)) {
	pgpTag tag = (pgpTag)((val & 0x40) ? (val & 0x3f) : ((val >> 2) & 0xf));
	_digp = (tag == PGPTAG_SIGNATURE) ? &_dig->signature : &_dig->pubkey;
	_digp->tag = (rpmuint8_t)tag;
    } else
	_digp = NULL;

    if (pgpGrabPkts(pkts, pktlen, &ppkts, &npkts) || ppkts == NULL) {
	_dig = pgpDigFree(_dig);
	return -1;
    }

    if (ppkts != NULL)
    for (i = 0, pleft = pktlen; i < npkts; i++, pleft -= len) {
	len = pgpPktLen(ppkts[i], pleft, pp);
	len = pgpPrtPkt(ppkts[i], pp->pktlen);
    }

    if (dig != NULL) {
	dig->ppkts = _free(dig->ppkts);		/* XXX memory leak plugged. */
	dig->ppkts = ppkts;
	dig->npkts = npkts;
    } else
	ppkts = _free(ppkts);

    (void) pgpDigFree(_dig);
    _dig = NULL;

    return 0;
}
/*@=globstate =incondefs =nullderef @*/

pgpArmor pgpArmorUnwrap(rpmiob iob, rpmuint8_t ** pkt, size_t * pktlen)
{
    const char * enc = NULL;
    const char * crcenc = NULL;
    rpmuint8_t * dec;
    rpmuint8_t * crcdec;
    size_t declen;
    size_t crclen;
    rpmuint32_t crcpkt, crc;
    const char * armortype = NULL;
    char * t, * te;
    int pstate = 0;
    pgpArmor ec = PGPARMOR_ERR_NO_BEGIN_PGP;	/* XXX assume failure */
    pgpTag tag = (pgpTag)0;
    int rc;

    if (iob == NULL)
	goto exit;

    /* Read unarmored packets. */
    if (pgpIsPkt(iob->b, &tag)) {
	switch (tag) {
	default:		ec = PGPARMOR_NONE;	break;
	case PGPTAG_PUBLIC_KEY:	ec = PGPARMOR_PUBKEY;	break;
	case PGPTAG_SIGNATURE:	ec = PGPARMOR_SIGNATURE;	break;
#ifdef	NOTYET
	case PGPTAG_SECRET_KEY:	ec = PGPARMOR_SECKEY;	break;
	case PGPTAG_FOO:	ec = PGPARMOR_MESSAGE;	break;
	case PGPTAG_FOO:	ec = PGPARMOR_SIGNED_MESSAGE;	break;
	case PGPTAG_FOO:	ec = PGPARMOR_FILE;	break;
	case PGPTAG_FOO:	ec = PGPARMOR_PRIVKEY;	break;
#endif
	}
	if (ec != PGPARMOR_NONE) {
	    pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
#ifdef	DYING
	    /* XXX Truncate blen to actual no. of octets in first packet. */
	    iob->blen = pgpPktLen(iob->b, iob->blen, pp);
#else
	    (void) pgpPktLen(iob->b, iob->blen, pp);
#endif
	}
	goto exit;
    }

#define	TOKEQ(_s, _tok)	(!strncmp((_s), (_tok), sizeof(_tok)-1))

    /* Read armored packets, converting to binary. */
    for (t = (char *)iob->b; t && *t; t = te) {
	if ((te = strchr(t, '\n')) == NULL)
	    te = t + strlen(t);
	else
	    te++;

	switch (pstate) {
	case 0:
	    armortype = NULL;
	    if (!TOKEQ(t, "-----BEGIN PGP "))
		continue;
	    t += sizeof("-----BEGIN PGP ")-1;

	    rc = pgpValTok(pgpArmorTbl, t, te);
	    if (rc < 0) {
		ec = PGPARMOR_ERR_UNKNOWN_ARMOR_TYPE;
		goto exit;
	    }
	    /* XXX Ignore clear signed message start. */
	    if (rc == PGPARMOR_SIGNED_MESSAGE)
		continue;
	    ec = (pgpArmor)rc;	/* Save the packet type as exit code. */
	    armortype = t;

	    t = strchr(t, '\n');
	    if (t == NULL)
		continue;
	    if (t[-1] == '\r')
		--t;
	    t -= (sizeof("-----")-1);
	    if (!TOKEQ(t, "-----"))
		continue;
	    *t = '\0';
	    pstate++;
	    /*@switchbreak@*/ break;
	case 1:
	    enc = NULL;
	    rc = pgpValTok(pgpArmorKeyTbl, t, te);
	    if (rc >= 0)
		continue;
	    if (!(*t == '\n' || *t == '\r')) {
		pstate = 0;
		continue;
	    }
	    enc = te;		/* Start of encoded packets */
	    pstate++;
	    /*@switchbreak@*/ break;
	case 2:
	    crcenc = NULL;
	    if (*t != '=')
		continue;
	    *t++ = '\0';	/* Terminate encoded packets */
	    crcenc = t;		/* Start of encoded crc */
	    pstate++;
	    /*@switchbreak@*/ break;
	case 3:
	    pstate = 0;
	    if (!TOKEQ(t, "-----END PGP ")) {
		ec = PGPARMOR_ERR_NO_END_PGP;
		goto exit;
	    }
	    *t = '\0';		/* Terminate encoded crc */
	    t += sizeof("-----END PGP ")-1;
	    if (t >= te) continue;

	    if (armortype == NULL) /* XXX can't happen */
		continue;
	    rc = strncmp(t, armortype, strlen(armortype));
	    if (rc)
		continue;

	    t += strlen(armortype);
	    if (t >= te) continue;

	    if (!TOKEQ(t, "-----")) {
		ec = PGPARMOR_ERR_NO_END_PGP;
		goto exit;
	    }
	    t += (sizeof("-----")-1);
	    if (t >= te) continue;
	    /* XXX permitting \r here is not RFC-2440 compliant <shrug> */
	    if (!(*t == '\n' || *t == '\r')) continue;

	    crcdec = NULL;
	    crclen = 0;
	    if (b64decode(crcenc, (void **)&crcdec, &crclen) != 0) {
		ec = PGPARMOR_ERR_CRC_DECODE;
		goto exit;
	    }
	    crcpkt = pgpGrab(crcdec, crclen);
	    crcdec = _free(crcdec);
	    dec = NULL;
	    declen = 0;
	    if (b64decode(enc, (void **)&dec, &declen) != 0) {
		ec = PGPARMOR_ERR_BODY_DECODE;
		goto exit;
	    }
	    crc = pgpCRC(dec, declen);
	    if (crcpkt != crc) {
		ec = PGPARMOR_ERR_CRC_CHECK;
		dec = _free(dec);	/* XXX coverity #1035921 */
		declen = 0;
		goto exit;
	    }
	    iob->b = _free(iob->b);
	    iob->b = dec;
	    iob->blen = declen;
	    dec = NULL;
	    declen = 0;
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}
    }
    ec = PGPARMOR_NONE;

exit:
    if (ec > PGPARMOR_NONE) {
	if (pkt)	*pkt = iob->b;
	if (pktlen)	*pktlen = iob->blen;
	iob->b = NULL;	/* XXX iob->b has been stolen */
    } else {
	if (pkt)	*pkt = NULL;
	if (pktlen)	*pktlen = 0;
    }
    return ec;
}

pgpArmor pgpReadPkts(const char * fn, rpmuint8_t ** pkt, size_t * pktlen)
{
    rpmiob iob = NULL;
    pgpArmor ec = !rpmiobSlurp(fn, &iob)
	? pgpArmorUnwrap(iob, pkt, pktlen)
	: PGPARMOR_ERR_NO_BEGIN_PGP;
    iob = rpmiobFree(iob);
    return ec;
}

char * pgpArmorWrap(rpmuint8_t atype, const unsigned char * s, size_t ns)
{
    const char * enc;
    char * t;
    size_t nt;
    char * val;
    int lc;

    nt = ((ns + 2) / 3) * 4;
    /*@-globs@*/
    /* Add additional bytes necessary for eol string(s). */
    if (b64encode_chars_per_line > 0 && b64encode_eolstr != NULL) {
	lc = (nt + b64encode_chars_per_line - 1) / b64encode_chars_per_line;
       if (((nt + b64encode_chars_per_line - 1) % b64encode_chars_per_line) != 0)
        ++lc;
	nt += lc * strlen(b64encode_eolstr);
    }
    /*@=globs@*/

    nt += 512;	/* XXX slop for armor and crc */

    val = t = (char *) xmalloc(nt + 1);
    *t = '\0';
    t = stpcpy(t, "-----BEGIN PGP ");
    t = stpcpy(t, pgpValStr(pgpArmorTbl, atype));
    /*@-globs@*/
    t = stpcpy( stpcpy(t, "-----\nVersion: RPM "), VERSION);
    /*@=globs@*/

    if (pgpImplVecs->_pgpNV && pgpImplVecs->_pgpNV[0]) {
	t = stpcpy(t, " (");
	t = stpcpy(t, pgpImplVecs->_pgpNV);
	t = stpcpy(t, ")");
    }

    t = stpcpy(t, "\n\n");

    if ((enc = b64encode(s, ns)) != NULL) {
	t = stpcpy(t, enc);
	enc = _free(enc);
	if ((enc = b64crc(s, ns)) != NULL) {
	    *t++ = '=';
	    t = stpcpy(t, enc);
	    enc = _free(enc);
	}
    }
	
    t = stpcpy(t, "-----END PGP ");
    t = stpcpy(t, pgpValStr(pgpArmorTbl, atype));
    t = stpcpy(t, "-----\n");

/*@-globstate@*/	/* XXX b64encode_eolstr needs annotation. */
    return val;
/*@=globstate@*/
}

pgpHashAlgo pgpHashAlgoStringToNumber(const char *name, size_t name_len)
{
    size_t i;

    if (name == NULL)
        return (pgpHashAlgo) -1;
    if (name_len == 0)
        name_len = strlen(name);
    for (i = 0; i < sizeof(pgpHashTbl)/sizeof(pgpHashTbl[0]); i++)
        if (xstrncasecmp(name, pgpHashTbl[i].str, name_len) == 0)
            return (pgpHashAlgo) pgpHashTbl[i].val;
    return PGPHASHALGO_ERROR;
}

