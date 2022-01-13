/** \ingroup rpmdep
 * \file lib/rpmte.c
 * Routine(s) to handle an "rpmte"  transaction element.
 */
#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */

#include <rpmtypes.h>
#include <rpmtag.h>
#include <rpmdb.h>		/* rpmmiFoo */
#include <pkgio.h>		/* rpmReadPackageFile */

#include "rpmds.h"
#define _RPMFI_INTERNAL		/* pre/post trans scripts */
#include "rpmfi.h"

#define	_RPMTE_INTERNAL
#include "rpmte.h"
#include "rpmts.h"

#include "debug.h"

/*@unchecked@*/
int _rpmte_debug = 0;

/*@access alKey @*/
/*@access rpmts @*/	/* XXX cast */
/*@access rpmtsi @*/

#ifdef __cplusplus
GENfree(rpmtsi)
GENfree(sharedFileInfo)
GENfree(tsortInfo)
#endif	/* __cplusplus */

void rpmteCleanDS(rpmte te)
{
    te->PRCO = rpmdsFreePRCO(te->PRCO);
}

/**
 * Destroy transaction element data.
 * @param p		transaction element
 */
static void delTE(rpmte p)
	/*@globals fileSystem @*/
	/*@modifies p, fileSystem @*/
{
    p->relocs = rpmfiFreeRelocations(p->relocs);

    rpmteCleanDS(p);

/*@-refcounttrans@*/	/* FIX: XfdFree annotation */
    if (p->fd != NULL)
        p->fd = fdFree(p->fd, "delTE");
/*@=refcounttrans@*/

    p->os = _free(p->os);
    p->arch = _free(p->arch);
    p->epoch = _free(p->epoch);
    p->name = _free(p->name);
    p->version = _free(p->version);
    p->release = _free(p->release);
#ifdef	RPM_VENDOR_MANDRIVA
    p->distepoch = _free(p->distepoch);
#endif
    p->NEVR = _free(p->NEVR);
    p->NEVRA = _free(p->NEVRA);
    p->pkgid = _free(p->pkgid);
    p->hdrid = _free(p->hdrid);
    p->sourcerpm = _free(p->sourcerpm);

    p->replaced = _free(p->replaced);

    p->flink.NEVRA = argvFree(p->flink.NEVRA);
    p->flink.Pkgid = argvFree(p->flink.Pkgid);
    p->flink.Hdrid = argvFree(p->flink.Hdrid);
    p->blink.NEVRA = argvFree(p->blink.NEVRA);
    p->blink.Pkgid = argvFree(p->blink.Pkgid);
    p->blink.Hdrid = argvFree(p->blink.Hdrid);

assert(p->txn == NULL);		/* XXX FIXME */
    p->txn = NULL;
    p->fi = rpmfiFree(p->fi);

    (void)headerFree(p->h);
    p->h = NULL;

    /*@-nullstate@*/ /* FIX: p->{NEVR,name} annotations */
    return;
    /*@=nullstate@*/
}

/**
 * Initialize transaction element data from header.
 * @param ts		transaction set
 * @param p		transaction element
 * @param h		header
 * @param key		(TR_ADDED) package retrieval key (e.g. file name)
 * @param relocs	(TR_ADDED) package file relocations
 */
static void addTE(rpmts ts, rpmte p, Header h,
		/*@dependent@*/ /*@null@*/ fnpyKey key,
		/*@null@*/ rpmRelocation relocs)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, h,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int scareMem = 0;
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int xx;

    he->tag = RPMTAG_NVRA;
    xx = headerGet(h, he, 0);
