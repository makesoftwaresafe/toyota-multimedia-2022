/** \ingroup rpmts
 * \file lib/order.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmlog.h>
#include <rpmmacro.h>       /* XXX %{_dependency_whiteout} */
#include <popt.h>

#include <rpmtypes.h>
#include <rpmtag.h>

#define	_RPMEVR_INTERNAL
#include <rpmds.h>
#include <rpmfi.h>

#define	_RPMTE_INTERNAL
#define	_RPMTS_ORDER_INTERNAL
#include <rpmte.h>
#define	_RPMTS_INTERNAL
#include <rpmts.h>

#include "debug.h"

typedef	rpmuint32_t rpm_color_t;

/*@access alKey @*/	/* XXX for reordering and RPMAL_NOMATCH assign */

/**
 */
typedef /*@abstract@*/ struct orderListIndex_s *	orderListIndex;
/*@access orderListIndex@*/

/**
 */
struct orderListIndex_s {
/*@dependent@*/
    alKey pkgKey;
    int orIndex;
};

/*
 * Strongly Connected Components
 * set of packages (indirectly) requiering each other
 */
struct scc_s {
    int count;		/* # of external requires this SCC has */
#if 0
    int qcnt;		/* # of external requires pointing to this SCC */
#endif
    int size;		/* # of members */
#ifdef	REFERENCE
    tsortInfo * members;
#else
    rpmte * members;
#endif	/* REFERENCE */
};

typedef struct scc_s * scc;

#ifdef	REFERENCE
struct relation_s {
    tsortInfo   rel_suc;	/* pkg requiring this package */
    rpmsenseFlags rel_flags;	/* accumulated flags of the requirements */
    struct relation_s * rel_next;
};

typedef struct relation_s * relation;

struct tsortInfo_s {
    rpmte te;
    int	     tsi_count;		/* #pkgs this pkg requires */
    int	     tsi_qcnt;		/* #pkgs requiring this package */
    int	     tsi_reqx;		/* requires Idx/mark as (queued/loop) */
    struct relation_s * tsi_relations;
    struct relation_s * tsi_forward_relations;
    tsortInfo tsi_suc;		/*  used for queuing (addQ) */
    int      tsi_SccIdx;	/* # of the SCC the node belongs to (1 for trivial SCCs) */
    int      tsi_SccLowlink;	/* used for SCC detection */
};
#endif	/* REFERENCE */

struct badDeps_s {
/*@observer@*/ /*@owned@*/ /*@null@*/
    const char * pname;
/*@observer@*/ /*@dependent@*/ /*@null@*/
    const char * qname;
};

#ifdef __cplusplus
GENfree(orderListIndex)
GENfree(rpmte *)
#endif	/* __cplusplus */

/*@unchecked@*/
static int badDepsInitialized;

/*@unchecked@*/ /*@only@*/ /*@null@*/
static struct badDeps_s * badDeps;

/**
 */
/*@-modobserver -observertrans @*/
static void freeBadDeps(void)
	/*@globals badDeps, badDepsInitialized @*/
	/*@modifies badDeps, badDepsInitialized @*/
{
    if (badDeps) {
	struct badDeps_s * bdp;
	/* bdp->qname is a pointer to pname so doesn't need freeing */
	for (bdp = badDeps; bdp->pname != NULL && bdp->qname != NULL; bdp++)
	    bdp->pname = _free(bdp->pname);
	badDeps = _free(badDeps);
    }
    badDepsInitialized = 0;
}
/*@=modobserver =observertrans @*/

/**
 * Check for dependency relations to be ignored.
 *
 * @param ts		transaction set
 * @param p		successor element (i.e. with Requires: )
 * @param q		predecessor element (i.e. with Provides: )
 * @return		1 if dependency is to be ignored.
 */
static int ignoreDep(const rpmts ts, const rpmte p, const rpmte q)
	/*@globals badDeps, badDepsInitialized,
		rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies badDeps, badDepsInitialized,
		rpmGlobalMacroContext, internalState @*/
{
    struct badDeps_s * bdp;

    if (!badDepsInitialized) {
	char * s = rpmExpand("%{?_dependency_whiteout}", NULL);
	const char ** av = NULL;
#ifdef	REFERENCE
	int msglvl = (rpmtsDFlags(ts) & RPMDEPS_FLAG_DEPLOOPS)
			? RPMLOG_WARNING : RPMLOG_DEBUG;
#else
	int anaconda = rpmtsDFlags(ts) & RPMDEPS_FLAG_ANACONDA;
	int msglvl = (anaconda || (rpmtsDFlags(ts) & RPMDEPS_FLAG_DEPLOOPS))
			? RPMLOG_WARNING : RPMLOG_DEBUG;
#endif
	int ac = 0;
	int i;

	if (s != NULL && *s != '\0'
	&& !(i = poptParseArgvString(s, &ac, (const char ***)&av))
	&& ac > 0 && av != NULL)
	{
	    bdp = badDeps = (struct badDeps_s *)xcalloc(ac+1, sizeof(*badDeps));
	    for (i = 0; i < ac; i++, bdp++) {
		char * pname, * qname;

		if (av[i] == NULL)
		    break;
		pname = xstrdup(av[i]);
		if ((qname = strchr(pname, '>')) != NULL)
		    *qname++ = '\0';
		bdp->pname = pname;
/*@-usereleased@*/
		bdp->qname = qname;
/*@=usereleased@*/
		rpmlog(msglvl,
			_("ignore package name relation(s) [%d]\t%s -> %s\n"),
			i, bdp->pname, (bdp->qname ? bdp->qname : "???"));
	    }
	    bdp->pname = NULL;
	    bdp->qname = NULL;
	}
	av = _free(av);
	s = _free(s);
	badDepsInitialized++;
    }

/*@-compdef@*/
    if (badDeps != NULL)
    for (bdp = badDeps; bdp->pname != NULL && bdp->qname != NULL; bdp++) {
	if (!strcmp(rpmteN(p), bdp->pname) && !strcmp(rpmteN(q), bdp->qname))
	    return 1;
    }
    return 0;
/*@=compdef@*/
}

#ifdef	REFERENCE
static void rpmTSIFree(tsortInfo tsi)
{
    relation rel;

    while (tsi->tsi_relations != NULL) {
	rel = tsi->tsi_relations;
	tsi->tsi_relations = tsi->tsi_relations->rel_next;
	rel = _free(rel);
    }
    while (tsi->tsi_forward_relations != NULL) {
	rel = tsi->tsi_forward_relations;
	tsi->tsi_forward_relations = \
	    tsi->tsi_forward_relations->rel_next;
	rel = _free(rel);
    }
}

static inline int addSingleRelation(rpmte p,
				    rpmte q,
				    rpmsenseFlags dsflags)
{
    struct tsortInfo_s *tsi_p, *tsi_q;
    relation rel;
    rpmElementType teType = rpmteType(p);
    rpmsenseFlags flags;

    /* Avoid deps outside this transaction and self dependencies */
    if (q == NULL || q == p)
	return 0;

    /* Erasures are reversed installs. */
    if (teType == TR_REMOVED) {
	rpmte r = p;
	p = q;
	q = r;
	flags = isErasePreReq(dsflags);
    } else {
	flags = isInstallPreReq(dsflags);
    }

    /* map legacy prereq to pre/preun as needed */
    if (isLegacyPreReq(dsflags)) {
	flags |= (teType == TR_ADDED) ?
	    RPMSENSE_SCRIPT_PRE : RPMSENSE_SCRIPT_PREUN;
    }

    tsi_p = rpmteTSI(p);
    tsi_q = rpmteTSI(q);

    /* if relation got already added just update the flags */
    if (tsi_q->tsi_relations && tsi_q->tsi_relations->rel_suc == tsi_p) {
	tsi_q->tsi_relations->rel_flags |= flags;
	tsi_p->tsi_forward_relations->rel_flags |= flags;
	return 0;
    }

    /* Record next "q <- p" relation (i.e. "p" requires "q"). */
    if (p != q) {
	/* bump p predecessor count */
	tsi_p->tsi_count++;
    }

    rel = (relation) xcalloc(1, sizeof(*rel));
    rel->rel_suc = tsi_p;
    rel->rel_flags = flags;

    rel->rel_next = tsi_q->tsi_relations;
    tsi_q->tsi_relations = rel;
    if (p != q) {
	/* bump q successor count */
	tsi_q->tsi_qcnt++;
    }

    rel = (relation) xcalloc(1, sizeof(*rel));
    rel->rel_suc = tsi_q;
    rel->rel_flags = flags;

    rel->rel_next = tsi_p->tsi_forward_relations;
    tsi_p->tsi_forward_relations = rel;

    return 0;
}
#endif	/* REFERENCE */

/**
 * Recursively mark all nodes with their predecessors.
 * @param tsi		successor chain
 * @param q		predecessor
 */
static void markLoop(/*@special@*/ tsortInfo tsi, rpmte q)
	/*@globals internalState @*/
	/*@uses tsi @*/
	/*@modifies internalState @*/
{
    rpmte p;

    while (tsi != NULL && (p = tsi->tsi_suc) != NULL) {
	tsi = tsi->tsi_next;
	if (rpmteTSI(p)->tsi_chain != NULL)
	    continue;
	/*@-assignexpose -temptrans@*/
	rpmteTSI(p)->tsi_chain = q;
	/*@=assignexpose =temptrans@*/
	if (rpmteTSI(p)->tsi_next != NULL)
	    markLoop(rpmteTSI(p)->tsi_next, p);
    }
}

/*
 * Return display string a dependency, adding contextual flags marker.
 * @param f		dependency flags
 * @return		display string
 */
static inline /*@observer@*/ const char * identifyDepend(rpmuint32_t f)
	/*@*/
{
    f = _notpre(f);
    if (f & RPMSENSE_SCRIPT_PRE)
	return "Requires(pre):";
    if (f & RPMSENSE_SCRIPT_POST)
	return "Requires(post):";
    if (f & RPMSENSE_SCRIPT_PREUN)
	return "Requires(preun):";
    if (f & RPMSENSE_SCRIPT_POSTUN)
	return "Requires(postun):";
    if (f & RPMSENSE_SCRIPT_VERIFY)
	return "Requires(verify):";
    if (f & RPMSENSE_MISSINGOK)
	return "Requires(hint):";
    if (f & RPMSENSE_FIND_REQUIRES)
	return "Requires(auto):";
    return "Requires:";
}

