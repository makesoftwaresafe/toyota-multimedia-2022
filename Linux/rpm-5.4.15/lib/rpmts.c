/** \ingroup rpmdep
 * \file lib/rpmts.c
 * Routine(s) to handle a "rpmts" transaction sets.
 */
#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmlog.h>
#include <iosm.h>		/* XXX iosmFileAction */
#include <rpmurl.h>
#include <rpmpgp.h>
#include <rpmmacro.h>		/* XXX rpmtsOpenDB() needs rpmGetPath */
#include <rpmkeyring.h>
#include <rpmhkp.h>
#include <rpmsx.h>

#include <rpmtypes.h>
#define	_RPMTAG_INTERNAL	/* XXX tagStore_s */
#include <rpmtag.h>
#include <pkgio.h>

#define	_RPMDB_INTERNAL		/* XXX almost opaque sigh */
#include "rpmdb.h"		/* XXX stealing db->db_mode. */

#include "rpmal.h"
#include "rpmds.h"
#include "rpmfi.h"
#include "rpmlock.h"
#include "rpmns.h"

#define	_RPMTE_INTERNAL		/* XXX te->h */
#include "rpmte.h"

#define	_RPMTS_INTERNAL
#define	_RPMBAG_INTERNAL
#include "rpmts.h"

#include <rpmcli.h>

#include "fs.h"

/* XXX FIXME: merge with existing (broken?) tests in system.h */
/* portability fiddles */
#if STATFS_IN_SYS_STATVFS
/*@-incondefs@*/
#if defined(__LCLINT__)
/*@-declundef -exportheader -protoparammatch @*/ /* LCL: missing annotation */
extern int statvfs (const char * file, /*@out@*/ struct statvfs * buf)
	/*@globals fileSystem @*/
	/*@modifies *buf, fileSystem @*/;
/*@=declundef =exportheader =protoparammatch @*/
/*@=incondefs@*/
#else
# include <sys/statvfs.h>
#endif
#else
# if STATFS_IN_SYS_VFS
#  include <sys/vfs.h>
# else
#  if STATFS_IN_SYS_MOUNT
#   include <sys/mount.h>
#  else
#   if STATFS_IN_SYS_STATFS
#    include <sys/statfs.h>
#   endif
#  endif
# endif
#endif

#include "debug.h"

/*@access FD_t @*/		/* XXX void * arg */
/*@access rpmdb @*/		/* XXX db->db_chrootDone, NULL */

/*@access rpmDiskSpaceInfo @*/
/*@access rpmKeyring @*/
/*@access rpmps @*/
/*@access rpmte @*/
/*@access rpmtsi @*/
/*@access fnpyKey @*/
/*@access pgpDig @*/
/*@access pgpDigParams @*/

#ifdef __cplusplus
GENfree(rpmDiskSpaceInfo)
GENfree(uint32_t *)
GENfree(fnpyKey *)
GENfree(rpmte *)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmts_debug = 0;

/*@unchecked@*/
int _rpmts_stats = 0;

/*@unchecked@*/
int _rpmts_macros = 0;

int rpmtsCloseDB(rpmts ts)
{
    int rc = 0;

    if (ts->rdb != NULL) {
	(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBGET), &ts->rdb->db_getops);
	(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBPUT), &ts->rdb->db_putops);
	(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBDEL), &ts->rdb->db_delops);
	rc = rpmdbClose(ts->rdb);
	ts->rdb = NULL;
    }
    return rc;
}

int rpmtsOpenDB(rpmts ts, int dbmode)
{
    int rc = 0;

    if (ts->rdb != NULL && ts->dbmode == dbmode)
	return 0;

    (void) rpmtsCloseDB(ts);

    /* XXX there's a db lock race here that is the callers responsibility. */

    ts->dbmode = dbmode;
    rc = rpmdbOpen(ts->rootDir, &ts->rdb, ts->dbmode, (mode_t)0644);
    if (rc) {
	const char * dn = rpmGetPath(ts->rootDir, "%{_dbpath}", NULL);
	rpmlog(RPMLOG_ERR, _("cannot open Packages database in %s\n"), dn);
	dn = _free(dn);
    }
    return rc;
}

int rpmtsRebuildDB(rpmts ts)
{
    void * lock = rpmtsAcquireLock(ts);
    rpmdb db = NULL;
    const char * fn;
    struct stat sb;
    int rc;
    int xx;

    /* XXX Seqno update needs O_RDWR. */
    rc = rpmtsOpenDB(ts, O_RDWR);
    if (rc) goto exit;
    db = rpmtsGetRdb(ts);

    if (!(db->db_api == 3 || db->db_api == 4))
	goto exit;

    rc = rpmtxnCheckpoint(db);
    if (rc) goto exit;

  { size_t dbix;
    for (dbix = 0; dbix < db->db_ndbi; dbix++) {
	tagStore_t dbiTags = &db->db_tags[dbix];

	/* Remove configured secondary indices. */
	switch (dbiTags->tag) {
	case RPMDBI_PACKAGES:
	case RPMDBI_AVAILABLE:
	case RPMDBI_ADDED:
	case RPMDBI_REMOVED:
	case RPMDBI_DEPCACHE:
	case RPMDBI_SEQNO:
	case RPMDBI_BTREE:
	case RPMDBI_HASH:
	case RPMDBI_QUEUE:
	case RPMDBI_RECNO:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	default:
	    fn = rpmGetPath(db->db_root, db->db_home, "/",
		(dbiTags->str != NULL ? dbiTags->str : tagName(dbiTags->tag)),
		NULL);
	    if (!Stat(fn, &sb))
		xx = Unlink(fn);
	    fn = _free(fn);
	    /*@switchbreak@*/ break;
	}

	/* Open (and re-create) each index. */
	(void) dbiOpen(db, dbiTags->tag, db->db_flags);
    }
  }

    /* Unreference header used by associated secondary index callbacks. */
    (void) headerFree(db->db_h);
    db->db_h = NULL;

    /* Reset the Seqno counter to the maximum primary key */
    rpmlog(RPMLOG_DEBUG, D_("rpmdb: max. primary key %u\n"),
		(unsigned)db->db_maxkey);
    fn = rpmGetPath(db->db_root, db->db_home, "/Seqno", NULL);
    if (!Stat(fn, &sb))
	xx = Unlink(fn);
    (void) dbiOpen(db, RPMDBI_SEQNO, db->db_flags);
    fn = _free(fn);

    rc = rpmtxnCheckpoint(db);

    xx = rpmtsCloseDB(ts);

exit:
    lock = rpmtsFreeLock(lock);
    return rc;
}

