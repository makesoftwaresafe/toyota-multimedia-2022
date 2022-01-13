#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include "rpmconstant.h"

MODULE = RPM::Constant		PACKAGE = RPM::Constant

PROTOTYPES: ENABLE

void
listallcontext()
    PREINIT:
    rpmconst consti = rpmconstNew();
    PPCODE:
    while (rpmconstNextL(consti)) {
        XPUSHs(sv_2mortal(newSVpv(rpmconstContext(consti), 0)));
    }
    consti = rpmconstFree(consti);

void
listcontext(context, hideprefix = 1)
    char * context
    int hideprefix
    PREINIT:
    rpmconst consti = rpmconstNew();
    PPCODE:
    if (rpmconstInitToContext(consti, context)) {
        while (rpmconstNextC(consti)) {
            XPUSHs(sv_2mortal(newSVpv(rpmconstName(consti, hideprefix), 0)));
        }
    }
    consti = rpmconstFree(consti);

void
getvalue(context, name, hideprefix = 1)
    char * context
    char * name
    int hideprefix
    PREINIT:
    rpmconst consti = rpmconstNew();
    int val = 0;
    PPCODE:
    if (rpmconstantFindName(context, name, &val, !hideprefix))
        XPUSHs(sv_2mortal(newSViv(val)));
    consti = rpmconstFree(consti);
