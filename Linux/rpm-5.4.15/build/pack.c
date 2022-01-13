/** \ingroup rpmbuild
 * \file build/pack.c
 *  Assemble components of an RPM package.
 */

#include "system.h"

#include <rpmio_internal.h>	/* XXX fdGetFp, fdInitDigest, fdFiniDigest */
#include <rpmcb.h>
#include <argv.h>

#include <rpmtypes.h>
#include <rpmtag.h>

#include <pkgio.h>
#include "signature.h"		/* XXX rpmTempFile */

#define	_RPMFI_INTERNAL		/* XXX fi->fsm */
#define	_RPMEVR_INTERNAL	/* XXX RPMSENSE_ANY */
#define _RPMTAG_INTERNAL
#include <rpmbuild.h>

#include "rpmfi.h"
#include "fsm.h"

#include <rpmversion.h>
#include "buildio.h"

#include "debug.h"

/*@access rpmts @*/
/*@access rpmfi @*/	/* compared with NULL */
/*@access Header @*/	/* compared with NULL */
/*@access FD_t @*/	/* compared with NULL */
/*@access CSA_t @*/

/**
 * @todo Create transaction set *much* earlier.
 */
static rpmRC cpio_doio(FD_t fdo, /*@unused@*/ Header h, CSA_t csa,
		const char * payload_format, const char * fmodeMacro)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fdo, csa, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmts ts = rpmtsCreate();
    rpmfi fi = csa->fi;
    const char *failedFile = NULL;
    FD_t cfd;
    rpmRC rc = RPMRC_OK;
    int xx;

    {	const char *fmode = rpmExpand(fmodeMacro, NULL);
	if (!(fmode && fmode[0] == 'w')) {
	    fmode = _free(fmode);
	    fmode = xstrdup("w9.gzdio");
	}
	/*@-nullpass@*/
	(void) Fflush(fdo);
	cfd = Fdopen(fdDup(Fileno(fdo)), fmode);
	/*@=nullpass@*/
	fmode = _free(fmode);
    }
    if (cfd == NULL)
	return RPMRC_FAIL;

    xx = fsmSetup(fi->fsm, IOSM_PKGBUILD, payload_format, ts, fi, cfd,
		&csa->cpioArchiveSize, &failedFile);
    if (xx)
	rc = RPMRC_FAIL;
    (void) Fclose(cfd);
    xx = fsmTeardown(fi->fsm);
    if (rc == RPMRC_OK && xx) rc = RPMRC_FAIL;

    if (rc) {
	const char * msg = iosmStrerror(rc);
	if (failedFile)
	    rpmlog(RPMLOG_ERR, _("create archive failed on file %s: %s\n"),
		failedFile, msg);
	else
	    rpmlog(RPMLOG_ERR, _("create archive failed: %s\n"), msg);
	msg = _free(msg);
	rc = RPMRC_FAIL;
    }

    failedFile = _free(failedFile);
    (void)rpmtsFree(ts); 
    ts = NULL;

    return rc;
}

/**
 */
static rpmRC cpio_copy(FD_t fdo, CSA_t csa)
	/*@globals fileSystem, internalState @*/
	/*@modifies fdo, csa, fileSystem, internalState @*/
{
    char buf[BUFSIZ];
    size_t nb;

    while((nb = Fread(buf, sizeof(buf[0]), sizeof(buf), csa->cpioFdIn)) > 0) {
	if (Fwrite(buf, sizeof(buf[0]), nb, fdo) != nb) {
	    rpmlog(RPMLOG_ERR, _("cpio_copy write failed: %s\n"),
			Fstrerror(fdo));
	    return RPMRC_FAIL;
	}
	csa->cpioArchiveSize += nb;
    }
    if (Ferror(csa->cpioFdIn)) {
	rpmlog(RPMLOG_ERR, _("cpio_copy read failed: %s\n"),
		Fstrerror(csa->cpioFdIn));
	return RPMRC_FAIL;
    }
    return RPMRC_OK;
}

/**
 */
static /*@only@*/ /*@null@*/ rpmiob addFileToTagAux(Spec spec,
		const char * file, /*@only@*/ rpmiob iob)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    char buf[BUFSIZ];
    const char * fn = buf;
    FILE * f;
    FD_t fd;

    fn = rpmGetPath("%{_builddir}/%{?buildsubdir:%{buildsubdir}/}", file, NULL);

    fd = Fopen(fn, "r.fdio");
    if (fn != buf) fn = _free(fn);
    if (fd == NULL || Ferror(fd)) {
	iob = rpmiobFree(iob);
	return NULL;
    }
    /*@-type@*/ /* FIX: cast? */
    if ((f = fdGetFp(fd)) != NULL)
    /*@=type@*/
    while (fgets(buf, (int)sizeof(buf), f)) {
	/* XXX display fn in error msg */
	if (expandMacros(spec, spec->macros, buf, sizeof(buf))) {
	    rpmlog(RPMLOG_ERR, _("line: %s\n"), buf);
	    iob = rpmiobFree(iob);
	    break;
	}
	iob = rpmiobAppend(iob, buf, 0);
    }
    (void) Fclose(fd);

    return iob;
}

/**
 */
static int addFileToTag(Spec spec, const char * file, Header h, rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmiob iob = rpmiobNew(0);
    int xx;

    he->tag = tag;
    xx = headerGet(h, he, 0);
    if (xx) {
	iob = rpmiobAppend(iob, he->p.str, 1);
	xx = headerDel(h, he, 0);
    }
    he->p.ptr = _free(he->p.ptr);

    if ((iob = addFileToTagAux(spec, file, iob)) == NULL)
	return 1;
    
    he->tag = tag;
    he->t = RPM_STRING_TYPE;
    he->p.str = rpmiobStr(iob);
    he->c = 1;
    xx = headerPut(h, he, 0);

    iob = rpmiobFree(iob);
    return 0;
}

/**
 */
