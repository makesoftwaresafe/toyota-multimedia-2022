#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include "rpmio.h"
#include "rpmiotypes.h"
#include "rpmmacro.h"

#include "rpmtypes.h"
#include "rpmtag.h"

#include "rpmfi.h"
#include "rpmts.h"

#include "rpmcli.h"

#include "rpmspec.h"
#include "rpmbuild.h"

MODULE = RPM::Spec		PACKAGE = RPM::Spec

PROTOTYPES: ENABLE

void
new(perlclass, specfile = NULL, ...)
    char * perlclass
    char * specfile
    PREINIT:
    rpmts ts = NULL;
    SV * passphrase = NULL;
    SV * rootdir = NULL;
    SV * cookies = NULL;
    SV * anyarch = 0;
    SV * force = 0;
    SV * verify = 0;
    int i;
    PPCODE:
    for(i=2; i < items; i++) {
        if(strcmp(SvPV_nolen(ST(i)), "transaction") == 0) {
            i++;
            if (sv_isobject(ST(i)) && (SvTYPE(SvRV(ST(i))) == SVt_PVMG)) {
                ts = (rpmts)SvIV((SV*)SvRV(ST(i)));
                ts = rpmtsLink(ts, "RPM::Spec");  
            } else {
                croak( "transaction is not a blessed SV reference" );
                XSRETURN_UNDEF;
            } 
        } else if (strcmp(SvPV_nolen(ST(i)), "force") == 0) {
            i++;
            force = ST(i);
        } else if (strcmp(SvPV_nolen(ST(i)), "verify") == 0) {
            i++;
            verify = ST(i);
        } else if (strcmp(SvPV_nolen(ST(i)), "anyarch") == 0) {
            i++;
            anyarch = ST(i);
        } else if (strcmp(SvPV_nolen(ST(i)), "passphrase") == 0) {
            i++;
            passphrase = ST(i);
        } else if (strcmp(SvPV_nolen(ST(i)), "root") == 0) {
            i++;
            rootdir = ST(i);
        } else {
            warn("Unknown options in RPM::Spec->new, ignored");
            i++;
        }
    }
    if (!ts)
        ts = rpmtsCreate();
    PUTBACK;
    _newspec(ts, specfile, passphrase, rootdir, cookies, anyarch, force, verify);
    SPAGAIN;
    (void)rpmtsFree(ts);
    ts = NULL;
    
void
DESTROY(spec)
    Spec spec
    CODE:
    spec = freeSpec(spec);

void
srcheader(spec)
    Spec spec
    PPCODE:
    initSourceHeader(spec, NULL); /* TODO NULL => @retval *sfp     srpm file list (may be NULL) */
    XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)headerLink(spec->sourceHeader))));

void
binheader(spec)
    Spec spec
    PREINIT:
    Package pkg;
    PPCODE:
    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next)
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)headerLink(pkg->header))));
    
void
srcrpm(spec)
    Spec spec
    PREINIT:
    const char * srctag;
    const char * srcpath;
    PPCODE:
    srcpath = rpmGetPath("%{_srcrpmdir}", NULL);
    srctag = headerSprintf(
        spec->packages->header,
        "%{NAME}-%{VERSION}-%{RELEASE}",
        NULL,
        rpmHeaderFormats,
        NULL
    );
    XPUSHs(sv_2mortal(newSVpvf("%s/%s.%ssrc.rpm",
        srcpath,
        srctag,
        spec->noSource ? "no" : ""
        )));
    _free(srcpath);
    _free(srctag);

void
binrpm(spec)
    Spec spec
    PREINIT:
    Package pkg;
    const char * binFormat;
    char * binRpm;
    const char * path;
    PPCODE:
    for(pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
        if (pkg->fileList == NULL)
            continue;
        /* headerCopyTags(h, pkg->header, copyTags); */
        binFormat = rpmGetPath("%{_rpmfilename}", NULL);
        binRpm = headerSprintf(pkg->header, binFormat, rpmTagTable,
                   rpmHeaderFormats, NULL);
        _free(binFormat);
        path = rpmGetPath("%{_rpmdir}/", binRpm, NULL);
        XPUSHs(sv_2mortal(newSVpv(path, 0)));
        _free(path);
        _free(binRpm);
    }

void
check(spec, ts = NULL)
    Spec spec
    PREINIT:
    rpmts ts = rpmtsCreate();
    rpmps ps;
    PPCODE:
    PUTBACK;
    if (ts)
        ts = rpmtsLink(ts, "Spec_check");
    else
        ts = rpmtsCreate();

    initSourceHeader(spec, NULL); /* TODO NULL => @retval *sfp     srpm file list (may be NULL) */

    if (!headerIsEntry(spec->sourceHeader, RPMTAG_REQUIRENAME)
     && !headerIsEntry(spec->sourceHeader, RPMTAG_CONFLICTNAME))
        /* XSRETURN_UNDEF; */
        return;

    (void) rpmtsAddInstallElement(ts, spec->sourceHeader, NULL, 0, NULL);

    if(rpmtsCheck(ts))
        croak("Can't check rpmts"); /* any better idea ? */

    ps = rpmtsProblems(ts);
    if (ps &&  rpmpsNumProblems(ps)) /* if no problem, return undef */
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Problems", ps)));
    (void)rpmtsFree(ts);
    ts = NULL;
    SPAGAIN;
    
    
int
build(spec, sv_buildflags)
    Spec spec
    SV * sv_buildflags
    PREINIT:
    rpmts ts = rpmtsCreate();
    CODE:
    RETVAL = _specbuild(ts, spec, sv_buildflags);
    (void)rpmtsFree(ts);
    ts = NULL;
    OUTPUT:
    RETVAL

const char *
specfile(spec)
    Spec spec
    CODE:
    RETVAL = spec->specFile;
    OUTPUT:
    RETVAL
        
void
sources(spec, is = 0)
    Spec spec
    int is
    PREINIT:
    struct Source *srcPtr;
    PPCODE:
    for (srcPtr = spec->sources; srcPtr != NULL; srcPtr = srcPtr->next) {
        if (is && !(srcPtr->flags & is))
            continue;
        XPUSHs(sv_2mortal(newSVpv(srcPtr->source, 0)));
    }

void
sources_url(spec, is = 0)
    Spec spec
    int is
    PREINIT:
    struct Source * srcPtr;
    PPCODE:
    for (srcPtr = spec->sources; srcPtr != NULL; srcPtr = srcPtr->next) {
        if (is && !(srcPtr->flags & is))
            continue;
        XPUSHs(sv_2mortal(newSVpv(srcPtr->fullSource, 0)));
    }

void
icon(spec)
    Spec spec
    PPCODE:
    if (spec->sources->flags & RPMFILE_ICON) {
        char * dest = NULL;
        int len;
        len = strlen(spec->sources->source);
        dest = xmalloc(len+1);
	dest[len] = '\0';
        memcpy(dest, spec->sources->source, len);
        XPUSHs(sv_2mortal(newSVpv(dest, len)));

    }

void
icon_url(spec)
    Spec spec
    PPCODE:
    if (spec->sources->flags & RPMFILE_ICON) {
        char * dest = NULL;
        int len;
        len = strlen(spec->sources->fullSource);
        dest = xmalloc(len+1);
        memcpy(dest, spec->sources->fullSource, len);
	dest[len] = '\0';
        XPUSHs(sv_2mortal(newSVpv(dest, len)));

    }

