/** \ingroup rpmts
 * \file lib/transaction.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmmacro.h>	/* XXX for rpmExpand */
#include <rpmsx.h>

#include <rpmtypes.h>
#include <rpmtag.h>
#include <pkgio.h>

#define	_FPRINT_INTERNAL
#include "fprint.h"

#define	_RPMDB_INTERNAL	/* XXX for dbiIndexFoo() */
#include <rpmdb.h>
#include "legacy.h"	/* XXX dodigest */

#define	_RPMFI_INTERNAL
#include <rpmfi.h>
#include "fsm.h"

#define	_RPMTE_INTERNAL
#include "rpmte.h"
#define	_RPMTS_INTERNAL
#include "rpmts.h"

#define	_RPMSQ_INTERNAL
#define	_RPMPSM_INTERNAL
#include "psm.h"

#include "rpmds.h"

#include "rpmlock.h"

#include "misc.h" /* XXX currentDirectory */

#if defined(RPM_VENDOR_MANDRIVA)
#include "filetriggers.h" /* XXX mayAddToFilesAwaitingFiletriggers, rpmRunFileTriggers */
#endif

#include <rpmcli.h>	/* XXX QVA_t INSTALL_FOO flags */
#include <rpmrollback.h>	/* IDTX prototypes */

#include "debug.h"

/*@access dbiIndexSet @*/

/*@access fnpyKey @*/

/*@access alKey @*/
/*@access rpmdb @*/	/* XXX cast */

/*@access rpmfi @*/
/*@access rpmps @*/	/* XXX need rpmProblemSetOK() */
/*@access rpmpsm @*/

/*@access rpmte @*/
/*@access rpmtsi @*/
/*@access rpmts @*/

/*@access IDT @*/
/*@access IDTX @*/
/*@access FD_t @*/