static int addFileToArrayTag(Spec spec, const char *file, Header h, rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext, fileSystem, internalState  @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmiob iob = rpmiobNew(0);
    const char *s;
    int xx;

    if ((iob = addFileToTagAux(spec, file, iob)) == NULL)
	return 1;

    s = rpmiobStr(iob);

    he->tag = tag;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &s;
    he->c = 1;
    he->append = 1;
    xx = headerPut(h, he, 0);
    he->append = 0;

    iob = rpmiobFree(iob);
    return 0;
}

rpmRC processScriptFiles(Spec spec, Package pkg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies pkg->header, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct TriggerFileEntry *p;
    int xx;
    
    if (pkg->preInFile) {
	if (addFileToTag(spec, pkg->preInFile, pkg->header, RPMTAG_PREIN)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PreIn file: %s\n"), pkg->preInFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->preUnFile) {
	if (addFileToTag(spec, pkg->preUnFile, pkg->header, RPMTAG_PREUN)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PreUn file: %s\n"), pkg->preUnFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->preTransFile) {
	if (addFileToTag(spec, pkg->preTransFile, pkg->header, RPMTAG_PRETRANS)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PreTrans file: %s\n"), pkg->preTransFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->postInFile) {
	if (addFileToTag(spec, pkg->postInFile, pkg->header, RPMTAG_POSTIN)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PostIn file: %s\n"), pkg->postInFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->postUnFile) {
	if (addFileToTag(spec, pkg->postUnFile, pkg->header, RPMTAG_POSTUN)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PostUn file: %s\n"), pkg->postUnFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->postTransFile) {
	if (addFileToTag(spec, pkg->postTransFile, pkg->header, RPMTAG_POSTTRANS)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open PostTrans file: %s\n"), pkg->postTransFile);
	    return RPMRC_FAIL;
	}
    }
    if (pkg->verifyFile) {
	if (addFileToTag(spec, pkg->verifyFile, pkg->header,
			 RPMTAG_VERIFYSCRIPT)) {
	    rpmlog(RPMLOG_ERR,
		     _("Could not open VerifyScript file: %s\n"), pkg->verifyFile);
	    return RPMRC_FAIL;
	}
    }

    if (pkg->sanityCheckFile) {
        if (addFileToTag(spec, pkg->sanityCheckFile, pkg->header, RPMTAG_SANITYCHECK)) {
            rpmlog(RPMLOG_ERR, _("Could not open Test file: %s\n"), pkg->sanityCheckFile);
            return RPMRC_FAIL;
        }
    }

    for (p = pkg->triggerFiles; p != NULL; p = p->next) {
	he->tag = RPMTAG_TRIGGERSCRIPTPROG;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = (const char **)&p->prog;	/* XXX NOCAST */
	he->c = 1;
	he->append = 1;
	xx = headerPut(pkg->header, he, 0);
	he->append = 0;
	if (p->script) {
	    he->tag = RPMTAG_TRIGGERSCRIPTS;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = (const char **)&p->script;	/* XXX NOCAST */
	    he->c = 1;
	    he->append = 1;
	    xx = headerPut(pkg->header, he, 0);
	    he->append = 0;
	} else if (p->fileName) {
	    if (addFileToArrayTag(spec, p->fileName, pkg->header,
				  RPMTAG_TRIGGERSCRIPTS)) {
		rpmlog(RPMLOG_ERR,
			 _("Could not open Trigger script file: %s\n"),
			 p->fileName);
		return RPMRC_FAIL;
	    }
	} else {
	    /*@observer@*/
	    static const char *bull = "";
	    he->tag = RPMTAG_TRIGGERSCRIPTS;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = &bull;
	    he->c = 1;
	    he->append = 1;
	    xx = headerPut(pkg->header, he, 0);
	    he->append = 0;
	}
    }

    return RPMRC_OK;
}

#if defined(DEAD)
int readRPM(const char *fileName, Spec *specp, void * l,
		Header *sigs, CSA_t csa)
{
    const char * msg = "";
    FD_t fdi;
    Spec spec;
    rpmRC rc;

    fdi = (fileName != NULL)
	? Fopen(fileName, "r.fdio")
	: fdDup(STDIN_FILENO);

    if (fdi == NULL || Ferror(fdi)) {
	rpmlog(RPMLOG_ERR, _("readRPM: open %s: %s\n"),
		(fileName ? fileName : "<stdin>"),
		Fstrerror(fdi));
	if (fdi) (void) Fclose(fdi);
	return RPMRC_FAIL;
    }

    {	const char item[] = "Lead";
	size_t nl = rpmpkgSizeof(item, NULL);

	if (nl == 0) {
	    rc = RPMRC_FAIL;
	    msg = xstrdup("item size is zero");
	} else {
	    l = xcalloc(1, nl);		/* XXX memory leak */
	    msg = NULL;
	    rc = rpmpkgRead(item, fdi, l, &msg);
	}
    }

    if (rc != RPMRC_OK) {
	rpmlog(RPMLOG_ERR, _("readRPM: read %s: %s\n"),
		(fileName ? fileName : "<stdin>"), msg);
	msg = _free(msg);
	return RPMRC_FAIL;
    }
    msg = _free(msg);
    /*@=sizeoftype@*/

    /* XXX FIXME: EPIPE on <stdin> */
    if (Fseek(fdi, 0, SEEK_SET) == -1) {
	rpmlog(RPMLOG_ERR, _("%s: Fseek failed: %s\n"),
			(fileName ? fileName : "<stdin>"), Fstrerror(fdi));
	return RPMRC_FAIL;
    }

    /* Reallocate build data structures */
    spec = newSpec();
    spec->packages = newPackage(spec);

    /* XXX the header just allocated will be allocated again */
    (void)headerFree(spec->packages->header);
    spec->packages->header = NULL;

    /* Read the rpm lead, signatures, and header */
    {	rpmts ts = rpmtsCreate();

	/* XXX W2DO? pass fileName? */
	/*@-mustmod@*/      /* LCL: segfault */
	rc = rpmReadPackageFile(ts, fdi, "readRPM",
			 &spec->packages->header);
	/*@=mustmod@*/

	(void)rpmtsFree(ts); 
	ts = NULL;

	if (sigs) *sigs = NULL;			/* XXX HACK */
    }

    switch (rc) {
    case RPMRC_OK:
    case RPMRC_NOKEY:
    case RPMRC_NOTTRUSTED:
	break;
    case RPMRC_NOTFOUND:
	rpmlog(RPMLOG_ERR, _("readRPM: %s is not an RPM package\n"),
		(fileName ? fileName : "<stdin>"));
	return RPMRC_FAIL;
    case RPMRC_FAIL:
    default:
	rpmlog(RPMLOG_ERR, _("readRPM: reading header from %s\n"),
		(fileName ? fileName : "<stdin>"));
	return RPMRC_FAIL;
	/*@notreached@*/ break;
    }

    if (specp)
	*specp = spec;
    else
	spec = freeSpec(spec);

    if (csa != NULL)
	csa->cpioFdIn = fdi;
    else
	(void) Fclose(fdi);

    return 0;
}
#endif

#if defined(DEAD)
#define	RPMPKGVERSION_MIN	30004
#define	RPMPKGVERSION_MAX	40003
/*@unchecked@*/
static int rpmpkg_version = -1;

static int rpmLeadVersion(void)
	/*@globals rpmpkg_version, rpmGlobalMacroContext, h_errno @*/
	/*@modifies rpmpkg_version, rpmGlobalMacroContext @*/
{
    int rpmlead_version;

    /* Intitialize packaging version from macro configuration. */
    if (rpmpkg_version < 0) {
	rpmpkg_version = rpmExpandNumeric("%{_package_version}");
	if (rpmpkg_version < RPMPKGVERSION_MIN)
	    rpmpkg_version = RPMPKGVERSION_MIN;
	if (rpmpkg_version > RPMPKGVERSION_MAX)
	    rpmpkg_version = RPMPKGVERSION_MAX;
    }

    rpmlead_version = rpmpkg_version / 10000;
    /* XXX silly sanity check. */
    if (rpmlead_version < 3 || rpmlead_version > 4)
	rpmlead_version = 3;
    return rpmlead_version;
}
#endif

void providePackageNVR(Header h)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char *N, *V, *R;
#ifdef RPM_VENDOR_MANDRIVA
    const char *D;
    int gotD;
#endif
    rpmuint32_t E;
    int gotE;
    const char *pEVR;
    char *p;
    rpmuint32_t pFlags = RPMSENSE_EQUAL;
    const char ** provides = NULL;
    const char ** providesEVR = NULL;
    rpmuint32_t * provideFlags = NULL;
    int providesCount;
    int bingo = 1;
    size_t nb;
    int xx;
    int i;

    /* Generate provides for this package N-V-R. */
    xx = headerNEVRA(h, &N, NULL, &V, &R, NULL);
    if (!(N && V && R)) {
	N = _free(N);
	V = _free(V);
	R = _free(R);
	return;
    }

    nb = 21 + strlen(V) + 1 + strlen(R) + 1;
#ifdef	RPM_VENDOR_MANDRIVA
    he->tag = RPMTAG_DISTEPOCH;
    gotD = headerGet(h, he, 0);
    D = (he->p.str ? he->p.str : NULL);
    nb += (gotD ? strlen(D) + 1 : 0);
#endif
    pEVR = p = alloca(nb);
    *p = '\0';
    he->tag = RPMTAG_EPOCH;
    gotE = headerGet(h, he, 0);
    E = (he->p.ui32p ? he->p.ui32p[0] : 0);
    he->p.ptr = _free(he->p.ptr);
    if (gotE) {
	sprintf(p, "%d:", E);
	p += strlen(p);
    }
    p = stpcpy( stpcpy( stpcpy(p, V) , "-") , R);
#ifdef	RPM_VENDOR_MANDRIVA
    if (gotD) {
	p = stpcpy( stpcpy( p, ":"), D);
    	D = _free(D);
    }
#endif
    V = _free(V);
    R = _free(R);

    /*
     * Rpm prior to 3.0.3 does not have versioned provides.
     * If no provides at all are available, we can just add.
     */
    he->tag = RPMTAG_PROVIDENAME;
/*@-nullstate@*/
    xx = headerGet(h, he, 0);
/*@=nullstate@*/
    provides = he->p.argv;
    providesCount = he->c;
    if (!xx)
	goto exit;

    /*
     * Otherwise, fill in entries on legacy packages.
     */
    he->tag = RPMTAG_PROVIDEVERSION;
/*@-nullstate@*/
    xx = headerGet(h, he, 0);
/*@=nullstate@*/
    providesEVR = he->p.argv;
    if (!xx) {
	for (i = 0; i < providesCount; i++) {
	    /*@observer@*/
	    static const char * vdummy = "";
	    static rpmsenseFlags fdummy = RPMSENSE_ANY;

	    he->tag = RPMTAG_PROVIDEVERSION;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = &vdummy;
	    he->c = 1;
	    he->append = 1;
/*@-nullstate@*/
	    xx = headerPut(h, he, 0);
/*@=nullstate@*/
	    he->append = 0;

	    he->tag = RPMTAG_PROVIDEFLAGS;
	    he->t = RPM_UINT32_TYPE;
	    he->p.ui32p = (void *) &fdummy;
	    he->c = 1;
	    he->append = 1;
/*@-nullstate@*/
	    xx = headerPut(h, he, 0);
/*@=nullstate@*/
	    he->append = 0;
	}
	goto exit;
    }

    he->tag = RPMTAG_PROVIDEFLAGS;
/*@-nullstate@*/
    xx = headerGet(h, he, 0);
/*@=nullstate@*/
    provideFlags = he->p.ui32p;

    /*@-nullderef@*/	/* LCL: providesEVR is not NULL */
    if (provides && providesEVR && provideFlags)
    for (i = 0; i < providesCount; i++) {
        if (!(provides[i] && providesEVR[i]))
            continue;
	if (!(provideFlags[i] == RPMSENSE_EQUAL &&
	    !strcmp(N, provides[i]) && !strcmp(pEVR, providesEVR[i])))
	    continue;
	bingo = 0;
	break;
    }
    /*@=nullderef@*/

exit:
/*@-usereleased@*/
    provides = _free(provides);
    providesEVR = _free(providesEVR);
    provideFlags = _free(provideFlags);
/*@=usereleased@*/

    if (bingo) {
	he->tag = RPMTAG_PROVIDENAME;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &N;
	he->c = 1;
	he->append = 1;
/*@-nullstate@*/
	xx = headerPut(h, he, 0);
/*@=nullstate@*/
	he->append = 0;

	he->tag = RPMTAG_PROVIDEVERSION;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &pEVR;
	he->c = 1;
	he->append = 1;
/*@-nullstate@*/
	xx = headerPut(h, he, 0);
/*@=nullstate@*/
	he->append = 0;

	he->tag = RPMTAG_PROVIDEFLAGS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &pFlags;
	he->c = 1;
	he->append = 1;
/*@-nullstate@*/
	xx = headerPut(h, he, 0);
/*@=nullstate@*/
	he->append = 0;
    }
    N = _free(N);
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static
unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char) (c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return (unsigned char) '\0';
}

rpmRC writeRPM(Header *hdrp, unsigned char ** pkgidp, const char * fn,
		CSA_t csa, char * passPhrase, const char ** cookie, void * _dig)

{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    FD_t fd = NULL;
    FD_t ifd = NULL;
pgpDig dig = _dig;
    rpmuint32_t sigtag;
    const char * sigtarget = NULL;	/* XXX coverity #1035966 */
    const char * rpmio_flags = NULL;
    const char * payload_format = NULL;
    const char * SHA1 = NULL;
    const char * msg = NULL;
    char * s;
    char buf[BUFSIZ];
    Header h;
    Header sigh = NULL;
    int addsig = 0;
    int isSource;
    rpmRC rc = RPMRC_OK;
    size_t nbr;
    size_t nbw;
    int xx;

    /* Transfer header reference form *hdrp to h. */
    h = headerLink(*hdrp);

    (void)headerFree(*hdrp);
    *hdrp = NULL;

    if (pkgidp)
	*pkgidp = NULL;

    /* Save payload information */
    isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);
    if (isSource) {
	payload_format = rpmExpand("%{?_source_payload_format}", NULL);
	rpmio_flags = rpmExpand("%{?_source_payload}", NULL);
    } else {
	payload_format = rpmExpand("%{?_binary_payload_format}", NULL);
	rpmio_flags = rpmExpand("%{?_binary_payload}", NULL);
    }

    if (!(payload_format && *payload_format)) {
	payload_format = _free(payload_format);
	payload_format = xstrdup("cpio");
    }
    if (!(rpmio_flags && *rpmio_flags)) {
	rpmio_flags = _free(rpmio_flags);
	rpmio_flags = xstrdup("w9.gzdio");
    }
    s = strchr(rpmio_flags, '.');
    if (s) {

	if (payload_format) {
	    if (!strcmp(payload_format, "tar")
	     || !strcmp(payload_format, "ustar")) {
		/* XXX addition to header is too late to be displayed/sorted. */
		/* Add prereq on rpm version that understands tar payloads */
		(void) rpmlibNeedsFeature(h, "PayloadIsUstar", "4.4.4-1");
	    }
#if defined(SUPPORT_AR_PAYLOADS)
	    if (!strcmp(payload_format, "ar")) {
		/* XXX addition to header is too late to be displayed/sorted. */
		/* Add prereq on rpm version that understands tar payloads */
		(void) rpmlibNeedsFeature(h, "PayloadIsAr", "5.1-1");
	    }
#endif

	    he->tag = RPMTAG_PAYLOADFORMAT;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = payload_format;
	    he->c = 1;
	    xx = headerPut(h, he, 0);
	}

	/* XXX addition to header is too late to be displayed/sorted. */
	if (s[1] == 'g' && s[2] == 'z') {
	    he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = xstrdup("gzip");
	    he->c = 1;
	    xx = headerPut(h, he, 0);
	    he->p.ptr = _free(he->p.ptr);
	} else if (s[1] == 'b' && s[2] == 'z') {
	    he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = xstrdup("bzip2");
	    he->c = 1;
	    xx = headerPut(h, he, 0);
	    he->p.ptr = _free(he->p.ptr);
	} else if (s[1] == 'l' && s[2] == 'z') {
	    he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = xstrdup("lzma");
	    he->c = 1;
	    xx = headerPut(h, he, 0);
	    he->p.ptr = _free(he->p.ptr);
	    (void) rpmlibNeedsFeature(h, "PayloadIsLzma", "4.4.6-1");
	} else if (s[1] == 'x' && s[2] == 'z') {
	    he->tag = RPMTAG_PAYLOADCOMPRESSOR;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = xstrdup("xz");
	    he->c = 1;
	    xx = headerPut(h, he, 0);
	    he->p.ptr = _free(he->p.ptr);
	    (void) rpmlibNeedsFeature(h, "PayloadIsXz", "5.2-1");
	}
	strncpy(buf, rpmio_flags, sizeof(buf)-1);
	buf[s - rpmio_flags] = '\0';

	he->tag = RPMTAG_PAYLOADFLAGS;
	he->t = RPM_STRING_TYPE;
	he->p.str = buf+1;
	he->c = 1;
	xx = headerPut(h, he, 0);
    }
    s = NULL;

    /* Create and add the cookie */
    if (cookie) {
	sprintf(buf, "%s %u", buildHost(), (unsigned) (*getBuildTime()));
	*cookie = xstrdup(buf);		/* XXX memory leak */
	he->tag = RPMTAG_COOKIE;
	he->t = RPM_STRING_TYPE;
	he->p.str = *cookie;
	he->c = 1;
	xx = headerPut(h, he, 0);
    }

    /* Add the non-repudiable public key (skip if --sign was specified) */
    if (!addsig && dig && dig->pub && dig->publen > 0) {
	rpmuint8_t atype = PGPARMOR_PUBKEY;
	s = pgpArmorWrap(atype, dig->pub, dig->publen);
assert(s);
	he->tag = (rpmTag) RPMTAG_PUBKEYS;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = (const char **) &s;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
	s = _free(s);
    }

    /* Reallocate the header into one contiguous region. */
    h = headerReload(h, RPMTAG_HEADERIMMUTABLE);
    if (h == NULL) {	/* XXX can't happen */
	rpmlog(RPMLOG_ERR, _("Unable to create immutable header region.\n"));
	rc = RPMRC_FAIL;
	goto exit;
    }
    /* Re-reference reallocated header. */
    *hdrp = headerLink(h);

    /*
     * Write the header+archive into a temp file so that the size of
     * archive (after compression) can be added to the header.
     */
    sigtarget = NULL;
    if (rpmTempFile(NULL, &sigtarget, &fd)) {
	rpmlog(RPMLOG_ERR, _("Unable to open temp file.\n"));
	rc = RPMRC_FAIL;
	goto exit;
    }

pgpDigParams sigp = pgpGetSignature(dig);
pgpDigParams pubp = pgpGetPubkey(dig);

    /* Write the header to a temp file, computing header SHA1 on the fly. */
    fdInitDigest(fd, PGPHASHALGO_SHA1, 0);
assert(fd->ndigests == 1);
    if (sigp->hash_algo != PGPHASHALGO_SHA1) {
	fdInitDigest(fd, sigp->hash_algo, 0);
assert(fd->ndigests == 2);
    }
    {	const char item[] = "Header";
	msg = NULL;
	rc = rpmpkgWrite(item, fd, h, &msg);
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", sigtarget, item,
		(msg && *msg ? msg : "write failed\n"));
	    msg = _free(msg);
	    rc = RPMRC_FAIL;
	    goto exit;
	}
	msg = _free(msg);
	(void) Fflush(fd);
    }

    {	/* XXX Dupe the header digest for the non-repudiable signature. */
	DIGEST_CTX ctx = (dig ? rpmDigestDup(fd->digests[fd->ndigests-1]) : NULL);

	/* Finalize the header signature digest. */
	/* XXX FIXME: get binary octets, not ASCII. */
	fdFiniDigest(fd, PGPHASHALGO_SHA1, &SHA1, NULL, 1);

sigp->signhash16[0] = (rpmuint8_t) (nibble(SHA1[0]) << 4) | nibble(SHA1[1]);
sigp->signhash16[1] = (rpmuint8_t) (nibble(SHA1[2]) << 4) | nibble(SHA1[3]);

	/* Sign the header SHA1. */
	if (ctx)
	    xx = pgpExportSignature(dig, ctx);

    }

    /* Append the payload to the temp file. */
    if (csa->fi != NULL)
	rc = cpio_doio(fd, h, csa, payload_format, rpmio_flags);
    else if (Fileno(csa->cpioFdIn) >= 0)
	rc = cpio_copy(fd, csa);
    else
assert(0);

    if (rc != RPMRC_OK)
	goto exit;

    (void) Fclose(fd);
    fd = NULL;
    (void) Unlink(fn);

    /* Generate the signature */
    (void) fflush(stdout);
    sigh = headerNew();
    (void) rpmAddSignature(sigh, sigtarget, RPMSIGTAG_SIZE, passPhrase);
    (void) rpmAddSignature(sigh, sigtarget, RPMSIGTAG_MD5, passPhrase);

    sigtag = RPMSIGTAG_GPG;
    addsig = (passPhrase && passPhrase[0]);

    if (addsig) {
	rpmlog(RPMLOG_NOTICE, _("Generating signature: %d\n"), sigtag);
	(void) rpmAddSignature(sigh, sigtarget, sigtag, passPhrase);
    }
    else if (dig && dig->sig && dig->siglen > 0) {
	switch (pubp->pubkey_algo) {
	default:
assert(0);
	    break;
	case PGPPUBKEYALGO_RSA:
	    he->tag = (rpmTag) RPMSIGTAG_RSA;
	    break;
	case PGPPUBKEYALGO_DSA:
	    he->tag = (rpmTag) RPMSIGTAG_DSA;
	    break;
	case PGPPUBKEYALGO_ECDSA:
	    he->tag = (rpmTag) RPMSIGTAG_ECDSA;
	    break;
	}
	he->t = RPM_BIN_TYPE;
	he->p.ptr = (void *) dig->sig;
	he->c = dig->siglen;
	xx = headerPut(sigh, he, 0);
	dig->sig = _free(dig->sig);		/* XXX lazily instead? */
	dig->siglen = 0;
    }
    
    if (SHA1) {
	he->tag = (rpmTag) RPMSIGTAG_SHA1;
	he->t = RPM_STRING_TYPE;
	he->p.str = SHA1;
	he->c = 1;
	xx = headerPut(sigh, he, 0);
	SHA1 = _free(SHA1);
    }

    {	rpmuint32_t payloadSize = csa->cpioArchiveSize;
	he->tag = (rpmTag) RPMSIGTAG_PAYLOADSIZE;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &payloadSize;
	he->c = 1;
	xx = headerPut(sigh, he, 0);
    }

    /* Reallocate the signature header into one contiguous region. */
    sigh = headerReload(sigh, RPMTAG_HEADERSIGNATURES);
    if (sigh == NULL) {	/* XXX can't happen */
	rpmlog(RPMLOG_ERR, _("Unable to reload signature header.\n"));
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* Pad the signature header to put the metadata header at known offset. */
    {	size_t slen = 0;
	void * uh = headerUnload(sigh, &slen);
	static const size_t align = 1024;
	size_t nb = align - 96 - 16 - 16;
	rpmuint8_t * b;

	uh = _free(uh);
assert(slen < nb);
	nb -= slen;
	b = memset(alloca(nb), 0, nb);
	he->tag = (rpmTag) RPMSIGTAG_PADDING;
	he->t = RPM_BIN_TYPE;
	he->p.ui8p = b;
	he->c = nb;
	xx = headerPut(sigh, he, 0);
	sigh = headerReload(sigh, RPMTAG_HEADERSIGNATURES);
assert(sigh != NULL);
    }

    /* Open the output file */
    fd = Fopen(fn, "w.fdio");
    if (fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("Could not open %s: %s\n"),
		fn, Fstrerror(fd));
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* Write the lead section into the package. */
    {	const char item[] = "Lead";
	size_t nl = rpmpkgSizeof(item, NULL);

	msg = NULL;
	if (nl == 0)
	    rc = RPMRC_FAIL;
	else {
	    void * l = memset(alloca(nl), 0, nl);
	    const char *N, *V, *R;
	    (void) headerNEVRA(h, &N, NULL, &V, &R, NULL);
	    sprintf(buf, "%s-%s-%s", N, V, R);
	    N = _free(N);
	    V = _free(V);
	    R = _free(R);
	    msg = buf;
	    rc = rpmpkgWrite(item, fd, l, &msg);
	}

	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, _("Unable to write package: %s\n"),
		 Fstrerror(fd));
	    rc = RPMRC_FAIL;
	    goto exit;
	}
    }

    /* Write the signature section into the package. */
    {	const char item[] = "Signature";

	msg = NULL;
	rc = rpmpkgWrite(item, fd, sigh, &msg);
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item,
                (msg && *msg ? msg : "write failed\n"));
	    msg = _free(msg);
	    rc = RPMRC_FAIL;
	    goto exit;
	}
	msg = _free(msg);
    }

    /* Append the header and archive */
    ifd = Fopen(sigtarget, "r.fdio");
    if (ifd == NULL || Ferror(ifd)) {
	rpmlog(RPMLOG_ERR, _("Unable to open sigtarget %s: %s\n"),
		sigtarget, Fstrerror(ifd));
	rc = RPMRC_FAIL;
	goto exit;
    }

    /* Add signatures to header, and write header into the package. */
    {	const char item[] = "Header";
	Header nh = NULL;

	msg = NULL;
	rc = rpmpkgRead(item, ifd, &nh, &msg);
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", sigtarget, item,
                (msg && *msg ? msg : "read failed\n"));
	    msg = _free(msg);
	    rc = RPMRC_FAIL;
	    goto exit;
	}
	msg = _free(msg);

