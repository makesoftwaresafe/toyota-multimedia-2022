/** \ingroup rpmts
 * \file lib/depends.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmcb.h>
#include <rpmbf.h>
#include <rpmmacro.h>		/* XXX rpmExpand("%{_dependency_whiteout}" */
#include <envvar.h>
#include <ugid.h>		/* XXX user()/group() probes */

#include <rpmtag.h>
#define	_RPMDB_INTERNAL		/* XXX response cache needs dbiOpen et al. */
#include <rpmdb.h>

#define	_RPMEVR_INTERNAL
#include <rpmds.h>
#include <rpmfi.h>

#define	_RPMTE_INTERNAL
#include <rpmte.h>
#define	_RPMTS_INTERNAL
#include <rpmcli.h>		/* XXX rpmcliPackagesTotal */

#include "debug.h"

/*@access tsortInfo @*/
/*@access rpmte @*/		/* XXX for install <-> erase associate. */
/*@access rpmts @*/
/*@access rpmDiskSpaceInfo @*/

#define	CACHE_DEPENDENCY_RESULT	1
#if defined(CACHE_DEPENDENCY_RESULT)
/*@unchecked@*/
int _cacheDependsRC = CACHE_DEPENDENCY_RESULT;
#endif

/*@observer@*/ /*@unchecked@*/
const char *rpmNAME = PACKAGE;

/*@observer@*/ /*@unchecked@*/
const char *rpmEVR = VERSION;

/*@unchecked@*/
int rpmFLAGS = RPMSENSE_EQUAL;

/**
 * Compare removed package instances (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
static int uintcmp(const void * a, const void * b)
	/*@requires maxRead(a) == 0 /\ maxRead(b) == 0 @*/
{
    const uint32_t * aptr = (const uint32_t *) a;
    const uint32_t * bptr = (const uint32_t *) b;
    int rc = (*aptr - *bptr);
    return rc;
}

/**
 * Add removed package instance to ordered transaction set.
 * @param ts		transaction set
 * @param h		header
 * @param hdrNum	rpm database instance
 * @retval *indexp	removed element index (if not NULL)
 * @param depends	installed package of pair (or RPMAL_NOMATCH on erase)
 * @return		0 on success
 */
static int removePackage(rpmts ts, Header h, uint32_t hdrNum,
		/*@null@*/ int * indexp,
		/*@exposed@*/ /*@dependent@*/ /*@null@*/ alKey depends)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, *indexp, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmte p;
    int xx;

    /* Filter out duplicate erasures. */
    if (ts->numRemovedPackages > 0 && ts->removedPackages != NULL) {
	uint32_t * needle = NULL;
	needle = (uint32_t *) bsearch(&hdrNum,
			ts->removedPackages, ts->numRemovedPackages,
			sizeof(*ts->removedPackages), uintcmp);
	if (needle != NULL) {
	    /* XXX lastx should be per-call, not per-ts. */
	    if (indexp != NULL)
	        *indexp = needle - ts->removedPackages;
	    return 0;
	}
    }

    if (ts->rbf == NULL) {
	static size_t n = 10000;	/* XXX population estimate */
	static double e = 1.0e-4;
	size_t m = 0;
	size_t k = 0;
	rpmbfParams(n, e, &m, &k);
	ts->rbf = rpmbfNew(m, k, 0);
    }

    if (ts->numRemovedPackages == ts->allocedRemovedPackages) {
	ts->allocedRemovedPackages += ts->delta;
	/* XXX coverity #1035956 */
	ts->removedPackages = (uint32_t *) xrealloc(ts->removedPackages,
		sizeof(*ts->removedPackages) * ts->allocedRemovedPackages);
    }

assert(ts->removedPackages != NULL);	/* XXX can't happen. */
    xx = rpmbfAdd(ts->rbf, &hdrNum, sizeof(hdrNum));
assert(xx == 0);
    ts->removedPackages[ts->numRemovedPackages] = hdrNum;
    ts->numRemovedPackages++;
    if (ts->numRemovedPackages > 1)
	qsort(ts->removedPackages, ts->numRemovedPackages,
			sizeof(*ts->removedPackages), uintcmp);

    if (ts->orderCount >= ts->orderAlloced) {
	ts->orderAlloced += (ts->orderCount - ts->orderAlloced) + ts->delta;
/*@-type +voidabstract @*/
	ts->order = (rpmte *) xrealloc(ts->order, sizeof(*ts->order) * ts->orderAlloced);
/*@=type =voidabstract @*/
    }

    p = rpmteNew(ts, h, TR_REMOVED, NULL, NULL, hdrNum, depends);
    ts->order[ts->orderCount] = p;
    ts->numErasedFiles += rpmfiFC(rpmteFI(p, RPMTAG_BASENAMES));
    if (indexp != NULL)
	*indexp = ts->orderCount;
    ts->orderCount++;

/*@-nullstate@*/	/* XXX FIX: ts->order[] can be NULL. */
   return 0;
/*@=nullstate@*/
}

/**
 * Are two headers identical?
 * @param first		first header
 * @param second	second header
 * @return		1 if headers are identical, 0 otherwise
 */
static int rpmHeadersIdentical(Header first, Header second)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char * one, * two;
    int rc = 0;
    int xx;

    he->tag = RPMTAG_HDRID;
    xx = headerGet(first, he, 0);
    one = he->p.str;
    he->tag = RPMTAG_HDRID;
    xx = headerGet(second, he, 0);
    two = he->p.str;

    if (one && two)
	rc = ((strcmp(one, two) == 0) ? 1 : 0);
    else if (one && !two)
	rc = 0;
    else if (!one && two)
	rc = 0;
    else {
	/* XXX Headers w/o digests case devolves to NEVR comparison. */
	rpmds A = rpmdsThis(first, RPMTAG_REQUIRENAME, RPMSENSE_EQUAL);
	rpmds B = rpmdsThis(second, RPMTAG_REQUIRENAME, RPMSENSE_EQUAL);
	rc = rpmdsCompare(A, B);
	(void)rpmdsFree(A);
	A = NULL;
	(void)rpmdsFree(B);
	B = NULL;
    }
    one = _free(one);
    two = _free(two);
    return rc;
}

/*@unchecked@*/
static rpmTag _upgrade_tag;
/*@unchecked@*/
static rpmTag _debuginfo_tag;
/*@unchecked@*/
static rpmTag _obsolete_tag;

/**
 * Add upgrade erasures to a transaction set.
 * @param ts		transaction set
 * @param p		transaction element
 * @param hcolor	header color
 * @param h		header
 * @return		0 on success
 */
static int rpmtsAddUpgrades(rpmts ts, rpmte p, rpmuint32_t hcolor, Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t tscolor = rpmtsColor(ts);
    alKey pkgKey = rpmteAddedKey(p);
    rpmuint32_t ohcolor;
    rpmmi mi;
    Header oh;
    int xx;

    if (_upgrade_tag == 0) {
	const char * t = rpmExpand("%{?_upgrade_tag}", NULL);
/*@-mods@*/
	_upgrade_tag = (!strcmp(t, "name") ? RPMTAG_NAME : RPMTAG_PROVIDENAME);
/*@=mods@*/
	t = _free(t);
    }

    mi = rpmtsInitIterator(ts, _upgrade_tag, rpmteN(p), 0);
    while((oh = rpmmiNext(mi)) != NULL) {
	int lastx;
	rpmte q;

	/* Ignore colored packages not in our rainbow. */
	ohcolor = hGetColor(oh);
	if (tscolor && hcolor && ohcolor && !(hcolor & ohcolor))
	    continue;

	/* Snarf the original install tid & time from older package(s). */
	he->tag = RPMTAG_ORIGINTID;
	xx = headerGet(oh, he, 0);
	if (xx && he->p.ui32p != NULL) {
	    if (p->originTid[0] == 0 || p->originTid[0] > he->p.ui32p[0]
	     || (he->c > 1 && p->originTid[0] == he->p.ui32p[0] && p->originTid[1] > he->p.ui32p[1]))
	    {
		p->originTid[0] = he->p.ui32p[0];
		p->originTid[1] = (he->c > 1 ? he->p.ui32p[1] : 0);
	    }
	    he->p.ptr = _free(he->p.ptr);
	}
	he->tag = RPMTAG_ORIGINTIME;
	xx = headerGet(oh, he, 0);
	if (xx && he->p.ui32p != NULL) {
	    if (p->originTime[0] == 0 || p->originTime[0] > he->p.ui32p[0]
	     || (he->c > 1 && p->originTime[0] == he->p.ui32p[0] && p->originTime[1] > he->p.ui32p[1]))
	    {
		p->originTime[0] = he->p.ui32p[0];
		p->originTime[1] = (he->c > 1 ? he->p.ui32p[1] : 0);
	    }
	    he->p.ptr = _free(he->p.ptr);
	}

#if defined(RPM_VENDOR_WINDRIVER)
	/*
	 * If we're capable of installing multiple colors
	 * but at least one of the packages are white (0), we
	 * further verify the arch is the same (or compatible) to trigger an upgrade
	 * we do have a special case to allow upgrades of noarch w/ a arch package
	 */
	if (tscolor && (!hcolor || !ohcolor)) {
	    const char * arch;
	    const char * oharch;
	    he->tag = RPMTAG_ARCH;
	    xx = headerGet(h, he, 0);
	    arch = (xx && he->p.str != NULL ? he->p.str : NULL);
	    he->tag = RPMTAG_ARCH;
	    xx = headerGet(oh, he, 0);
	    oharch = (xx && he->p.str != NULL ? he->p.str : NULL);
	    if (arch != NULL && oharch != NULL) {
	        if (strcmp("noarch", arch) || strcmp("noarch", oharch)) {
		    if (!_isCompatibleArch(arch, oharch)) {
			arch = _free(arch);
			oharch = _free(oharch);
			continue;
		    }
		}
	    }
	    arch = _free(arch);
	    oharch = _free(oharch);
	}
#endif

	/* Skip identical packages. */
	if (rpmHeadersIdentical(h, oh))
	    continue;

	/* Create an erasure element. */
	lastx = -1;
	xx = removePackage(ts, oh, rpmmiInstance(mi), &lastx, pkgKey);
assert(lastx >= 0 && lastx < ts->orderCount);
	q = ts->order[lastx];

	/* Chain through upgrade flink. */
	xx = rpmteChain(p, q, oh, "Upgrades");

/*@-nullptrarith@*/
	rpmlog(RPMLOG_DEBUG, D_("   upgrade erases %s\n"), rpmteNEVRA(q));
/*@=nullptrarith@*/

    }
    mi = rpmmiFree(mi);

    return 0;
}

