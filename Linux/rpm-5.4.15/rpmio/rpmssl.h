#ifndef	H_RPMSSL
#define	H_RPMSSL

/** \ingroup rpmpgp
 * \file rpmio/rpmssl.h
 */

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

/* Implementation specific includes. */
#if defined(_RPMSSL_INTERNAL)

#include <openssl/evp.h>
#if defined(OPENSSL_NO_EC) && !defined(OPENSSL_NO_ECDSA)
#define	OPENSSL_NO_ECDSA
#endif

#include <openssl/bn.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>

#include <openssl/conf.h>
#include <openssl/comp.h>
#endif

/**
 */
typedef	/*abstract@*/ struct rpmssl_s * rpmssl;

/**
 * Implementation specific parameter storage.
 */
#if defined(_RPMSSL_INTERNAL)
struct rpmssl_s {
    int in_fips_mode;	/* XXX trsa */
    unsigned int nbits;
    unsigned int qbits;
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

    BIO * out;			/* XXX tecdsa */
#if !defined(OPENSSL_NO_ECDSA)
    EC_builtin_curve * curves;	/* XXX tecdsa */
    size_t ncurves;
    EC_GROUP * group;		/* XXX tecdsa */
    EC_KEY * ec_bad;		/* XXX tecdsa */
#endif

    unsigned char * sig;
    size_t siglen;

    /* DSA parameters. */
    DSA * dsa;
    DSA_SIG * dsasig;

    /* RSA parameters. */
    RSA * rsa;
    BIGNUM * hm;

    /* ECDSA parameters. */
#if !defined(OPENSSL_NO_ECDSA)
    const char * curveN;
    int nid;
    ECDSA_SIG * ecdsasig;
    BIGNUM * priv;
#endif

    EVP_PKEY * pkey;
    const EVP_MD * md;

};
#endif

/*@unchecked@*/
extern pgpImplVecs_t rpmsslImplVecs;

int rpmsslExportPubkey(pgpDig dig)
	/*@*/;
int rpmsslExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

#endif	/* H_RPMSSL */