assert(he->p.str != NULL);
    p->NEVR = (xx ? he->p.str : xstrdup("?N-?V-?R.?A"));

    he->tag = RPMTAG_NAME;
    xx = headerGet(h, he, 0);
    p->name = (xx ? he->p.str : xstrdup("?RPMTAG_NAME?"));
    he->tag = RPMTAG_VERSION;
    xx = headerGet(h, he, 0);
    p->version = (char *)(xx ? he->p.str : xstrdup("?RPMTAG_VERSION?"));
    he->tag = RPMTAG_RELEASE;
    xx = headerGet(h, he, 0);
    p->release = (char *)(xx ? he->p.str : xstrdup("?RPMTAG_RELEASE?"));
    
    p->db_instance = 0;

    he->tag = RPMTAG_PKGID;
    xx = headerGet(h, he, 0);
    if (he->p.ui8p != NULL) {
	static const char hex[] = "0123456789abcdef";
	char * t;
	rpmuint32_t i;

	p->pkgid = t = (char *) xmalloc((2*he->c) + 1);
	for (i = 0 ; i < he->c; i++) {
	    *t++ = hex[ (unsigned)((he->p.ui8p[i] >> 4) & 0x0f) ];
	    *t++ = hex[ (unsigned)((he->p.ui8p[i]     ) & 0x0f) ];
	}
	*t = '\0';
	he->p.ptr = _free(he->p.ptr);
    } else
	p->pkgid = NULL;

    he->tag = RPMTAG_HDRID;
    xx = headerGet(h, he, 0);
    p->hdrid = (xx ? he->p.str : xstrdup("?RPMTAG_HDRID?"));

    he->tag = RPMTAG_SOURCERPM;
    xx = headerGet(h, he, 0);
    p->sourcerpm = (xx ? he->p.str : NULL);

    he->tag = RPMTAG_ARCH;
    xx = headerGet(h, he, 0);
    p->arch = (xx ? he->p.str : xstrdup("?RPMTAG_ARCH?"));

    he->tag = RPMTAG_OS;
    xx = headerGet(h, he, 0);
    p->os = (xx ? he->p.str : xstrdup("?RPMTAG_OS?"));

    p->isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);

    p->NEVRA = xstrdup(p->NEVR);

    he->tag = RPMTAG_EPOCH;
    xx = headerGet(h, he, 0);
    if (he->p.ui32p != NULL) {
	p->epoch = (char *) xmalloc(20);
	sprintf(p->epoch, "%u", he->p.ui32p[0]);
	he->p.ptr = _free(he->p.ptr);
    } else
	p->epoch = NULL;

#ifdef	RPM_VENDOR_MANDRIVA
    he->tag = RPMTAG_DISTEPOCH;
    xx = headerGet(h, he, 0);
    if (he->p.str != NULL) {
	p->distepoch = (char*)(xx ? he->p.str : xstrdup("?RPMTAG_DISTEPOCH?"));
    } else
	p->distepoch = NULL;
#endif

    p->installed = 0;

    p->relocs = rpmfiDupeRelocations(relocs, &p->nrelocs);
    p->autorelocatex = -1;

    p->key = key;
    p->fd = NULL;

    p->replaced = NULL;

    p->pkgFileSize = 0;
    memset(p->originTid, 0, sizeof(p->originTid));
    memset(p->originTime, 0, sizeof(p->originTime));

    p->PRCO = rpmdsNewPRCO(h);

    {	rpmte savep = rpmtsSetRelocateElement(ts, p);
	p->fi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
	(void) rpmtsSetRelocateElement(ts, savep);
    }
    p->txn = NULL;

    rpmteColorDS(p, RPMTAG_PROVIDENAME);
    rpmteColorDS(p, RPMTAG_REQUIRENAME);
/*@-compdef@*/
    return;
/*@=compdef@*/
}

static void rpmteFini(void * _te)
	/*@modifies _te @*/
{
    rpmte te = (rpmte) _te;

    delTE(te);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmtePool;

static rpmte rpmteGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmtePool, fileSystem, internalState @*/
	/*@modifies pool, _rpmtePool, fileSystem, internalState @*/
{
    rpmte te;

    if (_rpmtePool == NULL) {
	_rpmtePool = rpmioNewPool("te", sizeof(*te), -1, _rpmte_debug,
			NULL, NULL, rpmteFini);
	pool = _rpmtePool;
    }
    te = (rpmte) rpmioGetPool(pool, sizeof(*te));
    memset(((char *)te)+sizeof(te->_item), 0, sizeof(*te)-sizeof(te->_item));
    return te;
}

rpmte rpmteNew(const rpmts ts, Header h,
		rpmElementType type,
		fnpyKey key,
		rpmRelocation relocs,
		uint32_t dboffset,
		alKey pkgKey)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmte p = rpmteGetPool(_rpmtePool);
    int xx;

    p->type = type;

    addTE(ts, p, h, key, relocs);
    switch (type) {
    case TR_ADDED:
	p->u.addedKey = pkgKey;
	/* XXX 256 is only an estimate of signature header. */
	p->pkgFileSize = 96 + 256;
	he->tag = RPMTAG_SIGSIZE;
	xx = headerGet(h, he, 0);
	if (xx && he->p.ui32p)
	    p->pkgFileSize += *he->p.ui32p;
	he->p.ptr = _free(he->p.ptr);
	break;
    case TR_REMOVED:
	p->u.addedKey = pkgKey;
	p->u.removed.dboffset = dboffset;
	break;
    }
    return p;
}

