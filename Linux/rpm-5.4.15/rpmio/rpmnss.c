/** \ingroup rpmpgp
 * \file rpmio/rpmnss.c
 */

#include "system.h"
#include <rpmlog.h>

#include <rpmiotypes.h>
#define	_RPMPGP_INTERNAL
#if defined(WITH_NSS)
#define	_RPMNSS_INTERNAL
#include <rpmnss.h>
#endif
#include <rpmmacro.h>

#include "debug.h"

#if defined(WITH_NSS)

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

/*@unchecked@*/
extern int _rpmnss_init;

/*@unchecked@*/
extern void * _rpmnss_context;

/*@unchecked@*/
static int _rpmnss_debug;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmnss_debug || _pgp_debug < 0) \
	fprintf(stderr, "<-- %s(%p) %s\t%s/%s\n", __FUNCTION__, (_dig), \
		((_rc) ? "OK" : "BAD"), (_dig)->pubkey_algoN, (_dig)->hash_algoN); \
  }

/* XXX gcc has __builtin_clz */
#  if !defined __GNUC__ || __GNUC__ < 3
static int __builtin_clz(uint32_t x)
{
    int n = 0;

    if (x == 0)
	return 32;
    if ((x & 0xffff0000) == 0) { n += 16; x << 16; }
    if ((x & 0xff000000) == 0) { n +=  8; x <<  8; }
    if ((x & 0xf0000000) == 0) { n +=  4; x <<  4; }
    if ((x & 0xc0000000) == 0) { n +=  2; x <<  2; }
    if ((x & 0x80000000) == 0) { n +=  1; x <<  1; }
    return n;
}
#endif

/*==============================================================*/

typedef struct keyNV_s {
/*@observer@*/
    const char * N;		/* key name */
    uint32_t V;
} keyNV_t;

static int
keyNVCmp(const void * a, const void * b)
{
    return strcmp(((keyNV_t *)a)->N, ((keyNV_t *)b)->N);
}

static uint32_t
keyNV(keyNV_t * keys, size_t nkeys, /*@null@*/ const char *N)
{
    uint32_t V = 0;

    if (N && *N) {
	keyNV_t needle = { .N = N, .V = 0 };
	keyNV_t *k = (keyNV_t *)
		bsearch(&needle, keys, nkeys, sizeof(*keys), keyNVCmp);
	if (k)
	    V = k->V;
    }
    return V;
}

typedef struct keyVN_s {
    int V;
/*@observer@*/
    const char * N;		/* key name */
} keyVN_t;

static int
keyVNCmp(const void * a, const void * b)
{
    return (((keyVN_t *)a)->V - ((keyVN_t *)b)->V);
}

static const char *
keyVN(keyVN_t * keys, size_t nkeys, /*@null@*/ int V)
{
    const char * N = NULL;

    if (V) {
	/* XXX bsearch is overkill */
	keyVN_t needle = {};
	keyVN_t *k;

	needle.V = V;
	needle.N = NULL;
	k = (keyVN_t *)
		bsearch(&needle, keys, nkeys, sizeof(*keys), keyVNCmp);
	if (k)
	    N = k->N;
    }
    return N;
}

/*==============================================================*/

extern SECStatus
EC_DecodeParams(const SECItem *encodedParams, ECParams **ecparams);

static keyNV_t rpmnssOIDS[] = {
  { "c2onb191v4", SEC_OID_ANSIX962_EC_C2ONB191V4 },
  { "c2onb191v5", SEC_OID_ANSIX962_EC_C2ONB191V5 },
  { "c2onb239v4", SEC_OID_ANSIX962_EC_C2ONB239V4 },
  { "c2onb239v5", SEC_OID_ANSIX962_EC_C2ONB239V5 },
  { "c2pnb163v1", SEC_OID_ANSIX962_EC_C2PNB163V1 },
  { "c2pnb163v2", SEC_OID_ANSIX962_EC_C2PNB163V2 },
  { "c2pnb163v3", SEC_OID_ANSIX962_EC_C2PNB163V3 },
  { "c2pnb176v1", SEC_OID_ANSIX962_EC_C2PNB176V1 },
  { "c2pnb208w1", SEC_OID_ANSIX962_EC_C2PNB208W1 },
  { "c2pnb272w1", SEC_OID_ANSIX962_EC_C2PNB272W1 },
  { "c2pnb304w1", SEC_OID_ANSIX962_EC_C2PNB304W1 },
  { "c2pnb368w1", SEC_OID_ANSIX962_EC_C2PNB368W1 },
  { "c2tnb191v1", SEC_OID_ANSIX962_EC_C2TNB191V1 },
  { "c2tnb191v2", SEC_OID_ANSIX962_EC_C2TNB191V2 },
  { "c2tnb191v3", SEC_OID_ANSIX962_EC_C2TNB191V3 },
  { "c2tnb239v1", SEC_OID_ANSIX962_EC_C2TNB239V1 },
  { "c2tnb239v2", SEC_OID_ANSIX962_EC_C2TNB239V2 },
  { "c2tnb239v3", SEC_OID_ANSIX962_EC_C2TNB239V3 },
  { "c2tnb359v1", SEC_OID_ANSIX962_EC_C2TNB359V1 },
  { "c2tnb431r1", SEC_OID_ANSIX962_EC_C2TNB431R1 },
  { "nistb163", SEC_OID_SECG_EC_SECT163R2},
  { "nistb233", SEC_OID_SECG_EC_SECT233R1},
  { "nistb283", SEC_OID_SECG_EC_SECT283R1},
  { "nistb409", SEC_OID_SECG_EC_SECT409R1},
  { "nistb571", SEC_OID_SECG_EC_SECT571R1},
  { "nistk163", SEC_OID_SECG_EC_SECT163K1},
  { "nistk233", SEC_OID_SECG_EC_SECT233K1},
  { "nistk283", SEC_OID_SECG_EC_SECT283K1},
  { "nistk409", SEC_OID_SECG_EC_SECT409K1},
  { "nistk571", SEC_OID_SECG_EC_SECT571K1},
  { "nistp192", SEC_OID_SECG_EC_SECP192R1},
  { "nistp224", SEC_OID_SECG_EC_SECP224R1},
  { "nistp256", SEC_OID_SECG_EC_SECP256R1},
  { "nistp384", SEC_OID_SECG_EC_SECP384R1},
  { "nistp521", SEC_OID_SECG_EC_SECP521R1},
  { "prime192v1", SEC_OID_ANSIX962_EC_PRIME192V1 },
  { "prime192v2", SEC_OID_ANSIX962_EC_PRIME192V2 },
  { "prime192v3", SEC_OID_ANSIX962_EC_PRIME192V3 },
  { "prime239v1", SEC_OID_ANSIX962_EC_PRIME239V1 },
  { "prime239v2", SEC_OID_ANSIX962_EC_PRIME239V2 },
  { "prime239v3", SEC_OID_ANSIX962_EC_PRIME239V3 },
  { "secp112r1", SEC_OID_SECG_EC_SECP112R1},
  { "secp112r2", SEC_OID_SECG_EC_SECP112R2},
  { "secp128r1", SEC_OID_SECG_EC_SECP128R1},
  { "secp128r2", SEC_OID_SECG_EC_SECP128R2},
  { "secp160k1", SEC_OID_SECG_EC_SECP160K1},
  { "secp160r1", SEC_OID_SECG_EC_SECP160R1},
  { "secp160r2", SEC_OID_SECG_EC_SECP160R2},
  { "secp192k1", SEC_OID_SECG_EC_SECP192K1},
  { "secp192r1", SEC_OID_SECG_EC_SECP192R1},
  { "secp224k1", SEC_OID_SECG_EC_SECP224K1},
  { "secp224r1", SEC_OID_SECG_EC_SECP224R1},
  { "secp256k1", SEC_OID_SECG_EC_SECP256K1},
  { "secp256r1", SEC_OID_SECG_EC_SECP256R1},
  { "secp384r1", SEC_OID_SECG_EC_SECP384R1},
  { "secp521r1", SEC_OID_SECG_EC_SECP521R1},
  { "sect113r1", SEC_OID_SECG_EC_SECT113R1},
  { "sect113r2", SEC_OID_SECG_EC_SECT113R2},
  { "sect131r1", SEC_OID_SECG_EC_SECT131R1},
  { "sect131r2", SEC_OID_SECG_EC_SECT131R2},
  { "sect163k1", SEC_OID_SECG_EC_SECT163K1},
  { "sect163r1", SEC_OID_SECG_EC_SECT163R1},
  { "sect163r2", SEC_OID_SECG_EC_SECT163R2},
  { "sect193r1", SEC_OID_SECG_EC_SECT193R1},
  { "sect193r2", SEC_OID_SECG_EC_SECT193R2},
  { "sect233k1", SEC_OID_SECG_EC_SECT233K1},
  { "sect233r1", SEC_OID_SECG_EC_SECT233R1},
  { "sect239k1", SEC_OID_SECG_EC_SECT239K1},
  { "sect283k1", SEC_OID_SECG_EC_SECT283K1},
  { "sect283r1", SEC_OID_SECG_EC_SECT283R1},
  { "sect409k1", SEC_OID_SECG_EC_SECT409K1},
  { "sect409r1", SEC_OID_SECG_EC_SECT409R1},
  { "sect571k1", SEC_OID_SECG_EC_SECT571K1},
  { "sect571r1", SEC_OID_SECG_EC_SECT571R1},
};
static size_t nrpmnssOIDS = sizeof(rpmnssOIDS) / sizeof(rpmnssOIDS[0]);

