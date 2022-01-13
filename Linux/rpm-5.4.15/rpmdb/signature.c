/** \ingroup signature
 * \file rpmdb/signature.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmurl.h>
#include <rpmcb.h>	/* XXX rpmIsVerbose() */
#define	_RPMPGP_INTERNAL
#include <rpmpgp.h>
#include <rpmmacro.h>	/* XXX for rpmGetPath() */
#include <rpmhkp.h>
#include <rpmku.h>
#include <argv.h>

#include <rpmtag.h>
#include "rpmdb.h"
#include <pkgio.h>	/* XXX expects <rpmts.h> */
#include "legacy.h"	/* XXX for dodogest() */
#include "signature.h"

#include "debug.h"

/*@access FD_t@*/		/* XXX ufdio->read arg1 is void ptr */
/*@access Header@*/		/* XXX compared with NULL */
/*@access DIGEST_CTX@*/		/* XXX compared with NULL */
/*@access pgpDig@*/
/*@access pgpDigParams@*/

int rpmTempFile(const char * prefix, const char ** fnptr, void * fdptr)
{
    const char * tpmacro = "%{?_tmppath}%{!?_tmppath:/var/tmp/}";
    const char * tempfn = NULL;
    const char * tfn = NULL;
    static int _initialized = 0;
    int temput;
    FD_t fd = NULL;
    unsigned int ran;

    if (!prefix) prefix = "";

    /* Create the temp directory if it doesn't already exist. */
    if (!_initialized) {
	_initialized = 1;
	tempfn = rpmGenPath(prefix, tpmacro, NULL);
	if (rpmioMkpath(tempfn, 0755, (uid_t) -1, (gid_t) -1))
	    goto errxit;
    }

    /* XXX should probably use mkstemp here */
    ran = (unsigned) time(NULL);
    srand(ran);
    ran = rand() % 100000;

    /* maybe this should use link/stat? */

    do {
	char tfnbuf[64];
#ifndef	NOTYET
	sprintf(tfnbuf, "rpm-tmp.%u", ran++);
	tempfn = _free(tempfn);
	tempfn = rpmGenPath(prefix, tpmacro, tfnbuf);
#else
	strcpy(tfnbuf, "rpm-tmp.XXXXXX");
	tempfn = _free(tempfn);
	tempfn = rpmGenPath(prefix, tpmacro, mktemp(tfnbuf));
#endif

	temput = urlPath(tempfn, &tfn);
	if (*tfn == '\0') goto errxit;

	switch (temput) {
	case URL_IS_DASH:
	case URL_IS_HKP:
	case URL_IS_MONGO:	/* XXX FIXME */
	    goto errxit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case URL_IS_HTTPS:
	case URL_IS_HTTP:
	case URL_IS_FTP:
	default:
	    /*@switchbreak@*/ break;
	}

	fd = Fopen(tempfn, "w+x.fdio");
	/* XXX FIXME: errno may not be correct for ufdio */
    } while ((fd == NULL || Ferror(fd)) && errno == EEXIST);

    if (fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("error creating temporary file %s\n"), tempfn);
	goto errxit;
    }

    switch(temput) {
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
      {	struct stat sb, sb2;
	if (!stat(tfn, &sb) && S_ISLNK(sb.st_mode)) {
	    rpmlog(RPMLOG_ERR, _("error creating temporary file %s\n"), tfn);
	    goto errxit;
	}

	if (sb.st_nlink != 1) {
	    rpmlog(RPMLOG_ERR, _("error creating temporary file %s\n"), tfn);
	    goto errxit;
	}

	if (fstat(Fileno(fd), &sb2) == 0) {
	    if (sb2.st_ino != sb.st_ino || sb2.st_dev != sb.st_dev) {
		rpmlog(RPMLOG_ERR, _("error creating temporary file %s\n"), tfn);
		goto errxit;
	    }
	}
      }	break;
    default:
	break;
    }

    if (fnptr)
	*fnptr = tempfn;
    else 
	tempfn = _free(tempfn);
    if (fdptr)
	*(FD_t *)fdptr = fd;

    return 0;

errxit:
    tempfn = _free(tempfn);
    if (fnptr)
	*fnptr = NULL;
    /*@-usereleased@*/
    if (fd != NULL) (void) Fclose(fd);
    /*@=usereleased@*/
    return 1;
}


/**
 * Generate GPG signature(s) for a header+payload file.
 * @param file		header+payload file name
 * @retval *sigTagp	signature tag
 * @retval *pktp	signature packet(s)
 * @retval *pktlenp	signature packet(s) length
 * @param passPhrase	private key pass phrase
 * @return		0 on success, 1 on failure
 */
