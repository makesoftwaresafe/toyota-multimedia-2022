#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include "rpmio.h"
#include "rpmiotypes.h"		/* XXX fnpyKey */

#include "rpmtypes.h"
#include "rpmtag.h"

#define _RPMEVR_INTERNAL
#include "rpmds.h"
#include "rpmevr.h"

#include "rpmxs.h"

void _newdep(SV * sv_deptag, char * name, SV * sv_sense, SV * sv_evr) {
    rpmTag deptag = 0;
    rpmsenseFlags sense = RPMSENSE_ANY;
    rpmds Dep;
    char * evr = NULL;
    dSP;

    if (sv_deptag && SvOK(sv_deptag))
        deptag = sv2constant(sv_deptag, "rpmtag");
    if (sv_sense && SvOK(sv_sense))
        sense = sv2constant(sv_sense, "rpmsenseflags");
    if (sv_evr && SvOK(sv_evr))
        evr = SvPV_nolen(sv_evr);
    Dep = rpmdsSingle(deptag, 
        name,
        evr ? evr : "", sense);
    if (Dep) {
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Dependencies", Dep)));
    }
    PUTBACK;
}

MODULE = RPM::Dependencies		PACKAGE = RPM::Dependencies

PROTOTYPES: ENABLE

rpmds
new(class, header, sv_tag)
    const char * class
    Header header
    SV * sv_tag
    PREINIT:
    rpmTag tag = 0;
    CODE:
    tag = sv2constant(sv_tag, "rpmtag");
    RETVAL = rpmdsNew(header, tag, 0);
    if (RETVAL) RETVAL = rpmdsInit(RETVAL);
    OUTPUT:
    RETVAL

void
create(perlclass, sv_tag, name, sv_sense = NULL, sv_evr = NULL)
    char * perlclass
    SV * sv_tag
    char * name
    SV * sv_sense
    SV * sv_evr
    PPCODE:
    PUTBACK;
    _newdep(sv_tag, name, sv_sense, sv_evr);
    SPAGAIN;

void
DESTROY(Dep)
    rpmds Dep
    CODE:
    Dep = rpmdsFree(Dep);

int 
count(Dep)
    rpmds Dep
    CODE:
    RETVAL = rpmdsCount(Dep);
    OUTPUT:
    RETVAL

int
index(Dep)
    rpmds Dep
    CODE:
    RETVAL = rpmdsIx(Dep);
    OUTPUT:
    RETVAL

void
init(Dep)
    rpmds Dep
    CODE:
    rpmdsInit(Dep);
        
char *
next(Dep)
    rpmds Dep
    PREINIT:
    char val[16];
    int idx = 0;
    CODE:
    if ((idx = rpmdsNext(Dep)) ==  -1)
        RETVAL = NULL;
    else  {
        snprintf(val, 12, idx == 0 ? "%dE0" : "%d", idx);
        RETVAL = val;
    }
    OUTPUT:
    RETVAL

void
name(Dep)
    rpmds Dep
    PPCODE:
    CHECK_RPMDS_IX(Dep);
    XPUSHs(sv_2mortal(newSVpv(rpmdsN(Dep), 0)));

void
flags(Dep)
    rpmds Dep
    PPCODE:
    CHECK_RPMDS_IX(Dep);
    XPUSHs(sv_2mortal(newSViv(rpmdsFlags(Dep))));

void
evr(Dep)
    rpmds Dep
    PPCODE:
    CHECK_RPMDS_IX(Dep);
    XPUSHs(sv_2mortal(newSVpv(rpmdsEVR(Dep), 0)));

int
color(Dep)
    rpmds Dep
    CODE:
    RETVAL = rpmdsColor(Dep);
    OUTPUT:
    RETVAL
        
int
__find(Dep, depb)
    rpmds Dep
    rpmds depb
    CODE:
    RETVAL = rpmdsFind(Dep, depb);
    OUTPUT:
    RETVAL

int
__merge(Dep, depb)
    rpmds Dep
    rpmds depb
    CODE:
    RETVAL = rpmdsMerge(&Dep, depb);
    OUTPUT:
    RETVAL
        