/**
 * Find (and eliminate co-requisites) "q <- p" relation in dependency loop.
 * Search all successors of q for instance of p. Format the specific relation,
 * (e.g. p contains "Requires: q"). Unlink and free co-requisite (i.e.
 * pure Requires: dependencies) successor node(s).
 * @param q		successor (i.e. package required by p)
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param zap		max. no. of co-requisites to remove (-1 is all)?
 * @retval nzaps	address of no. of relations removed
 * @param msglvl	message level at which to spew
 * @return		(possibly NULL) formatted "q <- p" releation (malloc'ed)
 */
/*@-mustmod@*/ /* FIX: hack modifies, but -type disables */
static /*@owned@*/ /*@null@*/ const char *
zapRelation(rpmte q, rpmte p,
		int zap, /*@in@*/ /*@out@*/ int * nzaps, int msglvl)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies q, p, *nzaps, rpmGlobalMacroContext, internalState @*/
{
    rpmds requires;
    tsortInfo tsi_prev;
    tsortInfo tsi;
    const char *dp = NULL;

    for (tsi_prev = rpmteTSI(q), tsi = rpmteTSI(q)->tsi_next;
	 tsi != NULL;
	/* XXX Note: the loop traverses "not found", break on "found". */
	/*@-nullderef@*/
	 tsi_prev = tsi, tsi = tsi->tsi_next)
	/*@=nullderef@*/
    {
	rpmuint32_t Flags;

	/*@-abstractcompare@*/
	if (tsi->tsi_suc != p)
	    continue;
	/*@=abstractcompare@*/

	requires = rpmteDS((rpmteType(p) == TR_REMOVED ? q : p), tsi->tsi_tagn);
	if (requires == NULL) continue;		/* XXX can't happen */

	(void) rpmdsSetIx(requires, tsi->tsi_reqx);

	Flags = rpmdsFlags(requires);

	dp = rpmdsNewDNEVR( identifyDepend(Flags), requires);

	/*
	 * Attempt to unravel a dependency loop by eliminating Requires's.
	 */
	if (zap) {
	    rpmlog(msglvl,
			_("removing %s \"%s\" from tsort relations.\n"),
			(rpmteNEVRA(p) ?  rpmteNEVRA(p) : "???"), dp);
	    rpmteTSI(p)->tsi_count--;
	    if (tsi_prev) tsi_prev->tsi_next = tsi->tsi_next;
	    tsi->tsi_next = NULL;
	    tsi->tsi_suc = NULL;
	    tsi = _free(tsi);
	    if (nzaps)
		(*nzaps)++;
	    if (zap)
		zap--;
	}
	/* XXX Note: the loop traverses "not found", get out now! */
	break;
    }
    return dp;
}
/*@=mustmod@*/

#ifdef	REFERENCE
/**
 * Record next "q <- p" relation (i.e. "p" requires "q").
 * @param ts		transaction set
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param requires	relation
 * @return		0 always
 */
static inline int addRelation(rpmts ts,
			      rpmal al,
			      rpmte p,
			      rpmds requires)
{
    rpmte q;
    ARGV_const_t qcolls;
    rpmsenseFlags dsflags;

    dsflags = rpmdsFlags(requires);

    /* Avoid dependendencies which are not relevant for ordering */
    if (dsflags & (RPMSENSE_RPMLIB|RPMSENSE_CONFIG|RPMSENSE_PRETRANS))
	return 0;

    q = rpmalSatisfiesDepend(al, requires);

    /* Avoid deps outside this transaction and self dependencies */
    if (q == NULL || q == p)
	return 0;

    /* Avoid certain dependency relations. */
    if (ignoreDep(ts, p, q))
	return 0;

    addSingleRelation(p, q, dsflags);

    /* If q is a member of any collections, make sure p requires all packages
     * that are also in those collections */
    for (qcolls = rpmteCollections(q); qcolls && *qcolls; qcolls++) {
	rpmte *te;
	rpmte *tes = rpmalAllInCollection(al, *qcolls);
	for (te = tes; te && *te; te++) {
	    addSingleRelation(p, *te, RPMSENSE_SCRIPT_PRE);
	}
	_free(tes);
    }

    return 0;
}
#endif	/* REFERENCE */

/**
 * Record next "q <- p" relation (i.e. "p" requires "q").
 * @param ts		transaction set
 * @param al
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param requires	relation
 * @return		0 always
 */
static inline int orgrpmAddRelation(rpmts ts,
			      rpmal al,
			      rpmte p,
			      rpmds requires)
{
    rpmte q;
    struct tsortInfo_s * tsi_p, * tsi_q;
    relation rel;
    const char * N = rpmdsN(requires);
    rpmElementType teType = rpmteType(p);
    rpmsenseFlags flags;
    rpmsenseFlags dsflags;

#ifdef	REFERENCE
    if (N == NULL)
	return 0;

    dsflags = rpmdsFlags(requires);

    /* Avoid rpmlib feature and package config dependencies */
    if (dsflags & (RPMSENSE_RPMLIB|RPMSENSE_CONFIG))
	return 0;

    q = rpmalSatisfiesDepend(al, requires);
#else
    rpmtsi qi;
    fnpyKey key;
    alKey pkgKey;
    int i = 0;

    dsflags = rpmdsFlags(requires);

    /* Avoid certain NS dependencies. */
    switch (rpmdsNSType(requires)) {
    default:
	break;
    case RPMNS_TYPE_RPMLIB:
    case RPMNS_TYPE_CONFIG:
    case RPMNS_TYPE_CPUINFO:
    case RPMNS_TYPE_GETCONF:
    case RPMNS_TYPE_UNAME:
    case RPMNS_TYPE_SONAME:
    case RPMNS_TYPE_ACCESS:
    case RPMNS_TYPE_USER:
    case RPMNS_TYPE_GROUP:
    case RPMNS_TYPE_MOUNTED:
    case RPMNS_TYPE_DISKSPACE:
    case RPMNS_TYPE_DIGEST:
    case RPMNS_TYPE_GNUPG:
    case RPMNS_TYPE_MACRO:
    case RPMNS_TYPE_ENVVAR:
    case RPMNS_TYPE_RUNNING:
    case RPMNS_TYPE_SANITY:
    case RPMNS_TYPE_VCHECK:
    case RPMNS_TYPE_SIGNATURE:
	return 0;
	/*@notreached@*/ break;
    }

#ifndef	NOTYET
    /* Avoid looking up files/directories that are "owned" by _THIS_ package. */
    if (*N == '/') {
	rpmfi fi = rpmteFI(p, RPMTAG_BASENAMES);
	rpmbf bf = rpmfiFNBF(fi);
	if (bf != NULL && rpmbfChk(bf, N, strlen(N)) > 0)
	    return 0;
    }
#endif

    pkgKey = RPMAL_NOMATCH;
    key = rpmalSatisfiesDepend(al, requires, &pkgKey);

    /* Ordering depends only on added/erased package relations. */
    if (pkgKey == RPMAL_NOMATCH)
	return 0;

/* XXX Set q to the added/removed package that was found. */
    /* XXX pretend erasedPackages are just appended to addedPackages. */
    if (teType == TR_REMOVED)
	pkgKey = (alKey)(((long)pkgKey) + ts->numAddedPackages);

    for (qi = rpmtsiInit(ts), i = 0; (q = rpmtsiNext(qi, 0)) != NULL; i++) {
	if (pkgKey == rpmteAddedKey(q))
	    break;
    }
    qi = rpmtsiFree(qi);
#endif

    /* Avoid deps outside this transaction and self dependencies */
    if (q == NULL || q == p)
	return 0;

    /* Avoid certain dependency relations. */
    if (ignoreDep(ts, p, q))
	return 0;

    /* Erasures are reversed installs. */
    if (teType == TR_REMOVED) {
        rpmte r = p;
        p = q;
        q = r;
	flags = isErasePreReq(dsflags);
    } else {
	flags = isInstallPreReq(dsflags);
    }

#ifndef	DYING
#define	isLegacyPreReq(_x)  (((_x) & _ALL_REQUIRES_MASK) == RPMSENSE_PREREQ)
    /* map legacy prereq to pre/preun as needed */
    if (isLegacyPreReq(dsflags)) {
	flags |= (teType == TR_ADDED) ?
		 RPMSENSE_SCRIPT_PRE : RPMSENSE_SCRIPT_PREUN;
    }
#undef	isLegacyPreReq
#endif

    tsi_p = rpmteTSI(p);
    tsi_q = rpmteTSI(q);

    /* if relation got already added just update the flags */
    if (tsi_q->tsi_relations && tsi_q->tsi_relations->rel_suc == p) {
	tsi_q->tsi_relations->rel_flags |= flags;
	tsi_p->tsi_forward_relations->rel_flags |= flags;
	return 0;
    }

    /* Record next "q <- p" relation (i.e. "p" requires "q"). */
    if (p != q) {
	/* bump p predecessor count */
	rpmteTSI(p)->tsi_count++;
    }

    /* Save max. depth in dependency tree */
    if (rpmteDepth(p) <= rpmteDepth(q))
	(void) rpmteSetDepth(p, (rpmteDepth(q) + 1));
    if (rpmteDepth(p) > ts->maxDepth)
	ts->maxDepth = rpmteDepth(p);

    rel = (relation) xcalloc(1, sizeof(*rel));
    rel->rel_suc = p;
    rel->rel_flags = flags;

    rel->rel_next = rpmteTSI(q)->tsi_relations;
    rpmteTSI(q)->tsi_relations = rel;
    if (p != q) {
	/* bump q successor count */
	rpmteTSI(q)->tsi_qcnt++;
    }

    rel = (relation) xcalloc(1, sizeof(*rel));
    rel->rel_suc = q;
    rel->rel_flags = flags;

    rel->rel_next = rpmteTSI(p)->tsi_forward_relations;
    rpmteTSI(p)->tsi_forward_relations = rel;

    return 0;
}

/**
 * Record next "q <- p" relation (i.e. "p" requires "q").
 * @param ts		transaction set
 * @param al		added/erased package index
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param selected	boolean package selected array
 * @param requires	relation
 * @return		0 always
 */
