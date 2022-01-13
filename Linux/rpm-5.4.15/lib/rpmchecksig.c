/** \ingroup rpmcli
 * \file lib/rpmchecksig.c
 * Verify the signature of a package.
 */

#include "system.h"

#include <rpmio_internal.h>
#include <poptIO.h>
#include <rpmbc.h>		/* XXX beecrypt base64 */

#define	_RPMHKP_INTERNAL	/* XXX internal prototypes. */
#include <rpmhkp.h>

#include <rpmtag.h>
#include <rpmtypes.h>
#define	_RPMEVR_INTERNAL	/* XXX RPMSENSE_KEYRING */
#include <rpmevr.h>
#define	_RPMDB_INTERNAL		/* XXX db_txn */
#include <rpmdb.h>
#include <rpmtxn.h>
#include <rpmxar.h>
#include <pkgio.h>
#include "signature.h"

#define	_RPMTS_INTERNAL		/* XXX ts->hkp */
#include <rpmts.h>

#include "rpmgi.h"

#include <rpmversion.h>
#include <rpmcli.h>

#include "debug.h"

/*@access FD_t @*/		/* XXX stealing digests */
/*@access Header @*/		/* XXX void * arg */
/*@access pgpDig @*/
/*@access pgpDigParams @*/

#ifdef __cplusplus
GENfree(struct rpmlead *)
GENfree(rpmuint8_t **)
#endif	/* __cplusplus */

/*@unchecked@*/
int _print_pkts = 0;

/**
 */
static int manageFile(/*@out@*/ FD_t *fdp,
		/*@null@*/ /*@out@*/ const char **fnp,
		int flags, /*@unused@*/ int rc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *fdp, *fnp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    const char *fn;
    FD_t fd;

    if (fdp == NULL)	/* programmer error */
	return 1;

    /* close and reset *fdp to NULL */
    if (*fdp && (fnp == NULL || *fnp == NULL)) {
	(void) Fclose(*fdp);
	*fdp = NULL;
	return 0;
    }

    /* open a file and set *fdp */
    if (*fdp == NULL && fnp != NULL && *fnp != NULL) {
	fd = Fopen(*fnp, ((flags & O_WRONLY) ? "w.fdio" : "r.fdio"));
	if (fd == NULL || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("%s: open failed: %s\n"), *fnp,
		Fstrerror(fd));
	    return 1;
	}
	*fdp = fd;
	return 0;
    }

    /* open a temp file */
    if (*fdp == NULL && (fnp == NULL || *fnp == NULL)) {
	fn = NULL;
	if (rpmTempFile(NULL, (fnp ? &fn : NULL), &fd)) {
	    rpmlog(RPMLOG_ERR, _("rpmTempFile failed\n"));
	    return 1;
	}
	if (fnp != NULL)
	    *fnp = fn;
/*@-refcounttrans@*/	/* FIX: XfdLink/XfdFree annotation */
	*fdp = fdLink(fd, "manageFile return");
	fd = fdFree(fd, "manageFile return");
/*@=refcounttrans@*/
	return 0;
    }

    /* no operation */
    if (*fdp != NULL && fnp != NULL && *fnp != NULL)
	return 0;

    /* XXX never reached */
    return 1;
}

/**
 * Copy header+payload, calculating digest(s) on the fly.
 */
static int copyFile(FD_t *sfdp, const char **sfnp,
		FD_t *tfdp, const char **tfnp)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *sfdp, *sfnp, *tfdp, *tfnp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    unsigned char buf[BUFSIZ];
    ssize_t count;
    int rc = 1;

    if (manageFile(sfdp, sfnp, O_RDONLY, 0))
	goto exit;
    if (manageFile(tfdp, tfnp, O_WRONLY|O_CREAT|O_TRUNC, 0))
	goto exit;

    while ((count = Fread(buf, sizeof(buf[0]), sizeof(buf), *sfdp)) > 0)
    {
	if (Fwrite(buf, sizeof(buf[0]), count, *tfdp) != (size_t)count) {
	    rpmlog(RPMLOG_ERR, _("%s: Fwrite failed: %s\n"), *tfnp,
		Fstrerror(*tfdp));
	    goto exit;
	}
    }
    if (count < 0) {
	rpmlog(RPMLOG_ERR, _("%s: Fread failed: %s\n"), *sfnp, Fstrerror(*sfdp));
	goto exit;
    }
    if (Fflush(*tfdp) != 0) {
	rpmlog(RPMLOG_ERR, _("%s: Fflush failed: %s\n"), *tfnp,
	    Fstrerror(*tfdp));
	goto exit;
    }

    rc = 0;

exit:
    if (*sfdp)	(void) manageFile(sfdp, NULL, 0, rc);
    if (*tfdp)	(void) manageFile(tfdp, NULL, 0, rc);
    return rc;
}

/**
 * Retrieve signer fingerprint from an OpenPGP signature tag.
 * @param sigh		signature header
 * @param sigtag	signature tag
 * @retval signid	signer fingerprint
 * @return		0 on success
 */
static int getSignid(Header sigh, rpmSigTag sigtag, unsigned char * signid)
	/*@globals fileSystem, internalState @*/
	/*@modifies *signid, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int rc = 1;
    int xx;

    he->tag = (rpmTag) sigtag;
    xx = headerGet(sigh, he, 0);
    if (xx && he->p.ptr != NULL) {
	pgpDig dig = pgpDigNew(RPMVSF_DEFAULT, PGPPUBKEYALGO_UNKNOWN);

	/* XXX expose ppSignid() from rpmhkp.c? */
	pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
	(void) pgpPktLen(he->p.ui8p, he->c, pp);
	if (!rpmhkpLoadSignature(NULL, dig, pp)) {
	    memcpy(signid, dig->signature.signid, sizeof(dig->signature.signid));
	    rc = 0;
	}

	he->p.ptr = _free(he->p.ptr);
	dig = pgpDigFree(dig);
    }
    return rc;
}