/**
 * Check string for a suffix.
 * @param fn		string
 * @param suffix	suffix
 * @return		1 if string ends with suffix
 */
static inline int chkSuffix(const char * fn, const char * suffix)
        /*@*/
{
    size_t flen = strlen(fn);
    size_t slen = strlen(suffix);
    return (flen > slen && !strcmp(fn + flen - slen, suffix));
}

/**
 * Add unreferenced debuginfo erasures to a transaction set.
 * @param ts		transaction set
 * @param p		transaction element
 * @param h		header
 * @param pkgKey	added package key (erasure uses RPMAL_NOKEY)
 * @return		no. of references from build set
 */
static int rpmtsEraseDebuginfo(rpmts ts, rpmte p, Header h,
		/*@exposed@*/ /*@dependent@*/ /*@null@*/ alKey pkgKey)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const void *keyval = NULL;
    size_t keylen = 0;
    size_t nrefs = 0;
    rpmuint32_t debuginfoInstance = 0;
    Header debuginfoHeader = NULL;
    rpmmi mi;
    Header oh;
    int xx;

    /* XXX SOURCEPKGID is not populated reliably, do not use (yet). */
    if (_debuginfo_tag == 0) {
	const char * t = rpmExpand("%{?_debuginfo_tag}", NULL);
/*@-mods@*/
	_debuginfo_tag = (*t != '\0' && !strcmp(t, "pkgid")
		? RPMTAG_SOURCEPKGID : RPMTAG_SOURCERPM);
/*@=mods@*/
	t = _free(t);
    }

    /* Grab the retrieval key. */
    switch (_debuginfo_tag) {
    default:		return 0;	/*@notreached@*/	break;
    case RPMTAG_SOURCERPM:	keyval = rpmteSourcerpm(p);	break;
    }

    /* Count remaining members in build set, excluding -debuginfo (if any). */
    mi = rpmtsInitIterator(ts, _debuginfo_tag, keyval, keylen);
    xx = rpmmiPrune(mi, ts->removedPackages, ts->numRemovedPackages, 1);
    while((oh = rpmmiNext(mi)) != NULL) {
	/* Skip identical packages. */
	if (rpmHeadersIdentical(h, oh))
	    continue;

	he->tag = RPMTAG_NAME;
	xx = headerGet(oh, he, 0);
	if (!xx || he->p.str == NULL)
	    continue;
	/* Save the -debuginfo member. */
	if (chkSuffix(he->p.str, "-debuginfo")) {
	    debuginfoInstance = rpmmiInstance(mi);
	    debuginfoHeader = headerLink(oh);
	} else
	    nrefs++;
	he->p.str = _free(he->p.str);
    }
    mi = rpmmiFree(mi);

    /* Remove -debuginfo package when last build member is erased. */
    if (nrefs == 0 && debuginfoInstance > 0 && debuginfoHeader != NULL) {
	int lastx = -1;
	rpmte q;

	/* Create an erasure element. */
	lastx = -1;
	xx = removePackage(ts, debuginfoHeader, debuginfoInstance,
		&lastx, pkgKey);
assert(lastx >= 0 && lastx < ts->orderCount);
	q = ts->order[lastx];

	/* Chain through upgrade flink. */
	/* XXX avoid assertion failure when erasing. */
	if (pkgKey != RPMAL_NOMATCH)
	    xx = rpmteChain(p, q, oh, "Upgrades");

/*@-nullptrarith@*/
	rpmlog(RPMLOG_DEBUG, D_("   lastref erases %s\n"), rpmteNEVRA(q));
/*@=nullptrarith@*/

    }
    (void)headerFree(debuginfoHeader);
    debuginfoHeader = NULL;

    return (int)nrefs;
}

/**
 * Add Obsoletes: erasures to a transaction set.
 * @param ts		transaction set
 * @param p		transaction element
 * @param hcolor	header color
 * @return		0 on success
 */
static int rpmtsAddObsoletes(rpmts ts, rpmte p, rpmuint32_t hcolor)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmuint32_t tscolor = rpmtsColor(ts);
    alKey pkgKey = rpmteAddedKey(p);
    rpmuint32_t ohcolor;
    rpmds obsoletes;
    rpmuint32_t dscolor;
    rpmmi mi;
    Header oh;
    int xx;

    if (_obsolete_tag == 0) {
	const char *t = rpmExpand("%{?_obsolete_tag}", NULL);
/*@-mods@*/
	_obsolete_tag = (!strcmp(t, "name") ? RPMTAG_NAME : RPMTAG_PROVIDENAME);
/*@=mods@*/
	t = _free(t);
    }

    obsoletes = rpmdsLink(rpmteDS(p, RPMTAG_OBSOLETENAME), "Obsoletes");
    obsoletes = rpmdsInit(obsoletes);
    if (obsoletes != NULL)
    while (rpmdsNext(obsoletes) >= 0) {
	const char * Name;

	if ((Name = rpmdsN(obsoletes)) == NULL)
	    continue;	/* XXX can't happen */

	/* Ignore colored obsoletes not in our rainbow. */
#if 0
	/* XXX obsoletes are never colored, so this is for future devel. */
	dscolor = rpmdsColor(obsoletes);
#else
	dscolor = hcolor;
#endif
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	/* XXX avoid self-obsoleting packages. */
	if (!strcmp(rpmteN(p), Name))
	    continue;

	/* Obsolete containing package if given a file, otherwise provide. */
	if (Name[0] == '/')
	    mi = rpmtsInitIterator(ts, RPMTAG_BASENAMES, Name, 0);
	else
	    mi = rpmtsInitIterator(ts, _obsolete_tag, Name, 0);

	xx = rpmmiPrune(mi, ts->removedPackages, ts->numRemovedPackages, 1);

	while((oh = rpmmiNext(mi)) != NULL) {
	    int lastx;
	    rpmte q;

	    /* Ignore colored packages not in our rainbow. */
	    ohcolor = hGetColor(oh);

	    /* XXX provides *are* colored, effectively limiting Obsoletes:
		to matching only colored Provides: based on pkg coloring. */
	    if (tscolor && hcolor && ohcolor && !(hcolor & ohcolor))
		/*@innercontinue@*/ continue;

	    /*
	     * Rpm prior to 3.0.3 does not have versioned obsoletes.
	     * If no obsoletes version info is available, match all names.
	     */
	    if (!(rpmdsEVR(obsoletes) == NULL
	     || rpmdsAnyMatchesDep(oh, obsoletes, _rpmds_nopromote)))
		/*@innercontinue@*/ continue;

	    /* Create an erasure element. */
	    lastx = -1;
	    xx = removePackage(ts, oh, rpmmiInstance(mi), &lastx, pkgKey);
assert(lastx >= 0 && lastx < ts->orderCount);
	    q = ts->order[lastx];

	    /* Chain through obsoletes flink. */
	    xx = rpmteChain(p, q, oh, "Obsoletes");

/*@-nullptrarith@*/
	    rpmlog(RPMLOG_DEBUG, D_("  Obsoletes: %s\t\terases %s\n"),
			rpmdsDNEVR(obsoletes)+2, rpmteNEVRA(q));
/*@=nullptrarith@*/
	}
	mi = rpmmiFree(mi);
    }
    (void)rpmdsFree(obsoletes);
    obsoletes = NULL;

    return 0;
}

