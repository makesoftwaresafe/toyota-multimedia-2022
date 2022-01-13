/** \ingroup rpmio
 * \file rpmdb/rpmmdb.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>

#define	_RPMMDB_INTERNAL
#include <rpmmdb.h>

#include "debug.h"

/*@unchecked@*/
int _rpmmdb_debug = 0;

/*@unchecked@*/ /*@relnull@*/
rpmmdb _rpmmdbI;

/*==============================================================*/

static void rpmmdbFini(void * _mdb)
	/*@globals fileSystem @*/
	/*@modifies *_mdb, fileSystem @*/
{
    rpmmdb mdb = (rpmmdb) _mdb;

    mdb->fn = _free(mdb->fn);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmmdbPool = NULL;

static rpmmdb rpmmdbGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmmdbPool, fileSystem @*/
	/*@modifies pool, _rpmmdbPool, fileSystem @*/
{
    rpmmdb mdb;

    if (_rpmmdbPool == NULL) {
	_rpmmdbPool = rpmioNewPool("mdb", sizeof(*mdb), -1, _rpmmdb_debug,
			NULL, NULL, rpmmdbFini);
	pool = _rpmmdbPool;
    }
    mdb = (rpmmdb) rpmioGetPool(pool, sizeof(*mdb));
    memset(((char *)mdb)+sizeof(mdb->_item), 0, sizeof(*mdb)-sizeof(mdb->_item));
    return mdb;
}

rpmmdb rpmmdbNew(const char * fn, int flags)
{
    rpmmdb mdb = rpmmdbGetPool(_rpmmdbPool);

    if (fn)
	mdb->fn = xstrdup(fn);

    return rpmmdbLink(mdb);
}
