#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include "rpmconstant.h"

MODULE = RPM::Problems		PACKAGE = RPM::Problems

PROTOTYPES: ENABLE

void
new(perlclass, ts)
    char * perlclass
    rpmts ts
    PREINIT:
    rpmps ps;
    PPCODE:
    ps = rpmtsProblems(ts);
    if (ps &&  rpmpsNumProblems(ps)) /* if no problem, return undef */
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Problems", ps)));

void
DESTROY(ps)
    rpmps ps
    PPCODE:
    ps = rpmpsFree(ps);

int
count(ps)
    rpmps ps
    CODE:
    RETVAL = rpmpsNumProblems(ps);
    OUTPUT:
    RETVAL

void
print(ps, fp)
    rpmps ps
    FILE *fp
    PPCODE:
    rpmpsPrint(fp, ps);

void
pb_info(ps, numpb)
    rpmps ps
    int numpb
    PREINIT:
    rpmProblem p;
    PPCODE:
    if (rpmpsNumProblems(ps) < numpb)
        XSRETURN_UNDEF;
    else {
        p = rpmpsGetProblem(ps, numpb);
        XPUSHs(sv_2mortal(newSVpv("string", 0)));
        XPUSHs(sv_2mortal(newSVpv(rpmProblemString(p), 0)));
        XPUSHs(sv_2mortal(newSVpv("pkg_nevr", 0)));
        XPUSHs(sv_2mortal(newSVpv(rpmProblemGetPkgNEVR(p), 0)));
        XPUSHs(sv_2mortal(newSVpv("alt_pkg_nevr", 0)));
        XPUSHs(sv_2mortal(newSVpv(rpmProblemGetAltNEVR(p), 0)));
        XPUSHs(sv_2mortal(newSVpv("type", 0)));
        XPUSHs(sv_2mortal(newSViv(rpmProblemGetType(p))));
        XPUSHs(sv_2mortal(newSVpv("key", 0)));
        XPUSHs(sv_2mortal(newSVpv(rpmProblemKey(p), 0)));
    }
 
