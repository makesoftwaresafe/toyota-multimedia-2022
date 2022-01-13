/** \ingroup rpmdep
 * \file lib/rpmal.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmbf.h>

#include <rpmtag.h>
#include <rpmtypes.h>

#define	_RPMDS_INTERNAL
#include <rpmds.h>
#include <rpmal.h>

#include "debug.h"

typedef /*@abstract@*/ struct availablePackage_s * availablePackage;

/*@access alKey @*/
/*@access alNum @*/
/*@access rpmal @*/
/*@access rpmds @*/
/*@access availablePackage @*/

/*@access fnpyKey @*/	/* XXX suggestedKeys array */

/*@unchecked@*/
int _rpmal_debug = 0;

/** \ingroup rpmdep
 * Info about a single package to be installed.
 */
struct availablePackage_s {
/*@refcounted@*/ /*@null@*/
    rpmds provides;		/*!< Provides: dependencies. */
/*@refcounted@*/ /*@null@*/
    rpmbf bf;			/*!< File name Bloom filter. */

    rpmuint32_t tscolor;	/*!< Transaction color bits. */

/*@exposed@*/ /*@dependent@*/ /*@null@*/
    fnpyKey key;		/*!< Associated file name/python object */

};

typedef /*@abstract@*/ struct availableIndexEntry_s *	availableIndexEntry;
/*@access availableIndexEntry@*/

typedef enum {
    IET_PROVIDES=1		/*!< A Provides: dependency. */
} indexEntryType_e;

/** \ingroup rpmdep
 * A single available item (e.g. a Provides: dependency).
 */
struct availableIndexEntry_s {
/*@exposed@*/ /*@dependent@*/ /*@null@*/
    alKey pkgKey;		/*!< Containing package. */
/*@observer@*/
    const char * entry;		/*!< Dependency name. */
    unsigned short entryLen;	/*!< No. of bytes in name. */
    unsigned short entryIx;	/*!< Dependency index. */
    indexEntryType_e type;	/*!< Type of available item. */
};

typedef /*@abstract@*/ struct availableIndex_s *	availableIndex;
/*@access availableIndex@*/

/** \ingroup rpmdep
 * Index of all available items.
 */
struct availableIndex_s {
/*@null@*/
    availableIndexEntry index;	/*!< Array of available items. */
    int size;			/*!< No. of available items. */
    int k;			/*!< Current index. */
};

/** \ingroup rpmdep
 * Set of available packages, items, and directories.
 */
struct rpmal_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@owned@*/ /*@null@*/
    availablePackage list;	/*!< Set of packages. */
    struct availableIndex_s index;	/*!< Set of available items. */
    int delta;			/*!< Delta for pkg list reallocation. */
    int size;			/*!< No. of pkgs in list. */
    int alloced;		/*!< No. of pkgs allocated for list. */
    rpmuint32_t tscolor;	/*!< Transaction color. */
};

#ifdef __cplusplus
GENfree(availablePackage)
GENfree(availableIndexEntry)
GENfree(fnpyKey *)
#endif	/* __cplusplus */

static inline alNum alKey2Num(/*@unused@*/ /*@null@*/ const rpmal al,
		/*@null@*/ alKey pkgKey)
	/*@*/
{
    /*@-nullret -temptrans -retalias @*/
    union { alKey key; alNum num; } u;
    u.num = 0;
    u.key = pkgKey;
    return u.num;
    /*@=nullret =temptrans =retalias @*/
}

static inline alKey alNum2Key(/*@unused@*/ /*@null@*/ const rpmal al,
		/*@null@*/ alNum pkgNum)
	/*@*/
{
    /*@-nullret -temptrans -retalias @*/
    union { alKey key; alNum num; } u;
    u.key = 0;
    u.num = pkgNum;
    return u.key;
    /*@=nullret =temptrans =retalias @*/
}

/**
 * Destroy available item index.
 * @param al		available list
 */
static void rpmalFreeIndex(rpmal al)
	/*@modifies al @*/
{
    availableIndex ai = &al->index;
    if (ai->size > 0) {
	ai->index = _free(ai->index);
	ai->size = 0;
    }
}

