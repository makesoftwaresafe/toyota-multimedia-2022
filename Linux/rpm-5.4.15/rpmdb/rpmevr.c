/** \ingroup rpmds
 * \file rpmdb/rpmevr.c
 */
#include "system.h"

#include <rpmiotypes.h>
#include <rpmmacro.h>
#define	_MIRE_INTERNAL
#include <mire.h>

#include <rpmlog.h>
#include <set.h>

#include <rpmtag.h>
#define	_RPMEVR_INTERNAL
#include <rpmevr.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(EVR_t)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmevr_debug = 0;

#if !defined(MAX)
#define MAX(x, y) ( ((x)>(y))?(x):(y) )
#endif

EVR_t rpmEVRnew(uint32_t Flags, int initialize)
{
    EVR_t evr = (EVR_t) xcalloc(1, sizeof(*evr));
    evr->Flags = (evrFlags) Flags;
    if (initialize) {
/*@-observertrans -readonlytrans @*/
	evr->F[RPMEVR_E] = "0";
	evr->F[RPMEVR_V] = "";
	evr->F[RPMEVR_T] = "";
	evr->F[RPMEVR_R] = "";
	evr->F[RPMEVR_D] = "";
/*@=observertrans =readonlytrans @*/
    }
    return evr;
}

EVR_t rpmEVRfree(EVR_t evr)
{
    if (evr != NULL) {
	evr->str = _free(evr->str);
	memset(evr, 0, sizeof(*evr));
	evr = _free(evr);
    }
    return NULL;
}

/* XXX Force digits to beat alphas. See bugzilla #50977. */
/*@unchecked@*/
#if defined(RPM_VENDOR_MANDRIVA)  || defined(RPMVERCMP_DIGITS_BEAT_ALPHA) /* old-comparision-behaviour */
static int _invert_digits_alphas_comparison = -1;
#else
static int _invert_digits_alphas_comparison = 1;
#endif

/* XXX Punctuation characters that are not treated as alphas */
/*@unchecked@*/ /*@observer@*/
static const char * _rpmnotalpha = ".:-";

/**
 * Return rpm's analogue of xisalpha.
 * @param c		character to test
 * @return		is this an alpha character?
 */
static inline int xisrpmalpha(int c)
	/*@*/
{
    int rc = xisalpha(c);
    if (!rc)
	rc = xispunct(c);
    if (rc && _rpmnotalpha && *_rpmnotalpha)
	rc = (strchr(_rpmnotalpha, c) == NULL);
/*@-globstate@*/
    return rc;
/*@=globstate@*/
}

int rpmEVRcmp(const char * a, const char * b)
	/*@*/
{
    const char * ae = NULL, * be = NULL;
    int rc = 0;		/* assume equal */

assert(a != NULL);
assert(b != NULL);
    /* Compare version strings segment by segment. */
    for (; *a && *b && rc == 0; a = ae, b = be) {

	/* Skip leading non-alpha, non-digit characters. */
	while (*a && !(xisdigit((int)*a) || xisrpmalpha((int)*a))) a++;
	while (*b && !(xisdigit((int)*b) || xisrpmalpha((int)*b))) b++;

	/* Wildcard comparison? */
	/* Note: limited to suffix-only wildcard matching at the moment. */
	if (a[0] == '*' && a[1] == '\0') {
            be = strchr(b, '\0');	/* XXX be = b + strlen(b); */
	} else
	if (b[0] == '*' && b[1] == '\0') {
            ae = strchr(a, '\0');	/* XXX ae = a + strlen(a); */
	} else
	/* Digit string comparison? */
	if (xisdigit((int)*a) || xisdigit((int)*b)) {
	    /* Discard leading zeroes. */
	    while (a[0] == '0' && xisdigit((int)a[1])) a++;
	    while (b[0] == '0' && xisdigit((int)b[1])) b++;

	    /* Find end of digit strings. */
	    ae = a; while (xisdigit((int)*ae)) ae++;
	    be = b; while (xisdigit((int)*be)) be++;

	    /* Calculate digit comparison return code. */
	    if (a == ae || b == be)
		rc = (int)(*a - *b) * _invert_digits_alphas_comparison;
	    else {
		rc = (ae - a) - (be - b);
		if (!rc)
		    rc = strncmp(a, b, (ae - a));
	    }
	} else {
	    /* Find end of alpha strings. */
	    ae = a; while (xisrpmalpha((int)*ae)) ae++;
	    be = b; while (xisrpmalpha((int)*be)) be++;

	    /* Calculate alpha comparison return code. */
	    rc = strncmp(a, b, MAX((ae - a), (be - b)));
	}
    }

    /* Longer string wins. */
    if (!rc)
	rc = (int)(*a - *b);

    /* Force strict -1, 0, 1 return. */
    rc = (rc > 0 ? 1
	: rc < 0 ? -1
	: 0);
    return rc;
}