int
overlap(Dep1, Dep2)
    rpmds Dep1
    rpmds Dep2
    CODE:
    CHECK_RPMDS_IX(Dep1);
    CHECK_RPMDS_IX(Dep2);
    RETVAL = rpmdsCompare(Dep1, Dep2);
    OUTPUT:
    RETVAL

void
__info(Dep)
    rpmds Dep
    PREINIT:
    rpmsenseFlags flag;
    I32 gimme = GIMME_V;
    PPCODE:
    CHECK_RPMDS_IX(Dep);
    if (gimme == G_SCALAR) {
        XPUSHs(sv_2mortal(newSVpv(rpmdsDNEVR(Dep), 0)));
    } else {
        switch (rpmdsTagN(Dep)) {
            case RPMTAG_PROVIDENAME:
                XPUSHs(sv_2mortal(newSVpv("P", 0)));
            break;
            case RPMTAG_REQUIRENAME:
                XPUSHs(sv_2mortal(newSVpv("R", 0)));
            break;
            case RPMTAG_CONFLICTNAME:
                XPUSHs(sv_2mortal(newSVpv("C", 0)));
            break;
            case RPMTAG_OBSOLETENAME:
                XPUSHs(sv_2mortal(newSVpv("O", 0)));
            break;
            case RPMTAG_TRIGGERNAME:
                XPUSHs(sv_2mortal(newSVpv("T", 0)));
            break;
            default:
            break;
        }
        XPUSHs(sv_2mortal(newSVpv(rpmdsN(Dep), 0)));
        flag = rpmdsFlags(Dep);
        XPUSHs(sv_2mortal(newSVpvf("%s%s%s",
                        flag & RPMSENSE_LESS ? "<" : "",
                        flag & RPMSENSE_GREATER ? ">" : "",
                        flag & RPMSENSE_EQUAL ? "=" : "")));
        XPUSHs(sv_2mortal(newSVpv(rpmdsEVR(Dep), 0)));
    }

void
tag(Dep)
    rpmds Dep
    PPCODE:
    XPUSHs(sv_2mortal(newSViv(rpmdsTagN(Dep))));
    
int
__nopromote(Dep, sv_nopromote = NULL)
    rpmds Dep
    SV * sv_nopromote
    CODE:
    if (sv_nopromote == NULL) {
        RETVAL = rpmdsNoPromote(Dep);
    } else {
        RETVAL = rpmdsSetNoPromote(Dep, SvIV(sv_nopromote));
    }
    OUTPUT:
    RETVAL
    
    
int
add(Dep, name,  sv_sense = NULL, sv_evr = NULL)
    rpmds Dep
    char * name
    SV * sv_evr
    SV * sv_sense
    PREINIT:
    rpmsenseFlags sense = RPMSENSE_ANY;
    rpmds Deptoadd;
    char * evr = NULL;
    CODE:
    RETVAL = 0;
    if (sv_sense && SvOK(sv_sense))
        sense = sv2constant(sv_sense, "rpmsenseflags");
    if (sv_evr && SvOK(sv_evr))
        evr = SvPV_nolen(sv_evr);
    Deptoadd = rpmdsSingle(rpmdsTagN(Dep), name,
         evr ? evr : "", sense);
    if (Deptoadd) {
        rpmdsMerge(&Dep, Deptoadd);
        Deptoadd = rpmdsFree(Deptoadd);
        RETVAL = 1;
    }
    OUTPUT:
    RETVAL
        
int
__matchheader(Dep, header, sv_nopromote = NULL)
    Header header
    SV * sv_nopromote
    rpmds Dep
    PREINIT:
    int nopromote = 0;
    CODE:
    if (sv_nopromote != NULL)
        nopromote = SvIV(sv_nopromote);    
    RETVAL = _header_vs_dep(header, Dep, nopromote);
    OUTPUT:
    RETVAL

int
__matchheadername(Dep, header, sv_nopromote = NULL)
    rpmds Dep
    Header header
    SV * sv_nopromote
    PREINIT:
    int nopromote = 0;
    CODE:
    if (sv_nopromote != NULL)
        nopromote = SvIV(sv_nopromote);
    RETVAL = _headername_vs_dep(header, Dep, nopromote);
    OUTPUT:
    RETVAL
        

