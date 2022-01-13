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

#include <rpmns.h>
#include <rpmcli.h>

#include "genpgp.h"

/*@unchecked@*/
static int X_testPGP;		/* test clearsigned/detached signatures. */

static const char * str = "Signed\n";
static const char * X_DSApub =
"mQGiBEvXJ4MRBADufCObk8DJS4Hn72Hi8ykgiOgGyNx76BaeZArusQlVVk+7Qvqc\n"
"jE0itIMpQXbMm1oie3uGp9LgjlgHrCv85SWv5EJ3ijUWsJ742cmtT3wnDCqPHpQW\n"
"En+zFVP/iLWdrlwiFbJiSb6OgHCVO6Ks3PGU4M/3zCEtFodRfeYiDytFewCg27go\n"
"sKzeGFCKQWVtY4eKS/vmo5MD/RmIhmAC5GIRwPgJdbrXllsrXH8pu06efl5DgmYq\n"
"QsPIOFnOimr4PbBBJSwds6Pe3Iq9jFJH0Js/YJcWpjgIhbI1O0cYuSaO0u3FCeZD\n"
"htLdWBlk06B50R9KfYZ0myhwNi2qIio2oB4XJoTfv7mLFw53C+/a0RFW35aPQcfB\n"
"eguXA/0fXbYp6UYztzzeWruWvYZhOTwlV0Yqv1A65yMWDTNp4HPmHT0bXD/0L2co\n"
"pxAqLM6fCvm3B055klSOHvHRpx3ZT208F6z3FqQlEPoP220BHgsIDssdaSU5OCQb\n"
"bqr0tB1dZ8p+NGpfe5IHLRw4EcVHRr0CJz9EUxfwTEA1Uj11prQbRFNBcHViICgx\n"
"MDI0KSA8amJqQGpiai5vcmc+iGIEExECACIFAkvXJ4MCGwMGCwkIBwMCBhUIAgkK\n"
"CwQWAgMBAh4BAheAAAoJEG7Yx1bg/ydAvnAAniEBJ9HcjFaLj/PdhQG4dqlo/171\n"
"AJ98P5SlM7kp3kJFTNiopVC1b67+M5iNBEvXKMUBBADK0jbGV+DqlNA6eCpvR5oZ\n"
"rU97NgHNMuxph2GuCywhvO+MBievn/4Eu8hOunFLUcSxLZOiEC5tuNKiYVOUel4d\n"
"0NFyfDeNptCZ1mHjbWa0qFD44g9Ebx9oQm9ruBgcXPQiuDVBHOJCBM00Svb2wN84\n"
"DIUIrOhISEL9qz04k7PhDwARAQABtBtSU0FwdWIgKDEwMjQpIDxqYmpAamJqLm9y\n"
"Zz6IuAQTAQIAIgUCS9coxQIbAwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AACgkQ\n"
"aqP21Bd/XLRR4gQAwrc2QPI/dazDPloZkwtnqP1opbHFQag1jOTE2ETbmVGjSwkB\n"
"tQEeSPz/WXA3vL/cONvdh1P+TKB5eUTNlHPQjLt/9Omj8Dmr+Eski98HNQQ+d/SZ\n"
"HMOUZLyLc413Dxkqy0eQlBsfwmjUQlYbU/fAgQy0wr82GFAmLO6oCZ3vnBA=\n"
;
static const char * X_DSAsig =
"iEYEABECAAYFAkvXKMUACgkQbtjHVuD/J0DAWQCdHcAMmfw/gclWVtjwycH8mw8b\n"
"szUAmwexIfvOV9WPqTbszXPHVwOwd4BR\n"
;
static const char * X_RSApub =
"mQGiBEvXJ4MRBADufCObk8DJS4Hn72Hi8ykgiOgGyNx76BaeZArusQlVVk+7Qvqc\n"
"jE0itIMpQXbMm1oie3uGp9LgjlgHrCv85SWv5EJ3ijUWsJ742cmtT3wnDCqPHpQW\n"
"En+zFVP/iLWdrlwiFbJiSb6OgHCVO6Ks3PGU4M/3zCEtFodRfeYiDytFewCg27go\n"
"sKzeGFCKQWVtY4eKS/vmo5MD/RmIhmAC5GIRwPgJdbrXllsrXH8pu06efl5DgmYq\n"
"QsPIOFnOimr4PbBBJSwds6Pe3Iq9jFJH0Js/YJcWpjgIhbI1O0cYuSaO0u3FCeZD\n"
"htLdWBlk06B50R9KfYZ0myhwNi2qIio2oB4XJoTfv7mLFw53C+/a0RFW35aPQcfB\n"
"eguXA/0fXbYp6UYztzzeWruWvYZhOTwlV0Yqv1A65yMWDTNp4HPmHT0bXD/0L2co\n"
"pxAqLM6fCvm3B055klSOHvHRpx3ZT208F6z3FqQlEPoP220BHgsIDssdaSU5OCQb\n"
"bqr0tB1dZ8p+NGpfe5IHLRw4EcVHRr0CJz9EUxfwTEA1Uj11prQbRFNBcHViICgx\n"
"MDI0KSA8amJqQGpiai5vcmc+iGIEExECACIFAkvXJ4MCGwMGCwkIBwMCBhUIAgkK\n"
"CwQWAgMBAh4BAheAAAoJEG7Yx1bg/ydAvnAAniEBJ9HcjFaLj/PdhQG4dqlo/171\n"
"AJ98P5SlM7kp3kJFTNiopVC1b67+M5iNBEvXKMUBBADK0jbGV+DqlNA6eCpvR5oZ\n"
"rU97NgHNMuxph2GuCywhvO+MBievn/4Eu8hOunFLUcSxLZOiEC5tuNKiYVOUel4d\n"
"0NFyfDeNptCZ1mHjbWa0qFD44g9Ebx9oQm9ruBgcXPQiuDVBHOJCBM00Svb2wN84\n"
"DIUIrOhISEL9qz04k7PhDwARAQABtBtSU0FwdWIgKDEwMjQpIDxqYmpAamJqLm9y\n"
"Zz6IuAQTAQIAIgUCS9coxQIbAwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AACgkQ\n"
"aqP21Bd/XLRR4gQAwrc2QPI/dazDPloZkwtnqP1opbHFQag1jOTE2ETbmVGjSwkB\n"
"tQEeSPz/WXA3vL/cONvdh1P+TKB5eUTNlHPQjLt/9Omj8Dmr+Eski98HNQQ+d/SZ\n"
"HMOUZLyLc413Dxkqy0eQlBsfwmjUQlYbU/fAgQy0wr82GFAmLO6oCZ3vnBA=\n"
;
static const char * X_RSAsig =
"iJwEAAECAAYFAkvXKMUACgkQaqP21Bd/XLTHegP+Jwz7LIfLY/N+sUNbQTgiS3+M\n"
"6R2qC6AfUNvzdfmfb/0Bs5xpqRVLGoImNho3CJDsEDo3z+epkCgBBp73G1VK3Pap\n"
"zgjiUY8zOkPyv+sxM8A1vcjGtQV3Cu0NKvd2SeUvOOhBu4k/075XKSKA2dUDNzc1\n"
"OygF6KBppiB/AXl7qeA=\n"
;
static const char * X_ECDSAsig =
"iQBXAwUBSMjHk4QJEy1tIUM0EwhkLQEAw+SAPNMClecbCzvb8Rpx6fov5eJ3ShVs\n"
"gAtyf/w/IiwBAJNz/QD/qtHN13o1rlrJKlCXrbnfEsVl5gkMIULH+eEU\n"
;
static const char * X_ECDSApub =
"mQBSBEjIvdoTCCqGSM49AwEHAgMEilTubmrCj7X11p6CxMO+Ifg34Bzp8UZdHh5V\n"
"40oEoS7qcjFQ0e3gjWB9DfSkiTKLaOwhDBttR+Hw8TG/v4YtubQiRUNDIFAtMjU2\n"
"IDxlY2MtcC0yNTZAYnJhaW5odWIuY29tPokArAQQEwIAVAUCSMi92gUJCWYBgDAU\n"
"gAAAAAAgAAdwcmVmZXJyZWQtZW1haWwtZW5jb2RpbmdAcGdwLmNvbXBncG1pbWUD\n"
"CwkHAhkBBRsBAAAABR4BAAAAAxUICgAKCRAY1dFUqLaa54AuAQCgn5OR4KLbGzGS\n"
"4y223be1JC0KTK2GWCuJFIOE7g+llQEAgXbpaNqOcwyOV4PDdZq5Q0bP/QAIhkh8\n"
"hSF5N742KlS5AFUESMi92xIIKoZIzj0DAQcBCAcCAwRcohj5GzIlKZaKZuPqmr9l\n"
"B4jGEMvzgCglI1syEkJitvoOrR7dpAx/n56WBcMez3GPApv4ikmSGI3qB7HoqOep\n"
"iQBqBBgTAgASBQJIyL3bBQkJZgGABRsMAAAAAAoJEBjV0VSotprnk6oBAOetPj4t\n"
"5swFTcqfMZ3R4vh1RQ9k+tzlC36u8asprtxaAQCPH8HR2z/tggdsZKpdCtZyv3zn\n"
"GRTDr8dsiFqbp9mlIbkAUgRIyL3cEwgqhkjOPQMBBwIDBLUeFv+pPbEPmHfhqywM\n"
"weyvlMpIv8q/bwOhoU4hiUs+RMbrUx4tpe73T5kFKCa+9lmeurujkjx+63qpPPd2\n"
"ptyJAMoEGBMCAHIFAkjIvd0FCQlmAYAFGwIAAABfIAQZEwgABgUCSMi93AAKCRCE\n"
"CRMtbSFDNClhAQCxtGwZqqhYTw7GX1cEtxbYsWOD9ZkocZiU+x+ryCfqBgEA+WQN\n"
"Wfw5sfwG0GRScbPwFmkjlMnKwhuFK+g2V45+124ACgkQGNXRVKi2mufzNQEAuLNz\n"
"FeS/YwMWhTaedMduGYJG/sNxZhR+lVAgYNSy/FcA/RbDyY6W4C7geTB5iuTicfuK\n"
"cJmScU/alaH8fgp3uja1\n"
;

