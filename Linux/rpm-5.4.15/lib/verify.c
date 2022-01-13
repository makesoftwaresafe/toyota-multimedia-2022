/** \ingroup rpmcli
 * \file lib/verify.c
 * Verify installed payload files from package metadata.
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmcb.h>
#include "ugid.h"

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>

#define	_RPMFI_INTERNAL
#include <rpmfi.h>

#define	_RPMSQ_INTERNAL
#include "psm.h"

#include "legacy.h"	/* XXX dodigest(), uidToUname(), gnameToGid */

#define	_RPMPS_INTERNAL	/* XXX rpmps needs iterator. */
#define	_RPMTS_INTERNAL	/* XXX expose rpmtsSetScriptFd */
#include <rpmcli.h>

#include "debug.h"

/*@access rpmts @*/	/* XXX cast */
/*@access rpmpsm @*/	/* XXX for %verifyscript through rpmpsmStage() */

#define S_ISDEV(m) (S_ISBLK((m)) || S_ISCHR((m)))

/*@unchecked@*/
extern int _rpmds_unspecified_epoch_noise;

typedef struct rpmvf_s * rpmvf;
struct rpmvf_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * fn;
    const char * flink;
    struct stat  sb;
    rpmfileAttrs fflags;
    rpmfileState fstate;
    rpmVerifyAttrs vflags;
    int dalgo;
    size_t dlen;
    const unsigned char * digest;
    const char * fuser;
    const char * fgroup;
