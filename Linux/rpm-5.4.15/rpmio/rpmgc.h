#ifndef	H_RPMGC
#define	H_RPMGC

/** \ingroup rpmpgp
 * \file rpmio/rpmgc.h
 */

#include <rpmiotypes.h>
#include <rpmpgp.h>
#include <rpmsw.h>

/* Implementation specific includes. */
#if defined(_RPMGC_INTERNAL)
/* XXX http://stackoverflow.com/questions/10556299/compiler-warnings-with-libgcrypt-v1-5-0 */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gcrypt.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#endif

/**
 */
typedef	/*abstract@*/ struct rpmgc_s * rpmgc;

/**
 * Implementation specific parameter storage.
 */
#if defined(_RPMGC_INTERNAL)
struct rpmgc_s {
    int in_fips_mode;	/* XXX trsa */
    unsigned int nbits;
    unsigned int qbits;
    gcry_error_t badok;	/* XXX trsa */
    gcry_error_t err;

    void * digest;
    size_t digestlen;

    gcry_sexp_t key_spec;	/* XXX private to Generate? */
    gcry_sexp_t key_pair;	/* XXX private to Generate? */

    gcry_sexp_t pub_key;
    gcry_sexp_t sec_key;
    gcry_sexp_t hash;
    gcry_sexp_t sig;

    /* DSA/ELG parameters. */
    gcry_mpi_t p;	/* ECDSA too */
    gcry_mpi_t q;	/* ECDSA too */
    gcry_mpi_t g;	/* ECDSA too */
    gcry_mpi_t y;

    gcry_mpi_t r;	/* ECDSA too */
    gcry_mpi_t s;	/* ECDSA too */

    /* RSA parameters. */
    gcry_mpi_t n;	/* ECDSA too */
    gcry_mpi_t e;
    gcry_mpi_t c;

    /* ECDSA parameters. */
    gcry_mpi_t a;	/* unused */
    gcry_mpi_t b;	/* unused */
/*@only@*/
    const char * oid;	/* curve oid string like "1.2.840.10045.3.1.7" */
/*@only@*/
    const char * curve;	/* curve string like "NIST P-256" */

};
#endif

/*@unchecked@*/
extern pgpImplVecs_t rpmgcImplVecs;

int rpmgcExportPubkey(pgpDig dig)
	/*@*/;
int rpmgcExportSignature(pgpDig dig, /*@only@*/ DIGEST_CTX ctx)
	/*@*/;

#endif	/* H_RPMGC */
