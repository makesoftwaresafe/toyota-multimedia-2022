/* common function for perl module */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <utime.h>

#include "rpmio.h"
#include "rpmiotypes.h"
#include "rpmtypes.h"
#include "rpmtag.h"
#include "pkgio.h"
#include "rpmbuild.h"
#include "rpmconstant.h"

#include "rpmxs.h"

static int scalar2constant(SV * svconstant, const char * context, int * val) {
    int rc = 0;
    if (!svconstant || !SvOK(svconstant)) {
        warn("Use of an undefined value");
        return 0;
    } else if (SvIOK(svconstant)) {
        *val = SvIV(svconstant);
        rc = 1;
    } else if (SvPOK(svconstant)) {
        rc = rpmconstantFindName((char *)context, (void *) SvPV_nolen(svconstant), val, 0);
    } else {
    }
    return rc;
}

int sv2constant(SV * svconstant, const char * context) {
    AV * avparam;
    int val = 0;
    SV **tmpsv;
    int i;
    if (svconstant == NULL) {
        return 0;
    } else if (!SvOK(svconstant)) {
        return 0;
    } else if (SvPOK(svconstant) || SvIOK(svconstant)) {
        if (!scalar2constant(svconstant, context, &val))
            warn("Unknow value '%s' in '%s'", SvPV_nolen(svconstant), context);
    } else if (SvTYPE(SvRV(svconstant)) == SVt_PVAV) {
        avparam = (AV*) SvRV(svconstant);
        for (i = 0; i <= av_len(avparam); i++) {
            tmpsv = av_fetch(avparam, i, 0);
            if (!scalar2constant(*tmpsv, context, &val))
                warn("Unknow value '%s' in '%s' from array", SvPV_nolen(*tmpsv), context);
        }
    } else {
    }
    return val;
}

rpmTag sv2dbquerytag(SV * sv_tag) {
    int val = 0;
    if (!scalar2constant(sv_tag, "rpmdbi", &val) && !scalar2constant(sv_tag, "rpmtag", &val))
        croak("unknown tag value '%s'", SvPV_nolen(sv_tag));
    return val;
}

void _rpm2header(rpmts ts, char * filename, int checkmode) {
    FD_t fd;
    Header ret = NULL;
    rpmRC rc;
    dSP;
    if ((fd = Fopen(filename, "r"))) {
        rc = rpmReadPackageFile(ts, fd, filename, &ret);
	if (checkmode) {
	    XPUSHs(sv_2mortal(newSViv(rc)));
	    (void)headerFree(ret); /* For checking the package, we don't keep the header */
	    ret = NULL;
        } else {
            if (rc == 0) {
        	    XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", (void *)ret)));
            } else {
                XPUSHs(sv_2mortal(&PL_sv_undef));
            }
	}
        Fclose(fd);
    } else {
        XPUSHs(sv_2mortal(&PL_sv_undef));
    }
        
    PUTBACK;
    return;
}

void _newiterator(rpmts ts, SV * sv_tagname, SV * sv_tagvalue, int keylen) {
    rpmmi mi;
    int tag = RPMDBI_PACKAGES;
    void * value = NULL;
    int i = 0;
    dSP;
    if (sv_tagname == NULL || !SvOK(sv_tagname)) {
        tag = RPMDBI_PACKAGES; /* Assume search into installed packages */
    } else {
        tag = sv2dbquerytag(sv_tagname);
    }
    if (sv_tagvalue != NULL && SvOK(sv_tagvalue)) {
        switch (tag) {
            case RPMDBI_PACKAGES:
                i = SvIV(sv_tagvalue);
                value = &i;
                keylen = sizeof(i);
            break;
            default:
                value = (void *) SvPV_nolen(sv_tagvalue);
            break;
        }
    }
    mi = rpmtsInitIterator(ts, tag, value, keylen);
    XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::PackageIterator", mi)));
    PUTBACK;
    return;
}

int _headername_vs_dep(Header h, rpmds dep, int nopromote) {
    HE_t he = memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int rc = 0;
    CHECK_RPMDS_IX(dep);
    he->tag = RPMTAG_NAME;
    headerGet(h, he, 0);
    if (strcmp(he->p.str, rpmdsN(dep)) != 0)
        rc = 0;
    else
        rc = rpmdsNVRMatchesDep(h, dep, nopromote);
    he->p.ptr = _free(he->p.ptr);
    return rc;
    /* return 1 if match */
}

int _header_vs_dep(Header h, rpmds dep, int nopromote) {
    CHECK_RPMDS_IX(dep);
    return rpmdsAnyMatchesDep(h, dep, nopromote);
    /* return 1 if match */
}

/* Get a new specfile */
void _newspec(rpmts ts, char * filename, SV * svpassphrase, SV * svrootdir, SV * svcookies, SV * svanyarch, SV * svforce, SV * svverify) {
    Spec spec = NULL;
    char * passphrase = NULL;
    char * rootdir = NULL;
    char * cookies = NULL;
    int anyarch = 0;
    int force = 0;
    int verify = 0;
    dSP;

    if (svpassphrase && SvOK(svpassphrase))
        passphrase = SvPV_nolen(svpassphrase);
    
    if (svrootdir && SvOK(svrootdir))
	rootdir = SvPV_nolen(svrootdir);
    else
	rootdir = "/";
    
    if (svcookies && SvOK(svcookies))
	cookies = SvPV_nolen(svcookies);

    if (svanyarch && SvOK(svanyarch))
	anyarch = SvIV(svanyarch);
    
    if (svforce && SvOK(svforce))
	force = SvIV(svforce);
    
    if (svverify && SvOK(svverify))
	verify = SvIV(svverify);
    
    if (filename) {
        if (!parseSpec(ts, filename, rootdir, 0, passphrase, cookies, anyarch, force, verify))
            spec = rpmtsSetSpec(ts, NULL);
#ifdef HHACK
    } else {
        spec = newSpec();
#endif
    }
    if (spec) {
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Spec", (void *)spec)));
    } else
        XPUSHs(sv_2mortal(&PL_sv_undef));
    PUTBACK;
    return;
}

/* Building a spec file */
int _specbuild(rpmts ts, Spec spec, SV * sv_buildflags) {
    rpmBuildFlags buildflags = sv2constant(sv_buildflags, "rpmbuildflags");
    if (buildflags == RPMBUILD_NONE) croak("No action given for build");
    return buildSpec(ts, spec, buildflags, 0);
}

void _installsrpms(rpmts ts, char * filename) {
    const char * specfile = NULL;
    const char * cookies = NULL;
    dSP;
    I32 gimme = GIMME_V;
    if (rpmInstallSource(
                ts,
                filename,
                &specfile,
                &cookies) == 0) { 
        XPUSHs(sv_2mortal(newSVpv(specfile, 0)));
        if (gimme == G_ARRAY)
        XPUSHs(sv_2mortal(newSVpv(cookies, 0)));
    }
    PUTBACK;
}