/** \ingroup rpmcli
 * Create/modify elements in signature header.
 * @param ts		transaction set
 * @param qva		mode flags and parameters
 * @param argv		array of package file names (NULL terminated)
 * @return		0 on success
 */
static int rpmReSign(/*@unused@*/ rpmts ts,
		QVA_t qva, const char ** argv)
        /*@globals rpmGlobalMacroContext, h_errno,
                fileSystem, internalState @*/
        /*@modifies ts, rpmGlobalMacroContext,
                fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmgi gi = NULL;
    FD_t fd = NULL;
    FD_t ofd = NULL;
    struct rpmlead *lead = NULL;
    rpmSigTag sigtag;
    const char *sigtarget = NULL;
    char tmprpm[1024+1];
    Header sigh = NULL;
    int res = 1;	/* XXX assume failure */
    int deleting = (qva->qva_mode == RPMSIGN_DEL_SIGNATURE);
    rpmRC rpmrc = RPMRC_OK;
    int xx;
    int i;
    mode_t mode;
    
    tmprpm[0] = '\0';

    if (argv)
 {       /* start-of-arg-iteration */
    rpmuint32_t tag = (qva->qva_source == RPMQV_FTSWALK)
	? RPMDBI_FTSWALK : RPMDBI_ARGLIST;
    rpmgiFlags _giFlags = RPMGI_NONE;

    gi = rpmgiNew(ts, tag, NULL, 0);
/*@-mods@*/
    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
/*@=mods@*/
    rpmrc = rpmgiSetArgs(gi, argv, rpmioFtsOpts, (rpmgiFlags)(_giFlags|RPMGI_NOHEADER));

    while ((rpmrc = rpmgiNext(gi)) == RPMRC_OK) {
	const char * fn = rpmgiHdrPath(gi);
	const char * tfn;

	fprintf(stdout, "%s:\n", fn);

/*@-modobserver@*/	/* XXX rpmgiHdrPath should not be observer */
	if (manageFile(&fd, &fn, O_RDONLY, 0))
	    goto exit;
/*@=modobserver@*/

    {	const char item[] = "Lead";
	const char * msg = NULL;
	rpmRC rc = rpmpkgRead(item, fd, &lead, &msg);
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
	    msg = _free(msg);
	    goto exit;
	}
	msg = _free(msg);
    }

    {	const char item[] = "Signature";
	const char * msg = NULL;
	rpmRC rc = rpmpkgRead(item, fd, &sigh, &msg);
	switch (rc) {
	default:
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item,
			(msg && *msg ? msg : ""));
	    msg = _free(msg);
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case RPMRC_OK:
	    if (sigh == NULL) {
		rpmlog(RPMLOG_ERR, _("%s: No signature available\n"), fn);
		goto exit;
	    }
	    /*@switchbreak@*/ break;
	}
	msg = _free(msg);
    }

	/* Write the header and archive to a temp file */
	/* ASSERT: ofd == NULL && sigtarget == NULL */
/*@-modobserver@*/	/* XXX rpmgiHdrPath should not be observer */
	if (copyFile(&fd, &fn, &ofd, &sigtarget))
	    goto exit;
/*@=modobserver@*/
	/* Both fd and ofd are now closed. sigtarget contains tempfile name. */
	/* ASSERT: fd == NULL && ofd == NULL */

	/* Lose the immutable region (if present). */
	he->tag = RPMTAG_HEADERSIGNATURES;
	xx = headerGet(sigh, he, 0);
	if (xx) {
	    HE_t ohe = (HE_t) memset(alloca(sizeof(*ohe)), 0, sizeof(*ohe));
	    HeaderIterator hi;
	    Header oh;
	    Header nh;

	    nh = headerNew();
	    if (nh == NULL) {
		he->p.ptr = _free(he->p.ptr);
		goto exit;
	    }

	    oh = headerCopyLoad(he->p.ptr);
	    for (hi = headerInit(oh);
		headerNext(hi, ohe, 0);
		ohe->p.ptr = _free(ohe->p.ptr))
	    {
		if (ohe->p.ptr) {
		    xx = headerPut(nh, ohe, 0);
		}
	    }
	    hi = headerFini(hi);
	    (void)headerFree(oh);
	    oh = NULL;

	    (void)headerFree(sigh);
	    sigh = NULL;
	    sigh = headerLink(nh);
	    (void)headerFree(nh);
	    nh = NULL;
	}