#if defined(RPM_VENDOR_WINDRIVER)
/* Is "compat" compatible w/ arch? */
int _isCompatibleArch(const char * arch, const char * compat)
{
    const char * compatArch = rpmExpand(compat, " %{?_", compat, "_compat_arch}", NULL);
    const char * p, * pe, * t;
    int match = 0;

    /* Hack to ensure iX86 being automatically compatible */
    if (arch[0] == 'i' && arch[2] == '8' && arch[3] == '6') {
	if ((arch[0] == compat[0]) &&
	    (arch[2] == compat[2]) &&
	    (arch[3] == compat[3]))
	    match = 1;

        if (!strcmp(compat, "x86_32"))
	    match = 1;
    }

    for ( p = pe = compatArch ; *pe && match == 0 ; ) {
	while (*p && xisspace(*p)) p++;
	pe = p ; while (*pe && !xisspace(*pe)) pe++;
	if (p == pe)
	    break;
	t = strndup(p, (pe - p));
	p = pe; /* Advance to next chunk */
rpmlog(RPMLOG_DEBUG, D_("   Comparing compat archs %s ? %s\n"), arch, t);
	if (!strcmp(arch, t))
	    match = 1;
	t = _free(t);
    }
    compatArch = _free(compatArch);
    return match;
}
#endif

int rpmtsAddInstallElement(rpmts ts, Header h,
			fnpyKey key, int upgrade, rpmRelocation relocs)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmdepFlags depFlags = rpmtsDFlags(ts);
    rpmuint32_t tscolor = rpmtsColor(ts);
    rpmuint32_t hcolor;
    int isSource;
    int duplicate = 0;
    rpmtsi pi = NULL; rpmte p;
    const char * arch = NULL;
    const char * os = NULL;
    rpmds oldChk, newChk;
    alKey pkgKey;	/* addedPackages key */
    int xx;
    int ec = 0;
    int rc;
    int oc;

    hcolor = hGetColor(h);
    pkgKey = RPMAL_NOMATCH;

    /*
     * Always add source headers.
     */
    isSource =
	(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
	 headerIsEntry(h, RPMTAG_ARCH) != 0);
    if (isSource) {
	oc = ts->orderCount;
	goto addheader;
    }

    /*
     * Check platform affinity of binary packages.
     */
    he->tag = RPMTAG_ARCH;
    xx = headerGet(h, he, 0);
    arch = he->p.str;
    he->tag = RPMTAG_OS;
    xx = headerGet(h, he, 0);
    os = he->p.str;
    if (nplatpat > 1) {
	const char * platform = NULL;

	he->tag = RPMTAG_PLATFORM;
	xx = headerGet(h, he, 0);
	platform = he->p.str;
	if (!xx || platform == NULL)
	    platform = rpmExpand(arch, "-unknown-", os, NULL);

	rc = rpmPlatformScore(platform, platpat, nplatpat);
#if defined(RPM_VENDOR_MANDRIVA)
	/*
	 * If no match on platform tag, we'll try again with arch tag
	 * in case platform tag is inconsistent with it, which is the case
	 * for older noarch sub-packages built (mdvbz#61746).
	 */
	if(xx && rc <= 0) {
	    platform = _free(platform);
	    platform = rpmExpand(arch, "-unknown-", os, NULL);
	    rc = rpmPlatformScore(platform, platpat, nplatpat);
	}
#endif
	if (rc <= 0) {
	    rpmps ps = rpmtsProblems(ts);
	    he->tag = RPMTAG_NVRA;
	    xx = headerGet(h, he, 0);
assert(he->p.str != NULL);

	    rpmpsAppend(ps, RPMPROB_BADPLATFORM, he->p.str, key,
                        platform, NULL, NULL, 0);

	    /* XXX problem string should be printed by caller instead. */
	    if (rpmIsVerbose()) {
		const char * msg = rpmProblemString(rpmpsGetProblem(ps, -1));
		rpmlog(RPMLOG_WARNING, "%s\n", msg);
		msg = _free(msg);
	    }

	    ps = rpmpsFree(ps);
	    he->p.ptr = _free(he->p.ptr);
	    ec = 1;
	}
	platform = _free(platform);
	if (ec)
	    goto exit;
    }

    /*
     * Always install compatible binary packages.
     */
    if (!upgrade) {
	oc = ts->orderCount;
	goto addheader;
    }

    /*
     * Check that upgrade package is uniquely newer, replace older if necessary.
     */
    oldChk = rpmdsThis(h, RPMTAG_REQUIRENAME, (evrFlags)(RPMSENSE_LESS));
    newChk = rpmdsThis(h, RPMTAG_REQUIRENAME, (evrFlags)(RPMSENSE_EQUAL|RPMSENSE_GREATER));
    /* XXX can't use rpmtsiNext() filter or oc will have wrong value. */
    for (pi = rpmtsiInit(ts), oc = 0; (p = rpmtsiNext(pi, (rpmElementType)0)) != NULL; oc++) {
	rpmds ds;

	/* XXX Only added packages need be checked for dupes here. */
	if (rpmteType(p) == TR_REMOVED)
	    continue;

	/* XXX Never check source header NEVRAO. */
	if (rpmteIsSource(p))
	    continue;

	if (tscolor) {
	    const char * parch;
	    const char * pos;

	    if (arch == NULL || (parch = rpmteA(p)) == NULL)
		continue;
#if defined(RPM_VENDOR_WINDRIVER)
	    /* XXX hackery for alias matching. */
	    if (!_isCompatibleArch(arch, parch))
		continue;
#else
	    /* XXX hackery for i[3456]86 alias matching. */
	    if (arch[0] == 'i' && arch[2] == '8' && arch[3] == '6') {
		if (arch[0] != parch[0]) continue;
		if (arch[2] != parch[2]) continue;
		if (arch[3] != parch[3]) continue;
	    }
#endif
	    else if (strcmp(arch, parch))
		continue;
	    if (os == NULL || (pos = rpmteO(p)) == NULL)
		continue;

	    if (strcmp(os, pos))
		continue;
	}

	/* OK, binary rpm's with same arch and os.  Check NEVR. */
	if ((ds = rpmteDS(p, RPMTAG_NAME)) == NULL)
	    continue;	/* XXX can't happen */

	/* If newer NEVRAO already added, then skip adding older. */
	rc = rpmdsCompare(newChk, ds);
	if (rc != 0) {
	    const char * pkgNEVR = rpmdsDNEVR(ds);
	    const char * addNEVR = rpmdsDNEVR(oldChk);
	    if (rpmIsVerbose())
		rpmlog(RPMLOG_WARNING,
		    _("package %s was already added, skipping %s\n"),
		    (pkgNEVR ? pkgNEVR + 2 : "?pkgNEVR?"),
		    (addNEVR ? addNEVR + 2 : "?addNEVR?"));
	    ec = 1;
	    break;
	}

	/* If older NEVRAO already added, then replace old with new. */
	rc = rpmdsCompare(oldChk, ds);
	if (rc != 0) {
	    const char * pkgNEVR = rpmdsDNEVR(ds);
	    const char * addNEVR = rpmdsDNEVR(newChk);
	    if (rpmIsVerbose())
		rpmlog(RPMLOG_WARNING,
		    _("package %s was already added, replacing with %s\n"),
		    (pkgNEVR ? pkgNEVR + 2 : "?pkgNEVR?"),
		    (addNEVR ? addNEVR + 2 : "?addNEVR?"));
	    duplicate = 1;
	    pkgKey = rpmteAddedKey(p);
	    break;
	}
    }
    pi = rpmtsiFree(pi);
    (void)rpmdsFree(oldChk);
    oldChk = NULL;
    (void)rpmdsFree(newChk);
    newChk = NULL;

    /* If newer (or same) NEVRAO was already added, exit now. */
    if (ec)
	goto exit;

addheader:
    if (oc >= ts->orderAlloced) {
	ts->orderAlloced += (oc - ts->orderAlloced) + ts->delta;
/*@-type +voidabstract @*/
	ts->order = (rpmte *) xrealloc(ts->order, ts->orderAlloced * sizeof(*ts->order));
/*@=type =voidabstract @*/
    }

    p = rpmteNew(ts, h, TR_ADDED, key, relocs, -1, pkgKey);