/*@unchecked@*/ /*@observer@*/ /*@null@*/
static const char _evr_tuple_match[] =
	"^(?:([^:-]+):)?([^:-]+)(?:-([^:-]+))?(?::([^:-]+))?$";
/*@unchecked@*/ /*@only@*/ /*@observer@*/ /*@null@*/
const char * evr_tuple_match = NULL;
/*@unchecked@*/ /*@refcounted@*/ /*@null@*/
miRE evr_tuple_mire = NULL;

static miRE rpmEVRmire(void)
	/*@*/
{
/*@-globs -internalglobs -mods @*/
    if (evr_tuple_mire == NULL) {
	int xx;
	evr_tuple_match = rpmExpand("%{?evr_tuple_match}", NULL);
	if (evr_tuple_match == NULL || evr_tuple_match[0] == '\0') {
	    evr_tuple_match = _free(evr_tuple_match); /* XXX coverity #1035894 */
	    evr_tuple_match = xstrdup(_evr_tuple_match);
	}

	evr_tuple_mire = mireNew(RPMMIRE_REGEX, 0);
	xx = mireSetCOptions(evr_tuple_mire, RPMMIRE_REGEX, 0, 0, NULL);
	xx = mireRegcomp(evr_tuple_mire, evr_tuple_match);

    }
/*@=globs =internalglobs =mods @*/
assert(evr_tuple_match != NULL && evr_tuple_mire != NULL);
/*@-globstate -retalias@*/
    return evr_tuple_mire;
/*@=globstate =retalias@*/
}

int rpmEVRparse(const char * evrstr, EVR_t evr)
	/*@modifies evrstr, evr @*/
{
    miRE mire = rpmEVRmire();
    int noffsets = 6 * 3;
    int offsets[6 * 3];
    size_t nb;
    int xx;
    int i;

    memset(evr, 0, sizeof(*evr));
    evr->str = xstrdup(evrstr);
    nb = strlen(evr->str);

    memset(offsets, -1, sizeof(offsets));
    xx = mireSetEOptions(mire, offsets, noffsets);

    xx = mireRegexec(mire, evr->str, strlen(evr->str));

    for (i = 0; i < noffsets; i += 2) {
	int ix;

	if (offsets[i] < 0)
	    continue;

	switch (i/2) {
	default:
	case 0:	continue;	/*@notreached@*/ /*@switchbreak@*/ break;
	case 1:	ix = RPMEVR_E;	/*@switchbreak@*/break;
	case 2:	ix = RPMEVR_V;	/*@switchbreak@*/break;
	case 3:	ix = RPMEVR_T;	/*@switchbreak@*/break;
	case 4:	ix = RPMEVR_R;	/*@switchbreak@*/break;
	case 5:	ix = RPMEVR_D;	/*@switchbreak@*/break;
	}

assert(offsets[i  ] >= 0 && offsets[i  ] <= (int)nb);
assert(offsets[i+1] >= 0 && offsets[i+1] <= (int)nb);
	{   char * te = (char *) evr->str;
	    evr->F[ix] = te + offsets[i];
	    te += offsets[i+1];
	    *te = '\0';
	}

    }

    /* XXX HACK: postpone committing to single "missing" value for now. */
/*@-observertrans -readonlytrans@*/
    if (evr->F[RPMEVR_E] == NULL) evr->F[RPMEVR_E] = "0";
    if (evr->F[RPMEVR_V] == NULL) evr->F[RPMEVR_V] = "";
    if (evr->F[RPMEVR_T] == NULL) evr->F[RPMEVR_T] = "";
    if (evr->F[RPMEVR_R] == NULL) evr->F[RPMEVR_R] = "";
    if (evr->F[RPMEVR_D] == NULL) evr->F[RPMEVR_D] = "";
/*@=observertrans =readonlytrans@*/

    evr->Elong = strtoul(evr->F[RPMEVR_E], NULL, 10);

    xx = mireSetEOptions(mire, NULL, 0);

    return 0;
}

