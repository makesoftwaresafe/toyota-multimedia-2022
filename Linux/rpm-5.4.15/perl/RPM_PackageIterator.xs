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

#include <rpmio.h>
#include "rpmiotypes.h"

#include "rpmtypes.h"
#include "rpmtag.h"
#include "rpmdb.h"

#include "rpmte.h"
#include "rpmts.h"

#include "misc.h"

#include "rpmcli.h"

#include "rpmxs.h"

MODULE = RPM::PackageIterator		PACKAGE = RPM::PackageIterator

PROTOTYPES: ENABLE

void
new(class, sv_tagname = NULL, sv_tagvalue = NULL, keylen = 0)
    char * class
    SV * sv_tagname
    SV * sv_tagvalue
    int keylen
    PREINIT:
    rpmts ts = rpmtsCreate();
    PPCODE:
    PUTBACK;
    _newiterator(ts, sv_tagname, sv_tagvalue, keylen);
    SPAGAIN;
    /* FIXME: ts = rpmtsFree(ts); */

void
DESTROY(mi)
    rpmmi mi
    CODE:
    mi = rpmmiFree(mi);

void
prune(mi, ...)
    rpmmi mi
    PREINIT:
    uint32_t * exclude = NULL;
    int exclude_count = 0;
    int i = 0;
    CODE:
    exclude_count = items - 1;
    exclude = malloc(exclude_count * sizeof(*exclude));
    for (i = 1; i < items; i++) {
        if (!SvIOK(ST(i))) { /* TODO: */ }
        exclude[i - 1] = SvIV(ST(i));
    }
    rpmmiPrune(mi, exclude, exclude_count, 0);
    _free(exclude);
    
unsigned int
getoffset(mi)
    rpmmi mi
    CODE:
    RETVAL = rpmmiInstance(mi);
    OUTPUT:
    RETVAL

int
count(mi)
    rpmmi mi
    CODE:
    RETVAL = rpmmiCount(mi);
    OUTPUT:
    RETVAL

void
next(mi)
    rpmmi mi
    PREINIT:
    Header header = NULL;
    PPCODE:
    header = rpmmiNext(mi);
    if (header) {
        XPUSHs(sv_2mortal(sv_setref_pv(newSVpv("", 0), "RPM::Header", headerLink(header))));
    }
    
