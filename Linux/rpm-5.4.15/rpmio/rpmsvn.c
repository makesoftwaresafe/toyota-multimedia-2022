/** \ingroup rpmio
 * \file rpmio/rpmsvn.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>

#if defined(WITH_SUBVERSION)
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_repos.h"
#include "svn_subst.h"
#endif

#define _RPMSVN_INTERNAL
#include <rpmsvn.h>

#include "debug.h"

/*@unchecked@*/
int _rpmsvn_debug = 0;

/*==============================================================*/

/*==============================================================*/

static int npools;

static void rpmsvnFini(void * _svn)
	/*@globals fileSystem @*/
	/*@modifies *_svn, fileSystem @*/
{
    rpmsvn svn = (rpmsvn) _svn;

#if defined(WITH_SUBVERSION)
    if (svn->pool) {
	svn_pool_destroy(svn->pool);
	svn->pool = NULL;
    }
    if (--npools <= 0)
	apr_terminate();
    svn->allocator = NULL;
#endif

    svn->fn = _free(svn->fn);

}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsvnPool = NULL;

static rpmsvn rpmsvnGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsvnPool, fileSystem @*/
	/*@modifies pool, _rpmsvnPool, fileSystem @*/
{
    rpmsvn svn;

    if (_rpmsvnPool == NULL) {
	_rpmsvnPool = rpmioNewPool("svn", sizeof(*svn), -1, _rpmsvn_debug,
			NULL, NULL, rpmsvnFini);
	pool = _rpmsvnPool;
    }
    svn = (rpmsvn) rpmioGetPool(pool, sizeof(*svn));
    memset(((char *)svn)+sizeof(svn->_item), 0, sizeof(*svn)-sizeof(svn->_item));
    return svn;
}

rpmsvn rpmsvnNew(const char * fn, int flags)
{
    rpmsvn svn = rpmsvnGetPool(_rpmsvnPool);
    int xx;

#if defined(WITH_SUBVERSION)
    if (npools++ <= 0) {
	xx = apr_initialize();
assert(xx == APR_SUCCESS);
    }
    xx = apr_allocator_create(&svn->allocator);
assert(xx == 0);
    apr_allocator_max_free_set(svn->allocator, SVN_ALLOCATOR_RECOMMENDED_MAX_FREE);
    svn->pool = svn_pool_create_ex(NULL, svn->allocator);
    apr_allocator_owner_set(svn->allocator, svn->pool);

#endif

    if (fn)
	svn->fn = xstrdup(fn);


    return rpmsvnLink(svn);
}