/**
 * Dressed rpmEVRcmp, handling missing values.
 * @param a		1st string
 * @param b		2nd string
 * @return		+1 if a is "newer", 0 if equal, -1 if b is "newer"
 */
static int compare_values(const char *a, const char *b)
	/*@*/
{
    return rpmvercmp(a, b);
}

/*@unchecked@*/ /*@observer@*/ /*@null@*/
static const char _evr_tuple_order[] = "EVR";
/*@unchecked@*/ /*@only@*/ /*@observer@*/ /*@null@*/
static const char * evr_tuple_order = NULL;

/**
 * Return precedence permutation string.
 * @return		precedence permutation
 */
/*@observer@*/
static const char * rpmEVRorder(void)
	/*@*/
{
/*@-globs -internalglobs -mods @*/
    if (evr_tuple_order == NULL) {
	evr_tuple_order = rpmExpand("%{?evr_tuple_order}", NULL);
	if (evr_tuple_order == NULL || evr_tuple_order[0] == '\0') {
	    evr_tuple_order = _free(evr_tuple_order); /* XXX coverity #1035895 */
	    evr_tuple_order = xstrdup(_evr_tuple_order);
	}
    }
/*@=globs =internalglobs =mods @*/
assert(evr_tuple_order != NULL && evr_tuple_order[0] != '\0');
    return evr_tuple_order;
}

int rpmEVRcompare(const EVR_t a, const EVR_t b)
{
    const char * s;
    int rc = 0;

assert(a->F[RPMEVR_E] != NULL);
assert(a->F[RPMEVR_V] != NULL);
assert(a->F[RPMEVR_T] != NULL);
assert(a->F[RPMEVR_R] != NULL);
assert(a->F[RPMEVR_D] != NULL);
assert(b->F[RPMEVR_E] != NULL);
assert(b->F[RPMEVR_V] != NULL);
assert(b->F[RPMEVR_T] != NULL);
assert(b->F[RPMEVR_R] != NULL);
assert(b->F[RPMEVR_D] != NULL);

    for (s = rpmEVRorder(); *s != '\0'; s++) {
	int ix = 0;

	switch ((int)*s) {
	default:	continue;	/*@notreached@*/ /*@switchbreak@*/break;
	case 'E':	ix = RPMEVR_E;	/*@switchbreak@*/break;
	case 'V':	ix = RPMEVR_V;	/*@switchbreak@*/break;
	case 'T':	ix = RPMEVR_T;	/*@switchbreak@*/break;
	case 'R':	ix = RPMEVR_R;	/*@switchbreak@*/break;
	case 'D':	ix = RPMEVR_D;	/*@switchbreak@*/break;
	}
#if defined(RPM_VENDOR_MANDRIVA) /* mdvbz#55810 */
	if (ix >= RPMEVR_R && (b->Flags & (~RPMSENSE_GREATER & RPMSENSE_EQUAL))
			&& !(ix == RPMEVR_D && (b->Flags & RPMSENSE_LESS))
			&& *(b->F[ix]) == '\0')
		break;
#endif

	/* XXX ALT version-set comparison */
	if (ix == RPMEVR_V
	 && !strncmp(a->F[ix], "set:", sizeof("set:")-1)
	 && !strncmp(b->F[ix], "set:", sizeof("set:")-1))
	{
	    rc = rpmsetCmp(a->F[ix], b->F[ix]);
	    if (rc < -1) {
		if (rc == -3)
		    rpmlog(RPMLOG_WARNING, _("failed to decode %s\n"), a->F[ix]);
		if (rc == -4)
		    rpmlog(RPMLOG_WARNING, _("failed to decode %s\n"), b->F[ix]);
		/* neither is subset of each other */
		rc = 0;
	    }
	} else
	if (ix == RPMEVR_T)	/* XXX twiddle-in-version "negative" compare */
	    rc = -compare_values(a->F[ix], b->F[ix]);
	else
	    rc = compare_values(a->F[ix], b->F[ix]);
	if (rc)
	    break;
    }
    return rc;
}

