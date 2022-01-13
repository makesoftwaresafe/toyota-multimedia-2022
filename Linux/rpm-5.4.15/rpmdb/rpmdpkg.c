/** \ingroup rpmds
 * \file rpmdb/rpmdpkg.c
 */
#include "system.h"

#include <rpmiotypes.h>
#include <rpmtag.h>

#define	_RPMEVR_INTERNAL
#include <rpmdpkg.h>

#include "debug.h"

/*@access EVR_t @*/

/*@unchecked@*/
int _rpmdpkg_debug = 0;

/* assume ascii */
static inline int dpkgEVRctype(char x)
	/*@*/
{
    int c = (int)x;
    return (
	  x == '~' ? -1 
	: xisdigit(c) ? 0
	: x == '\0' ? 0 \
	: xisalpha(c) ? c
	: c + 256
    );
}

int dpkgEVRcmp(const char *a, const char *b)
{
    if (a == NULL) a = "";
    if (b == NULL) b = "";

    while (*a || *b) {
	int first_diff= 0;

	while ( (*a && !xisdigit((int)*a)) || (*b && !xisdigit((int)*b)) ) {
	    int vc = dpkgEVRctype(*a);
	    int rc = dpkgEVRctype(*b);
	    if (vc != rc) return vc - rc;
	    a++; b++;
	}

	while (*a == '0') a++;
	while (*b == '0') b++;
	while (xisdigit((int)*a) && xisdigit((int)*b)) {
	    if (!first_diff) first_diff = (int)(*a - *b);
	    a++; b++;
	}
	if (xisdigit((int)*a)) return 1;
	if (xisdigit((int)*b)) return -1;
	if (first_diff) return first_diff;
    }
    return 0;
}

int dpkgEVRparse(const char * evrstr, EVR_t evr)
{
    return rpmEVRparse(evrstr, evr);
}

int dpkgEVRcompare(const EVR_t a, const EVR_t b)
{
    int r;

    if (a->Elong > b->Elong) return 1;
    if (a->Elong < b->Elong) return -1;
    r = dpkgEVRcmp(a->F[RPMEVR_V], b->F[RPMEVR_V]);  if (r) return r;
    return dpkgEVRcmp(a->F[RPMEVR_R], b->F[RPMEVR_R]);
}
