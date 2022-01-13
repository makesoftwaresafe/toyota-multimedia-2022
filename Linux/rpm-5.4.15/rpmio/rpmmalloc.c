/** \ingroup rpmio
 * \file rpmio/rpmmalloc.c
 */

#include "system.h"
#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <yarn.h>
#include "debug.h"

#ifdef __cplusplus
GENfree(rpmioItem)
#endif	/* __cplusplus */

#if defined(WITH_DMALLOC)
#undef xmalloc
#undef xcalloc
#undef xrealloc
#undef xstrdup
#endif

#if !defined(EXIT_FAILURE)
#define	EXIT_FAILURE	1
#endif

/*@-modfilesys@*/
/*@only@*/ void *vmefail(size_t size)
{
    fprintf(stderr, _("memory alloc (%u bytes) returned NULL.\n"), (unsigned)size);
    exit(EXIT_FAILURE);
    /*@notreached@*/
/*@-nullret@*/
    return NULL;
/*@=nullret@*/
}
/*@=modfilesys@*/

/**
 */
struct rpmioPool_s {
    yarnLock have;		/*!< unused items available, lock for list */
/*@relnull@*/
    void *pool;
/*@relnull@*/
    rpmioItem head;		/*!< linked list of available items */
/*@dependent@*/
    rpmioItem * tail;
    size_t size;		/*!< size of items in this pool */
    int limit;			/*!< number of new items allowed, or -1 */
    int flags;
/*@null@*/
    const char * (*dbg) (void *item)
	/*@*/;			/*!< generate string w Unlink/Link debugging */
/*@null@*/
    void (*init) (void *item)
	/*@modifies *item @*/;	/*!< create item contents. */
/*@null@*/
    void (*fini) (void *item)
	/*@modifies *item @*/;	/*!< destroy item contents. */
    int reused;			/*!< number of items reused */
    int made;			/*!< number of items made */
/*@observer@*/
    const char *name;
/*@null@*/
    void * zlog;
};

/*@unchecked@*/ /*@only@*/ /*@null@*/
static rpmioPool _rpmioPool;

rpmioPool rpmioFreePool(rpmioPool pool)
	/*@globals _rpmioPool @*/
	/*@modifies _rpmioPool @*/
{
    if (pool == NULL) {
	pool = _rpmioPool;
	_rpmioPool = NULL;
    }
    if (pool != NULL) {
	rpmioItem item;
	int count = 0;
	yarnPossess(pool->have);
VALGRIND_HG_CLEAN_MEMORY(pool, sizeof(*pool));
	while ((item = pool->head) != NULL) {
VALGRIND_HG_CLEAN_MEMORY(item, pool->size);
	    pool->head = (rpmioItem) item->pool;	/* XXX pool == next */
	    if (item->use != NULL)
		item->use = yarnFreeLock(item->use);
	    item = _free(item);
	    count++;
	}
	yarnRelease(pool->have);
	pool->have = yarnFreeLock(pool->have);
	rpmlog(RPMLOG_DEBUG, D_("pool %s:\treused %d, alloc'd %d, free'd %d items.\n"), pool->name, pool->reused, pool->made, count);
#ifdef	NOTYET
assert(pool->made == count);
#else
if (pool->made != count)
rpmlog(RPMLOG_WARNING, D_("pool %s: FIXME: made %d, count %d\nNote: This is a harmless memory leak discovered while exiting, relax ...\n"), pool->name, pool->made, count);
#endif
	(void) _free(pool);
	VALGRIND_DESTROY_MEMPOOL(pool);
    }
    return NULL;
}