/*@-mustmod@*/
static inline int addRelation(rpmts ts, rpmal al,
		/*@dependent@*/ rpmte p,
		unsigned char * selected,
		rpmds requires)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, *selected, rpmGlobalMacroContext,
		fileSystem, internalState @*/
{
    rpmtsi qi; rpmte q;
    tsortInfo tsi;
    const char * N = rpmdsN(requires);
    fnpyKey key;
    int teType = rpmteType(p);
    alKey pkgKey;
    int i = 0;

    /* Avoid certain NS dependencies. */
    switch (rpmdsNSType(requires)) {
    default:
	break;
    case RPMNS_TYPE_RPMLIB:
    case RPMNS_TYPE_CONFIG:
    case RPMNS_TYPE_CPUINFO:
    case RPMNS_TYPE_GETCONF:
    case RPMNS_TYPE_UNAME:
    case RPMNS_TYPE_SONAME:
    case RPMNS_TYPE_ACCESS:
    case RPMNS_TYPE_USER:
    case RPMNS_TYPE_GROUP:
    case RPMNS_TYPE_MOUNTED:
    case RPMNS_TYPE_DISKSPACE:
    case RPMNS_TYPE_DIGEST:
    case RPMNS_TYPE_GNUPG:
    case RPMNS_TYPE_MACRO:
    case RPMNS_TYPE_ENVVAR:
    case RPMNS_TYPE_RUNNING:
    case RPMNS_TYPE_SANITY:
    case RPMNS_TYPE_VCHECK:
    case RPMNS_TYPE_SIGNATURE:
	return 0;
	/*@notreached@*/ break;
    }

    /* Avoid looking up files/directories that are "owned" by _THIS_ package. */
    if (*N == '/') {
	rpmfi fi = rpmteFI(p, RPMTAG_BASENAMES);
	rpmbf bf = rpmfiFNBF(fi);
	if (bf != NULL && rpmbfChk(bf, N, strlen(N)) > 0)
	    return 0;
    }

    pkgKey = RPMAL_NOMATCH;
    key = rpmalSatisfiesDepend(al, requires, &pkgKey);

    /* Ordering depends only on added/erased package relations. */
    if (pkgKey == RPMAL_NOMATCH)
	return 0;

/* XXX Set q to the added/removed package that was found. */
    /* XXX pretend erasedPackages are just appended to addedPackages. */
    if (teType == TR_REMOVED)
	pkgKey = (alKey)(((long)pkgKey) + ts->numAddedPackages);

    for (qi = rpmtsiInit(ts), i = 0; (q = rpmtsiNext(qi, 0)) != NULL; i++) {
	if (pkgKey == rpmteAddedKey(q))
	    break;
    }
    qi = rpmtsiFree(qi);
    if (q == NULL || i >= ts->orderCount)
	return 0;

    /* Avoid certain dependency relations. */
    if (ignoreDep(ts, p, q))
	return 0;

    /* Avoid redundant relations. */
    if (selected[i] != 0)
	return 0;
    selected[i] = 1;

    /* Erasures are reversed installs. */
    if (teType == TR_REMOVED) {
	rpmte r = p;
	p = q;
	q = r;
    }

    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
    rpmteTSI(p)->tsi_count++;			/* bump p predecessor count */

    if (rpmteDepth(p) <= rpmteDepth(q))	/* Save max. depth in dependency tree */
	(void) rpmteSetDepth(p, (rpmteDepth(q) + 1));
    if (rpmteDepth(p) > ts->maxDepth)
	ts->maxDepth = rpmteDepth(p);

    tsi = (tsortInfo) xcalloc(1, sizeof(*tsi));
    tsi->tsi_suc = p;

    tsi->tsi_tagn = rpmdsTagN(requires);
    tsi->tsi_reqx = rpmdsIx(requires);

    tsi->tsi_next = rpmteTSI(q)->tsi_next;
    rpmteTSI(q)->tsi_next = tsi;
    rpmteTSI(q)->tsi_qcnt++;			/* bump q successor count */

    return 0;
}
/*@=mustmod@*/

/**
 * Compare ordered list entries by index (qsort/bsearch).
 * @param one		1st ordered list entry
 * @param two		2nd ordered list entry
 * @return		result of comparison
 */
static int orderListIndexCmp(const void * one, const void * two)	/*@*/
{
    /*@-castexpose@*/
    long a = (long) ((const orderListIndex)one)->pkgKey;
    long b = (long) ((const orderListIndex)two)->pkgKey;
    /*@=castexpose@*/
    return (a - b);
}

#ifdef	REFERENCE
/**
 * Add element to list sorting by tsi_qcnt.
 * @param p		new element
 * @retval qp		address of first element
 * @retval rp		address of last element
 * @param prefcolor
 */
static void addQ(tsortInfo p, tsortInfo * qp, tsortInfo * rp,
		rpm_color_t prefcolor)
{
    tsortInfo q, qprev;

    /* Mark the package as queued. */
    p->tsi_reqx = 1;

    if ((*rp) == NULL) {	/* 1st element */
	/* FIX: double indirection */
	(*rp) = (*qp) = p;
	return;
    }

    /* Find location in queue using metric tsi_qcnt. */
    for (qprev = NULL, q = (*qp);
	 q != NULL;
	 qprev = q, q = q->tsi_suc)
    {
	/* XXX Insure preferred color first. */
	if (rpmteColor(p->te) != prefcolor && rpmteColor(p->te) != rpmteColor(q->te))
	    continue;

	if (q->tsi_qcnt <= p->tsi_qcnt)
	    break;
    }

    if (qprev == NULL) {	/* insert at beginning of list */
	p->tsi_suc = q;
	(*qp) = p;		/* new head */
    } else if (q == NULL) {	/* insert at end of list */
	qprev->tsi_suc = p;
	(*rp) = p;		/* new tail */
    } else {			/* insert between qprev and q */
	p->tsi_suc = q;
	qprev->tsi_suc = p;
    }
}
#endif	/* REFERENCE */

/**
 * Add element to list sorting by tsi_qcnt.
 * @param p		new element
 * @retval *qp		first element
 * @retval *rp		last element
 * @param prefcolor	preferred color
 */
static void addQ(/*@dependent@*/ rpmte p,
		/*@in@*/ /*@out@*/ rpmte * qp,
		/*@in@*/ /*@out@*/ rpmte * rp,
		rpmuint32_t prefcolor)
	/*@modifies p, *qp, *rp @*/
{
    rpmte q, qprev;

#ifdef	REFERENCE
    /* Mark the package as queued. */
    rpmteTSI(p)->tsi_reqx = 1;
#endif	/* REFERENCE */

    if ((*rp) == NULL) {	/* 1st element */
	(*rp) = (*qp) = p;
	return;
    }

    /* Find location in queue using metric tsi_qcnt. */
    for (qprev = NULL, q = (*qp);
	 q != NULL;
	 qprev = q, q = rpmteTSI(q)->tsi_suc)
    {
	/* XXX Insure preferred color first. */
	if (rpmteColor(p) != prefcolor && rpmteColor(p) != rpmteColor(q))
	    continue;

#ifdef	REFERENCE
#else	/* REFERENCE */
	/* XXX Insure removed after added. */
	if (rpmteType(p) == TR_REMOVED && rpmteType(p) != rpmteType(q))
	    continue;

	/* XXX Follow all previous generations in the queue. */
	if (rpmteTSI(p)->tsi_queued > rpmteTSI(q)->tsi_queued)
	    continue;
#endif	/* REFERENCE */

	/* XXX Within a generation, queue behind more "important". */
	if (rpmteTSI(q)->tsi_qcnt <= rpmteTSI(p)->tsi_qcnt)
	    break;
    }

    if (qprev == NULL) {	/* insert at beginning of list */
	rpmteTSI(p)->tsi_suc = q;
	(*qp) = p;		/* new head */
    } else if (q == NULL) {	/* insert at end of list */
	rpmteTSI(qprev)->tsi_suc = p;
	(*rp) = p;		/* new tail */
    } else {			/* insert between qprev and q */
	rpmteTSI(p)->tsi_suc = q;
	rpmteTSI(qprev)->tsi_suc = p;
    }
}

/*@unchecked@*/
#ifdef	NOTYET
static rpmuint32_t _autobits = _notpre(_ALL_REQUIRES_MASK);
#else
static rpmuint32_t _autobits = 0xffffffff;
#endif
#define isAuto(_x)	((_x) & _autobits)

typedef struct sccData_s {
    int index;			/* DFS node number counter */
#ifdef	REFERENCE
    tsortInfo *stack;		/* Stack of nodes */
#else
    rpmte * stack;		/* Stack of nodes */
#endif
    int stackcnt;		/* Stack top counter */
    scc SCCs;			/* Array of SCC's found */
    int sccCnt;			/* Number of SCC's found */
} * sccData;