/* Get the DB Instance value */
uint32_t rpmteDBInstance(rpmte te) 
{
    return (te != NULL ? te->db_instance : 0);
}

/* Set the DB Instance value */
void rpmteSetDBInstance(rpmte te, unsigned int instance) 
{
    if (te != NULL) 
	te->db_instance = instance;
}

Header rpmteHeader(rpmte te)
{
/*@-castexpose -retalias -retexpose @*/
    return (te != NULL && te->h != NULL ? headerLink(te->h) : NULL);
/*@=castexpose =retalias =retexpose @*/
}

Header rpmteSetHeader(rpmte te, Header h)
{
    if (te != NULL)  {
	(void)headerFree(te->h);
	te->h = NULL;
/*@-assignexpose -castexpose @*/
	if (h != NULL)
	    te->h = headerLink(h);
/*@=assignexpose =castexpose @*/
    }
    return NULL;
}

rpmElementType rpmteType(rpmte te)
{
    return (te != NULL ? te->type : (rpmElementType)-1);
}

const char * rpmteN(rpmte te)
{
    return (te != NULL ? te->name : NULL);
}

const char * rpmteE(rpmte te)
{
    return (te != NULL ? te->epoch : NULL);
}

const char * rpmteV(rpmte te)
{
    return (te != NULL ? te->version : NULL);
}

const char * rpmteR(rpmte te)
{
    return (te != NULL ? te->release : NULL);
}

const char * rpmteD(rpmte te)
{
#ifdef	RPM_VENDOR_MANDRIVA
    return (te != NULL ? te->distepoch : NULL);
#else
    return NULL;
#endif
}

const char * rpmteA(rpmte te)
{
    return (te != NULL ? te->arch : NULL);
}

const char * rpmteO(rpmte te)
{
    return (te != NULL ? te->os : NULL);
}

int rpmteIsSource(rpmte te)
{
    return (te != NULL ? te->isSource : 0);
}

rpmuint32_t rpmteColor(rpmte te)
{
    return (te != NULL ? te->color : 0);
}

rpmuint32_t rpmteSetColor(rpmte te, rpmuint32_t color)
{
    int ocolor = 0;
    if (te != NULL) {
	ocolor = te->color;
	te->color = color;
    }
    return ocolor;
}

rpmuint32_t rpmtePkgFileSize(rpmte te)
{
    return (te != NULL ? te->pkgFileSize : 0);
}

rpmuint32_t * rpmteOriginTid(rpmte te)
{
    return te->originTid;
}

rpmuint32_t * rpmteOriginTime(rpmte te)
{
    return te->originTime;
}

int rpmteDepth(rpmte te)
{
    return (te != NULL ? te->depth : 0);
}

int rpmteSetDepth(rpmte te, int ndepth)
{
    int odepth = 0;
    if (te != NULL) {
	odepth = te->depth;
	te->depth = ndepth;
    }
    return odepth;
}

int rpmteBreadth(rpmte te)
{
    return (te != NULL ? te->depth : 0);
}

int rpmteSetBreadth(rpmte te, int nbreadth)
{
    int obreadth = 0;
    if (te != NULL) {
	obreadth = te->breadth;
	te->breadth = nbreadth;
    }
    return obreadth;
}

int rpmteNpreds(rpmte te)
{
    return (te != NULL ? te->npreds : 0);
}

