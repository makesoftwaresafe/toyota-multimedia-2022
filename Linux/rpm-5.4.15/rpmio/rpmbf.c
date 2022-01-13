/** \ingroup rpmio
 * \file rpmio/rpmbf.c
 */

#include "system.h"
#include <math.h>

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>

#define	_RPMBF_INTERNAL
#include <rpmbf.h>

#include "debug.h"

/* Any pair of 32 bit hashes can be used. lookup3.c generates pairs, will do. */
#define	_JLU3_jlu32lpair	1
#include "lookup3.c"

/*@unchecked@*/
int _rpmbf_debug = 0;

/*@-mustmod@*/	/* XXX splint on crack */
static void rpmbfFini(void * _bf)
	/*@globals fileSystem @*/
	/*@modifies *_bf, fileSystem @*/
{
    rpmbf bf = (rpmbf) _bf;

    bf->bits = PBM_FREE(bf->bits);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmbfPool = NULL;

static rpmbf rpmbfGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmbfPool, fileSystem @*/
	/*@modifies pool, _rpmbfPool, fileSystem @*/
{
    rpmbf bf;

    if (_rpmbfPool == NULL) {
	_rpmbfPool = rpmioNewPool("bf", sizeof(*bf), -1, _rpmbf_debug,
			NULL, NULL, rpmbfFini);
	pool = _rpmbfPool;
    }
    return (rpmbf) rpmioGetPool(pool, sizeof(*bf));
}

rpmbf rpmbfNew(size_t m, size_t k, unsigned flags)
{
    static size_t nestimate = 1024;	/* XXX default estimated population. */
    rpmbf bf = rpmbfGetPool(_rpmbfPool);

    if (k == 0) k = 16;
    if (m == 0) m = (3 * nestimate * k) / 2;

    bf->k = k;
    bf->m = m;
    bf->n = 0;
    bf->bits = (unsigned char *) PBM_ALLOC(bf->m-1);

    return rpmbfLink(bf);
}

int rpmbfAdd(rpmbf bf, const void * _s, size_t ns)
{
    const char * s = (const char *) _s;
    rpmuint32_t h0 = 0;
    rpmuint32_t h1 = 0;

    if (bf == NULL) return -1;

    if (ns == 0) ns = strlen(s);
    jlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < bf->k; ns++) {
	rpmuint32_t h = h0 + ns * h1;
	rpmuint32_t ix = (h % bf->m);
	PBM_SET(ix, bf);
    }
    bf->n++;
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p,\"%s\") bf{%u,%u}[%u]\n", __FUNCTION__, bf, s, (unsigned)bf->m, (unsigned)bf->k, (unsigned)bf->n);
    return 0;
}

int rpmbfChk(rpmbf bf, const void * _s, size_t ns)
{
    const char * s = (const char *) _s;
    rpmuint32_t h0 = 0;
    rpmuint32_t h1 = 0;
    int rc = 1;

    if (bf == NULL) return -1;

    if (ns == 0) ns = strlen(s);
    jlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < bf->k; ns++) {
	rpmuint32_t h = h0 + ns * h1;
	rpmuint32_t ix = (h % bf->m);
	if (PBM_ISSET(ix, bf))
	    continue;
	rc = 0;
	break;
    }
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p,\"%s\") bf{%u,%u}[%u] rc %d\n", __FUNCTION__, bf, s, (unsigned)bf->m, (unsigned)bf->k, (unsigned)bf->n, rc);
    return rc;
}

int rpmbfClr(rpmbf bf)
{
    static size_t nbw = (__PBM_NBITS/8);
    __pbm_bits * bits;
    size_t nw;

    if (bf == NULL) return -1;

    bits = __PBM_BITS(bf);
    nw = (__PBM_IX(bf->m-1) + 1);
    memset(bits, 0, nw * nbw);
    bf->n = 0;
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p) bf{%u,%u}[%u]\n", __FUNCTION__, bf, (unsigned)bf->m, (unsigned)bf->k, (unsigned)bf->n);
    return 0;
}

int rpmbfDel(rpmbf bf, const void * _s, size_t ns)
{
    const char * s = (const char *) _s;
    rpmuint32_t h0 = 0;
    rpmuint32_t h1 = 0;

    if (bf == NULL) return -1;

    if (ns == 0) ns = strlen(s);
assert(ns > 0);
    jlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < bf->k; ns++) {
	rpmuint32_t h = h0 + ns * h1;
	rpmuint32_t ix = (h % bf->m);
	PBM_CLR(ix, bf);
    }
    if (bf->n != 0)
	bf->n--;
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p,\"%s\") bf{%u,%u}[%u]\n", __FUNCTION__, bf, s, (unsigned)bf->m, (unsigned)bf->k, (unsigned)bf->n);
    return 0;
}

int rpmbfIntersect(rpmbf a, const rpmbf b)
{
    __pbm_bits * abits;
    __pbm_bits * bbits;
    size_t nw;
    size_t i;

    if (a == NULL || b == NULL) return -1;

    abits = __PBM_BITS(a);
    bbits = __PBM_BITS(b);
    nw = (__PBM_IX(a->m-1) + 1);

    if (!(a->m == b->m && a->k == b->k))
	return -1;
    for (i = 0; i < nw; i++)
	abits[i] &= bbits[i];
    a->n = 1;		/* XXX what is population estimate? */
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p,%p) bf{%u,%u}[%u]\n", __FUNCTION__, a, b, (unsigned)a->m, (unsigned)a->k, (unsigned)a->n);
    return 0;
}

int rpmbfUnion(rpmbf a, const rpmbf b)
{
    __pbm_bits * abits;
    __pbm_bits * bbits;
    size_t nw;
    size_t i;

    if (a == NULL || b == NULL) return -1;

    abits = __PBM_BITS(a);
    bbits = __PBM_BITS(b);
    nw = (__PBM_IX(a->m-1) + 1);

    if (!(a->m == b->m && a->k == b->k))
	return -1;
    for (i = 0; i < nw; i++)
	abits[i] |= bbits[i];
    a->n += b->n;
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%p,%p) bf{%u,%u}[%u]\n", __FUNCTION__, a, b, (unsigned)a->m, (unsigned)a->k, (unsigned)a->n);
    return 0;
}

void rpmbfParams(size_t n, double e, size_t * mp, size_t * kp)
{
    static size_t _nmin = 10;
    static size_t _n = 10000;
    static size_t _nmax = 1 * 1000 * 1000 * 1000;
    static double _emin = 1.0e-10;
    static double _e = 1.0e-4;
    static double _emax = 1.0e-2;
    size_t m;
    size_t k;

    /* XXX sanity checks on (n,e) args. */
    if (!(n >= _nmin && _n <= _nmax))
	n = _n;
    if (!(e >= _emin && _e <= _emax))
	e = _e;

    m = (size_t)((n * log(e)) / (log(1.0 / pow(2.0, log(2.0)))) + 0.5);
    k = (size_t) ((m * log(2.0)) / n);
    if (mp) *mp = m;
    if (kp) *kp = k;
if (_rpmbf_debug)
fprintf(stderr, "<-- %s(%u, %g) m %u k %u\n", __FUNCTION__, (unsigned)n, e, (unsigned)m, (unsigned)k);
}