static void rpmalFini(void * _al)
{
    rpmal al = (rpmal) _al;
    availablePackage alp;
    int i;

    if ((alp = al->list) != NULL)
    for (i = 0; i < al->size; i++, alp++) {
	(void)rpmdsFree(alp->provides);
	alp->provides = NULL;
	(void)rpmbfFree(alp->bf);
	alp->bf = NULL;
    }

    al->list = _free(al->list);
    al->alloced = 0;
    rpmalFreeIndex(al);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmalPool;

static rpmal rpmalGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmdsPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmdsPool, fileSystem, internalState @*/
{
    rpmal al;

    if (_rpmalPool == NULL) {
	_rpmalPool = rpmioNewPool("al", sizeof(*al), -1, _rpmal_debug,
			NULL, NULL, rpmalFini);
	pool = _rpmalPool;
    }
    al = (rpmal) rpmioGetPool(pool, sizeof(*al));
    memset(((char *)al)+sizeof(al->_item), 0, sizeof(*al)-sizeof(al->_item));
    return al;
}

rpmal rpmalNew(int delta)
{
    rpmal al = rpmalGetPool(_rpmalPool);
    availableIndex ai = &al->index;

    al->delta = delta;
    al->size = 0;
    al->list = (availablePackage) xcalloc(al->delta, sizeof(*al->list));
    al->alloced = al->delta;

    ai->index = NULL;
    ai->size = 0;

    return rpmalLink(al, __FUNCTION__);
}

void rpmalDel(rpmal al, alKey pkgKey)
{
    alNum pkgNum = alKey2Num(al, pkgKey);
    availablePackage alp;

    if (al == NULL || al->list == NULL)
	return;		/* XXX can't happen */

    alp = al->list + pkgNum;

    (void)rpmdsFree(alp->provides);
    alp->provides = NULL;
    (void)rpmbfFree(alp->bf);
    alp->bf = NULL;

    memset(alp, 0, sizeof(*alp));	/* XXX trash and burn */
    return;
}

alKey rpmalAdd(rpmal * alistp, alKey pkgKey, fnpyKey key,
		rpmds provides, rpmfi fi, rpmuint32_t tscolor)
{
    alNum pkgNum;
    rpmal al;
    availablePackage alp;

    /* If list doesn't exist yet, create. */
    if (*alistp == NULL)
	*alistp = rpmalNew(5);
    al = *alistp;
    pkgNum = alKey2Num(al, pkgKey);

    if (pkgNum >= 0 && pkgNum < al->size) {
	rpmalDel(al, pkgKey);
    } else {
	if (al->size == al->alloced) {
	    al->alloced += al->delta;
	    al->list = (availablePackage) xrealloc(al->list, sizeof(*al->list) * al->alloced);
	}
	pkgNum = al->size++;
    }

    if (al->list == NULL)
	return RPMAL_NOMATCH;		/* XXX can't happen */

    alp = al->list + pkgNum;

    alp->key = key;
    alp->tscolor = tscolor;

/*@-assignexpose -castexpose @*/
    alp->provides = rpmdsLink(provides, "Provides (rpmalAdd)");
    alp->bf = rpmbfLink(rpmfiFNBF(fi));
/*@=assignexpose =castexpose @*/

    rpmalFreeIndex(al);

assert(((alNum)(alp - al->list)) == pkgNum);
    return ((alKey)(alp - al->list));
}

/**
 * Compare two available index entries by name (qsort/bsearch).
 * @param one		1st available index entry
 * @param two		2nd available index entry
 * @return		result of comparison
 */
static int indexcmp(const void * one, const void * two)
	/*@*/
{
    /*@-castexpose@*/
    const availableIndexEntry a = (const availableIndexEntry) one;
    const availableIndexEntry b = (const availableIndexEntry) two;
    /*@=castexpose@*/
    int lenchk;

    lenchk = a->entryLen - b->entryLen;
    if (lenchk)
	return lenchk;

    return strcmp(a->entry, b->entry);
}

void rpmalAddProvides(rpmal al, alKey pkgKey, rpmds provides, rpmuint32_t tscolor)
{
    rpmuint32_t dscolor;
    const char * Name;
    alNum pkgNum = alKey2Num(al, pkgKey);
    availableIndex ai = &al->index;
    availableIndexEntry aie;
    int ix;

    if (provides == NULL || pkgNum < 0 || pkgNum >= al->size)
	return;
    if (ai->index == NULL || ai->k < 0 || ai->k >= ai->size)
	return;

    if (rpmdsInit(provides) != NULL)
    while (rpmdsNext(provides) >= 0) {

	if ((Name = provides->N[provides->i]) == NULL)
	    continue;	/* XXX can't happen */

	/* Ignore colored provides not in our rainbow. */
	dscolor = rpmdsColor(provides);
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	aie = ai->index + ai->k;
	ai->k++;

	aie->pkgKey = pkgKey;
/*@-assignexpose@*/
	aie->entry = Name;
/*@=assignexpose@*/
	aie->entryLen = (unsigned short)strlen(Name);
	ix = rpmdsIx(provides);

/* XXX make sure that element index fits in unsigned short */
assert(ix < 0x10000);

	aie->entryIx = ix;
	aie->type = IET_PROVIDES;
    }
}

void rpmalMakeIndex(rpmal al)
{
    availableIndex ai;
    availablePackage alp;
    int i;

    if (al == NULL || al->list == NULL) return;
    ai = &al->index;

    ai->size = 0;
    for (i = 0; i < al->size; i++) {
	alp = al->list + i;
	if (alp->provides != NULL)
	    ai->size += rpmdsCount(alp->provides);
    }
    if (ai->size == 0) return;

    ai->index = (availableIndexEntry) xrealloc(ai->index, ai->size * sizeof(*ai->index));
    ai->k = 0;
    for (i = 0; i < al->size; i++) {
	alp = al->list + i;
	rpmalAddProvides(al, alNum2Key(NULL, (alNum)i), alp->provides, alp->tscolor);
    }

    /* Reset size to the no. of provides added. */
    ai->size = ai->k;
    qsort(ai->index, ai->size, sizeof(*ai->index), indexcmp);
}

fnpyKey *
rpmalAllFileSatisfiesDepend(const rpmal al, const rpmds ds, alKey * keyp)
{
    fnpyKey * ret = NULL;
    int found = 0;
    const char * fn;
    size_t nfn;
    int i;

    if (keyp) *keyp = RPMAL_NOMATCH;

    if (al == NULL || (fn = rpmdsN(ds)) == NULL || *fn != '/')
	goto exit;
    nfn = strlen(fn);

    if (al->list != NULL)	/* XXX always true */
    for (i = 0; i < al->size; i++) {
	availablePackage alp = al->list + i;

	if (!rpmbfChk(alp->bf, fn, nfn))
	    continue;

	rpmdsNotify(ds, _("(added files)"), 0);

	ret = (fnpyKey *) xrealloc(ret, (found + 2) * sizeof(*ret));
	if (ret)	/* can't happen */
	    ret[found] = alp->key;
	if (keyp)
	    *keyp = alNum2Key(al, i);
	found++;
    }

    if (ret)
	ret[found] = NULL;

exit:
/*@-nullstate@*/ /* FIX: *keyp may be NULL */
    return ret;
/*@=nullstate@*/
}

fnpyKey *
rpmalAllSatisfiesDepend(const rpmal al, const rpmds ds, alKey * keyp)
{
    availableIndex ai;
    availableIndexEntry needle;
    availableIndexEntry match;
    fnpyKey * ret = NULL;
    int found = 0;
    const char * KName;
    availablePackage alp;
    int rc;

    if (keyp) *keyp = RPMAL_NOMATCH;

    if (al == NULL || ds == NULL || (KName = rpmdsN(ds)) == NULL)
	goto exit;

    if (*KName == '/') {
	/* First, look for files "contained" in package ... */
	ret = rpmalAllFileSatisfiesDepend(al, ds, keyp);
	if (ret != NULL && *ret != NULL)
	    goto exit;
	ret = _free(ret);
	/* ... then, look for files "provided" by package. */
    }

    ai = &al->index;
    if (ai->index == NULL || ai->size <= 0)
	goto exit;

    needle = (availableIndexEntry)
	memset(alloca(sizeof(*needle)), 0, sizeof(*needle));
    /*@-assignexpose -temptrans@*/
    needle->entry = KName;
    /*@=assignexpose =temptrans@*/
    needle->entryLen = (unsigned short)strlen(needle->entry);

    match = (availableIndexEntry)
	bsearch(needle, ai->index, ai->size, sizeof(*ai->index), indexcmp);
    if (match == NULL)
	goto exit;

    /* rewind to the first match */
    while (match > ai->index && indexcmp(match-1, needle) == 0)
	match--;

    if (al->list != NULL)	/* XXX always true */
    for (ret = NULL, found = 0;
	 match < ai->index + ai->size && indexcmp(match, needle) == 0;
	 match++)
    {
	alp = al->list + alKey2Num(al, match->pkgKey);

	rc = 0;
	if (alp->provides != NULL)	/* XXX can't happen */
	switch (match->type) {
	case IET_PROVIDES:
	    /* XXX single step on rpmdsNext to regenerate DNEVR string */
	    (void) rpmdsSetIx(alp->provides, match->entryIx - 1);
	    if (rpmdsNext(alp->provides) >= 0)
		rc = rpmdsCompare(alp->provides, ds);

	    if (rc)
		rpmdsNotify(ds, _("(added provide)"), 0);

	    /*@switchbreak@*/ break;
	}

	if (rc) {
	    ret = (fnpyKey *) xrealloc(ret, (found + 2) * sizeof(*ret));
	    if (ret)	/* can't happen */
		ret[found] = alp->key;
/*@-dependenttrans@*/
	    if (keyp)
		*keyp = match->pkgKey;
/*@=dependenttrans@*/
	    found++;
	}
    }

    if (ret)
	ret[found] = NULL;

exit:
/*@-nullstate@*/ /* FIX: *keyp may be NULL */
    return ret;
/*@=nullstate@*/
}

fnpyKey
rpmalSatisfiesDepend(const rpmal al, const rpmds ds, alKey * keyp)
{
    fnpyKey * tmp = rpmalAllSatisfiesDepend(al, ds, keyp);

    if (tmp) {
	fnpyKey ret = tmp[0];
	free(tmp);
	return ret;
    }
    return NULL;
}