#ifdef __cplusplus

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#define TSF_ISSET(_tsflags, _FLAG)	((_tsflags) & (RPMTRANS_FLAG_##_FLAG))
#define TSF_SET(_tsflags, _FLAG)	\
	(*((unsigned *)&(_tsflags)) |= (RPMTRANS_FLAG_##_FLAG))
#define TSF_CLR(_tsflags, _FLAG)	\
	(*((unsigned *)&(_tsflags)) &= ~(RPMTRANS_FLAG_##_FLAG))

#define IIF_ISSET(_iflags, _FLAG)	((_iflags) & (INSTALL_##_FLAG))
#define IIF_SET(_iflags, _FLAG)	\
	(*((unsigned *)&(_iflags)) |= (INSTALL_##_FLAG))
#define IIF_CLR(_iflags, _FLAG)	\
	(*((unsigned *)&(_iflags)) &= ~(INSTALL_##_FLAG))

GENfree(int *)
GENfree(struct fingerPrint_s *)

#else	/* __cplusplus */

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) &  (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |=  (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#define TSF_ISSET(_tsflags, _FLAG)	((_tsflags) &  (RPMTRANS_FLAG_##_FLAG))
#define TSF_SET(_tsflags, _FLAG)		(_tsflags) |=  (RPMTRANS_FLAG_##_FLAG)
#define TSF_CLR(_tsflags, _FLAG)		(_tsflags) &= ~(RPMTRANS_FLAG_##_FLAG)

#define IIF_ISSET(_iflags, _FLAG)	((_iflags) &  (INSTALL_##_FLAG))
#define IIF_SET(_iflags, _FLAG)		(_iflags) |=  (INSTALL_##_FLAG)
#define IIF_CLR(_iflags, _FLAG)		(_iflags) &= ~(INSTALL_##_FLAG)

#endif	/* __cplusplus */

static int handleInstInstalledFile(const rpmts ts, rpmte p, rpmfi fi,
				   Header otherHeader, rpmfi otherFi,
				   int beingRemoved)
	/*@modifies ts, p, fi @*/
{
    unsigned int fx = rpmfiFX(fi);
    int isCfgFile = (FF_ISSET(rpmfiFFlags(otherFi), CONFIG) || FF_ISSET(rpmfiFFlags(fi), CONFIG));
#ifdef	REFERENCE
    rpmfs fs = rpmteGetFileStates(p);
    if (XFA_SKIPPING(rpmfsGetAction(fs, fx)))
#else
    if (iosmFileActionSkipped((iosmFileAction) fi->actions[fx]))
#endif
	return 0;

    if (rpmfiCompare(otherFi, fi)) {
#ifdef	REFERENCE
	rpm_color_t tscolor = rpmtsColor(ts);
	rpm_color_t prefcolor = rpmtsPrefColor(ts);
	rpm_color_t FColor = rpmfiFColor(fi) & tscolor;
	rpm_color_t oFColor = rpmfiFColor(otherFi) & tscolor;
#else
	rpmuint32_t tscolor = rpmtsColor(ts);
	rpmuint32_t prefcolor = rpmtsPrefColor(ts);
	rpmuint32_t FColor = rpmfiFColor(fi) & tscolor;
	rpmuint32_t oFColor = rpmfiFColor(otherFi) & tscolor;
#endif
	int rConflicts;

	rConflicts = !(beingRemoved || (rpmtsFilterFlags(ts) & RPMPROB_FILTER_REPLACEOLDFILES));
	/* Resolve file conflicts to prefer Elf64 (if not forced). */
	if (tscolor != 0 && FColor != 0 && FColor != oFColor) {
	    if (oFColor & prefcolor) {
#ifdef	REFERENCE
		rpmfsSetAction(fs, fx, FA_SKIPCOLOR);
#else
		fi->actions[fx] = FA_SKIPCOLOR;
#endif
		rConflicts = 0;
	    } else if (FColor & prefcolor) {
#ifdef	REFERENCE
		rpmfsSetAction(fs, fx, FA_CREATE);
#else
		fi->actions[fx] = FA_CREATE;
#endif
		rConflicts = 0;
	    }
	}

	if (rConflicts) {
	    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
	    rpmps ps = rpmtsProblems(ts);
	    int xx;
	    he->tag = RPMTAG_NVRA;
	    xx = headerGet(otherHeader, he, 0);
	    rpmpsAppend(ps, RPMPROB_FILE_CONFLICT,
			rpmteNEVRA(p), rpmteKey(p),
			rpmfiDN(fi), rpmfiBN(fi),
			he->p.str,
			0);
	    he->p.ptr = _free(he->p.ptr);
	    ps = rpmpsFree(ps);
	}

	/* Save file identifier to mark as state REPLACED. */
#ifdef	REFERENCE
	if ( !(isCfgFile || XFA_SKIPPING(rpmfsGetAction(fs, fx))) ) {
	    if (!beingRemoved)
		rpmfsAddReplaced(rpmteGetFileStates(p), rpmfiFX(fi),
				 headerGetInstance(otherHeader),
				 rpmfiFX(otherFi));
	}
#else
	if ( !(isCfgFile || iosmFileActionSkipped((iosmFileAction) fi->actions[fx])) ) {
	    if (!beingRemoved) {
		struct sharedFileInfo_s _shared;

		p->replaced = (sharedFileInfo) xrealloc(p->replaced,
			sizeof(*p->replaced) * (p->nreplaced + 1));
		memset(p->replaced + p->nreplaced, 0, sizeof(*p->replaced));

		_shared.pkgFileNum = fx;
		_shared.otherFileNum = rpmfiFX(otherFi);
		_shared.otherPkg = headerGetInstance(otherHeader);
		_shared.isRemoved = 0;
		p->replaced[p->nreplaced++] = _shared;
	    }
	}
#endif
    }

    /* Determine config file disposition, skipping missing files (if any). */
    if (isCfgFile) {
	rpmtransFlags tsflags = rpmtsFlags(ts);
	int skipMissing = (TSF_ISSET(tsflags, ALLFILES) ? 0 : 1);
#ifdef	REFERENCE
	rpmFileAction action = rpmfiDecideFate(otherFi, fi, skipMissing);
	rpmfsSetAction(fs, fx, action);
#else
	fi->actions[fx] = rpmfiDecideFate(otherFi, fi, skipMissing);
#endif
    }
#ifdef	REFERENCE
    rpmfiSetFReplacedSize(fi, rpmfiFSize(otherFi));
#else
    fi->replacedSizes[fx] = rpmfiFSize(otherFi);
#endif

    return 0;
}

#define	ISROOT(_d)	(((_d)[0] == '/' && (_d)[1] == '\0') ? "" : (_d))

/*@unchecked@*/
int _fps_debug = 0;
#define	FPSDEBUG(_debug, _list)	if ((_debug) || _fps_debug) fprintf _list

/**
 * Update disk space needs on each partition for this package's files.
 */
/* XXX only ts->{probs,di} modified */
static void handleOverlappedFiles(const rpmts ts, const rpmte p, rpmfi fi)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies ts, fi, fileSystem, internalState @*/
{
    uint32_t fixupSize = 0;
    rpmps ps;
    const char * fn;
    int i, j;

    uint32_t tscolor = rpmtsColor(ts);
    uint32_t prefcolor = rpmtsPrefColor(ts);
#ifdef	REFERENCE
    rpmfs fs = rpmteGetFileStates(p);
    rpmfs otherFs;
#endif	/* REFERENCE */

FPSDEBUG(0, (stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, ts, p, fi));
    ps = rpmtsProblems(ts);
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while ((i = rpmfiNext(fi)) >= 0) {
	uint32_t oFColor;
	uint32_t FColor;
	struct fingerPrint_s * fiFps;
	int otherPkgNum, otherFileNum;
	rpmfi otherFi;

	rpmte otherTe;
#ifdef	REFERENCE
	rpmfileAttrs FFlags;
	rpm_mode_t FMode;
#else	/* REFERENCE */
	rpmuint32_t FFlags;
	rpmuint16_t FMode;
#endif	/* REFERENCE */
	struct rpmffi_s ** recs;
	int numRecs;

	if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
	    continue;

	fn = rpmfiFN(fi);
#ifdef	REFERENCE
	fiFps = rpmfiFpsIndex(fi, i);
#else	/* REFERENCE */
	fiFps = fi->fps + i;
#endif	/* REFERENCE */
	FFlags = rpmfiFFlags(fi);
	FMode = rpmfiFMode(fi);
	FColor = rpmfiFColor(fi);
	FColor &= tscolor;

	fixupSize = 0;

	/*
	 * Retrieve all records that apply to this file. Note that the
	 * file info records were built in the same order as the packages
	 * will be installed and removed so the records for an overlapped
	 * files will be sorted in exactly the same order.
	 */
	recs = NULL;
	numRecs = 0;
#ifdef	REFERENCE
	(void) rpmFpHashGetEntry(ht, fiFps, &recs, &numRecs, NULL);
#else	/* REFERENCE */
	(void) htGetEntry(ts->ht, fiFps, &recs, &numRecs, NULL);
#endif	/* REFERENCE */

	/*
	 * If this package is being added, look only at other packages
	 * being added -- removed packages dance to a different tune.
	 *
	 * If both this and the other package are being added, overlapped
	 * files must be identical (or marked as a conflict). The
	 * disposition of already installed config files leads to
	 * a small amount of extra complexity.
	 *
	 * If this package is being removed, then there are two cases that
	 * need to be worried about:
	 * If the other package is being added, then skip any overlapped files
	 * so that this package removal doesn't nuke the overlapped files
	 * that were just installed.
	 * If both this and the other package are being removed, then each
	 * file removal from preceding packages needs to be skipped so that
	 * the file removal occurs only on the last occurrence of an overlapped
	 * file in the transaction set.
	 */

	/* Locate this overlapped file in the set of added/removed packages. */
	for (j = 0; j < numRecs && recs[j]->p != p; j++) {
FPSDEBUG(0, (stderr, "\trecs %p[%u:%u] te %p != %p\n", recs, (unsigned)j, (unsigned)numRecs, recs[j]->p, p));
	}
FPSDEBUG(0, (stderr, "*** got recs %p[%u:%u]\n", recs, (unsigned)j, (unsigned)numRecs));

	/* Find what the previous disposition of this file was. */
	otherFileNum = -1;			/* keep gcc quiet */
	otherFi = NULL;
	otherTe = NULL;
#ifdef	REFERENCE
	otherFs = NULL;
#endif	/* REFERENCE */

	for (otherPkgNum = j - 1; otherPkgNum >= 0; otherPkgNum--) {
FPSDEBUG(0, (stderr, "\trecs %p[%u:%u] %p -> {%p,%d}\n", recs, (unsigned)otherPkgNum, (unsigned)numRecs, recs[otherPkgNum], recs[otherPkgNum]->p, recs[otherPkgNum]->fileno));
	    otherTe = recs[otherPkgNum]->p;
	    otherFi = rpmteFI(otherTe, RPMTAG_BASENAMES);
	    otherFileNum = recs[otherPkgNum]->fileno;
#ifdef	REFERENCE
	    otherFs = rpmteGetFileStates(otherTe);
#endif	/* REFERENCE */

	    /* Added packages need only look at other added packages. */
	    if (rpmteType(p) == TR_ADDED && rpmteType(otherTe) != TR_ADDED)
		/*@innercontinue@*/ continue;

	    (void) rpmfiSetFX(otherFi, otherFileNum);

	    /* XXX Happens iff fingerprint for incomplete package install. */
	    if (otherFi->actions[otherFileNum] != FA_UNKNOWN)
		/*@innerbreak@*/ break;
	}

	oFColor = rpmfiFColor(otherFi);
	oFColor &= tscolor;

	switch (rpmteType(p)) {
	case TR_ADDED:
	  { int reportConflicts =
		!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_REPLACENEWFILES);
	    int done = 0;

	    if (otherPkgNum < 0) {
		iosmFileAction action;
		/* XXX is this test still necessary? */
		if (fi->actions[i] != FA_UNKNOWN)
		    /*@switchbreak@*/ break;
#ifdef	REFERENCE
		if (rpmfiConfigConflict(fi))
#else
		if (FF_ISSET(FFlags, CONFIG) && FF_ISSET(FFlags, EXISTS))
#endif
		{
		    /* Here is a non-overlapped pre-existing config file. */
		    action = FF_ISSET(FFlags, NOREPLACE)
			? FA_ALTNAME : FA_BACKUP;
		} else {
		    action = FA_CREATE;
		}
#ifdef	REFERENCE
		rpmfsSetAction(fs, i, action);
#else
		fi->actions[i] = action;
#endif
		/*@switchbreak@*/ break;
	    }

assert(otherFi != NULL);
	    /* Mark added overlapped non-identical files as a conflict. */
	    if (rpmfiCompare(otherFi, fi)) {
		int rConflicts;

		rConflicts = reportConflicts;
		/* Resolve file conflicts to prefer Elf64 (if not forced) ... */
		if (tscolor != 0) {
		    if (FColor & prefcolor) {
			/* ... last file of preferred colour is installed ... */
			if (!iosmFileActionSkipped((iosmFileAction) fi->actions[i])) {
#ifdef	DEAD
			    /* XXX static helpers are order dependent. Ick. */
			    if (strcmp(fn, "/usr/sbin/libgcc_post_upgrade")
			     && strcmp(fn, "/usr/sbin/glibc_post_upgrade"))
#endif
				otherFi->actions[otherFileNum] = FA_SKIPCOLOR;
			}
			fi->actions[i] = FA_CREATE;
			rConflicts = 0;
		    } else
		    if (oFColor & prefcolor) {
			/* ... first file of preferred colour is installed ... */
			if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
			    otherFi->actions[otherFileNum] = FA_CREATE;
			fi->actions[i] = FA_SKIPCOLOR;
			rConflicts = 0;
		    } else
		    if (FColor == 0 && oFColor == 0) {
			/* ... otherwise, do both, last in wins. */
			otherFi->actions[otherFileNum] = FA_CREATE;
			fi->actions[i] = FA_CREATE;
			rConflicts = 0;
		    }
		    done = 1;
		}

		if (rConflicts) {
		    rpmpsAppend(ps, RPMPROB_NEW_FILE_CONFLICT,
			rpmteNEVR(p), rpmteKey(p),
			fn, NULL,
			rpmteNEVR((rpmte) otherFi->te),
			0);
		}
	    }

	    /* Try to get the disk accounting correct even if a conflict. */
	    fixupSize = rpmfiFSize(otherFi);

#ifdef	REFERENCE
	    if (rpmfiConfigConflict(fi))
#else	/* REFERENCE */
	    if (FF_ISSET(FFlags, CONFIG) && FF_ISSET(FFlags, EXISTS))
#endif	/* REFERENCE */
	    {
		/* Here is an overlapped  pre-existing config file. */
		fi->actions[i] = FF_ISSET(FFlags, NOREPLACE)
			? FA_ALTNAME : FA_SKIP;
	    } else {
		if (!done)
		    fi->actions[i] = FA_CREATE;
	    }
	  } /*@switchbreak@*/ break;

	case TR_REMOVED:
	    if (otherPkgNum >= 0) {
assert(otherFi != NULL);
		/* Here is an overlapped added file we don't want to nuke. */
		if (otherFi->actions[otherFileNum] != FA_ERASE) {
		    /* On updates, don't remove files. */
		    fi->actions[i] = FA_SKIP;
		    /*@switchbreak@*/ break;
		}
		/* Here is an overlapped removed file: skip in previous. */
		otherFi->actions[otherFileNum] = FA_SKIP;
	    }
	    if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
		/*@switchbreak@*/ break;
	    if (rpmfiFState(fi) != RPMFILE_STATE_NORMAL)
		/*@switchbreak@*/ break;

	    /* Disposition is assumed to be FA_ERASE. */
	    fi->actions[i] = FA_ERASE;
	    if (!(S_ISREG(FMode) && FF_ISSET(FFlags, CONFIG)))
		/*@switchbreak@*/ break;

	    /* Check for pre-existing modified config file that needs saving. */
	    if (!FF_ISSET(FFlags,SPARSE))
	    {	int dalgo = 0;
		size_t dlen = 0;
		const unsigned char * digest = rpmfiDigest(fi, &dalgo, &dlen);
		unsigned char * fdigest;
assert(digest != NULL);
		
		fdigest = (unsigned char *) xcalloc(1, dlen);
		/* Save (by renaming) locally modified config files. */
		if (!dodigest(dalgo, fn, fdigest, 0, NULL)
		 && memcmp(digest, fdigest, dlen))
		    fi->actions[i] = FA_BACKUP;
		fdigest = _free(fdigest);
	    }
	    /*@switchbreak@*/ break;
	}

	/* Update disk space info for a file. */
#ifdef	REFERENCE
	rpmtsUpdateDSI(ts, fiFps->entry->dev, rpmfiFSize(fi),
		       rpmfiFReplacedSize(fi), fixupSize, rpmfsGetAction(fs, i));
#else
	rpmtsUpdateDSI(ts, fiFps->entry->dev, rpmfiFSize(fi),
		fi->replacedSizes[i], fixupSize, fi->actions[i]);
#endif

    }
    ps = rpmpsFree(ps);
}

/**
 * Ensure that current package is newer than installed package.
 * @param ts		transaction set
 * @param p		current transaction element
 * @param h		installed header
 * @return		0 if not newer, 1 if okay
 */
/*@-nullpass@*/
static int ensureOlder(rpmts ts,
		const rpmte p, const Header h)
	/*@globals internalState @*/
	/*@modifies ts, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    evrFlags reqFlags = (evrFlags) (RPMSENSE_LESS | RPMSENSE_EQUAL);
    const char * reqEVR;
    rpmds req;
    char * t;
    size_t nb;
    int rc;

FPSDEBUG(0, (stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, ts, p, h));
    if (p == NULL || h == NULL)
	return 1;

    nb = strlen(rpmteNEVR(p)) + (rpmteE(p) != NULL ? strlen(rpmteE(p)) : 0) + 1;
#ifdef	RPM_VENDOR_MANDRIVA
    nb += (rpmteD(p) != NULL ? strlen(rpmteD(p)) + 1 : 0);
#endif
    t = (char *) alloca(nb);
    *t = '\0';
    reqEVR = t;
    if (rpmteE(p) != NULL)	t = stpcpy( stpcpy(t, rpmteE(p)), ":");
    if (rpmteV(p) != NULL)	t = stpcpy(t, rpmteV(p));
    *t++ = '-';
    if (rpmteR(p) != NULL)	t = stpcpy(t, rpmteR(p));
#ifdef RPM_VENDOR_MANDRIVA
    if (rpmteD(p) != NULL)	*t++ = ':', t = stpcpy(t, rpmteD(p));
#endif

    req = rpmdsSingle(RPMTAG_REQUIRENAME, rpmteN(p), reqEVR, reqFlags);
    rc = rpmdsNVRMatchesDep(h, req, _rpmds_nopromote);
    (void)rpmdsFree(req);
    req = NULL;

    if (rc == 0) {
	rpmps ps = rpmtsProblems(ts);
	he->tag = RPMTAG_NVRA;
	rc = headerGet(h, he, 0);
assert(he->p.str != NULL);
	rpmpsAppend(ps, RPMPROB_OLDPACKAGE,
		rpmteNEVR(p), rpmteKey(p),
		NULL, NULL,
		he->p.str,
		0);
	he->p.ptr = _free(he->p.ptr);
	ps = rpmpsFree(ps);
	rc = 1;
    } else
	rc = 0;

    return rc;
}
/*@=nullpass@*/

/**
 * Skip any files that do not match install policies.
 * @param ts		transaction set
 * @param fi		file info set
 */
/*@-mustmod@*/ /* FIX: fi->actions is modified. */
/*@-nullpass@*/
static void rpmtsSkipFiles(const rpmts ts, rpmfi fi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies fi, rpmGlobalMacroContext, internalState @*/
{
    rpmuint32_t tscolor = rpmtsColor(ts);
    rpmuint32_t FColor;
    rpmtransFlags tsflags = rpmtsFlags(ts);
    int noConfigs = TSF_ISSET(tsflags, NOCONFIGS);
    int noDocs = TSF_ISSET(tsflags, NODOCS);
    ARGV_t netsharedPaths = NULL;
    ARGV_t languages = NULL;
    const char * dn, * bn;
    size_t dnlen, bnlen;
    int ix;
    const char * s;
    int * drc;
    char * dff;
    int dc;
    int i, j;
    int xx;

FPSDEBUG(0, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, ts, fi));
#if defined(RPM_VENDOR_OPENPKG) /* allow-excludedocs-default */
    /* The "%_excludedocs" macro is intended to set the _default_ if
       both --excludedocs and --includedocs are not specified and it
       is evaluated already before. So, do not override it here again,
       because it would not allow us to make "%_excludedocs 1" the
       default. */
#else
    if (!noDocs)
	noDocs = rpmExpandNumeric("%{_excludedocs}");
#endif

    {	const char *tmpPath = rpmExpand("%{?_netsharedpath}", NULL);
	if (tmpPath && *tmpPath)
	    xx = argvSplit(&netsharedPaths, tmpPath, ":");
	tmpPath = _free(tmpPath);
    }

    s = rpmExpand("%{?_install_langs}", NULL);
    if (!(s && *s))
	s = _free(s);
    if (s) {
	xx = argvSplit(&languages, s, ":");
	s = _free(s);
    }

    /* Compute directory refcount, skip directory if now empty. */
    dc = rpmfiDC(fi);
    drc = (int *) alloca(dc * sizeof(*drc));
    memset(drc, 0, dc * sizeof(*drc));
    dff = (char *) alloca(dc * sizeof(*dff));
    memset(dff, 0, dc * sizeof(*dff));

    fi = rpmfiInit(fi, 0);
    if (fi != NULL)	/* XXX lclint */
    while ((i = rpmfiNext(fi)) >= 0)
    {
	ARGV_t nsp;

	bn = rpmfiBN(fi);
	bnlen = strlen(bn);
	ix = rpmfiDX(fi);
	dn = rpmfiDN(fi);
	if (dn == NULL)
	    continue;	/* XXX can't happen */
	dnlen = strlen(dn);

	drc[ix]++;

	/* Don't bother with skipped files */
	if (iosmFileActionSkipped((iosmFileAction) fi->actions[i])) {
	    drc[ix]--; dff[ix] = 1;
	    continue;
	}

	/* Ignore colored files not in our rainbow. */
	FColor = rpmfiFColor(fi);
	if (tscolor && FColor && !(tscolor & FColor)) {
	    drc[ix]--;	dff[ix] = 1;
	    fi->actions[i] = FA_SKIPCOLOR;
	    continue;
	}

	/*
	 * Skip net shared paths.
	 * Net shared paths are not relative to the current root (though
	 * they do need to take package relocations into account).
	 */
	for (nsp = netsharedPaths; nsp && *nsp; nsp++) {
	    size_t len;

	    len = strlen(*nsp);
	    if (dnlen >= len) {
		if (strncmp(dn, *nsp, len))
		    /*@innercontinue@*/ continue;
		/* Only directories or complete file paths can be net shared */
		if (!(dn[len] == '/' || dn[len] == '\0'))
		    /*@innercontinue@*/ continue;
	    } else {
		if (len < (dnlen + bnlen))
		    /*@innercontinue@*/ continue;
		if (strncmp(dn, *nsp, dnlen))
		    /*@innercontinue@*/ continue;
		/* Insure that only the netsharedpath basename is compared. */
		if ((s = strchr((*nsp) + dnlen, '/')) != NULL && s[1] != '\0')
		    /*@innercontinue@*/ continue;
		if (strncmp(bn, (*nsp) + dnlen, bnlen))
		    /*@innercontinue@*/ continue;
		len = dnlen + bnlen;
		/* Only directories or complete file paths can be net shared */
		if (!((*nsp)[len] == '/' || (*nsp)[len] == '\0'))
		    /*@innercontinue@*/ continue;
	    }

	    /*@innerbreak@*/ break;
	}

	if (nsp && *nsp) {
	    drc[ix]--;	dff[ix] = 1;
	    fi->actions[i] = FA_SKIPNETSHARED;
	    continue;
	}

	/*
	 * Skip i18n language specific files.
	 */
	if (languages != NULL && fi->flangs != NULL && *fi->flangs[i]) {
	    ARGV_t lang;
	    const char *l, *le;
	    for (lang = languages; *lang != NULL; lang++) {
		if (!strcmp(*lang, "all"))
		    /*@innerbreak@*/ break;
		for (l = fi->flangs[i]; *l != '\0'; l = le) {
		    for (le = l; *le != '\0' && *le != '|'; le++)
			{};
		    if ((le-l) > 0 && !strncmp(*lang, l, (le-l)))
			/*@innerbreak@*/ break;
		    if (*le == '|') le++;	/* skip over | */
		}
		if (*l != '\0')
		    /*@innerbreak@*/ break;
	    }
	    if (*lang == NULL) {
		drc[ix]--;	dff[ix] = 1;
		fi->actions[i] = FA_SKIPNSTATE;
		continue;
	    }
	}

	/*
	 * Skip config files if requested.
	 */
	if (noConfigs && FF_ISSET(rpmfiFFlags(fi), CONFIG)) {
	    drc[ix]--;	dff[ix] = 1;
	    fi->actions[i] = FA_SKIPNSTATE;
	    continue;
	}

	/*
	 * Skip documentation if requested.
	 */
	if (noDocs && FF_ISSET(rpmfiFFlags(fi), DOC)) {
	    drc[ix]--;	dff[ix] = 1;
	    fi->actions[i] = FA_SKIPNSTATE;
	    continue;
	}
    }

    /* Skip (now empty) directories that had skipped files. */
#ifndef	NOTYET
    if (fi != NULL)	/* XXX can't happen */
    for (j = 0; j < dc; j++)
#else
    if ((fi = rpmfiInitD(fi)) != NULL)
    while (j = rpmfiNextD(fi) >= 0)
#endif
    {

	if (drc[j]) continue;	/* dir still has files. */
	if (!dff[j]) continue;	/* dir was not emptied here. */
	
	/* Find parent directory and basename. */
	dn = fi->dnl[j];	dnlen = strlen(dn) - 1;
	bn = dn + dnlen;	bnlen = 0;
	while (bn > dn && bn[-1] != '/') {
		bnlen++;
		dnlen--;
		bn--;
	}

	/* If explicitly included in the package, skip the directory. */
	fi = rpmfiInit(fi, 0);
	if (fi != NULL)		/* XXX lclint */
	while ((i = rpmfiNext(fi)) >= 0) {
	    const char * fdn, * fbn;
	    rpmuint16_t fFMode;

	    if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
		/*@innercontinue@*/ continue;

	    fFMode = rpmfiFMode(fi);

	    if (!S_ISDIR(fFMode))
		/*@innercontinue@*/ continue;
	    fdn = rpmfiDN(fi);
	    if (strlen(fdn) != dnlen)
		/*@innercontinue@*/ continue;
	    if (strncmp(fdn, dn, dnlen))
		/*@innercontinue@*/ continue;
	    fbn = rpmfiBN(fi);
	    if (strlen(fbn) != bnlen)
		/*@innercontinue@*/ continue;
	    if (strncmp(fbn, bn, bnlen))
		/*@innercontinue@*/ continue;
	    rpmlog(RPMLOG_DEBUG, D_("excluding directory %s\n"), dn);
	    fi->actions[i] = FA_SKIPNSTATE;
	    /*@innerbreak@*/ break;
	}
    }

/*@-dependenttrans@*/
    netsharedPaths = argvFree(netsharedPaths);
    languages = argvFree(languages);
/*@=dependenttrans@*/
}
/*@=nullpass@*/
/*@=mustmod@*/

/**
 * Return transaction element's file info.
 * @todo Take a rpmfi refcount here.
 * @param tsi		transaction element iterator
 * @return		transaction element file info
 */
static /*@null@*/
rpmfi rpmtsiFi(const rpmtsi tsi)
	/*@*/
{
    rpmfi fi = NULL;

    if (tsi != NULL && tsi->ocsave != -1) {
	/*@-type -abstract@*/ /* FIX: rpmte not opaque */
	rpmte te = rpmtsElement(tsi->ts, tsi->ocsave);
	/*@-assignexpose@*/
	if (te != NULL && (fi = te->fi) != NULL)
	    fi->te = te;
	/*@=assignexpose@*/
	/*@=type =abstract@*/
    }
    /*@-compdef -refcounttrans -usereleased @*/
    return fi;
    /*@=compdef =refcounttrans =usereleased @*/
}

/**
 * Search for string B in argv array AV.
 * @param AV		argv array
 * @param B		string
 * @return		1 if found, 0 otherwise
 */
static int cmpArgvStr(/*@null@*/ const char ** AV, /*@null@*/ const char * B)
	/*@*/
{
    const char ** a;

    if (AV != NULL && B != NULL)
    for (a = AV; *a != NULL; a++) {
	if (**a && *B && !strcmp(*a, B))
	    return 1;
    }
    return 0;
}

/**
 * Mark all erasure elements linked to installed element p as failed.
 * @param ts		transaction set
 * @param p		failed install transaction element
 * @return		0 always
 */
static int rpmtsMarkLinkedFailed(rpmts ts, rpmte p)
	/*@globals fileSystem @*/
	/*@modifies ts, p, fileSystem @*/
{
    rpmtsi qi; rpmte q;
    int bingo;

    p->linkFailed = 1;

    qi = rpmtsiInit(ts);
    while ((q = rpmtsiNext(qi, TR_REMOVED)) != NULL) {

	if (q->done)
	    continue;

	/*
	 * Either element may have missing data and can have multiple entries.
	 * Try for hdrid, then pkgid, finally NEVRA, argv vs. argv compares.
	 */
	bingo = cmpArgvStr(q->flink.Hdrid, p->hdrid);
	if (!bingo)
		bingo = cmpArgvStr(q->flink.Pkgid, p->pkgid);
	if (!bingo)
		bingo = cmpArgvStr(q->flink.NEVRA, p->NEVRA);

	if (!bingo)
	    continue;

	q->linkFailed = p->linkFailed;
    }
    qi = rpmtsiFree(qi);

    return 0;
}

/* ================================================================= */

/* Get a rpmdbMatchIterator containing all files in
 * the rpmdb that share the basename with one from
 * the transaction.
 * @param ts		transaction set
 * @param fileCount	no. of files
 * @return		rpmmi sorted by (package, fileNum)
 */
static
rpmmi rpmtsFindBaseNamesInDB(rpmts ts, uint32_t fileCount)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtsi pi;  rpmte p;
    rpmfi fi;
    rpmmi mi;
    int i, xx;
    const char * s;
    size_t ns;
    void * ptr;
    static rpmTag _tag = RPMTAG_BASENAMES;
    size_t n = (fileCount > 10 ? fileCount : 10);
    static double e = 1.0e-4;
    size_t m = 0;
    size_t k = 0;
    rpmbf bf;

FPSDEBUG(0, (stderr, "--> %s(%p,%u)\n", __FUNCTION__, ts, (unsigned)fileCount));
    rpmbfParams(n, e, &m, &k);
    bf = rpmbfNew(m, k, 0);

    mi = rpmmiInit(rpmtsGetRdb(ts), _tag, NULL, 0);

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {

	(void) rpmdbCheckSignals();

	if ((fi = rpmteFI(p, _tag)) == NULL)
	    continue;   /* XXX can't happen */

	ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_TRANS_PROGRESS, rpmtsiOc(pi),
		    ts->orderCount);

	/* Gather all installed headers with matching basename's. */
	fi = rpmfiInit(fi, 0);
	while ((i = rpmfiNext(fi)) >= 0) {
	    s = rpmfiBN(fi);
	    ns = strlen(s);

	    if (ns == 0)	/* XXX "/" fixup */
		/*@innercontinue@*/ continue;
	    if (rpmbfChk(bf, s, ns))
		/*@innercontinue@*/ continue;

	    xx = rpmmiGrowBasename(mi, s);

	    xx = rpmbfAdd(bf, s, ns);
	 }
    }
    pi = rpmtsiFree(pi);
    bf = rpmbfFree(bf);

    (void) rpmmiSort(mi);

    return mi;
}

/* Check files in the transactions against the rpmdb
 * Lookup all files with the same basename in the rpmdb
 * and then check for matching finger prints
 * @param ts		transaction set
 * @param fpc		global finger print cache
 */
static
int rpmtsCheckInstalledFiles(rpmts ts, uint32_t fileCount,
		hashTable ht, fingerPrintCache fpc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, fpc, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTagData BN = { NULL };
    rpmTagData DN = { NULL };
    rpmTagData DI = { NULL };
    rpmTagData FSTATES = { NULL };
    rpmuint32_t fc;

    rpmte p;
    rpmmi mi;
    Header h;
    rpmfi fi;

    const char * oldDir;
    int beingRemoved;
    rpmfi otherFi = NULL;
    unsigned int fileNum;
    int xx;
    int rc = 0;

FPSDEBUG(0, (stderr, "--> %s(%p,%u,%p,%p)\n", __FUNCTION__, ts, (unsigned)fileCount, ht, fpc));

rpmlog(RPMLOG_DEBUG, D_("computing file dispositions\n"));

    /* XXX fileCount == 0 installing src.rpm's */
    if (fileCount == 0)
	return rc;

    mi = rpmtsFindBaseNamesInDB(ts, fileCount);

    /* Loop over all packages from the rpmdb */
    while ((h = rpmmiNext(mi)) != NULL) {
	fingerPrint fp;
	uint32_t hdrNum = rpmmiInstance(mi);
	uint32_t tagNum = rpmmiBNTag(mi);
	int i;
	int j;

	/* Is this package being removed? */
	beingRemoved = 0;
	if (ts->removedPackages != NULL)
	for (j = 0; j < ts->numRemovedPackages; j++) {
	    if (ts->removedPackages[j] != hdrNum)
	        /*@innercontinue@*/ continue;
	    beingRemoved = 1;
	    /*@innerbreak@*/ break;
	}

	he->tag = RPMTAG_BASENAMES;
	xx = headerGet(h, he, 0);
	BN.argv = (xx ? he->p.argv : NULL);
	fc = (xx ? he->c : 0);

	he->tag = RPMTAG_DIRNAMES;
	xx = headerGet(h, he, 0);
	DN.argv = (xx ? he->p.argv : NULL);
	he->tag = RPMTAG_DIRINDEXES;
	xx = headerGet(h, he, 0);
	DI.ui32p = (xx ? he->p.ui32p : NULL);
	he->tag = RPMTAG_FILESTATES;
	xx = headerGet(h, he, 0);
	FSTATES.ui8p = (xx ? he->p.ui8p : NULL);

	/* loop over all interesting files in that package */
	oldDir = NULL;
	for (i = 0; i < (int)fc; i++) {
	    const char * baseName = BN.argv[i];
	    rpmuint32_t baseKey = hashFunctionString(0, baseName, 0);
	    int gotRecs;
	    struct rpmffi_s ** recs;
	    int numRecs;
	    const char * dirName;

	    /* Skip uninteresting basenames. */
	    if (baseKey != tagNum)
		/*@innercontinue@*/ continue;
	    fileNum = i;
	    dirName = DN.argv[DI.ui32p[fileNum]];

	    /* lookup finger print for this file */
	    if (dirName == oldDir) {
	        /* directory is the same as last round */
	        fp.baseName = baseName;
	    } else {
	        fp = fpLookup(fpc, dirName, baseName, 1);
		oldDir = dirName;
	    }

	    /* search for files in the transaction with same finger print */
	    recs = NULL;
	    numRecs = 0;
#ifdef	REFERENCE
	    gotRecs = rpmFpHashGetEntry(ht, &fp, &recs, &numRecs, NULL);
#else	/* REFERENCE */
	    gotRecs = (htGetEntry(ts->ht, &fp, &recs, &numRecs, NULL) == 0);
#endif	/* REFERENCE */

	    for (j = 0; j < numRecs && gotRecs; j++) {
	        p = recs[j]->p;
		fi = rpmteFI(p, RPMTAG_BASENAMES);

		/* Determine the fate of each file. */
		switch (rpmteType(p)) {
		case TR_ADDED:
		    if (otherFi == NULL) {
			static int scareMem = 0;
		        otherFi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
		    }
		    (void) rpmfiSetFX(fi, recs[j]->fileno);
		    (void) rpmfiSetFX(otherFi, fileNum);
		    xx = handleInstInstalledFile(ts, p, fi, h, otherFi, beingRemoved);
		    /*@switchbreak@*/ break;
		case TR_REMOVED:
		    if (!beingRemoved) {
		        (void) rpmfiSetFX(fi, recs[j]->fileno);
#ifdef	REFERENCE
			if (*rpmtdGetChar(&ostates) == RPMFILE_STATE_NORMAL) {
			    rpmfs fs = rpmteGetFileStates(p);
			    rpmfsSetAction(fs, recs[j].fileno, FA_SKIP);
			}
#else
			if (FSTATES.ui8p[fileNum] == RPMFILE_STATE_NORMAL)
			    fi->actions[recs[j]->fileno] = FA_SKIP;
#endif
		    }
		    /*@switchbreak@*/ break;
		}
	    }

	}

	otherFi = rpmfiFree(otherFi);
	FSTATES.ptr = _free(FSTATES.ptr);
	DI.ptr = _free(DI.ptr);
	DN.ptr = _free(DN.ptr);
	BN.ptr = _free(BN.ptr);
    }

    mi = rpmmiFree(mi);

    return rc;
}

/*
 * For packages being installed:
 * - verify package arch/os.
 * - verify package epoch:version-release is newer.
 */
static rpmps rpmtsSanityCheck(rpmts ts, uint32_t * tfcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, *tfcp, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmps ps;
    rpmtsi pi;
    rpmte p;
    rpmfi fi;
    uint32_t totalFileCount = 0;
    int fc;

FPSDEBUG(0, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, ts, tfcp));
rpmlog(RPMLOG_DEBUG, D_("sanity checking %d elements\n"), rpmtsNElements(ts));
    ps = rpmtsProblems(ts);
    /* The ordering doesn't matter here */
    pi = rpmtsiInit(ts);
    /* XXX Only added packages need be checked. */
    while ((p = rpmtsiNext(pi, TR_ADDED)) != NULL) {
	int xx;

	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	fc = rpmfiFC(fi);

#ifdef	REFERENCE
	if (!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_IGNOREARCH))
	    if (!archOkay(rpmteA(p)))
		rpmpsAppend(ps, RPMPROB_BADARCH,
			rpmteNEVRA(p), rpmteKey(p),
			rpmteA(p), NULL,
			NULL, 0);

	if (!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_IGNOREOS))
	    if (!osOkay(rpmteO(p)))
		rpmpsAppend(ps, RPMPROB_BADOS,
			rpmteNEVRA(p), rpmteKey(p),
			rpmteO(p), NULL,
			NULL, 0);
#endif	/* REFERENCE */

	if (!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_OLDPACKAGE)) {
	    rpmmi mi = rpmtsInitIterator(ts, RPMTAG_NAME, rpmteN(p), 0);
	    Header h;
	    while ((h = rpmmiNext(mi)) != NULL)
		xx = ensureOlder(ts, p, h);
	    mi = rpmmiFree(mi);
	}

	if (!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_REPLACEPKG)) {
	    ARGV_t keys = NULL;
	    int nkeys;
	    xx = rpmdbMireApply(rpmtsGetRdb(ts), RPMTAG_NVRA,
		RPMMIRE_STRCMP, rpmteNEVRA(p), &keys);
	    nkeys = argvCount(keys);

	    /* mdvbz: #63711
	     * workaround for epoch & distepoch not being part of RPMTAG_NVRA,
	     * leading to packages of same VRA but with different epoch or distepoch
	     * being treated as the same package */
	    if (nkeys > 0) {
		int i, t;
		rpmTag tags[2] = { RPMTAG_EPOCH, RPMTAG_DISTEPOCH };
		for (t = 0; t < 2; t++) {
		    for (i = 0; i < nkeys; i++) {
			rpmmi mi = rpmtsInitIterator(ts, RPMTAG_NVRA, keys[i], 0);
			Header h;
			while ((h = rpmmiNext(mi)) != NULL) {
			    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
			    const char *val = NULL;
			    he->tag = tags[t];
			    xx = headerGet(h, he, 0);
			    if (he->tag == RPMTAG_EPOCH)
				val = rpmteE(p);
			    else if (he->tag == RPMTAG_DISTEPOCH)
				val = rpmteD(p);
			    if (strcmp(he->p.str ? he->p.str : "", val ? val : ""))
				nkeys--;
			    he->p.ptr = _free(he->p.ptr);
			}
			mi = rpmmiFree(mi);
		    }
		}
	    }
	    if (nkeys > 0)
		rpmpsAppend(ps, RPMPROB_PKG_INSTALLED,
			rpmteNEVR(p), rpmteKey(p),
			NULL, NULL,
			NULL, 0);
	    keys = argvFree(keys);
	}

#ifdef	REFERENCE
	/* XXX rpmte problems can only be relocation problems atm */
	if (!(rpmtsFilterFlags(ts) & RPMPROB_FILTER_FORCERELOCATE)) {
	    rpmpsi psi = rpmpsInitIterator(rpmteProblems(p));
	    while (rpmpsNextIterator(psi) >= 0) {
		rpmpsAppendProblem(ps, rpmpsGetProblem(psi));
	    }
	    rpmpsFreeIterator(psi);
	}
#endif	/* REFERENCE */

	/* Count no. of files (if any). */
	totalFileCount += fc;

    }
    pi = rpmtsiFree(pi);

    /* The ordering doesn't matter here */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_REMOVED)) != NULL) {

	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	fc = rpmfiFC(fi);

	totalFileCount += fc;
    }
    pi = rpmtsiFree(pi);

    if (tfcp)
	*tfcp = totalFileCount;

    return ps;
}

