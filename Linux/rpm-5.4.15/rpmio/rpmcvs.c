/** \ingroup rpmio
 * \file rpmio/rpmcvs.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>
#define	_RPMCVS_INTERNAL
#include <rpmcvs.h>

#include "debug.h"

/*@unchecked@*/
int _rpmcvs_debug = 0;

static void rpmcvsFini(void * _cvs)
	/*@globals fileSystem @*/
	/*@modifies *_cvs, fileSystem @*/
{
    rpmcvs cvs = (rpmcvs) _cvs;

    cvs->fn = _free(cvs->fn);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmcvsPool = NULL;

static rpmcvs rpmcvsGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmcvsPool, fileSystem @*/
	/*@modifies pool, _rpmcvsPool, fileSystem @*/
{
    rpmcvs cvs;

    if (_rpmcvsPool == NULL) {
	_rpmcvsPool = rpmioNewPool("cvs", sizeof(*cvs), -1, _rpmcvs_debug,
			NULL, NULL, rpmcvsFini);
	pool = _rpmcvsPool;
    }
    cvs = (rpmcvs) rpmioGetPool(pool, sizeof(*cvs));
    memset(((char *)cvs)+sizeof(cvs->_item), 0, sizeof(*cvs)-sizeof(cvs->_item));
    return cvs;
}

rpmcvs rpmcvsNew(const char * fn, int flags)
{
    rpmcvs cvs = rpmcvsGetPool(_rpmcvsPool);

    if (fn)
	cvs->fn = xstrdup(fn);

    return rpmcvsLink(cvs);
}