#ifdef	NOTYET
	(void) headerMergeLegacySigs(nh, sigh);
#endif

	msg = NULL;
	rc = rpmpkgWrite(item, fd, nh, &msg);
	(void)headerFree(nh);
	nh = NULL;
	if (rc != RPMRC_OK) {
	    rpmlog(RPMLOG_ERR, "%s: %s: %s\n", fn, item,
                (msg && *msg ? msg : "write failed\n"));
	    msg = _free(msg);
	    rc = RPMRC_FAIL;
	    goto exit;
	}
	msg = _free(msg);
    }
	
    /* Write the payload into the package. */
    while ((nbr = Fread(buf, sizeof(buf[0]), sizeof(buf), ifd)) > 0) {
	if (Ferror(ifd)) {
	    rpmlog(RPMLOG_ERR, _("Unable to read payload from %s: %s\n"),
		     sigtarget, Fstrerror(ifd));
	    rc = RPMRC_FAIL;
	    goto exit;
	}
	nbw = (int)Fwrite(buf, sizeof(buf[0]), nbr, fd);
	if (nbr != nbw || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("Unable to write payload to %s: %s\n"),
		     fn, Fstrerror(fd));
	    rc = RPMRC_FAIL;
	    goto exit;
	}
    }
    rc = RPMRC_OK;