#ifdef	REFERENCE
static void tarjan(sccData sd, tsortInfo tsi)
{
    tsortInfo tsi_q;
    relation rel;

    /* use negative index numbers */
    sd->index--;
    /* Set the depth index for p */
    tsi->tsi_SccIdx = sd->index;
    tsi->tsi_SccLowlink = sd->index;

    sd->stack[sd->stackcnt++] = tsi;                   /* Push p on the stack */
    for (rel = tsi->tsi_relations; rel != NULL; rel = rel->rel_next) {
	/* Consider successors of p */
	tsi_q = rel->rel_suc;
	if (tsi_q->tsi_SccIdx > 0)
	    /* Ignore already found SCCs */
	    continue;
	if (tsi_q->tsi_SccIdx == 0) {
	    /* Was successor q not yet visited? */
	    tarjan(sd, tsi_q);                       /* Recurse */
	    /* negative index numbers: use max as it is closer to 0 */
	    tsi->tsi_SccLowlink = (tsi->tsi_SccLowlink > tsi_q->tsi_SccLowlink)
		? tsi->tsi_SccLowlink : tsi_q->tsi_SccLowlink;
	} else {
	    tsi->tsi_SccLowlink = (tsi->tsi_SccLowlink > tsi_q->tsi_SccIdx)
		? tsi->tsi_SccLowlink : tsi_q->tsi_SccIdx;
	}
    }

    if (tsi->tsi_SccLowlink == tsi->tsi_SccIdx) {
	/* v is the root of an SCC? */
	if (sd->stack[sd->stackcnt-1] == tsi) {
	    /* ignore trivial SCCs */
	    tsi_q = sd->stack[--sd->stackcnt];
	    tsi_q->tsi_SccIdx = 1;
	} else {
	    int stackIdx = sd->stackcnt;
	    do {
		tsi_q = sd->stack[--stackIdx];
		tsi_q->tsi_SccIdx = sd->sccCnt;
	    } while (tsi_q != tsi);

	    stackIdx = sd->stackcnt;
	    do {
		tsi_q = sd->stack[--stackIdx];
		/* Calculate count for the SCC */
		sd->SCCs[sd->sccCnt].count += tsi_q->tsi_count;
		/* Subtract internal relations */
		for (rel=tsi_q->tsi_relations; rel != NULL; rel=rel->rel_next) {
		    if (rel->rel_suc != tsi_q
		     && rel->rel_suc->tsi_SccIdx == sd->sccCnt)
			sd->SCCs[sd->sccCnt].count--;
		}
	    } while (tsi_q != tsi);
	    sd->SCCs[sd->sccCnt].size = sd->stackcnt - stackIdx;
	    /* copy members */
	    sd->SCCs[sd->sccCnt].members =
			(tsortInfo *) xcalloc(sd->SCCs[sd->sccCnt].size, sizeof(tsortInfo));
	    memcpy(sd->SCCs[sd->sccCnt].members, sd->stack + stackIdx,
		   sd->SCCs[sd->sccCnt].size * sizeof(tsortInfo));
	    sd->stackcnt = stackIdx;
	    sd->sccCnt++;
	}
    }
}
#else	/* REFERENCE */
static void tarjan(sccData sd, rpmte p)
{
    tsortInfo tsi = rpmteTSI(p);
    rpmte q;
    tsortInfo tsi_q;
    relation rel;

    /* use negative index numbers */
    sd->index--;
    /* Set the depth index for p */
    tsi->tsi_SccIdx = sd->index;
    tsi->tsi_SccLowlink = sd->index;

    sd->stack[sd->stackcnt++] = p;                   /* Push p on the stack */
    for (rel = tsi->tsi_relations; rel != NULL; rel = rel->rel_next) {
	/* Consider successors of p */
	q = rel->rel_suc;
	tsi_q = rpmteTSI(q);
	if (tsi_q->tsi_SccIdx > 0)
	    /* Ignore already found SCCs */
	    continue;
	if (tsi_q->tsi_SccIdx == 0) {
	    /* Was successor q not yet visited? */
	    tarjan(sd, q);                       /* Recurse */
	    /* negative index numbers: use max as it is closer to 0 */
	    tsi->tsi_SccLowlink = (tsi->tsi_SccLowlink > tsi_q->tsi_SccLowlink)
		    ? tsi->tsi_SccLowlink : tsi_q->tsi_SccLowlink;
	} else {
	    tsi->tsi_SccLowlink = (tsi->tsi_SccLowlink > tsi_q->tsi_SccIdx)
		    ? tsi->tsi_SccLowlink : tsi_q->tsi_SccIdx;
	}
    }

    if (tsi->tsi_SccLowlink == tsi->tsi_SccIdx) {
	/* v is the root of an SCC? */
	if (sd->stack[sd->stackcnt-1] == p) {
	    /* ignore trivial SCCs */
	    q = sd->stack[--sd->stackcnt];
	    tsi_q = rpmteTSI(q);
	    tsi_q->tsi_SccIdx = 1;
	} else {
	    int stackIdx = sd->stackcnt;
	    do {
		q = sd->stack[--stackIdx];
		tsi_q = rpmteTSI(q);
		tsi_q->tsi_SccIdx = sd->sccCnt;
	    } while (q != p);

	    stackIdx = sd->stackcnt;
	    do {
		q = sd->stack[--stackIdx];
		tsi_q = rpmteTSI(q);
		/* Calculate count for the SCC */
		sd->SCCs[sd->sccCnt].count += tsi_q->tsi_count;
		/* Subtract internal relations */
		for (rel=tsi_q->tsi_relations; rel != NULL; rel=rel->rel_next) {
		    if (rel->rel_suc != q
		     && rpmteTSI(rel->rel_suc)->tsi_SccIdx == sd->sccCnt)
			sd->SCCs[sd->sccCnt].count--;
		}
	    } while (q != p);
	    sd->SCCs[sd->sccCnt].size = sd->stackcnt - stackIdx;
	    /* copy members */
	    sd->SCCs[sd->sccCnt].members =
			(rpmte *) xcalloc(sd->SCCs[sd->sccCnt].size, sizeof(rpmte));
	    memcpy(sd->SCCs[sd->sccCnt].members, sd->stack + stackIdx,
		   sd->SCCs[sd->sccCnt].size * sizeof(rpmte));
	    sd->stackcnt = stackIdx;
	    sd->sccCnt++;
	}
    }
}
#endif	/* REFERENCE */

/* Search for SCCs and return an array last entry has a .size of 0 */
#ifdef	REFERENCE
static scc detectSCCs(tsortInfo orderInfo, int nelem, int debugloops)
#else	/* REFERENCE */
static scc detectSCCs(rpmts ts)
#endif	/* REFERENCE */
{
int nelem = ts->orderCount;
    scc _SCCs = (scc) xcalloc(nelem+3, sizeof(*_SCCs));
#ifdef	REFERENCE
    tsortInfo *_stack = (tsortInfo *) xcalloc(nelem, sizeof(*_stack));
#else
    rpmte * _stack = (rpmte *) xcalloc(nelem , sizeof(*_stack));
#endif
    struct sccData_s sd = { 0, _stack, 0, _SCCs, 2 };

#ifdef	REFERENCE
    for (int i = 0; i < nelem; i++) {
	tsortInfo tsi = &orderInfo[i];
	/* Start a DFS at each node */
	if (tsi->tsi_SccIdx == 0)
	    tarjan(&sd, tsi);
    }
#else	/* REFERENCE */
    {	rpmtsi pi;
	rpmte p;

	pi = rpmtsiInit(ts);
	while ((p = rpmtsiNext(pi, 0)) != NULL) {
	    /* Start a DFS at each node */
	    if (rpmteTSI(p)->tsi_SccIdx == 0)
		tarjan(&sd, p);
	}
	pi = rpmtsiFree(pi);
    }
#endif	/* REFERENCE */

    sd.stack = _free(sd.stack);

    sd.SCCs = (scc) xrealloc(sd.SCCs, (sd.sccCnt+1)*sizeof(*sd.SCCs));

    /* Debug output */
    if (sd.sccCnt > 2) {
#ifdef	REFERENCE
int debugloops = (rpmtsDFlags(ts) & RPMDEPS_FLAG_DEPLOOPS);
	int msglvl = debugloops ?  RPMLOG_WARNING : RPMLOG_DEBUG;
#else	/* REFERENCE */
int debugloops = (rpmtsDFlags(ts) & (RPMDEPS_FLAG_ANACONDA|RPMDEPS_FLAG_DEPLOOPS));
	rpmlogLvl msglvl = debugloops ? RPMLOG_WARNING : RPMLOG_ERR;
	int i, j;
#endif	/* REFERENCE */

	rpmlog(msglvl, "%d Strongly Connected Components\n", sd.sccCnt-2);
	for (i = 2; i < sd.sccCnt; i++) {
	    rpmlog(msglvl, "SCC #%d: requires %d packages\n",
		   i, sd.SCCs[i].count);
	    /* loop over members */
	    for (j = 0; j < sd.SCCs[i].size; j++) {
#ifdef	REFERENCE
		tsortInfo member = SCCs[i].members[j];
		rpmlog(msglvl, "\t%s\n", rpmteNEVRA(member->te));
		/* show relations between members */
		relation rel = member->tsi_forward_relations;
		for (; rel != NULL; rel=rel->rel_next) {
		    if (rel->rel_suc->tsi_SccIdx!=i) continue;
		    rpmlog(msglvl, "\t\t%s %s\n",
			   rel->rel_flags ? "=>" : "->",
			   rpmteNEVRA(rel->rel_suc->te));
		}
#else	/* REFERENCE */
		/* show relations between members */
		rpmte member = sd.SCCs[i].members[j];
		rpmlog(msglvl, "\t%s\n", rpmteNEVRA(sd.SCCs[i].members[j]));
		relation rel = rpmteTSI(member)->tsi_forward_relations;
		for (; rel != NULL; rel=rel->rel_next) {
		    if (rpmteTSI(rel->rel_suc)->tsi_SccIdx!=i) continue;
		    rpmlog(msglvl, "\t\t%s %s\n",
			   rel->rel_flags ? "=>" : "->",
			   rpmteNEVRA(rel->rel_suc));
		}
#endif	/* REFERENCE */
	    }
	}
    }

    return sd.SCCs;
}

#ifdef	REFERENCE
static void collectTE(rpm_color_t prefcolor, tsortInfo q,
		      rpmte * newOrder, int * newOrderCount,
		      scc SCCs,
		      tsortInfo * queue_end,
		      tsortInfo * outer_queue,
		      tsortInfo * outer_queue_end)
{
    char deptypechar = (rpmteType(q->te) == TR_REMOVED ? '-' : '+');

    if (rpmIsDebug()) {
	int depth = 1;
	/* figure depth in tree for nice formatting */
	for (rpmte p = q->te; (p = rpmteParent(p)); depth++) {}
	rpmlog(RPMLOG_DEBUG, "%5d%5d%5d%5d %*s%c%s\n",
	       *newOrderCount, q->tsi_count, q->tsi_qcnt,
	       depth, (2 * depth), "",
	       deptypechar, rpmteNEVRA(q->te));
    }

    newOrder[*newOrderCount] = q->te;
    (*newOrderCount)++;

    /* T6. Erase relations. */
    for (relation rel = q->tsi_relations; rel != NULL; rel = rel->rel_next) {
	tsortInfo p = rel->rel_suc;
	/* ignore already collected packages */
	if (p->tsi_SccIdx == 0) continue;
	if (p == q) continue;

	if (p && (--p->tsi_count) == 0) {
	    (void) rpmteSetParent(p->te, q->te);

	    if (q->tsi_SccIdx > 1 && q->tsi_SccIdx != p->tsi_SccIdx) {
                /* Relation point outside of this SCC: add to outside queue */
		assert(outer_queue != NULL && outer_queue_end != NULL);
		addQ(p, outer_queue, outer_queue_end, prefcolor);
	    } else {
		addQ(p, &q->tsi_suc, queue_end, prefcolor);
	    }
	}
	if (p && p->tsi_SccIdx > 1 &&
	                 p->tsi_SccIdx != q->tsi_SccIdx) {
	    if (--SCCs[p->tsi_SccIdx].count == 0) {
		/* New SCC is ready, add this package as representative */
		(void) rpmteSetParent(p->te, q->te);

		if (outer_queue != NULL) {
		    addQ(p, outer_queue, outer_queue_end, prefcolor);
		} else {
		    addQ(p, &q->tsi_suc, queue_end, prefcolor);
		}
	    }
	}
    }
    q->tsi_SccIdx = 0;
}
#endif	/* REFERENCE */

