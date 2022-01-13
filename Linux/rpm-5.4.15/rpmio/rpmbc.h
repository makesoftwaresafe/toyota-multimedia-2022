#ifndef	H_RPMBC
#define	H_RPMBC

/** \ingroup rpmpgp
 * \file rpmio/rpmbc.h
 */

#include <beecrypt/api.h>

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

/* Drag in the beecrypt includes. */
#include <beecrypt/beecrypt.h>
#include <beecrypt/base64.h>
#include <beecrypt/dsa.h>
#include <beecrypt/endianness.h>
#include <beecrypt/md4.h>
#include <beecrypt/md5.h>
#include <beecrypt/mp.h>
#include <beecrypt/rsa.h>
#include <beecrypt/rsapk.h>
#include <beecrypt/elgamal.h>
#include <beecrypt/ripemd128.h>
#include <beecrypt/ripemd160.h>
#include <beecrypt/ripemd256.h>
#include <beecrypt/ripemd320.h>
#include <beecrypt/sha1.h>
#include <beecrypt/sha224.h>
#include <beecrypt/sha256.h>
#include <beecrypt/sha384.h>
#include <beecrypt/sha512.h>

/**
 */
typedef	/*abstract@*/ struct rpmbc_s * rpmbc;

/**
 */
#if defined(_RPMBC_INTERNAL)
struct rpmbc_s {
    int in_fips_mode;	/* XXX trsa */
    unsigned int nbits;
    unsigned int qbits;
    int badok;		/* XXX trsa */
    int err;

    void * digest;
    size_t digestlen;

    randomGeneratorContext rngc;

    rsakp rsa_keypair;

    dsakp dsa_keypair;

    dlkp_p elg_keypair;
#ifdef	DYING
dldp_p elg_params;
#endif

    /* DSA parameters. */
    mpnumber r;
    mpnumber s;
    mpnumber hm;

    /* RSA parameters. */
    mpnumber c;
    mpnumber md;

    /* ECDSA parameters (unimplemented). */
    mpnumber Q;
};
#endif

/*@unchecked@*/
extern pgpImplVecs_t rpmbcImplVecs;

int rpmbcExportPubkey(pgpDig dig)
	/*@*/;
int rpmbcExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

#endif	/* H_RPMBC */