if (sigh != NULL) {
	/* Eliminate broken digest values. */
	he->tag = (rpmTag)RPMSIGTAG_LEMD5_1;
	xx = headerDel(sigh, he, 0);
	he->tag = (rpmTag)RPMSIGTAG_LEMD5_2;
	xx = headerDel(sigh, he, 0);
	he->tag = (rpmTag)RPMSIGTAG_BADSHA1_1;
	xx = headerDel(sigh, he, 0);
	he->tag = (rpmTag)RPMSIGTAG_BADSHA1_2;
	xx = headerDel(sigh, he, 0);

	/* Toss and recalculate header+payload size and digests. */
	{   static const rpmuint32_t sigs[] =
		{ RPMSIGTAG_SIZE, RPMSIGTAG_MD5, RPMSIGTAG_SHA1 };
	    size_t nsigs = sizeof(sigs) / sizeof(sigs[0]);
	    for (i = 0; i < (int)nsigs; i++) {
		he->tag = (rpmTag)sigs[i];
		xx = headerDel(sigh, he, 0);
		xx = rpmAddSignature(sigh, sigtarget, (rpmSigTag) he->tag, qva->passPhrase);
		if (xx)
		    goto exit;
	    }
	}

	if (deleting) {
	    /* Nuke all the signature tags. */
	    static const rpmuint32_t sigs[] =
		{ RPMSIGTAG_GPG, RPMSIGTAG_PGP5, RPMSIGTAG_PGP,
		  RPMSIGTAG_DSA, RPMSIGTAG_RSA, RPMSIGTAG_ECDSA };
	    size_t nsigs = sizeof(sigs) / sizeof(sigs[0]);
	    for (i = 0; i < (int)nsigs; i++) {
		he->tag = (rpmTag)sigs[i];
		xx = headerDel(sigh, he, 0);
	    }
	} else {	/* If gpg/pgp is configured, replace the signature. */
	  int addsig = 0;
	  sigtag = RPMSIGTAG_GPG;
	  addsig = 1;

	  if (addsig) {
	    unsigned char oldsignid[8], newsignid[8];

	    /* Grab the old signature fingerprint (if any) */
	    memset(oldsignid, 0, sizeof(oldsignid));
	    xx = getSignid(sigh, sigtag, oldsignid);

	    switch (sigtag) {
	    default:
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_ECDSA:
#ifdef	NOTYET		/* XXX FIXME: W2DO? */
		he->tag = (rpmTag)RPMSIGTAG_GPG;
		xx = headerDel(sigh, he, 0);
#endif
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_DSA:
		he->tag = (rpmTag)RPMSIGTAG_GPG;
		xx = headerDel(sigh, he, 0);
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_RSA:
		he->tag = (rpmTag)RPMSIGTAG_PGP;
		xx = headerDel(sigh, he, 0);
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_GPG:
		he->tag = (rpmTag)RPMSIGTAG_PGP;
		xx = headerDel(sigh, he, 0);
		he->tag = (rpmTag)RPMSIGTAG_DSA;
		xx = headerDel(sigh, he, 0);
		/*@fallthrough@*/
	    case RPMSIGTAG_PGP5:
	    case RPMSIGTAG_PGP:
		he->tag = (rpmTag)RPMSIGTAG_RSA;
		xx = headerDel(sigh, he, 0);
		/*@switchbreak@*/ break;
	    }

	    he->tag = (rpmTag)sigtag;
	    xx = headerDel(sigh, he, 0);
	    xx = rpmAddSignature(sigh, sigtarget, sigtag, qva->passPhrase);
	    if (xx)
		goto exit;

	    /* If package was previously signed, check for same signer. */
	    memset(newsignid, 0, sizeof(newsignid));
	    if (memcmp(oldsignid, newsignid, sizeof(oldsignid))) {

		/* Grab the new signature fingerprint */
		xx = getSignid(sigh, sigtag, newsignid);

		/* If same signer, skip resigning the package. */
		if (!memcmp(oldsignid, newsignid, sizeof(oldsignid))) {

		    rpmlog(RPMLOG_WARNING,
			_("%s: was already signed by key ID %s, skipping\n"),
			fn, pgpHexStr(newsignid+4, sizeof(newsignid)-4));

		    /* Clean up intermediate target */
		    xx = Unlink(sigtarget);
		    sigtarget = _free(sigtarget);
		    continue;
		}
	    }
	  }
	}

	/* Reallocate the signature into one contiguous region. */
	sigh = headerReload(sigh, RPMTAG_HEADERSIGNATURES);
	if (sigh == NULL)	/* XXX can't happen */
	    goto exit;
}

	/* Write the lead/signature of the output rpm */
	(void) stpcpy( stpcpy(tmprpm, fn), ".XXXXXX");

#if defined(HAVE_MKSTEMP)
         mode = umask(0077);
	(void) close(mkstemp(tmprpm));
        (void) umask(mode);
#else
	(void) mktemp(tmprpm);
#endif
	tfn = tmprpm;

	if (manageFile(&ofd, &tfn, O_WRONLY|O_CREAT|O_TRUNC, 0))
	    goto exit;

	{   const char item[] = "Lead";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgWrite(item, ofd, lead, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", tfn, item, Fstrerror(ofd));
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}

	{   const char item[] = "Signature";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgWrite(item, ofd, sigh, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", tfn, item, Fstrerror(ofd));
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void)headerFree(sigh);
	sigh = NULL;

	/* Append the header and archive from the temp file */
	/* ASSERT: fd == NULL && ofd != NULL */
	if (copyFile(&fd, &sigtarget, &ofd, &tfn))
	    goto exit;
	/* Both fd and ofd are now closed. */
	/* ASSERT: fd == NULL && ofd == NULL */

	/* Move final target into place. */
	xx = Unlink(fn);
	xx = Rename(tfn, fn);
	tmprpm[0] = '\0';

	/* Clean up intermediate target */
	xx = Unlink(sigtarget);
	sigtarget = _free(sigtarget);
    }

    /* XXX disambiguate end-of-iteration from item failures. */
    if (rpmrc == RPMRC_NOTFOUND)
        rpmrc = (rpmRC) rpmgiRc(gi);

 }	/* end-of-arg-iteration */

    if (rpmrc == RPMRC_OK)
	res = 0;

exit:
    gi = rpmgiFree(gi);

    if (fd)	(void) manageFile(&fd, NULL, 0, res);
    if (ofd)	(void) manageFile(&ofd, NULL, 0, res);

    lead = _free(lead);
    (void)headerFree(sigh);
    sigh = NULL;

    if (sigtarget) {
	xx = Unlink(sigtarget);
	sigtarget = _free(sigtarget);
    }
    if (tmprpm[0] != '\0') {
	xx = Unlink(tmprpm);
	tmprpm[0] = '\0';
    }

    return res;
}