int rpmEVRoverlap(EVR_t a, EVR_t b)
{
    rpmsenseFlags aF = a->Flags;
    rpmsenseFlags bF = b->Flags;
    int sense;
    int result;

    /* XXX HACK: postpone committing to single "missing" value for now. */
/*@-mods -observertrans -readonlytrans @*/
    if (a->F[RPMEVR_E] == NULL)	a->F[RPMEVR_E] = "0";
    if (b->F[RPMEVR_E] == NULL)	b->F[RPMEVR_E] = "0";
    if (a->F[RPMEVR_V] == NULL)	a->F[RPMEVR_V] = "";
    if (b->F[RPMEVR_V] == NULL)	b->F[RPMEVR_V] = "";
    if (a->F[RPMEVR_T] == NULL)	a->F[RPMEVR_T] = "";
    if (b->F[RPMEVR_T] == NULL)	b->F[RPMEVR_T] = "";
    if (a->F[RPMEVR_R] == NULL)	a->F[RPMEVR_R] = "";
    if (b->F[RPMEVR_R] == NULL)	b->F[RPMEVR_R] = "";
    if (a->F[RPMEVR_D] == NULL)	a->F[RPMEVR_D] = "";
    if (b->F[RPMEVR_D] == NULL)	b->F[RPMEVR_D] = "";
/*@=mods =observertrans =readonlytrans @*/
    sense = rpmEVRcompare(a, b);

    /* Detect overlap of {A,B} range. */
    if (aF == RPMSENSE_NOTEQUAL || bF == RPMSENSE_NOTEQUAL)
        result = (sense != 0);
    else if (sense < 0 && ((aF & RPMSENSE_GREATER) || (bF & RPMSENSE_LESS)))
        result = 1;
    else if (sense > 0 && ((aF & RPMSENSE_LESS) || (bF & RPMSENSE_GREATER)))
        result = 1;
    else if (sense == 0 &&
        (((aF & RPMSENSE_EQUAL) && (bF & RPMSENSE_EQUAL)) ||
         ((aF & RPMSENSE_LESS) && (bF & RPMSENSE_LESS)) ||
         ((aF & RPMSENSE_GREATER) && (bF & RPMSENSE_GREATER))))
        result = 1;
    else
        result = 0;
    return result;
}

/*@-redecl@*/
int (*rpmvercmp) (const char *a, const char *b) = rpmEVRcmp;
/*@=redecl@*/

/**
 */
/*@unchecked@*/ /*@observer@*/
static struct EVRop_s {
/*@observer@*/ /*@null@*/
    const char * opstr;
    rpmsenseFlags sense;
} cops[] = {
    { "<=", (rpmsenseFlags) (RPMSENSE_LESS ^ RPMSENSE_EQUAL) },
    { "=<", (rpmsenseFlags) (RPMSENSE_LESS ^ RPMSENSE_EQUAL) },

    { "==", (rpmsenseFlags) (RPMSENSE_EQUAL) },
    { "!=", (rpmsenseFlags) (RPMSENSE_NOTEQUAL) },
    
    { ">=", (rpmsenseFlags) (RPMSENSE_GREATER ^ RPMSENSE_EQUAL) },
    { "=>", (rpmsenseFlags) (RPMSENSE_GREATER ^ RPMSENSE_EQUAL) },

    { "<",  (rpmsenseFlags) (RPMSENSE_LESS) },
    { "=",  (rpmsenseFlags) (RPMSENSE_EQUAL) },
    { ">",  (rpmsenseFlags) (RPMSENSE_GREATER) },

    { NULL, (rpmsenseFlags) 0 },
};