/*@-internalglobs@*/
rpmioPool rpmioNewPool(const char * name, size_t size, int limit, int flags,
		char * (*dbg) (void *item),
		void (*init) (void *item),
		void (*fini) (void *item))
	/*@*/
{
    rpmioPool pool = (rpmioPool) xcalloc(1, sizeof(*pool));
#if defined(WITH_VALGRIND)
    static int rzB = 0;		/* size of red-zones (if any) */
    static int is_zeroed = 0;	/* does pool return zero'd allocations? */
    rzB = rzB;			/* XXX CentOS5 valgrind doesn't use. */
    is_zeroed = is_zeroed;	/* XXX CentOS5 valgrind doesn't use. */
#endif
    VALGRIND_CREATE_MEMPOOL(pool, rzB, is_zeroed);
    pool->have = yarnNewLock(0);
    pool->pool = NULL;
    pool->head = NULL;
    pool->tail = &pool->head;
    pool->size = size;
    pool->limit = limit;
    pool->flags = flags;
    pool->dbg = (const char* (*)(void*)) dbg;
    pool->init = init;
    pool->fini = fini;
    pool->reused = 0;
    pool->made = 0;
    pool->name = name;
    pool->zlog = NULL;
    rpmlog(RPMLOG_DEBUG, D_("pool %s:\tcreated size %u limit %d flags %d\n"), pool->name, (unsigned)pool->size, pool->limit, pool->flags);
    return pool;
}
/*@=internalglobs@*/

/*@-internalglobs@*/
rpmioItem rpmioUnlinkPoolItem(rpmioItem item, const char * msg,
		const char * fn, unsigned ln)
{
    rpmioPool pool;
    if (item == NULL) return NULL;
    yarnPossess(item->use);
ANNOTATE_HAPPENS_AFTER(item);
    if ((pool = (rpmioPool) item->pool) != NULL && pool->flags && msg != NULL) {
	const char * imsg = (pool->dbg ? (*pool->dbg)((void *)item) : "");
/*@-modfilesys@*/
	fprintf(stderr, "--> %s %p -- %ld %s at %s:%u%s\n", pool->name,
			item, yarnPeekLock(item->use), msg, fn, ln, imsg);
/*@=modfilesys@*/
    }
    yarnTwist(item->use, BY, -1);
/*@-retalias@*/	/* XXX returning the deref'd item is used to detect nrefs = 0 */
    return item;
/*@=retalias@*/
}
/*@=internalglobs@*/

/*@-internalglobs@*/
rpmioItem rpmioLinkPoolItem(rpmioItem item, const char * msg,
		const char * fn, unsigned ln)
{
    rpmioPool pool;
    if (item == NULL) return NULL;
    yarnPossess(item->use);
    if ((pool = (rpmioPool) item->pool) != NULL && pool->flags && msg != NULL) {
	const char * imsg = (pool->dbg ? (*pool->dbg)((void *)item) : "");
/*@-modfilesys@*/
	fprintf(stderr, "--> %s %p ++ %ld %s at %s:%u%s\n", pool->name,
			item, yarnPeekLock(item->use)+1, msg, fn, ln, imsg);
/*@=modfilesys@*/
    }
ANNOTATE_HAPPENS_BEFORE(item);
    yarnTwist(item->use, BY, 1);
    return item;
}
/*@=internalglobs@*/

/*@-internalglobs@*/
/*@null@*/
void * rpmioFreePoolItem(/*@killref@*/ /*@null@*/ rpmioItem item,
                const char * msg, const char * fn, unsigned ln)
        /*@modifies item @*/
{
    rpmioPool pool;
    if (item == NULL) return NULL;

#ifdef	NOTYET
assert(item->pool != NULL);	/* XXX (*pool->fini) is likely necessary */
#endif
    yarnPossess(item->use);
ANNOTATE_HAPPENS_AFTER(item);
    if ((pool = (rpmioPool) item->pool) != NULL && pool->flags && msg != NULL) {
	const char * imsg = (pool->dbg ? (*pool->dbg)((void *)item) : "");
/*@-modfilesys@*/
	fprintf(stderr, "--> %s %p -- %ld %s at %s:%u%s\n", pool->name,
			item, yarnPeekLock(item->use), msg, fn, ln, imsg);
/*@=modfilesys@*/
    }
    if (yarnPeekLock(item->use) <= 1L) {
VALGRIND_HG_CLEAN_MEMORY(item, pool->size);
	if (pool != NULL && pool->fini != NULL)
	    (*pool->fini) ((void *)item);
	VALGRIND_MEMPOOL_FREE(pool, item + 1);
	item = rpmioPutPool(item);
    } else
	yarnTwist(item->use, BY, -1);
/*@-retalias@*/	/* XXX returning the deref'd item is used to detect nrefs = 0 */
    return (void *) item;
/*@=retalias@*/
}
/*@=internalglobs@*/