static void collectTE(rpm_color_t prefcolor, rpmte q,
		      rpmte * newOrder, int * newOrderCount,
		      scc SCCs,
		      rpmte * queue_end,
		      rpmte * outer_queue,
		      rpmte * outer_queue_end)
{
    int treex = rpmteTree(q);
    int depth = rpmteDepth(q);
    char deptypechar = (rpmteType(q) == TR_REMOVED ? '-' : '+');
    tsortInfo tsi = rpmteTSI(q);
    relation rel;

    rpmlog(RPMLOG_DEBUG, "%5d%5d%5d%5d%5d %*s%c%s\n",
	   *newOrderCount, rpmteNpreds(q),
	   rpmteTSI(q)->tsi_qcnt,
	   treex, depth,
	   (2 * depth), "",
	   deptypechar,
	   (rpmteNEVRA(q) ? rpmteNEVRA(q) : "???"));

    (void) rpmteSetDegree(q, 0);

    newOrder[*newOrderCount] = q;
    (*newOrderCount)++;

    /* T6. Erase relations. */
    for (rel = rpmteTSI(q)->tsi_relations; rel != NULL; rel = rel->rel_next) {
	rpmte p = rel->rel_suc;
	tsortInfo p_tsi = rpmteTSI(p);

	/* ignore already collected packages */
	if (p_tsi->tsi_SccIdx == 0) continue;
	if (p == q) continue;

	if (p && (--p_tsi->tsi_count) == 0) {

	    (void) rpmteSetTree(p, treex);
	    (void) rpmteSetDepth(p, depth+1);
	    (void) rpmteSetParent(p, q);
	    (void) rpmteSetDegree(q, rpmteDegree(q)+1);

	    if (tsi->tsi_SccIdx > 1 && tsi->tsi_SccIdx != p_tsi->tsi_SccIdx) {
                /* Relation point outside of this SCC: add to outside queue */
		assert(outer_queue != NULL && outer_queue_end != NULL);
		addQ(p, outer_queue, outer_queue_end, prefcolor);
	    } else {
		addQ(p, &tsi->tsi_suc, queue_end, prefcolor);
	    }
	}
	if (p && p_tsi->tsi_SccIdx > 1 &&
	                 p_tsi->tsi_SccIdx != rpmteTSI(q)->tsi_SccIdx) {
	    if (--SCCs[p_tsi->tsi_SccIdx].count == 0) {
		/* New SCC is ready, add this package as representative */

		(void) rpmteSetTree(p, treex);
		(void) rpmteSetDepth(p, depth+1);
		(void) rpmteSetParent(p, q);
		(void) rpmteSetDegree(q, rpmteDegree(q)+1);

		if (outer_queue != NULL) {
		    addQ(p, outer_queue, outer_queue_end, prefcolor);
		} else {
		    addQ(p, &rpmteTSI(q)->tsi_suc, queue_end, prefcolor);
		}
	    }
	}
    }
    tsi->tsi_SccIdx = 0;
}

#ifdef	REFERENCE
static void collectSCC(rpm_color_t prefcolor, tsortInfo p_tsi,
		       rpmte * newOrder, int * newOrderCount,
		       scc SCCs, tsortInfo * queue_end)
{
    int sccNr = p_tsi->tsi_SccIdx;
    struct scc_s SCC = SCCs[sccNr];
    int i;
    int start, end;
    relation rel;

    /* remove p from the outer queue */
    tsortInfo outer_queue_start = p_tsi->tsi_suc;
    p_tsi->tsi_suc = NULL;

    /*
     * Run a multi source Dijkstra's algorithm to find relations
     * that can be zapped with least danger to pre reqs.
     * As weight of the edges is always 1 it is not necessary to
     * sort the vertices by distance as the queue gets them
     * already in order
    */

    /* can use a simple queue as edge weights are always 1 */
    tsortInfo * queue = (tsortInfo *) xmalloc((SCC.size+1) * sizeof(*queue));

    /*
     * Find packages that are prerequired and use them as
     * starting points for the Dijkstra algorithm
     */
    start = end = 0;
    for (i = 0; i < SCC.size; i++) {
	tsortInfo tsi = SCC.members[i];
	tsi->tsi_SccLowlink = INT_MAX;
	for (rel=tsi->tsi_forward_relations; rel != NULL; rel=rel->rel_next) {
	    if (rel->rel_flags && rel->rel_suc->tsi_SccIdx == sccNr) {
		if (rel->rel_suc != tsi) {
		    tsi->tsi_SccLowlink =  0;
		    queue[end++] = tsi;
		} else {
		    tsi->tsi_SccLowlink =  INT_MAX/2;
		}
		break;
	    }
	}
    }

    if (start == end) { /* no regular prereqs; add self prereqs to queue */
	for (i = 0; i < SCC.size; i++) {
	    tsortInfo tsi = SCC.members[i];
	    if (tsi->tsi_SccLowlink != INT_MAX) {
		queue[end++] = tsi;
	    }
	}
    }

    /* Do Dijkstra */
    while (start != end) {
	tsortInfo tsi = queue[start++];
	for (rel=tsi->tsi_forward_relations; rel != NULL; rel=rel->rel_next) {
	    tsortInfo next_tsi = rel->rel_suc;
	    if (next_tsi->tsi_SccIdx != sccNr) continue;
	    if (next_tsi->tsi_SccLowlink > tsi->tsi_SccLowlink+1) {
		next_tsi->tsi_SccLowlink = tsi->tsi_SccLowlink + 1;
		queue[end++] = rel->rel_suc;
	    }
	}
    }
    queue = _free(queue);


    while (1) {
	tsortInfo best = NULL;
	tsortInfo inner_queue_start, inner_queue_end;
	int best_score = 0;

	/* select best candidate to start with */
	for (int i = 0; i < SCC.size; i++) {
	    tsortInfo tsi = SCC.members[i];
	    if (tsi->tsi_SccIdx == 0) /* package already collected */
		continue;
	    if (tsi->tsi_SccLowlink >= best_score) {
		best = tsi;
		best_score = tsi->tsi_SccLowlink;
	    }
	}

	if (best == NULL) /* done */
	    break;

	/* collect best candidate and all packages that get freed */
	inner_queue_start = inner_queue_end = NULL;
	addQ(best, &inner_queue_start, &inner_queue_end, prefcolor);

	for (; inner_queue_start != NULL;
	     inner_queue_start = inner_queue_start->tsi_suc) {
	    /* Mark the package as unqueued. */
	    inner_queue_start->tsi_reqx = 0;
	    collectTE(prefcolor, inner_queue_start, newOrder, newOrderCount,
		      SCCs, &inner_queue_end, &outer_queue_start, queue_end);
	}
    }

    /* restore outer queue */
    p_tsi->tsi_suc = outer_queue_start;
}
#endif	/* REFERENCE */

static void collectSCC(rpm_color_t prefcolor, rpmte p,
		       rpmte * newOrder, int * newOrderCount,
		       scc SCCs, rpmte * queue_end)
{
    int sccNr = rpmteTSI(p)->tsi_SccIdx;
    struct scc_s SCC = SCCs[sccNr];
    int i;
    int start, end;
    relation rel;

    /* remove p from the outer queue */
    rpmte outer_queue_start = rpmteTSI(p)->tsi_suc;
    rpmteTSI(p)->tsi_suc = NULL;

    /*
     * Run a multi source Dijkstra's algorithm to find relations
     * that can be zapped with least danger to pre reqs.
     * As weight of the edges is always 1 it is not necessary to
     * sort the vertices by distance as the queue gets them
     * already in order
    */

    /* can use a simple queue as edge weights are always 1 */
    rpmte * queue = (rpmte *) xmalloc((SCC.size+1) * sizeof(rpmte));

    /*
     * Find packages that are prerequired and use them as
     * starting points for the Dijkstra algorithm
     */
    start = end = 0;
    for (i = 0; i < SCC.size; i++) {
	rpmte q = SCC.members[i];
	tsortInfo tsi = rpmteTSI(q);
	tsi->tsi_SccLowlink = INT_MAX;
	for (rel=tsi->tsi_forward_relations; rel != NULL; rel=rel->rel_next) {
	    if (rel->rel_flags && rpmteTSI(rel->rel_suc)->tsi_SccIdx == sccNr) {
		if (rel->rel_suc != q) {
		    tsi->tsi_SccLowlink =  0;
		    queue[end++] = q;
		} else {
		    tsi->tsi_SccLowlink =  INT_MAX/2;
		}
		break;
	    }
	}
    }

    if (start == end) { /* no regular prereqs; add self prereqs to queue */
	for (i = 0; i < SCC.size; i++) {
	    rpmte q = SCC.members[i];
	    tsortInfo tsi = rpmteTSI(q);
	    if (tsi->tsi_SccLowlink != INT_MAX) {
		queue[end++] = q;
	    }
	}
    }

    /* Do Dijkstra */
    while (start != end) {
	rpmte q = queue[start++];
	tsortInfo tsi = rpmteTSI(q);
	for (rel=tsi->tsi_forward_relations; rel != NULL; rel=rel->rel_next) {
	    tsortInfo next_tsi = rpmteTSI(rel->rel_suc);
	    if (next_tsi->tsi_SccIdx != sccNr) continue;
	    if (next_tsi->tsi_SccLowlink > tsi->tsi_SccLowlink+1) {
		next_tsi->tsi_SccLowlink = tsi->tsi_SccLowlink + 1;
		queue[end++] = rel->rel_suc;
	    }
	}
    }
    queue = _free(queue);


    while (1) {
	rpmte best = NULL;
	rpmte inner_queue_start, inner_queue_end;
	int best_score = 0;
	int i;

	/* select best candidate to start with */
	for (i = 0; i < SCC.size; i++) {
	    rpmte q = SCC.members[i];
	    tsortInfo tsi = rpmteTSI(q);
	    if (tsi->tsi_SccIdx == 0) /* package already collected */
		continue;
	    if (tsi->tsi_SccLowlink >= best_score) {
		best = q;
		best_score = tsi->tsi_SccLowlink;
	    }
	}

	if (best == NULL) /* done */
	    break;

	/* collect best candidate and all packages that get freed */
	inner_queue_start = inner_queue_end = NULL;
	addQ(best, &inner_queue_start, &inner_queue_end, prefcolor);

	for (; inner_queue_start != NULL;
	     inner_queue_start = rpmteTSI(inner_queue_start)->tsi_suc) {
	    /* Mark the package as unqueued. */
	    rpmteTSI(inner_queue_start)->tsi_reqx = 0;
	    collectTE(prefcolor, inner_queue_start, newOrder, newOrderCount,
		      SCCs, &inner_queue_end, &outer_queue_start, queue_end);
	}
    }

    /* restore outer queue */
    rpmteTSI(p)->tsi_suc = outer_queue_start;
}