static int makeGPGSignature(const char * file, rpmSigTag * sigTagp,
		/*@out@*/ rpmuint8_t ** pktp, /*@out@*/ rpmuint32_t * pktlenp,
		/*@null@*/ const char * passPhrase)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *pktp, *pktlenp, *sigTagp, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    char * sigfile = (char *) alloca(strlen(file)+sizeof(".sig"));
    pid_t pid;
    int status;
    int inpipe[2];
    FILE * fpipe;
    struct stat st;
    const char * cmd;
    char *const *av;
    pgpDig dig = NULL;
    pgpDigParams sigp = NULL;
    const char * pw = NULL;
    int rc;

    (void) stpcpy( stpcpy(sigfile, file), ".sig");

    addMacro(NULL, "__plaintext_filename", NULL, file, -1);
    addMacro(NULL, "__signature_filename", NULL, sigfile, -1);

    inpipe[0] = inpipe[1] = 0;
    if (pipe(inpipe) < 0) {
	rpmlog(RPMLOG_ERR, _("Couldn't create pipe for signing: %m"));
	return 1;
    }

    if (!(pid = fork())) {
	const char *gpg_path = rpmExpand("%{?_gpg_path}", NULL);

	(void) dup2(inpipe[0], 3);
	(void) close(inpipe[1]);

	if (gpg_path && *gpg_path != '\0')
	    (void) setenv("GNUPGHOME", gpg_path, 1);

	unsetenv("MALLOC_CHECK_");
	cmd = rpmExpand("%{?__gpg_sign_cmd}", NULL);
	rc = poptParseArgvString(cmd, NULL, (const char ***)&av);
	if (!rc)
	    rc = execve(av[0], av+1, environ);

	rpmlog(RPMLOG_ERR, _("Could not exec %s: %s\n"), "gpg",
			strerror(errno));
	_exit(EXIT_FAILURE);
    }

    delMacro(NULL, "__plaintext_filename");
    delMacro(NULL, "__signature_filename");

    pw = rpmkuPassPhrase(passPhrase);
    if (pw == NULL) {
	rpmlog(RPMLOG_ERR, _("Failed rpmkuPassPhrase(passPhrase): %s\n"),
			strerror(errno));
	return 1;
    }

    fpipe = fdopen(inpipe[1], "w");
    (void) close(inpipe[0]);
    if (fpipe) {
	fprintf(fpipe, "%s\n", (pw ? pw : ""));
	(void) fclose(fpipe);
    }

    if (pw != NULL) {
	(void) memset((void *)pw, 0, strlen(pw));
	pw = _free(pw);
    }

/*@+longunsignedintegral@*/
    (void) waitpid(pid, &status, 0);
/*@=longunsignedintegral@*/
    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
	rpmlog(RPMLOG_ERR, _("gpg exec failed (%d)\n"), WEXITSTATUS(status));
	return 1;
    }

    if (Stat(sigfile, &st)) {
	/* GPG failed to write signature */
	if (sigfile) (void) Unlink(sigfile);  /* Just in case */
	rpmlog(RPMLOG_ERR, _("gpg failed to write signature\n"));
	return 1;
    }

    *pktlenp = (rpmuint32_t)st.st_size;
    rpmlog(RPMLOG_DEBUG, D_("GPG sig size: %u\n"), (unsigned)*pktlenp);
    *pktp = (rpmuint8_t *) xmalloc(*pktlenp);

    {	FD_t fd;

	rc = 0;
	fd = Fopen(sigfile, "r.ufdio");
	if (fd != NULL && !Ferror(fd)) {
	    rc = (int) Fread(*pktp, sizeof((*pktp)[0]), *pktlenp, fd);
	    if (sigfile) (void) Unlink(sigfile);
	    (void) Fclose(fd);
	}
	if ((rpmuint32_t)rc != *pktlenp) {
	    *pktp = _free(*pktp);
	    rpmlog(RPMLOG_ERR, _("unable to read the signature\n"));
	    return 1;
	}
    }

    rpmlog(RPMLOG_DEBUG, D_("Got %u bytes of GPG sig\n"), (unsigned)*pktlenp);

    /* Parse the signature, change signature tag as appropriate. */
    dig = pgpDigNew(RPMVSF_DEFAULT, (pgpPubkeyAlgo)0);

    (void) pgpPrtPkts(*pktp, *pktlenp, dig, 0);
    sigp = pgpGetSignature(dig);

    /* Identify the type of signature being returned. */
    /* XXX FIXME: RPMSIGTAG{DSA,RSA} are interchangeable. */
    switch (*sigTagp) {
    default:
assert(0);	/* XXX never happens. */
	/*@notreached@*/ break;
    case RPMSIGTAG_SIZE:
    case RPMSIGTAG_MD5:
    case RPMSIGTAG_SHA1:
	break;
    case RPMSIGTAG_RSA:
	if (sigp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_DSA)
	    *sigTagp = RPMSIGTAG_DSA;
	break;
    case RPMSIGTAG_DSA:
	/* XXX check hash algorithm too? */
	if (sigp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_RSA)
	    *sigTagp = RPMSIGTAG_RSA;
	break;
    case RPMSIGTAG_ECDSA:
	break;
    }

    dig = pgpDigFree(dig);

    return 0;
}