exit:
    SHA1 = _free(SHA1);
    rpmio_flags = _free(rpmio_flags);
    payload_format = _free(payload_format);
    (void)headerFree(h);
    h = NULL;

    /* XXX Fish the pkgid out of the signature header. */
    if (sigh != NULL && pkgidp != NULL) {
	he->tag = (rpmTag) RPMSIGTAG_MD5;
	xx = headerGet(sigh, he, 0);
	if (he->t == RPM_BIN_TYPE && he->p.ptr != NULL && he->c == 16)
	    *pkgidp = he->p.ui8p;		/* XXX memory leak */
    }

    (void)headerFree(sigh);
    sigh = NULL;
    if (ifd) {
	(void) Fclose(ifd);
	ifd = NULL;
    }
    if (fd) {
	(void) Fclose(fd);
	fd = NULL;
    }
    if (sigtarget) {
	(void) Unlink(sigtarget);
	sigtarget = _free(sigtarget);
    }

    if (rc == RPMRC_OK)
	rpmlog(RPMLOG_NOTICE, _("Wrote: %s\n"), fn);
    else
	(void) Unlink(fn);

    return rc;
}

static int rpmlibMarkers(Header h)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t val;
    int xx;

    he->tag = RPMTAG_RPMVERSION;
    he->t = RPM_STRING_TYPE;
    he->p.str = xstrdup(VERSION);
    he->c = 1;
    xx = headerPut(h, he, 0);
    he->p.ptr = _free(he->p.ptr);

  if (!(_rpmbuildFlags & 4)) {
    val = (rpmuint32_t)rpmlibTimestamp();
    he->tag = RPMTAG_RPMLIBTIMESTAMP;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &val;
    he->c = 1;
    xx = headerPut(h, he, 0);

    val = (rpmuint32_t)rpmlibVendor();
    he->tag = RPMTAG_RPMLIBVENDOR;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &val;
    he->c = 1;
    xx = headerPut(h, he, 0);

    val = (rpmuint32_t)rpmlibVersion();
    he->tag = RPMTAG_RPMLIBVERSION;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = &val;
    he->c = 1;
    xx = headerPut(h, he, 0);
  }

    he->tag = RPMTAG_BUILDHOST;
    he->t = RPM_STRING_TYPE;
    he->p.str = buildHost();
    he->c = 1;
    xx = headerPut(h, he, 0);

    he->tag = RPMTAG_BUILDTIME;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = getBuildTime();
    he->c = 1;
    xx = headerPut(h, he, 0);

    return 0;
}