/*@-compdef@*/ /* keyp might not be defined. */
rpmmi rpmtsInitIterator(const rpmts ts, rpmTag rpmtag,
			const void * keyp, size_t keylen)
{
    rpmmi mi = (ts->rdb == NULL && rpmtsOpenDB(ts, ts->dbmode))
	? NULL
	: rpmmiInit(ts->rdb, rpmtag, keyp, keylen);
    return mi;
}
/*@=compdef@*/

int rpmtsCloseSDB(rpmts ts)
{
    rpmbag bag = ts->bag;
    int rc = 0;

    if (bag != NULL) {
	rpmsdb * sdbp = bag->sdbp;
	int i = bag->nsdbp;
	if (sdbp)
	while (--i >= 0) {
	    rpmdb sdb;
	    if (sdbp[i] == NULL)
		continue;
	    sdb = (rpmdb) sdbp[i]->_db;
	    if (sdb) {
		int xx;
		(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBGET), &sdb->db_getops);
		(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBPUT), &sdb->db_putops);
		(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_DBDEL), &sdb->db_delops);
		xx = rpmdbClose(sdb);
		if (xx && rc == 0)
		    rc = xx;
	    }
	    (void) rpmbagDel(bag, i);
	}
	ts->bag = rpmbagFree(ts->bag);
    }
    return rc;
}

int rpmtsOpenSDB(rpmts ts, int dbmode)
{
    static int has_sdbpath = -1;
    rpmbag bag = ts->bag;
    rpmsdb * sdbp = NULL;
    rpmdb sdb = NULL;
    int sdbmode = O_RDONLY;
    const char * s = NULL;
#ifdef	DYING	/* XXX solevedb's never need chroot prefix. */
    const char * rootDir = ts->rootDir;
#else
    const char * rootDir = "/";
#endif
    ARGV_t av = NULL;
    int ac = 0;
    int rc = 0;
    int xx;
    int i;

    if (bag == NULL) {
	bag = ts->bag = rpmbagNew(NULL, 0);
	if (bag == NULL)
	    goto exit;
    }
    sdbp = bag->sdbp;
    sdb = (rpmdb) (sdbp[0] ? sdbp[0]->_db : NULL);
    sdbmode = (sdbp[0] ? sdbp[0]->dbmode : O_RDONLY);

    if (sdb != NULL && sdbmode == dbmode) {
	rc = 0;
	goto exit;
    }

    if (has_sdbpath < 0)
	has_sdbpath = rpmExpandNumeric("%{?_solve_dbpath:1}");

    /* If not configured, don't try to open. */
    if (has_sdbpath <= 0) {
	rc = 1;
	goto exit;
    }

    s = rpmExpand("%{?_solve_dbpath}", NULL);
    xx = argvSplit(&av, s, ":");
    ac = argvCount(av);

    for (i = 0; i < ac; i++) {
	const char * fn;
	urltype ut;

	if (av[i] == NULL || *av[i] == '\0')
	    continue;

	fn = NULL;
	ut = urlPath(av[i], &fn);

	/* XXX Lstat(fn, &sb) to ensure a directory? */
	addMacro(NULL, "_dbpath", NULL, fn, RMIL_DEFAULT);
	xx = rpmdbOpen(rootDir, &sdb, dbmode, (mode_t)0644);
	delMacro(NULL, "_dbpath");

	if (xx) {
	    const char * dn = rpmGetPath(rootDir, "/", fn, NULL);
	    rpmlog(RPMLOG_WARNING, _("cannot open Solve database in %s\n"), dn);
	    dn = _free(dn);
	    if (rc == 0)
		rc = xx;

	    /* XXX only try to open the solvedb once. */
	    has_sdbpath = 0;
	    continue;
	}

	xx = rpmbagAdd(bag, sdb, dbmode);
    }

    av = argvFree(av);
    s = _free(s);

exit:
if (_rpmts_debug)
fprintf(stderr, "<-- %s(%p, 0%o) rc %d\n", __FUNCTION__, ts, dbmode, rc);
    return rc;
}

/**
 * Compare suggested package resolutions (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
static int sugcmp(const void * a, const void * b)
	/*@*/
{
    const char * astr = *(const char **)a;
    const char * bstr = *(const char **)b;
    return strcmp(astr, bstr);
}

int rpmtsSolve(rpmts ts, rpmds ds, /*@unused@*/ const void * data)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmbag bag = ts->bag;
    rpmsdb * sdbp = NULL;
    const char * errstr = NULL;
    const char * str = NULL;
    rpmmi mi;
    Header bh = NULL;
    Header h = NULL;
    size_t bhnamelen = 0;
    time_t bhtime = 0;
    rpmTag rpmtag;
    const char * keyp;
    size_t keylen = 0;
    int rc = 1;		/* assume not found */
    int xx;
    int i;