/**
 * Generate header only signature(s) from a header+payload file.
 * @param sigh		signature header
 * @param file		header+payload file name
 * @param sigTag	type of signature(s) to add
 * @param passPhrase	private key pass phrase
 * @return		0 on success, -1 on failure
 */
/*@-mustmod@*/ /* sigh is modified */
static int makeHDRSignature(Header sigh, const char * file, rpmSigTag sigTag,
		/*@null@*/ const char * passPhrase)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies sigh, sigTag, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Header h = NULL;
    FD_t fd = NULL;
    rpmuint8_t * pkt;
    rpmuint32_t pktlen;
    const char * fn = NULL;
    const char * msg;
    rpmRC rc;
    int ret = -1;	/* assume failure. */
    int xx;

    switch (sigTag) {
    default:
assert(0);	/* XXX never happens. */
	/*@notreached@*/ break;
    case RPMSIGTAG_SIZE:
    case RPMSIGTAG_MD5:
    case RPMSIGTAG_PGP5:	/* XXX legacy */
    case RPMSIGTAG_PGP:
    case RPMSIGTAG_GPG:
	goto exit;
	/*@notreached@*/ break;
    case RPMSIGTAG_SHA1:
    {	const char * SHA1 = NULL;
	fd = Fopen(file, "r.fdio");
	if (fd == NULL || Ferror(fd))
	    goto exit;
	{   const char item[] = "Header";
	    msg = NULL;
	    rc = rpmpkgRead(item, fd, &h, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void) Fclose(fd);	fd = NULL;

	if (headerIsEntry(h, RPMTAG_HEADERIMMUTABLE)) {
	    unsigned char * hmagic = NULL;
	    size_t nmagic = 0;
	    DIGEST_CTX ctx;
	
	    he->tag = RPMTAG_HEADERIMMUTABLE;
	    if (!headerGet(h, he, 0) || he->p.ptr == NULL)
	    {
		(void)headerFree(h);
		h = NULL;
		goto exit;
	    }
	    (void) headerGetMagic(NULL, &hmagic, &nmagic);
	    ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
	    if (hmagic && nmagic > 0)
		(void) rpmDigestUpdate(ctx, hmagic, nmagic);
	    (void) rpmDigestUpdate(ctx, he->p.ptr, he->c);
	    (void) rpmDigestFinal(ctx, &SHA1, NULL, 1);
	    he->p.ptr = _free(he->p.ptr);
	}
	(void)headerFree(h);
	h = NULL;

	if (SHA1 == NULL)
	    goto exit;
	he->tag = (rpmTag) RPMSIGTAG_SHA1;
	he->t = RPM_STRING_TYPE;
	he->p.str = SHA1;
	he->c = 1;
	xx = headerPut(sigh, he, 0);
	SHA1 = _free(SHA1);
	if (!xx)
	    goto exit;
	ret = 0;
   }	break;
   case RPMSIGTAG_DSA:
	fd = Fopen(file, "r.fdio");
	if (fd == NULL || Ferror(fd))
	    goto exit;
	{   const char item[] = "Header";
	    msg = NULL;
	    rc = rpmpkgRead(item, fd, &h, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void) Fclose(fd);	fd = NULL;

	if (rpmTempFile(NULL, &fn, &fd))
	    goto exit;
	{   const char item[] = "Header";
	    msg = NULL;
	    rc = rpmpkgWrite(item, fd, h, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void) Fclose(fd);	fd = NULL;

	if (makeGPGSignature(fn, &sigTag, &pkt, &pktlen, passPhrase))
	    goto exit;
	he->tag = (rpmTag) sigTag;
	he->t = RPM_BIN_TYPE;
	he->p.ptr = pkt;
	he->c = pktlen;
	xx = headerPut(sigh, he, 0);
	if (!xx)
	    goto exit;
	ret = 0;
	break;
    case RPMSIGTAG_ECDSA:	/* XXX necessary when gnupg2 supports ECDSA */
	fd = Fopen(file, "r.fdio");
	if (fd == NULL || Ferror(fd))
	    goto exit;
	{   const char item[] = "Header";
	    msg = NULL;
	    rc = rpmpkgRead(item, fd, &h, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void) Fclose(fd);	fd = NULL;

	if (rpmTempFile(NULL, &fn, &fd))
	    goto exit;
	{   const char item[] = "Header";
	    msg = NULL;
	    rc = rpmpkgWrite(item, fd, h, &msg);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item, msg);
		msg = _free(msg);
		goto exit;
	    }
	    msg = _free(msg);
	}
	(void) Fclose(fd);	fd = NULL;

	if (makeGPGSignature(fn, &sigTag, &pkt, &pktlen, passPhrase))
	    goto exit;
	he->tag = (rpmTag) sigTag;
	he->t = RPM_BIN_TYPE;
	he->p.ptr = pkt;
	he->c = pktlen;
	xx = headerPut(sigh, he, 0);
	if (!xx)
	    goto exit;
	ret = 0;
	break;
    }

exit:
    if (fn) {
	(void) Unlink(fn);
	fn = _free(fn);
    }
    (void)headerFree(h);
    h = NULL;
    if (fd != NULL) (void) Fclose(fd);
    return ret;
}
/*@=mustmod@*/

int rpmAddSignature(Header sigh, const char * file, rpmSigTag sigTag,
		const char * passPhrase)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct stat st;
    rpmuint8_t * pkt;
    rpmuint32_t pktlen;
    int ret = -1;	/* assume failure. */
    int xx;

    switch (sigTag) {
    default:
assert(0);	/* XXX never happens. */
	/*@notreached@*/ break;
    case RPMSIGTAG_SIZE:
	if (Stat(file, &st) != 0)
	    break;
	pktlen = (rpmuint32_t)st.st_size;
	he->tag = (rpmTag) sigTag;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &pktlen;
	he->c = 1;
/*@-compmempass@*/
	xx = headerPut(sigh, he, 0);
/*@=compmempass@*/
	if (!xx)
	    break;
	ret = 0;
	break;
    case RPMSIGTAG_MD5:
	pktlen = 128/8;
	pkt = (rpmuint8_t *) memset(alloca(pktlen), 0, pktlen);
	if (dodigest(PGPHASHALGO_MD5, file, (unsigned char *)pkt, 0, NULL))
	    break;
	he->tag = (rpmTag) sigTag;
	he->t = RPM_BIN_TYPE;
	he->p.ptr = pkt;
	he->c = pktlen;
	xx = headerPut(sigh, he, 0);
	if (!xx)
	    break;
	ret = 0;
	break;
    case RPMSIGTAG_GPG:
	ret = makeHDRSignature(sigh, file, RPMSIGTAG_DSA, passPhrase);
	break;
    case RPMSIGTAG_SHA1:
    case RPMSIGTAG_RSA:
    case RPMSIGTAG_DSA:
    case RPMSIGTAG_ECDSA:
	ret = makeHDRSignature(sigh, file, sigTag, passPhrase);
	break;
    }

    return ret;
}