/*
 * Run pre/post transaction script.
 * param ts	transaction set
 * param stag	RPMTAG_PRETRANS or RPMTAG_POSTTRANS
 * return	0 on success
 */
static int rpmtsRunScript(rpmts ts, rpmTag stag) 
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtsi pi; 
    rpmte p;
    rpmfi fi;
    rpmpsm psm;
    int xx;
    rpmTag ptag;

FPSDEBUG(0, (stderr, "--> %s(%p,%s(%u))\n", __FUNCTION__, ts, tagName(stag), (unsigned)stag));
    switch (stag) {
    default:
assert(0);
	/*@notreached@*/ break;
    case RPMTAG_PRETRANS:	ptag = RPMTAG_PRETRANSPROG;	break;
    case RPMTAG_POSTTRANS:	ptag = RPMTAG_POSTTRANSPROG;	break;
    }

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_ADDED)) != NULL) {
	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */

	/* If no prre/post transaction script, then don't bother. */
	if (!rpmteHaveTransScript(p, stag))
	    continue;

    	if (rpmteOpen(p, ts, 0)) {
	    if (p->fi != NULL)	/* XXX can't happen */
		p->fi->te = p;
#ifdef	REFERENCE
	    psm = rpmpsmNew(ts, p);
#else	/* REFERENCE */
	    psm = rpmpsmNew(ts, p, p->fi);
#endif	/* REFERENCE */
	    xx = rpmpsmScriptStage(psm, stag, ptag);
	    psm = rpmpsmFree(psm, __FUNCTION__);
	    xx = rpmteClose(p, ts, 0);
	}
    }
    pi = rpmtsiFree(pi);

    return 0;
}