if (_rpmts_debug)
fprintf(stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, ts, ds, data);

    /* Make suggestions only for installing Requires: */
    if (ts->goal != TSM_INSTALL)
	goto exit;

    switch (rpmdsTagN(ds)) {
    case RPMTAG_CONFLICTNAME:
    default:
	goto exit;
	/*@notreached@*/ break;
    case RPMTAG_DIRNAMES:	/* XXX perhaps too many wrong answers? */
    case RPMTAG_REQUIRENAME:
    case RPMTAG_FILELINKTOS:
	break;
    }

    keyp = rpmdsN(ds);
    if (keyp == NULL)
	goto exit;

    if (bag == NULL) {
	xx = rpmtsOpenSDB(ts, O_RDONLY);
	if (xx)
	    goto exit;
	bag = ts->bag;
	if (bag == NULL)
	    goto exit;
    }

    sdbp = bag->sdbp;

    if (sdbp)
    for (i = 0; i < (int)bag->nsdbp; i++) {
	rpmdb sdb = NULL;

	if (sdbp[i] == NULL)
	    continue;
	sdb = (rpmdb) sdbp[i]->_db;
	if (sdb == NULL)
	    continue;

	/* Look for a matching Provides: in suggested universe. */
	rpmtag = (*keyp == '/' ? RPMTAG_FILEPATHS : RPMTAG_PROVIDENAME);
	mi = rpmmiInit(sdb, rpmtag, keyp, keylen);
	while ((h = rpmmiNext(mi)) != NULL) {
	    size_t hnamelen;
	    time_t htime;

	    if (rpmtag == RPMTAG_PROVIDENAME && !rpmdsAnyMatchesDep(h, ds, 1))
		continue;

	    he->tag = RPMTAG_NAME;
	    xx = headerGet(h, he, 0);
	    hnamelen = ((xx && he->p.str) ? strlen(he->p.str) : 0);
	    he->p.ptr = _free(he->p.ptr);

	    /* XXX Prefer the shortest pkg N for basenames/provides resp. */
	    if (bhnamelen > 0 && hnamelen > bhnamelen)
		continue;

	    /* XXX Prefer the newest build if given alternatives. */
	    he->tag = RPMTAG_BUILDTIME;
	    xx = headerGet(h, he, 0);
	    htime = (xx && he->p.ui32p ? he->p.ui32p[0] : 0);
	    he->p.ptr = _free(he->p.ptr);

	    if (htime <= bhtime)
		continue;

	    /* Save new "best" candidate. */
	    (void)headerFree(bh);
	    bh = NULL;
	    bh = headerLink(h);
	    bhtime = htime;
	    bhnamelen = hnamelen;
	}
	mi = rpmmiFree(mi);
    }

    /* Is there a suggested resolution? */
    if (bh == NULL)
	goto exit;

    /* Get the path to the package file. */
    he->tag = RPMTAG_PACKAGEORIGIN;
    he->p.ptr = NULL;
    xx = headerGet(bh, he, 0);
    if (he->p.str) {
	str = he->p.str;
	he->p.str = NULL;
    } else {
	/* Format the suggested resolution path. */
	const char * qfmt = rpmExpand("%{?_solve_name_fmt}", NULL);
	if (qfmt == NULL || *qfmt == '\0') {
	    qfmt = _free(qfmt);		/* coverity #1214095 */
	    goto exit;
	}
	str = headerSprintf(bh, qfmt, NULL, rpmHeaderFormats, &errstr);
	qfmt = _free(qfmt);
    }

    (void) headerFree(bh);
    bh = NULL;
    if (errstr) {
	rpmlog(RPMLOG_ERR, _("incorrect solve path format: %s\n"), errstr);
	goto exit;
    }

    if (ts->depFlags & RPMDEPS_FLAG_ADDINDEPS) {
	FD_t fd;
	rpmRC rpmrc;

	fd = Fopen(str, "r.fdio");
	if (fd == NULL || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("open of %s failed: %s\n"), str,
			Fstrerror(fd));
            if (fd != NULL) {
                xx = Fclose(fd);
                fd = NULL;
            }
	    str = _free(str);
	    goto exit;
	}
	rpmrc = rpmReadPackageFile(ts, fd, str, &h);
	xx = Fclose(fd);
	switch (rpmrc) {
	default:
	    str = _free(str);
	    break;
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
	case RPMRC_OK:
	    if (h != NULL &&
	        !rpmtsAddInstallElement(ts, h, (fnpyKey)str, 1, NULL))
	    {
		rpmlog(RPMLOG_DEBUG, D_("Adding: %s\n"), str);
		rc = -1;	/* XXX restart unsatisfiedDepends() */
		break;
	    }
	    break;
	}
	(void)headerFree(h);
	h = NULL;
	goto exit;
    }

    rpmlog(RPMLOG_DEBUG, D_("Suggesting: %s\n"), str);
    /* If suggestion is already present, don't bother. */
    if (ts->suggests != NULL && ts->nsuggests > 0) {
	if (bsearch(&str, ts->suggests, ts->nsuggests,
			sizeof(*ts->suggests), sugcmp))
	{
	    str = _free(str);
	    goto exit;
	}
    }

    /* Add a new (unique) suggestion. */
    ts->suggests = (const void **) xrealloc(ts->suggests,
			sizeof(*ts->suggests) * (ts->nsuggests + 2));
    ts->suggests[ts->nsuggests] = str;
    ts->nsuggests++;
    ts->suggests[ts->nsuggests] = NULL;

    if (ts->nsuggests > 1)
	qsort(ts->suggests, ts->nsuggests, sizeof(*ts->suggests), sugcmp);

exit:
if (_rpmts_debug)
fprintf(stderr, "<-- %s(%p,%p,%p) rc %d N %s EVR %s F 0x%x\n", __FUNCTION__, ts, ds, data, rc, rpmdsN(ds), rpmdsEVR(ds), rpmdsFlags(ds));
    return rc;
}

int rpmtsAvailable(rpmts ts, const rpmds ds)
{
    fnpyKey * sugkey;
    int rc = 1;	/* assume not found */

    if (ts->availablePackages == NULL)
	return rc;
    sugkey = rpmalAllSatisfiesDepend(ts->availablePackages, ds, NULL);
    if (sugkey == NULL)
	return rc;

    /* XXX no alternatives yet */
    if (sugkey[0] != NULL) {
	ts->suggests = (const void **) xrealloc(ts->suggests,
			sizeof(*ts->suggests) * (ts->nsuggests + 2));
	ts->suggests[ts->nsuggests] = sugkey[0];
	sugkey[0] = NULL;
	ts->nsuggests++;
	ts->suggests[ts->nsuggests] = NULL;
    }
    sugkey = _free(sugkey);
    return rc;
}

int rpmtsSetSolveCallback(rpmts ts,
		int (*solve) (rpmts ts, rpmds key, const void * data),
		const void * solveData)
{
    int rc = 0;

    if (ts) {
/*@-assignexpose -temptrans @*/
	ts->solve = solve;
	ts->solveData = solveData;
/*@=assignexpose =temptrans @*/
    }
    return rc;
}

rpmps rpmtsProblems(rpmts ts)
{
    static const char msg[] = "rpmtsProblems";
    rpmps ps = NULL;
    if (ts) {
	if (ts->probs == NULL)
	    ts->probs = rpmpsCreate();
/*@-castexpose@*/
	ps = rpmpsLink(ts->probs, msg);
/*@=castexpose@*/
    }
    return ps;
}

void rpmtsClean(rpmts ts)
{
    rpmtsi pi; rpmte p;

    if (ts == NULL)
	return;

    /* Clean up after dependency checks. */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType)0)) != NULL)
	rpmteCleanDS(p);
    pi = rpmtsiFree(pi);

    ts->addedPackages = rpmalFree(ts->addedPackages);
    ts->numAddedPackages = 0;

    ts->erasedPackages = rpmalFree(ts->erasedPackages);
    ts->numErasedPackages = 0;

    ts->suggests = _free(ts->suggests);
    ts->nsuggests = 0;

    ts->probs = rpmpsFree(ts->probs);

    rpmtsCleanDig(ts);
}