int rpmCheckPassPhrase(const char * passPhrase)
{
    const char *pw;
    int p[2];
    pid_t pid;
    int status;
    int rc;
    int xx;

    if (!(passPhrase && passPhrase[0]))
	return 0;

    p[0] = p[1] = 0;
    xx = pipe(p);

    if (!(pid = fork())) {
	const char * cmd;
	ARGV_t av;
	int fdno;

	xx = close(STDIN_FILENO);
	xx = close(STDOUT_FILENO);
	xx = close(p[1]);
	if (!rpmIsVerbose())
	    xx = close(STDERR_FILENO);
	if ((fdno = open("/dev/null", O_RDONLY)) != STDIN_FILENO) {
	    xx = dup2(fdno, STDIN_FILENO);
	    xx = close(fdno);
	}
	if ((fdno = open("/dev/null", O_WRONLY)) != STDOUT_FILENO) {
	    xx = dup2(fdno, STDOUT_FILENO);
	    xx = close(fdno);
	}
	xx = dup2(p[0], 3);

	unsetenv("MALLOC_CHECK_");
	{   const char *gpg_path = rpmExpand("%{?_gpg_path}", NULL);

	    if (gpg_path && *gpg_path != '\0')
  		(void) setenv("GNUPGHOME", gpg_path, 1);

	    cmd = rpmExpand("%{?__gpg_check_password_cmd}", NULL);
	    rc = argvSplit(&av, cmd, NULL);
	    if (!rc)
		rc = execve(av[0], (char *const *)av+1, environ);

	    rpmlog(RPMLOG_ERR, _("Could not exec %s: %s\n"), "gpg",
			strerror(errno));
	    cmd = _free(cmd);
	    gpg_path = _free(gpg_path);
	}
	av = argvFree(av);
    }

    pw = rpmkuPassPhrase(passPhrase);
    if (pw == NULL) {
	rpmlog(RPMLOG_ERR, _("Failed rpmkuPassPhrase(passPhrase): %s\n"),
			strerror(errno));
	return 1;
    }

    xx = close(p[0]);
    xx = (int) write(p[1], pw, strlen(pw));
    xx = (int) write(p[1], "\n", 1);
    xx = close(p[1]);

    if (pw != NULL) {
	(void) memset((void *)pw, 0, strlen(pw));
	pw = _free(pw);
    }

/*@+longunsignedintegral@*/
    (void) waitpid(pid, &status, 0);
/*@=longunsignedintegral@*/

    return ((!WIFEXITED(status) || WEXITSTATUS(status)) ? 1 : 0);
}