/* Add fingerprint for each file not skipped. */
static void rpmtsAddFingerprints(rpmts ts, uint32_t fileCount, hashTable ht,
		fingerPrintCache fpc)
	/*@modifies ts, fpc @*/
{
    rpmtsi pi;
    rpmte p;
    rpmfi fi;
    int i;

    hashTable symlinks = htCreate(fileCount/16+16, 0, 0, fpHashFunction, fpEqual);

FPSDEBUG(0, (stderr, "--> %s(%p,%u,%p,%p)\n", __FUNCTION__, ts, (unsigned)fileCount, ht, fpc));
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	(void) rpmdbCheckSignals();

	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */

	(void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), 0);

	rpmfiFpLookup(fi, fpc);

	/* Collect symlinks. */
 	fi = rpmfiInit(fi, 0);
 	if (fi != NULL)		/* XXX lclint */
	while ((i = rpmfiNext(fi)) >= 0) {
	    char const *linktarget;
	    linktarget = rpmfiFLink(fi);
	    if (!(linktarget && *linktarget != '\0'))
		/*@innercontinue@*/ continue;
	    if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
		/*@innercontinue@*/ continue;
#ifdef	REFERENCE
	    {	struct rpmffi_s ffi;
		ffi.p = p;
		ffi.fileno = i;
		htAddEntry(symlinks, rpmfiFpsIndex(fi, i), ffi);
	    }
#else
	    {	struct rpmffi_s *ffip = (struct rpmffi_s *) alloca(sizeof(*ffip));
/*@-dependenttrans@*/
		ffip->p = p;
/*@=dependenttrans@*/
		ffip->fileno = i;
		htAddEntry(symlinks, fi->fps + i, (void *) ffip);
	    }
#endif
	}

	(void) rpmswExit(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), rpmfiFC(fi));

    }
    pi = rpmtsiFree(pi);

    /* ===============================================
     * Check fingerprints if they contain symlinks
     * and add them to the hash table
     */

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	(void) rpmdbCheckSignals();

	if (p->isSource) continue;
	if ((fi = rpmteFI(p, RPMTAG_BASENAMES)) == NULL)
	    continue;	/* XXX can't happen */
	fi = rpmfiInit(fi, 0);
	(void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), 0);
	while ((i = rpmfiNext(fi)) >= 0) {
#ifdef	REFERENCE
	    if (XFA_SKIPPING(rpmfsGetAction(rpmteGetFileStates(p), i)))
		continue;
#else
	    if (iosmFileActionSkipped((iosmFileAction) fi->actions[i]))
		/*@innercontinue@*/ continue;
#endif
	    fpLookupSubdir(symlinks, ht, fpc, p, i);
	}
	(void) rpmswExit(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), 0);
    }
    pi = rpmtsiFree(pi);

    symlinks = htFree(symlinks);

}