rpmsenseFlags rpmEVRflags(const char *op, const char **end)
{
    rpmsenseFlags Flags = (rpmsenseFlags) 0;
    struct EVRop_s *cop;

    if (op == NULL || *op == '\0')
	Flags = RPMSENSE_EQUAL;
    else
    for (cop = cops; cop->opstr != NULL; cop++) {
	if (strncmp(op, cop->opstr, strlen(cop->opstr)))
	    continue;
	Flags = cop->sense;
	if (end)
	    *end = op + strlen(cop->opstr);
	break;
    }
    return Flags;
}

int rpmVersionCompare(Header A, Header B)
{
    HE_t Ahe = (HE_t) memset(alloca(sizeof(*Ahe)), 0, sizeof(*Ahe));
    HE_t Bhe = (HE_t) memset(alloca(sizeof(*Bhe)), 0, sizeof(*Bhe));
    const char * one, * two;
    rpmuint32_t Eone, Etwo;
    const char * s;
    int rc = 0;
    int xx;

    for (s = rpmEVRorder(); *s != '\0'; s++) {
	switch ((int)*s) {
	default:	continue;	/*@notreached@*/ /*@switchbreak@*/break;
	case 'E':
	    Ahe->tag = RPMTAG_EPOCH;
	    xx = headerGet(A, Ahe, 0);
	    Eone = (xx && Ahe->p.ui32p ? Ahe->p.ui32p[0] : 0);
	    Bhe->tag = RPMTAG_EPOCH;
	    xx = headerGet(B, Bhe, 0);
	    Etwo = (xx && Bhe->p.ui32p ? Bhe->p.ui32p[0] : 0);
	    if (Eone < Etwo)
		rc = -1;
	    else if (Eone > Etwo)
		rc = 1;
	    /*@switchbreak@*/ break;
	case 'V':
	    Ahe->tag = RPMTAG_VERSION;
	    xx = headerGet(A, Ahe, 0);
	    one = (xx && Ahe->p.str ? Ahe->p.str : "");
	    Bhe->tag = RPMTAG_VERSION;
	    xx = headerGet(B, Bhe, 0);
	    two = (xx && Bhe->p.str ? Bhe->p.str : "");
	    rc = rpmvercmp(one, two);
	    /*@switchbreak@*/ break;
	case 'R':
	    Ahe->tag = RPMTAG_RELEASE;
	    xx = headerGet(A, Ahe, 0);
	    one = (xx && Ahe->p.str ? Ahe->p.str : "");
	    Bhe->tag = RPMTAG_RELEASE;
	    xx = headerGet(B, Bhe, 0);
	    two = (xx && Bhe->p.str ? Bhe->p.str : "");
	    rc = rpmvercmp(one, two);
	    /*@switchbreak@*/ break;
	case 'D':
	    Ahe->tag = RPMTAG_DISTEPOCH;
	    xx = headerGet(A, Ahe, 0);
	    one = (xx && Ahe->p.str ? Ahe->p.str : "");
	    Bhe->tag = RPMTAG_DISTEPOCH;
	    xx = headerGet(B, Bhe, 0);
	    two = (xx && Bhe->p.str ? Bhe->p.str : "");
	    rc = rpmvercmp(one, two);
	    /*@switchbreak@*/ break;
	}
	Ahe->p.ptr = _free(Ahe->p.ptr);
	Bhe->p.ptr = _free(Bhe->p.ptr);
	if (rc)
	    break;
    }
    return rc;
}
