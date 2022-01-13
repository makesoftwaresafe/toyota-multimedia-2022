/** \ingroup rpmio
 * \file rpmio/rpmbag.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>	/* for *Pool methods */
#include <rpmlog.h>
#include <rpmurl.h>
#define	_RPMBAG_INTERNAL
#include <rpmbag.h>

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmsdb)
GENfree(rpmsdb *)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmbag_debug = 0;

static size_t _maxnsdbp = 5;

static void rpmbagFini(void * _bag)
	/*@globals fileSystem @*/
	/*@modifies *_bag, fileSystem @*/
{
    rpmbag bag = (rpmbag) _bag;

    bag->sdbp = _free(bag->sdbp);
    bag->nsdbp = 0;
    bag->fn = _free(bag->fn);
    bag->flags = 0;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmbagPool = NULL;

static rpmbag rpmbagGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmbagPool, fileSystem @*/
	/*@modifies pool, _rpmbagPool, fileSystem @*/
{
    rpmbag bag;

    if (_rpmbagPool == NULL) {
	_rpmbagPool = rpmioNewPool("bag", sizeof(*bag), -1, _rpmbag_debug,
			NULL, NULL, rpmbagFini);
	pool = _rpmbagPool;
    }
    bag = (rpmbag) rpmioGetPool(pool, sizeof(*bag));
    memset(((char *)bag)+sizeof(bag->_item), 0, sizeof(*bag)-sizeof(bag->_item));
    return bag;
}

rpmbag rpmbagNew(const char * fn, int flags)
{
    rpmbag bag = rpmbagGetPool(_rpmbagPool);

    if (fn)
	bag->fn = xstrdup(fn);
    bag->flags = flags;

    bag->sdbp = (rpmsdb *) xcalloc(_maxnsdbp, sizeof(*bag->sdbp));

    return rpmbagLink(bag);
}

int rpmbagAdd(rpmbag bag, void *_db, int dbmode)
{
    if (bag && bag->sdbp && bag->nsdbp < _maxnsdbp) {
	rpmsdb * sdbp = bag->sdbp;
	int i = bag->nsdbp++;		/* XXX find empty slot */
	sdbp[i] = (rpmsdb) xcalloc(1, sizeof(*sdbp[i]));
	sdbp[i]->dbmode = dbmode;
	sdbp[i]->_db = _db;
    }

    return 0;
}

int rpmbagDel(rpmbag bag, int i)
{

    if (bag && bag->sdbp && i >= 0 && i <= (int)_maxnsdbp) {
	rpmsdb * sdbp = bag->sdbp;
	memset(sdbp[i], 0, sizeof(*sdbp[i]));
	sdbp[i] = _free(sdbp[i]);
	if ((i+1) == (int)bag->nsdbp)	/* XXX find empty slot */
	    bag->nsdbp--;
    }

    return 0;
}