void rpmtsEmpty(rpmts ts)
{
    rpmtsi pi; rpmte p;
    int oc;

    if (ts == NULL)
	return;

/*@-nullstate@*/	/* FIX: partial annotations */
    rpmtsClean(ts);
/*@=nullstate@*/

    for (pi = rpmtsiInit(ts), oc = 0; (p = rpmtsiNext(pi, (rpmElementType)0)) != NULL; oc++) {
/*@-type -unqualifiedtrans @*/
	ts->order[oc] = rpmteFree(ts->order[oc]);
/*@=type =unqualifiedtrans @*/
    }
    pi = rpmtsiFree(pi);
    ts->numAddedFiles = 0;
    ts->numErasedFiles = 0;

    ts->orderCount = 0;
    ts->ntrees = 0;
    ts->maxDepth = 0;

    ts->numRemovedPackages = 0;
/*@-nullstate@*/	/* FIX: partial annotations */
    return;
/*@=nullstate@*/
}

static void rpmtsPrintStat(const char * name, /*@null@*/ struct rpmop_s * op)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    static unsigned int scale = (1000 * 1000);
    if (op != NULL && op->count > 0)
	fprintf(stderr, "   %s %8d %6lu.%06lu MB %6lu.%06lu secs\n",
		name, op->count,
		(unsigned long)op->bytes/scale, (unsigned long)op->bytes%scale,
		op->usecs/scale, op->usecs%scale);
}

/*@unchecked@*/ /*@relnull@*/
extern rpmop _hdr_loadops;
/*@unchecked@*/ /*@relnull@*/
extern rpmop _hdr_getops;

static void rpmtsPrintStats(rpmts ts)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_TOTAL), 0);

    if (_hdr_loadops)
	(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_HDRLOAD), _hdr_loadops);
    if (_hdr_getops)
	(void) rpmswAdd(rpmtsOp(ts, RPMTS_OP_HDRGET), _hdr_getops);

    rpmtsPrintStat("total:       ", rpmtsOp(ts, RPMTS_OP_TOTAL));
    rpmtsPrintStat("check:       ", rpmtsOp(ts, RPMTS_OP_CHECK));
    rpmtsPrintStat("order:       ", rpmtsOp(ts, RPMTS_OP_ORDER));
    rpmtsPrintStat("fingerprint: ", rpmtsOp(ts, RPMTS_OP_FINGERPRINT));
    rpmtsPrintStat("repackage:   ", rpmtsOp(ts, RPMTS_OP_REPACKAGE));
    rpmtsPrintStat("install:     ", rpmtsOp(ts, RPMTS_OP_INSTALL));
    rpmtsPrintStat("erase:       ", rpmtsOp(ts, RPMTS_OP_ERASE));
    rpmtsPrintStat("scriptlets:  ", rpmtsOp(ts, RPMTS_OP_SCRIPTLETS));
    rpmtsPrintStat("compress:    ", rpmtsOp(ts, RPMTS_OP_COMPRESS));
    rpmtsPrintStat("uncompress:  ", rpmtsOp(ts, RPMTS_OP_UNCOMPRESS));
    rpmtsPrintStat("digest:      ", rpmtsOp(ts, RPMTS_OP_DIGEST));
    rpmtsPrintStat("signature:   ", rpmtsOp(ts, RPMTS_OP_SIGNATURE));
    rpmtsPrintStat("dbadd:       ", rpmtsOp(ts, RPMTS_OP_DBADD));
    rpmtsPrintStat("dbremove:    ", rpmtsOp(ts, RPMTS_OP_DBREMOVE));
    rpmtsPrintStat("dbget:       ", rpmtsOp(ts, RPMTS_OP_DBGET));
    rpmtsPrintStat("dbput:       ", rpmtsOp(ts, RPMTS_OP_DBPUT));
    rpmtsPrintStat("dbdel:       ", rpmtsOp(ts, RPMTS_OP_DBDEL));
    rpmtsPrintStat("readhdr:     ", rpmtsOp(ts, RPMTS_OP_READHDR));
    rpmtsPrintStat("hdrload:     ", rpmtsOp(ts, RPMTS_OP_HDRLOAD));
    rpmtsPrintStat("hdrget:      ", rpmtsOp(ts, RPMTS_OP_HDRGET));
/*@-globstate@*/
    return;
/*@=globstate@*/
}