#include "debug.h"

/* FIPS-186 DSA test vectors. */
static const char * fips_p = "8df2a494492276aa3d25759bb06869cbeac0d83afb8d0cf7cbb8324f0d7882e5d0762fc5b7210eafc2e9adac32ab7aac49693dfbf83724c2ec0736ee31c80291";
static const char * fips_q = "c773218c737ec8ee993b4f2ded30f48edace915f";
static const char * fips_g = "626d027839ea0a13413163a55b4cb500299d5522956cefcb3bff10f399ce2c2e71cb9de5fa24babf58e5b79521925c9cc42e9f6f464b088cc572af53e6d78802";

static const char * fips_hm = "a9993e364706816aba3e25717850c26c9cd0d89d";

static const char * fips_y = "19131871d75b1612a819f29d78d1b0d7346f7aa77bb62a859bfd6c5675da9d212d3a36ef1672ef660b8c7c255cc0ec74858fba33f44c06699630a76b030ee333";

static const char * fips_r = "8bac1ab66410435cb7181f95b16ab97c92b341c0";
static const char * fips_s = "41e2345f1f56df2458f426d155b4ba2db6dcd8c8";

static int testFIPS186(void)
{
    pgpImplVecs_t * saveImplVecs = pgpImplVecs;
    pgpDig dig;
    rpmbc bc;
    rpmRC rc;

    pgpImplVecs = &rpmbcImplVecs;
    dig = pgpDigNew(RPMVSF_DEFAULT, 0);
pgpDigParams pubp = pgpGetPubkey(dig);
pubp->pubkey_algo = PGPPUBKEYALGO_DSA;
    bc = dig->impl;

    mpbzero(&bc->p);	mpbsethex(&bc->p, fips_p);
    mpbzero(&bc->q);	mpbsethex(&bc->q, fips_q);
    mpnzero(&bc->g);	mpnsethex(&bc->g, fips_g);
    mpnzero(&bc->y);	mpnsethex(&bc->y, fips_y);
    mpnzero(&bc->r);	mpnsethex(&bc->r, fips_r);
    mpnzero(&bc->s);	mpnsethex(&bc->s, fips_s);
    mpnzero(&bc->hm);	mpnsethex(&bc->hm, fips_hm);

    rc = pgpImplVerify(dig);

    dig = pgpDigFree(dig);
    pgpImplVecs = saveImplVecs;

    return rc;
}

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