assert(p != NULL);

    if (duplicate && oc < ts->orderCount) {
	ts->numAddedFiles -= rpmfiFC(rpmteFI(ts->order[oc], RPMTAG_BASENAMES));
/*@-type -unqualifiedtrans@*/
	ts->order[oc] = rpmteFree(ts->order[oc]);
/*@=type =unqualifiedtrans@*/
    }

    ts->order[oc] = p;
    ts->numAddedFiles += rpmfiFC(rpmteFI(p, RPMTAG_BASENAMES));
    if (!duplicate) {
	ts->orderCount++;
	rpmcliPackagesTotal++;
    }
    
    pkgKey = rpmalAdd(&ts->addedPackages, pkgKey, rpmteKey(p),
			rpmteDS(p, RPMTAG_PROVIDENAME),
			rpmteFI(p, RPMTAG_BASENAMES), tscolor);
    if (pkgKey == RPMAL_NOMATCH) {
	ts->order[oc] = rpmteFree(ts->order[oc]);
	ts->teInstall = NULL;
	ec = 1;
	goto exit;
    }
    (void) rpmteSetAddedKey(p, pkgKey);

    if (!duplicate) {
	ts->numAddedPackages++;
    }

    ts->teInstall = ts->order[oc];

    /* XXX rpmgi hack: Save header in transaction element if requested. */
    if (upgrade & 0x2)
	(void) rpmteSetHeader(p, h);

    /* If not upgrading, then we're done. */
    if (!(upgrade & 0x1))
	goto exit;

    /* If source rpm, then we're done. */
    if (isSource)
	goto exit;

    /* Do lazy (readonly?) open of rpm database. */
    if (rpmtsGetRdb(ts) == NULL && rpmtsDBMode(ts) != -1) {
	if ((ec = rpmtsOpenDB(ts, rpmtsDBMode(ts)) != 0))
	    goto exit;
    }

    /* Add upgrades to the transaction (if not disabled). */
    if (!(depFlags & RPMDEPS_FLAG_NOUPGRADE)) {
	/*
	 * Don't upgrade -debuginfo until build set is empty.
	 *
	 * XXX Almost, but not quite, correct since the test depends on
	 * added package arrival order.
	 * I.e. -debuginfo additions must always follow all
	 * other additions so that erasures of other members in the
	 * same build set are seen if/when included in the same transaction.
	 */
	xx = rpmtsEraseDebuginfo(ts, p, h, pkgKey);
	if (!chkSuffix(rpmteN(p), "-debuginfo") || xx == 0)
	    xx = rpmtsAddUpgrades(ts, p, hcolor, h);
    }

    /* Add Obsoletes: to the transaction (if not disabled). */
    if (!(depFlags & RPMDEPS_FLAG_NOOBSOLETES)) {
	xx = rpmtsAddObsoletes(ts, p, hcolor);
    }

    ec = 0;

exit:
    arch = _free(arch);
    os = _free(os);
    pi = rpmtsiFree(pi);
    return ec;
}

int rpmtsAddEraseElement(rpmts ts, Header h, uint32_t hdrNum)
{
    int oc = -1;
    int rc = removePackage(ts, h, hdrNum, &oc, RPMAL_NOMATCH);
    if (rc == 0 && oc >= 0 && oc < ts->orderCount) {
	(void) rpmtsEraseDebuginfo(ts, ts->order[oc], h, RPMAL_NOMATCH);
	ts->teErase = ts->order[oc];
    } else
	ts->teErase = NULL;
    return rc;
}

/*@only@*/ /*@null@*/ /*@unchecked@*/
static char *sysinfo_path = NULL;

/*@refcounted@*/ /*@null@*/ /*@unchecked@*/
static rpmds rpmlibP = NULL;
/*@refcounted@*/ /*@null@*/ /*@unchecked@*/
rpmds cpuinfoP = NULL;
/*@refcounted@*/ /*@null@*/ /*@unchecked@*/
static rpmds getconfP = NULL;
/*@refcounted@*/ /*@null@*/ /*@unchecked@*/
static rpmds unameP = NULL;

void rpmnsClean(void)
	/*@globals sysinfo_path, _sysinfo_path, rpmlibP, cpuinfoP, getconfP, unameP @*/
	/*@modifies sysinfo_path, _sysinfo_path, rpmlibP, cpuinfoP, getconfP, unameP @*/
{
    (void)rpmdsFree(rpmlibP);
    rpmlibP = NULL;
    (void)rpmdsFree(cpuinfoP);
    cpuinfoP = NULL;
    (void)rpmdsFree(getconfP);
    getconfP = NULL;
    (void)rpmdsFree(unameP);
    unameP = NULL;
/*@-observertrans@*/
    _sysinfo_path = _free(_sysinfo_path);
/*@=observertrans@*/
    sysinfo_path = _free(sysinfo_path);
}

/**
 * Check dep for an unsatisfied dependency.
 * @param ts		transaction set
 * @param dep		dependency
 * @param adding	dependency is from added package set?
 * @return		0 if satisfied, 1 if not satisfied, 2 if error
 */
static int unsatisfiedDepend(rpmts ts, rpmds dep, int adding)
	/*@globals _cacheDependsRC, rpmGlobalMacroContext, h_errno,
		sysinfo_path, fileSystem, internalState @*/
	/*@modifies ts, dep, _cacheDependsRC, rpmGlobalMacroContext,
		sysinfo_path, fileSystem, internalState @*/
{
    DBT * key = (DBT *) alloca(sizeof(*key));
    DBT * data = (DBT *) alloca(sizeof(*data));
    rpmmi mi;
    nsType NSType;
    const char * Name;
    rpmuint32_t Flags;
    Header h;
#if defined(CACHE_DEPENDENCY_RESULT)
    int _cacheThisRC = 1;
#endif
    int rc;
    int xx;
    int retries = 20;

    if ((Name = rpmdsN(dep)) == NULL)
	return 0;	/* XXX can't happen */
    Flags = rpmdsFlags(dep);
    NSType = rpmdsNSType(dep);

#if defined(CACHE_DEPENDENCY_RESULT)
    /*
     * Check if dbiOpen/dbiPut failed (e.g. permissions), we can't cache.
     */
    if (_cacheDependsRC) {
	dbiIndex dbi;
	dbi = dbiOpen(rpmtsGetRdb(ts), RPMDBI_DEPCACHE, 0);
	if (dbi == NULL)
	    _cacheDependsRC = 0;
	else {
	    const char * DNEVR;

	    rc = -1;
	    if ((DNEVR = rpmdsDNEVR(dep)) != NULL) {
		DBC * dbcursor = NULL;
		void * datap = NULL;
		size_t datalen = 0;
		size_t DNEVRlen = strlen(DNEVR);

		xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, 0);

		memset(key, 0, sizeof(*key));
/*@i@*/		key->data = (void *) DNEVR;
		key->size = DNEVRlen;
		memset(data, 0, sizeof(*data));
		data->data = datap;
		data->size = datalen;
/*@-nullstate@*/ /* FIX: data->data may be NULL */
		xx = dbiGet(dbi, dbcursor, key, data, DB_SET);
/*@=nullstate@*/
		DNEVR = (char *) key->data;
		DNEVRlen = key->size;
		datap = data->data;
		datalen = data->size;

		if (xx == 0 && datap && datalen == 4)
		    memcpy(&rc, datap, datalen);
		xx = dbiCclose(dbi, dbcursor, 0);
	    }

	    if (rc >= 0) {
		rpmdsNotify(dep, _("(cached)"), rc);
		return rpmdsNegateRC(dep, rc);
	    }
	}
    }
#endif