static void rpmtsFini(void * _ts)
	/*@modifies _ts @*/
{
    rpmts ts = (rpmts) _ts;

/*@-nullstate@*/	/* FIX: partial annotations */
    /* XXX there's a recursion here ... release and reacquire the lock */
#ifndef	BUGGY
    yarnRelease(ts->_item.use);	/* XXX hack-o-round */
#endif
    rpmtsEmpty(ts);
#ifndef	BUGGY
    yarnPossess(ts->_item.use);	/* XXX hack-o-round */
#endif
/*@=nullstate@*/

    ts->PRCO = rpmdsFreePRCO(ts->PRCO);

    (void) rpmtsCloseDB(ts);
assert(ts->txn == NULL);	/* XXX FIXME */
    ts->txn = NULL;

    (void) rpmtsCloseSDB(ts);

    (void) rpmbfFree(ts->rbf);
    ts->rbf = NULL;
    ts->removedPackages = _free(ts->removedPackages);

    ts->availablePackages = rpmalFree(ts->availablePackages);
    ts->numAvailablePackages = 0;

    ts->dsi = _free(ts->dsi);

    if (ts->scriptFd != NULL) {
/*@-refcounttrans@*/	/* FIX: XfdFree annotation */
	ts->scriptFd = fdFree(ts->scriptFd, __FUNCTION__);
/*@=refcounttrans@*/
	ts->scriptFd = NULL;
    }
    ts->rootDir = _free(ts->rootDir);
    ts->currDir = _free(ts->currDir);

/*@-type +voidabstract @*/	/* FIX: double indirection */
    ts->order = _free(ts->order);
/*@=type =voidabstract @*/
    ts->orderAlloced = 0;

    ts->keyring = rpmKeyringFree(ts->keyring);
    (void) rpmhkpFree(ts->hkp);
    ts->hkp = NULL;

    if (_rpmts_stats)
	rpmtsPrintStats(ts);

    if (_rpmts_macros) {
	const char ** av = NULL;
/*@-globs@*/	/* Avoid rpmGlobalMcroContext et al. */
	(void)rpmGetMacroEntries(NULL, NULL, 1, &av);
/*@=globs@*/
	argvPrint("macros used", av, NULL);
	av = argvFree(av);
    }
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmtsPool;

static rpmts rpmtsGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmtsPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmtsPool, fileSystem, internalState @*/
{
    rpmts ts;

    if (_rpmtsPool == NULL) {
	_rpmtsPool = rpmioNewPool("ts", sizeof(*ts), -1, _rpmts_debug,
			NULL, NULL, rpmtsFini);
	pool = _rpmtsPool;
    }
    ts = (rpmts) rpmioGetPool(pool, sizeof(*ts));
    memset(((char *)ts)+sizeof(ts->_item), 0, sizeof(*ts)-sizeof(ts->_item));
    return ts;
}

void * rpmtsGetKeyring(rpmts ts, /*@unused@*/ int autoload)
{
    rpmKeyring keyring = NULL;
    if (ts) {
#ifdef	NOTYET
	if (ts->keyring == NULL && autoload)
	    loadKeyring(ts);
	keyring = rpmKeyringLink(ts->keyring);
#else
	keyring = ts->keyring;
#endif
    }
/*@-refcounttrans@*/
    return (void *)keyring;
/*@=refcounttrans@*/
}

int rpmtsSetKeyring(rpmts ts, void * _keyring)
{
    rpmKeyring keyring = (rpmKeyring) _keyring;

    if (ts == NULL)
       return -1;

#ifdef	NOTYET
    /*
     * Should we permit switching keyring on the fly? For now, require
     * rpmdb isn't open yet (fairly arbitrary limitation)...
     */
    if (rpmtsGetRdb(ts) != NULL)
	return -1;
#endif

/*@-modnomods@*/
    ts->keyring = rpmKeyringFree(ts->keyring);
/*@=modnomods@*/

#ifdef	NOTYET
    ts->keyring = rpmKeyringLink(keyring);
#else
/*@-assignexpose -newreftrans @*/
/*@i@*/    ts->keyring = keyring;
/*@=assignexpose =newreftrans @*/
#endif

    return 0;
}

rpmVSFlags rpmtsVSFlags(/*@unused@*/ rpmts ts)
{
    return pgpDigVSFlags;
}

rpmVSFlags rpmtsSetVSFlags(/*@unused@*/ rpmts ts, rpmVSFlags vsflags)
	/*@globals pgpDigVSFlags @*/
	/*@modifies pgpDigVSFlags @*/
{
    rpmVSFlags ovsflags;
    ovsflags = pgpDigVSFlags;
    pgpDigVSFlags = vsflags;
    return ovsflags;
}

/*
 * This allows us to mark transactions as being of a certain type.
 * The three types are:
 *
 *     RPM_TRANS_NORMAL 	
 *     RPM_TRANS_ROLLBACK
 *     RPM_TRANS_AUTOROLLBACK
 *
 * ROLLBACK and AUTOROLLBACK transactions should always be ran as
 * a best effort.  In particular this is important to the autorollback
 * feature to avoid rolling back a rollback (otherwise known as
 * dueling rollbacks (-;).  AUTOROLLBACK's additionally need instance
 * counts passed to scriptlets to be altered.
 */
/* Let them know what type of transaction we are */
rpmTSType rpmtsType(rpmts ts)
{
    return (rpmTSType) ((ts != NULL) ? ts->type : 0);
}

void rpmtsSetType(rpmts ts, rpmTSType type)
{
    if (ts != NULL)
	ts->type = type;
}

rpmuint32_t rpmtsARBGoal(rpmts ts)
{
    return ((ts != NULL) ?  ts->arbgoal : 0);
}

void rpmtsSetARBGoal(rpmts ts, rpmuint32_t goal)
{
    if (ts != NULL)
	ts->arbgoal = goal;
}

int rpmtsUnorderedSuccessors(rpmts ts, int first)
{
    int unorderedSuccessors = 0;
    if (ts != NULL) {
	unorderedSuccessors = ts->unorderedSuccessors;
	if (first >= 0)
	    ts->unorderedSuccessors = first;
    }
    return unorderedSuccessors;
}

const char * rpmtsRootDir(rpmts ts)
{
    const char * rootDir = NULL;

    if (ts != NULL && ts->rootDir != NULL) {
	urltype ut = urlPath(ts->rootDir, &rootDir);
	switch (ut) {
	case URL_IS_UNKNOWN:
	case URL_IS_PATH:
	    break;
	case URL_IS_DASH:
	case URL_IS_HKP:
	case URL_IS_FTP:
	case URL_IS_HTTP:
	case URL_IS_HTTPS:
	case URL_IS_MONGO:	/* XXX FIXME */
	default:
	    rootDir = "/";
	    break;
	}
    }
    return rootDir;
}

void rpmtsSetRootDir(rpmts ts, const char * rootDir)
{
    if (ts != NULL) {
	size_t rootLen;

	ts->rootDir = _free(ts->rootDir);

	if (rootDir == NULL) {
#ifdef	DYING
	    ts->rootDir = xstrdup("");
#endif
	    return;
	}
	rootLen = strlen(rootDir);

	/* Make sure that rootDir has trailing / */
	if (!(rootLen && rootDir[rootLen - 1] == '/')) {
	    char * t = (char *) alloca(rootLen + 2);
	    *t = '\0';
	    (void) stpcpy( stpcpy(t, rootDir), "/");
	    rootDir = t;
	}
	ts->rootDir = xstrdup(rootDir);
    }
}

const char * rpmtsCurrDir(rpmts ts)
{
    const char * currDir = NULL;
    if (ts != NULL) {
	currDir = ts->currDir;
    }
    return currDir;
}

void rpmtsSetCurrDir(rpmts ts, const char * currDir)
{
    if (ts != NULL) {
	ts->currDir = _free(ts->currDir);
	if (currDir)
	    ts->currDir = xstrdup(currDir);
    }
}

FD_t rpmtsScriptFd(rpmts ts)
{
    FD_t scriptFd = NULL;
    if (ts != NULL) {
	scriptFd = ts->scriptFd;
    }
/*@-compdef -refcounttrans -usereleased@*/
    return scriptFd;
/*@=compdef =refcounttrans =usereleased@*/
}

void rpmtsSetScriptFd(rpmts ts, FD_t scriptFd)
{

    if (ts != NULL) {
	if (ts->scriptFd != NULL) {
/*@-assignexpose@*/
	    ts->scriptFd = fdFree(ts->scriptFd, "rpmtsSetScriptFd");
/*@=assignexpose@*/
	    ts->scriptFd = NULL;
	}
/*@-assignexpose -castexpose @*/
	if (scriptFd != NULL)
	    ts->scriptFd = fdLink((void *)scriptFd, "rpmtsSetScriptFd");
/*@=assignexpose =castexpose @*/
    }
}

int rpmtsSELinuxEnabled(rpmts ts)
{
    int selinuxEnabled = 0;
    if (ts)
	selinuxEnabled = (ts->selinuxEnabled > 0);
    return selinuxEnabled;
}

int rpmtsChrootDone(rpmts ts)
{
    return (ts != NULL ? ts->chrootDone : 0);
}

int rpmtsSetChrootDone(rpmts ts, int chrootDone)
{
    int ochrootDone = 0;
    if (ts != NULL) {
	ochrootDone = ts->chrootDone;
	if (ts->rdb != NULL)
	    ts->rdb->db_chrootDone = chrootDone;
	ts->chrootDone = chrootDone;
    }
    return ochrootDone;
}

rpmuint32_t rpmtsGetTid(rpmts ts)
{
    rpmuint32_t tid = 0;	/* XXX -1 is time(2) error return. */
    if (ts != NULL) {
	tid = ts->tid[0];
    }
    return tid;
}

rpmuint32_t rpmtsSetTid(rpmts ts, rpmuint32_t tid)
{
    rpmuint32_t otid = 0;	/* XXX -1 is time(2) error return. */
    if (ts != NULL) {
	otid = ts->tid[0];
	ts->tid[0] = tid;
	ts->tid[1] = 0;
    }
    return otid;
}

rpmPRCO rpmtsPRCO(rpmts ts)
{
    rpmPRCO PRCO = NULL;

    if (ts != NULL) {
	static int oneshot = 0;
	if (!oneshot) {
	    const char * fn = rpmGetPath("%{?_rpmds_sysinfo_path}", NULL);
	    int xx;

	    ts->PRCO = rpmdsNewPRCO(NULL);
	    if (fn && *fn != '\0' && !rpmioAccess(fn, NULL, R_OK))
		xx = rpmdsSysinfo(ts->PRCO, NULL);
	    fn = _free(fn);
	    oneshot++;
	}
	PRCO = ts->PRCO;
    }
/*@-compdef -retexpose -usereleased @*/
    return PRCO;
/*@=compdef =retexpose =usereleased @*/
}

int rpmtsInitDSI(const rpmts ts)
{
    rpmDiskSpaceInfo dsi;
    struct stat sb;
    int rc;
    size_t i;

    if (rpmtsFilterFlags(ts) & RPMPROB_FILTER_DISKSPACE)
	return 0;
    if (ts->filesystems != NULL)
	return 0;

    rpmlog(RPMLOG_DEBUG, D_("mounted filesystems:\n"));
    rpmlog(RPMLOG_DEBUG,
	D_("    i        dev    bsize       bavail       iavail mount point\n"));

    rc = rpmGetFilesystemList(&ts->filesystems, &ts->filesystemCount);
    if (rc || ts->filesystems == NULL || ts->filesystemCount == 0)
	return rc;

    /* Get available space on mounted file systems. */

    ts->dsi = _free(ts->dsi);
    ts->dsi = (rpmDiskSpaceInfo) xcalloc((ts->filesystemCount + 1), sizeof(*ts->dsi));

    dsi = ts->dsi;

    if (dsi != NULL)
    for (i = 0; (i < ts->filesystemCount) && dsi; i++, dsi++) {
#if STATFS_IN_SYS_STATVFS
	struct statvfs sfb;
	memset(&sfb, 0, sizeof(sfb));
	rc = statvfs(ts->filesystems[i], &sfb);
#else
	struct statfs sfb;
	memset(&sfb, 0, sizeof(sfb));
#  if STAT_STATFS4
/* This platform has the 4-argument version of the statfs call.  The last two
 * should be the size of struct statfs and 0, respectively.  The 0 is the
 * filesystem type, and is always 0 when statfs is called on a mounted
 * filesystem, as we're doing.
 */
	rc = statfs(ts->filesystems[i], &sfb, sizeof(sfb), 0);
#  else
	rc = statfs(ts->filesystems[i], &sfb);
#  endif
#endif
	if (rc)
	    break;

	rc = stat(ts->filesystems[i], &sb);
	if (rc)
	    break;
	dsi->dev = sb.st_dev;
/* XXX figger out how to get this info for non-statvfs systems. */
#if STATFS_IN_SYS_STATVFS
	dsi->f_frsize = sfb.f_frsize;
#if defined(RPM_OS_AIX)
	dsi->f_fsid = 0; /* sfb.f_fsid is a structure on AIX */
#else
	dsi->f_fsid = sfb.f_fsid;
#endif
	dsi->f_flag = sfb.f_flag;
	dsi->f_favail = (long long) sfb.f_favail;
	dsi->f_namemax = sfb.f_namemax;
#elif defined(__APPLE__) && defined(__MACH__) && !defined(_SYS_STATVFS_H_)
	dsi->f_fsid = 0; /* "Not meaningful in this implementation." */
	dsi->f_namemax = pathconf(ts->filesystems[i], _PC_NAME_MAX);
#elif defined(__OpenBSD__)
	dsi->f_fsid = 0; /* sfb.f_fsid is a structure on OpenBSD */
	dsi->f_namemax = pathconf(ts->filesystems[i], _PC_NAME_MAX);
#else
	dsi->f_fsid = sfb.f_fsid;
	dsi->f_namemax = sfb.f_namelen;
#endif

	dsi->f_bsize = sfb.f_bsize;
	dsi->f_blocks = (unsigned long long)sfb.f_blocks;
	dsi->f_bfree = (unsigned long long)sfb.f_bfree;
	dsi->f_files = (unsigned long long)sfb.f_files;
	dsi->f_ffree = (unsigned long long)sfb.f_ffree;

	dsi->bneeded = 0;
	dsi->ineeded = 0;
#ifdef STATFS_HAS_F_BAVAIL
	dsi->f_bavail = (long long)(sfb.f_bavail ? sfb.f_bavail : 1);
	if (sfb.f_ffree > 0 && sfb.f_files > 0 && sfb.f_favail > 0)
	    dsi->f_favail = (long long)sfb.f_favail;
	else	/* XXX who knows what evil lurks here? */
	    dsi->f_favail = !(sfb.f_ffree == 0 && sfb.f_files == 0)
				?  (signed long long) sfb.f_ffree : -1;
#else
/* FIXME: the statfs struct doesn't have a member to tell how many blocks are
 * available for non-superusers.  f_blocks - f_bfree is probably too big, but
 * it's about all we can do.
 */
	dsi->f_bavail = sfb.f_blocks - sfb.f_bfree;
	/* XXX Avoid FAT and other file systems that have not inodes. */
	dsi->f_favail = !(sfb.f_ffree == 0 && sfb.f_files == 0)
				? sfb.f_ffree : -1;
#endif

#if !defined(ST_RDONLY)
#define	ST_RDONLY	1
#endif
	rpmlog(RPMLOG_DEBUG, "%5u 0x%08x %8u %12ld %12ld %s %s\n",
		(unsigned)i, (unsigned) dsi->dev, (unsigned) dsi->f_bsize,
		(signed long) dsi->f_bavail, (signed long) dsi->f_favail,
		((dsi->f_flag & ST_RDONLY) ? "ro" : "rw"),
		ts->filesystems[i]);
    }
    return rc;
}

void rpmtsUpdateDSI(const rpmts ts, dev_t dev,
		rpmuint32_t fileSize, rpmuint32_t prevSize, rpmuint32_t fixupSize,
		int _action)
{
    iosmFileAction action = (iosmFileAction) _action;
    rpmDiskSpaceInfo dsi;
    rpmuint64_t bneeded;

    dsi = ts->dsi;
    if (dsi) {
	while (dsi->f_bsize && dsi->dev != dev)
	    dsi++;
	if (dsi->f_bsize == 0)
	    dsi = NULL;
    }
    if (dsi == NULL)
	return;

    bneeded = BLOCK_ROUND(fileSize, dsi->f_bsize);

    switch (action) {
    case FA_BACKUP:
    case FA_SAVE:
    case FA_ALTNAME:
	dsi->ineeded++;
	dsi->bneeded += bneeded;
	/*@switchbreak@*/ break;

    /*
     * FIXME: If two packages share a file (same md5sum), and
     * that file is being replaced on disk, will dsi->bneeded get
     * adjusted twice? Quite probably!
     */
    case FA_CREATE:
	dsi->bneeded += bneeded;
	dsi->bneeded -= BLOCK_ROUND(prevSize, dsi->f_bsize);
	/*@switchbreak@*/ break;

    case FA_ERASE:
	dsi->ineeded--;
	dsi->bneeded -= bneeded;
	/*@switchbreak@*/ break;

    default:
	/*@switchbreak@*/ break;
    }

    if (fixupSize)
	dsi->bneeded -= BLOCK_ROUND(fixupSize, dsi->f_bsize);
}

void rpmtsCheckDSIProblems(const rpmts ts, const rpmte te)
{
    rpmDiskSpaceInfo dsi;
    rpmps ps;
    int fc;
    size_t i;

    if (ts->filesystems == NULL || ts->filesystemCount == 0)
	return;

    dsi = ts->dsi;
    if (dsi == NULL)
	return;
    fc = rpmfiFC( rpmteFI(te, RPMTAG_BASENAMES) );
    if (fc <= 0)
	return;

    ps = rpmtsProblems(ts);
    for (i = 0; i < ts->filesystemCount; i++, dsi++) {

	if (dsi->f_bavail > 0 && adj_fs_blocks(dsi->bneeded) > dsi->f_bavail) {
	    if (dsi->bneeded != dsi->obneeded) {
		rpmpsAppend(ps, RPMPROB_DISKSPACE,
			rpmteNEVR(te), rpmteKey(te),
			ts->filesystems[i], NULL, NULL,
 		  (adj_fs_blocks(dsi->bneeded) - dsi->f_bavail) * dsi->f_bsize);
		dsi->obneeded = dsi->bneeded;
	    }
	}

	if (dsi->f_favail > 0 && adj_fs_blocks(dsi->ineeded) > dsi->f_favail) {
	    if (dsi->ineeded != dsi->oineeded) {
		rpmpsAppend(ps, RPMPROB_DISKNODES,
			rpmteNEVR(te), rpmteKey(te),
			ts->filesystems[i], NULL, NULL,
 			(adj_fs_blocks(dsi->ineeded) - dsi->f_favail));
		dsi->oineeded = dsi->ineeded;
	    }
	}

	if ((dsi->bneeded || dsi->ineeded) && (dsi->f_flag & ST_RDONLY)) {
	    rpmpsAppend(ps, RPMPROB_RDONLY,
			rpmteNEVR(te), rpmteKey(te),
			ts->filesystems[i], NULL, NULL, 0);
	}
    }
    ps = rpmpsFree(ps);
}

void * rpmtsNotify(rpmts ts, rpmte te,
		rpmCallbackType what, rpmuint64_t amount, rpmuint64_t total)
{
    void * ptr = NULL;
    if (ts && ts->notify) {
	Header h;
	fnpyKey cbkey;
	/*@-type@*/ /* FIX: cast? */
	/*@-noeffectuncon @*/ /* FIX: check rc */
	if (te) {
/*@-castexpose -mods@*/	/* XXX noisy in transaction.c */
	    h = headerLink(te->h);
/*@=castexpose =mods@*/
	    cbkey = rpmteKey(te);
	} else {
	    h = NULL;
	    cbkey = NULL;
	}
	ptr = ts->notify(h, what, amount, total, cbkey, ts->notifyData);
	(void)headerFree(h);
	h = NULL;
	/*@=noeffectuncon @*/
	/*@=type@*/
    }
    return ptr;
}

int rpmtsNElements(rpmts ts)
{
    int nelements = 0;
    if (ts != NULL && ts->order != NULL) {
	nelements = ts->orderCount;
    }
    return nelements;
}

rpmte rpmtsElement(rpmts ts, int ix)
{
    rpmte te = NULL;
    if (ts != NULL && ts->order != NULL) {
	if (ix >= 0 && ix < ts->orderCount)
	    te = ts->order[ix];
    }
    /*@-compdef@*/
    return te;
    /*@=compdef@*/
}

rpmprobFilterFlags rpmtsFilterFlags(rpmts ts)
{
    return (rpmprobFilterFlags) (ts != NULL ? ts->ignoreSet : 0);
}

rpmtransFlags rpmtsFlags(rpmts ts)
{
    rpmtransFlags transFlags = (rpmtransFlags)0;
    if (ts != NULL) {
	transFlags = ts->transFlags;
	if (rpmtsSELinuxEnabled(ts) > 0)
	    transFlags = (rpmtransFlags)(transFlags & ~RPMTRANS_FLAG_NOCONTEXTS);
	else
	    transFlags = (rpmtransFlags)(transFlags | RPMTRANS_FLAG_NOCONTEXTS);
    }
    return transFlags;
}

rpmtransFlags rpmtsSetFlags(rpmts ts, rpmtransFlags transFlags)
{
    rpmtransFlags otransFlags = (rpmtransFlags) 0;
    if (ts != NULL) {
	otransFlags = ts->transFlags;
	if (rpmtsSELinuxEnabled(ts) > 0)
	    transFlags = (rpmtransFlags)
		(transFlags & ~RPMTRANS_FLAG_NOCONTEXTS);
	else
	    transFlags = (rpmtransFlags)
		(transFlags | RPMTRANS_FLAG_NOCONTEXTS);
	ts->transFlags = transFlags;
    }
    return otransFlags;
}

rpmdepFlags rpmtsDFlags(rpmts ts)
{
    return (rpmdepFlags) (ts != NULL ? ts->depFlags : 0);
}

rpmdepFlags rpmtsSetDFlags(rpmts ts, rpmdepFlags depFlags)
{
    rpmdepFlags odepFlags = (rpmdepFlags) 0;
    if (ts != NULL) {
	odepFlags = ts->depFlags;
	ts->depFlags = depFlags;
    }
    return odepFlags;
}

Spec rpmtsSpec(rpmts ts)
{
/*@-compdef -retexpose -usereleased@*/
    return ts->spec;
/*@=compdef =retexpose =usereleased@*/
}

Spec rpmtsSetSpec(rpmts ts, Spec spec)
{
    Spec ospec = ts->spec;
/*@-assignexpose -temptrans@*/
    ts->spec = spec;
/*@=assignexpose =temptrans@*/
    return ospec;
}

rpmte rpmtsRelocateElement(rpmts ts)
{
/*@-compdef -retexpose -usereleased@*/
    return ts->relocateElement;
/*@=compdef =retexpose =usereleased@*/
}

rpmte rpmtsSetRelocateElement(rpmts ts, rpmte relocateElement)
{
    rpmte orelocateElement = ts->relocateElement;
/*@-assignexpose -temptrans@*/
    ts->relocateElement = relocateElement;
/*@=assignexpose =temptrans@*/
    return orelocateElement;
}

tsmStage rpmtsGoal(rpmts ts)
{
    return (ts != NULL ? ts->goal : TSM_UNKNOWN);
}

tsmStage rpmtsSetGoal(rpmts ts, tsmStage goal)
{
    tsmStage ogoal = TSM_UNKNOWN;
    if (ts != NULL) {
	ogoal = ts->goal;
	ts->goal = goal;
    }
    return ogoal;
}

int rpmtsDBMode(rpmts ts)
{
    return (ts != NULL ? ts->dbmode : 0);
}

int rpmtsSetDBMode(rpmts ts, int dbmode)
{
    int odbmode = 0;
    if (ts != NULL) {
	odbmode = ts->dbmode;
	ts->dbmode = dbmode;
    }
    return odbmode;
}

rpmuint32_t rpmtsColor(rpmts ts)
{
    return (ts != NULL ? ts->color : 0);
}

rpmuint32_t rpmtsSetColor(rpmts ts, rpmuint32_t color)
{
    rpmuint32_t ocolor = 0;
    if (ts != NULL) {
	ocolor = ts->color;
	ts->color = color;
    }
    return ocolor;
}

rpmuint32_t rpmtsPrefColor(rpmts ts)
{
    return (ts != NULL ? ts->prefcolor : 0);
}

int rpmtsSetNotifyCallback(rpmts ts,
		rpmCallbackFunction notify, rpmCallbackData notifyData)
{
    if (ts != NULL) {
	ts->notify = notify;
	ts->notifyData = notifyData;
    }
    return 0;
}

rpmts rpmtsCreate(void)
{
    rpmts ts = rpmtsGetPool(_rpmtsPool);
    int xx;

    memset(&ts->ops, 0, sizeof(ts->ops));
    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_TOTAL), -1);
    ts->type = RPMTRANS_TYPE_NORMAL;
    ts->goal = TSM_UNKNOWN;
    ts->filesystemCount = 0;
    ts->filesystems = NULL;
    ts->dsi = NULL;

    ts->solve = rpmtsSolve;
    ts->solveData = NULL;
    ts->nsuggests = 0;
    ts->suggests = NULL;

    ts->PRCO = NULL;

    ts->bag = NULL;

    ts->rdb = NULL;
    ts->dbmode = O_RDONLY;
    ts->txn = NULL;

    ts->scriptFd = NULL;
    {	struct timeval tv;
	xx = gettimeofday(&tv, NULL);
	ts->tid[0] = (rpmuint32_t) tv.tv_sec;
	ts->tid[1] = (rpmuint32_t) tv.tv_usec;
    }
    ts->delta = 5;

    ts->color = rpmExpandNumeric("%{?_transaction_color}");
    ts->prefcolor = rpmExpandNumeric("%{?_prefer_color}");
    if (!ts->prefcolor) ts->prefcolor = 0x2;

    ts->rbf = NULL;
    ts->numRemovedPackages = 0;
    ts->allocedRemovedPackages = ts->delta;
    ts->removedPackages = (uint32_t *) xcalloc(ts->allocedRemovedPackages,
			sizeof(*ts->removedPackages));

    ts->rootDir = NULL;
    ts->currDir = NULL;
    ts->chrootDone = 0;

    ts->selinuxEnabled = rpmsxEnabled(NULL);

    ts->numAddedPackages = 0;
    ts->addedPackages = NULL;

    ts->numErasedPackages = 0;
    ts->erasedPackages = NULL;

    ts->numAvailablePackages = 0;
    ts->availablePackages = NULL;

    ts->orderAlloced = 0;
    ts->orderCount = 0;
    ts->order = NULL;
    ts->ntrees = 0;
    ts->maxDepth = 0;

    ts->probs = NULL;

    ts->keyring = NULL;
    ts->hkp = NULL;
    ts->dig = NULL;

    /* Set autorollback goal to the end of time. */
    ts->arbgoal = 0xffffffff;

    return rpmtsLink(ts, "tsCreate");
}