/*@unchecked@*/
static rpmTag copyTags[] = {
    RPMTAG_CHANGELOGTIME,
    RPMTAG_CHANGELOGNAME,
    RPMTAG_CHANGELOGTEXT,
    0
};

static rpmRC checkPackages(const char *pkgcheck)
{
    int fail = rpmExpandNumeric("%{?_nonzero_exit_pkgcheck_terminate_build}");
    int xx;
    
    rpmlog(RPMLOG_NOTICE, _("Executing \"%s\":\n"), pkgcheck);
    /* XXX FIXME: use popen(3) through %(...) instead. */
    xx = system(pkgcheck);
    if (WEXITSTATUS(xx) == -1 || WEXITSTATUS(xx) == 127) {
	rpmlog(RPMLOG_ERR, _("Execution of \"%s\" failed.\n"), pkgcheck);
	if (fail) return 127;
    }
    if (WEXITSTATUS(xx) != 0) {
	rpmlog(RPMLOG_ERR, _("Package check \"%s\" failed.\n"), pkgcheck);
	if (fail) return RPMRC_FAIL;
    }
    
    return RPMRC_OK;
}

rpmRC packageBinaries(Spec spec)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct cpioSourceArchive_s csabuf;
    CSA_t csa = &csabuf;
    const char *errorString;
    Package pkg;
    rpmRC rc = RPMRC_OK;	/* assume success */
    int xx;
    ARGV_t pkglist = NULL;

    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
	const char *fn;

	if (pkg->fileList == NULL)
	    continue;

	if (spec->cookie) {
	    he->tag = RPMTAG_COOKIE;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = spec->cookie;
	    he->c = 1;
	    xx = headerPut(pkg->header, he, 0);
	}

	/* Copy changelog from src rpm */
	headerCopyTags(spec->packages->header, pkg->header, copyTags);

	/* Add rpmlib markers for tracking. */
	(void) rpmlibMarkers(pkg->header);
	
	he->tag = RPMTAG_OPTFLAGS;
	he->t = RPM_STRING_TYPE;
	he->p.str = rpmExpand("%{optflags}", NULL);
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);
	he->p.ptr = _free(he->p.ptr);

