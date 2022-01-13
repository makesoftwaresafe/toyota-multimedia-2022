/** \ingroup rpmcli
 * \file lib/rpmrollback.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmcb.h>
#include <argv.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>
#include <rpmdb.h>

#include <rpmds.h>
#include "manifest.h"
#include "misc.h"		/* XXX rpmGlob() */

#define	_RPMTE_INTERNAL		/* XXX findErases needs rpmte internals. */
#define	_RPMTS_INTERNAL		/* XXX ts->teErase, ts->probs */
#define _RPMTS_PRINT
#include <rpmgi.h>		/* XXX rpmgiEscapeSpaces */

#include <rpmcli.h>
#define	_RPMROLLBACK_INTERNAL
#include <rpmrollback.h>

#include "debug.h"

/*@access FD_t @*/	/* XXX void * arg */
/*@access rpmts @*/
/*@access rpmte @*/	/* XXX p->hdrid, p->pkgid, p->NEVRA */
/*@access IDTX @*/
/*@access IDT @*/

#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

GENfree(IDTX)
GENfree(IDT)

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

/*@unchecked@*/
static int reverse = -1;

/**
 */
static int IDTintcmp(const void * a, const void * b)
	/*@*/
{
    /*@-castexpose@*/
    return ( reverse * (((IDT)a)->val.u32 - ((IDT)b)->val.u32) );
    /*@=castexpose@*/
}

IDTX IDTXfree(IDTX idtx)
{
    if (idtx) {
	int i;
	if (idtx->idt)
	for (i = 0; i < idtx->nidt; i++) {
	    IDT idt = idtx->idt + i;
	    (void)headerFree(idt->h);
	    idt->h = NULL;
	    idt->key = _free(idt->key);
	}
	idtx->idt = _free(idtx->idt);
	idtx = _free(idtx);
    }
    return NULL;
}

IDTX IDTXnew(void)
{
    IDTX idtx = (IDTX) xcalloc(1, sizeof(*idtx));
    idtx->delta = 10;
    idtx->size = (int)sizeof(*((IDT)0));
    return idtx;
}

IDTX IDTXgrow(IDTX idtx, int need)
{
    if (need < 0) return NULL;
    if (idtx == NULL)
	idtx = IDTXnew();
    if (need == 0) return idtx;

    if ((idtx->nidt + need) > idtx->alloced) {
	while (need > 0) {
	    idtx->alloced += idtx->delta;
	    need -= idtx->delta;
	}
	idtx->idt = (IDT) xrealloc(idtx->idt, (idtx->alloced * idtx->size));
    }
    return idtx;
}

IDTX IDTXsort(IDTX idtx)
{
    if (idtx != NULL && idtx->idt != NULL && idtx->nidt > 0)
	qsort(idtx->idt, idtx->nidt, idtx->size, IDTintcmp);
    return idtx;
}

IDTX IDTXload(rpmts ts, rpmTag tag, rpmuint32_t rbtid)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    IDTX idtx = NULL;
    rpmmi mi;
    Header h;
    rpmuint32_t tid;
    int xx;

    mi = rpmtsInitIterator(ts, tag, NULL, 0);
#ifdef	NOTYET
    (void) rpmmiAddPattern(mi, RPMTAG_NAME, RPMMIRE_DEFAULT, '!gpg-pubkey');
#endif
    while ((h = rpmmiNext(mi)) != NULL) {
	he->tag = tag;
	xx = headerGet(h, he, 0);
	if (!xx || he->p.ui32p == NULL)
	    continue;
	tid = (he->p.ui32p ? he->p.ui32p[0] : 0);
	he->p.ptr = _free(he->p.ptr);

	if (tid == 0 || tid == 0xffffffff)
	    continue;

	/* Don't bother with headers installed prior to the rollback goal. */
	if (tid < rbtid)
	    continue;

	idtx = IDTXgrow(idtx, 1);
	if (idtx == NULL || idtx->idt == NULL)
	    continue;

	{   IDT idt;
	    /*@-nullderef@*/
	    idt = idtx->idt + idtx->nidt;
	    /*@=nullderef@*/
	    idt->done = 0;
	    idt->h = headerLink(h);
	    idt->key = NULL;
	    idt->instance = rpmmiInstance(mi);
	    idt->val.u32 = tid;
	}
	idtx->nidt++;
    }
    mi = rpmmiFree(mi);

    return IDTXsort(idtx);
}