#ifdef	REFERENCE
int rpmtsOrder(rpmts ts)
{
    tsMembers tsmem = rpmtsMembers(ts);
    rpm_color_t prefcolor = rpmtsPrefColor(ts);
    rpmtsi pi; rpmte p;
    tsortInfo q, r;
    rpmte * newOrder;
    int newOrderCount = 0;
    int rc;
    rpmal erasedPackages = rpmalCreate(5, rpmtsColor(ts), prefcolor);
    scc SCCs;
    int nelem = rpmtsNElements(ts);
    tsortInfo sortInfo = (tsortInfo) xcalloc(nelem, sizeof(struct tsortInfo_s));

    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    /* Create erased package index. */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_REMOVED)) != NULL) {
        rpmalAdd(erasedPackages, p);
    }
    pi = rpmtsiFree(pi);

    for (int i = 0; i < nelem; i++) {
	sortInfo[i].te = tsmem->order[i];
	rpmteSetTSI(tsmem->order[i], &sortInfo[i]);
    }

    /* Record relations. */
    rpmlog(RPMLOG_DEBUG, "========== recording tsort relations\n");
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL) {
	rpmal al = (rpmteType(p) == TR_REMOVED) ? 
		   erasedPackages : tsmem->addedPackages;
	rpmds requires = rpmdsInit(rpmteDS(p, RPMTAG_REQUIRENAME));

	while (rpmdsNext(requires) >= 0) {
	    /* Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) addRelation(ts, al, p, requires);
	}
    }
    pi = rpmtsiFree(pi);

    newOrder = (rpmte *) xcalloc(tsmem->orderCount, sizeof(*newOrder));
    SCCs = detectSCCs(sortInfo, nelem, (rpmtsFlags(ts) & RPMTRANS_FLAG_DEPLOOPS));

    rpmlog(RPMLOG_DEBUG, "========== tsorting packages (order, #predecessors, #succesors, depth)\n");

    for (int i = 0; i < 2; i++) {
	/* Do two separate runs: installs first - then erases */
	int oType = !i ? TR_ADDED : TR_REMOVED;
	q = r = NULL;
	/* Scan for zeroes and add them to the queue */
	for (int e = 0; e < nelem; e++) {
	    tsortInfo p = &sortInfo[e];
	    if (rpmteType(p->te) != oType) continue;
	    if (p->tsi_count != 0)
		continue;
	    p->tsi_suc = NULL;
	    addQ(p, &q, &r, prefcolor);
	}

	/* Add one member of each leaf SCC */
	for (int i = 2; SCCs[i].members != NULL; i++) {
	    tsortInfo member = SCCs[i].members[0];
	    if (SCCs[i].count == 0 && rpmteType(member->te) == oType) {
		addQ(member, &q, &r, prefcolor);
	    }
	}

	while (q != NULL) {
	    /* Mark the package as unqueued. */
	    q->tsi_reqx = 0;
	    if (q->tsi_SccIdx > 1) {
		collectSCC(prefcolor, q, newOrder, &newOrderCount, SCCs, &r);
	    } else {
		collectTE(prefcolor, q, newOrder, &newOrderCount, SCCs, &r,
			  NULL, NULL);
	    }
	    q = q->tsi_suc;
	}
    }

    /* Clean up tsort data */
    for (int i = 0; i < nelem; i++) {
	rpmteSetTSI(tsmem->order[i], NULL);
	rpmTSIFree(&sortInfo[i]);
    }
    free(sortInfo);

    assert(newOrderCount == tsmem->orderCount);

    tsmem->order = _free(tsmem->order);
    tsmem->order = newOrder;
    tsmem->orderAlloced = tsmem->orderCount;
    rc = 0;

    freeBadDeps();

    for (int i = 2; SCCs[i].members != NULL; i++) {
	free(SCCs[i].members);
    }
    free(SCCs);
    rpmalFree(erasedPackages);

    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    return rc;
}
#endif	/* REFERENCE */

int _orgrpmtsOrder(rpmts ts)
{
    rpm_color_t prefcolor = rpmtsPrefColor(ts);
    rpmtsi pi; rpmte p;
    rpmte q;
    rpmte r;
    rpmte * newOrder;
    int newOrderCount = 0;
    int treex;
    int rc;
#ifdef	REFERENCE
    rpmal erasedPackages = rpmalCreate(5, rpmtsColor(ts), prefcolor);
#endif
    scc SCCs;
    int j;
    int i;

    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    /*
     * XXX FIXME: this gets needlesly called twice on normal usage patterns,
     * should track the need for generating the index somewhere
     */
    rpmalMakeIndex(ts->addedPackages);

    /* Create erased package index. */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_REMOVED)) != NULL) {
#ifdef	REFERENCE
        rpmalAdd(ts->erasedPackages, p);
#else
	alKey pkgKey;
	fnpyKey key;
	rpmuint32_t tscolor = rpmtsColor(ts);
	pkgKey = RPMAL_NOMATCH;
/*@-abstract@*/
	key = (fnpyKey) p;
/*@=abstract@*/
	pkgKey = rpmalAdd(&ts->erasedPackages, pkgKey, key,
			rpmteDS(p, RPMTAG_PROVIDENAME),
			rpmteFI(p, RPMTAG_BASENAMES), tscolor);
	/* XXX pretend erasedPackages are just appended to addedPackages. */
	pkgKey = (alKey)(((long)pkgKey) + ts->numAddedPackages);
	(void) rpmteSetAddedKey(p, pkgKey);
#endif
    }
    pi = rpmtsiFree(pi);
    rpmalMakeIndex(ts->erasedPackages);

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL)
	rpmteNewTSI(p);
    pi = rpmtsiFree(pi);

    /* Record relations. */
    rpmlog(RPMLOG_DEBUG, "========== recording tsort relations\n");
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL) {
	rpmal al = (rpmteType(p) == TR_REMOVED) ? 
		   ts->erasedPackages : ts->addedPackages;
	rpmds requires = rpmdsInit(rpmteDS(p, RPMTAG_REQUIRENAME));

	while (rpmdsNext(requires) >= 0) {
	    /* Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) orgrpmAddRelation(ts, al, p, requires);
	}

#ifdef	NOTYET
	/* Ensure that erasures follow installs during upgrades. */
      if (rpmteType(p) == TR_REMOVED && p->flink.Pkgid && p->flink.Pkgid[0]) {
	rpmtsi qi;

	qi = rpmtsiInit(ts);
	while ((q = rpmtsiNext(qi, TR_ADDED)) != NULL) {

	    if (strcmp(q->pkgid, p->flink.Pkgid[0]))
		/*@innercontinue@*/ continue;

	    requires = rpmdsFromPRCO(q->PRCO, RPMTAG_NAME);
	    if (requires != NULL) {
		/* XXX disable erased arrow reversal. */
		p->type = TR_ADDED;
		(void) orgrpmAddRelation(ts, ts->addedPackages, p, requires);
		p->type = TR_REMOVED;
	    }
	}
	qi = rpmtsiFree(qi);
      }
#endif	/* NOTYET */

#ifdef	NOTYET
	/* Order by requiring parent directories as prerequisites. */
	requires = rpmdsInit(rpmteDS(p, RPMTAG_DIRNAMES));
	if (requires != NULL)
	while (rpmdsNext(requires) >= 0) {

#ifdef	DYING
	    /* XXX Attempt to avoid loops by filtering out deep paths. */
	    if (countSlashes(rpmdsN(requires)) > slashDepth)
		/*@innercontinue@*/ continue;
#endif

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) orgrpmAddRelation(ts, al, p, requires);

	}
#endif	/* NOTYET */

#ifdef	NOTYET
	/* Order by requiring no dangling symlinks. */
	requires = rpmdsInit(rpmteDS(p, RPMTAG_FILELINKTOS));
	if (requires != NULL)
	while (rpmdsNext(requires) >= 0) {

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) orgrpmAddRelation(ts, al, p, requires);

	}