rpmRC rpmcliImportPubkey(const rpmts ts, const unsigned char * pkt, ssize_t pktlen)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    static unsigned char zeros[] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const char * afmt = "%{pubkeys:armor}";
    const char * group = "Public Keys";
    const char * license = "pubkey";
    const char * buildhost = "localhost";
    rpmuint32_t pflags = (RPMSENSE_KEYRING|RPMSENSE_EQUAL);
    rpmuint32_t zero = 0;
    pgpDig dig = NULL;
    pgpDigParams pubp = NULL;
    const char * d = NULL;
    const char * enc = NULL;
    const char * n = NULL;
    const char * u = NULL;
    const char * v = NULL;
    const char * r = NULL;
    const char * evr = NULL;
    Header h = NULL;
    rpmRC rc = RPMRC_FAIL;		/* assume failure */
    char * t;
    int xx;
rpmhkp hkp = NULL;
pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));
int validate = 1;

    /* XXX coverity #1035781 */
    /* Validate the pubkey. */
    if (ts->hkp == NULL)
        ts->hkp = rpmhkpNew(NULL, 0);
    hkp = rpmhkpLink(ts->hkp);
hkp->pkt = (rpmuint8_t *)pkt;
hkp->pktlen = pktlen;

    if (pkt == NULL || pktlen <= 0)
	goto exit;
    if (rpmtsOpenDB(ts, (O_RDWR|O_CREAT)))
	goto exit;

/*@-moduncon@*/
    if ((enc = b64encode(pkt, pktlen)) == NULL)
	goto exit;
/*@=moduncon@*/

    dig = pgpDigNew(RPMVSF_DEFAULT, PGPPUBKEYALGO_UNKNOWN);
    pubp = pgpGetPubkey(dig);

    xx = pgpGrabPkts(hkp->pkt, hkp->pktlen, &hkp->pkts, &hkp->npkts);
    if (!xx)
	(void) pgpPubkeyFingerprint(hkp->pkt, hkp->pktlen, hkp->keyid);
    memcpy(pubp->signid, hkp->keyid, sizeof(pubp->signid)); /* XXX useless */

    xx = pgpPktLen(hkp->pkt, hkp->pktlen, pp);

    xx = rpmhkpLoadKey(hkp, dig, 0, 0);

    /* Validate pubkey self-signatures. */
    if (validate) {
	rpmRC yy = rpmhkpValidate(hkp, NULL);
	switch (yy) {
	case RPMRC_OK:
	    break;
	case RPMRC_NOTFOUND:
	case RPMRC_FAIL:	/* XXX remap to NOTFOUND? */
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	default:
#ifdef	NOTYET	/* XXX make check-pubkey fails?!? . */
	    rc = yy;
	    goto exit;
#endif
	    /*@notreached@*/ break;
	}
    }

    /* XXX hack up a user id (if not already present) */
    if (pubp->userid == NULL) {
	if (hkp->uidx >= 0 && hkp->uidx < hkp->npkts) {
	    size_t nb = pgpPktLen(hkp->pkts[hkp->uidx], hkp->pktlen, pp);
	    char * t;
	    nb = pp->hlen;
	    t = (char *) memcpy(xmalloc(nb + 1), pp->u.u->userid, nb);
	    t[nb] = '\0';
	    pubp->userid = t;
	} else
	    pubp->userid = xstrdup(pgpHexStr(pubp->signid+4, 4));
    }

#ifdef  DYING
_rpmhkpDumpDig(__FUNCTION__, dig);
#endif

    /* Build header elements. */
    if (!memcmp(pubp->signid, zeros, sizeof(pubp->signid))
     || !memcmp(pubp->time, zeros, sizeof(pubp->time))
     || pubp->userid == NULL)
	goto exit;

    v = t = (char *) xmalloc(16+1);
    t = stpcpy(t, pgpHexStr(pubp->signid, sizeof(pubp->signid)));

    r = t = (char *) xmalloc(8+1);
    t = stpcpy(t, pgpHexStr(pubp->time, sizeof(pubp->time)));

    n = t = (char *) xmalloc(sizeof("gpg()")+8);
    t = stpcpy( stpcpy( stpcpy(t, "gpg("), v+8), ")");

    {	const char * userid =
		(pubp->userid ? pubp->userid : pgpHexStr(pubp->signid+4, 4));
	u = t = (char *) xmalloc(sizeof("gpg()")+strlen(userid));
	t = stpcpy( stpcpy( stpcpy(t, "gpg("), userid), ")");
    }

    evr = t = (char *) xmalloc(sizeof("4X:-")+strlen(v)+strlen(r));
    t = stpcpy(t, (pubp->version == 4 ? "4:" : "3:"));
    t = stpcpy( stpcpy( stpcpy(t, v), "-"), r);

    /* Check for pre-existing header. */

    /* Build pubkey header. */
    h = headerNew();

    he->append = 1;

    he->tag = RPMTAG_PUBKEYS;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &enc;
    he->c = 1;
    xx = headerPut(h, he, 0);

    he->append = 0;

    d = headerSprintf(h, afmt, NULL, rpmHeaderFormats, NULL);
    if (d == NULL)
	goto exit;

    he->t = RPM_STRING_TYPE;
    he->c = 1;
    he->tag = RPMTAG_NAME;
    he->p.str = xstrdup("gpg-pubkey");
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);
    he->tag = RPMTAG_VERSION;
    he->p.str = v+8;
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_RELEASE;
    he->p.str = xstrdup(r);
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);

    /* Add Summary/Description/Group. */
    he->tag = RPMTAG_DESCRIPTION;
    he->p.str = xstrdup(d);
#if defined(SUPPORT_I18NSTRING_TYPE)
    xx = headerAddI18NString(h, he->tag, he->p.str, "C");
#else
    xx = headerPut(h, he, 0);
#endif
    he->p.ptr = _free(he->p.ptr);

    he->tag = RPMTAG_GROUP;
    he->p.str = xstrdup(group);
#if defined(SUPPORT_I18NSTRING_TYPE)
    xx = headerAddI18NString(h, he->tag, he->p.str, "C");
