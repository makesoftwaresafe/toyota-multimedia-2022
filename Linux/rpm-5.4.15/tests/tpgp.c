/**
 * \file lib/tpgp.c
 * Test RFC-4880 clearsigned and detached signatures.
 */

extern int _pgp_debug;
extern int _pgp_print;

#include "system.h"

#define	_RPMPGP_INTERNAL
#include <rpmio.h>
#include <rpmbc.h>
#include <rpmcdsa.h>
#include <rpmgc.h>
#include <rpmltc.h>
#include <rpmnss.h>
#include <rpmssl.h>

#include <rpmns.h>
#include <rpmcli.h>

#include "genpgp.h"

#include "debug.h"

static
rpmRC probeTest(rpmts ts, const char * sigtype)
{
    rpmRC rc = RPMRC_FAIL;

    if (!strcmp("DSA", sigtype)) {
	rc = rpmnsProbeSignature(ts, DSApem, NULL, DSApub, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, DSApub, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, DSApubpem, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, DSApub, DSApubid, 0);

	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, DSApubpem, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, NULL, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, NULL, DSApubid, 0);
    }

    if (!strcmp("RSA", sigtype)) {
	rc = rpmnsProbeSignature(ts, RSApem, NULL, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsig, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApubpem, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApubpem, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsig, NULL, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, NULL, RSApubid, 0);
    }
    
    return rc;
}

static
rpmRC generateTest(rpmts ts, const char * text, int pubkey_algo, int hash_algo)
{
    pgpDig dig = pgpDigNew(RPMVSF_DEFAULT, 0);
    pgpDigParams pubp = pgpGetPubkey(dig);
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmRC rc = RPMRC_OK;		/* assume success */
    int xx;

    pubp->pubkey_algo = pubkey_algo;
dig->pubkey_algoN = pgpPubkeyAlgo2Name(pubp->pubkey_algo);
    sigp->pubkey_algo = pubkey_algo;
    sigp->hash_algo = hash_algo;
dig->hash_algoN = pgpHashAlgo2Name(sigp->hash_algo);

    xx = pgpImplGenerate(dig);
    if (xx) {
	DIGEST_CTX ctx = rpmDigestInit(sigp->hash_algo, 0);
	uint8_t * digest = NULL;
	size_t digestlen = 0;
	xx = rpmDigestUpdate(ctx, text, strlen(text));
	xx = rpmDigestFinal(rpmDigestDup(ctx), &digest, &digestlen, 0);
	switch (pubp->pubkey_algo) {
	case PGPPUBKEYALGO_RSA:
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetRSA(rpmDigestDup(ctx), dig, sigp);
	    xx = pgpImplSign(dig);
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetRSA(ctx, dig, sigp);
	    xx = pgpImplVerify(dig);
	    break;
	case PGPPUBKEYALGO_DSA:
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetDSA(rpmDigestDup(ctx), dig, sigp);
	    xx = pgpImplSign(dig);
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetDSA(ctx, dig, sigp);
	    xx = pgpImplVerify(dig);
	    break;
	case PGPPUBKEYALGO_ELGAMAL:
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetELG(rpmDigestDup(ctx), dig, sigp);
	    xx = pgpImplSign(dig);
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetELG(ctx, dig, sigp);
	    xx = pgpImplVerify(dig);
	    break;
	case PGPPUBKEYALGO_ECDSA:
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetECDSA(rpmDigestDup(ctx), dig, sigp);
	    xx = pgpImplSign(dig);
	    sigp->signhash16[0] = digest[0];
	    sigp->signhash16[1] = digest[1];
	    xx = pgpImplSetECDSA(ctx, dig, sigp);
	    xx = pgpImplVerify(dig);
	    break;
	default:
	    xx = rpmDigestFinal(ctx, NULL, NULL, 0);
	    break;
	}
	digest = _free(digest);
    }

    dig = pgpDigFree(dig);
    return rc;
}

static struct poptOption optionsTable[] = {
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
        N_("Common options:"),
        NULL },

   POPT_AUTOALIAS
   POPT_AUTOHELP
   POPT_TABLEEND
};      

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    rpmts ts = rpmtsCreate();
    int ec = rpmtsOpenDB(ts, O_RDONLY);
    int rc;

fprintf(stderr, " DSA");
    rc = probeTest(ts, "DSA");
    if (rc != RPMRC_OK) ec++;
    rc = generateTest(ts, "abc", PGPPUBKEYALGO_DSA, PGPHASHALGO_SHA1);
    if (rc != RPMRC_OK) ec++;
fprintf(stderr, " RSA");
    rc = probeTest(ts, "RSA");
    if (rc != RPMRC_OK) ec++;
    rc = generateTest(ts, "abc", PGPPUBKEYALGO_RSA, PGPHASHALGO_SHA1);
    if (rc != RPMRC_OK) ec++;

#ifdef	NOTYET	/* XXX FIXME: pig slow w/o gcrypt transient-key support. */
    if (pgpImplVecs == &rpmgcImplVecs) {
fprintf(stderr, " ELG");
	rc = generateTest(ts, "abc", PGPPUBKEYALGO_ELGAMAL, PGPHASHALGO_SHA1);
	if (rc != RPMRC_OK) ec++;
    }
#endif

    if (0
#if defined(WITH_GCRYPT)
     || pgpImplVecs == &rpmgcImplVecs
#endif
#if defined(WITH_TOMCRYPT)
     || pgpImplVecs == &rpmltcImplVecs
#endif
#if defined(WITH_NSS)
     || pgpImplVecs == &rpmnssImplVecs
#endif
#if defined(WITH_SSL)
     || pgpImplVecs == &rpmsslImplVecs
#endif
    ) {
fprintf(stderr, " ECDSA");
	rc = generateTest(ts, "abc", PGPPUBKEYALGO_ECDSA, PGPHASHALGO_SHA256);
	if (rc != RPMRC_OK) ec++;
    }
fprintf(stderr, "\n");

    (void) rpmtsFree(ts); 
    ts = NULL;

    optCon = rpmcliFini(optCon);

    return ec;
}
