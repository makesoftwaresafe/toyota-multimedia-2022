#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#undef Fflush
#undef Mkdir
#undef Stat
#undef Fstat

#include "../config.h"
/* XXX artifacts needed to include "rpmio_internal.h" w/o "system.h" */
void * vmefail(size_t size);
#define xstrdup(_str)   (strcpy((malloc(strlen(_str)+1) ? : vmefail(strlen(_str)+1)), (_str)))

#include "rpmio_internal.h"
#include "rpmtypes.h"
#include "rpmtag.h"
#include "rpmfi.h"

MODULE = RPM::Files		PACKAGE = RPM::Files

PROTOTYPES: ENABLE

rpmfi
new(class, header, ts = NULL)
    char * class
    Header header
    rpmts ts
    CODE:
    RETVAL = rpmfiNew(ts, header, RPMTAG_BASENAMES, 0);
    OUTPUT:
    RETVAL

void
DESTROY(Files)
    rpmfi Files
    PPCODE:
    Files = rpmfiFree(Files);

int
count(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFC(Files);
    OUTPUT:
    RETVAL

void
init(Files)
    rpmfi Files
    CODE:
    rpmfiInit(Files, 0);

char *
next(Files)
    rpmfi Files
    PREINIT:
    char val[16];
    int idx = 0;
    CODE:
    if ((idx = rpmfiNext(Files)) == -1) {
        RETVAL = NULL;
    } else {
        snprintf(val, 12, idx == 0 ? "%dE0" : "%d", idx);
        RETVAL = val;
    }
    OUTPUT:
    RETVAL

int
count_dir(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiDC(Files);
    OUTPUT:
    RETVAL


void
init_dir(Files)
    rpmfi Files
    CODE:
    rpmfiInitD(Files, 0);

char *
next_dir(Files)
    rpmfi Files
    PREINIT:
    char val[16];
    int idx = 0;
    CODE:
    if ((idx = rpmfiNextD(Files)) == -1) {
        RETVAL = NULL;
    } else {
        snprintf(val, 12, idx == 0 ? "%dE0" : "%d", idx);
        RETVAL = val;
    }
    OUTPUT:
    RETVAL

int
_compare(Files, Fb)
    rpmfi Files
    rpmfi Fb
    CODE:
    RETVAL = rpmfiCompare(Files, Fb);
    OUTPUT:
    RETVAL
        
const char *
filename(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFN(Files);
    OUTPUT:
    RETVAL

const char *
dirname(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiDN(Files);
    OUTPUT:
    RETVAL

const char *
basename(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiBN(Files);
    OUTPUT:
    RETVAL

unsigned int
fflags(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFFlags(Files);
    OUTPUT:
    RETVAL

unsigned int
mode(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFMode(Files);
    OUTPUT:
    RETVAL

void
digest(Files)
    rpmfi Files
    PREINIT:
    const unsigned char * digest;
    int algop = 0;
    size_t lenp = 0;
    PPCODE:
    if ((digest = rpmfiDigest(Files, &algop, &lenp)) != NULL
    /* return undef if empty */) {
	if (lenp && strlen((const char*)digest)) {
	    const char * hexDigest = pgpHexStr(digest, lenp);
	    XPUSHs(sv_2mortal(newSViv(algop)));
	    XPUSHs(sv_2mortal(newSVpv(hexDigest, lenp<<1)));
	}
    }

const char *
link(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFLink(Files);
    OUTPUT:
    RETVAL

const char *
user(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFUser(Files);
    OUTPUT:
    RETVAL

const char *
group(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFGroup(Files);
    OUTPUT:
    RETVAL

int
inode(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFInode(Files);
    OUTPUT:
    RETVAL
    
int
size(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFSize(Files);
    OUTPUT:
    RETVAL

int
dev(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFRdev(Files);
    OUTPUT:
    RETVAL

int
color(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFColor(Files);
    OUTPUT:
    RETVAL

const char *
class(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFClass(Files);
    OUTPUT:
    RETVAL

int
mtime(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFMtime(Files);
    OUTPUT:
    RETVAL

int
_nlink(Files)
    rpmfi Files
    CODE:
    RETVAL = rpmfiFNlink(Files);
    OUTPUT:
    RETVAL