#else
    xx = headerPut(h, he, 0);
#endif
    he->p.ptr = _free(he->p.ptr);

    he->tag = RPMTAG_SUMMARY;
    he->p.str = xstrdup(u);
#if defined(SUPPORT_I18NSTRING_TYPE)
    xx = headerAddI18NString(h, he->tag, he->p.str, "C");
#else
    xx = headerPut(h, he, 0);
#endif
    he->p.ptr = _free(he->p.ptr);

#ifdef	NOTYET	/* XXX can't erase pubkeys with "pubkey" arch. */
    /* Add a "pubkey" arch/os to avoid missing value NULL ptrs. */
    he->tag = RPMTAG_ARCH;
    he->p.str = "pubkey";
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_OS;
    he->p.str = "pubkey";
    xx = headerPut(h, he, 0);
#endif

    he->tag = RPMTAG_LICENSE;
    he->p.str = xstrdup(license);
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);

    he->tag = RPMTAG_SIZE;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &zero;
    he->c = 1;
    xx = headerPut(h, he, 0);

    he->append = 1;

    /* Provides: gpg(IDENTITY) = PUBKEYVERSIONTYPE:PUBKEYID-CREATION */
    he->tag = RPMTAG_PROVIDENAME;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &u;
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_PROVIDEVERSION;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &evr;
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_PROVIDEFLAGS;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &pflags;
    he->c = 1;
    xx = headerPut(h, he, 0);

    /* Provides: gpg(PUBKEYID) = PUBKEYVERSION:PUBKEYID-CREATION */
    he->tag = RPMTAG_PROVIDENAME;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &n;
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_PROVIDEVERSION;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &evr;
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->tag = RPMTAG_PROVIDEFLAGS;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &pflags;
    he->c = 1;
    xx = headerPut(h, he, 0);

    he->append = 0;

    he->tag = RPMTAG_RPMVERSION;
    he->t = RPM_STRING_TYPE;
    he->p.str = xstrdup(RPMVERSION);
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);

    /* XXX W2DO: tag value inherited from parent? */
    he->tag = RPMTAG_BUILDHOST;
    he->t = RPM_STRING_TYPE;
    he->p.str = xstrdup(buildhost);
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);

    {   rpmuint32_t tid = rpmtsGetTid(ts);
	he->tag = RPMTAG_INSTALLTIME;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &tid;
	he->c = 1;
	xx = headerPut(h, he, 0);
	/* XXX W2DO: tag value inherited from parent? */
	he->tag = RPMTAG_BUILDTIME;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &tid;
	he->c = 1;
	xx = headerPut(h, he, 0);
    }

#ifdef	NOTYET
    /* XXX W2DO: tag value inherited from parent? */
    he->tag = RPMTAG_SOURCERPM;
    he->t = RPM_STRING_TYPE;
    he->p.str = fn;
    he->c = 1;
    xx = headerPut(h, he, 0);
#endif

    /* Reallocate the pubkey header into an immutable region. */
    he->tag = RPMTAG_HEADERIMMUTABLE;
    h = headerReload(h, he->tag);
    {	size_t length = 0;
	he->t = RPM_BIN_TYPE;
	he->p.ptr = headerUnload(h, &length);
	he->c = length;
    }

    /* Calculate the header-only SHA1 digest. */
    {	DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	unsigned char * hmagic = NULL;
	size_t nmagic = 0;
	const char * SHA1 = NULL;
	
	(void) headerGetMagic(NULL, &hmagic, &nmagic);
	if (hmagic && nmagic > 0)
	    (void) rpmDigestUpdate(ctx, hmagic, nmagic);
	(void) rpmDigestUpdate(ctx, he->p.ptr, he->c);
	(void) rpmDigestFinal(ctx, &SHA1, NULL, 1);
	he->p.ptr = _free(he->p.ptr);

        if (SHA1 == NULL)
            goto exit;
        he->tag = RPMTAG_SHA1HEADER;
        he->t = RPM_STRING_TYPE;
        he->p.str = SHA1;
        he->c = 1;
        xx = headerPut(h, he, 0);
        SHA1 = _free(SHA1);
    }

    he->tag = RPMTAG_PACKAGECOLOR;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &zero;
    he->c = 1;
    xx = headerPut(h, he, 0);

    /* Add header to database. */
    xx = rpmtxnBegin(rpmtsGetRdb(ts), NULL, NULL);
    xx = rpmdbAdd(rpmtsGetRdb(ts), rpmtsGetTid(ts), h, NULL);
    if (xx != 0) {
	xx = rpmtxnAbort(rpmtsGetRdb(ts)->db_txn);
	rpmtsGetRdb(ts)->db_txn = NULL;
	goto exit;
    } else
	xx = rpmtxnCommit(rpmtsGetRdb(ts)->db_txn);
    rpmtsGetRdb(ts)->db_txn = NULL;
    xx = rpmtxnCheckpoint(rpmtsGetRdb(ts));
    rc = RPMRC_OK;

exit:
    /* Clean up. */
hkp->pkt = NULL;
hkp->pktlen = 0;
hkp->pkts = _free(hkp->pkts);
hkp->npkts = 0;
    (void) rpmhkpFree(hkp);
    hkp = NULL;
    (void)headerFree(h);
    h = NULL;
    dig = pgpDigFree(dig);
    n = _free(n);
    u = _free(u);
    v = _free(v);
    r = _free(r);
    evr = _free(evr);
    enc = _free(enc);
    d = _free(d);
    
    return rc;
}

/** \ingroup rpmcli
 * Import public key(s).
 * @todo Implicit --update policy for gpg-pubkey headers.
 * @param ts		transaction set
 * @param qva		mode flags and parameters
 * @param argv		array of pubkey file names (NULL terminated)
 * @return		0 on success
 */
