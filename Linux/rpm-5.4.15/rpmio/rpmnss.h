#ifndef	H_RPMNSS
#define	H_RPMNSS

/** \ingroup rpmpgp
 * \file rpmio/rpmnss.h
 */

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

#if defined(_RPMNSS_INTERNAL)
#if defined(__LCLINT__)
#define	__i386__
#endif
#include <nss.h>
#include <sechash.h>
#include <keyhi.h>
#include <cryptohi.h>
#include <pk11pub.h>
#include <pk11pqg.h>
#include <secerr.h>
#endif

/** \ingroup rpmpgp
 */
typedef	/*abstract@*/ struct rpmnss_s * rpmnss;

/** \ingroup rpmpgp
 */
#if defined(_RPMNSS_INTERNAL)
struct rpmnss_s {
    int in_fips_mode;	/* XXX trsa */
    unsigned int nbits;
    unsigned int qbits;
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

	/* key_spec */
	/* key_pair */
    SECKEYPrivateKey * sec_key;
    SECKEYPublicKey * pub_key;
	/* hash */
    SECItem * sig;

    SECOidTag encAlg;
    SECOidTag hashAlg;
    SECItem item;

    /* RSA parameters. */

    /* DSA parameters. */

    /* ELG parameters. */

    /* ECDSA parameters. */
SECKEYECParams * ecparams;
const char * curveN;
SECOidTag curveOid;

};
#endif

/** \ingroup rpmpgp
 */
/*@unchecked@*/
extern pgpImplVecs_t rpmnssImplVecs;

int rpmnssExportPubkey(pgpDig dig)
	/*@*/;
int rpmnssExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

#endif	/* H_RPMNSS */