static
rpmRC testSIG(pgpImplVecs_t * testImplVecs, const char * sigtype,
		pgpHashAlgo dalgo,
		const char * plaintext,
		const char *pubstr, const char * sigstr)
{
    pgpImplVecs_t * saveImplVecs = pgpImplVecs;
    pgpDig dig;
    DIGEST_CTX ctx;
    pgpDigParams dsig;
    pgpDigParams pubp;
    rpmRC rc;
    int printing = -1;
#ifdef	NOTYET
    rpmiob iob = NULL;
    rc = rpmiobSlurp(plaintextfn, &iob);
#endif

    pgpImplVecs = testImplVecs;

    dig = pgpDigNew(RPMVSF_DEFAULT, 0);

fprintf(stderr, "=============================== %s Public Key\n", sigtype);
    if ((rc = doit(pubstr, dig, printing)) != 0) {
	fprintf(stderr, "==> FAILED: rc %d\n", rc);
	goto exit;
    }

fprintf(stderr, "=============================== %s Signature of \"%s\"\n", sigtype, str);
    if ((rc = doit(sigstr, dig, printing)) != 0) {
	fprintf(stderr, "==> FAILED: rc %d\n", rc);
	goto exit;
    }

    ctx = rpmDigestInit(dalgo, RPMDIGEST_NONE);
    dsig = pgpGetSignature(dig);
	
    rpmDigestUpdate(ctx, str, strlen(str));
    rpmDigestUpdate(ctx, dsig->hash, dsig->hashlen);

pubp = pgpGetPubkey(dig);
    if (!strcmp(sigtype, "RSA")) {
	(void) pgpImplSetRSA(ctx, dig, dsig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_RSA);
pubp->pubkey_algo = PGPPUBKEYALGO_RSA;
	rc = pgpImplVerify(dig);
    } else if (!strcmp(sigtype, "DSA")) {
	(void) pgpImplSetDSA(ctx, dig, dsig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_DSA);
pubp->pubkey_algo = PGPPUBKEYALGO_DSA;
	rc = pgpImplVerify(dig);
    } else if (!strcmp(sigtype, "ECDSA")) {
	(void) pgpImplSetECDSA(ctx, dig, dsig);
assert(pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA);
pubp->pubkey_algo = PGPPUBKEYALGO_ECDSA;
	rc = pgpImplVerify(dig);
    }
fprintf(stderr, "=============================== %s verify: rc %d\n", sigtype, rc);

exit:
#ifdef	NOTYET
    iob = rpmiobFree(iob);
#endif
    dig = pgpDigFree(dig);
    pgpImplVecs = saveImplVecs;

    return rc;
}