#if defined(__LCLINT__NOTYET)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define VF_ISSET(_vflags, _FLAG)	((_vflags) & (RPMVERIFY_##_FLAG))
#define VF_SET(_vflags, _FLAG)	\
	(*((unsigned *)&(_vflags)) |= (RPMVERIFY_##_FLAG))
#define VF_CLR(_vflags, _FLAG)	\
	(*((unsigned *)&(_vflags)) &= ~(RPMVERIFY_##_FLAG))

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

GENfree(rpmvf)

#else	/* __cplusplus */

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define VF_ISSET(_vflags, _FLAG)	((_vflags) & (RPMVERIFY_##_FLAG))
#define VF_SET(_vflags, _FLAG)		(_vflags) |= (RPMVERIFY_##_FLAG)
#define VF_CLR(_vflags, _FLAG)		(_vflags) &= ~(RPMVERIFY_##_FLAG)

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

static rpmvf rpmvfFree(/*@only@*/ rpmvf vf)
	/*@modifies vf @*/
{

    if (vf) {
#ifdef	NOTYET
	yarnPossess(vf->_item.use);
	if (yarnPeekLock(vf->_item.use) <= 1L) {
	    yarnLock use = vf->_item.use;
	    vf->fn = _free(vf->fn);
	    vf = _free(vf);
	    yarnTwist(use, TO, 0);
	    use = yarnFreeLock(use);
	} else
	    yarnTwist(vf->_item.use, BY, -1);
#else
	vf->fn = _free(vf->fn);
	vf = _free(vf);
#endif
    }
    return NULL;
}

/*@only@*/
static rpmvf rpmvfNew(rpmts ts, rpmfi fi, int i, rpmVerifyAttrs omitMask)
	/*@*/
{
    rpmvf vf = (rpmvf) DRD_xcalloc(1, sizeof(*vf));

#ifdef	NOTYET
    vf->_item.use = yarnNewLock(1);
    vf->_item.pool = NULL;
#endif

/*@-mods@*/
    vf->fn = rpmGetPath(rpmtsRootDir(ts), fi->dnl[fi->dil[i]], fi->bnl[i], NULL);
/*@=mods@*/
    vf->flink = fi->flinks[i];
    vf->fuser = fi->fuser[i];
    vf->fgroup = fi->fgroup[i];

    {   struct stat *st = &vf->sb;
        st->st_dev =
        st->st_rdev = fi->frdevs[i];
        st->st_ino = fi->finodes[i];
        st->st_mode = fi->fmodes[i];
#ifdef	NOTNEEDED
        st->st_nlink = rpmfiFNlink(fi) + (int)S_ISDIR(st->st_mode);
#endif
        if (unameToUid(vf->fuser, &st->st_uid) == -1)
            st->st_uid = 0;             /* XXX */
        if (gnameToGid(vf->fgroup, &st->st_gid) == -1)
            st->st_gid = 0;             /* XXX */
        st->st_size = fi->fsizes[i];
        st->st_blksize = 4 * 1024;      /* XXX */
        st->st_blocks = (st->st_size + (st->st_blksize - 1)) / st->st_blksize;
        st->st_atime =
        st->st_ctime =
        st->st_mtime = fi->fmtimes[i];
    }

    vf->fflags = (rpmfileAttrs) fi->fflags[i];
    vf->fstate = (rpmfileState) fi->fstates[i];
    vf->vflags = (rpmVerifyAttrs) fi->vflags[i];
    vf->dalgo = fi->fdigestalgos
		? fi->fdigestalgos[i]
		: fi->digestalgo;
    vf->dlen = fi->digestlen;
    vf->digest = fi->digests + (fi->digestlen * i);

    /* Don't verify any features in omitMask. */
    {	unsigned * _vflagsp = (unsigned *)&vf->vflags;
	*_vflagsp &= ~(omitMask | RPMVERIFY_FAILURES);
    }

    /* Content checks of %ghost files are meaningless. */
    if (FF_ISSET(vf->fflags, GHOST)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, LINKTO);
	VF_CLR(vf->vflags, HMAC);
    }

    return vf;
}

/** \ingroup rpmcli
 * Verify file attributes (including file digest).
 * @param vf		file data to verify
 * @param spew		should verify results be printed?
 * @return		0 on success (or not installed), 1 on error
 */
static int rpmvfVerify(rpmvf vf, int spew)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies vf, fileSystem, internalState @*/
{
    rpmVerifyAttrs res = RPMVERIFY_NONE;
    struct stat sb;
    int ec = 0;

    /* Check to see if the file was installed - if not pretend all is OK. */
    switch (vf->fstate) {
    default:
    case RPMFILE_STATE_NETSHARED:
    case RPMFILE_STATE_REPLACED:
    case RPMFILE_STATE_NOTINSTALLED:
    case RPMFILE_STATE_WRONGCOLOR:
	goto exit;
	/*@notreached@*/ break;
    case RPMFILE_STATE_NORMAL:
	break;
    }

assert(vf->fn != NULL);
    if (vf->fn == NULL || Lstat(vf->fn, &sb) != 0) {
	VF_SET(res, LSTATFAIL);
	ec = 1;
	goto exit;
    }

    /* Not all attributes of non-regular files can be verified. */
    if (S_ISDIR(sb.st_mode)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, LINKTO);
	VF_CLR(vf->vflags, HMAC);
    } else if (S_ISLNK(sb.st_mode)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, MODE);
	VF_CLR(vf->vflags, HMAC);
#if CHOWN_FOLLOWS_SYMLINK
	VF_CLR(vf->vflags, USER);
	VF_CLR(vf->vflags, GROUP);
#endif
    }
    else if (S_ISFIFO(sb.st_mode)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, LINKTO);
	VF_CLR(vf->vflags, HMAC);
    } else if (S_ISCHR(sb.st_mode)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, LINKTO);
	VF_CLR(vf->vflags, HMAC);
    } else if (S_ISBLK(sb.st_mode)) {
	VF_CLR(vf->vflags, FDIGEST);
	VF_CLR(vf->vflags, FILESIZE);
	VF_CLR(vf->vflags, MTIME);
	VF_CLR(vf->vflags, LINKTO);
	VF_CLR(vf->vflags, HMAC);
    } else {
	VF_CLR(vf->vflags, LINKTO);
    }

    if (VF_ISSET(vf->vflags, FDIGEST) || VF_ISSET(vf->vflags, HMAC)) {
	if (vf->digest == NULL || vf->dlen == 0)
	    VF_SET(res, FDIGEST);
	else {
	/* XXX If --nofdigest, then prelinked library sizes fail to verify. */
	    unsigned char * fdigest = (unsigned char *)
			memset(alloca(vf->dlen), 0, vf->dlen);
	    size_t fsize = 0;
#define	_mask	(RPMVERIFY_FDIGEST|RPMVERIFY_HMAC)
	    unsigned dflags = (vf->vflags & _mask) == RPMVERIFY_HMAC
		? 0x2 : 0x0;
#undef	_mask
	    int rc = dodigest(vf->dalgo, vf->fn, fdigest, dflags, &fsize);
	    sb.st_size = fsize;
	    if (rc) {
		VF_SET(res, READFAIL);
		VF_SET(res, FDIGEST);
	    } else
	    if (memcmp(fdigest, vf->digest, vf->dlen))
		VF_SET(res, FDIGEST);
	}
    }

    if (VF_ISSET(vf->vflags, LINKTO)) {
	char linkto[1024+1];
	int size = 0;

	if ((size = Readlink(vf->fn, linkto, sizeof(linkto)-1)) == -1) {
	    VF_SET(res, READLINKFAIL);
	    VF_SET(res, LINKTO);
	} else {
	    linkto[size] = '\0';
	    if (vf->flink == NULL || strcmp(linkto, vf->flink))
		VF_SET(res, LINKTO);
	}
    }

    if (VF_ISSET(vf->vflags, FILESIZE)) {
	if (sb.st_size != vf->sb.st_size)
	    VF_SET(res, FILESIZE);
    }

    if (VF_ISSET(vf->vflags, MODE)) {
	/* XXX AIX has sizeof(mode_t) > sizeof(unsigned short) */
	unsigned short metamode = (unsigned short)vf->sb.st_mode;
	unsigned short filemode = (unsigned short)sb.st_mode;

	/* Comparing type of %ghost files is meaningless, but perms are OK. */
	if (FF_ISSET(vf->fflags, GHOST)) {
	    metamode &= ~0xf000;
	    filemode &= ~0xf000;
	}
	if (metamode != filemode)
	    VF_SET(res, MODE);
    }

    if (VF_ISSET(vf->vflags, RDEV)) {
	if (S_ISCHR(vf->sb.st_mode) != S_ISCHR(sb.st_mode)
	 || S_ISBLK(vf->sb.st_mode) != S_ISBLK(sb.st_mode))
	    VF_SET(res, RDEV);
	else if (S_ISDEV(vf->sb.st_mode) && S_ISDEV(sb.st_mode)) {
	    rpmuint16_t st_rdev = (rpmuint16_t)(sb.st_rdev & 0xffff);
	    rpmuint16_t frdev = (rpmuint16_t)(vf->sb.st_rdev & 0xffff);
	    if (st_rdev != frdev)
		VF_SET(res, RDEV);
	}
    }

    if (VF_ISSET(vf->vflags, MTIME)) {
	if (sb.st_mtime != vf->sb.st_mtime)
	    VF_SET(res, MTIME);
    }

    if (VF_ISSET(vf->vflags, USER)) {
	const char * fuser = uidToUname(sb.st_uid);
	if (fuser == NULL || vf->fuser == NULL || strcmp(fuser, vf->fuser))
	    VF_SET(res, USER);
    }

    if (VF_ISSET(vf->vflags, GROUP)) {
	const char * fgroup = gidToGname(sb.st_gid);
	if (fgroup == NULL || vf->fgroup == NULL || strcmp(fgroup, vf->fgroup))
	    VF_SET(res, GROUP);
    }

exit:

    if (spew) {	/* XXX no output w verify(...) probe. */
	char buf[BUFSIZ];
	char * t = buf;
	char * te = t;
	*te = '\0';
	if (ec) {
	    if (!(FF_ISSET(vf->fflags, MISSINGOK) ||FF_ISSET(vf->fflags, GHOST))
	     || rpmIsVerbose())
	    {
		sprintf(te, _("missing   %c %s"),
			(FF_ISSET(vf->fflags, CONFIG)	? 'c' :
			 FF_ISSET(vf->fflags, DOC)	? 'd' :
			 FF_ISSET(vf->fflags, GHOST)	? 'g' :
			 FF_ISSET(vf->fflags, LICENSE)	? 'l' :
			 FF_ISSET(vf->fflags, PUBKEY)	? 'P' :
			 FF_ISSET(vf->fflags, README)	? 'r' : ' '),
			vf->fn);
                if (VF_ISSET(res, LSTATFAIL) && errno != ENOENT) {
		    te += strlen(te);
                    sprintf(te, " (%s)", strerror(errno));
                }
	    }
	} else if (res || rpmIsVerbose()) {
	    /*@observer@*/ static const char aok[] = ".";
	    /*@observer@*/ static const char unknown[] = "?";

#define	_verify(_FLAG, _C)	\
	(VF_ISSET(res, _FLAG) ? _C : aok)
#define	_verifylink(_FLAG, _C)	\
	(VF_ISSET(res, READLINKFAIL) ? unknown : \
	 VF_ISSET(res, _FLAG) ? _C : aok)
#define	_verifyfile(_FLAG, _C)	\
	(VF_ISSET(res, READFAIL) ? unknown : \
	 VF_ISSET(res, _FLAG) ? _C : aok)
	
	    const char * digest = _verifyfile(FDIGEST, "5");
	    const char * size = _verify(FILESIZE, "S");
	    const char * link = _verifylink(LINKTO, "L");
	    const char * mtime = _verify(MTIME, "T");
	    const char * rdev = _verify(RDEV, "D");
	    const char * user = _verify(USER, "U");
	    const char * group = _verify(GROUP, "G");
	    const char * mode = _verify(MODE, "M");

#undef _verifyfile
#undef _verifylink
#undef _verify

	    sprintf(te, "%s%s%s%s%s%s%s%s  %c %s",
		    size, mode, digest, rdev, link, user, group, mtime,
			(FF_ISSET(vf->fflags, CONFIG)	? 'c' :
			 FF_ISSET(vf->fflags, DOC)	? 'd' :
			 FF_ISSET(vf->fflags, GHOST)	? 'g' :
			 FF_ISSET(vf->fflags, LICENSE)	? 'l' :
			 FF_ISSET(vf->fflags, PUBKEY)	? 'P' :
			 FF_ISSET(vf->fflags, README)	? 'r' : ' '),
			vf->fn);

	}

	if (t && *t)
	    rpmlog(RPMLOG_NOTICE, "%s\n", t);

    }

    return (res != 0);
}

/**
 * Return exit code from running verify script from header.
 * @todo malloc/free/refcount handling is fishy here.
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param fi		file info set
 * @param scriptFd      file handle to use for stderr (or NULL)
 * @return              0 on success
 */
static int rpmVerifyScript(/*@unused@*/ QVA_t qva, rpmts ts,
		rpmfi fi, /*@null@*/ FD_t scriptFd)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, fi, scriptFd, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmpsm psm;
    rpmRC rc;
    int ec = 0;

    if (scriptFd != NULL)
	rpmtsSetScriptFd(ts, scriptFd);

    psm = rpmpsmNew(ts, NULL, fi);

    rc = rpmpsmScriptStage(psm, RPMTAG_VERIFYSCRIPT, RPMTAG_VERIFYSCRIPTPROG);
    if (rc != RPMRC_OK)
	ec = 1;

    rc = rpmpsmScriptStage(psm, RPMTAG_SANITYCHECK, RPMTAG_SANITYCHECKPROG);
    if (rc != RPMRC_OK)
	ec = 1;

    psm = rpmpsmFree(psm, __FUNCTION__);

    if (scriptFd != NULL)
	rpmtsSetScriptFd(ts, NULL);

    return ec;
}

