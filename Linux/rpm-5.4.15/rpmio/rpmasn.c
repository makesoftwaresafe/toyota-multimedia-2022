/** \ingroup rpmio
 * \file rpmio/rpmasn.c
 */

#include "system.h"

#if defined(HAVE_LIBTASN1_H)
#include <libtasn1.h>
#endif

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>
#define	_RPMASN_INTERNAL
#include <rpmasn.h>

#include "debug.h"

/*@unchecked@*/
int _rpmasn_debug = 0;

/*@-mustmod@*/	/* XXX splint on crack */
static void rpmasnFini(void * _asn)
	/*@globals fileSystem @*/
	/*@modifies *_asn, fileSystem @*/
{
    rpmasn asn = (rpmasn) _asn;

#if defined(WITH_LIBTASN1)
    asn1_delete_structure(&asn->tree);
    asn->tree = ASN1_TYPE_EMPTY;
#endif

    asn->fn = _free(asn->fn);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmasnPool = NULL;

static rpmasn rpmasnGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmasnPool, fileSystem @*/
	/*@modifies pool, _rpmasnPool, fileSystem @*/
{
    rpmasn asn;

    if (_rpmasnPool == NULL) {
	_rpmasnPool = rpmioNewPool("asn", sizeof(*asn), -1, _rpmasn_debug,
			NULL, NULL, rpmasnFini);
	pool = _rpmasnPool;
    }
    return (rpmasn) rpmioGetPool(pool, sizeof(*asn));
}

rpmasn rpmasnNew(const char * fn, int flags)
{
    rpmasn asn = rpmasnGetPool(_rpmasnPool);

    if (fn)
	asn->fn = xstrdup(fn);

#if defined(WITH_LIBTASN1)
    asn->tree = ASN1_TYPE_EMPTY;
    int xx = asn1_parser2tree(fn, &asn->tree, asn->error);
    (void)xx;
	/* XXX errors. */
#endif

    return rpmasnLink(asn);
}

#ifdef	NOTYET
const char * rpmasnFile(rpmasn asn, const char *fn)
{
    const char * t = NULL;

if (_rpmasn_debug)
fprintf(stderr, "--> rpmasnFile(%p, %s)\n", asn, (fn ? fn : "(nil)"));

#if defined(WITH_LIBTASN1)
#endif

    if (t == NULL) t = "";
    t = xstrdup(t);

if (_rpmasn_debug)
fprintf(stderr, "<-- rpmasnFile(%p, %s) %s\n", asn, (fn ? fn : "(nil)"), t);
    return t;
}

const char * rpmasnBuffer(rpmasn asn, const char * b, size_t nb)
{
    const char * t = NULL;

if (_rpmasn_debug)
fprintf(stderr, "--> rpmasnBuffer(%p, %p[%d])\n", asn, b, (int)nb);
    if (nb == 0) nb = strlen(b);

#if defined(WITH_LIBTASN1)
#endif

    if (t == NULL) t = "";
    t = xstrdup(t);

if (_rpmasn_debug)
fprintf(stderr, "<-- rpmasnBuffer(%p, %p[%d]) %s\n", asn, b, (int)nb, t);
    return t;
}
#endif	/* NOTYET */