/*@-internalglobs@*/
rpmioItem rpmioGetPool(rpmioPool pool, size_t size)
{
    rpmioItem item;

    if (pool != NULL) {
	/* if can't create any more, wait for a space to show up */
	yarnPossess(pool->have);
	if (pool->limit == 0)
	    yarnWaitFor(pool->have, NOT_TO_BE, 0);

	/* if a space is available, pull it from the list and return it */
	if (pool->head != NULL) {
	    item = pool->head;
	    pool->head = (rpmioItem) item->pool;	/* XXX pool == next */
	    if (pool->head == NULL)
		pool->tail = &pool->head;
	    pool->reused++;
	    item->pool = pool;		/* remember the pool this belongs to */
	    yarnTwist(pool->have, BY, -1);      /* one less in pool */
	    VALGRIND_MEMPOOL_ALLOC(pool,
		item + 1,
		size - sizeof(struct rpmioItem_s));
	    return item;
	}

	/* nothing available, don't want to wait, make a new item */
assert(pool->limit != 0);
	if (pool->limit > 0)
	    pool->limit--;
	pool->made++;
	yarnRelease(pool->have);
    }

    item = (rpmioItem) xcalloc(1, size);
    item->use = yarnNewLock(0);		/* XXX newref? */
    item->pool = pool;
    VALGRIND_MEMPOOL_ALLOC(pool,
	item + 1,
	size - sizeof(struct rpmioItem_s));
    return item;
}
/*@=internalglobs@*/

/*@-internalglobs@*/
rpmioItem rpmioPutPool(rpmioItem item)
{
    rpmioPool pool;

    if ((pool = (rpmioPool) item->pool) != NULL) {
	yarnPossess(pool->have);
	item->pool = NULL;		/* XXX pool == next */
	*pool->tail = item;
	pool->tail = (rpmioItem *)&item->pool;/* XXX pool == next */
	yarnTwist(pool->have, BY, 1);
	if (item->use != NULL)
	    yarnTwist(item->use, TO, 0);
	return NULL;
    }

    if (item->use != NULL) {
	yarnTwist(item->use, TO, 0);
	item->use = yarnFreeLock(item->use);
    }
    (void) _free(item);
    return NULL;
}
/*@=internalglobs@*/

#if !(HAVE_MCHECK_H && defined(__GNUC__)) && !defined(__LCLINT__)

/*@out@*/ /*@only@*/ void * xmalloc (size_t size)
{
    register void *value;
    if (size == 0) size++;
    value = malloc (size);
    if (value == 0)
	value = vmefail(size);
    return value;
}

/*@only@*/ void * xcalloc (size_t nmemb, size_t size)
{
    register void *value;
    if (size == 0) size++;
    if (nmemb == 0) nmemb++;
    value = calloc (nmemb, size);
    if (value == 0)
	value = vmefail(size);
    return value;
}

/*@only@*/ void * xrealloc (/*@only@*/ void *ptr, size_t size)
{
    register void *value;
    if (size == 0) size++;
    value = realloc (ptr, size);
    if (value == 0)
	value = vmefail(size);
    return value;
}

/*@only@*/ char * xstrdup (const char *str)
{
    size_t size = strlen(str) + 1;
    char *newstr = (char *) malloc (size);
    if (newstr == 0)
	newstr = (char *) vmefail(size);
    strcpy (newstr, str);
    return newstr;
}

#endif	/* !(HAVE_MCHECK_H && defined(__GNUC__)) */