if (!(_rpmbuildFlags & 4)) {
	if (spec->sourcePkgId != NULL) {
	    he->tag = RPMTAG_SOURCEPKGID;
	    he->t = RPM_BIN_TYPE;
	    he->p.ptr = spec->sourcePkgId;
	    he->c = 16;
	    xx = headerPut(pkg->header, he, 0);
	}
}
	
	{   const char *binFormat = rpmGetPath("%{_rpmfilename}", NULL);
	    char *binRpm, *binDir;
	    binRpm = headerSprintf(pkg->header, binFormat, NULL,
			       rpmHeaderFormats, &errorString);
	    binFormat = _free(binFormat);
	    if (binRpm == NULL) {
		he->tag = RPMTAG_NVRA;
		xx = headerGet(pkg->header, he, 0);
		rpmlog(RPMLOG_ERR, _("Could not generate output "
		     "filename for package %s: %s\n"), he->p.str, errorString);
		he->p.ptr = _free(he->p.ptr);
		rc = RPMRC_FAIL;
		goto exit;
	    }
	    fn = rpmGetPath("%{_rpmdir}/", binRpm, NULL);
	    if ((binDir = strchr(binRpm, '/')) != NULL) {
		struct stat st;
		const char *dn;
		*binDir = '\0';
		dn = rpmGetPath("%{_rpmdir}/", binRpm, NULL);
		if (Stat(dn, &st) < 0) {
		    switch(errno) {
		    case  ENOENT:
			if (rpmioMkpath(dn, 0755, -1, -1) == 0)
			    /*@switchbreak@*/ break;
			/*@fallthrough@*/
		    default:
			rpmlog(RPMLOG_ERR,_("cannot create %s: %s\n"),
			    dn, strerror(errno));
			/*@switchbreak@*/ break;
		    }
		}
		dn = _free(dn);
	    }
	    binRpm = _free(binRpm);
	}

	memset(csa, 0, sizeof(*csa));
	csa->cpioArchiveSize = 0;
	/*@-type@*/ /* LCL: function typedefs */