static int rpmtsSetup(rpmts ts, rpmprobFilterFlags ignoreSet, rpmsx * sxp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, *sxp, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtransFlags tsflags = rpmtsFlags(ts);
    int xx;

/*@+voidabstract@*/
FPSDEBUG(0, (stderr, "--> %s(%p,0x%x,%p)\n", __FUNCTION__, ts, ignoreSet, (void *)sxp));
/*@=voidabstract@*/
    /* --noscripts implies no scripts or triggers, duh. */
    if (TSF_ISSET(tsflags, NOSCRIPTS)) {
	TSF_SET(tsflags, NOPRETRANS);
	TSF_SET(tsflags, NOPRE);
	TSF_SET(tsflags, NOPOST);
	TSF_SET(tsflags, NOPREUN);
	TSF_SET(tsflags, NOPOSTUN);
	TSF_SET(tsflags, NOPOSTTRANS);

	TSF_SET(tsflags, NOTRIGGERPREIN);
	TSF_SET(tsflags, NOTRIGGERIN);
	TSF_SET(tsflags, NOTRIGGERUN);
	TSF_SET(tsflags, NOTRIGGERPOSTUN);

	(void) rpmtsSetFlags(ts, tsflags);
    }
    /* --notriggers implies no triggers, duh. */
    if (TSF_ISSET(tsflags, NOTRIGGERS)) {
	TSF_SET(tsflags, NOTRIGGERPREIN);
	TSF_SET(tsflags, NOTRIGGERIN);
	TSF_SET(tsflags, NOTRIGGERUN);
	TSF_SET(tsflags, NOTRIGGERPOSTUN);
	(void) rpmtsSetFlags(ts, tsflags);
    }

    /* --justdb implies no scripts or triggers, duh. */
    if (TSF_ISSET(tsflags, JUSTDB)) {
	TSF_SET(tsflags, NOPRETRANS);
	TSF_SET(tsflags, NOPRE);
	TSF_SET(tsflags, NOPOST);
	TSF_SET(tsflags, NOPREUN);
	TSF_SET(tsflags, NOPOSTUN);
	TSF_SET(tsflags, NOPOSTTRANS);

	TSF_SET(tsflags, NOTRIGGERPREIN);
	TSF_SET(tsflags, NOTRIGGERIN);
	TSF_SET(tsflags, NOTRIGGERUN);
	TSF_SET(tsflags, NOTRIGGERPOSTUN);

	(void) rpmtsSetFlags(ts, tsflags);
    }

    /* if SELinux isn't enabled or init fails, don't bother... */
    if (!rpmtsSELinuxEnabled(ts)) {
	TSF_SET(tsflags, NOCONTEXTS);
	TSF_SET(tsflags, NOPOLICY);
	(void) rpmtsSetFlags(ts, tsflags);
    }

    if (!(TSF_ISSET(tsflags, NOCONTEXTS) || TSF_ISSET(tsflags, NOPOLICY))) {
	*sxp = rpmsxNew("%{?_install_file_context_path}", 0);
        if (*sxp == NULL) {
	    TSF_SET(tsflags, NOCONTEXTS);
	    TSF_SET(tsflags, NOPOLICY);
	    (void) rpmtsSetFlags(ts, tsflags);
	}
    } else
	*sxp = NULL;

    /* XXX Make sure the database is open RDWR for package install/erase. */
    {	int dbmode = O_RDONLY;

	if (!TSF_ISSET(tsflags, TEST)) {
	    rpmtsi pi;
	    rpmte p;
	    pi = rpmtsiInit(ts);
	    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
		if (p->isSource) continue;
		dbmode = (O_RDWR|O_CREAT);
		break;
	    }
	    pi = rpmtsiFree(pi);
	}

	/* Open database RDWR for installing packages. */
	if (rpmtsOpenDB(ts, dbmode))
	    return -1;	/* XXX W2DO? */

    }

    ts->ignoreSet = ignoreSet;
    ts->probs = rpmpsFree(ts->probs);

    {	const char * currDir = currentDirectory();
	rpmtsSetCurrDir(ts, currDir);
	currDir = _free(currDir);
    }

    (void) rpmtsSetChrootDone(ts, 0);

    /* XXX rpmtsCreate() sets the transaction id, but apps may not honor. */
    {	rpmuint32_t tid = (rpmuint32_t) time(NULL);
	(void) rpmtsSetTid(ts, tid);
    }

    /* Get available space on mounted file systems. */
    xx = rpmtsInitDSI(ts);

    return 0;
}