int rpmteSetNpreds(rpmte te, int npreds)
{
    int opreds = 0;
    if (te != NULL) {
	opreds = te->npreds;
	te->npreds = npreds;
    }
    return opreds;
}

int rpmteTree(rpmte te)
{
    return (te != NULL ? te->tree : 0);
}

int rpmteSetTree(rpmte te, int ntree)
{
    int otree = 0;
    if (te != NULL) {
	otree = te->tree;
	te->tree = ntree;
    }
    return otree;
}

rpmte rpmteParent(rpmte te)
{
    return (te != NULL ? te->parent : NULL);
}

rpmte rpmteSetParent(rpmte te, rpmte pte)
{
    rpmte opte = NULL;
    if (te != NULL) {
	opte = te->parent;
	/*@-assignexpose -temptrans@*/
	te->parent = pte;
	/*@=assignexpose =temptrans@*/
    }
    return opte;
}

int rpmteDegree(rpmte te)
{
    return (te != NULL ? te->degree : 0);
}

int rpmteSetDegree(rpmte te, int ndegree)
{
    int odegree = 0;
    if (te != NULL) {
	odegree = te->degree;
	te->degree = ndegree;
    }
    return odegree;
}

tsortInfo rpmteTSI(rpmte te)
{
    /*@-compdef -retalias -retexpose -usereleased @*/
    return te->tsi;
    /*@=compdef =retalias =retexpose =usereleased @*/
}

void rpmteFreeTSI(rpmte te)
{
    if (te != NULL && rpmteTSI(te) != NULL) {
	tsortInfo tsi;

	/* Clean up tsort remnants (if any). */
	while ((tsi = rpmteTSI(te)->tsi_next) != NULL) {
	    rpmteTSI(te)->tsi_next = tsi->tsi_next;
	    tsi->tsi_next = NULL;
	    tsi = _free(tsi);
	}
	te->tsi = _free(te->tsi);
    }
    /*@-nullstate@*/ /* FIX: te->tsi is NULL */
    return;
    /*@=nullstate@*/
}

void rpmteNewTSI(rpmte te)
{
    if (te != NULL) {
	rpmteFreeTSI(te);
	te->tsi = (tsortInfo) xcalloc(1, sizeof(*te->tsi));
    }
}

alKey rpmteAddedKey(rpmte te)
{
    return (te != NULL ? te->u.addedKey : RPMAL_NOMATCH);
}

alKey rpmteSetAddedKey(rpmte te, alKey npkgKey)
{
    alKey opkgKey = RPMAL_NOMATCH;
    if (te != NULL) {
	opkgKey = te->u.addedKey;
	te->u.addedKey = npkgKey;
    }
    return opkgKey;
}


int rpmteDBOffset(rpmte te)
{
    return (te != NULL ? te->u.removed.dboffset : 0);
}

const char * rpmteNEVR(rpmte te)
{
    return (te != NULL ? te->NEVR : NULL);
}

const char * rpmteNEVRA(rpmte te)
{
    return (te != NULL ? te->NEVRA : NULL);
}

const char * rpmtePkgid(rpmte te)
{
    return (te != NULL ? te->pkgid : NULL);
}

const char * rpmteHdrid(rpmte te)
{
    return (te != NULL ? te->hdrid : NULL);
}

const char * rpmteSourcerpm(rpmte te)
{
    return (te != NULL ? te->sourcerpm : NULL);
}

FD_t rpmteFd(rpmte te)
{
    /*@-compdef -refcounttrans -retalias -retexpose -usereleased @*/
    return (te != NULL ? te->fd : NULL);
    /*@=compdef =refcounttrans =retalias =retexpose =usereleased @*/
}

fnpyKey rpmteKey(rpmte te)
{
    return (te != NULL ? te->key : NULL);
}

rpmds rpmteDS(rpmte te, rpmTag tag)
{
    return (te != NULL ? rpmdsFromPRCO(te->PRCO, tag) : NULL);
}