/*@-onlytrans@*/
	csa->cpioFdIn = fdNew("init (packageBinaries)");
/*@=onlytrans@*/
/*@-assignexpose -newreftrans@*/
	csa->fi = rpmfiLink(pkg->fi, "packageBinaries");
/*@=assignexpose =newreftrans@*/
assert(csa->fi != NULL);

	rc = writeRPM(&pkg->header, NULL, fn,
		    csa, spec->passPhrase, NULL, spec->dig);

/*@-onlytrans@*/
	csa->fi->te = _free(csa->fi->te);	/* XXX memory leak */
/*@=onlytrans@*/
	csa->fi = rpmfiFree(csa->fi);
/*@-nullpass -onlytrans -refcounttrans @*/
	csa->cpioFdIn = fdFree(csa->cpioFdIn, "init (packageBinaries)");
/*@=nullpass =onlytrans =refcounttrans @*/
	/*@=type@*/
	if (rc == RPMRC_OK) {
	    /* Do check each written package if enabled */
	    char *pkgcheck = rpmExpand("%{?_build_pkgcheck} ", fn, NULL);
	    if (pkgcheck[0] != ' ') {
		rc = checkPackages(pkgcheck);
	    }
	    pkgcheck = _free(pkgcheck);
	    xx = argvAdd(&pkglist, fn);
	}
	fn = _free(fn);
	if (rc) {
	    pkglist = argvFree(pkglist);
	    goto exit;
	}
    }

    /* Now check the package set if enabled */
    if (pkglist != NULL) {
	char *pkgcheck_set = NULL;
	char *pkgs = NULL;
	size_t pkglen = 0;
	int i;
        for (i = 0; i < argvCount(pkglist); i++)
	    pkglen += strlen(pkglist[i]) + 1;
	pkgs = xcalloc(1, pkglen);
	for (i = 0; i < argvCount(pkglist); i++) {
	    if (i)
		strcat(pkgs, " ");
	    strcat(pkgs, pkglist[i]);
	}

	pkgcheck_set = rpmExpand("%{?_build_pkgcheck_set} ", pkgs,  NULL);
	if (pkgcheck_set[0] != ' ') {	/* run only if _build_pkgcheck_set is defined */
	    rc = checkPackages(pkgcheck_set);
	}
	pkgcheck_set = _free(pkgcheck_set);
	pkglist = argvFree(pkglist);
	pkgs = _free(pkgs);
     }