#define _ENTRY(_v)      { SEC_ERROR_##_v, #_v }
/* XXX sorted table */
static keyVN_t rpmnssERRS[] = {
    _ENTRY(IO),
    _ENTRY(LIBRARY_FAILURE),
    _ENTRY(BAD_DATA),
    _ENTRY(OUTPUT_LEN),
    _ENTRY(INPUT_LEN),
    _ENTRY(INVALID_ARGS),
    _ENTRY(INVALID_ALGORITHM),
    _ENTRY(INVALID_AVA),
    _ENTRY(INVALID_TIME),
    _ENTRY(BAD_DER),
    _ENTRY(BAD_SIGNATURE),
    _ENTRY(EXPIRED_CERTIFICATE),
    _ENTRY(REVOKED_CERTIFICATE),
    _ENTRY(UNKNOWN_ISSUER),
    _ENTRY(BAD_KEY),
    _ENTRY(BAD_PASSWORD),
    _ENTRY(RETRY_PASSWORD),
    _ENTRY(NO_NODELOCK),
    _ENTRY(BAD_DATABASE),
    _ENTRY(NO_MEMORY),
    _ENTRY(UNTRUSTED_ISSUER),
    _ENTRY(UNTRUSTED_CERT),
    _ENTRY(DUPLICATE_CERT),
    _ENTRY(DUPLICATE_CERT_NAME),
    _ENTRY(ADDING_CERT),
    _ENTRY(FILING_KEY),
    _ENTRY(NO_KEY),
    _ENTRY(CERT_VALID),
    _ENTRY(CERT_NOT_VALID),
    _ENTRY(CERT_NO_RESPONSE),
    _ENTRY(EXPIRED_ISSUER_CERTIFICATE),
    _ENTRY(CRL_EXPIRED),
    _ENTRY(CRL_BAD_SIGNATURE),
    _ENTRY(CRL_INVALID),
    _ENTRY(EXTENSION_VALUE_INVALID),
    _ENTRY(EXTENSION_NOT_FOUND),
    _ENTRY(CA_CERT_INVALID),
    _ENTRY(PATH_LEN_CONSTRAINT_INVALID),
    _ENTRY(CERT_USAGES_INVALID),
/* SEC_INTERNAL_ONLY */
    _ENTRY(INVALID_KEY),
    _ENTRY(UNKNOWN_CRITICAL_EXTENSION),
    _ENTRY(OLD_CRL),
    _ENTRY(NO_EMAIL_CERT),
    _ENTRY(NO_RECIPIENT_CERTS_QUERY),
    _ENTRY(NOT_A_RECIPIENT),
    _ENTRY(PKCS7_KEYALG_MISMATCH),
    _ENTRY(PKCS7_BAD_SIGNATURE),
    _ENTRY(UNSUPPORTED_KEYALG),
    _ENTRY(DECRYPTION_DISALLOWED),
/* Fortezza Alerts */
/* XP_SEC_FORTEZZA_BAD_CARD */
/* XP_SEC_FORTEZZA_NO_CARD */
/* XP_SEC_FORTEZZA_NONE_SELECTED */
/* XP_SEC_FORTEZZA_MORE_INFO */
/* XP_SEC_FORTEZZA_PERSON_NOT_FOUND */
/* XP_SEC_FORTEZZA_NO_MORE_INFO */
/* XP_SEC_FORTEZZA_BAD_PIN */
/* XP_SEC_FORTEZZA_PERSON_ERROR */
    _ENTRY(NO_KRL),
    _ENTRY(KRL_EXPIRED),
    _ENTRY(KRL_BAD_SIGNATURE),
    _ENTRY(REVOKED_KEY),
    _ENTRY(KRL_INVALID),
    _ENTRY(NEED_RANDOM),
    _ENTRY(NO_MODULE),
    _ENTRY(NO_TOKEN),
    _ENTRY(READ_ONLY),
    _ENTRY(NO_SLOT_SELECTED),
    _ENTRY(CERT_NICKNAME_COLLISION),
    _ENTRY(KEY_NICKNAME_COLLISION),
    _ENTRY(SAFE_NOT_CREATED),
    _ENTRY(BAGGAGE_NOT_CREATED),
/* XP_JAVA_REMOVE_PRINCIPAL_ERROR */
/* XP_JAVA_DELETE_PRIVILEGE_ERROR */
/* XP_JAVA_CERT_NOT_EXISTS_ERROR */
    _ENTRY(BAD_EXPORT_ALGORITHM),
    _ENTRY(EXPORTING_CERTIFICATES),
    _ENTRY(IMPORTING_CERTIFICATES),
    _ENTRY(PKCS12_DECODING_PFX),
    _ENTRY(PKCS12_INVALID_MAC),
    _ENTRY(PKCS12_UNSUPPORTED_MAC_ALGORITHM),
    _ENTRY(PKCS12_UNSUPPORTED_TRANSPORT_MODE),
    _ENTRY(PKCS12_CORRUPT_PFX_STRUCTURE),
    _ENTRY(PKCS12_UNSUPPORTED_PBE_ALGORITHM),
    _ENTRY(PKCS12_UNSUPPORTED_VERSION),
    _ENTRY(PKCS12_PRIVACY_PASSWORD_INCORRECT),
    _ENTRY(PKCS12_CERT_COLLISION),
    _ENTRY(USER_CANCELLED),
    _ENTRY(PKCS12_DUPLICATE_DATA),
    _ENTRY(MESSAGE_SEND_ABORTED),
    _ENTRY(INADEQUATE_KEY_USAGE),
    _ENTRY(INADEQUATE_CERT_TYPE),
    _ENTRY(CERT_ADDR_MISMATCH),
    _ENTRY(PKCS12_UNABLE_TO_IMPORT_KEY),
    _ENTRY(PKCS12_IMPORTING_CERT_CHAIN),
    _ENTRY(PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME),
    _ENTRY(PKCS12_UNABLE_TO_EXPORT_KEY),
    _ENTRY(PKCS12_UNABLE_TO_WRITE),
    _ENTRY(PKCS12_UNABLE_TO_READ),
    _ENTRY(PKCS12_KEY_DATABASE_NOT_INITIALIZED),
    _ENTRY(KEYGEN_FAIL),
    _ENTRY(INVALID_PASSWORD),
    _ENTRY(RETRY_OLD_PASSWORD),
    _ENTRY(BAD_NICKNAME),
    _ENTRY(NOT_FORTEZZA_ISSUER),
    _ENTRY(CANNOT_MOVE_SENSITIVE_KEY),
    _ENTRY(JS_INVALID_MODULE_NAME),
    _ENTRY(JS_INVALID_DLL),
    _ENTRY(JS_ADD_MOD_FAILURE),
    _ENTRY(JS_DEL_MOD_FAILURE),
    _ENTRY(OLD_KRL),
    _ENTRY(CKL_CONFLICT),
    _ENTRY(CERT_NOT_IN_NAME_SPACE),
    _ENTRY(KRL_NOT_YET_VALID),
    _ENTRY(CRL_NOT_YET_VALID),
    _ENTRY(UNKNOWN_CERT),
    _ENTRY(UNKNOWN_SIGNER),
    _ENTRY(CERT_BAD_ACCESS_LOCATION),
    _ENTRY(OCSP_UNKNOWN_RESPONSE_TYPE),
    _ENTRY(OCSP_BAD_HTTP_RESPONSE),
    _ENTRY(OCSP_MALFORMED_REQUEST),
    _ENTRY(OCSP_SERVER_ERROR),
    _ENTRY(OCSP_TRY_SERVER_LATER),
    _ENTRY(OCSP_REQUEST_NEEDS_SIG),
    _ENTRY(OCSP_UNAUTHORIZED_REQUEST),
    _ENTRY(OCSP_UNKNOWN_RESPONSE_STATUS),
    _ENTRY(OCSP_UNKNOWN_CERT),
    _ENTRY(OCSP_NOT_ENABLED),
    _ENTRY(OCSP_NO_DEFAULT_RESPONDER),
    _ENTRY(OCSP_MALFORMED_RESPONSE),
    _ENTRY(OCSP_UNAUTHORIZED_RESPONSE),
    _ENTRY(OCSP_FUTURE_RESPONSE),
    _ENTRY(OCSP_OLD_RESPONSE),
/* smime stuff */
    _ENTRY(DIGEST_NOT_FOUND),
    _ENTRY(UNSUPPORTED_MESSAGE_TYPE),
    _ENTRY(MODULE_STUCK),
    _ENTRY(BAD_TEMPLATE),
    _ENTRY(CRL_NOT_FOUND),
    _ENTRY(REUSED_ISSUER_AND_SERIAL),
    _ENTRY(BUSY),
    _ENTRY(EXTRA_INPUT),
/* error codes used by elliptic curve code */
    _ENTRY(UNSUPPORTED_ELLIPTIC_CURVE),
    _ENTRY(UNSUPPORTED_EC_POINT_FORM),
    _ENTRY(UNRECOGNIZED_OID),
    _ENTRY(OCSP_INVALID_SIGNING_CERT),
/* new revocation errors */
    _ENTRY(REVOKED_CERTIFICATE_CRL),
    _ENTRY(REVOKED_CERTIFICATE_OCSP),
    _ENTRY(CRL_INVALID_VERSION),
    _ENTRY(CRL_V1_CRITICAL_EXTENSION),
    _ENTRY(CRL_UNKNOWN_CRITICAL_EXTENSION),
    _ENTRY(UNKNOWN_OBJECT_TYPE),
    _ENTRY(INCOMPATIBLE_PKCS11),
    _ENTRY(NO_EVENT),
    _ENTRY(CRL_ALREADY_EXISTS),
    _ENTRY(NOT_INITIALIZED),
    _ENTRY(TOKEN_NOT_LOGGED_IN),
    _ENTRY(OCSP_RESPONDER_CERT_INVALID),
    _ENTRY(OCSP_BAD_SIGNATURE),
    _ENTRY(OUT_OF_SEARCH_LIMITS),
    _ENTRY(INVALID_POLICY_MAPPING),
    _ENTRY(POLICY_VALIDATION_FAILED),
/* No longer used.  Unknown AIA location types are now silently ignored. */
    _ENTRY(UNKNOWN_AIA_LOCATION_TYPE),
    _ENTRY(BAD_HTTP_RESPONSE),
    _ENTRY(BAD_LDAP_RESPONSE),
    _ENTRY(FAILED_TO_ENCODE_DATA),
    _ENTRY(BAD_INFO_ACCESS_LOCATION),
    _ENTRY(LIBPKIX_INTERNAL),
    _ENTRY(PKCS11_GENERAL_ERROR),
    _ENTRY(PKCS11_FUNCTION_FAILED),
    _ENTRY(PKCS11_DEVICE_ERROR),
#if defined(SEC_ERROR_BAD_INFO_ACCESS_METHOD)
    _ENTRY(BAD_INFO_ACCESS_METHOD),
#endif
#if defined(SEC_ERROR_CRL_IMPORT_FAILED)
    _ENTRY(CRL_IMPORT_FAILED),
#endif
#if defined(SEC_ERROR_EXPIRED_PASSWORD)
    _ENTRY(EXPIRED_PASSWORD),
#endif
#if defined(SEC_ERROR_LOCKED_PASSWORD)
    _ENTRY(LOCKED_PASSWORD),
#endif
#if defined(SEC_ERROR_UNKNOWN_PKCS11_ERROR)
    _ENTRY(UNKNOWN_PKCS11_ERROR),
#endif
#if defined(SEC_ERROR_BAD_CRL_DP_URL)
    _ENTRY(BAD_CRL_DP_URL),
#endif
#if defined(SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED)
    _ENTRY(CERT_SIGNATURE_ALGORITHM_DISABLED),
#endif
#if defined(SEC_ERROR_LEGACY_DATABASE)
    _ENTRY(LEGACY_DATABASE),
#endif
#if defined(SEC_ERROR_APPLICATION_CALLBACK_ERROR)
    _ENTRY(APPLICATION_CALLBACK_ERROR),
#endif
};
static size_t nrpmnssERRS = sizeof(rpmnssERRS) / sizeof(rpmnssERRS[0]);
#undef _ENTRY