#ifdef	REFERENCE
rpmfi rpmteFI(rpmte te)
{
    if (te == NULL)
	return NULL;

    return te->fi; /* XXX take fi reference here? */
}
#else
rpmfi rpmteFI(rpmte te, rpmTag tag)
{
    /*@-compdef -refcounttrans -retalias -retexpose -usereleased @*/
    if (te == NULL)
	return NULL;

    if (tag == RPMTAG_BASENAMES)
	return te->fi;
    else
	return NULL;
    /*@=compdef =refcounttrans =retalias =retexpose =usereleased @*/
}
#endif

rpmfi rpmteSetFI(rpmte te, rpmfi fi)
{
    if (te != NULL)  {
	te->fi = rpmfiFree(te->fi);
/*@-assignexpose -castexpose @*/
	if (fi != NULL)
	    te->fi = rpmfiLink(fi, __FUNCTION__);
/*@=assignexpose =castexpose @*/
    }
    return NULL;
}

void rpmteColorDS(rpmte te, rpmTag tag)
{
    rpmfi fi = rpmteFI(te, RPMTAG_BASENAMES);
    rpmds ds = rpmteDS(te, tag);
    char deptype = 'R';
    char mydt;
    const rpmuint32_t * ddict;
    rpmuint32_t * colors;
    rpmuint32_t * refs;
    rpmuint32_t val;
    int Count;
    size_t nb;
    unsigned ix;
    int ndx, i;

    if (!(te && (Count = rpmdsCount(ds)) > 0 && rpmfiFC(fi) > 0))
	return;

    switch (tag) {
    default:
	return;
	/*@notreached@*/ break;
    case RPMTAG_PROVIDENAME:
	deptype = 'P';
	break;
    case RPMTAG_REQUIRENAME:
	deptype = 'R';
	break;
    }

    nb = Count * sizeof(*colors);
    colors = (rpmuint32_t *) memset(alloca(nb), 0, nb);
    nb = Count * sizeof(*refs);
    refs = (rpmuint32_t *) memset(alloca(nb), -1, nb);

    /* Calculate dependency color and reference count. */
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	val = rpmfiFColor(fi);
	ddict = NULL;
	ndx = rpmfiFDepends(fi, &ddict);
	if (ddict != NULL)
	while (ndx-- > 0) {
	    ix = *ddict++;
	    mydt = (char)((ix >> 24) & 0xff);
	    if (mydt != deptype)
		/*@innercontinue@*/ continue;
	    ix &= 0x00ffffff;
assert ((int)ix < Count);
	    colors[ix] |= val;
	    refs[ix]++;
	}
    }

    /* Set color/refs values in dependency set. */
    ds = rpmdsInit(ds);
    while ((i = rpmdsNext(ds)) >= 0) {
	val = colors[i];
	te->color |= val;
	(void) rpmdsSetColor(ds, val);
	val = refs[i];
	(void) rpmdsSetRefs(ds, val);
    }
}

/*@unchecked@*/
static int __mydebug = 0;

int rpmteChain(rpmte p, rpmte q, Header oh, const char * msg)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * blinkNEVRA = NULL;
    const char * blinkPkgid = NULL;
    const char * blinkHdrid = NULL;
    int xx;

    if (msg == NULL)
	msg = "";
    he->tag = RPMTAG_NVRA;
    xx = headerGet(oh, he, 0);
assert(he->p.str != NULL);
    blinkNEVRA = he->p.str;

    /*
     * Convert binary pkgid to a string.
     * XXX Yum's borken conception of a "header" doesn't have signature
     * tags appended.
     */
    he->tag = RPMTAG_PKGID;
    xx = headerGet(oh, he, 0);
    if (xx && he->p.ui8p != NULL) {
	static const char hex[] = "0123456789abcdef";
	char * t;
	rpmuint32_t i;

	blinkPkgid = t = (char *) xmalloc((2*he->c) + 1);
	for (i = 0 ; i < he->c; i++) {
	    *t++ = hex[ ((he->p.ui8p[i] >> 4) & 0x0f) ];
	    *t++ = hex[ ((he->p.ui8p[i]     ) & 0x0f) ];
	}
	*t = '\0';
	he->p.ptr = _free(he->p.ptr);
    } else
	blinkPkgid = NULL;

    he->tag = RPMTAG_HDRID;
    xx = headerGet(oh, he, 0);
    blinkHdrid = he->p.str;