IDTX IDTXglob(rpmts ts, const char * globstr, rpmTag tag, rpmuint32_t rbtid)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    IDTX idtx = NULL;
    Header h;
    rpmuint32_t tid;
    FD_t fd;
    const char ** av = NULL;
    const char * fn;
    int ac = 0;
    rpmRC rpmrc;
    int xx;
    int i;

    av = NULL;	ac = 0;
    fn = rpmgiEscapeSpaces(globstr);
    xx = rpmGlob(fn, &ac, &av);
    fn = _free(fn);

    if (xx == 0)
    for (i = 0; i < ac; i++) {
	int isSource;

	fd = Fopen(av[i], "r.fdio");
	if (fd == NULL || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("open of %s failed: %s\n"), av[i],
	    		Fstrerror(fd));
	    if (fd != NULL) (void) Fclose(fd);
	    continue;
	}

	rpmrc = rpmReadPackageFile(ts, fd, av[i], &h);
	(void) Fclose(fd);
	switch (rpmrc) {
	default:
	    goto bottom;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    isSource =
		(headerIsEntry(h, RPMTAG_SOURCERPM) == 0 &&
		 headerIsEntry(h, RPMTAG_ARCH) != 0);
	    if (isSource)
		goto bottom;
	    /*@switchbreak@*/ break;
	}

{ const char * origin = headerGetOrigin(h);
assert(origin != NULL);
assert(!strcmp(av[i], origin));
}
	he->tag = tag;
	xx = headerGet(h, he, 0);
	if (!xx || he->p.ui32p == NULL)
	    goto bottom;
	tid = (he->p.ui32p ? he->p.ui32p[0] : 0);
	he->p.ptr = _free(he->p.ptr);

	/* Don't bother with headers installed prior to the rollback goal. */
	if (tid < rbtid)
	    goto bottom;

	idtx = IDTXgrow(idtx, 1);
	if (idtx == NULL || idtx->idt == NULL)
	    goto bottom;

	{   IDT idt;
	    idt = idtx->idt + idtx->nidt;
	    idt->done = 0;
	    idt->h = headerLink(h);
	    idt->key = av[i];
	    av[i] = NULL;
	    idt->instance = 0;
	    idt->val.u32 = tid;
	}
	idtx->nidt++;
bottom:
	(void)headerFree(h);
	h = NULL;
    }

    for (i = 0; i < ac; i++)
	av[i] = _free(av[i]);
    av = _free(av);	ac = 0;

    return IDTXsort(idtx);
}

/**
 * Search for string B in argv array AV.
 * @param ts		transaction set
 * @param lname		type of link
 * @param AV		argv array
 * @param AC		no. of args
 * @param B		string
 * @return		1 if found, 0 not found, -1 error
 */
static int cmpArgvStr(rpmts ts, const char *lname, const char ** AV, int AC,
		/*@null@*/ const char * B)
	/*@modifies ts @*/
{
    const char * A;
    int i;

    if (AV != NULL && AC > 0 && B == NULL) {
      if (!strcmp(lname, "NEVRA")) {
	rpmps ps = rpmtsProblems(ts);
	for (i = 0; i < AC && (A = AV[i]) != NULL; i++) {
	    rpmpsAppend(ps, RPMPROB_NOREPACKAGE,
			NULL, NULL,	/* NEVRA, key */
			lname, NULL,	/* dn, bn */
			A,		/* altNEVRA */
			0);
	}
	ps = rpmpsFree(ps);
      }
	return 0;
    }

    if (AV != NULL && B != NULL)
    for (i = 0; i < AC && (A = AV[i]) != NULL; i++) {
	if (*A && *B && !strcmp(A, B))
	    return 1;
    }
    return 0;
}

/**
 * Find (and add to transaction set) all erase elements with matching blink.
 * In addition, recreate any added transaction element linkages.
 *
 * XXX rp->h should have FLINK{HDRID,PKGID,NEVRA} populated.
 * XXX ip->h should have BLINK{HDRID,PKGID,NEVRA} populated.
 * XXX p = ts->teInstall is added transaction element from rp->h.
 *
 * @param ts		transaction set (ts->teInstall set to last added pkg)
 * @param p		most recently added install element (NULL skips linking)
 * @param thistid	current transaction id
 * @param ip		currently installed package(s) to be erased
 * @param niids		no. of currently installed package(s)
 * @return		-1 on error, otherwise no. of erase elemnts added
 */