static int rpmcliImportPubkeys(const rpmts ts,
		/*@unused@*/ QVA_t qva,
		/*@null@*/ const char ** argv)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    const char * fn;
    rpmuint8_t * pkt = NULL;
    size_t pktlen = 0;
    char * t = NULL;
    int res = 0;
    rpmRC rpmrc;
    int rc;

    if (argv == NULL) return res;

    while ((fn = *argv++) != NULL) {

	rpmtsClean(ts);
	pkt = _free(pkt);
	t = _free(t);

	/* If arg looks like a keyid, then attempt keyserver retrieve. */
	if (fn[0] == '0' && fn[1] == 'x') {
	    const char * s;
	    int i;
	    for (i = 0, s = fn+2; *s && isxdigit(*s); s++, i++)
		{};
	    if (i == 8 || i == 16) {
		t = rpmExpand("%{_hkp_keyserver_query}", fn, NULL);
		if (t && *t != '%')
		    fn = t;
	    }
	}

	/* Read pgp packet. */
	if ((rc = pgpReadPkts(fn, &pkt, &pktlen)) <= 0) {
	    rpmlog(RPMLOG_ERR, _("%s: import read failed(%d).\n"), fn, rc);
	    res++;
	    continue;
	}
	if (rc != PGPARMOR_PUBKEY) {
	    rpmlog(RPMLOG_ERR, _("%s: not an armored public key.\n"), fn);
	    res++;
	    continue;
	}

	/* Import pubkey packet(s). */
	if ((rpmrc = rpmcliImportPubkey(ts, pkt, pktlen)) != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, _("%s: import failed.\n"), fn);
	    res++;
	    continue;
	}

    }
    
rpmtsClean(ts);
    pkt = _free(pkt);
    t = _free(t);
    return res;
}

/**
 * @todo If the GPG key was known available, the md5 digest could be skipped.
 */
static rpmRC readFile(FD_t fd, const char * fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fd, fileSystem, internalState @*/
{
rpmxar xar = fdGetXAR(fd);
pgpDig dig = fdGetDig(fd);
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    unsigned char buf[4*BUFSIZ];
    ssize_t count;
    unsigned ix;
    rpmRC rc;
    int xx;

    dig->nbytes = 0;

    /* Read the header from the package. */
    {	Header h = NULL;
	const char item[] = "Header";
	const char * msg = NULL;
	rc = rpmpkgRead(item, fd, &h, &msg);
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
	    msg = _free(msg);
	    goto exit;
	}
	msg = _free(msg);

	dig->nbytes += headerSizeof(h);

	/* Fish out the autosign pubkey (if present). */
	he->tag = RPMTAG_PUBKEYS;
	xx = headerGet(h, he, 0);
	if (xx && he->p.argv != NULL && he->c > 0)
	switch (he->t) {
	default:
	    break;
	case RPM_STRING_ARRAY_TYPE:
	    ix = he->c - 1; /* XXX FIXME: assumes last pubkey */
	    dig->pub = _free(dig->pub);
	    dig->publen = 0;
	    {	rpmiob iob = rpmiobNew(0);
		iob = rpmiobAppend(iob, he->p.argv[ix], 0);
		xx = pgpArmorUnwrap(iob, (rpmuint8_t **)&dig->pub, &dig->publen);
		iob = rpmiobFree(iob);
	    }
	    if (xx != PGPARMOR_PUBKEY) {
		dig->pub = _free(dig->pub);
		dig->publen = 0;
	    }
	    break;
	}
	he->p.ptr = _free(he->p.ptr);

	if (headerIsEntry(h, RPMTAG_HEADERIMMUTABLE)) {
	    unsigned char * hmagic = NULL;
	    size_t nmagic = 0;
	
	    he->tag = RPMTAG_HEADERIMMUTABLE;
	    xx = headerGet(h, he, 0);
	    if (!xx || he->p.ptr == NULL) {
		(void)headerFree(h);
		h = NULL;
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, _("headerGet failed"),
			_("failed to retrieve original header\n"));
		rc = RPMRC_FAIL;
		goto exit;
	    }
	    (void) headerGetMagic(NULL, &hmagic, &nmagic);
	    /* XXX dig->hsha? */
	    dig->hdsa = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	    if (hmagic && nmagic > 0)
		(void) rpmDigestUpdate(dig->hdsa, hmagic, nmagic);
	    (void) rpmDigestUpdate(dig->hdsa, he->p.ptr, he->c);
	    dig->hrsa = rpmDigestInit((pgpHashAlgo)dig->signature.hash_algo, RPMDIGEST_NONE);
	    if (hmagic && nmagic > 0)
		(void) rpmDigestUpdate(dig->hrsa, hmagic, nmagic);
	    (void) rpmDigestUpdate(dig->hrsa, he->p.ptr, he->c);
	    he->p.ptr = _free(he->p.ptr);
	}
	(void)headerFree(h);
	h = NULL;
    }

    if (xar != NULL) {
	const char item[] = "Payload";
	if ((xx = rpmxarNext(xar)) != 0 || (xx = rpmxarPull(xar, item)) != 0) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item,
		_("XAR file not found (or no XAR support)"));
	    rc = RPMRC_NOTFOUND;
	    goto exit;
	}
    }

    /* Read the payload from the package. */
    while ((count = Fread(buf, sizeof(buf[0]), sizeof(buf), fd)) > 0)
	dig->nbytes += count;
    if (count < 0 || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, _("Fread failed"), Fstrerror(fd));
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* XXX Steal the digest-in-progress from the file handle. */
    fdStealDigest(fd, dig);

    rc = RPMRC_OK;	/* XXX unnecessary */

exit:
    return rc;
}