/*@-modfilesys@*/
if (__mydebug)
fprintf(stderr, "%s argvAdd(&q->flink.NEVRA, \"%s\")\n", msg, p->NEVRA);
	xx = argvAdd(&q->flink.NEVRA, p->NEVRA);
if (__mydebug)
fprintf(stderr, "%s argvAdd(&p->blink.NEVRA, \"%s\")\n", msg, blinkNEVRA);
	xx = argvAdd(&p->blink.NEVRA, blinkNEVRA);
if (__mydebug)
fprintf(stderr, "%s argvAdd(&q->flink.Pkgid, \"%s\")\n", msg, p->pkgid);
    if (p->pkgid != NULL)
	xx = argvAdd(&q->flink.Pkgid, p->pkgid);
if (__mydebug)
fprintf(stderr, "%s argvAdd(&p->blink.Pkgid, \"%s\")\n", msg, blinkPkgid);
    if (blinkPkgid != NULL)
	xx = argvAdd(&p->blink.Pkgid, blinkPkgid);
if (__mydebug)
fprintf(stderr, "%s argvAdd(&q->flink.Hdrid, \"%s\")\n", msg, p->hdrid);
    if (p->hdrid != NULL)
	xx = argvAdd(&q->flink.Hdrid, p->hdrid);
if (__mydebug)
fprintf(stderr, "%s argvAdd(&p->blink.Hdrid, \"%s\")\n", msg, blinkHdrid);
    if (blinkHdrid != NULL)
	xx = argvAdd(&p->blink.Hdrid, blinkHdrid);
/*@=modfilesys@*/

    blinkNEVRA = _free(blinkNEVRA);
    blinkPkgid = _free(blinkPkgid);
    blinkHdrid = _free(blinkHdrid);

    return 0;
}

int rpmtsiOc(rpmtsi tsi)
{
    return tsi->ocsave;
}

/*@-mustmod@*/
static void rpmtsiFini(void * _tsi)
	/*@modifies _tsi @*/
{
    rpmtsi tsi = (rpmtsi) _tsi;
/*@-internalglobs@*/
    (void)rpmtsFree(tsi->ts);
    tsi->ts = NULL;
/*@=internalglobs@*/
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmtsiPool;

static rpmtsi rpmtsiGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmtsiPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmtsiPool, fileSystem, internalState @*/
{
    rpmtsi tsi;

    if (_rpmtsiPool == NULL) {
	_rpmtsiPool = rpmioNewPool("tsi", sizeof(*tsi), -1, _rpmte_debug,
			NULL, NULL, rpmtsiFini);/* XXX _rpmtsi_debug? */
	pool = _rpmtsiPool;
    }
    tsi = (rpmtsi) rpmioGetPool(pool, sizeof(*tsi));
    memset(((char *)tsi)+sizeof(tsi->_item), 0, sizeof(*tsi)-sizeof(tsi->_item));
    return tsi;
}

rpmtsi XrpmtsiInit(rpmts ts, const char * fn, unsigned int ln)
{
    rpmtsi tsi = rpmtsiGetPool(_rpmtsiPool);

/*@-assignexpose -castexpose @*/
    tsi->ts = rpmtsLink(ts, "rpmtsi");
/*@=assignexpose =castexpose @*/
    tsi->reverse = 0;
    tsi->oc = (tsi->reverse ? (rpmtsNElements(ts) - 1) : 0);
    tsi->ocsave = tsi->oc;

    return (rpmtsi) rpmioLinkPoolItem((rpmioItem)tsi, "rpmtsiInit", fn, ln);
}

/**
 * Return next transaction element.
 * @param tsi		transaction element iterator
 * @return		transaction element, NULL on termination
 */
static /*@null@*/ /*@dependent@*/
rpmte rpmtsiNextElement(rpmtsi tsi)
	/*@modifies tsi @*/
{
    rpmte te = NULL;
    int oc = -1;

    if (tsi == NULL || tsi->ts == NULL || rpmtsNElements(tsi->ts) <= 0)
	return te;

    if (tsi->reverse) {
	if (tsi->oc >= 0)		oc = tsi->oc--;
    } else {
    	if (tsi->oc < rpmtsNElements(tsi->ts))	oc = tsi->oc++;
    }
    tsi->ocsave = oc;
    if (oc != -1)
	te = rpmtsElement(tsi->ts, oc);
    return te;
}

rpmte rpmtsiNext(rpmtsi tsi, rpmElementType type)
{
    rpmte te;

    while ((te = rpmtsiNextElement(tsi)) != NULL) {
	if (type == 0 || (te->type & type) != 0)
	    break;
    }
    return te;
}

/*@-mods@*/
Header rpmteDBHeader(rpmts ts, uint32_t rec)
{
    rpmmi mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES, &rec, sizeof(rec));
    Header h;

    /* iterator returns weak refs, grab hold of header */
    if ((h = rpmmiNext(mi)) != NULL)
	h = headerLink(h);
    mi = rpmmiFree(mi);
    return h;
}
/*@=mods@*/