exit:

    return rc;
}

rpmRC packageSources(Spec spec)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct cpioSourceArchive_s csabuf;
    CSA_t csa = &csabuf;
    rpmRC rc;
    int xx;
#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_FEDORA) || defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) /* backward-compat-rpmtag-sourcepackage */
    rpmuint32_t val;
#endif

    /* Add rpmlib markers for tracking. */
    (void) rpmlibMarkers(spec->sourceHeader);

#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_FEDORA) || defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) /* backward-compat-rpmtag-sourcepackage */
    /* Mark package as a SRPM for backward compatibility with RPM < 4.4.6 */
    he->tag = RPMTAG_SOURCEPACKAGE;
    he->t = RPM_UINT32_TYPE;
    val = 1;
    he->p.ui32p = &val;
    he->c = 1;
    xx = headerPut(spec->sourceHeader, he, 0);
#endif

    /* Add build scriptlet status/time (if any) to SRPM's. */
    {	size_t ix;	/* XXX coverity #1035951 */
	for (ix = 0; ix < RPMSCRIPT_MAX; ix++) {
	    if (spec->sstates[ix] == 0)
		continue;
	    if (spec->smetrics[ix] == 0)
		continue;
	    break;
	}
	if (ix < RPMSCRIPT_MAX) {
	    he->tag = RPMTAG_SCRIPTSTATES;
	    he->t = RPM_UINT32_TYPE;
	    he->p.ui32p = spec->sstates;
	    he->c = RPMSCRIPT_MAX;
	    xx = headerPut(spec->sourceHeader, he, 0);
	    he->tag = RPMTAG_SCRIPTMETRICS;
	    he->t = RPM_UINT32_TYPE;
	    he->p.ui32p = spec->smetrics;
	    he->c = RPMSCRIPT_MAX;
	    xx = headerPut(spec->sourceHeader, he, 0);
	}
    }
	
    /* Add macros used during build to SRPM's. */
    {	const char ** av = NULL;
	(void)rpmGetMacroEntries(NULL, NULL, 1, &av);
	if (av != NULL && av[0] != NULL) {
	    he->tag = RPMTAG_BUILDMACROS;
	    he->t = RPM_STRING_ARRAY_TYPE;
	    he->p.argv = av;
	    he->c = argvCount(av);
	    xx = headerPut(spec->sourceHeader, he, 0);
	}
/*@-nullstate@*/
	av = argvFree(av);
/*@=nullstate@*/
    }

    spec->cookie = _free(spec->cookie);
    
    /* XXX this should be %_srpmdir */
    {	const char *srcrpmdir = rpmGetPath("%{_srcrpmdir}/", NULL);
	const char *fn = rpmGetPath("%{_srcrpmdir}/", spec->sourceRpmName,NULL);
	const char *pkgcheck = rpmExpand("%{?_build_pkgcheck_srpm} ", fn, NULL);

	rc = rpmioMkpath(srcrpmdir, 0755, -1, -1);

	memset(csa, 0, sizeof(*csa));
	csa->cpioArchiveSize = 0;
	/*@-type@*/ /* LCL: function typedefs */
/*@-onlytrans@*/
	csa->cpioFdIn = fdNew("init (packageSources)");
/*@=onlytrans@*/
/*@-assignexpose -newreftrans@*/
	csa->fi = rpmfiLink(spec->fi, "packageSources");
/*@=assignexpose =newreftrans@*/
#ifdef	DYING
assert(csa->fi != NULL);
#else
	if (csa->fi == NULL) {	/* XXX segfault avoidance */
	    srcrpmdir = _free(srcrpmdir);
	    fn = _free(fn);
	    pkgcheck = _free(pkgcheck);
	    return RPMRC_FAIL;
	}
#endif

	spec->sourcePkgId = NULL;
	rc = writeRPM(&spec->sourceHeader, &spec->sourcePkgId, fn,
		csa, spec->passPhrase, &spec->cookie, spec->dig);

	/* Do check SRPM package if enabled */
	if (rc == RPMRC_OK && pkgcheck[0] != ' ') {
	    rc = checkPackages(pkgcheck);
	}

/*@-onlytrans@*/
	csa->fi->te = _free(csa->fi->te);	/* XXX memory leak */
/*@=onlytrans@*/
	csa->fi = rpmfiFree(csa->fi);
/*@-nullpass -onlytrans -refcounttrans @*/
	csa->cpioFdIn = fdFree(csa->cpioFdIn, "init (packageSources)");
/*@=nullpass =onlytrans =refcounttrans @*/
	/*@=type@*/
	srcrpmdir = _free(srcrpmdir);
	fn = _free(fn);
	pkgcheck = _free(pkgcheck);
    }

    rc = (rc ? RPMRC_FAIL : RPMRC_OK);

    return rc;
}