int rpmVerifySignatures(QVA_t qva, rpmts ts, void * _fd, const char * fn)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HE_t she = (HE_t) memset(alloca(sizeof(*she)), 0, sizeof(*she));
/*@-castexpose@*/
    FD_t fd = (FD_t)_fd;
/*@=castexpose@*/
    char result[1024];
    char buf[8192], * b;
    char missingKeys[7164], * m;
    char untrustedKeys[7164], * u;
    pgpDig dig;
    pgpDigParams sigp;
    Header sigh = NULL;
    HeaderIterator hi = NULL;
    int res = 0;
    int xx;
    rpmRC sigres;
    int failed;
    int nodigests = !(qva->qva_flags & VERIFY_DIGEST);
    int nosignatures = !(qva->qva_flags & VERIFY_SIGNATURE);
pgpPkt pp = (pgpPkt) alloca(sizeof(*pp));

    {
	{   const char item[] = "Lead";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgRead(item, fd, NULL, &msg);
	    switch (rc) {
	    default:
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		res++;
		goto exit;
		/*@notreachea@*/ break;
	    case RPMRC_OK:
		break;
	    }
	    msg = _free(msg);
	}

	{   const char item[] = "Signature";
	    const char * msg = NULL;
	    rpmRC rc = rpmpkgRead(item, fd, &sigh, &msg);
	    switch (rc) {
	    default:
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item,
			(msg && *msg ? msg : ""));
		msg = _free(msg);
		res++;
		goto exit;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    case RPMRC_OK:
		if (sigh == NULL) {
		    rpmlog(RPMLOG_ERR, _("%s: No signature available\n"), fn);
		    res++;
		    goto exit;
		}
		/*@switchbreak@*/ break;
	    }
	    msg = _free(msg);
	}

	/* Grab a hint of what needs doing to avoid duplication. */
	she->tag = (rpmTag)0;
	if (she->tag == 0 && !nosignatures) {
	    if (headerIsEntry(sigh, (rpmTag) RPMSIGTAG_DSA))
		she->tag = (rpmTag) RPMSIGTAG_DSA;
	    else if (headerIsEntry(sigh, (rpmTag) RPMSIGTAG_RSA))
		she->tag = (rpmTag) RPMSIGTAG_RSA;
	    else if (headerIsEntry(sigh, (rpmTag) RPMSIGTAG_ECDSA))
		she->tag = (rpmTag) RPMSIGTAG_ECDSA;
	}
	if (she->tag == 0 && !nodigests) {
	    if (headerIsEntry(sigh, (rpmTag) RPMSIGTAG_MD5))
		she->tag = (rpmTag) RPMSIGTAG_MD5;
	    else if (headerIsEntry(sigh, (rpmTag) RPMSIGTAG_SHA1))
		she->tag = (rpmTag) RPMSIGTAG_SHA1;	/* XXX never happens */
	}

	dig = rpmtsDig(ts);
/*@-mods@*/	/* LCL: avoid void * _fd annotation for now. */
	(void) fdSetDig(fd, dig);
/*@=mods@*/
	sigp = pgpGetSignature(dig);

	/* XXX RSA needs the hash_algo, so decode early. */
	if ((rpmSigTag) she->tag == RPMSIGTAG_RSA) {
	    he->tag = she->tag;
	    xx = headerGet(sigh, he, 0);
	    xx = pgpPktLen(he->p.ui8p, he->c, pp);
	    xx = rpmhkpLoadSignature(NULL, dig, pp);
	    he->p.ptr = _free(he->p.ptr);
	}

/*@-mods@*/	/* LCL: avoid void * _fd annotation for now. */
	if (headerIsEntry(sigh, (rpmTag)RPMSIGTAG_MD5))
	    fdInitDigest(fd, PGPHASHALGO_MD5, 0);
/*@=mods@*/

	/* Read the file, generating digest(s) on the fly. */
/*@-mods@*/	/* LCL: avoid void * _fd annotation for now. */
	if (dig == NULL || sigp == NULL
	 || readFile(fd, fn) != RPMRC_OK)
	{
	    res++;
	    goto exit;
	}