static uint32_t curve2oid(const char * name)
{
    uint32_t oid = keyNV(rpmnssOIDS, nrpmnssOIDS, name);
    if (oid == 0)
	oid = SEC_OID_UNKNOWN;

if (_pgp_debug)
fprintf(stderr, "<-- %s(%s) oid %u\n", __FUNCTION__, name, oid);

    return oid;
}

static const char * rpmnssStrerror(int err)
{
    static char buf[64];
    const char * errN = keyVN(rpmnssERRS, nrpmnssERRS, err);
    if (errN == NULL) {
	snprintf(buf, sizeof(buf), "SEC_ERROR(%d)", err);
	errN = buf;
    }
    return errN;
}

static
int rpmnssErr(rpmnss nss, const char * msg, int rc)
	/*@*/
{
#ifdef	REFERENCE
    /* XXX Don't spew on expected failures ... */
    if (err && gcry_err_code(err) != gc->badok)
	fprintf (stderr, "rpmgc: %s(0x%0x): %s/%s\n",
		msg, (unsigned)err, gcry_strsource(err), gcry_strerror(err));
#endif
    if (rc != SECSuccess) {
	int err = PORT_GetError();
	fprintf (stderr, "rpmnss: %s rc(%d) err(%d) %s\n",
		msg, rc, err, rpmnssStrerror(err));
    }
    return rc;
}

/*==============================================================*/
static SECOidTag getEncAlg(unsigned pubkey_algo, unsigned hash_algo)
{
    SECOidTag encAlg = SEC_OID_UNKNOWN;

    switch (pubkey_algo) {
    case PGPPUBKEYALGO_RSA:
	switch (hash_algo) {
	case PGPHASHALGO_MD2:
	    encAlg = SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION;	break;
#ifdef	DEAD	/* XXX not implemented in modern NSS */
	case PGPHASHALGO_MD4:
	    encAlg = SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION;	break;
#endif
	case PGPHASHALGO_MD5:
	    encAlg = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;	break;
	case PGPHASHALGO_SHA1:
	    encAlg = SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION;	break;
	case PGPHASHALGO_SHA224:
	    encAlg = SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION;	break;
	case PGPHASHALGO_SHA256:
	    encAlg = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;	break;
	case PGPHASHALGO_SHA384:
	    encAlg = SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION;	break;
	case PGPHASHALGO_SHA512:
	    encAlg = SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION;	break;
	default:
	    encAlg = SEC_OID_PKCS1_RSA_ENCRYPTION;	break;
	}
	break;
    case PGPPUBKEYALGO_DSA:
	switch(hash_algo) {
	case PGPHASHALGO_SHA1:
	    encAlg = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;	break;
	case PGPHASHALGO_SHA224:
	    encAlg = SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA224_DIGEST;	break;
	case PGPHASHALGO_SHA256:
	    encAlg = SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA256_DIGEST;	break;
	default:
	    encAlg = SEC_OID_ANSIX9_DSA_SIGNATURE;	break;
	}
	break;
    case PGPPUBKEYALGO_ECDSA:
	switch(hash_algo) {
	case PGPHASHALGO_SHA1:
	    encAlg = SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE;	break;
	case PGPHASHALGO_SHA224:
	    encAlg = SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE;	break;
	case PGPHASHALGO_SHA256:
	    encAlg = SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE;	break;
	case PGPHASHALGO_SHA384:
	    encAlg = SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE;	break;
	case PGPHASHALGO_SHA512:
	    encAlg = SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE;	break;
#ifdef	NOTYET
	case SEC_OID_ANSIX962_ECDSA_SIGNATURE_RECOMMENDED_DIGEST:
	case SEC_OID_ANSIX962_ECDSA_SIGNATURE_SPECIFIED_DIGEST:
#endif
	default:
	    encAlg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;	break;
	}
	break;
    case PGPPUBKEYALGO_ELGAMAL:	/*@fallthrough@*/
    default:
	break;
    }
    return encAlg;
}

static SECOidTag getHashAlg(unsigned hash_algo)
{
    SECOidTag hashAlg = SEC_OID_UNKNOWN;

    switch (hash_algo) {
    case PGPHASHALGO_MD2:	hashAlg = SEC_OID_MD2;		break;
#ifdef	DEAD	/* XXX not implemented in modern NSS? */
    case PGPHASHALGO_MD4:	hashAlg = SEC_OID_MD4;		break;
#endif
    case PGPHASHALGO_MD5:	hashAlg = SEC_OID_MD5;		break;
    case PGPHASHALGO_SHA1:	hashAlg = SEC_OID_SHA1;		break;
    case PGPHASHALGO_SHA224:	hashAlg = SEC_OID_SHA224;	break;
    case PGPHASHALGO_SHA256:	hashAlg = SEC_OID_SHA256;	break;
    case PGPHASHALGO_SHA384:	hashAlg = SEC_OID_SHA384;	break;
    case PGPHASHALGO_SHA512:	hashAlg = SEC_OID_SHA512;	break;
    case PGPHASHALGO_RIPEMD160:	/*@fallthrough@*/
    case PGPHASHALGO_TIGER192:	/*@fallthrough@*/
    case PGPHASHALGO_HAVAL_5_160:	/*@fallthrough@*/
    default:
	break;
    }
    return hashAlg;
}

/*==============================================================*/

static
int rpmnssSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 1;		/* assume error */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_RSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_RSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

nss->digest = _free(nss->digest);
nss->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    nss->encAlg = getEncAlg(sigp->pubkey_algo, 0);	/* XXX hash_algo? */
    nss->hashAlg = getHashAlg(sigp->hash_algo);
    if (nss->hashAlg == SEC_OID_UNKNOWN)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmnssGenerateRSA(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure */

if (nss->nbits == 0) nss->nbits = 1024; /* XXX FIXME */
assert(nss->nbits);

    {	void * _cx = NULL;
	CK_MECHANISM_TYPE _type = CKM_RSA_PKCS_KEY_PAIR_GEN;
	PK11SlotInfo * _slot = PK11_GetBestSlot(_type, _cx);
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_TRUE;

	if (_slot) {
	    static unsigned _pe = 0x10001;	/* XXX FIXME: pass in e */
	    PK11RSAGenParams rsaparams = {};
	    void * params = &rsaparams;

	    rsaparams.keySizeInBits = nss->nbits;
	    rsaparams.pe = _pe;

	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, params,
			&nss->pub_key, _isPerm, _isSensitive, _cx);

	    PK11_FreeSlot(_slot);
	}
    }

    rc = (nss->sec_key && nss->pub_key);	/* XXX gud enuf? */

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 1;		/* assume error */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_DSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_DSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