static int findErases(rpmts ts, /*@null@*/ rpmte p, unsigned thistid,
		/*@null@*/ IDT ip, int niids)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, ip, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int rc = 0;
    int xx;

    /* Erase the previously installed packages for this transaction. 
     * Provided this transaction is not excluded from the rollback.
     */
    while (ip != NULL && ip->val.u32 == thistid) {

	if (ip->done)
	    goto bottom;

	{
	    const char ** flinkPkgid = NULL;
	    const char ** flinkHdrid = NULL;
	    const char ** flinkNEVRA = NULL;
	    rpmuint32_t pn, hn, nn;
	    int bingo;

	    he->tag = RPMTAG_BLINKPKGID;
	    xx = headerGet(ip->h, he, 0);
	    flinkPkgid = he->p.argv;
	    pn = he->c;

	    /* XXX Always erase packages at beginning of upgrade chain. */
	    if (pn == 1 && flinkPkgid[0] != NULL && !strcmp(flinkPkgid[0], RPMTE_CHAIN_END)) {
		flinkPkgid = _free(flinkPkgid);
		goto erase;
	    }

	    he->tag = RPMTAG_BLINKHDRID;
	    xx = headerGet(ip->h, he, 0);
	    flinkHdrid = he->p.argv;
	    hn = he->c;
	    he->tag = RPMTAG_BLINKNEVRA;
	    xx = headerGet(ip->h, he, 0);
	    flinkNEVRA = he->p.argv;
	    nn = he->c;

	    /*
	     * Link data may be missing and can have multiple entries.
	     */
	    /* XXX Until link tags are reliably populated, check in the order
	     * 	NEVRA -> hdrid -> pkgid
	     * because NEVRA is easier to debug (hdrid/pkgid are more precise.)
	    */
	    bingo = 0;
	    if (!bingo)
		bingo = cmpArgvStr(ts, "NEVRA", flinkNEVRA, nn, (p ? p->NEVRA : NULL));
	    if (!bingo)
		bingo = cmpArgvStr(ts, "Hdrid", flinkHdrid, hn, (p ? p->hdrid : NULL));
/*@-nullstate@*/
	    if (!bingo)
		bingo = cmpArgvStr(ts, "Pkgid", flinkPkgid, pn, (p ? p->pkgid : NULL));
/*@=nullstate@*/
	    flinkPkgid = _free(flinkPkgid);
	    flinkHdrid = _free(flinkHdrid);
	    flinkNEVRA = _free(flinkNEVRA);

	    if (bingo < 0) {
		rc = -1;
		goto exit;
	    }

	    if (!bingo)
		goto bottom;
	}

erase:
	rpmlog(RPMLOG_DEBUG, D_("\t--- erase h#%u\n"), ip->instance);

	rc = rpmtsAddEraseElement(ts, ip->h, ip->instance);
	if (rc != 0)
	    goto exit;

	/* Cross link the transaction elements to mimic --upgrade. */
	if (p != NULL) {
	    rpmte q = ts->teErase;
	    xx = rpmteChain(p, q, ip->h, "Rollback");
	}

#ifdef	NOTYET
	ip->instance = 0;
#endif
	ip->done = 1;

bottom:

	/* Go to the next header in the rpmdb */
	niids--;
	if (niids > 0)
	    ip++;
	else
	    ip = NULL;
    }

exit:
    return rc;
}