Header rpmteFDHeader(rpmts ts, rpmte te)
{
    Header h = NULL;
    te->fd = (FD_t) rpmtsNotify(ts, te, RPMCALLBACK_INST_OPEN_FILE, 0, 0);
    if (te->fd != NULL) {
	rpmVSFlags nvsflags = (rpmVSFlags)
		(rpmtsVSFlags(ts) | RPMVSF_NEEDPAYLOAD);
	rpmVSFlags ovsflags = rpmtsSetVSFlags(ts, nvsflags);
	rpmRC pkgrc = rpmReadPackageFile(ts, rpmteFd(te), rpmteNEVR(te), &h);
	ovsflags = rpmtsSetVSFlags(ts, ovsflags);
	switch (pkgrc) {
	default:
	    (void) rpmteClose(te, ts, 1);
	    break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    break;
	}
    }
    return h;
}

/*@-mods@*/
int rpmteOpen(rpmte te, rpmts ts, int reload_fi)
{
    Header h = NULL;
    uint32_t instance;
    int rc = 0;	/* assume failure */

    if (te == NULL || ts == NULL)
	goto exit;

    instance = rpmteDBInstance(te);
    (void) rpmteSetHeader(te, NULL);

    switch (rpmteType(te)) {
    case TR_ADDED:
	h = instance ? rpmteDBHeader(ts, instance) : rpmteFDHeader(ts, te);
	break;
    case TR_REMOVED:
	h = rpmteDBHeader(ts, instance);
    	break;
    }
    if (h != NULL) {
#ifdef	REFERENCE
	if (reload_fi) {
	    te->fi = getFI(te, ts, h);
	}
#else
	if (reload_fi) {
	    static int scareMem = 0;
	    te->fi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
	}
#endif
	
	(void) rpmteSetHeader(te, h);
	h = headerFree(h);
	rc = 1;
    }

exit:
    return rc;
}
/*@=mods@*/

int rpmteClose(rpmte te, rpmts ts, int reset_fi)
{
    if (te == NULL || ts == NULL)
	return 0;

    switch (te->type) {
    case TR_ADDED:
	if (te->fd != NULL) {
	    void * ptr;
	    ptr = rpmtsNotify(ts, te, RPMCALLBACK_INST_CLOSE_FILE, 0, 0);
	    te->fd = NULL;
	}
	break;
    case TR_REMOVED:
	/* eventually we'll want notifications for erase open too */
	break;
    }
    (void) rpmteSetHeader(te, NULL);
    if (reset_fi)
	(void) rpmteSetFI(te, NULL);
    return 1;
}

int rpmteFailed(rpmte te)
{
    return (te != NULL) ? te->linkFailed : -1;
}

int rpmteHaveTransScript(rpmte te, rpmTag tag)
{
    rpmfi fi = (te ? te->fi : NULL);
    int rc = 0;

    switch (tag) {
    default:
	break;
    case RPMTAG_PRETRANS:
	if (fi)
	    rc = (fi->pretrans || fi->pretransprog) ? 1 : 0;
	break;
    case RPMTAG_POSTTRANS:
	if (fi)
	    rc = (fi->posttrans || fi->posttransprog) ? 1 : 0;
	break;
    }
    return rc;
}