static /*@observer@*/ const char * rpmSigString(rpmRC res)
	/*@*/
{
    const char * str;
    switch (res) {
    case RPMRC_OK:		str = "OK";		break;
    default:
    case RPMRC_NOTFOUND:	str = "UNKNOWN";	break;
    case RPMRC_FAIL:		str = "BAD";		break;
    case RPMRC_NOTTRUSTED:	str = "NOTTRUSTED";	break;
    case RPMRC_NOKEY:		str = "NOKEY";		break;
    case RPMRC_NOSIG:		str = "NOSIG";		break;
    }
    return str;
}

static rpmRC
verifySize(const pgpDig dig, /*@out@*/ char * t)
	/*@modifies *t @*/
{
    const void * sig = pgpGetSig(dig);
    rpmRC res;
    rpmuint32_t size = 0xffffffff;

    *t = '\0';
    t = stpcpy(t, _("Header+Payload size: "));

    if (sig == NULL || dig == NULL || dig->nbytes == 0) {
	res = RPMRC_NOKEY;
	t = stpcpy(t, rpmSigString(res));
	goto exit;
    }

    memcpy(&size, sig, sizeof(size));

    if (size !=(rpmuint32_t) dig->nbytes) {
	res = RPMRC_FAIL;
	t = stpcpy(t, rpmSigString(res));
	sprintf(t, " Expected(%u) != (%u)\n", (unsigned)size, (unsigned)dig->nbytes);
    } else {
	res = RPMRC_OK;
	t = stpcpy(t, rpmSigString(res));
	sprintf(t, " (%u)", (unsigned)dig->nbytes);
    }

exit:
    return res;
}

static rpmRC
verifyMD5(pgpDig dig, /*@out@*/ char * t, /*@null@*/ DIGEST_CTX md5ctx)
	/*@globals internalState @*/
	/*@modifies *t, internalState @*/
{
    const void * sig = pgpGetSig(dig);
    rpmuint32_t siglen = pgpGetSiglen(dig);
    rpmRC res;
    rpmuint8_t * md5sum = NULL;
    size_t md5len = 0;

assert(dig != NULL);
assert(md5ctx != NULL);
assert(sig != NULL);

    *t = '\0';

    /* Identify the hash. */
    t = stpcpy(t, rpmDigestName(md5ctx));
    t = stpcpy(t, _(" digest: "));

    if (sig == NULL) {		/* XXX can't happen, DYING */
	res = RPMRC_NOKEY;
	t = stpcpy(t, rpmSigString(res));
	goto exit;
    }

    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 10);	/* RPMTS_OP_DIGEST */
	(void) rpmswEnter(op, 0);
	(void) rpmDigestFinal(rpmDigestDup(md5ctx), &md5sum, &md5len, 0);
	(void) rpmswExit(op, 0);
	if (op != NULL) op->count--;	/* XXX one too many */
    }

    if (md5len != siglen || memcmp(md5sum, sig, md5len)) {
	res = RPMRC_FAIL;
	t = stpcpy(t, rpmSigString(res));
	t = stpcpy(t, " Expected(");
	(void) pgpHexCvt(t, (rpmuint8_t *)sig, siglen);
	t += strlen(t);
	t = stpcpy(t, ") != (");
    } else {
	res = RPMRC_OK;
	t = stpcpy(t, rpmSigString(res));
	t = stpcpy(t, " (");
    }
    (void) pgpHexCvt(t, md5sum, md5len);
    t += strlen(t);
    t = stpcpy(t, ")");

exit:
    md5sum = _free(md5sum);
    return res;
}

/**
 * Verify header immutable region SHA-1 digest.
 * @param dig		container
 * @retval t		verbose success/failure text
 * @param shactx	SHA-1 digest context
 * @return 		RPMRC_OK on success
 */
static rpmRC
verifySHA1(pgpDig dig, /*@out@*/ char * t, /*@null@*/ DIGEST_CTX shactx)
	/*@globals internalState @*/
	/*@modifies *t, internalState @*/
{
    const void * sig = pgpGetSig(dig);
#ifdef	NOTYET
    rpmuint32_t siglen = pgpGetSiglen(dig);
#endif
    rpmRC res;
    const char * SHA1 = NULL;

assert(dig != NULL);
assert(shactx != NULL);
assert(sig != NULL);

    *t = '\0';
    t = stpcpy(t, _("Header "));

    /* Identify the hash. */
    t = stpcpy(t, rpmDigestName(shactx));
    t = stpcpy(t, _(" digest: "));

    if (sig == NULL) {		/* XXX can't happen, DYING */
	res = RPMRC_NOKEY;
	t = stpcpy(t, rpmSigString(res));
	goto exit;
    }

    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 10);	/* RPMTS_OP_DIGEST */
	(void) rpmswEnter(op, 0);
	(void) rpmDigestFinal(rpmDigestDup(shactx), &SHA1, NULL, 1);
	(void) rpmswExit(op, 0);
    }

    if (SHA1 == NULL
     || strlen(SHA1) != strlen((char *)sig)
     || strcmp(SHA1, (char *)sig))
    {
	res = RPMRC_FAIL;
	t = stpcpy(t, rpmSigString(res));
	t = stpcpy(t, " Expected(");
	t = stpcpy(t, (char *)sig);
	t = stpcpy(t, ") != (");
    } else {
	res = RPMRC_OK;
	t = stpcpy(t, rpmSigString(res));
	t = stpcpy(t, " (");
    }
    if (SHA1)
	t = stpcpy(t, SHA1);
    t = stpcpy(t, ")");