nss->digest = _free(nss->digest);
nss->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    nss->encAlg = getEncAlg(sigp->pubkey_algo, 0);	/* XXX hash_algo? */
    nss->hashAlg = getHashAlg(sigp->hash_algo);
    if (nss->hashAlg == SEC_OID_UNKNOWN)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmnssGenerateDSA(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure */
    pgpDigParams sigp = pgpGetSignature(dig);
unsigned _J = 8;	/* XXX DSA1 1024 bits */
unsigned _L = 0;
unsigned _N = 0;
unsigned _seedBytes = 0;
int _passed = 0;
PK11SlotInfo * _slot = NULL;
PQGParams *pqgParams = NULL;
PQGVerify *pqgVfy = NULL;
int xx;

    /* XXX Set the no. of qbits based on the digest being used. */
    if (nss->qbits == 0)
    switch (sigp->hash_algo) {
    default:	/* XXX default */
    case PGPHASHALGO_SHA1:		nss->qbits = 160;	break;
    case PGPHASHALGO_SHA224:	nss->qbits = 224;	break;
    case PGPHASHALGO_SHA256:	nss->qbits = 256;	break;
    case PGPHASHALGO_SHA384:	nss->qbits = 384;	break;
    case PGPHASHALGO_SHA512:	nss->qbits = 512;	break;
    }
assert(nss->qbits);

    /* XXX Set the no. of nbits for non-truncated digest in use. */
    if (nss->nbits == 0)
    switch (nss->qbits) {
    default:	/* XXX default */
    case 160:	nss->nbits = 1024;	break;
    case 224:	nss->nbits = 2048;	break;
#ifdef	PAINFUL
    case 256:	nss->nbits = 3072;	break;
    case 384:	nss->nbits = 7680;	break;
    case 512:	nss->nbits = 15360;	break;
#else
    case 256:	nss->nbits = 2048;	break;
    case 384:	nss->nbits = 2048;	nss->qbits = 256;	break;
    case 512:	nss->nbits = 2048;	nss->qbits = 256;	break;
#endif
	}
assert(nss->nbits);

/*
 * Generate PQGParams and PQGVerify structs.
 * Length of P specified by L.
 *   if L is greater than 1024 then the resulting verify parameters will be
 *   DSA2.
 * Length of Q specified by N. If zero, The PKCS #11 module will
 *   pick an appropriately sized Q for P. If N is specified and L = 1024, then
 *   the resulting verify parameters will be DSA2, Otherwise DSA1 parameters 
 *   will be returned.
 * Length of SEED in bytes specified in seedBytes.
 *
 * The underlying PKCS #11 module will check the values for L, N, 
 * and seedBytes. The rules for softoken are:
 * 
 * If L <= 1024, then L must be between 512 and 1024 in increments of 64 bits.
 * If L <= 1024, then N must be 0 or 160.
 * If L >= 1024, then L and N must match the following table:
 *   L=1024   N=0 or 160
 *   L=2048   N=0 or 224
 *   L=2048   N=256
 *   L=3072   N=0 or 256
 * if L <= 1024
 *   seedBytes must be in the range [20..256].
 * if L >= 1024
 *   seedBytes must be in the range [20..L/16].
 */
/* seedBytes == L/8 for probable primes, N/8 for Shawe-Taylor Primes */

    switch (nss->nbits) {
    default:		/* XXX sanity */
    case 1024:
	_L = 1024;
	_N = 0;		/* XXX 160 uses DSA2 */
	_seedBytes = 0;
	break;
    case 2048:
	_L = 2048;
	_N = (nss->qbits == 256) ? 256 : 224;	/* 256 or 224 */
	_seedBytes = _N/8;
	break;
    case 3072:
	_L = 3072;
	_N = 256;
	_seedBytes = _N/8;
	break;
    }

    {	CK_MECHANISM_TYPE _type = CKM_DSA_KEY_PAIR_GEN;
	void * _cx = NULL;
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_TRUE;

	_slot = PK11_GetBestSlot(_type, _cx);
	if (_slot) {
	    void * params = NULL;

	    if (_L > 1024 || _N != 0)
		xx = rpmnssErr(nss, "PK11_PQG_ParamGenV2",
			PK11_PQG_ParamGenV2(_L, _N, _seedBytes,
				&pqgParams, &pqgVfy));
	    else if (_seedBytes)
		xx = rpmnssErr(nss, "PK11_PQG_ParamGenSeedLen",
			PK11_PQG_ParamGenSeedLen(_J, _seedBytes,
				&pqgParams, &pqgVfy));
	    else
		xx = rpmnssErr(nss, "PK11_PQG_ParamGen",
			PK11_PQG_ParamGen(_J, &pqgParams, &pqgVfy));
	    if (xx != SECSuccess)
		goto exit;

	    xx = rpmnssErr(nss, "PK11_PQG_VerifyParams",
			PK11_PQG_VerifyParams(pqgParams, pqgVfy, &_passed));
	    if (xx != SECSuccess || _passed != SECSuccess)
		goto exit;

	    params = pqgParams;
	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, params,
			&nss->pub_key, _isPerm, _isSensitive, _cx);

	}
    }

    rc = (nss->sec_key && nss->pub_key);	/* XXX gud enuf? */

exit:
    if (pqgParams) PK11_PQG_DestroyParams(pqgParams);
    if (pqgVfy) PK11_PQG_DestroyVerify(pqgVfy);
    if (_slot) PK11_FreeSlot(_slot);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 1;		/* assume failure */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ELGAMAL);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
nss->digest = _free(nss->digest);
nss->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

    rc = 1;	/* XXX always fail */

SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static
int rpmnssSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 1;		/* assume failure */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
assert(sigp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
dig->pubkey_algoN = pgpPubkeyAlgo2Name(sigp->pubkey_algo);
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

nss->digest = _free(nss->digest);
nss->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    nss->encAlg = getEncAlg(sigp->pubkey_algo, 0);	/* XXX hash_algo? */
    nss->hashAlg = getHashAlg(sigp->hash_algo);
    if (nss->hashAlg == SEC_OID_UNKNOWN)
	goto exit;

    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));

    /* XXX FIXME: avoid spurious "BAD" error msg while signing. */
    if (rc && sigp->signhash16[0] == 0 && sigp->signhash16[1] == 0)
	rc = 0;

exit:
SPEW(0, !rc, dig);	/* XXX don't spew on mismatch. */
    return rc;
}

static int rpmnssLoadParams(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    const char * name;
    SECOidTag curveOid = SEC_OID_UNKNOWN;
    SECOidData * oidData = NULL;
    int rc = 1;		/* assume failure. */
    
    name = nss->curveN;
    if (name == NULL)
	goto exit;
    nss->curveOid = curveOid = curve2oid(name);
    if (curveOid == SEC_OID_UNKNOWN)
	goto exit;
    oidData = SECOID_FindOIDByTag(curveOid);
    if (oidData == NULL)
	goto exit;

    if (nss->ecparams != NULL) {
	SECITEM_FreeItem(nss->ecparams, PR_FALSE);
	nss->ecparams = NULL;
    }
    nss->ecparams = SECITEM_AllocItem(NULL, NULL, (2 + oidData->oid.len));
    nss->ecparams->data[0] = SEC_ASN1_OBJECT_ID;
    nss->ecparams->data[1] = oidData->oid.len;
    memcpy(nss->ecparams->data + 2, oidData->oid.data, oidData->oid.len);
    rc = 0;

exit:
if (_pgp_debug)
fprintf(stderr, "<-- %s(%p,%s) oid %u params %p\n", __FUNCTION__, dig, name, nss->curveOid, nss->ecparams);
    return rc;
}

static
int rpmnssGenerateECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure. */

    /* XXX Set the no. of bits based on the digest being used. */
    if (nss->nbits == 0) {
	if (!strcasecmp(dig->hash_algoN, "SHA224"))
	    nss->nbits = 224;
	else if (!strcasecmp(dig->hash_algoN, "SHA256"))
	    nss->nbits = 256;
	else if (!strcasecmp(dig->hash_algoN, "SHA384"))
	    nss->nbits = 384;
	else if (!strcasecmp(dig->hash_algoN, "SHA512"))
	    nss->nbits = 512;
	else
	    nss->nbits = 256;	/* XXX default */
    }
assert(nss->nbits);

    /* XXX Choose the curve parameters from the no. of digest bits. */
    if (nss->curveN == NULL)	/* XXX FIXME */
    switch (nss->nbits) {	/* XXX only NIST prime curves for now */
    default:	goto exit;	/*@notreached@*/ break;
    case 192:	nss->curveN = xstrdup("nistp192");	break;
    case 224:	nss->curveN = xstrdup("nistp224");	break;
    case 256:	nss->curveN = xstrdup("nistp256");	break;
    case 384:	nss->curveN = xstrdup("nistp384");	break;
    case 512:	/* XXX sanity */
    case 521:	nss->curveN = xstrdup("nistp521");	break;
    }
assert(nss->curveN);

    /* Load the curve parameters. */
    rc = rpmnssLoadParams(dig);