static
rpmRC testPGP(rpmts ts, const char * sigtype)
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

#ifdef	NOTYET
    if (!strcmp("ECDSA", sigtype)) {
	rc = rpmnsProbeSignature(ts, ECDSApem, NULL, ECDSApub, ECDSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsig, ECDSApub, ECDSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsig, ECDSApubpem, ECDSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsigpem, ECDSApub, ECDSApubid, 0);

	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsigpem, ECDSApubpem, ECDSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsig, NULL, ECDSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, ECDSAsigpem, NULL, ECDSApubid, 0);
    }
#endif

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

static struct poptOption optionsTable[] = {
 { "testPGP", '\0', POPT_ARG_VAL,      &X_testPGP, -1,
        N_("test clearsigned/detached signatures"), NULL },

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
    int rc = rpmtsOpenDB(ts, O_RDONLY);

    pgpImplVecs_t * testImplVecs = &rpmnssImplVecs;

    /* Test RFC 2440/4880 clearsigned/detached plaintext signatires. */
    if (X_testPGP) {
fprintf(stderr, " DSA");
	rc = testPGP(ts, "DSA");
#ifdef	NOTYET
fprintf(stderr, " ECDSA");
	rc = testPGP(ts, "ECDSA");
#endif
fprintf(stderr, " RSA");
	rc = testPGP(ts, "RSA");
fprintf(stderr, "\n");
	goto exit;
    }

    rc = testFIPS186();
fprintf(stderr, "=============================== DSA FIPS-186-1: rc %d\n", rc);

    rc = testSIG(testImplVecs, "DSA", PGPHASHALGO_SHA1,
		str, X_DSApub, X_DSAsig);

    rc = testSIG(testImplVecs, "RSA", PGPHASHALGO_SHA1,
		str, X_RSApub, X_RSAsig);

    rc = testSIG(testImplVecs, "ECDSA", PGPHASHALGO_SHA256,
		str, X_ECDSApub, X_ECDSAsig);

exit:

    (void) rpmtsFree(ts);
    ts = NULL;

    optCon = rpmcliFini(optCon);

    return rc;
}