exit:
    SHA1 = _free(SHA1);
    return res;
}

/**
 * Verify RSA signature.
 * @param dig		container
 * @retval t		verbose success/failure text
 * @param hrsa	RSA digest context
 * @return 		RPMRC_OK on success
 */
static rpmRC
verifyRSA(pgpDig dig, /*@out@*/ char * t, /*@null@*/ DIGEST_CTX hrsa)
	/*@globals internalState @*/
	/*@modifies dig, *t, internalState */
{
    const void * sig = pgpGetSig(dig);
#ifdef	NOTYET
    rpmuint32_t siglen = pgpGetSiglen(dig);
#endif
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmRC res = RPMRC_OK;
    int xx;

if (_rpmhkp_debug)
fprintf(stderr, "--> %s(%p,%p,%p) sig %p sigp %p\n", __FUNCTION__, dig, t, hrsa, sig, sigp);

assert(dig != NULL);
assert(hrsa != NULL);
assert(sigp != NULL);
assert(sigp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_RSA);
assert(sigp->hash_algo == (rpmuint8_t)rpmDigestAlgo(hrsa));
assert(pgpGetSigtag(dig) == RPMSIGTAG_RSA);
assert(sig != NULL);

    /* Retrieve the matching public key. */
    /* XXX FIXME: this needs to be done early for SetRSA() PKCS1 generation. */
    res = (rpmRC) pgpFindPubkey(dig);
    if (res != RPMRC_OK)
	goto exit;

    *t = '\0';
    if (dig->hrsa == hrsa)
	t = stpcpy(t, _("Header "));

    /* Identify the signature version. */
    *t++ = 'V';
    switch (sigp->version) {
    case 3:	*t++ = '3';	break;
    case 4:	*t++ = '4';	break;
    }

    /* Identify the RSA/hash. */
    {   const char * hashname = rpmDigestName(hrsa);
	t = stpcpy(t, " RSA");
	if (strcmp(hashname, "UNKNOWN")) {
	    *t++ = '/';
	    t = stpcpy(t, hashname);
	}
    }
    t = stpcpy(t, _(" signature: "));

    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 10);	/* RPMTS_OP_DIGEST */
	DIGEST_CTX ctx = rpmDigestDup(hrsa);

	(void) rpmswEnter(op, 0);
	if (sigp->hash != NULL)
	    xx = rpmDigestUpdate(ctx, sigp->hash, sigp->hashlen);

	if (sigp->version == (rpmuint8_t) 4) {
	    rpmuint8_t trailer[6];
	    trailer[0] = sigp->version;
	    trailer[1] = (rpmuint8_t)0xff;
	    trailer[2] = (sigp->hashlen >> 24);
	    trailer[3] = (sigp->hashlen >> 16);
	    trailer[4] = (sigp->hashlen >>  8);
	    trailer[5] = (sigp->hashlen      );
	    xx = rpmDigestUpdate(ctx, trailer, sizeof(trailer));
	}
	(void) rpmswExit(op, sigp->hashlen);
	if (op != NULL) op->count--;	/* XXX one too many */

	if ((xx = pgpImplSetRSA(ctx, dig, sigp)) != 0) {
	    res = RPMRC_FAIL;
	    goto exit;
	}
    }

    /* Verify the RSA signature. */
    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 11);	/* RPMTS_OP_SIGNATURE */
	(void) rpmswEnter(op, 0);
	xx = pgpImplVerify(dig);
	(void) rpmswExit(op, 0);
	res = (xx ? RPMRC_OK : RPMRC_FAIL);
    }

exit:
    /* Identify the pubkey fingerprint. */
    t = stpcpy(t, rpmSigString(res));
    if (sigp != NULL) {
	t = stpcpy(t, ", key ID ");
	(void) pgpHexCvt(t, sigp->signid+4, sizeof(sigp->signid)-4);
	t += strlen(t);
    }
    return res;
}

/**
 * Verify DSA signature.
 * @param dig		container
 * @retval t		verbose success/failure text
 * @param hdsa		DSA digest context
 * @return 		RPMRC_OK on success
 */