static int rpmtsFinish(rpmts ts, /*@only@*/ rpmsx sx)
	/*@modifies sx @*/
{
FPSDEBUG(0, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, ts, sx));
#ifdef	REFERENCE
    rpmtransFlags tsflags = rpmtsFlags(ts);
    if (!TSF_ISSET(tsflags, NOCONTEXTS))
	matchpathcon_fini();
#else	/* REFERENCE */
    if (sx != NULL) sx = rpmsxFree(sx);
#endif	/* REFERENCE */
    return 0;
}

static int rpmtsPrepare(rpmts ts, rpmsx sx, uint32_t fileCount,
		uint32_t * nrmvdp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, *nrmvdp, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtsi pi;
    rpmte p;
    fingerPrintCache fpc;
    rpmfi fi;
    int xx;
    int rc = 0;

#ifdef	REFERENCE
    uint64_t fileCount = countFiles(ts);
    const char * rootDir = rpmtsRootDir(ts);
    int dochroot = (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/');

    rpmFpHash ht = rpmFpHashCreate(fileCount/2+1, fpHashFunction, fpEqual,
			     NULL, NULL);

    fpc = fpCacheCreate(fileCount/2 + 10001);

    rpmlog(RPMLOG_DEBUG, "computing %" PRIu64 " file fingerprints\n", fileCount);

    /* Skip netshared paths, not our i18n files, and excluded docs */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, TR_ADDED)) != NULL) {
	if ((fi = rpmteFI(p)) == NULL)
	    continue;	/* XXX can't happen */

	if (rpmfiFC(fi) > 0) 
	    rpmtsSkipFiles(ts, p);
    }
    pi = rpmtsiFree(pi);

    /* Enter chroot for fingerprinting if necessary */
    if (!rpmtsChrootDone(ts)) {
	xx = chdir("/");
	if (dochroot) {
	    /* opening db before chroot not optimal, see rhbz#103852 c#3 */
	    xx = rpmdbOpenAll(ts->rdb);
	    if (chroot(rootDir) == -1) {
		rpmlog(RPMLOG_ERR, _("Unable to change root directory: %m\n"));
		rc = -1;
		goto exit;
	    }
	}
	(void) rpmtsSetChrootDone(ts, 1);
    }
    
#else	/* REFERENCE */

    void * ptr;
    uint32_t numAdded = 0;
    uint32_t numRemoved = 0;

FPSDEBUG(0, (stderr, "--> %s(%p,%p,%u,%p)\n", __FUNCTION__, ts, sx, (unsigned)fileCount, nrmvdp));
rpmlog(RPMLOG_DEBUG, D_("computing %u file fingerprints\n"), (unsigned)fileCount);

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	int fc;

	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	fc = rpmfiFC(fi);

	switch (rpmteType(p)) {
	case TR_ADDED:
	    numAdded++;
	    fi->record = 0;
	    /* Skip netshared paths, not our i18n files, and excluded docs */
	    if (fc > 0)
		rpmtsSkipFiles(ts, fi);
	    /*@switchbreak@*/ break;
	case TR_REMOVED:
	    numRemoved++;
	    fi->record = rpmteDBOffset(p);
	    /*@switchbreak@*/ break;
	}

	fi->fps = (struct fingerPrint_s *)
		(fc > 0 ? xmalloc(fc * sizeof(*fi->fps)) : NULL);
    }
    pi = rpmtsiFree(pi);

    if (nrmvdp)
	*nrmvdp = numRemoved;

    if (!rpmtsChrootDone(ts)) {
	const char * rootDir = rpmtsRootDir(ts);
	static int openall_before_chroot = -1;

	if (openall_before_chroot < 0)
	    openall_before_chroot = rpmExpandNumeric("%{?_openall_before_chroot}");

	xx = Chdir("/");
	/*@-modobserver@*/
	if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/') {
	    if (openall_before_chroot)
		xx = rpmdbOpenAll(rpmtsGetRdb(ts));
	    xx = Chroot(rootDir);
	}
	/*@=modobserver@*/
	(void) rpmtsSetChrootDone(ts, 1);
    }

    ts->ht = htCreate(fileCount/2 + 1, 0, 1, fpHashFunction, fpEqual);
    fpc = fpCacheCreate(fileCount/2 + 10001);

#endif	/* REFERENCE */

    ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_TRANS_START, 6, ts->orderCount);

    /* ===============================================
     * Add fingerprint for each file not skipped.
     */
#ifdef	REFERENCE
    rpmtsAddFingerprints(ts, fileCount, ht, fpc);
#else	/* REFERENCE */
    rpmtsAddFingerprints(ts, fileCount, ts->ht, fpc);
#endif	/* REFERENCE */

    /* ===============================================
     * Compute file disposition for each package in transaction set.
     */
#ifdef	REFERENCE
    rpmtsCheckInstalledFiles(ts, fileCount, ht, fpc);
#else	/* REFERENCE */
    rc = rpmtsCheckInstalledFiles(ts, fileCount, ts->ht, fpc);
    if (rc)
	goto exit;
#endif	/* REFERENCE */

    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	if ((fi = rpmteFI(p, RPMTAG_BASENAMES)) == NULL)
	    continue;   /* XXX can't happen */
	/* XXX Set all SRPM files to FA_CREATE. */
	if (p->isSource) {
	    int i;
	    fi = rpmfiInit(fi, 0);
	    if (fi != NULL)
	    while ((i = rpmfiNext(fi)) >= 0)
		fi->actions[i] = FA_CREATE;
	    continue;
	}

	(void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), 0);

	/* Update disk space needs on each partition for this package. */
/*@-nullpass@*/
#ifdef	REFERENCE
	handleOverlappedFiles(ts, ht, p, fi);
#else	/* REFERENCE */
	handleOverlappedFiles(ts, p, fi);
#endif	/* REFERENCE */
/*@=nullpass@*/

	/* Check added package has sufficient space on each partition used. */
	switch (rpmteType(p)) {
	case TR_ADDED:
	    rpmtsCheckDSIProblems(ts, p);
	    /*@switchbreak@*/ break;
	case TR_REMOVED:
	    /*@switchbreak@*/ break;
	}
	(void) rpmswExit(rpmtsOp(ts, RPMTS_OP_FINGERPRINT), rpmfiFC(fi));
    }
    pi = rpmtsiFree(pi);

    if (rpmtsChrootDone(ts)) {
	const char * rootDir = rpmtsRootDir(ts);
	const char * currDir = rpmtsCurrDir(ts);
	/*@-modobserver@*/
	if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/')
	    xx = Chroot(".");
	/*@=modobserver@*/
	(void) rpmtsSetChrootDone(ts, 0);
	if (currDir != NULL)
	    xx = Chdir(currDir);
    }

    ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_TRANS_STOP, 6, ts->orderCount);

    /* ===============================================
     * Free unused memory as soon as possible.
     */
#ifdef	REFERENCE
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	rpmteSetFI(p, NULL);
    }
    pi = rpmtsiFree(pi);
#else	/* REFERENCE */
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	if (rpmfiFC(fi) == 0)
	    continue;
	fi->fps = _free(fi->fps);
    }
    pi = rpmtsiFree(pi);
#endif	/* REFERENCE */

exit:
#ifdef	REFERENCE
    ht = rpmFpHashFree(ht);
#else	/* REFERENCE */
    ts->ht = htFree(ts->ht);
#endif	/* REFERENCE */
    fpc = fpCacheFree(fpc);

    return rc;
}

/*
 * Transaction main loop: install and remove packages
 */
static int rpmtsProcess(rpmts ts, rpmprobFilterFlags ignoreSet,
		int rollbackFailures)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtsi pi;
    rpmte p;
    int rc = 0;