retry:
    rc = 0;	/* assume dependency is satisfied */

    /* Expand macro probe dependencies. */
    if (NSType == RPMNS_TYPE_FUNCTION) {
	xx = rpmExpandNumeric(Name);
	rc = (xx ? 0 : 1);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(function probe)"), rc);
	goto exit;
    }

    /* Evaluate user/group lookup probes. */
    if (NSType == RPMNS_TYPE_USER) {
	const char *s;
	uid_t uid = 0;
	s = Name; while (*s && xisdigit(*s)) s++;

	if (*s)
	    xx = unameToUid(Name, &uid);
	else {
	    uid = strtol(Name, NULL, 10);
	    xx = (uidToUname(uid) ? 0 : -1);
	}
	rc = (xx >= 0 ? 0 : 1);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(user lookup)"), rc);
	goto exit;
    }
    if (NSType == RPMNS_TYPE_GROUP) {
	const char *s;
	gid_t gid = 0;
	s = Name; while (*s && xisdigit(*s)) s++;

	if (*s)
	    xx = gnameToGid(Name, &gid);
	else {
	    gid = strtol(Name, NULL, 10);
	    xx = (gidToGname(gid) ? 0 : -1);
	}
	rc = (xx >= 0 ? 0 : 1);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(group lookup)"), rc);
	goto exit;
    }

    /* Evaluate access(2) probe dependencies. */
    if (NSType == RPMNS_TYPE_ACCESS) {
	rc = rpmioAccess(Name, NULL, X_OK);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(access probe)"), rc);
	goto exit;
    }

    /* Evaluate mtab lookup and diskspace probe dependencies. */
    if (NSType == RPMNS_TYPE_MOUNTED) {
	const char ** fs = NULL;
	int nfs = 0;
	int i = 0;

	xx = rpmtsInitDSI(ts);
	fs = ts->filesystems;
	nfs = ts->filesystemCount;

	if (fs != NULL)
	for (i = 0; i < nfs; i++) {
	    if (!strcmp(fs[i], Name))
		break;
	}
	rc = (i < nfs ? 0 : 1);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(mtab probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_DISKSPACE) {
	size_t nb = strlen(Name);
	rpmDiskSpaceInfo dsi = NULL;
	const char ** fs = NULL;
	size_t fslen = 0, longest = 0;
	int nfs = 0;
	int i = 0;

	xx = rpmtsInitDSI(ts);
	fs = ts->filesystems;
	nfs = ts->filesystemCount;

	if (fs != NULL)
	for (i = 0; i < nfs; i++) {
	    fslen = strlen(fs[i]);
	    if (fslen > nb)
		continue;
	    if (strncmp(fs[i], Name, fslen))
		continue;
	    if (fslen > 1 && Name[fslen] != '/' && Name[fslen] != '\0')
		continue;
	    if (fslen < longest)
		continue;
	    longest = fslen;
	    dsi = ts->dsi + i;
	}
	if (dsi == NULL)
	    rc = 1;	/* no mounted paths !?! */
	else {
	    char * end = NULL;
/*@-unrecog@*/
	    rpmuint64_t needed = strtoll(rpmdsEVR(dep), &end, 0);
/*@=unrecog@*/

	    if (end && *end) {
		if (strchr("Gg", end[0]) && strchr("Bb", end[1]) && !end[2])
		    needed *= 1024 * 1024 * 1024;
		if (strchr("Mm", end[0]) && strchr("Bb", end[1]) && !end[2])
		    needed *= 1024 * 1024;
		if (strchr("Kk", end[0]) && strchr("Bb", end[1]) && !end[2])
		    needed *= 1024;
	    } else
		needed *= 1024 * 1024;	/* XXX assume Mb if no units given */

	    needed = BLOCK_ROUND(needed, dsi->f_bsize);
	    xx = (dsi->f_bavail - needed);
	    if ((Flags & RPMSENSE_LESS) && xx < 0) rc = 0;
	    else if ((Flags & RPMSENSE_GREATER) && xx > 0) rc = 0;
	    else if ((Flags & RPMSENSE_EQUAL) && xx == 0) rc = 0;
	    else rc = 1;
	}
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(diskspace probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_DIGEST) {
	const char * EVR = rpmdsEVR(dep);
        const char *filename;
        pgpHashAlgo digestHashAlgo;
        FD_t fd;
        char *cp;
        pgpHashAlgo algo;

        filename = Name;
        digestHashAlgo = PGPHASHALGO_MD5;
        if ((cp = (char *) strchr(filename, ':')) != NULL) {
            if ((algo = pgpHashAlgoStringToNumber(filename, cp-filename)) != PGPHASHALGO_ERROR) {
                digestHashAlgo = algo;
                filename = cp + 1;
            }
        }
	rc = 1;		/* XXX assume failure */
        fd = Fopen(filename, "r.fdio");
	if (fd && !Ferror(fd)) {
	    DIGEST_CTX ctx = rpmDigestInit(digestHashAlgo, RPMDIGEST_NONE);
	    const char * digest = NULL;
	    size_t digestlen = 0;
	    int asAscii = 1;
	    size_t nbuf = 8 * BUFSIZ;
	    char * buf = (char *) alloca(nbuf);
	    size_t nb;

	    while ((nb = Fread(buf, sizeof(buf[0]), nbuf, fd)) > 0)
		xx = rpmDigestUpdate(ctx, buf, nb);
	    xx = Fclose(fd);	fd = NULL;
	    xx = rpmDigestFinal(ctx, &digest, &digestlen, asAscii);

	    xx = (EVR && *EVR && digest && *digest) ? strcasecmp(EVR, digest) : -1;
	    /* XXX only equality makes sense for digest compares */
	    if ((Flags & RPMSENSE_EQUAL) && xx == 0) rc = 0;
	}
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(digest probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_SIGNATURE) {
	const char * EVR = rpmdsEVR(dep);
	ARGV_t avN = NULL;
	ARGV_t avEVR = NULL;
	rpmRC res;

	/* Split /fn:/sig */
	xx = argvSplit(&avN, Name, ":");

	/* Split /pub:id */
	xx = (EVR && *EVR) ? argvSplit(&avEVR, EVR, ":") : argvAdd(&avEVR, "");

	res = rpmnsProbeSignature(ts, avN[0], avN[1], avEVR[0], avEVR[1], 0);
	rc = (res == RPMRC_OK ? 0 : 1);

	avN = argvFree(avN);
	avEVR = argvFree(avEVR);

	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(signature probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_VERIFY) {
	QVA_t qva = (QVA_t) memset(alloca(sizeof(*qva)), 0, sizeof(*qva));

	qva->qva_mode = 'v';
	qva->qva_flags = (rpmQueryFlags)
		(VERIFY_ALL & ~(VERIFY_DEPS|VERIFY_SCRIPT));
	rc = 0;		/* assume success */
	if (rpmtsGetRdb(ts) != NULL) {
	    if (!strcmp(Name, "*"))			/* -Va probe */
		mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES, NULL, 0);
	    else if (Name[0] == '/')		/* -Vf probe */
		mi = rpmtsInitIterator(ts, RPMTAG_BASENAMES, Name, 0);
	    else				/* -V probe */
		mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, Name, 0);
	    while ((h = rpmmiNext(mi)) != NULL) {
		if (!(Name[0] == '/' || !strcmp(Name, "*")))
		if (!rpmdsAnyMatchesDep(h, dep, _rpmds_nopromote))
		    continue;
		xx = (showVerifyPackage(qva, ts, h) ? 1 : 0);
		if (xx)
		    rc = 1;
	    }
	    mi = rpmmiFree(mi);
	}

	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(verify probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_GNUPG) {
	const char * EVR = rpmdsEVR(dep);
	if (!(EVR && *EVR)) {
	    static const char gnupg_pre[] = "%(%{__gpg} --batch --no-tty --quiet --verify ";
	    static const char gnupg_post[] = " 2>/dev/null; echo $?)";
	    const char * t = rpmExpand(gnupg_pre, Name, gnupg_post, NULL);
	    rc = (t && t[0] == '0') ? 0 : 1;
	    t = _free(t);
        }
        else {
	    static const char gnupg_pre[] = "%(%{__gpg} --batch --no-tty --quiet --verify ";
	    static const char gnupg_post[] = " 2>&1 | grep '^Primary key fingerprint:' | sed -e 's;^.*: *;;' -e 's; *;;g')";
	    const char * t = rpmExpand(gnupg_pre, Name, gnupg_post, NULL);
	    rc = ((Flags & RPMSENSE_EQUAL) && strcasecmp(EVR, t) == 0) ? 0 : 1;
	    t = _free(t);
        }
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(gnupg probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_MACRO) {
	static const char macro_pre[] = "%{?";
	static const char macro_post[] = ":0}";
	const char * a = rpmExpand(macro_pre, Name, macro_post, NULL);

	rc = (a && a[0] == '0') ? 0 : 1;
	a = _free(a);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(macro probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_ENVVAR) {
	const char * a = envGet(Name);
	const char * b = rpmdsEVR(dep);

	/* Existence test if EVR is missing/empty. */
	if (!(b && *b))
	    rc = (!(a && *a));
	else {
	    int sense = (a && *a) ? strcmp(a, b) : -1;

	    if ((Flags & RPMSENSE_SENSEMASK) == RPMSENSE_NOTEQUAL)
		rc = (sense == 0);
	    else if (sense < 0 && (Flags & RPMSENSE_LESS))
		rc = 0;
	    else if (sense > 0 && (Flags & RPMSENSE_GREATER))
		rc = 0;
	    else if (sense == 0 && (Flags & RPMSENSE_EQUAL))
		rc = 0;
	    else
		rc = (sense != 0);
	}

	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(envvar probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_RUNNING) {
	char *t = NULL;
	pid_t pid = strtol(Name, &t, 10);

	if (t == NULL || *t != '\0') {
	    const char * fn = rpmGetPath("%{_varrun}/", Name, ".pid", NULL);
	    FD_t fd = NULL;

	    if (fn && *fn != '%' && (fd = Fopen(fn, "r.fdio")) && !Ferror(fd)) {
		char buf[32];
		size_t nb = Fread(buf, sizeof(buf[0]), sizeof(buf), fd);

		if (nb > 0)
		    pid = strtol(buf, &t, 10);
	    } else
		pid = 0;
	    if (fd != NULL)
		(void) Fclose(fd);
	    fn = _free(fn);
	}
	rc = (pid > 0 ? (kill(pid, 0) < 0 && errno == ESRCH) : 1);
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(running probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_SANITY) {
	/* XXX only the installer does not have the database open here. */
	rc = 1;		/* assume failure */
	if (rpmtsGetRdb(ts) != NULL) {
	    mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, Name, 0);
	    while ((h = rpmmiNext(mi)) != NULL) {
		if (!rpmdsAnyMatchesDep(h, dep, _rpmds_nopromote))
		    continue;
		rc = (headerIsEntry(h, RPMTAG_SANITYCHECK) == 0);
		if (rc == 0) {
		    /* XXX FIXME: actually run the sanitycheck script. */
		    break;
		}
	    }
	    mi = rpmmiFree(mi);
	}
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(sanity probe)"), rc);
	goto exit;
    }

    if (NSType == RPMNS_TYPE_VCHECK) {
	rc = 1;		/* assume failure */
	if (rpmtsGetRdb(ts) != NULL) {
	    mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, Name, 0);
	    while ((h = rpmmiNext(mi)) != NULL) {
		if (!rpmdsAnyMatchesDep(h, dep, _rpmds_nopromote))
		    continue;
		rc = (headerIsEntry(h, RPMTAG_TRACK) == 0);
		if (rc == 0) {
		    /* XXX FIXME: actually run the vcheck script. */
		    break;
		}
	    }
	    mi = rpmmiFree(mi);
	}
	if (Flags & RPMSENSE_MISSINGOK)
	    goto unsatisfied;
	rpmdsNotify(dep, _("(vcheck probe)"), rc);
	goto exit;
    }

    /* Search system configured provides. */
    if (sysinfo_path == NULL) {
	sysinfo_path = rpmExpand("%{?_rpmds_sysinfo_path}", NULL);
	if (!(sysinfo_path != NULL && *sysinfo_path == '/')) {
	    sysinfo_path = _free(sysinfo_path);
	    sysinfo_path = xstrdup(SYSCONFIGDIR "/sysinfo");
	}
    }

    if (!rpmioAccess(sysinfo_path, NULL, R_OK)) {
#ifdef	NOTYET	/* XXX just sysinfo Provides: for now. */
	rpmTag tagN = (Name[0] == '/' ? RPMTAG_DIRNAMES : RPMTAG_PROVIDENAME);
#else
	rpmTag tagN = RPMTAG_PROVIDENAME;
#endif
	rpmds P = rpmdsFromPRCO(rpmtsPRCO(ts), tagN);
	if (rpmdsSearch(P, dep) >= 0) {
	    rpmdsNotify(dep, _("(sysinfo provides)"), rc);
	    goto exit;
	}
    }

    /*
     * New features in rpm packaging implicitly add versioned dependencies
     * on rpmlib provides. The dependencies look like "rpmlib(YaddaYadda)".
     * Check those dependencies now.
     */
    if (NSType == RPMNS_TYPE_RPMLIB) {
	static int oneshot = -1;

	if (oneshot)
	    oneshot = rpmdsRpmlib(&rpmlibP, NULL);
	if (rpmlibP == NULL)
	    goto unsatisfied;

	if (rpmdsSearch(rpmlibP, dep) >= 0) {
	    rpmdsNotify(dep, _("(rpmlib provides)"), rc);
	    goto exit;
	}
	goto unsatisfied;
    }

    if (NSType == RPMNS_TYPE_CPUINFO) {
	static int oneshot = -1;

	if (oneshot && cpuinfoP == NULL)
	    oneshot = rpmdsCpuinfo(&cpuinfoP, NULL);
	if (cpuinfoP == NULL)
	    goto unsatisfied;

	if (rpmdsSearch(cpuinfoP, dep) >= 0) {
	    rpmdsNotify(dep, _("(cpuinfo provides)"), rc);
	    goto exit;
	}
	goto unsatisfied;
    }

    if (NSType == RPMNS_TYPE_GETCONF) {
	static int oneshot = -1;

	if (oneshot)
	    oneshot = rpmdsGetconf(&getconfP, NULL);
	if (getconfP == NULL)
	    goto unsatisfied;

	if (rpmdsSearch(getconfP, dep) >= 0) {
	    rpmdsNotify(dep, _("(getconf provides)"), rc);
	    goto exit;
	}
	goto unsatisfied;
    }

    if (NSType == RPMNS_TYPE_UNAME) {
	static int oneshot = -1;

	if (oneshot)
	    oneshot = rpmdsUname(&unameP, NULL);
	if (unameP == NULL)
	    goto unsatisfied;

	if (rpmdsSearch(unameP, dep) >= 0) {
	    rpmdsNotify(dep, _("(uname provides)"), rc);
	    goto exit;
	}
	goto unsatisfied;
    }

    if (NSType == RPMNS_TYPE_SONAME) {
	rpmds sonameP = NULL;
	rpmPRCO PRCO = rpmdsNewPRCO(NULL);
	char * fn = strcpy((char *)alloca(strlen(Name)+1), Name);
	int flags = 0;	/* XXX RPMELF_FLAG_SKIPREQUIRES? */
	rpmds ds;

	/* XXX Only absolute paths for now. */
	if (*fn != '/')
	    goto unsatisfied;
	fn[strlen(fn)-1] = '\0';

	/* Extract ELF Provides: from /path/to/DSO. */
	xx = rpmdsELF(fn, flags, rpmdsMergePRCO, PRCO);
	sonameP = rpmdsFromPRCO(PRCO, RPMTAG_PROVIDENAME);
	if (!(xx == 0 && sonameP != NULL))
	    goto unsatisfied;

	/* Search using the original {EVR,"",Flags} from the dep set. */
	ds = rpmdsSingle(rpmdsTagN(dep), rpmdsEVR(dep), "", (evrFlags)Flags);
	xx = rpmdsSearch(sonameP, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
	PRCO = rpmdsFreePRCO(PRCO);

	/* Was the dependency satisfied? */
	if (xx >= 0) {
	    rpmdsNotify(dep, _("(soname provides)"), rc);
	    goto exit;
	}
	goto unsatisfied;
    }

    /* Search added packages for the dependency. */
    if (rpmalSatisfiesDepend(ts->addedPackages, dep, NULL) != NULL) {
#if defined(CACHE_DEPENDENCY_RESULT)
	/*
	 * XXX Ick, context sensitive answers from dependency cache.
	 * XXX Always resolve added dependencies within context to disambiguate.
	 */
	if (_rpmds_nopromote)
	    _cacheThisRC = 0;
#endif
	goto exit;
    }

    /* XXX only the installer does not have the database open here. */
    if (rpmtsGetRdb(ts) != NULL) {
	/* XXX Always satisfy Requires: /, SuSE (others?) doesn't package "/" */
	if (Name[0] == '/' && Name[1] == '\0') {
	    rpmdsNotify(dep, _("(root files)"), rc);
	    goto exit;
	}
	if (Name[0] == '/') {
	    mi = rpmtsInitIterator(ts, RPMTAG_BASENAMES, Name, 0);
	    (void) rpmmiPrune(mi,
			ts->removedPackages, ts->numRemovedPackages, 1);
	    while ((h = rpmmiNext(mi)) != NULL) {
		rpmdsNotify(dep, _("(db files)"), rc);
		mi = rpmmiFree(mi);
		goto exit;
	    }
	    mi = rpmmiFree(mi);
	}

	mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, Name, 0);
	(void) rpmmiPrune(mi,
			ts->removedPackages, ts->numRemovedPackages, 1);
	while ((h = rpmmiNext(mi)) != NULL) {
	    if (rpmdsAnyMatchesDep(h, dep, _rpmds_nopromote)) {
		rpmdsNotify(dep, _("(db provides)"), rc);
		mi = rpmmiFree(mi);
		goto exit;
	    }
	}
	mi = rpmmiFree(mi);
    }

    /*
     * Search for an unsatisfied dependency.
     */
    if (adding == 1 && retries > 0 && !(rpmtsDFlags(ts) & RPMDEPS_FLAG_NOSUGGEST)) {
	if (ts->solve != NULL) {
	    xx = (*ts->solve) (ts, dep, ts->solveData);
	    if (xx == 0)
		goto exit;
	    if (xx == -1) {
		retries--;
		rpmalMakeIndex(ts->addedPackages);
		goto retry;
	    }
	}
    }

unsatisfied:
    if (Flags & RPMSENSE_MISSINGOK) {
	rc = 0;	/* dependency is unsatisfied, but just a hint. */
#if defined(CACHE_DEPENDENCY_RESULT)
	_cacheThisRC = 0;
#endif
	rpmdsNotify(dep, _("(hint skipped)"), rc);
    } else {
	rc = 1;	/* dependency is unsatisfied */
	rpmdsNotify(dep, NULL, rc);
    }

exit:
    /*
     * If dbiOpen/dbiPut fails (e.g. permissions), we can't cache.
     */
#if defined(CACHE_DEPENDENCY_RESULT)
    if (_cacheDependsRC && _cacheThisRC) {
	dbiIndex dbi;
	dbi = dbiOpen(rpmtsGetRdb(ts), RPMDBI_DEPCACHE, 0);
	if (dbi == NULL) {
	    _cacheDependsRC = 0;
	} else {
	    const char * DNEVR;
	    xx = 0;
	    if ((DNEVR = rpmdsDNEVR(dep)) != NULL) {
		DBC * dbcursor = NULL;
		size_t DNEVRlen = strlen(DNEVR);

		xx = dbiCopen(dbi, dbiTxnid(dbi), &dbcursor, DB_WRITECURSOR);

		memset(key, 0, sizeof(*key));
/*@i@*/		key->data = (void *) DNEVR;
		key->size = DNEVRlen;
		memset(data, 0, sizeof(*data));
		data->data = &rc;
		data->size = sizeof(rc);

		/*@-compmempass@*/
		xx = dbiPut(dbi, dbcursor, key, data, 0);
		/*@=compmempass@*/
		xx = dbiCclose(dbi, dbcursor, DB_WRITECURSOR);
	    }
	    if (xx)
		_cacheDependsRC = 0;
	}
    }
#endif

    return rpmdsNegateRC(dep, rc);
}

/**
 * Check added requires/conflicts against against installed+added packages.
 * @param ts		transaction set
 * @param pkgNEVRA	package name-version-release.arch
 * @param requires	Requires: dependencies (or NULL)
 * @param conflicts	Conflicts: dependencies (or NULL)
 * @param dirnames	Dirnames: dependencies (or NULL)
 * @param linktos	Filelinktos: dependencies (or NULL)
 * @param depName	dependency name to filter (or NULL)
 * @param tscolor	color bits for transaction set (0 disables)
 * @param adding	dependency is from added package set?
 * @return		0 = deps ok, 1 = dep problems, 2 = error
 */
static int checkPackageDeps(rpmts ts, const char * pkgNEVRA,
		/*@null@*/ rpmds requires,
		/*@null@*/ rpmds conflicts,
		/*@null@*/ rpmds dirnames,
		/*@null@*/ rpmds linktos,
		/*@null@*/ const char * depName,
		rpmuint32_t tscolor, int adding)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, requires, conflicts, dirnames, linktos,
		rpmGlobalMacroContext, fileSystem, internalState */
{
    rpmps ps = rpmtsProblems(ts);
    rpmuint32_t dscolor;
    const char * Name;
    int terminate = 2;		/* XXX terminate if rc >= terminate */
    int rc;
    int ourrc = 0;
#if defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) || defined(RPM_OPTIONAL_DIRNAME_AND_SYMLINK_DEPS) /* optional-dirname-and-symlink-deps */
    int dirname_deps;
    int symlink_deps;
#endif

    requires = rpmdsInit(requires);
    if (requires != NULL)
    while (ourrc < terminate && rpmdsNext(requires) >= 0) {

	if ((Name = rpmdsN(requires)) == NULL)
	    continue;	/* XXX can't happen */

	/* Filter out requires that came along for the ride. */
	if (depName != NULL && strcmp(depName, Name))
	    continue;

	/* Ignore colored requires not in our rainbow. */
	dscolor = rpmdsColor(requires);
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	rc = unsatisfiedDepend(ts, requires, adding);

	switch (rc) {
	case 0:		/* requirements are satisfied. */
	    /*@switchbreak@*/ break;
	case 1:		/* requirements are not satisfied. */
	{   fnpyKey * suggestedKeys = NULL;

	    if (ts->availablePackages != NULL) {
		suggestedKeys = rpmalAllSatisfiesDepend(ts->availablePackages,
				requires, NULL);
	    }

	    rpmdsProblem(ps, pkgNEVRA, requires, suggestedKeys, adding);

	}
	    ourrc = 1;
	    /*@switchbreak@*/ break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 2;
	    /*@switchbreak@*/ break;
	}
    }

    conflicts = rpmdsInit(conflicts);
    if (conflicts != NULL)
    while (ourrc < terminate && rpmdsNext(conflicts) >= 0) {

	if ((Name = rpmdsN(conflicts)) == NULL)
	    continue;	/* XXX can't happen */

	/* Filter out conflicts that came along for the ride. */
	if (depName != NULL && strcmp(depName, Name))
	    continue;

	/* Ignore colored conflicts not in our rainbow. */
	dscolor = rpmdsColor(conflicts);
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	rc = unsatisfiedDepend(ts, conflicts, adding);

	/* 1 == unsatisfied, 0 == satsisfied */
	switch (rc) {
	case 0:		/* conflicts exist. */
	    rpmdsProblem(ps, pkgNEVRA, conflicts, NULL, adding);
	    ourrc = 1;
	    /*@switchbreak@*/ break;
	case 1:		/* conflicts don't exist. */
	    /*@switchbreak@*/ break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 2;
	    /*@switchbreak@*/ break;
	}
    }

#if defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) || defined(RPM_OPTIONAL_DIRNAME_AND_SYMLINK_DEPS) /* optional-dirname-and-symlink-deps */
    dirname_deps = rpmExpandNumeric("%{?_check_dirname_deps}%{?!_check_dirname_deps:1}");
    if (dirname_deps) {
#endif
    dirnames = rpmdsInit(dirnames);
    if (dirnames != NULL)
    while (ourrc < terminate && rpmdsNext(dirnames) >= 0) {

	if ((Name = rpmdsN(dirnames)) == NULL)
	    continue;	/* XXX can't happen */

	/* Filter out dirnames that came along for the ride. */
	if (depName != NULL && strcmp(depName, Name))
	    continue;

	/* Ignore colored dirnames not in our rainbow. */
	dscolor = rpmdsColor(dirnames);
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	rc = unsatisfiedDepend(ts, dirnames, adding);

	switch (rc) {
	case 0:		/* requirements are satisfied. */
	    /*@switchbreak@*/ break;
	case 1:		/* requirements are not satisfied. */
	{   fnpyKey * suggestedKeys = NULL;

	    if (ts->availablePackages != NULL) {
		suggestedKeys = rpmalAllSatisfiesDepend(ts->availablePackages,
				dirnames, NULL);
	    }

	    rpmdsProblem(ps, pkgNEVRA, dirnames, suggestedKeys, adding);

	}
	    ourrc = 1;
	    /*@switchbreak@*/ break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 2;
	    /*@switchbreak@*/ break;
	}
    }
#if defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) || defined(RPM_OPTIONAL_DIRNAME_AND_SYMLINK_DEPS) /* optional-dirname-and-symlink-deps */
    }

    symlink_deps = rpmExpandNumeric("%{?_check_symlink_deps}%{?!_check_symlink_deps:1}");
    if (symlink_deps) {
#endif
    linktos = rpmdsInit(linktos);
    if (linktos != NULL)
    while (ourrc < terminate && rpmdsNext(linktos) >= 0) {

	if ((Name = rpmdsN(linktos)) == NULL)
	    continue;	/* XXX can't happen */
	if (*Name == '\0')	/* XXX most linktos are empty */
		continue;

	/* Filter out linktos that came along for the ride. */
	if (depName != NULL && strcmp(depName, Name))
	    continue;

	/* Ignore colored linktos not in our rainbow. */
	dscolor = rpmdsColor(linktos);
	if (tscolor && dscolor && !(tscolor & dscolor))
	    continue;

	rc = unsatisfiedDepend(ts, linktos, adding);

	switch (rc) {
	case 0:		/* requirements are satisfied. */
	    /*@switchbreak@*/ break;
	case 1:		/* requirements are not satisfied. */
	{   fnpyKey * suggestedKeys = NULL;

	    if (ts->availablePackages != NULL) {
		suggestedKeys = rpmalAllSatisfiesDepend(ts->availablePackages,
				linktos, NULL);
	    }

	    rpmdsProblem(ps, pkgNEVRA, linktos, suggestedKeys, adding);

	}
	    ourrc = 1;
	    /*@switchbreak@*/ break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 2;
	    /*@switchbreak@*/ break;
	}
    }
#if defined(RPM_VENDOR_MANDRIVA) || defined(RPM_VENDOR_ARK) || defined(RPM_OPTIONAL_DIRNAME_AND_SYMLINK_DEPS) /* optional-dirname-and-symlink-deps */
    }
#endif    

    ps = rpmpsFree(ps);
    return ourrc;
}

/**
 * Check dependency against installed packages.
 * Adding: check name/provides dep against each conflict match,
 * Erasing: check name/provides/filename dep against each requiredby match.
 * @param ts		transaction set
 * @param depName	dependency name
 * @param mi		rpm database iterator
 * @param adding	dependency is from added package set?
 * @return		0 no problems found
 */
static int checkPackageSet(rpmts ts, const char * depName,
		/*@only@*/ /*@null@*/ rpmmi mi, int adding)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, mi, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmdepFlags depFlags = rpmtsDFlags(ts);
    rpmuint32_t tscolor = rpmtsColor(ts);
    int scareMem = 0;
    Header h;
    int terminate = 2;		/* XXX terminate if rc >= terminate */
    int ourrc = 0;

    (void) rpmmiPrune(mi,
		ts->removedPackages, ts->numRemovedPackages, 1);
    while (ourrc < terminate && (h = rpmmiNext(mi)) != NULL) {
	rpmds requires = NULL;
	rpmds conflicts = NULL;
	rpmds dirnames = NULL;
	rpmds linktos = NULL;
	int rc;

	he->tag = RPMTAG_NVRA;
	rc = (headerGet(h, he, 0) ? 0 : 2);
	if (rc > ourrc)
	    ourrc = rc;
	if (ourrc >= terminate) {
	    he->p.str = _free(he->p.str);
	    break;
	}

	if (!(depFlags & RPMDEPS_FLAG_NOREQUIRES))
	    requires = rpmdsNew(h, RPMTAG_REQUIRENAME, scareMem);
	if (!(depFlags & RPMDEPS_FLAG_NOCONFLICTS))
	    conflicts = rpmdsNew(h, RPMTAG_CONFLICTNAME, scareMem);
	if (!(depFlags & RPMDEPS_FLAG_NOPARENTDIRS))
	    dirnames = rpmdsNew(h, RPMTAG_DIRNAMES, scareMem);
	if (!(depFlags & RPMDEPS_FLAG_NOLINKTOS))
	    linktos = rpmdsNew(h, RPMTAG_FILELINKTOS, scareMem);

	(void) rpmdsSetNoPromote(requires, _rpmds_nopromote);
	(void) rpmdsSetNoPromote(conflicts, _rpmds_nopromote);
	(void) rpmdsSetNoPromote(dirnames, _rpmds_nopromote);
	(void) rpmdsSetNoPromote(linktos, _rpmds_nopromote);

	rc = checkPackageDeps(ts, he->p.str,
		requires, conflicts, dirnames, linktos,
		depName, tscolor, adding);

	(void)rpmdsFree(linktos);
	linktos = NULL;
	(void)rpmdsFree(dirnames);
	dirnames = NULL;
	(void)rpmdsFree(conflicts);
	conflicts = NULL;
	(void)rpmdsFree(requires);
	requires = NULL;
	he->p.str = _free(he->p.str);

	if (rc > ourrc)
	    ourrc = rc;
    }
    mi = rpmmiFree(mi);

    return ourrc;
}

/**
 * Check to-be-erased dependencies against installed requires.
 * @param ts		transaction set
 * @param depName	requires name
 * @return		0 no problems found
 */
static int checkDependentPackages(rpmts ts, const char * depName)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int rc = 0;

    /* XXX rpmdb can be closed here, avoid error msg. */
    if (rpmtsGetRdb(ts) != NULL) {
	rpmmi mi;
	mi = rpmtsInitIterator(ts, RPMTAG_REQUIRENAME, depName, 0);
	rc = checkPackageSet(ts, depName, mi, 0);
    }
    return rc;
}

/**
 * Check to-be-added dependencies against installed conflicts.
 * @param ts		transaction set
 * @param depName	conflicts name
 * @return		0 no problems found
 */
static int checkDependentConflicts(rpmts ts, const char * depName)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int rc = 0;

    /* XXX rpmdb can be closed here, avoid error msg. */
    if (rpmtsGetRdb(ts) != NULL) {
	rpmmi mi;
	mi = rpmtsInitIterator(ts, RPMTAG_CONFLICTNAME, depName, 0);
	rc = checkPackageSet(ts, depName, mi, 1);
    }

    return rc;
}

int _rpmtsCheck(rpmts ts)
{
    const char * depName = NULL;
    rpmdepFlags depFlags = rpmtsDFlags(ts);
    rpmuint32_t tscolor = rpmtsColor(ts);
    rpmmi mi = NULL;
    rpmtsi pi = NULL; rpmte p;
    int closeatexit = 0;
    int xx;
    int terminate = 2;		/* XXX terminate if rc >= terminate */
    int rc = 0;
    int ourrc = 0;

if (_rpmts_debug)
fprintf(stderr, "--> %s(%p) tsFlags 0x%x\n", __FUNCTION__, ts, rpmtsFlags(ts));

    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_CHECK), 0);

    /* Do lazy, readonly, open of rpm database. */
    if (rpmtsGetRdb(ts) == NULL && rpmtsDBMode(ts) != -1) {
	rc = (rpmtsOpenDB(ts, rpmtsDBMode(ts)) ? 2 : 0);
	closeatexit = (rc == 0);
    }
    if (rc && (ourrc = rc) >= terminate)
	goto exit;

    ts->probs = rpmpsFree(ts->probs);

    rpmalMakeIndex(ts->addedPackages);

    /*
     * Look at all of the added packages and make sure their dependencies
     * are satisfied.
     */
    pi = rpmtsiInit(ts);
    while (ourrc < terminate && (p = rpmtsiNext(pi, TR_ADDED)) != NULL) {
	rpmds provides, requires, conflicts, dirnames, linktos;
	rpmfi fi;

/*@-nullpass@*/	/* FIX: rpmts{A,O} can return null. */
	rpmlog(RPMLOG_DEBUG, "========== +++ %s %s/%s 0x%x\n",
		rpmteNEVR(p), rpmteA(p), rpmteO(p), rpmteColor(p));
/*@=nullpass@*/
	requires = (!(depFlags & RPMDEPS_FLAG_NOREQUIRES)
	    ? rpmteDS(p, RPMTAG_REQUIRENAME) : NULL);
	conflicts = (!(depFlags & RPMDEPS_FLAG_NOCONFLICTS)
	    ? rpmteDS(p, RPMTAG_CONFLICTNAME) : NULL);
	/* XXX srpm's don't have directory paths. */
	if (p->isSource) {
	    dirnames = NULL;
	    linktos = NULL;
	} else {
	    dirnames = (!(depFlags & RPMDEPS_FLAG_NOPARENTDIRS)
		? rpmteDS(p, RPMTAG_DIRNAMES) : NULL);
	    linktos = (!(depFlags & RPMDEPS_FLAG_NOLINKTOS)
		? rpmteDS(p, RPMTAG_FILELINKTOS) : NULL);
	}

	rc = checkPackageDeps(ts, rpmteNEVRA(p),
			requires, conflicts, dirnames, linktos,
			NULL, tscolor, 1);
	if (rc && (ourrc = rc) >= terminate)
	    break;

	provides = rpmteDS(p, RPMTAG_PROVIDENAME);
	provides = rpmdsInit(provides);
	if (provides != NULL)
	while (ourrc < terminate && rpmdsNext(provides) >= 0) {
	    depName = _free(depName);
	    depName = xstrdup(rpmdsN(provides));

#ifdef	NOTYET
	    if (rpmdsNSType(provides) == RPMNS_TYPE_ENVVAR) {
		const char * EVR = rpmdsEVR(provides);
		if (rpmdsNegateRC(provides, 0))
		    EVR = NULL;
		if (envPut(depName, EVR));
		    rc = 2;
	    } else
#endif

	    /* Adding: check provides key against conflicts matches. */
	    if (checkDependentConflicts(ts, depName))
		rc = 1;
	}
	if (rc && (ourrc = rc) >= terminate)
	    break;

	fi = rpmteFI(p, RPMTAG_BASENAMES);
	fi = rpmfiInit(fi, 0);
	while (ourrc < terminate && rpmfiNext(fi) >= 0) {
	    depName = _free(depName);
	    depName = xstrdup(rpmfiFN(fi));
	    /* Adding: check filename against conflicts matches. */
	    if (checkDependentConflicts(ts, depName))
		rc = 1;
	}
	if (rc && (ourrc = rc) >= terminate)
	    break;
    }
    pi = rpmtsiFree(pi);
    if (rc && (ourrc = rc) >= terminate)
	goto exit;

    /*
     * Look at the removed packages and make sure they aren't critical.
     */
    pi = rpmtsiInit(ts);
    while (ourrc < terminate && (p = rpmtsiNext(pi, TR_REMOVED)) != NULL) {
	rpmds provides;
	rpmfi fi;

/*@-nullpass@*/	/* FIX: rpmts{A,O} can return null. */
	rpmlog(RPMLOG_DEBUG, "========== --- %s %s/%s 0x%x\n",
		rpmteNEVR(p), rpmteA(p), rpmteO(p), rpmteColor(p));
/*@=nullpass@*/

	provides = rpmteDS(p, RPMTAG_PROVIDENAME);
	provides = rpmdsInit(provides);
	if (provides != NULL)
	while (ourrc < terminate && rpmdsNext(provides) >= 0) {
	    depName = _free(depName);
	    depName = xstrdup(rpmdsN(provides));

	    /* Erasing: check provides against requiredby matches. */
	    if (checkDependentPackages(ts, depName))
		rc = 1;
	}
	if (rc && (ourrc = rc) >= terminate)
	    break;

	fi = rpmteFI(p, RPMTAG_BASENAMES);
	fi = rpmfiInit(fi, 0);
	while (ourrc < terminate && rpmfiNext(fi) >= 0) {
	    depName = _free(depName);
	    depName = xstrdup(rpmfiFN(fi));
	    /* Erasing: check filename against requiredby matches. */
	    if (checkDependentPackages(ts, depName))
		rc = 1;
	}
	if (rc && (ourrc = rc) >= terminate)
	    break;
    }
    pi = rpmtsiFree(pi);
    if (rc && (ourrc = rc) >= terminate)
	goto exit;

    /*
     * Make sure transaction dependencies are satisfied.
     */
    {	const char * tsNEVRA = "transaction dependencies";
	rpmds R = rpmdsFromPRCO(rpmtsPRCO(ts), RPMTAG_REQUIRENAME);
	rpmds C = rpmdsFromPRCO(rpmtsPRCO(ts), RPMTAG_CONFLICTNAME);
	rpmds D = NULL;
	rpmds L = NULL;
	const char * dep = NULL;
	int adding = 2;
	tscolor = 0;	/* XXX no coloring for transaction dependencies. */
	rc = checkPackageDeps(ts, tsNEVRA, R, C, D, L, dep, tscolor, adding);
    }
    if (rc && (ourrc = rc) >= terminate)
	goto exit;

exit:
    mi = rpmmiFree(mi);
    pi = rpmtsiFree(pi);
    depName = _free(depName);

    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_CHECK), 0);

    if (closeatexit)
	xx = rpmtsCloseDB(ts);
#if defined(CACHE_DEPENDENCY_RESULT)
    else if (_cacheDependsRC)
	xx = rpmdbCloseDBI(rpmtsGetRdb(ts), RPMDBI_DEPCACHE);
#endif

#ifdef	NOTYET
     /* On failed dependencies, perform the autorollback goal (if any). */
    {	rpmps ps = rpmtsProblems(ts);
	if (rc || rpmpsNumProblems(ps) > 0)
	    (void) rpmtsRollback(ts, RPMPROB_FILTER_NONE, 0, NULL);
	ps = rpmpsFree(ps);
    }
#endif

    return ourrc;
}

int (*rpmtsCheck) (rpmts ts)
	= _rpmtsCheck;