assert(nss->ecparams);

    {	void * _cx = NULL;
	CK_MECHANISM_TYPE _type = CKM_EC_KEY_PAIR_GEN;
	PK11SlotInfo * _slot = PK11_GetBestSlot(_type, _cx);
#ifdef	NOTYET
/* Create an EC key pair in any slot able to do so, 
 * This is a "session" (temporary), not "token" (permanent) key. 
 * Because of the high probability that this key will need to be moved to
 * another token, and the high cost of moving "sensitive" keys, we attempt
 * to create this key pair without the "sensitive" attribute, but revert to 
 * creating a "sensitive" key if necessary.
 */

	PK11AttrFlags _aFlags = (PK11_ATTR_SESSION
				| PK11_ATTR_INSENSITIVE | PK11_ATTR_PUBLIC);
	CK_FLAGS _opFlags = CKF_DERIVE;
	CK_FLAGS _opFlagsMask = CKF_DERIVE | CKF_SIGN;

	if (_slot) {

	    nss->sec_key = PK11_GenerateKeyPairWithOpFlags(_slot, _type,
			nss->ecparams,
			&nss->pub_key, _aFlags, _opFlags, _opFlagsMask, _cx);

	    if (nss->sec_key == NULL) {
		 _aFlags = (PK11_ATTR_SESSION
			| PK11_ATTR_SENSITIVE | PK11_ATTR_PRIVATE);
		nss->sec_key = PK11_GenerateKeyPairWithOpFlags(_slot, _type,
			nss->ecparams,
			&nss->pub_key, _aFlags, _opFlags, _opFlagsMask, _cx);
		
	    }
#else
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_TRUE;

	if (_slot) {

	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, nss->ecparams,
			&nss->pub_key, _isPerm, _isSensitive, _cx);
#endif

	    PK11_FreeSlot(_slot);
	}
    }

    rc = (nss->sec_key && nss->pub_key);	/* XXX gud enuf? */

exit:
SPEW(!rc, rc, dig);

    return rc;
}

static int rpmnssErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	NOTYET
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmnssStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmnssAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
    return rc;
}

static int rpmnssAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
    SECOidTag hashAlgo = getHashAlg(algo);
    rc = (hashAlgo == SEC_OID_UNKNOWN);	/* XXX C, not boolean, return */
    return rc;
}

static int rpmnssAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
    SECOidTag encAlgo = getEncAlg(algo, 0);	/* XXX no hash_algo */
    rc = (encAlgo == SEC_OID_UNKNOWN);	/* XXX C, not boolean, return */
    return rc;
}

static int rpmnssVerify(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

assert(nss->encAlg != SEC_OID_UNKNOWN);
assert(nss->hashAlg != SEC_OID_UNKNOWN);

    nss->item.type = siBuffer;
    nss->item.data = (unsigned char *) nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
	break;
    case PGPPUBKEYALGO_DSA:
assert(nss->encAlg == SEC_OID_ANSIX9_DSA_SIGNATURE);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	goto exit;
	break;
    case PGPPUBKEYALGO_ECDSA:
assert(nss->encAlg == SEC_OID_ANSIX962_EC_PUBLIC_KEY);
	break;
    }

    rc = rpmnssErr(nss, "VFY_VerifyDigestDirect",
		VFY_VerifyDigestDirect(&nss->item, nss->pub_key,
				nss->sig, nss->encAlg, nss->hashAlg, NULL));
    rc = (rc == SECSuccess);

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssSign(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    pgpDigParams pubp = pgpGetPubkey(dig);
    int rc = 0;		/* assume failure */

assert(nss->hashAlg != SEC_OID_UNKNOWN);

    nss->item.type = siBuffer;
    nss->item.data = (unsigned char *) nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

    if (nss->sig != NULL) {
	SECITEM_ZfreeItem(nss->sig, PR_TRUE);
	nss->sig = NULL;
    }
    nss->sig = SECITEM_AllocItem(NULL, NULL, 0);
    nss->sig->type = siBuffer;

    switch (pubp->pubkey_algo) {
    default:
assert(0);
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmnssErr(nss, "SGN_Digest",
		SGN_Digest(nss->sec_key, nss->hashAlg, nss->sig, &nss->item));
	break;
    case PGPPUBKEYALGO_DSA:
    case PGPPUBKEYALGO_ECDSA:
      {	SECItem sig = { siBuffer, NULL, 0 };
	rc = rpmnssErr(nss, "SGN_Digest",
		SGN_Digest(nss->sec_key, nss->hashAlg, &sig, &nss->item));

nss->qbits = 8 * (sig.len/2);
	if (rc == SECSuccess)
	    rc = rpmnssErr(nss, "DSAU_EncodeDerSigWithLen",
		DSAU_EncodeDerSigWithLen(nss->sig, &sig, sig.len));
	sig.data = _free(sig.data);

      }	break;
    case PGPPUBKEYALGO_ELGAMAL:
	goto exit;
	break;
    }
    rc = (rc == SECSuccess);

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssGenerate(pgpDig dig)
{
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

if (nss->sec_key != NULL) {
    SECKEY_DestroyPrivateKey(nss->sec_key);
    nss->sec_key = NULL;
}
if (nss->pub_key != NULL) {
    SECKEY_DestroyPublicKey(nss->pub_key);
    nss->pub_key = NULL;
}

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmnssGenerateRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmnssGenerateDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	rc = rpmnssGenerateELG(dig);
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmnssGenerateECDSA(dig);
	break;
    }
SPEW(!rc, rc, dig);
    return rc;
}

/**
 * Copy OpenPGP unsigned integer padding to lbits.
 * @return		0 on success
 */
static
int rpmnssMpiSet(const char * pre, unsigned int lbits,
		/*@out@*/ void * dest, const rpmuint8_t * p,
		/*@null@*/ const rpmuint8_t * pend)
	/*@modifies *dest @*/
{
    unsigned int mbits = pgpMpiBits(p);
    unsigned int nbits;
    unsigned int nb;
    char * t = (char *) dest;
    unsigned int ix;

    if (pend != NULL && (p + ((mbits+7) >> 3)) > pend)
	return 1;

    if (mbits > lbits)
	return 1;

    nbits = (lbits > mbits ? lbits : mbits);
    nb = ((nbits + 7) >> 3);
    ix = ((nbits - mbits) >> 3);

/*@-modfilesystem @*/
if (_pgp_debug)
fprintf(stderr, "*** mbits %u nbits %u nb %u ix %u\n", mbits, nbits, nb, ix);
    if (ix > 0) memset(t, (int)'\0', ix);
    memcpy(t+ix, p+2, nb-ix);
if (_pgp_debug && _pgp_print)
fprintf(stderr, "\t %s %s\n", pre, pgpHexStr((rpmuint8_t *)dest, nb));
/*@=modfilesystem @*/
    return 0;
}

/**
 * @return		NULL on error
 */
static
/*@only@*/ /*@null@*/
SECItem * rpmnssMpiCopy(PRArenaPool * arena, /*@returned@*/ SECItem * item,
		const rpmuint8_t * p)
	/*@modifies item @*/
{
    const rpmuint8_t * b = p + 2;
    unsigned int nb = pgpMpiLen(p)-2;

/*@-moduncon@*/
    if (item == NULL) {
	if ((item = SECITEM_AllocItem(arena, item, nb)) == NULL)
	    return item;
    } else {
	if (arena != NULL)
	    item->data = (unsigned char *) PORT_ArenaGrow(arena, item->data, item->len, nb);
	else
	    item->data = (unsigned char *) PORT_Realloc(item->data, nb);
 	
	if (item->data == NULL) {
	    if (arena == NULL)
		SECITEM_FreeItem(item, PR_TRUE);
	    return NULL;
	}
    }
/*@=moduncon@*/

    memcpy(item->data, b, nb);
    item->len = nb;
/*@-temptrans@*/
    return item;
/*@=temptrans@*/
}

static /*@null@*/
SECKEYPublicKey * rpmnssNewPublicKey(KeyType type)
	/*@*/
{
    PRArenaPool *arena;
    SECKEYPublicKey *key;

/*@-moduncon@*/
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	return NULL;

    key = (SECKEYPublicKey *) PORT_ArenaZAlloc(arena, sizeof(*key));

    if (key == NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }
/*@=moduncon@*/
    
    key->keyType = type;
    key->pkcs11ID = CK_INVALID_HANDLE;
    key->pkcs11Slot = NULL;
    key->arena = arena;
/*@-nullret@*/	/* key->pkcs11Slot can be NULL */
    return key;
/*@=nullret@*/
}