static rpmRC
verifyDSA(pgpDig dig, /*@out@*/ char * t, /*@null@*/ DIGEST_CTX hdsa)
	/*@globals internalState @*/
	/*@modifies dig, *t, internalState */
{
    const void * sig = pgpGetSig(dig);
#ifdef	NOTYET
    rpmuint32_t siglen = pgpGetSiglen(dig);
#endif
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmRC res;
    int xx;

if (_rpmhkp_debug)
fprintf(stderr, "--> %s(%p,%p,%p) sig %p sigp %p\n", __FUNCTION__, dig, t, hdsa, sig, sigp);

assert(dig != NULL);
assert(hdsa != NULL);
assert(sigp != NULL);
assert(sigp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_DSA);
assert(sigp->hash_algo == (rpmuint8_t)rpmDigestAlgo(hdsa));
assert(pgpGetSigtag(dig) == RPMSIGTAG_DSA);
assert(sig != NULL);

    *t = '\0';
    if (dig != NULL && dig->hdsa == hdsa)
	t = stpcpy(t, _("Header "));

    /* Identify the signature version. */
    *t++ = 'V';
    switch (sigp->version) {
    case 3:    *t++ = '3';     break;
    case 4:    *t++ = '4';     break;
    }

    /* Identify the DSA/hash. */
    {   const char * hashname = rpmDigestName(hdsa);
	t = stpcpy(t, " DSA");
	if (strcmp(hashname, "UNKNOWN") && strcmp(hashname, "SHA1")) {
	    *t++ = '/';
	    t = stpcpy(t, hashname);
	}
    }
    t = stpcpy(t, _(" signature: "));

    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 10);	/* RPMTS_OP_DIGEST */
	DIGEST_CTX ctx = rpmDigestDup(hdsa);

	(void) rpmswEnter(op, 0);
	if (sigp->hash != NULL)
	    xx = rpmDigestUpdate(ctx, sigp->hash, sigp->hashlen);

	if (sigp->version == (rpmuint8_t) 4) {
	    rpmuint8_t trailer[6];
	    trailer[0] = sigp->version;
	    trailer[1] = (rpmuint8_t)0xff;
	    trailer[2] = (sigp->hashlen >> 24);
	    trailer[3] = (sigp->hashlen >> 16);
	    trailer[4] = (sigp->hashlen >>  8);
	    trailer[5] = (sigp->hashlen      );
	    xx = rpmDigestUpdate(ctx, trailer, sizeof(trailer));
	}
	(void) rpmswExit(op, sigp->hashlen);
	if (op != NULL) op->count--;	/* XXX one too many */

	if (pgpImplSetDSA(ctx, dig, sigp)) {
	    res = RPMRC_FAIL;
	    goto exit;
	}
    }

    /* Retrieve the matching public key. */
    res = (rpmRC) pgpFindPubkey(dig);
    if (res != RPMRC_OK)
	goto exit;

    /* Verify the DSA signature. */
    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 11); /* RPMTS_OP_SIGNATURE */
	(void) rpmswEnter(op, 0);
	xx = pgpImplVerify(dig);
	res = (xx ? RPMRC_OK : RPMRC_FAIL);
	(void) rpmswExit(op, 0);
    }

exit:
    /* Identify the pubkey fingerprint. */
    t = stpcpy(t, rpmSigString(res));
    if (sigp != NULL) {
	t = stpcpy(t, ", key ID ");
	(void) pgpHexCvt(t, sigp->signid+4, sizeof(sigp->signid)-4);
	t += strlen(t);
    }

if (_rpmhkp_debug)
fprintf(stderr, "<-- %s(%p,%p,%p) res %d %s\n", __FUNCTION__, dig, t, hdsa, res, t);

    return res;
}

/**
 * Verify ECDSA signature.
 * @param dig		container
 * @retval t		verbose success/failure text
 * @param hecdsa		ECDSA digest context
 * @return 		RPMRC_OK on success
 */