#ifdef	REFERENCE
int rpmteMarkFailed(rpmte te, rpmts ts)
{
    rpmtsi pi = rpmtsiInit(ts);
    int rc = 0;
    rpmte p;

    te->failed = 1;
    /* XXX we can do a much better here than this... */
    while ((p = rpmtsiNext(pi, TR_REMOVED))) {
	if (rpmteDependsOn(p) == te) {
	    p->failed = 1;
	}
    }
    rpmtsiFree(pi);
    return rc;
}

rpmps rpmteProblems(rpmte te)
{
    return te ? te->probs : NULL;
}

rpmfs rpmteGetFileStates(rpmte te) {
    return te->fs;
}

rpmfs rpmfsNew(unsigned int fc, rpmElementType type) {
    rpmfs fs = (rpmfs) xmalloc(sizeof(*fs));
    fs->fc = fc;
    fs->replaced = NULL;
    fs->states = NULL;
    if (type == TR_ADDED) {
	fs->states = (sharedFileInfo) xmalloc(sizeof(*fs->states) * fs->fc);
	memset(fs->states, RPMFILE_STATE_NORMAL, fs->fc);
    }
    fs->actions = (rpmFIleAction *) xmalloc(fc * sizeof(*fs->actions));
    memset(fs->actions, FA_UNKNOWN, fc * sizeof(*fs->actions));
    fs->numReplaced = fs->allocatedReplaced = 0;
    return fs;
}

rpmfs rpmfsFree(rpmfs fs) {
    fs->replaced = _free(fs->replaced);
    fs->states = _free(fs->states);
    fs->actions = _free(fs->actions);

    fs = _free(fs);
    return fs;
}

rpm_count_t rpmfsFC(rpmfs fs) {
    return fs->fc;
}

void rpmfsAddReplaced(rpmfs fs, int pkgFileNum, int otherPkg, int otherFileNum)
{
    if (!fs->replaced) {
	fs->replaced = (sharedFileInfo) xcalloc(3, sizeof(*fs->replaced));
	fs->allocatedReplaced = 3;
    }
    if (fs->numReplaced>=fs->allocatedReplaced) {
	fs->allocatedReplaced += (fs->allocatedReplaced>>1) + 2;
	fs->replaced = (sharedFileInfo) xrealloc(fs->replaced, fs->allocatedReplaced*sizeof(*fs->replaced));
    }
    fs->replaced[fs->numReplaced].pkgFileNum = pkgFileNum;
    fs->replaced[fs->numReplaced].otherPkg = otherPkg;
    fs->replaced[fs->numReplaced].otherFileNum = otherFileNum;

    fs->numReplaced++;
}

sharedFileInfo rpmfsGetReplaced(rpmfs fs)
{
    if (fs && fs->numReplaced)
        return fs->replaced;
    else
        return NULL;
}

sharedFileInfo rpmfsNextReplaced(rpmfs fs , sharedFileInfo replaced)
{
    if (fs && replaced) {
        replaced++;
	if (replaced - fs->replaced < fs->numReplaced)
	    return replaced;
    }
    return NULL;
}

void rpmfsSetState(rpmfs fs, unsigned int ix, rpmfileState state)
{
    assert(ix < fs->fc);
    fs->states[ix] = state;
}

rpmfileState rpmfsGetState(rpmfs fs, unsigned int ix)
{
    assert(ix < fs->fc);
    if (fs->states) return fs->states[ix];
    return RPMFILE_STATE_MISSING;
}

rpm_fstate_t * rpmfsGetStates(rpmfs fs)
{
    return fs->states;
}

rpmFileAction rpmfsGetAction(rpmfs fs, unsigned int ix)
{
    rpmFileAction action;
    if (fs->actions != NULL && ix < fs->fc) {
	action = fs->actions[ix];
    } else {
	action = FA_UNKNOWN;
    }
    return action;
}

void rpmfsSetAction(rpmfs fs, unsigned int ix, rpmFileAction action)
{
    if (fs->actions != NULL && ix < fs->fc) {
	fs->actions[ix] = action;
    }
}
#endif	/* REFERENCE */