#endif	/* NOTYET */

    }
    pi = rpmtsiFree(pi);

    /* Save predecessor count and mark tree roots. */
    treex = 0;
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL) {
	int npreds = rpmteTSI(p)->tsi_count;

	(void) rpmteSetNpreds(p, npreds);
	(void) rpmteSetDepth(p, 1);

	if (npreds == 0)
	    (void) rpmteSetTree(p, treex++);
	else
	    (void) rpmteSetTree(p, -1);
    }
    pi = rpmtsiFree(pi);
    ts->ntrees = treex;

    newOrder = (rpmte *) xcalloc(ts->orderCount, sizeof(*newOrder));
    SCCs = detectSCCs(ts);

    rpmlog(RPMLOG_DEBUG, "========== tsorting packages (order, #predecessors, #succesors, tree, depth)\n");

    for (j = 0; j < 2; j++) {
	/* Do two separate runs: installs first - then erases */
	unsigned oType = (j == 0 ? TR_ADDED : TR_REMOVED);
	q = r = NULL;
	pi = rpmtsiInit(ts);
	/* Scan for zeroes and add them to the queue */
	while ((p = rpmtsiNext(pi, oType)) != NULL) {
	    if (rpmteTSI(p)->tsi_count != 0)
		continue;
	    rpmteTSI(p)->tsi_suc = NULL;
	    addQ(p, &q, &r, prefcolor);
	}
	pi = rpmtsiFree(pi);

	/* Add one member of each leaf SCC */
	for (i = 2; SCCs[i].members != NULL; i++) {
	    if (SCCs[i].count == 0 && rpmteType(SCCs[i].members[0]) == oType) {
		p = SCCs[i].members[0];
		rpmteTSI(p)->tsi_suc = NULL;
		addQ(p, &q, &r, prefcolor);
	    }
	}

	/* Process queue */
	for (; q != NULL; q = rpmteTSI(q)->tsi_suc) {
	    /* Mark the package as unqueued. */
	    rpmteTSI(q)->tsi_reqx = 0;
	    if (rpmteTSI(q)->tsi_SccIdx > 1) {
		collectSCC(prefcolor, q, newOrder, &newOrderCount, SCCs, &r);
	    } else {
		collectTE(prefcolor, q, newOrder, &newOrderCount, SCCs, &r,
			  NULL, NULL);
	    }
	}
    }

    /* Clean up tsort data */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL)
	rpmteFreeTSI(p);
    pi = rpmtsiFree(pi);

    assert(newOrderCount == ts->orderCount);

    ts->order = _free(ts->order);
    ts->order = newOrder;
    ts->orderAlloced = ts->orderCount;
    rc = 0;

    for (i = 2; SCCs[i].members != NULL; i++)
	SCCs[i].members = _free(SCCs[i].members);
    SCCs = _free(SCCs);

#ifdef	DYING
    rpmalFree(ts->erasedPackages);
    ts->erasedPackages = NULL;
#endif
    freeBadDeps();

    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    return rc;
}

int _rpmtsOrder(rpmts ts)
{
    rpmds requires;
    rpmuint32_t Flags;

    rpmuint32_t prefcolor = rpmtsPrefColor(ts);
    rpmtsi pi; rpmte p;
    rpmtsi qi; rpmte q;
    rpmtsi ri; rpmte r;
    rpmte * newOrder;
    int newOrderCount = 0;
    int treex;
    int rc = -1;	/* assume failure */
    int i;
    int j;

    rpmal al;

#ifdef  REFERENCE
    rpmal erasedPackages = rpmalCreate(5, rpmtsColor(ts), prefcolor);
    scc SCCs;
#else	/* REFERENCE */
    rpmuint32_t tscolor = rpmtsColor(ts);
    int anaconda = rpmtsDFlags(ts) & RPMDEPS_FLAG_ANACONDA;

    tsortInfo tsi;
    tsortInfo tsi_next;
    alKey * ordering;
    int orderingCount = 0;

    unsigned char * selected = (unsigned char *) alloca(sizeof(*selected) * (ts->orderCount + 1));
    int loopcheck;
    orderListIndex orderList;
    int numOrderList;
    int npeer = 128;	/* XXX more than deep enough for now. */
    int * peer = (int *) memset(alloca(npeer*sizeof(*peer)), 0, (npeer*sizeof(*peer)));
    int nrescans = 100;
    int _printed = 0;
    char deptypechar;
    size_t tsbytes = 0;
    int oType = 0;
    int depth;
    int breadth;
    int qlen;
#endif	/* REFERENCE */

if (_rpmts_debug)
fprintf(stderr, "--> %s(%p) tsFlags 0x%x\n", __FUNCTION__, ts, rpmtsFlags(ts));

    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    /* Create added package index. */
#ifdef	REFERENCE
    rpmalMakeIndex(ts->addedPackages);
#else
#endif

    /* Create erased package index. */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_REMOVED)) != NULL) {
#ifdef	REFERENCE
	rpmalAdd(ts->erasedPackages, p);
#else
/*@-abstract@*/
	fnpyKey key = (fnpyKey) p;
/*@=abstract@*/
	alKey pkgKey = RPMAL_NOMATCH;

	pkgKey = rpmalAdd(&ts->erasedPackages, pkgKey, key,
			rpmteDS(p, RPMTAG_PROVIDENAME),
			rpmteFI(p, RPMTAG_BASENAMES), tscolor);
	/* XXX pretend erasedPackages are just appended to addedPackages. */
	pkgKey = (alKey)(((long)pkgKey) + ts->numAddedPackages);
	(void) rpmteSetAddedKey(p, pkgKey);
#endif
    }
    pi = rpmtsiFree(pi);
    rpmalMakeIndex(ts->erasedPackages);

    /* T1. Initialize. */
#ifdef	REFERENCE
#else
    if (oType == 0)
	numOrderList = ts->orderCount;
    else {
	numOrderList = 0;
	if (oType & TR_ADDED)
	    numOrderList += ts->numAddedPackages;
	if (oType & TR_REMOVED)
	    numOrderList += ts->numRemovedPackages;
     }
    ordering = (alKey *) alloca(sizeof(*ordering) * (numOrderList + 1));
    loopcheck = numOrderList;
#endif

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, oType)) != NULL)
	rpmteNewTSI(p);
    pi = rpmtsiFree(pi);

    /* Record all relations. */
    rpmlog(RPMLOG_DEBUG, D_("========== recording tsort relations\n"));
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, oType)) != NULL) {
	al = (rpmteType(p) == TR_ADDED ? ts->addedPackages : ts->erasedPackages);

	/* T2. Next "q <- p" relation. */

#ifdef	REFERENCE
#else
	memset(selected, 0, sizeof(*selected) * ts->orderCount);

	/* Avoid narcisstic relations. */
	selected[rpmtsiOc(pi)] = 1;
#endif

	requires = rpmteDS(p, RPMTAG_REQUIRENAME);
	requires = rpmdsInit(requires);
	if (requires != NULL)
	while (rpmdsNext(requires) >= 0) {

	    Flags = rpmdsFlags(requires);
	    if (!isAuto(Flags))
		/*@innercontinue@*/ continue;

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
#ifdef	REFERENCE
	    (void) orgrpmAddRelation(ts, al, p, requires);
#else
	    (void) addRelation(ts, al, p, selected, requires);
#endif

	}

#ifdef	REFERENCE
#else	/* REFERENCE */
	/* Ensure that erasures follow installs during upgrades. */
      if (rpmteType(p) == TR_REMOVED && p->flink.Pkgid && p->flink.Pkgid[0]) {

	qi = rpmtsiInit(ts);
	while ((q = rpmtsiNext(qi, TR_ADDED)) != NULL) {

	    if (strcmp(q->pkgid, p->flink.Pkgid[0]))
		/*@innercontinue@*/ continue;

	    requires = rpmdsFromPRCO(q->PRCO, RPMTAG_NAME);
	    if (requires != NULL) {
		/* XXX disable erased arrow reversal. */
		p->type = TR_ADDED;
#ifdef	REFERENCE
		(void) orgrpmAddRelation(ts, ts->addedPackages, p, requires);
#else
		(void) addRelation(ts, ts->addedPackages, p, selected, requires);
#endif
		p->type = TR_REMOVED;
	    }
	}
	qi = rpmtsiFree(qi);
      }

	/* Order by requiring parent directories as prerequisites. */
	requires = rpmdsInit(rpmteDS(p, RPMTAG_DIRNAMES));
	if (requires != NULL)
	while (rpmdsNext(requires) >= 0) {

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
#ifdef	REFERENCE
	    (void) orgrpmAddRelation(ts, al, p, requires);
#else
	    (void) addRelation(ts, al, p, selected, requires);
#endif

	}

	/* Order by requiring no dangling symlinks. */
	requires = rpmdsInit(rpmteDS(p, RPMTAG_FILELINKTOS));
	if (requires != NULL)
	while (rpmdsNext(requires) >= 0) {

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
#ifdef	REFERENCE
	    (void) orgrpmAddRelation(ts, al, p, requires);
#else
	    (void) addRelation(ts, al, p, selected, requires);
#endif

	}
#endif	/* REFERENCE */
    }
    pi = rpmtsiFree(pi);

    /* Save predecessor count and mark tree roots. */
    treex = 0;
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, oType)) != NULL) {
	int npreds = rpmteTSI(p)->tsi_count;

	(void) rpmteSetNpreds(p, npreds);
#ifdef	REFERENCE
	(void) rpmteSetDepth(p, 1);
#else
	(void) rpmteSetDepth(p, 0);
#endif

	if (npreds == 0) {
#ifdef	REFERENCE
	    (void) rpmteSetTree(p, treex++);
#else
	    (void) rpmteSetTree(p, ++treex);
	    (void) rpmteSetBreadth(p, treex);
#endif
	} else
	    (void) rpmteSetTree(p, -1);
#ifdef	UNNECESSARY
	(void) rpmteSetParent(p, NULL);
#endif

    }
    pi = rpmtsiFree(pi);
    ts->ntrees = treex;

#ifdef	REFERENCE
    /* Remove dependency loops. */
    newOrder = (rpmte *) xcalloc(ts->orderCount, sizeof(*newOrder));
    SCCs = detectSCCs(ts);
#endif

    /* T4. Scan for zeroes. */
    rpmlog(RPMLOG_DEBUG, D_("========== tsorting packages (order, #predecessors, #succesors, tree, Ldepth, Rbreadth)\n"));