FPSDEBUG(0, (stderr, "--> %s(%p,0x%x,%d)\n", __FUNCTION__, ts, ignoreSet, rollbackFailures));
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {
	rpmfi fi;
	rpmop sw;
	rpmpsm psm = NULL;
	pkgStage stage = PSM_UNKNOWN;
	int failed;
	int gotfd;
	int xx;

#ifdef	REFERENCE
	rpmElementType tetype = rpmteType(p);
	rpmtsOpX op = (tetype == TR_ADDED) ? RPMTS_OP_INSTALL : RPMTS_OP_ERASE;
#endif	/* REFERENCE */

	(void) rpmdbCheckSignals();

	failed = 1;
	gotfd = 0;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	
	if (rpmteFailed(p)) {
	    /* XXX this should be a warning, need a better message though */
	    rpmlog(RPMLOG_DEBUG, D_("element %s marked as failed, skipping\n"),
					rpmteNEVRA(p));
	    rc++;
	    continue;
	}

	psm = rpmpsmNew(ts, p, fi);
	{   int async = (rpmtsiOc(pi) >= rpmtsUnorderedSuccessors(ts, -1)) ? 
			1 : 0;
	    rpmpsmSetAsync(psm, async);
	}

	switch (rpmteType(p)) {
	case TR_ADDED:
	    rpmlog(RPMLOG_DEBUG, "========== +++ %s %s-%s 0x%x\n",
		rpmteNEVR(p), rpmteA(p), rpmteO(p), rpmteColor(p));
	    stage = PSM_PKGINSTALL;
	    sw = rpmtsOp(ts, RPMTS_OP_INSTALL);
#ifdef	REFERENCE
	    if (rpmteOpen(p, ts, 1)) {
		(void) rpmswEnter(rpmtsOp(ts, op), 0);
		failed = rpmpsmStage(psm, stage);
		(void) rpmswExit(rpmtsOp(ts, op), 0);
		rpmteClose(p, ts, 1);
	    }
#else	/* REFERENCE */
	    if ((p->h = rpmteFDHeader(ts, p)) != NULL)
		gotfd = 1;

	    if (gotfd && rpmteFd(p) != NULL) {
		/*
		 * XXX Sludge necessary to transfer existing fstates/actions
		 * XXX around a recreated file info set.
		 */
		rpmuint8_t * fstates = fi->fstates;
		iosmFileAction * actions = (iosmFileAction *) fi->actions;
		int mapflags = fi->mapflags;
		rpmte savep;
		int scareMem = 0;

		psm->fi = rpmfiFree(psm->fi);

		fi->fstates = NULL;
		fi->actions = NULL;
/*@-nullstate@*/ /* FIX: fi->actions is NULL */
		fi = rpmfiFree(fi);
/*@=nullstate@*/

		savep = rpmtsSetRelocateElement(ts, p);
		fi = rpmfiNew(ts, p->h, RPMTAG_BASENAMES, scareMem);
		(void) rpmtsSetRelocateElement(ts, savep);

		if (fi != NULL) {	/* XXX can't happen */
		    fi->te = p;
		    fi->fstates = _free(fi->fstates);
		    fi->fstates = fstates;
		    fi->actions = _free(fi->actions);
		    fi->actions = (int *) actions;
		    if (mapflags & IOSM_SBIT_CHECK)
			fi->mapflags |= IOSM_SBIT_CHECK;
		    p->fi = fi;
		}

		psm->fi = rpmfiLink(p->fi, __FUNCTION__);

		(void) rpmswEnter(sw, 0);
		failed = (rpmpsmStage(psm, stage) != RPMRC_OK);
		(void) rpmswExit(sw, 0);

		xx = rpmteClose(p, ts, 0);
		gotfd = 0;
	    }

#endif	/* REFERENCE */
	    /*@switchbreak@*/ break;

	case TR_REMOVED:
	    rpmlog(RPMLOG_DEBUG, "========== --- %s %s-%s 0x%x\n",
		rpmteNEVR(p), rpmteA(p), rpmteO(p), rpmteColor(p));
	    stage = PSM_PKGERASE;
	    sw = rpmtsOp(ts, RPMTS_OP_ERASE);
#ifdef	REFERENCE
	    if (rpmteOpen(p, ts, 1)) {
		(void) rpmswEnter(rpmtsOp(ts, op), 0);
		failed = rpmpsmStage(psm, stage);
		(void) rpmswExit(rpmtsOp(ts, op), 0);
		rpmteClose(p, ts, 1);
	    }
#else	/* REFERENCE */
	    (void) rpmswEnter(sw, 0);
	    failed = (rpmpsmStage(psm, stage) != RPMRC_OK);
	    (void) rpmswExit(sw, 0);
#endif	/* REFERENCE */
	    /*@switchbreak@*/ break;
	}

#if defined(RPM_VENDOR_MANDRIVA)
	{   rpmtransFlags tsflags = rpmtsFlags(ts);
	    if (!failed && !TSF_ISSET(tsflags, TEST)) {
		if(!rpmteIsSource(p))
		    xx = mayAddToFilesAwaitingFiletriggers(rpmtsRootDir(ts),
				fi, (rpmteType(p) == TR_ADDED ? 1 : 0));
		p->done = 1;
	    }
	}
#endif

/*@-nullstate@*/ /* FIX: psm->fi may be NULL */
	psm = rpmpsmFree(psm, __FUNCTION__);
/*@=nullstate@*/

	if (failed) {
	    rc++;
#ifdef	REFERENCE
	    rpmteMarkFailed(p, ts);
#else	/* REFERENCE */
	    xx = rpmtsMarkLinkedFailed(ts, p);
	/* If we received an error, lets break out and rollback, provided
	 * autorollback is enabled.
	 */
	    if (rollbackFailures) {
		xx = rpmtsRollback(ts, ignoreSet, 1, p);
		break;
	    }
#endif	/* REFERENCE */
	}

	if (p->h != NULL) {
	    (void) headerFree(p->h);
	    p->h = NULL;
	}

#ifdef	REFERENCE
	(void) rpmdbSync(rpmtsGetRdb(ts));
#endif	/* REFERENCE */

    }
    pi = rpmtsiFree(pi);
    return rc;
}

/* ================================================================= */
static int rpmtsRepackage(rpmts ts, uint32_t numRemoved)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    rpmtransFlags tsflags = rpmtsFlags(ts);
    rpmpsm psm;
    rpmfi fi;
    rpmtsi pi;
    rpmte p;
    void * ptr;
    int progress = 0;
    int rc = 0;
    int xx;

FPSDEBUG(0, (stderr, "--> %s(%p,%u)\n", __FUNCTION__, ts, (unsigned)numRemoved));
    pi = rpmtsiInit(ts);
    while ((p = rpmtsiNext(pi, (rpmElementType) 0)) != NULL) {

	(void) rpmdbCheckSignals();

	if (p->isSource) continue;
	if ((fi = rpmtsiFi(pi)) == NULL)
	    continue;	/* XXX can't happen */
	switch (rpmteType(p)) {
	case TR_ADDED:
	    /*@switchbreak@*/ break;
	case TR_REMOVED:
	    if (!TSF_ISSET(tsflags, REPACKAGE))
		/*@switchbreak@*/ break;
	    if (!progress)
		ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_REPACKAGE_START,
				7, numRemoved);

	    ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_REPACKAGE_PROGRESS,
				progress, numRemoved);
	    progress++;

	    (void) rpmswEnter(rpmtsOp(ts, RPMTS_OP_REPACKAGE), 0);

	/* XXX TR_REMOVED needs IOSM_MAP_{ABSOLUTE,ADDDOT} IOSM_ALL_HARDLINKS */
	    fi->mapflags |= IOSM_MAP_ABSOLUTE;
	    fi->mapflags |= IOSM_MAP_ADDDOT;
	    fi->mapflags |= IOSM_ALL_HARDLINKS;
	    psm = rpmpsmNew(ts, p, fi);
assert(psm != NULL);
	    xx = rpmpsmStage(psm, PSM_PKGSAVE);
	    psm = rpmpsmFree(psm, __FUNCTION__);
	    fi->mapflags &= ~IOSM_MAP_ABSOLUTE;
	    fi->mapflags &= ~IOSM_MAP_ADDDOT;
	    fi->mapflags &= ~IOSM_ALL_HARDLINKS;

	    (void) rpmswExit(rpmtsOp(ts, RPMTS_OP_REPACKAGE), 0);

	    /*@switchbreak@*/ break;
	}
    }
    pi = rpmtsiFree(pi);
    if (progress)
	ptr = rpmtsNotify(ts, NULL, RPMCALLBACK_REPACKAGE_STOP, 7, numRemoved);

    return rc;
}

/**
 * Force add a failed package into the rpmdb.
 * @param ts		current transaction set
 * @param p 		failed rpmte. 
 * @return 		RPMRC_OK, or RPMRC_FAIL
 */
/*@-nullpass@*/
static rpmRC _processFailedPackage(rpmts ts, rpmte p)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, p, rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
{
    int rc  = RPMRC_OK;	/* assume success */

FPSDEBUG(0, (stderr, "--> %s(%p,%p)\n", __FUNCTION__, ts, p));
    /* Handle failed packages. */
    /* XXX TODO: Add header to rpmdb in PSM_INIT, not PSM_POST. */
    if (p != NULL && rpmteType(p) == TR_ADDED && !p->installed) {
/*@-compdef -usereleased@*/	/* p->fi->te undefined */
	rpmpsm psm = rpmpsmNew(ts, p, p->fi);
/*@=compdef =usereleased@*/
	/*
	 * If it died before the header was put in the rpmdb, we need
	 * do to something wacky which is add the header to the DB anyway.
	 * This will allow us to add the failed package as an erase
	 * to the rollback transaction.  This must be done because we
	 * want the the erase scriptlets to run, and the only way that
	 * is going is if the header is in the rpmdb.
	 */
assert(psm != NULL);
	psm->stepName = "failed";	/* XXX W2DO? */
	rc = rpmpsmStage(psm, PSM_RPMDB_ADD);
	psm = rpmpsmFree(psm, __FUNCTION__);
    }
    return (rpmRC) rc;
}
/*@=nullpass@*/

/*@-nullpass@*/
rpmRC rpmtsRollback(rpmts rbts, rpmprobFilterFlags ignoreSet, int running, rpmte rbte)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rbts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * semfn = NULL;
    rpmRC rc = (rpmRC) 0;
    rpmuint32_t arbgoal = rpmtsARBGoal(rbts);
    QVA_t ia = (QVA_t) memset(alloca(sizeof(*ia)), 0, sizeof(*ia));
    time_t ttid;
    int xx;