/** @todo Transaction handling, more, needs work. */
int rpmRollback(rpmts ts, QVA_t ia, const char ** argv)
{
    int ifmask= (INSTALL_UPGRADE|INSTALL_FRESHEN|INSTALL_INSTALL|INSTALL_ERASE);
    unsigned thistid = 0xffffffff;
    unsigned prevtid;
    time_t tid;
    IDTX itids = NULL;
    IDTX rtids = NULL;
    IDT rp;
    int nrids = 0;
    IDT ip;
    int niids = 0;
    int rc = 0;
    rpmVSFlags vsflags, ovsflags;
    int numAdded;
    int numRemoved;
    unsigned int _unsafe_rollbacks = 0;
    rpmtransFlags transFlags = ia->transFlags;
    rpmdepFlags depFlags = ia->depFlags;
    int xx;

    if (argv != NULL && *argv != NULL) {
	rc = -1;
	goto exit;
    }

    _unsafe_rollbacks = rpmExpandNumeric("%{?_unsafe_rollbacks}");

    vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_erase}");
    vsflags = (rpmVSFlags) 0;	/* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
    if (!QVA_ISSET(ia->qva_flags, DIGEST)) {
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
    }
    if (!QVA_ISSET(ia->qva_flags, SIGNATURE)) {
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    }
    if (!QVA_ISSET(ia->qva_flags, HDRCHK)) {
	VSF_SET(vsflags, NOHDRCHK);
    }
    VSF_SET(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif
    ovsflags = rpmtsSetVSFlags(ts, vsflags);

    (void) rpmtsSetFlags(ts, transFlags);
    (void) rpmtsSetDFlags(ts, depFlags);

    /*  Make the transaction a rollback transaction.  In a rollback
     *  a best effort is what we want 
     */
    rpmtsSetType(ts, RPMTRANS_TYPE_ROLLBACK);

    itids = IDTXload(ts, RPMTAG_INSTALLTID, ia->rbtid);
    if (itids != NULL) {
	ip = itids->idt;
	niids = itids->nidt;
    } else {
	ip = NULL;
	niids = 0;
    }

    {	const char * globstr = rpmExpand("%{_repackage_dir}/*/*.rpm", NULL);
	if (globstr == NULL || *globstr == '%') {
	    globstr = _free(globstr);
	    rc = -1;
	    goto exit;
	}
	rtids = IDTXglob(ts, globstr, RPMTAG_REMOVETID, ia->rbtid);

	if (rtids != NULL) {
	    rp = rtids->idt;
	    nrids = rtids->nidt;
	} else {
	    rp = NULL;
	    nrids = 0;
	}
	globstr = _free(globstr);
    }

    {	int notifyFlags;
	notifyFlags = ia->installInterfaceFlags | (rpmIsVerbose() ? INSTALL_LABEL : 0 );
	xx = rpmtsSetNotifyCallback(ts,
			rpmShowProgress, (void *) ((long)notifyFlags));
    }

    /* Run transactions until rollback goal is achieved. */
    do {
	prevtid = thistid;
	rc = 0;
	rpmcliPackagesTotal = 0;
	numAdded = 0;
	numRemoved = 0;
	ia->installInterfaceFlags = (rpmInstallInterfaceFlags)
			(ia->installInterfaceFlags & ~ifmask);

	/* Find larger of the remaining install/erase transaction id's. */
	thistid = 0;
	if (ip != NULL && ip->val.u32 > thistid)
	    thistid = ip->val.u32;
	if (rp != NULL && rp->val.u32 > thistid)
	    thistid = rp->val.u32;

	/* If we've achieved the rollback goal, then we're done. */
	if (thistid == 0 || thistid < ia->rbtid)
	    break;

	/* If we've reached the (configured) rollback goal, then we're done. */
	if (_unsafe_rollbacks && thistid <= _unsafe_rollbacks)
	    break;

	/* Is this transaction excluded from the rollback? */
	if (ia->rbtidExcludes != NULL && ia->numrbtidExcludes > 0)
	{
	    rpmuint32_t *excludedTID;
	    int excluded = 0;
	    for(excludedTID = ia->rbtidExcludes; 
		excludedTID < ia->rbtidExcludes + ia->numrbtidExcludes;
		excludedTID++) {
		if (thistid == *excludedTID) {
		    time_t ttid = (time_t)thistid;
		    rpmlog(RPMLOG_NOTICE,
			_("Excluding TID from rollback: %-24.24s (0x%08x)\n"),
				ctime(&ttid), thistid);
		    excluded = 1;
		    /*@innerbreak@*/ break;	
		}
	    }	
	    if (excluded) {
		/* Iterate over repackaged packages */
		while (rp != NULL && rp->val.u32 == thistid) {
		    /* Go to the next repackaged package */
		    nrids--;
		    if (nrids > 0)
			rp++;
		    else
			rp = NULL;
		}
		/* Iterate over installed packages */
		while (ip != NULL && ip->val.u32 == thistid) {
		    /* Go to the next header in the rpmdb */
		    niids--;
		    if (niids > 0)
			ip++;
		    else
			ip = NULL;
		}
		continue;		/* with next transaction */
	    }
	}

	rpmtsEmpty(ts);
	(void) rpmtsSetFlags(ts, transFlags);
	(void) rpmtsSetDFlags(ts, depFlags);
	ts->probs = rpmpsFree(ts->probs);

	/* Install the previously erased packages for this transaction. 
	 */
	while (rp != NULL && rp->val.u32 == thistid) {
	    if (!rp->done) {
		rpmlog(RPMLOG_DEBUG, D_("\t+++ install %s\n"),
			(rp->key ? rp->key : "???"));

/*@-abstract@*/
		rc = rpmtsAddInstallElement(ts, rp->h, (fnpyKey)rp->key,
			       0, ia->relocations);
/*@=abstract@*/
		if (rc != 0)
		    goto exit;

		numAdded++;
		rpmcliPackagesTotal++;
		if (!(ia->installInterfaceFlags & ifmask))
		    ia->installInterfaceFlags = (rpmInstallInterfaceFlags)
			(ia->installInterfaceFlags | INSTALL_UPGRADE);

		/* Re-add linked (i.e. from upgrade/obsoletes) erasures. */
		rc = findErases(ts, ts->teInstall, thistid, ip, niids);
		if (rc < 0)
		    goto exit;
#ifdef	NOTYET
	    	(void)headerFree(rp->h);
		rpm->h = NULL;
#endif
		rp->done = 1;
	    }

	    /* Go to the next repackaged package */
	    nrids--;
	    if (nrids > 0)
		rp++;
	    else
		rp = NULL;
	}

	/* Re-add pure (i.e. not from upgrade/obsoletes) erasures. */
	rc = findErases(ts, NULL, thistid, ip, niids);
	if (rc < 0)
	    goto exit;

	/* Check that all erasures have been re-added. */
	while (ip != NULL && ip->val.u32 == thistid) {
#ifdef	NOTNOW
/* XXX Prevent incomplete rollback transactions. */
assert(ip->done || ia->no_rollback_links);
#endif
	    if (!(ip->done || ia->no_rollback_links)) {
		numRemoved++;

		if (_unsafe_rollbacks != 0)
		    rpmcliPackagesTotal++;

		if (!(ia->installInterfaceFlags & ifmask))
		    ia->installInterfaceFlags = (rpmInstallInterfaceFlags)
			(ia->installInterfaceFlags | INSTALL_ERASE);
	    }

	    /* Go to the next header in the rpmdb */
	    niids--;
	    if (niids > 0)
		ip++;
	    else
		ip = NULL;
	}

	/* Print any rollback transaction problems */
	xx = rpmcliInstallProblems(ts, _("Missing re-packaged package(s)"), 1);

	/* Anything to do? */
	if (rpmcliPackagesTotal <= 0)
	    break;

	tid = (time_t)thistid;
	rpmlog(RPMLOG_NOTICE,
		_("Rollback packages (+%d/-%d) to %-24.24s (0x%08x):\n"),
			numAdded, numRemoved, ctime(&tid), thistid);

	rc = (ia->rbCheck ? (*ia->rbCheck) (ts) : 0);
	if (rc != 0)
	    goto exit;

	rc = (ia->rbOrder ? (*ia->rbOrder) (ts) : 0);
	if (rc != 0)
	    goto exit;

	/* Drop added/available package indices and dependency sets. */
	rpmtsClean(ts);

	/* Print the transaction set. */
	xx = rpmtsPrint(ts, stdout);

	rc = (ia->rbRun
	    ? (*ia->rbRun)(ts, NULL, (rpmprobFilterFlags)(ia->probFilter|RPMPROB_FILTER_OLDPACKAGE))
	    : 0);
	if (rc != 0)
	    goto exit;

	/* Remove repackaged packages after successful reinstall. */
	if (rtids && !rpmIsDebug()) {
	    int i;
	    rpmlog(RPMLOG_NOTICE, _("Cleaning up repackaged packages:\n"));
	    if (rtids->idt)
	    for (i = 0; i < rtids->nidt; i++) {
		IDT rrp = rtids->idt + i;
		if (rrp->val.u32 != thistid)
		    /*@innercontinue@*/ continue;
		if (rrp->key) {	/* XXX can't happen */
		    rpmlog(RPMLOG_NOTICE, _("\tRemoving %s:\n"), rrp->key);
		    (void) unlink(rrp->key);	/* XXX: Should check rc??? */
		}
	    }
	}

	/* The rpmdb has changed, so reload installed package chains. */
	itids = IDTXfree(itids);
	itids = IDTXload(ts, RPMTAG_INSTALLTID, ia->rbtid);
	if (itids != NULL) {
	    ip = itids->idt;
	    niids = itids->nidt;
	} else {
	    ip = NULL;
	    niids = 0;
	}

	/* Re-position the iterator at the current install tid. */
	while (ip != NULL && ip->val.u32 == thistid) {
	    /* Go to the next header in the rpmdb */
	    niids--;
	    if (niids > 0)
		ip++;
	    else
		ip = NULL;
	}

    } while (1);

exit:
    rtids = IDTXfree(rtids);
    itids = IDTXfree(itids);

    rpmtsEmpty(ts);
    (void) rpmtsSetFlags(ts, transFlags);
    (void) rpmtsSetDFlags(ts, depFlags);

    return rc;
}