#ifdef	REFERENCE
  for (j = 0; j < 2; j++) {
    /* Do two separate runs: installs first - then erases */
    unsigned oType = (j == 0 ? TR_ADDED : TR_REMOVED);

#else
rescan:
    if (pi != NULL) pi = rpmtsiFree(pi);
#endif
    q = r = NULL;
    qlen = 0;
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, oType)) != NULL) {

	/* Prefer packages in chainsaw or anaconda presentation order. */
	if (anaconda)
	    rpmteTSI(p)->tsi_qcnt = (ts->orderCount - rpmtsiOc(pi));

	if (rpmteTSI(p)->tsi_count != 0)
	    continue;

	/* Mark the package as queued. */
	rpmteTSI(p)->tsi_queued = orderingCount + 1;
	rpmteTSI(p)->tsi_suc = NULL;
	addQ(p, &q, &r, prefcolor);
	qlen++;
    }
    pi = rpmtsiFree(pi);

    /* T5. Output front of queue (T7. Remove from queue.) */
    for (; q != NULL; q = rpmteTSI(q)->tsi_suc) {

	/* Mark the package as unqueued. */
	rpmteTSI(q)->tsi_queued = 0;

	if (oType != 0)
	switch (rpmteType(q)) {
	case TR_ADDED:
	    if (!(oType & TR_ADDED))
		continue;
	    /*@switchbreak@*/ break;
	case TR_REMOVED:
	    if (!(oType & TR_REMOVED))
		continue;
	    /*@switchbreak@*/ break;
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}
	deptypechar = (rpmteType(q) == TR_REMOVED ? '-' : '+');

	treex = rpmteTree(q);
	depth = rpmteDepth(q);
	breadth = ((depth < npeer) ? peer[depth]++ : 0);
	(void) rpmteSetBreadth(q, breadth);

	rpmlog(RPMLOG_DEBUG, "%5d%5d%5d%5d%5d%5d %*s%c%s\n",
			orderingCount, rpmteNpreds(q),
			rpmteTSI(q)->tsi_qcnt,
			treex, depth, breadth,
			(2 * depth), "",
			deptypechar,
			(rpmteNEVRA(q) ? rpmteNEVRA(q) : "???"));

	(void) rpmteSetDegree(q, 0);
	tsbytes += rpmtePkgFileSize(q);

	ordering[orderingCount] = rpmteAddedKey(q);
	orderingCount++;
	qlen--;
	loopcheck--;

	/* T6. Erase relations. */
	tsi_next = rpmteTSI(q)->tsi_next;
	rpmteTSI(q)->tsi_next = NULL;
	while ((tsi = tsi_next) != NULL) {
	    tsi_next = tsi->tsi_next;
	    tsi->tsi_next = NULL;
	    p = tsi->tsi_suc;
	    if (p && (--rpmteTSI(p)->tsi_count) <= 0) {

		(void) rpmteSetTree(p, treex);
		(void) rpmteSetDepth(p, depth+1);
		(void) rpmteSetParent(p, q);
		(void) rpmteSetDegree(q, rpmteDegree(q)+1);

		/* Mark the package as queued. */
		rpmteTSI(p)->tsi_queued = orderingCount + 1;
		/* XXX TODO: add control bit. */
		rpmteTSI(p)->tsi_suc = NULL;
/*@-nullstate@*/	/* XXX FIX: rpmteTSI(q)->tsi_suc can be NULL. */
		addQ(p, &rpmteTSI(q)->tsi_suc, &r, prefcolor);
/*@=nullstate@*/
		qlen++;
	    }
	    tsi = _free(tsi);
	}
	if (!_printed && loopcheck == qlen && rpmteTSI(q)->tsi_suc != NULL) {
	    _printed++;
	    (void) rpmtsUnorderedSuccessors(ts, orderingCount);
	    rpmlog(RPMLOG_DEBUG,
		D_("========== successors only (%d bytes)\n"), (int)tsbytes);

	    /* Relink the queue in presentation order. */
	    tsi = rpmteTSI(q);
	    pi = rpmtsiInit(ts);
	    while ((p = rpmtsiNext(pi, oType)) != NULL) {
		/* Is this element in the queue? */
		if (rpmteTSI(p)->tsi_queued == 0)
		    /*@innercontinue@*/ continue;
		tsi->tsi_suc = p;
		tsi = rpmteTSI(p);
	    }
	    pi = rpmtsiFree(pi);
	    tsi->tsi_suc = NULL;
	}
    }
#ifdef	REFERENCE
  }
#else	/* REFERENCE */
    /* T8. End of process. Check for loops. */
    if (loopcheck != 0) {
	int nzaps;

	/* T9. Initialize predecessor chain. */
	nzaps = 0;
	qi = rpmtsiInit(ts);
	while ((q = rpmtsiNext(qi, oType)) != NULL) {
	    rpmteTSI(q)->tsi_chain = NULL;
	    rpmteTSI(q)->tsi_queued = 0;
	    /* Mark packages already sorted. */
	    if (rpmteTSI(q)->tsi_count == 0)
		rpmteTSI(q)->tsi_count = -1;
	}
	qi = rpmtsiFree(qi);

	/* T10. Mark all packages with their predecessors. */
	qi = rpmtsiInit(ts);
	while ((q = rpmtsiNext(qi, oType)) != NULL) {
	    if ((tsi = rpmteTSI(q)->tsi_next) == NULL)
		continue;
	    rpmteTSI(q)->tsi_next = NULL;
	    markLoop(tsi, q);
	    rpmteTSI(q)->tsi_next = tsi;
	}
	qi = rpmtsiFree(qi);

	/* T11. Print all dependency loops. */
	ri = rpmtsiInit(ts);
	while ((r = rpmtsiNext(ri, oType)) != NULL)
	{
	    int printed;

	    printed = 0;

	    /* T12. Mark predecessor chain, looking for start of loop. */
	    for (q = rpmteTSI(r)->tsi_chain; q != NULL;
		 q = rpmteTSI(q)->tsi_chain)
	    {
		if (rpmteTSI(q)->tsi_queued)
		    /*@innerbreak@*/ break;
		rpmteTSI(q)->tsi_queued = 1;
	    }

	    /* T13. Print predecessor chain from start of loop. */
	    while ((p = q) != NULL && (q = rpmteTSI(p)->tsi_chain) != NULL) {
#if 0
		const char * nevra;
#endif
		const char * dp;
		rpmlogLvl msglvl = (anaconda || (rpmtsDFlags(ts) & RPMDEPS_FLAG_DEPLOOPS))
			? RPMLOG_WARNING : RPMLOG_ERR;
#if defined(RPM_VENDOR_MANDRIVA) /* loop-detection-optional-loglevel */
		// Report loops as debug-level message by default (7 = RPMLOG_DEBUG), overridable
		msglvl = rpmExpandNumeric("%{?_loop_detection_loglevel}%{?!_loop_detection_loglevel:7}");
#endif

		/* Unchain predecessor loop. */
		rpmteTSI(p)->tsi_chain = NULL;

		if (!printed) {
		    rpmlog(msglvl, _("LOOP:\n"));
		    printed = 1;
		}

		/* Find (and destroy if co-requisite) "q <- p" relation. */
		dp = zapRelation(q, p, 1, &nzaps, msglvl);

#if 0
		/* Print next member of loop. */
		nevra = rpmteNEVRA(p);
		rpmlog(msglvl, "    %-40s %s\n", (nevra ? nevra : "???"),
			(dp ? dp : "not found!?!"));
#endif

		dp = _free(dp);
	    }

	    /* Walk (and erase) linear part of predecessor chain as well. */
	    for (p = r, q = rpmteTSI(r)->tsi_chain; q != NULL;
		 p = q, q = rpmteTSI(q)->tsi_chain)
	    {
		/* Unchain linear part of predecessor loop. */
		rpmteTSI(p)->tsi_chain = NULL;
		rpmteTSI(p)->tsi_queued = 0;
	    }
	}
	ri = rpmtsiFree(ri);

	/* If a relation was eliminated, then continue sorting. */
	/* XXX TODO: add control bit. */
	if (nzaps && nrescans-- > 0) {
	    rpmlog(RPMLOG_DEBUG, D_("========== continuing tsort ...\n"));
	    goto rescan;
	}

	/* Return no. of packages that could not be ordered. */
	rpmlog(RPMLOG_ERR, _("rpmtsOrder failed, %d elements remain\n"),
			loopcheck);

#ifdef	NOTYET
	/* Do autorollback goal since we could not sort this transaction properly. */
	(void) rpmtsRollback(ts, RPMPROB_FILTER_NONE, 0, NULL);
#endif

	return loopcheck;
    }
#endif	/* REFERENCE */

    /* Clean up tsort remnants (if any). */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, 0)) != NULL)
	rpmteFreeTSI(p);
    pi = rpmtsiFree(pi);

#ifdef	REFERENCE
#else	/* REFERENCE */
    /* The order ends up as installed packages followed by removed packages. */
    orderList = (orderListIndex) xcalloc(numOrderList, sizeof(*orderList));
    j = 0;
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, oType)) != NULL) {
	/* Prepare added/erased package ordering permutation. */
	orderList[j].pkgKey = rpmteAddedKey(p);
	orderList[j].orIndex = rpmtsiOc(pi);
	j++;
    }
    pi = rpmtsiFree(pi);

    qsort(orderList, numOrderList, sizeof(*orderList), orderListIndexCmp);

/*@-type@*/
    newOrder = (rpmte *) xcalloc(ts->orderCount, sizeof(*newOrder));
/*@=type@*/
    for (i = 0, newOrderCount = 0; i < orderingCount; i++)
    {
	struct orderListIndex_s key;
	orderListIndex needle;

	key.pkgKey = ordering[i];
	needle = bsearch(&key, orderList, numOrderList,
				sizeof(key), orderListIndexCmp);
	if (needle == NULL)	/* XXX can't happen */
	    continue;

	j = needle->orIndex;
	if ((q = ts->order[j]) == NULL || needle->pkgKey == RPMAL_NOMATCH)
	    continue;

	newOrder[newOrderCount++] = q;
	ts->order[j] = NULL;
    }
    orderList = _free(orderList);
#endif	/* REFERENCE */

assert(newOrderCount == ts->orderCount);
    rc = 0;

/*@+voidabstract@*/
    ts->order = _free(ts->order);
/*@=voidabstract@*/
    ts->order = newOrder;
    ts->orderAlloced = ts->orderCount;

#ifdef	REFERENCE
    for (i = 2; SCCs[i].members != NULL; i++)
	SCCs[i].members = _free(SCCs[i].members);
    SCCs = _free(SCCs);

    rpmalFree(ts->erasedPackages);
    ts->erasedPackages = NULL;
#else	/* REFERENCE */
#ifdef	DYING	/* XXX now done at the CLI level just before rpmtsRun(). */
    rpmtsClean(ts);
#endif
#endif	/* REFERENCE */

    freeBadDeps();

    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_ORDER), 0);

    return rc;
}

int (*rpmtsOrder) (rpmts ts)
	= _rpmtsOrder;