FPSDEBUG(0, (stderr, "--> %s(%p,0x%x,%d,%p)\n", __FUNCTION__, rbts, ignoreSet, running, rbte));
    /* Don't attempt rollback's of rollback transactions */
    if ((rpmtsType(rbts) & RPMTRANS_TYPE_ROLLBACK) ||
	(rpmtsType(rbts) & RPMTRANS_TYPE_AUTOROLLBACK))
	return RPMRC_OK;

    if (arbgoal == 0xffffffff) 
	arbgoal = rpmtsGetTid(rbts);

    /* Don't attempt rollbacks if no goal is set. */
    if (!running && arbgoal == 0xffffffff)
	return RPMRC_OK;

    /* We need to remove an headers that were going to be removed so 
     * as to not foul up the regular rollback mechanism which will not 
     * handle properly a file being in the repackaged package directory
     * and also its header still in the DB.
     */
    {	rpmtsi tsi;
	rpmte te;

	/* XXX Insure an O_RDWR rpmdb. */
	xx = rpmtsOpenDB(rbts, O_RDWR);

	tsi = rpmtsiInit(rbts);
	while((te = rpmtsiNext(tsi, TR_REMOVED)) != NULL) {
	    if (te->isSource) continue;
	    if(!te->u.removed.dboffset)
		continue;
	    rc = (rpmRC) rpmdbRemove(rpmtsGetRdb(rbts),
			rpmtsGetTid(rbts),
			te->u.removed.dboffset, NULL);
	    if (rc != RPMRC_OK) {
		rpmlog(RPMLOG_ERR, _("rpmdb erase failed. NEVRA: %s\n"),
			rpmteNEVRA(te));
		break;
	    }
	}
	tsi = rpmtsiFree(tsi);
	if (rc != RPMRC_OK) 
	    goto cleanup;
    }

    /* Process the failed package */
    rc = _processFailedPackage(rbts, rbte);
    if (rc != RPMRC_OK)
	goto cleanup;

    rpmtsEmpty(rbts);

    ttid = (time_t)arbgoal;
    rpmlog(RPMLOG_NOTICE, _("Rollback to %-24.24s (0x%08x)\n"),
	ctime(&ttid), arbgoal);

    /* Set the verify signature flags:
     *  - can't verify signatures/digests on repackaged packages.
     *  - header check are out.
     */
    {
	rpmVSFlags vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_erase}");
	vsflags = (rpmVSFlags) 0; /* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
	/* --nodigest */
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
	/* --nosignature */
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
	/* --nohdrchk */
	VSF_SET(vsflags, NOHDRCHK);
	VSF_SET(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif
	xx = rpmtsSetVSFlags(rbts, vsflags); 
    }

    /* Set transaction flags to be the same as the running transaction */
    {
	rpmtransFlags tsflags = rpmtsFlags(rbts);
	TSF_CLR(tsflags, DIRSTASH);	/* No repackage of rollbacks */
	TSF_CLR(tsflags, REPACKAGE);	/* No repackage of rollbacks */
	TSF_SET(tsflags, NOFDIGESTS);	/* Don't check file digests */
	tsflags = rpmtsSetFlags(rbts, tsflags);
    }

    /* Create install arguments structure */ 	
    ia->rbtid = arbgoal;
    /* transFlags/depFlags from rbts, (re-)set in rpmRollback(). */
    ia->transFlags = rpmtsFlags(rbts);
    ia->depFlags = rpmtsDFlags(rbts);
    /* XXX probFilter is normally set in main(). */
    ia->probFilter = ignoreSet;	/* XXX RPMPROB_FILTER_NONE? */
    /* XXX installInterfaceFlags is normally set in main(). */
    IIF_SET(ia->installInterfaceFlags, UPGRADE);
    IIF_SET(ia->installInterfaceFlags, HASH);

    /* rpmtsCheck and rpmtsOrder failures do not have links. */
    ia->no_rollback_links = 1;

    /* Create a file semaphore. */
    semfn = rpmExpand("%{?semaphore_backout}", NULL);
    if (semfn && *semfn) {
	FD_t fd = Fopen(semfn, "w.fdio");
	if (fd)
	    xx = Fclose(fd);
    }

/*@-compmempass@*/
    rc = (rpmRC) rpmRollback(rbts, ia, NULL);
/*@=compmempass@*/

cleanup: 
    /* Remove the file semaphore. */
    if (semfn && *semfn)
	xx = Unlink(semfn);
    semfn = _free(semfn);

    return rc;
}
/*@=nullpass@*/

int _rpmtsRun(rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
{
    rpmtransFlags tsflags = rpmtsFlags(ts);
    int ourrc = -1;	/* assume failure */
    uint32_t totalFileCount = 0;
    rpmps ps;
    rpmsx sx = NULL;
    uint32_t numRemoved;
    int rollbackFailures = 0;
    void * lock = NULL;
    int xx;

FPSDEBUG(0, (stderr, "--> %s(%p,%p,0x%x)\n", __FUNCTION__, ts, okProbs, ignoreSet));
if (_rpmts_debug)
fprintf(stderr, "--> %s(%p,%p,0x%x) tsflags 0x%x\n", __FUNCTION__, ts, okProbs, (unsigned) ignoreSet, tsflags);

    /* XXX programmer error segfault avoidance. */
    if (rpmtsNElements(ts) <= 0) {
	rpmlog(RPMLOG_ERR,
	    _("Invalid number of transaction elements.\n"));
	return -1;
    }

    /* Don't acquire the transaction lock if testing. */
    if (!TSF_ISSET(tsflags, TEST))
	lock = rpmtsAcquireLock(ts);

    rollbackFailures = rpmExpandNumeric("%{?_rollback_transaction_on_failure}");
    /* Don't rollback unless repackaging. */
    if (!TSF_ISSET(tsflags, REPACKAGE))
	rollbackFailures = 0;
    /* Don't rollback if testing. */
    if (TSF_ISSET(tsflags, TEST))
	rollbackFailures = 0;

    if (rpmtsType(ts) & (RPMTRANS_TYPE_ROLLBACK | RPMTRANS_TYPE_AUTOROLLBACK))
	rollbackFailures = 0;

    /* ===============================================
     * Setup flags and such, open the rpmdb in O_RDWR mode.
     */
    sx = NULL;
    if (rpmtsSetup(ts, ignoreSet, &sx))
        goto exit;

    /* ===============================================
     * For packages being installed:
     * - verify package epoch:version-release is newer.
     * - count files.
     * For packages being removed:
     * - count files.
     */

    totalFileCount = 0;
    ps = rpmtsSanityCheck(ts, &totalFileCount);
    ps = rpmpsFree(ps);

    /* ===============================================
     * Run pre-transaction scripts, but only if no known problems exist.
     */
    if (!TSF_ISSET(tsflags, NOPRETRANS) &&
       (!(TSF_ISSET(tsflags, BUILD_PROBS) || TSF_ISSET(tsflags, TEST))
     	  || (rpmpsNumProblems(ts->probs) &&
		(okProbs == NULL || rpmpsTrim(ts->probs, okProbs)))))
    {
	rpmlog(RPMLOG_DEBUG, D_("running pre-transaction scripts\n"));
	xx = rpmtsRunScript(ts, RPMTAG_PRETRANS);
    }

    /* ===============================================
     * Compute file disposition for each package in transaction set.
     */
    numRemoved = 0;
    if (rpmtsPrepare(ts, sx, totalFileCount, &numRemoved))
	goto exit;

    /* ===============================================
     * If unfiltered problems exist, free memory and return.
     */
    if (TSF_ISSET(tsflags, BUILD_PROBS)
     || (rpmpsNumProblems(ts->probs) &&
		(okProbs == NULL || rpmpsTrim(ts->probs, okProbs)))
       )
    {
	lock = rpmtsFreeLock(lock);
	if (sx != NULL) sx = rpmsxFree(sx);
	return ts->orderCount;
    }

    /* ===============================================
     * Save removed files before erasing (w/o --justdb).
     */
    if ((TSF_ISSET(tsflags, DIRSTASH) || TSF_ISSET(tsflags, REPACKAGE))
     && !TSF_ISSET(tsflags, JUSTDB))
    {
	xx = rpmtsRepackage(ts, numRemoved);
    }

#ifdef	NOTYET
    xx = rpmtxnBegin(rpmtsGetRdb(ts), NULL, &ts->txn);
#endif

    /* ===============================================
     * Install and remove packages.
     */
    ourrc = rpmtsProcess(ts, ignoreSet, rollbackFailures);

    /* ===============================================
     * Run post-transaction scripts unless disabled.
     */
    if (!TSF_ISSET(tsflags, NOPOSTTRANS) && !TSF_ISSET(tsflags, TEST))
    {

#if defined(RPM_VENDOR_MANDRIVA)
	/* XXX FIXME: simplify the logic. */
	if (!TSF_ISSET(tsflags, NOTRIGGERPREIN)
	 || !TSF_ISSET(tsflags, NOTRIGGERIN)
	 || !TSF_ISSET(tsflags, NOTRIGGERUN)
	 || !TSF_ISSET(tsflags, NOTRIGGERPOSTUN))
	    rpmRunFileTriggers(rpmtsRootDir(ts));
#endif

	rpmlog(RPMLOG_DEBUG, D_("running post-transaction scripts\n"));
	xx = rpmtsRunScript(ts, RPMTAG_POSTTRANS);
    }

exit:
    xx = rpmtsFinish(ts, sx);

    lock = rpmtsFreeLock(lock);

    /*@-nullstate@*/ /* FIX: ts->flList may be NULL */
    if (ourrc) {
	if (ts->txn != NULL)
	    xx = rpmtxnAbort(ts->txn);
	ts->txn = NULL;
    	return -1;
    } else {
	if (ts->txn != NULL)
	    xx = rpmtxnCommit(ts->txn);
	ts->txn = NULL;
	xx = rpmtxnCheckpoint(rpmtsGetRdb(ts));
	return 0;
    }
    /*@=nullstate@*/
}

int (*rpmtsRun) (rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
	= _rpmtsRun;