static
int rpmnssMpiItem(const char * pre, pgpDig dig, int itemno,
		const rpmuint8_t * p, /*@null@*/ const rpmuint8_t * pend)
	/*@*/
{
    rpmnss nss = (rpmnss) dig->impl;
    unsigned int  nb = (pend >= p ? (pend - p) : 0);
    unsigned int mbits = (((8 * (nb - 2)) + 0x1f) & ~0x1f);
    unsigned int nz;
    int rc = 0;

/*@-moduncon@*/
    switch (itemno) {
    default:
assert(0);
	break;
    case 10:		/* RSA m**d */
	nss->nbits = mbits;
assert(nss->sig == NULL);
	nss->sig = SECITEM_AllocItem(NULL, nss->sig, mbits/8);
assert(nss->sig != NULL);
	nz = nss->sig->len - (nb - 2);
	if (nz)		/* XXX resurrect leading zero bytes. */
	    memset(nss->sig->data, 0, nz);
	memcpy(nss->sig->data+nz, p+2, nb-2);
	break;
    case 20:		/* DSA r */
	nss->qbits = mbits;
	nss->item.type = (SECItemType) 0;
	nss->item.len = 2 * (nss->qbits/8);
	nss->item.data = (unsigned char *) xcalloc(1, nss->item.len);
	rc = rpmnssMpiSet(pre, nss->qbits, nss->item.data, p, pend);
	break;
    case 21:		/* DSA s */
assert(mbits == nss->qbits);
	rc = rpmnssMpiSet(pre, nss->qbits, nss->item.data + (nss->qbits/8), p, pend);
assert(nss->sig == NULL);
	if ((nss->sig = SECITEM_AllocItem(NULL, NULL, 0)) == NULL
	 || DSAU_EncodeDerSigWithLen(nss->sig, &nss->item, nss->item.len) != SECSuccess)
	    rc = 1;
	nss->item.data = _free(nss->item.data);
	nss->item.len = 0;
	break;
    case 30:		/* RSA n */
	nss->nbits = mbits;
assert(nss->pub_key == NULL);
	nss->pub_key = rpmnssNewPublicKey(rsaKey);
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.rsa.modulus, p);
	break;
    case 31:		/* RSA e */
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.rsa.publicExponent, p);
	break;
    case 40:		/* DSA p */
	nss->nbits = mbits;
assert(nss->pub_key == NULL);
	nss->pub_key = rpmnssNewPublicKey(dsaKey);
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.dsa.params.prime, p);
	break;
    case 41:		/* DSA q */
	nss->qbits = mbits;
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.dsa.params.subPrime, p);
	break;
    case 42:		/* DSA g */
assert(mbits == nss->nbits);
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.dsa.params.base, p);
	break;
    case 43:		/* DSA y */
assert(mbits == nss->nbits);
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.dsa.publicValue, p);
	break;
    case 50:		/* ECDSA r */
	nss->qbits = mbits;
	nss->item.type = (SECItemType) 0;
	nss->item.len = 2 * (nss->qbits/8);
	nss->item.data = (unsigned char *) xcalloc(1, nss->item.len);
	rc = rpmnssMpiSet(pre, nss->qbits, nss->item.data, p, pend);
	break;
    case 51:		/* ECDSA s */
assert(mbits == nss->qbits);
	rc = rpmnssMpiSet(pre, nss->qbits, nss->item.data + (nss->qbits/8), p, pend);
assert(nss->sig == NULL);
	nss->sig = SECITEM_AllocItem(NULL, NULL, 0);
assert(nss->sig != NULL);
	if (DSAU_EncodeDerSigWithLen(nss->sig, &nss->item, nss->item.len) != SECSuccess)
	    rc = 1;
	nss->item.data = _free(nss->item.data);
	nss->item.len = 0;
	break;
    case 60:		/* ECDSA curve OID */
assert(nss->pub_key == NULL);
	nss->pub_key = rpmnssNewPublicKey(ecKey);
assert(nss->pub_key != NULL);

	{   const char * s = xstrdup(pgpHexStr(p, nb));
	    char * t = NULL;

	   if (!strcasecmp(s, "2a8648ce3d030008"))
		t = " c2onb191v4";
	   else if (!strcasecmp(s, "2a8648ce3d030009"))
		t = "c2onb191v5";
	   else if (!strcasecmp(s, "2a8648ce3d03000e"))
		t = "c2onb239v4";
	   else if (!strcasecmp(s, "2a8648ce3d03000f"))
		t = "c2onb239v5";
	   else if (!strcasecmp(s, "2a8648ce3d030001"))
		t = "c2pnb163v1";
	   else if (!strcasecmp(s, "2a8648ce3d030002"))
		t = "c2pnb163v2";
	   else if (!strcasecmp(s, "2a8648ce3d030003"))
		t = "c2pnb163v3";
	   else if (!strcasecmp(s, "2a8648ce3d030004"))
		t = "c2pnb176v1";
	   else if (!strcasecmp(s, "2a8648ce3d03000a"))
		t = "c2pnb208w1";
	   else if (!strcasecmp(s, "2a8648ce3d030010"))
		t = "c2pnb272w1";
	   else if (!strcasecmp(s, "2a8648ce3d030011"))
		t = "c2pnb304w1";
	   else if (!strcasecmp(s, "2a8648ce3d030013"))
		t = "c2pnb368w1";
	   else if (!strcasecmp(s, "2a8648ce3d030005"))
		t = "c2tnb191v1";
	   else if (!strcasecmp(s, "2a8648ce3d030006"))
		t = "c2tnb191v2";
	   else if (!strcasecmp(s, "2a8648ce3d030007"))
		t = "c2tnb191v3";
	   else if (!strcasecmp(s, "2a8648ce3d03000b"))
		t = "c2tnb239v1";
	   else if (!strcasecmp(s, "2a8648ce3d03000c"))
		t = "c2tnb239v2";
	   else if (!strcasecmp(s, "2a8648ce3d03000d"))
		t = "c2tnb239v3";
	   else if (!strcasecmp(s, "2a8648ce3d030012"))
		t = "c2tnb359v1";
	   else if (!strcasecmp(s, "2a8648ce3d030014"))
		t = "c2tnb431r1";
	   else if (!strcasecmp(s, "2b8104000f"))
		t = "nistb163";
	   else if (!strcasecmp(s, "2b8104001b"))
		t = "nistb233";
	   else if (!strcasecmp(s, "2b81040011"))
		t = "nistb283";
	   else if (!strcasecmp(s, "2b81040025"))
		t = "nistb409";
	   else if (!strcasecmp(s, "2b81040027"))
		t = "nistb571";
	   else if (!strcasecmp(s, "2b81040001"))
		t = "nistk163";
	   else if (!strcasecmp(s, "2b8104001a"))
		t = "nistk233";
	   else if (!strcasecmp(s, "2b81040010"))
		t = "nistk283";
	   else if (!strcasecmp(s, "2b81040024"))
		t = "nistk409";
	   else if (!strcasecmp(s, "2b81040026"))
		t = "nistk571";
	   else if (!strcasecmp(s, "2a8648ce3d030101"))
		t = "nistp192";
	   else if (!strcasecmp(s, "2b81040021"))
		t = "nistp224";
	   else if (!strcasecmp(s, "2a8648ce3d030107"))
		t = "nistp256";
	   else if (!strcasecmp(s, "2b81040022"))
		t = "nistp384";
	   else if (!strcasecmp(s, "2b81040023"))
		t = "nistp521";
	   else if (!strcasecmp(s, "2a8648ce3d030101"))
		t = "prime192v1";
	   else if (!strcasecmp(s, "2a8648ce3d030102"))
		t = "prime192v2";
	   else if (!strcasecmp(s, "2a8648ce3d030103"))
		t = "prime192v3";
	   else if (!strcasecmp(s, "2a8648ce3d030104"))
		t = "prime239v1";
	   else if (!strcasecmp(s, "2a8648ce3d030105"))
		t = "prime239v2";
	   else if (!strcasecmp(s, "2a8648ce3d030106"))
		t = "prime239v3";
	   else if (!strcasecmp(s, "2b81040006"))
		t = "secp112r1";
	   else if (!strcasecmp(s, "2b81040007"))
		t = "secp112r2";
	   else if (!strcasecmp(s, "2b8104001c"))
		t = "secp128r1";
	   else if (!strcasecmp(s, "2b8104001d"))
		t = "secp128r2";
	   else if (!strcasecmp(s, "2b81040009"))
		t = "secp160k1";
	   else if (!strcasecmp(s, "2b81040008"))
		t = "secp160r1";
	   else if (!strcasecmp(s, "2b8104001e"))
		t = "secp160r2";
	   else if (!strcasecmp(s, "2b8104001f"))
		t = "secp192k1";
	   else if (!strcasecmp(s, "2a8648ce3d030101"))
		t = "secp192r1";
	   else if (!strcasecmp(s, "2b81040020"))
		t = "secp224k1";
	   else if (!strcasecmp(s, "2b81040021"))
		t = "secp224r1";
	   else if (!strcasecmp(s, "2b8104000a"))
		t = "secp256k1";
	   else if (!strcasecmp(s, "2a8648ce3d030107"))
		t = "secp256r1";
	   else if (!strcasecmp(s, "2b81040022"))
		t = "secp384r1";
	   else if (!strcasecmp(s, "2b81040023"))
		t = "secp521r1";
	   else if (!strcasecmp(s, "2b81040004"))
		t = "sect113r1";
	   else if (!strcasecmp(s, "2b81040005"))
		t = "sect113r2";
	   else if (!strcasecmp(s, "2b81040016"))
		t = "sect131r1";
	   else if (!strcasecmp(s, "2b81040017"))
		t = "sect131r2";
	   else if (!strcasecmp(s, "2b81040001"))
		t = "sect163k1";
	   else if (!strcasecmp(s, "2b81040002"))
		t = "sect163r1";
	   else if (!strcasecmp(s, "2b8104000f"))
		t = "sect163r2";
	   else if (!strcasecmp(s, "2b81040018"))
		t = "sect193r1";
	   else if (!strcasecmp(s, "2b81040019"))
		t = "sect193r2";
	   else if (!strcasecmp(s, "2b8104001a"))
		t = "sect233k1";
	   else if (!strcasecmp(s, "2b8104001b"))
		t = "sect233r1";
	   else if (!strcasecmp(s, "2b81040003"))
		t = "sect239k1";
	   else if (!strcasecmp(s, "2b81040010"))
		t = "sect283k1";
	   else if (!strcasecmp(s, "2b81040011"))
		t = "sect283r1";
	   else if (!strcasecmp(s, "2b81040024"))
		t = "sect409k1";
	   else if (!strcasecmp(s, "2b81040025"))
		t = "sect409r1";
	   else if (!strcasecmp(s, "2b81040026"))
		t = "sect571k1";
	   else if (!strcasecmp(s, "2b81040027"))
		t = "sect571r1";
	   else
assert(0);
	   nss->curveN = xstrdup(t);
	   s = _free(s);
	}

	{   SECKEYECParams * ecp = &nss->pub_key->u.ec.DEREncodedParams;
	    ecp->data = (unsigned char *) PORT_ArenaZAlloc(nss->pub_key->arena, nb + 2);
	    ecp->data[0] = SEC_ASN1_OBJECT_ID;
	    ecp->data[1] = nb;
	    memcpy(ecp->data + 2, p, nb);
	    ecp->len = nb + 2;
	}
	break;
    case 61:		/* ECDSA Q */
