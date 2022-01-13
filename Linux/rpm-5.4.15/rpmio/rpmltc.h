#ifndef	H_RPMLTC
#define	H_RPMLTC

/** \ingroup rpmpgp
 * \file rpmio/rpmltc.h
 */

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

/* Implementation specific includes. */
#if defined(_RPMLTC_INTERNAL)
#include <tomcrypt.h>
#include <tommath.h>
#endif

/**
 */
typedef	/*abstract@*/ struct rpmltc_s * rpmltc;

/**
 * Implementation specific parameter storage.
 */
#if defined(_RPMLTC_INTERNAL)
struct rpmltc_s {
    int in_fips_mode;	/* XXX trsa */
    unsigned int nbits;
    unsigned int qbits;
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

    int hashIdx;
    int prngIdx;
    int cipherIdx;

    /* DSA parameters. */
    dsa_key dsa;
    mp_int * r;
    mp_int * s;

    /* RSA parameters. */
    rsa_key rsa;
    mp_int * c;

    /* ECDSA parameters. */
    ecc_key ecdsa;
    const char * curveN;

};
#endif

/*@unchecked@*/
extern pgpImplVecs_t rpmltcImplVecs;

int rpmltcExportPubkey(pgpDig dig)
	/*@*/;
int rpmltcExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

#endif	/* H_RPMLTC */