static rpmRC
verifyECDSA(pgpDig dig, /*@out@*/ char * t, /*@null@*/ DIGEST_CTX hecdsa)
	/*@globals internalState @*/
	/*@modifies dig, *t, internalState */
{
    const void * sig = pgpGetSig(dig);
#ifdef	NOTYET
    rpmuint32_t siglen = pgpGetSiglen(dig);
#endif
    pgpDigParams sigp = pgpGetSignature(dig);
    rpmRC res;
    int xx;

if (_rpmhkp_debug)
fprintf(stderr, "--> %s(%p,%p,%p) sig %p sigp %p\n", __FUNCTION__, dig, t, hecdsa, sig, sigp);

assert(dig != NULL);
assert(hecdsa != NULL);
assert(sigp != NULL);
assert(sigp->pubkey_algo == (rpmuint8_t)PGPPUBKEYALGO_ECDSA);
assert(sigp->hash_algo == (rpmuint8_t)rpmDigestAlgo(hecdsa));
assert(pgpGetSigtag(dig) == RPMSIGTAG_ECDSA);
assert(sig != NULL);

    *t = '\0';
    if (dig != NULL && dig->hecdsa == hecdsa)
	t = stpcpy(t, _("Header "));

    /* Identify the signature version. */
    *t++ = 'V';
    switch (sigp->version) {
    case 3:    *t++ = '3';     break;
    case 4:    *t++ = '4';     break;
    }

    /* Identify the ECDSA/hash. */
    {   const char * hashname = rpmDigestName(hecdsa);
	t = stpcpy(t, " ECDSA");
	if (strcmp(hashname, "UNKNOWN") && strcmp(hashname, "SHA1")) {
	    *t++ = '/';
	    t = stpcpy(t, hashname);
	}
    }
    t = stpcpy(t, _(" signature: "));

    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 10);	/* RPMTS_OP_DIGEST */
	DIGEST_CTX ctx = rpmDigestDup(hecdsa);

	(void) rpmswEnter(op, 0);
	if (sigp->hash != NULL)
	    xx = rpmDigestUpdate(ctx, sigp->hash, sigp->hashlen);

	if (sigp->version == (rpmuint8_t) 4) {
	    rpmuint8_t trailer[6];
	    trailer[0] = sigp->version;
	    trailer[1] = (rpmuint8_t)0xff;
	    trailer[2] = (sigp->hashlen >> 24);
	    trailer[3] = (sigp->hashlen >> 16);
	    trailer[4] = (sigp->hashlen >>  8);
	    trailer[5] = (sigp->hashlen      );
	    xx = rpmDigestUpdate(ctx, trailer, sizeof(trailer));
	}
	(void) rpmswExit(op, sigp->hashlen);
	if (op != NULL) op->count--;	/* XXX one too many */

	if (pgpImplSetECDSA(ctx, dig, sigp)) {
	    res = RPMRC_FAIL;
	    goto exit;
	}
    }

    /* Retrieve the matching public key. */
    res = (rpmRC) pgpFindPubkey(dig);
    if (res != RPMRC_OK)
	goto exit;

    /* Verify the ECDSA signature. */
    {	rpmop op = (rpmop)pgpStatsAccumulator(dig, 11); /* RPMTS_OP_SIGNATURE */
	(void) rpmswEnter(op, 0);
	xx = pgpImplVerify(dig);
	res = (xx ? RPMRC_OK : RPMRC_FAIL);
	(void) rpmswExit(op, 0);
    }

exit:
    /* Identify the pubkey fingerprint. */
    t = stpcpy(t, rpmSigString(res));
    if (sigp != NULL) {
	t = stpcpy(t, ", key ID ");
	(void) pgpHexCvt(t, sigp->signid+4, sizeof(sigp->signid)-4);
	t += strlen(t);
    }

if (_rpmhkp_debug)
fprintf(stderr, "<-- %s(%p,%p,%p) res %d %s\n", __FUNCTION__, dig, t, hecdsa, res, t);

    return res;
}

rpmRC
rpmVerifySignature(void * _dig, char * result)
{
    pgpDig dig = (pgpDig) _dig;
    const void * sig = pgpGetSig(dig);
    rpmuint32_t siglen = pgpGetSiglen(dig);
    rpmSigTag sigtag = (rpmSigTag) pgpGetSigtag(dig);
    rpmRC res;
pgpDigParams pubp = NULL;
pgpDigParams sigp = NULL;

if (_rpmhkp_debug)
fprintf(stderr, "--> %s(%p,%p) sig %p[%u]\n", __FUNCTION__, _dig, result, sig, siglen);

    if (dig == NULL || sig == NULL || siglen == 0) {
	sprintf(result, _("Verify signature: BAD PARAMETERS\n"));
	res = RPMRC_NOTFOUND;
	goto exit;
    }

pubp = pgpGetPubkey(dig);
sigp = pgpGetSignature(dig);
    switch (sigtag) {
    case RPMSIGTAG_SIZE:
	res = verifySize(dig, result);
	break;
    case RPMSIGTAG_MD5:
	res = verifyMD5(dig, result, dig->md5ctx);
	break;
    case RPMSIGTAG_SHA1:
	/* XXX dig->hsha? */
	res = verifySHA1(dig, result, dig->hdsa);
	break;
    case RPMSIGTAG_RSA:
pubp->pubkey_algo = PGPPUBKEYALGO_RSA;
sigp->pubkey_algo = PGPPUBKEYALGO_RSA;
	res = verifyRSA(dig, result, dig->hrsa);
	break;
    case RPMSIGTAG_DSA:
pubp->pubkey_algo = PGPPUBKEYALGO_DSA;
sigp->pubkey_algo = PGPPUBKEYALGO_DSA;
	res = verifyDSA(dig, result, dig->hdsa);
	break;
    case RPMSIGTAG_ECDSA:
pubp->pubkey_algo = PGPPUBKEYALGO_ECDSA;
sigp->pubkey_algo = PGPPUBKEYALGO_ECDSA;
	res = verifyECDSA(dig, result, dig->hecdsa);
	break;
    default:
	sprintf(result, _("Signature: UNKNOWN (%u)\n"), (unsigned)sigtag);
	res = RPMRC_NOTFOUND;
	break;
    }

exit:
if (_rpmhkp_debug)
fprintf(stderr, "<-- %s(%p,%p) res %d %s\n", __FUNCTION__, _dig, result, res, result);

    return res;
}