assert(nss->pub_key != NULL);
	(void) rpmnssMpiCopy(nss->pub_key->arena, &nss->pub_key->u.ec.publicValue, p);
	nss->pub_key->u.ec.size = mbits;
	break;
    }
/*@=moduncon@*/
    return rc;
}

/*@-mustmod@*/
static
void rpmnssClean(void * impl)
	/*@modifies impl @*/
{
    rpmnss nss = (rpmnss) impl;
/*@-moduncon@*/
    if (nss != NULL) {
	nss->in_fips_mode = 0;
	nss->nbits = 0;
	nss->qbits = 0;
	nss->badok = 0;
	nss->err = 0;

	nss->digest = _free(nss->digest);
	nss->digestlen = 0;

	if (nss->sec_key != NULL) {
	    SECKEY_DestroyPrivateKey(nss->sec_key);
	    nss->sec_key = NULL;
	}
	if (nss->pub_key != NULL) {
	    SECKEY_DestroyPublicKey(nss->pub_key);
	    nss->pub_key = NULL;
	}
	if (nss->sig != NULL) {
	    SECITEM_ZfreeItem(nss->sig, PR_TRUE);
	    nss->sig = NULL;
	}

	nss->encAlg = SEC_OID_UNKNOWN;
	nss->hashAlg = SEC_OID_UNKNOWN;

	nss->item.type = siBuffer;
	nss->item.data = (unsigned char *) NULL;
	nss->item.len = (unsigned) 0;

	if (nss->ecparams != NULL) {
	    SECITEM_FreeItem(nss->ecparams, PR_FALSE);
	    nss->ecparams = NULL;
	}
	nss->curveN = _free(nss->curveN);
	nss->curveOid = SEC_OID_UNKNOWN;
/*@=moduncon@*/
    }
}
/*@=mustmod@*/

static /*@null@*/
void * rpmnssFree(/*@only@*/ void * impl)
	/*@*/
{
    rpmnssClean(impl);
    impl = _free(impl);
    return NULL;
}

static
void * rpmnssInit(void)
	/*@globals _rpmnss_init @*/
	/*@modifies _rpmnss_init @*/
{
    rpmnss nss = (rpmnss) xcalloc(1, sizeof(*nss));

#ifdef	HAVE_NSS_INITCONTEXT
    if (_rpmnss_context == NULL) {
	const char * _configdir = rpmExpand("%{?_nssdb_path}", NULL);
	const char * _certPrefix = rpmExpand("%{?_nssdb_certprefix}", NULL);
	const char * _keyPrefix = rpmExpand("%{?_nssdb_keyprefix}", NULL);
	const char * _secmodName = rpmExpand("%{?_nssdb_secmodname}", NULL);
	NSSInitParameters _initParams;
	uint32_t _flags = rpmExpandNumeric("%{?_nssdb_flags}");
	int msglvl = RPMLOG_DEBUG;
	unsigned i;

/* <nss3/nss.h>
 * parameters used to initialize softoken. Mostly strings used to 
 * internationalize softoken. Memory for the strings are owned by the caller,
 * who is free to free them once NSS_ContextInit returns. If the string 
 * parameter is NULL (as opposed to empty, zero length), then the softoken
 * default is used. These are equivalent to the parameters for 
 * PK11_ConfigurePKCS11().
 *
 * field names match their equivalent parameter names for softoken strings 
 * documented at https://developer.mozilla.org/en/PKCS11_Module_Specs.
 * 
 * minPWLen 
 *     Minimum password length in bytes. 
 * manufacturerID 
 *     Override the default manufactureID value for the module returned in 
 *     the CK_INFO, CK_SLOT_INFO, and CK_TOKEN_INFO structures with an 
 *     internationalize string (UTF8). This value will be truncated at 32 
 *     bytes (not including the trailing NULL, partial UTF8 characters will be
 *     dropped). 
 * libraryDescription 
 *     Override the default libraryDescription value for the module returned in
 *     the CK_INFO structure with an internationalize string (UTF8). This value
 *     will be truncated at 32 bytes(not including the trailing NULL, partial 
 *     UTF8 characters will be dropped). 
 * cryptoTokenDescription 
 *     Override the default label value for the internal crypto token returned
 *     in the CK_TOKEN_INFO structure with an internationalize string (UTF8).
 *     This value will be truncated at 32 bytes (not including the trailing
 *     NULL, partial UTF8 characters will be dropped). 
 * dbTokenDescription 
 *     Override the default label value for the internal DB token returned in 
 *     the CK_TOKEN_INFO structure with an internationalize string (UTF8). This
 *     value will be truncated at 32 bytes (not including the trailing NULL,
 *     partial UTF8 characters will be dropped). 
 * FIPSTokenDescription 
 *     Override the default label value for the internal FIPS token returned in
 *     the CK_TOKEN_INFO structure with an internationalize string (UTF8). This
 *     value will be truncated at 32 bytes (not including the trailing NULL,
 *     partial UTF8 characters will be dropped). 
 * cryptoSlotDescription 
 *     Override the default slotDescription value for the internal crypto token
 *     returned in the CK_SLOT_INFO structure with an internationalize string
 *     (UTF8). This value will be truncated at 64 bytes (not including the
 *     trailing NULL, partial UTF8 characters will be dropped). 
 * dbSlotDescription 
 *     Override the default slotDescription value for the internal DB token 
 *     returned in the CK_SLOT_INFO structure with an internationalize string 
 *     (UTF8). This value will be truncated at 64 bytes (not including the
 *     trailing NULL, partial UTF8 characters will be dropped). 
 * FIPSSlotDescription 
 *     Override the default slotDecription value for the internal FIPS token
 *     returned in the CK_SLOT_INFO structure with an internationalize string
 *     (UTF8). This value will be truncated at 64 bytes (not including the
 *     trailing NULL, partial UTF8 characters will be dropped). 
 *
 */
	memset((void *) &_initParams, '\0', sizeof(_initParams));
	_initParams.length = sizeof(_initParams);

/* <nss3/nss.h>
 * Open the Cert, Key, and Security Module databases, read/write.
 * Initialize the Random Number Generator.
 * Does not initialize the cipher policies or enables.
 * Default policy settings disallow all ciphers.
 *
 * This allows using application defined prefixes for the cert and key db's
 * and an alternate name for the secmod database. NOTE: In future releases,
 * the database prefixes my not necessarily map to database names.
 *
 * configdir - base directory where all the cert, key, and module datbases live.
 * certPrefix - prefix added to the beginning of the cert database example: "
 * 			"https-server1-"
 * keyPrefix - prefix added to the beginning of the key database example: "
 * 			"https-server1-"
 * secmodName - name of the security module database (usually "secmod.db").
 * flags - change the open options of NSS_Initialize as follows:
 * 	NSS_INIT_READONLY - Open the databases read only.
 * 	NSS_INIT_NOCERTDB - Don't open the cert DB and key DB's, just 
 * 			initialize the volatile certdb.
 * 	NSS_INIT_NOMODDB  - Don't open the security module DB, just 
 *			initialize the 	PKCS #11 module.
 *      NSS_INIT_FORCEOPEN - Continue to force initializations even if the 
 * 			databases cannot be opened.
 *      NSS_INIT_NOROOTINIT - Don't try to look for the root certs module
 *			automatically.
 *      NSS_INIT_OPTIMIZESPACE - Use smaller tables and caches.
 *      NSS_INIT_PK11THREADSAFE - only load PKCS#11 modules that are
 *                      thread-safe, ie. that support locking - either OS
 *                      locking or NSS-provided locks . If a PKCS#11
 *                      module isn't thread-safe, don't serialize its
 *                      calls; just don't load it instead. This is necessary
 *                      if another piece of code is using the same PKCS#11
 *                      modules that NSS is accessing without going through
 *                      NSS, for example the Java SunPKCS11 provider.
 *      NSS_INIT_PK11RELOAD - ignore the CKR_CRYPTOKI_ALREADY_INITIALIZED
 *                      error when loading PKCS#11 modules. This is necessary
 *                      if another piece of code is using the same PKCS#11
 *                      modules that NSS is accessing without going through
 *                      NSS, for example Java SunPKCS11 provider.
 *      NSS_INIT_NOPK11FINALIZE - never call C_Finalize on any
 *                      PKCS#11 module. This may be necessary in order to
 *                      ensure continuous operation and proper shutdown
 *                      sequence if another piece of code is using the same
 *                      PKCS#11 modules that NSS is accessing without going
 *                      through NSS, for example Java SunPKCS11 provider.
 *                      The following limitation applies when this is set :
 *                      SECMOD_WaitForAnyTokenEvent will not use
 *                      C_WaitForSlotEvent, in order to prevent the need for
 *                      C_Finalize. This call will be emulated instead.
 *      NSS_INIT_RESERVED - Currently has no effect, but may be used in the
 *                      future to trigger better cooperation between PKCS#11
 *                      modules used by both NSS and the Java SunPKCS11
 *                      provider. This should occur after a new flag is defined
 *                      for C_Initialize by the PKCS#11 working group.
 *      NSS_INIT_COOPERATE - Sets 4 recommended options for applications that
 *                      use both NSS and the Java SunPKCS11 provider.
 *
 * Also NOTE: This is not the recommended method for initializing NSS. 
 * The preferred method is NSS_init().
 */
	_flags |= NSS_INIT_READONLY;
	if (_configdir == NULL || *_configdir != '/') {
	    _configdir = _free(_configdir);
	    _flags |= NSS_INIT_NOCERTDB;
	    _flags |= NSS_INIT_NOMODDB;
	    _flags |= NSS_INIT_FORCEOPEN;
	    _flags |= NSS_INIT_NOROOTINIT;
	    _flags |= NSS_INIT_OPTIMIZESPACE;
	}
	/* NSS_INIT_PK11THREADSAFE */
	/* NSS_INIT_PK11RELOAD */
	/* NSS_INIT_NOPK11FINALIZE */
	/* NSS_INIT_RESERVED */
	/* NSS_INIT_COOPERATE  (is all of the above) */

	rpmlog(msglvl, "---------- NSS %s configuration:\n", NSS_VERSION);
	rpmlog(msglvl, "   version: %s\n", NSS_GetVersion());
	rpmlog(msglvl, " configdir: %s\n", _configdir);
	rpmlog(msglvl, "certPrefix: %s\n", _certPrefix);
	rpmlog(msglvl, " keyPrefix: %s\n", _keyPrefix);
	rpmlog(msglvl, "secmodName: %s\n", _secmodName);
	rpmlog(msglvl, "     flags: 0x%x\n", _flags);

	/* List the ECC curves. */
	for (i = 0; i < nrpmnssOIDS; i++) {
	    const char * N = rpmnssOIDS[i].N;
	    uint32_t V = rpmnssOIDS[i].V;
	    SECOidData * o = SECOID_FindOIDByTag(V);

	    if (i == 0)
		rpmlog(msglvl, " EC curves:\n");
	    if (o == NULL)
		continue;
	    rpmlog(msglvl,      "   %s\n", o->desc);
	    rpmlog(msglvl,      "   %12s%5d %s\n",
		N, V, pgpHexStr(o->oid.data, o->oid.len));
	}
	rpmlog(msglvl, "----------\n");

	_rpmnss_context = (void *) NSS_InitContext(_configdir,
		_certPrefix, _keyPrefix, _secmodName, &_initParams, _flags);

	_configdir = _free(_configdir);
	_certPrefix = _free(_certPrefix);
	_keyPrefix = _free(_keyPrefix);
	_secmodName = _free(_secmodName);
assert(_rpmnss_context != NULL);
#ifdef	NOTYET
	NSS_ShutdownFunc _sFunc = foo;
	void * _appData = bar;
	SECStatus rv;
	rv = NSS_RegisterShutdown(_sFunc, _appData);
	rv = NSS_UnregisterShutdown(_sFunc, _appData);
#endif
	
    }
#else
    if (!NSS_IsInitialized()) {
	const char * _configdir = rpmExpand("%{?_nssdb_path}", NULL);
	SECStatus rv;
	if (_configdir != NULL && *_configdir == '/')
	    rv = NSS_Init(_configdir);
	else
	    rv NSS_NoDB_Init(NULL);
	_configdir = _free(_configdir);
assert(rv == SECSuccess);
    }
#endif

    _rpmnss_init = 1;

    return (void *) nss;
}

