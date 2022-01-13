/** \ingroup rpmio signature
 * \file rpmio/tkey.c
 * Routines to handle RFC-2440 detached signatures.
 */

static int _debug = 0;
extern int _pgp_debug;
extern int _pgp_print;

#include "system.h"
#include <rpmio.h>

#define	_RPMPGP_INTERNAL
#define	_RPMBC_INTERNAL
#include <rpmbc.h>
#define	_RPMGC_INTERNAL
#include <rpmgc.h>
#define	_RPMNSS_INTERNAL
#include <rpmnss.h>
#define	_RPMSSL_INTERNAL
#include <rpmssl.h>

#include "gengpg.h"

#include "debug.h"

static int doit(const char *sig, pgpDig dig, int printing)
{
    const char *s, *t;
    unsigned char * dec;
    size_t declen;
    char * enc;
    int rc;
    int i;

if (_debug)
fprintf(stderr, "*** before\n%s\n", sig);

    if ((rc = b64decode(sig, (void **)&dec, &declen)) != 0) {
	fprintf(stderr, "*** b64decode returns %d\n", rc);
	return rc;
    }
    rc = pgpPrtPkts(dec, declen, dig, printing);
    if (rc < 0) {
	fprintf(stderr, "*** pgpPrtPkts returns %d\n", rc);
	return rc;
    }

    if ((enc = b64encode(dec, declen)) == NULL) {
	fprintf(stderr, "*** b64encode failed\n");
	return rc;
    }
    dec = _free(dec);

if (_debug)
fprintf(stderr, "***  after\n%s\n", enc);

rc = 0;
for (i = 0, s = sig, t = enc; *s & *t; i++, s++, t++) {
    if (*s == '\n') s++;
    if (*t == '\n') t++;
    if (*s == *t) continue;
if (_debug)
fprintf(stderr, "??? %5d %02x != %02x '%c' != '%c'\n", i, (*s & 0xff), (*t & 0xff), *s, *t);
    rc = 5;
}
    enc = _free(enc);

    return rc;
}

/* FIPS-186 test vectors. */
static const char * fips_p = "8df2a494492276aa3d25759bb06869cbeac0d83afb8d0cf7cbb8324f0d7882e5d0762fc5b7210eafc2e9adac32ab7aac49693dfbf83724c2ec0736ee31c80291";
static const char * fips_q = "c773218c737ec8ee993b4f2ded30f48edace915f";
static const char * fips_g = "626d027839ea0a13413163a55b4cb500299d5522956cefcb3bff10f399ce2c2e71cb9de5fa24babf58e5b79521925c9cc42e9f6f464b088cc572af53e6d78802";

static const char * fips_hm = "a9993e364706816aba3e25717850c26c9cd0d89d";

static const char * fips_y = "19131871d75b1612a819f29d78d1b0d7346f7aa77bb62a859bfd6c5675da9d212d3a36ef1672ef660b8c7c255cc0ec74858fba33f44c06699630a76b030ee333";

static const char * fips_r = "8bac1ab66410435cb7181f95b16ab97c92b341c0";
static const char * fips_s = "41e2345f1f56df2458f426d155b4ba2db6dcd8c8";