/**
 * Check installed package dependencies for problems.
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @param h		header
 * @return		number of problems found (0 for no problems)
 */
static int verifyDependencies(/*@unused@*/ QVA_t qva, rpmts ts,
		Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/
{
#ifdef	NOTYET
    uint32_t hdrNum = headerGetInstance(h);
#endif
    rpmps ps;
    int rc = 0;		/* assume no problems */
    int xx;

    rpmtsEmpty(ts);

#ifdef	NOTYET
    if (hdrNum > 0)
	(void) rpmtsAddEraseElement(ts, h, hdrNum);
    else
#endif
	(void) rpmtsAddInstallElement(ts, h, headerGetOrigin(h), 0, NULL);

    xx = rpmtsCheck(ts);
    ps = rpmtsProblems(ts);

    if (rpmpsNumProblems(ps) > 0) {
	const char * altNEVR;
	const char * pkgNEVR = NULL;
	rpmpsi psi;
	rpmProblem prob;
	char * t, * te;
	int nb = 512;

	psi = rpmpsInitIterator(ps);
	while (rpmpsNextIterator(psi) >= 0) {
	    prob = rpmpsProblem(psi);
	    if (pkgNEVR == NULL)
		pkgNEVR = rpmProblemGetPkgNEVR(prob);

	    altNEVR = rpmProblemGetAltNEVR(prob);
assert(altNEVR != NULL);
	    if (altNEVR[0] == 'R' && altNEVR[1] == ' ')
		nb += sizeof("\tRequires: ")-1;
	    if (altNEVR[0] == 'C' && altNEVR[1] == ' ')
		nb += sizeof("\tConflicts: ")-1;
	    nb += strlen(altNEVR+2) + sizeof("\n") - 1;

	}
	psi = rpmpsFreeIterator(psi);

	te = t = (char *) alloca(nb);
	*te = '\0';
	sprintf(te, _("Unsatisfied dependencies for %s:\n"), pkgNEVR);
	te += strlen(te);

	psi = rpmpsInitIterator(ps);
	while (rpmpsNextIterator(psi) >= 0) {
	    prob = rpmpsProblem(psi);

	    if ((altNEVR = rpmProblemGetAltNEVR(prob)) == NULL)
		altNEVR = "? ?altNEVR?";
	    if (altNEVR[0] == 'R' && altNEVR[1] == ' ')
		te = stpcpy(te, "\tRequires: ");
	    if (altNEVR[0] == 'C' && altNEVR[1] == ' ')
		te = stpcpy(te, "\tConflicts: ");
	    te = stpcpy( stpcpy(te, altNEVR+2), "\n");

	    rc++;
	}
	psi = rpmpsFreeIterator(psi);

	if (te > t) {
	    *te++ = '\n';
	    *te = '\0';
	    rpmlog(RPMLOG_NOTICE, "%s", t);
	    te = t;
	    *t = '\0';
	}
    }

    ps = rpmpsFree(ps);

    rpmtsEmpty(ts);

    return rc;
}

int showVerifyPackage(QVA_t qva, rpmts ts, Header h)
{
    static int scareMem = 0;
    rpmVerifyAttrs omitMask = (rpmVerifyAttrs)
	((qva->qva_flags & VERIFY_ATTRS) ^ VERIFY_ATTRS);
    int spew = (qva->qva_mode != 'v');	/* XXX no output w verify(...) probe. */
    int ec = 0;
    int i;
rpmfi fi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
uint32_t fc = rpmfiFC(fi);

  {
    /* Verify header digest/signature. */
    if (qva->qva_flags & (VERIFY_DIGEST | VERIFY_SIGNATURE))
    {
	const char * horigin = headerGetOrigin(h);
	const char * msg = NULL;
	size_t uhlen = 0;
	void * uh = headerUnload(h, &uhlen);
	int lvl = headerCheck(rpmtsDig(ts), uh, uhlen, &msg) == RPMRC_FAIL
		? RPMLOG_ERR : RPMLOG_DEBUG;
	rpmlog(lvl, "%s: %s\n",
		(horigin ? horigin : "verify"), (msg ? msg : ""));
	rpmtsCleanDig(ts);
	uh = _free(uh);
	msg = _free(msg);
    }

    /* Verify file digests. */
    if (fc > 0 && (qva->qva_flags & VERIFY_FILES))
#if defined(_OPENMP)
    #pragma omp parallel for private(i) reduction(+:ec)
#endif
    for (i = 0; i < (int)fc; i++) {
	int fflags = fi->fflags[i];
	rpmvf vf;
	int rc;

	/* If not querying %config, skip config files. */
	if (FF_ISSET(qva->qva_fflags, CONFIG) && FF_ISSET(fflags, CONFIG))
	    continue;

	/* If not querying %doc, skip doc files. */
	if (FF_ISSET(qva->qva_fflags, DOC) && FF_ISSET(fflags, DOC))
	    continue;

	/* If not verifying %ghost, skip ghost files. */
	if (!FF_ISSET(qva->qva_fflags, GHOST) && FF_ISSET(fflags, GHOST))
	    continue;

	/* Gather per-file data into a carrier. */
	vf = rpmvfNew(ts, fi, i, omitMask);

	/* Verify per-file metadata. */
	rc = rpmvfVerify(vf, spew);
	if (rc)
	    ec += rc;

	(void) rpmvfFree(vf);
	vf = NULL;
    }

    /* Run verify/sanity scripts (if any). */
    if (qva->qva_flags & VERIFY_SCRIPT)
    {
	int rc;

	if (headerIsEntry(h, RPMTAG_VERIFYSCRIPT) ||
	    headerIsEntry(h, RPMTAG_SANITYCHECK))
	{
	    FD_t fdo = fdDup(STDOUT_FILENO);

	    rc = rpmfiSetHeader(fi, h);
	    if ((rc = rpmVerifyScript(qva, ts, fi, fdo)) != 0)
		ec += rc;
	    if (fdo != NULL)
		rc = Fclose(fdo);
	    rc = rpmfiSetHeader(fi, NULL);
	}
    }

    /* Verify dependency assertions. */
    if (qva->qva_flags & VERIFY_DEPS)
    {
	int save_noise = _rpmds_unspecified_epoch_noise;
	int rc;

/*@-mods@*/
	if (rpmIsVerbose())
	    _rpmds_unspecified_epoch_noise = 1;
	if ((rc = verifyDependencies(qva, ts, h)) != 0)
	    ec += rc;
	_rpmds_unspecified_epoch_noise = save_noise;
/*@=mods@*/
    }
  }

    fi = rpmfiFree(fi);

    return ec;
}

int rpmcliVerify(rpmts ts, QVA_t qva, const char ** argv)
{
    rpmdepFlags depFlags = qva->depFlags, odepFlags;
    rpmtransFlags transFlags = qva->transFlags, otransFlags;
    rpmVSFlags vsflags, ovsflags;
    int ec = 0;

#if defined(_OPENMP)
(void) tagName((rpmTag)0);	/* XXX instantiate the tagname store. */
omp_set_nested(1);	/* XXX permit nested thread teams. */
#endif

    if (qva->qva_showPackage == NULL)
        qva->qva_showPackage = showVerifyPackage;

    /* XXX verify flags are inverted from query. */
    vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_verify}");
    vsflags = (rpmVSFlags) 0;	/* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
    if (!QVA_ISSET(qva->qva_flags, DIGEST)) {
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
    }
    if (!QVA_ISSET(qva->qva_flags, SIGNATURE)) {
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    }
    if (!QVA_ISSET(qva->qva_flags, HDRCHK)) {
	VSF_SET(vsflags, NOHDRCHK);
    }
    VSF_CLR(vsflags, NEEDPAYLOAD);
#endif

    odepFlags = rpmtsSetDFlags(ts, depFlags);
    otransFlags = rpmtsSetFlags(ts, transFlags);
    ovsflags = rpmtsSetVSFlags(ts, vsflags);
    ec = rpmcliArgIter(ts, qva, argv);
    vsflags = rpmtsSetVSFlags(ts, ovsflags);
    transFlags = rpmtsSetFlags(ts, otransFlags);
    depFlags = rpmtsSetDFlags(ts, odepFlags);

    if (qva->qva_showPackage == showVerifyPackage)
        qva->qva_showPackage = NULL;

    rpmtsEmpty(ts);

    return ec;
}