struct pgpImplVecs_s rpmnssImplVecs = {
	"NSS " NSS_VERSION,
	rpmnssSetRSA,
	rpmnssSetDSA,
	rpmnssSetELG,
	rpmnssSetECDSA,

	rpmnssErrChk,
	rpmnssAvailableCipher, rpmnssAvailableDigest, rpmnssAvailablePubkey,
	rpmnssVerify, rpmnssSign, rpmnssGenerate,

	rpmnssMpiItem, rpmnssClean,
	rpmnssFree, rpmnssInit
};

int rpmnssExportPubkey(pgpDig dig)
{
    uint8_t pkt[8192];
    uint8_t * be = pkt;
    size_t pktlen;
    time_t now = time(NULL);
    uint32_t bt = now;
    uint16_t bn;
    pgpDigParams pubp = pgpGetPubkey(dig);
    rpmnss nss = (rpmnss) dig->impl;
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
	/* RSA n */
	bn = 8 * nss->pub_key->u.rsa.modulus.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.rsa.modulus.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.rsa.modulus.data, bn/8);
	be += bn/8;

	/* RSA e */
	bn = 8 * nss->pub_key->u.rsa.publicExponent.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.rsa.publicExponent.data,4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.rsa.publicExponent.data, bn/8);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_DSA:
	/* DSA p */
	bn = 8 * nss->pub_key->u.dsa.params.prime.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.dsa.params.prime.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.dsa.params.prime.data, bn/8);
	be += bn/8;

	/* DSA q */
	bn = 8 * nss->pub_key->u.dsa.params.subPrime.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.dsa.params.subPrime.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.dsa.params.subPrime.data, bn/8);
	be += bn/8;

	/* DSA g */
	bn = 8 * nss->pub_key->u.dsa.params.base.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.dsa.params.base.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.dsa.params.base.data, bn/8);
	be += bn/8;

	bn = 8 * nss->pub_key->u.dsa.publicValue.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.dsa.publicValue.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.dsa.publicValue.data, bn/8);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_ECDSA:
	/* ECDSA oid */
	{   SECKEYECParams * ecp = &nss->pub_key->u.ec.DEREncodedParams;
	    *be++ = ecp->len - 2;
	    memcpy(be, ecp->data+2, ecp->len-2);
	    be += ecp->len - 2;
	}

	/* ECDSA Q */
	bn = 8 * nss->pub_key->u.ec.publicValue.len;
	bn -= __builtin_clz(pgpGrab(nss->pub_key->u.ec.publicValue.data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7; bn &= ~7;
	memcpy(be, nss->pub_key->u.ec.publicValue.data, bn/8);
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
    return 0;
}

int rpmnssExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
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
    rpmnss nss = (rpmnss) dig->impl;
    int rc = 0;		/* assume failure */
    int xx;

    sigp->tag = PGPTAG_SIGNATURE;
    *be++ = 0x80 | (sigp->tag << 2) | 0x01;
    be += 2;				/* pktlen */

    sigp->hash = be;
    *be++ = sigp->version = 0x04;		/* version */
    *be++ = sigp->sigtype = PGPSIGTYPE_BINARY;	/* sigtype */
assert(sigp->pubkey_algo == pubp->pubkey_algo);
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
	xx = pgpImplSetRSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
	break;
    case PGPPUBKEYALGO_DSA:
	xx = pgpImplSetDSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
	break;
    case PGPPUBKEYALGO_ECDSA:
	xx = pgpImplSetECDSA(ctx, dig, sigp);	/* XXX signhash16 check fails */
	break;
    }
    h = (uint8_t *) nss->digest;
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
	bn = 8 * nss->sig->len - __builtin_clz(pgpGrab(nss->sig->data, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	memcpy(be, nss->sig->data, bn/8);
	be += bn/8;
	break;
    case PGPPUBKEYALGO_DSA:
      { unsigned int nb = nss->qbits/8;	/* XXX FIXME */
	SECItem * sig = DSAU_DecodeDerSigToLen(nss->sig, 2 * nb);
assert(sig != NULL);
	bn = 8 * (sig->len/2) - __builtin_clz(pgpGrab(sig->data           , 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	memcpy(be, sig->data, bn/8);
	be += bn/8;

	bn = 8 * (sig->len/2) - __builtin_clz(pgpGrab(sig->data+sig->len/2, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	memcpy(be, sig->data + (bn/8), bn/8);
	be += bn/8;
	SECITEM_ZfreeItem(sig, PR_TRUE);
      }	break;
    case PGPPUBKEYALGO_ECDSA:
      { unsigned int nb = nss->qbits/8;	/* XXX FIXME */
	SECItem * sig = DSAU_DecodeDerSigToLen(nss->sig, 2 * nb);
assert(sig != NULL);
	bn = 8 * (sig->len/2) - __builtin_clz(pgpGrab(sig->data           , 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	memcpy(be, sig->data, bn/8);
	be += bn/8;

	bn = 8 * (sig->len/2) - __builtin_clz(pgpGrab(sig->data+sig->len/2, 4));
	*be++ = (bn >> 8);	*be++ = (bn     );
	bn += 7;	bn &= ~7;
	memcpy(be, sig->data + (bn/8), bn/8);
	be += bn/8;
	SECITEM_ZfreeItem(sig, PR_TRUE);
      }	break;
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

#endif	/* WITH_NSS */