int
main(int argc, char *argv[])
{
    pgpImplVecs_t * testImplVecs = &rpmnssImplVecs;
    pgpDig dig;
pgpDigParams pubp;
    rpmbc bc;
    int printing = -1;
    int rc;

    pgpImplVecs = &rpmbcImplVecs;

    dig = pgpDigNew(RPMVSF_DEFAULT, 0);
pubp = pgpGetPubkey(dig);
    bc = dig->impl;

    mpbzero(&bc->p);	mpbsethex(&bc->p, fips_p);
    mpbzero(&bc->q);	mpbsethex(&bc->q, fips_q);
    mpnzero(&bc->g);	mpnsethex(&bc->g, fips_g);
    mpnzero(&bc->y);	mpnsethex(&bc->y, fips_y);
    mpnzero(&bc->r);	mpnsethex(&bc->r, fips_r);
    mpnzero(&bc->s);	mpnsethex(&bc->s, fips_s);
    mpnzero(&bc->hm);	mpnsethex(&bc->hm, fips_hm);

pubp->pubkey_algo = PGPPUBKEYALGO_DSA;	/* XXX assert? */
    rc = pgpImplVerify(dig);

fprintf(stderr, "=============================== DSA FIPS-186-1: rc %d\n", rc);

    dig = pgpDigFree(dig);

    pgpImplVecs = testImplVecs;

    dig = pgpDigNew(RPMVSF_DEFAULT, 0);
pubp = pgpGetPubkey(dig);
_pgp_debug = 1;
_pgp_print = 1;

fprintf(stderr, "=============================== DSA Public Key\n");
    if ((rc = doit(DSApub, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

fprintf(stderr, "=============================== DSA Signature of \"%s\"\n", str);
    if ((rc = doit(DSAsig, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

    {	DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	pgpDigParams dsig = pgpGetSignature(dig);
	
	rpmDigestUpdate(ctx, str, strlen(str));
	rpmDigestUpdate(ctx, dsig->hash, dsig->hashlen);

	(void) pgpImplSetDSA(ctx, dig, dsig);
    }

pubp->pubkey_algo = PGPPUBKEYALGO_DSA;	/* XXX assert? */
    rc = pgpImplVerify(dig);
    
fprintf(stderr, "=============================== DSA verify: rc %d\n", rc);

    dig = pgpDigFree(dig);

    pgpImplVecs = testImplVecs;

    dig = pgpDigNew(RPMVSF_DEFAULT, 0);
pubp = pgpGetPubkey(dig);
_pgp_debug = 1;
_pgp_print = 1;

fprintf(stderr, "=============================== RSA Public Key\n");
    if ((rc = doit(RSApub, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

fprintf(stderr, "=============================== RSA Signature of \"%s\"\n", str);
    if ((rc = doit(RSAsig, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

    {	DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	pgpDigParams dsig = pgpGetSignature(dig);
	
	rpmDigestUpdate(ctx, str, strlen(str));
	rpmDigestUpdate(ctx, dsig->hash, dsig->hashlen);

	(void) pgpImplSetRSA(ctx, dig, dsig);
    }

pubp->pubkey_algo = PGPPUBKEYALGO_RSA;	/* XXX assert? */
    rc = pgpImplVerify(dig);
    
fprintf(stderr, "=============================== RSA verify: rc %d\n", rc);

    dig = pgpDigFree(dig);

    pgpImplVecs = testImplVecs;

    dig = pgpDigNew(RPMVSF_DEFAULT, 0);
pubp = pgpGetPubkey(dig);
_pgp_debug = 1;
_pgp_print = 1;

fprintf(stderr, "=============================== ECDSA Public Key\n");
    if ((rc = doit(ECDSApub, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

fprintf(stderr, "=============================== ECDSA Signature of \"%s\"\n", str);
    if ((rc = doit(ECDSAsig, dig, printing)) != 0)
	fprintf(stderr, "==> FAILED: rc %d\n", rc);

    {	DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA256, RPMDIGEST_NONE);
	pgpDigParams dsig = pgpGetSignature(dig);
	
	rpmDigestUpdate(ctx, str, strlen(str));
	rpmDigestUpdate(ctx, dsig->hash, dsig->hashlen);

	(void) pgpImplSetECDSA(ctx, dig, dsig);
    }

pubp->pubkey_algo = PGPPUBKEYALGO_ECDSA;	/* XXX assert? */
    rc = pgpImplVerify(dig);
    
fprintf(stderr, "=============================== ECDSA verify: rc %d\n", rc);

    dig = pgpDigFree(dig);

    if (pgpImplVecs == &rpmsslImplVecs)
	NSS_Shutdown();

    return rc;
}