/*@=mods@*/

	failed = 0;
	b = buf;		*b = '\0';
	m = missingKeys;	*m = '\0';
	u = untrustedKeys;	*u = '\0';
	sprintf(b, "%s:%c", fn, (rpmIsVerbose() ? '\n' : ' ') );
	b += strlen(b);

	if (sigh != NULL)
	for (hi = headerInit(sigh);
	    headerNext(hi, she, 0) != 0;
	    she->p.ptr = _free(she->p.ptr))
	{

assert(she->p.ptr != NULL);

	    /* Clean up parameters from previous she->tag. */
	    pgpDigClean(dig);

/*@-ownedtrans -noeffect@*/
	    xx = pgpSetSig(dig, she->tag, she->t, she->p.ptr, she->c);
/*@=ownedtrans =noeffect@*/

	    switch ((rpmSigTag)she->tag) {
	    case RPMSIGTAG_RSA:
	    case RPMSIGTAG_DSA:
	    case RPMSIGTAG_ECDSA:
		if (nosignatures)
		     continue;

		xx = pgpPktLen(she->p.ui8p, she->c, pp);
		xx = rpmhkpLoadSignature(NULL, dig, pp);
		if (sigp->version != 3 && sigp->version != 4) {
		    rpmlog(RPMLOG_ERR,
		_("skipping package %s with unverifiable V%u signature\n"),
			fn, sigp->version);
		    res++;
		    goto exit;
		}
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_SHA1:
		if (nodigests)
		     continue;
		/* XXX Don't bother with header sha1 if header dsa. */
		if (!nosignatures && (rpmSigTag)she->tag == RPMSIGTAG_DSA)
		    continue;
		/*@switchbreak@*/ break;
	    case RPMSIGTAG_MD5:
		if (nodigests)
		     continue;
		/*@switchbreak@*/ break;
	    default:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    }

	    sigres = rpmVerifySignature(dig, result);

	    if (sigres) {
		failed = 1;
		if (rpmIsVerbose())
		    b = stpcpy( stpcpy( stpcpy(b, "    "), result), "\n");
		else
		switch ((rpmSigTag)she->tag) {
		case RPMSIGTAG_SIZE:
		    b = stpcpy(b, "SIZE ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_SHA1:
		    b = stpcpy(b, "SHA1 ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_MD5:
		    b = stpcpy(b, "MD5 ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_RSA:
		    b = stpcpy(b, "RSA ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_DSA:
		    b = stpcpy(b, "(SHA1) DSA ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_ECDSA:
		    b = stpcpy(b, "ECDSA ");
		    /*@switchbreak@*/ break;	/* coverity #1214084 */
		default:
		    b = stpcpy(b, "?UnknownSignatureType? ");
		    /*@switchbreak@*/ break;
		}
	    } else {
		if (rpmIsVerbose())
		    b = stpcpy( stpcpy( stpcpy(b, "    "), result), "\n");
		else
		switch ((rpmSigTag)she->tag) {
		case RPMSIGTAG_SIZE:
		    b = stpcpy(b, "size ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_SHA1:
		    b = stpcpy(b, "sha1 ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_MD5:
		    b = stpcpy(b, "md5 ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_RSA:
		    b = stpcpy(b, "rsa ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_DSA:
		    b = stpcpy(b, "(sha1) dsa ");
		    /*@switchbreak@*/ break;
		case RPMSIGTAG_ECDSA:
		    b = stpcpy(b, "ecdsa ");
		    /*@switchbreak@*/ break;
		default:
		    b = stpcpy(b, "??? ");
		    /*@switchbreak@*/ break;
		}
	    }
	}
	hi = headerFini(hi);
	/* XXX clear the already free'd signature data. */
/*@-noeffect@*/
	xx = pgpSetSig(dig, 0, 0, NULL, 0);
/*@=noeffect@*/

	res += failed;

	if (failed) {
	    if (rpmIsVerbose()) {
		rpmlog(RPMLOG_NOTICE, "%s", buf);
	    } else {
		rpmlog(RPMLOG_NOTICE, "%s%s%s%s%s%s%s%s\n", buf,
			_("NOT_OK"),
			(missingKeys[0] != '\0') ? _(" (MISSING KEYS:") : "",
			missingKeys,
			(missingKeys[0] != '\0') ? _(") ") : "",
			(untrustedKeys[0] != '\0') ? _(" (UNTRUSTED KEYS:") : "",
			untrustedKeys,
			(untrustedKeys[0] != '\0') ? _(")") : "");

	    }
	} else {
	    if (rpmIsVerbose()) {
		rpmlog(RPMLOG_NOTICE, "%s", buf);
	    } else {
		rpmlog(RPMLOG_NOTICE, "%s%s%s%s%s%s%s%s\n", buf,
			_("OK"),
			(missingKeys[0] != '\0') ? _(" (MISSING KEYS:") : "",
			missingKeys,
			(missingKeys[0] != '\0') ? _(") ") : "",
			(untrustedKeys[0] != '\0') ? _(" (UNTRUSTED KEYS:") : "",
			untrustedKeys,
			(untrustedKeys[0] != '\0') ? _(")") : "");
	    }
	}

    }

exit:
    rpmtsCleanDig(ts);
    (void)headerFree(sigh);
    sigh = NULL;
    return res;
}

int rpmcliSign(rpmts ts, QVA_t qva, const char ** argv)
	/*@globals rpmioFtsOpts @*/
	/*@modifies rpmioFtsOpts @*/
{
    rpmRC rpmrc;
    int res = 0;

    if (argv == NULL) return res;

    switch (qva->qva_mode) {
    case RPMSIGN_CHK_SIGNATURE:
	break;
    case RPMSIGN_IMPORT_PUBKEY:
	return rpmcliImportPubkeys(ts, qva, argv);
	/*@notreached@*/ break;
    case RPMSIGN_NEW_SIGNATURE:
    case RPMSIGN_ADD_SIGNATURE:
    case RPMSIGN_DEL_SIGNATURE:
	return rpmReSign(ts, qva, argv);
	/*@notreached@*/ break;
    case RPMSIGN_NONE:
    default:
	return -1;
	/*@notreached@*/ break;
    }

{       /* start-of-arg-iteration */

    int tag = (qva->qva_source == RPMQV_FTSWALK)
	? RPMDBI_FTSWALK : RPMDBI_ARGLIST;
    rpmgi gi = rpmgiNew(ts, tag, NULL, 0);
    rpmgiFlags _giFlags = RPMGI_NONE;
    rpmRC rc;

    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
    rc = (rpmRC)
	rpmgiSetArgs(gi, argv, rpmioFtsOpts,
			(rpmgiFlags)(_giFlags|RPMGI_NOHEADER));
    while ((rpmrc = rpmgiNext(gi)) == RPMRC_OK) {
	const char * fn = rpmgiHdrPath(gi);
	FD_t fd;
	int xx;

	fd = Fopen(fn, "r.fdio");
	if (fd == NULL || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("%s: open failed: %s\n"), 
		     fn, Fstrerror(fd));
	    res++;
	} else if (rpmVerifySignatures(qva, ts, fd, fn)) {
	    res++;
	}

	if (fd != NULL) {
	    xx = Fclose(fd);
	}
    }

    /* XXX disambiguate end-of-iteration from item failures. */
    if (res == 0 && rpmrc == RPMRC_NOTFOUND) {
	rpmrc = (rpmRC) rpmgiRc(gi);
	if (rpmrc != RPMRC_OK)
	    res++;
    }

    gi = rpmgiFree(gi);

}	/* end-of-arg-iteration */

    return res;
}
