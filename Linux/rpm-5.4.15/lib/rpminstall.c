/** \ingroup rpmcli
 * \file lib/rpminstall.c
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <poptIO.h>

#include <rpmtag.h>
#define	_RPMEVR_INTERNAL	/* XXX expose rpmVersionCompare prototype */
#include <rpmevr.h>
#include "rpmdb.h"
#ifdef	NOTYET
#include "rpmds.h"		/* XXX ts->suggests, +foo -foo =foo args */
#endif

#include "rpmte.h"		/* XXX rpmtsPrint() */
#define	_RPMTS_INTERNAL		/* XXX ts->suggests */
#include <rpmts.h>

#include "manifest.h"
#define	_RPMGI_INTERNAL		/* XXX "+bing" args need gi->h. */
#include "rpmgi.h"

#include <rpmlib.h>

#include <rpmcli.h>
#define	_RPMROLLBACK_INTERNAL
#include <rpmrollback.h>

#include "debug.h"

/*@access FD_t @*/	/* XXX void * arg */
/*@access rpmts @*/	/* XXX ts->suggests */
/*@access rpmgi @*/	/* XXX gi->h */
/*@access fnpyKey @*/	/* XXX cast */

#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

GENfree(const void **)

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

/*@unchecked@*/
int rpmcliPackagesTotal = 0;
/*@unchecked@*/
int rpmcliHashesCurrent = 0;
/*@unchecked@*/
int rpmcliHashesTotal = 0;
/*@unchecked@*/
rpmuint64_t rpmcliProgressCurrent = 0;
/*@unchecked@*/
rpmuint64_t rpmcliProgressTotal = 0;

/**
 * Print a CLI progress bar.
 * @todo Unsnarl isatty(STDOUT_FILENO) from the control flow.
 * @param amount	current
 * @param total		final
 */
static void printHash(const rpmuint64_t amount, const rpmuint64_t total)
	/*@globals rpmcliHashesCurrent, rpmcliHashesTotal,
		rpmcliProgressCurrent, fileSystem @*/
	/*@modifies rpmcliHashesCurrent, rpmcliHashesTotal,
		rpmcliProgressCurrent, fileSystem @*/
{
    int hashesNeeded;

    rpmcliHashesTotal = (isatty (STDOUT_FILENO) ? 44 : 50);

    if (rpmcliHashesCurrent != rpmcliHashesTotal) {
	float pct = (float) (total ? (((float) amount) / total) : 1);
	hashesNeeded = (int)((rpmcliHashesTotal * pct) + 0.5);
	while (hashesNeeded > rpmcliHashesCurrent) {
	    if (isatty (STDOUT_FILENO)) {
		int i;
		for (i = 0; i < rpmcliHashesCurrent; i++)
		    (void) putchar ('#');
		for (; i < rpmcliHashesTotal; i++)
		    (void) putchar (' ');
		fprintf(stdout, "(%3d%%)", (int)((100 * pct) + 0.5));
		for (i = 0; i < (rpmcliHashesTotal + 6); i++)
		    (void) putchar ('\b');
	    } else
		fprintf(stdout, "#");

	    rpmcliHashesCurrent++;
	}
	(void) fflush(stdout);

	if (rpmcliHashesCurrent == rpmcliHashesTotal) {
	    int i;
	    rpmcliProgressCurrent++;
	    if (isatty(STDOUT_FILENO)) {
	        for (i = 1; i < rpmcliHashesCurrent; i++)
		    (void) putchar ('#');
		pct = (float) (rpmcliProgressTotal
		    ? (((float) rpmcliProgressCurrent) / rpmcliProgressTotal)
		    : 1);
		fprintf(stdout, " [%3d%%]", (int)((100 * pct) + 0.5));
	    }
	    fprintf(stdout, "\n");
	}
	(void) fflush(stdout);
    }
}

void * rpmShowProgress(/*@null@*/ const void * arg,
			const rpmCallbackType what,
			const rpmuint64_t amount,
			const rpmuint64_t total,
			/*@null@*/ fnpyKey key,
			/*@null@*/ void * data)
	/*@globals rpmcliHashesCurrent, rpmcliProgressCurrent, rpmcliProgressTotal,
		rpmGlobalMacroContext, fileSystem @*/
	/*@modifies rpmcliHashesCurrent, rpmcliProgressCurrent, rpmcliProgressTotal,
		rpmGlobalMacroContext, fileSystem @*/
{
/*@-abstract -castexpose @*/
    Header h = (Header) arg;
/*@=abstract =castexpose @*/
    const char * s;
    int flags = (int) ((long)data);
    void * rc = NULL;
/*@-abstract -assignexpose @*/
    const char * filename = (const char *)key;
/*@=abstract =assignexpose @*/
    static FD_t fd = NULL;
    int xx;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
	if (filename == NULL || filename[0] == '\0')
	    return NULL;
	fd = Fopen(filename, "r%{?_rpmgio}");

	/* XXX Retry once to handle http:// server timeout reopen's. */
	if (Ferror(fd)) {
	    int ut = urlPath(filename, NULL);
	    if (ut == URL_IS_HTTP || ut == URL_IS_HTTPS) {
		/* XXX HACK: Fclose(fd) no workie here. */
		fd = Fopen(filename, "r%{?_rpmgio}");
	    }
	}

	if (fd == NULL || Ferror(fd)) {
	    rpmlog(RPMLOG_ERR, _("open of %s failed: %s\n"), filename,
			Fstrerror(fd));
	    if (fd != NULL) {
		xx = Fclose(fd);
		fd = NULL;
	    }
	} else
	    fd = fdLink(fd, "persist (showProgress)");

#if defined(POSIX_FADV_WILLNEED)
	(void) Fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
#endif

/*@+voidabstract@*/
	return (void *)fd;
/*@=voidabstract@*/
	/*@notreached@*/ break;

    case RPMCALLBACK_INST_CLOSE_FILE:
	/*@-type@*/ /* FIX: still necessary? */
	fd = fdFree(fd, "persist (showProgress)");
	/*@=type@*/
	if (fd != NULL) {
	    xx = Fclose(fd);
	    fd = NULL;
	}
	break;

    case RPMCALLBACK_INST_START:
	rpmcliHashesCurrent = 0;
	if (h == NULL || !(flags & INSTALL_LABEL))
	    break;
	/* @todo Remove headerSprintf() on a progress callback. */
	if (flags & INSTALL_HASH) {
	    s = headerSprintf(h, "%{NAME}",
				NULL, rpmHeaderFormats, NULL);
	    if (isatty (STDOUT_FILENO))
		fprintf(stdout, "%4d:%-23.23s", (int)rpmcliProgressCurrent + 1, s);
	    else
		fprintf(stdout, "%-28.28s", s);
	    (void) fflush(stdout);
	    s = _free(s);
	} else {
	    char * t = rpmExpand("%{?___NVRA}%{!?___NVRA:%%{NAME}-%%{VERSION}-%%{RELEASE}}", NULL);
	    s = headerSprintf(h, t, NULL, rpmHeaderFormats, NULL);
	    fprintf(stdout, "%s\n", s);
	    (void) fflush(stdout);
	    s = _free(s);
	    t = _free(t);
	}
	break;

    case RPMCALLBACK_TRANS_PROGRESS:
    case RPMCALLBACK_INST_PROGRESS:
/*@+relaxtypes@*/
	if (flags & INSTALL_PERCENT)
	    fprintf(stdout, "%%%% %f\n", (double) (total
				? ((((float) amount) / total) * 100)
				: 100.0));
	else if (flags & INSTALL_HASH)
	    printHash(amount, total);
/*@=relaxtypes@*/
	(void) fflush(stdout);
	break;

    case RPMCALLBACK_TRANS_START:
	rpmcliHashesCurrent = 0;
	rpmcliProgressTotal = 1;
	rpmcliProgressCurrent = 0;
	if (!(flags & INSTALL_LABEL))
	    break;
	if (flags & INSTALL_HASH)
	    fprintf(stdout, "%-28s", _("Preparing..."));
	else
	    fprintf(stdout, "%s\n", _("Preparing packages for installation..."));
	(void) fflush(stdout);
	break;

    case RPMCALLBACK_TRANS_STOP:
	if (flags & INSTALL_HASH)
	    printHash(1, 1);	/* Fixes "preparing..." progress bar */
	rpmcliProgressTotal = rpmcliPackagesTotal;
	rpmcliProgressCurrent = 0;
	break;

    case RPMCALLBACK_REPACKAGE_START:
	rpmcliHashesCurrent = 0;
	rpmcliProgressTotal = total;
	rpmcliProgressCurrent = 0;
	if (!(flags & INSTALL_LABEL))
	    break;
	if (flags & INSTALL_HASH)
	    fprintf(stdout, "%-28s\n", _("Repackaging..."));
	else
	    fprintf(stdout, "%s\n", _("Repackaging erased files..."));
	(void) fflush(stdout);
	break;

    case RPMCALLBACK_REPACKAGE_PROGRESS:
	if (amount && (flags & INSTALL_HASH))
	    printHash(1, 1);	/* Fixes "preparing..." progress bar */
	break;

    case RPMCALLBACK_REPACKAGE_STOP:
	rpmcliProgressTotal = total;
	rpmcliProgressCurrent = total;
	if (flags & INSTALL_HASH)
	    printHash(1, 1);	/* Fixes "preparing..." progress bar */
	rpmcliProgressTotal = rpmcliPackagesTotal;
	rpmcliProgressCurrent = 0;
	if (!(flags & INSTALL_LABEL))
	    break;
	if (flags & INSTALL_HASH)
	    fprintf(stdout, "%-28s\n", _("Upgrading..."));
	else
	    fprintf(stdout, "%s\n", _("Upgrading packages..."));
	(void) fflush(stdout);
	break;

    case RPMCALLBACK_UNINST_PROGRESS:
	break;
    case RPMCALLBACK_UNINST_START:
	break;
    case RPMCALLBACK_UNINST_STOP:
	break;
    case RPMCALLBACK_UNPACK_ERROR:
	break;
    case RPMCALLBACK_CPIO_ERROR:
	break;
    case RPMCALLBACK_SCRIPT_ERROR:
	break;
    case RPMCALLBACK_UNKNOWN:
    default:
	break;
    }

    return rc;
}

int rpmcliInstallProblems(rpmts ts, const char * msg, int rc)
	/*@globals fileSystem @*/
	/*@modifies ts, fileSystem @*/
{
    rpmps ps = rpmtsProblems(ts);

    if (rc && rpmpsNumProblems(ps) > 0) {
	if (msg)
	    rpmlog(RPMLOG_ERR, "%s:\n", msg);
	rpmpsPrint(NULL, ps);
    }
    ps = rpmpsFree(ps);
    return rc;
}

int rpmcliInstallSuggests(rpmts ts)
{
    if (ts->suggests != NULL && ts->nsuggests > 0) {
	const char * s;
	int i;

	rpmlog(RPMLOG_NOTICE, _("    Suggested resolutions:\n"));
	for (i = 0; i < ts->nsuggests && (s = (char *)ts->suggests[i]) != NULL;
	    s = _free(s), ts->suggests[i++] = NULL)
	{
	    rpmlog(RPMLOG_NOTICE, "\t%s\n", s);
	}
	ts->suggests = _free(ts->suggests);
    }
    return 0;
}

int rpmcliInstallCheck(rpmts ts)
{
/*@-evalorder@*/
    return rpmcliInstallProblems(ts, _("Failed dependencies"), rpmtsCheck(ts));
/*@=evalorder@*/
}

int rpmcliInstallOrder(rpmts ts)
{
/*@-evalorder@*/
    return rpmcliInstallProblems(ts, _("Ordering problems"), rpmtsOrder(ts));
/*@=evalorder@*/
}

int rpmcliInstallRun(rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
{
/*@-evalorder@*/
    return rpmcliInstallProblems(ts, _("Install/Erase problems"),
			rpmtsRun(ts, okProbs, ignoreSet));
/*@=evalorder@*/
}

static rpmRC rpmcliEraseElement(rpmts ts, const char * arg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmmi mi;
    Header h;
    rpmRC rc = RPMRC_OK;
    int xx;

    mi = rpmtsInitIterator(ts, RPMTAG_NVRA, arg, 0);
    if (mi == NULL)
	return RPMRC_NOTFOUND;

    while ((h = rpmmiNext(mi)) != NULL) {
	uint32_t hdrNum = rpmmiInstance(mi);

	if (hdrNum == 0) {	/* XXX can't happen. */
	    rc = RPMRC_FAIL;
	    break;
	}
	xx = rpmtsAddEraseElement(ts, h, hdrNum);
    }
    mi = rpmmiFree(mi);

    /* XXX FIXME: return rc? */
    return (rpmRC)0;
}

static const char * rpmcliWalkFirst(ARGV_t av, miRE mire)
	/*@globals fileSystem, internalState @*/
	/*@modifies mire, fileSystem, internalState @*/
{
    /* XXX use global ftsOpts? */
    /* XXX changing FTS_LOGICAL to FTS_PHYSICAL prevents symlink follow. */
    /* XXX FTS_NOCHDIR is automatically assumed for URI's */
    int _ftsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
    FTS * ftsp = NULL;
    FTSENT * fts;
    const char * fn = NULL;
    int fts_level = 1;
    int xx;

    if (av != NULL && av[0] != NULL)
	ftsp = Fts_open((char *const *)av, _ftsOpts, NULL);
    if (ftsp != NULL)
    while((fts = Fts_read(ftsp)) != NULL) {
	switch (fts->fts_info) {
	/* No-op conditions. */
	case FTS_D:		/* preorder directory */
	case FTS_DP:		/* postorder directory */
	    /* XXX Don't recurse downwards, all elements should be files. */
	    if (fts_level > 0 && fts->fts_level >= fts_level)
		xx = Fts_set(ftsp, fts, FTS_SKIP);
	    /*@fallthrough@*/
	case FTS_DOT:		/* dot or dot-dot */
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	case FTS_F:		/* regular file */
	    if (mireRegexec(mire, fts->fts_accpath, 0) < 0)
		continue;
	    /*@switchbreak@*/ break;
	/* Error conditions. */
	case FTS_NS:		/* stat(2) failed */
	case FTS_DNR:		/* unreadable directory */
	case FTS_ERR:		/* error; errno is set */
	case FTS_DC:		/* directory that causes cycles */
	case FTS_DEFAULT:	/* none of the above */
	case FTS_INIT:		/* initialized only */
	case FTS_NSOK:		/* no stat(2) requested */
	case FTS_SL:		/* symbolic link */
	case FTS_SLNONE:	/* symbolic link without target */
	case FTS_W:		/* whiteout object */
	default:
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}

	/* Stop on first file that matches. */
	fn = xstrdup(fts->fts_accpath);
	break;
    }

exit:
    xx = Fts_close(ftsp);
    return fn;
}

static const char * rpmcliInstallElementPath(/*@unused@*/ rpmts ts,
		const char * arg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    /* A glob pattern list to match repository directories. */
    const char * fn = rpmExpand(
	"%{?_rpmgi_pattern_glob}"
	"%{!?_rpmgi_pattern_glob:.}",
	NULL
    );
    /* A regex pattern list to match candidate *.rpm files. */
    const char * mirePattern = rpmExpand(
        "%{?_rpmgi_pattern_regex:%{_rpmgi_pattern_regex ", arg, "}}"
        "%{!?_rpmgi_pattern_regex:", arg, "-[^-]+-[^-]+\\.[^.]+\\.rpm$}",
        NULL
    );
    miRE mire = mireNew(RPMMIRE_REGEX, 0);
    ARGV_t dav = NULL;
    int dac = 0;
    ARGV_t av = NULL;
    int xx = mireRegcomp(mire, mirePattern);
    int i;

    /* Get list of candidate repository patterns. */
    xx = argvSplit(&dav, fn, ":");
    fn = _free(fn);
    if (xx || dav == NULL)
	goto exit;

    dac = argvCount(dav);
    for (i = 0; i < dac; i++) {
	ARGV_t nav = NULL;
	int nac = 0;

	/* Make sure only directory paths are matched. */
	fn = rpmGetPath(dav[i], "/", NULL);
	xx = rpmGlob(fn, &nac, &nav);

	if (nav != NULL)
	for (i = 0; i < nac; i++) {
	    const char * t = nav[i];
	    size_t nt = strlen(t);

	    /* Make sure that final directory paths have trailing '/' */
	    if (!(nt > 0 && t[nt-1] == '/'))
		continue;

	    t = rpmExpand(t, "/", NULL);
	    nav[i] = _free(nav[i]);
	    nav[i] = t;
	}

	/* Append matches to list of repository directories. */
	if (nac > 0 && nav != NULL)
	    xx = argvAppend(&av, nav);
	nav = argvFree(nav);
	nac = 0;
	fn = _free(fn);
    }

    /* Walk (possibly multi-root'd) directories, until 1st match is found. */
    fn = rpmcliWalkFirst(av, mire);

exit:
    av = argvFree(av);
    dav = argvFree(dav);
    mire = mireFree(mire);
    mirePattern = _free(mirePattern);

    return fn;
}

/*@-redef@*/	/* XXX Add rpmfi methods to make rpmRelocation opaque. */
struct rpmRelocation_s {
/*@only@*/ /*@null@*/
    const char * oldPath;       /*!< NULL here evals to RPMTAG_DEFAULTPREFIX, */
/*@only@*/ /*@null@*/
    const char * newPath;       /*!< NULL means to omit the file completely! */
};
/*@=redef@*/

/** @todo Generalize --freshen policies. */
int rpmcliInstall(rpmts ts, QVA_t ia, const char ** argv)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    ARGV_t avfn = NULL;
    int acfn = 0;
    int numFailed = 0;
    int numRPMS = 0;
    rpmRelocation relocations = NULL;
    rpmVSFlags vsflags, ovsflags;
    rpmRC rpmrc;
    int rc;
    int xx;

    if (argv == NULL) goto exit;

    (void) rpmtsSetGoal(ts, TSM_INSTALL);
    rpmcliPackagesTotal = 0;

    if (rpmExpandNumeric("%{?_repackage_all_erasures}"))
	ia->transFlags = (rpmtransFlags)
		(ia->transFlags | RPMTRANS_FLAG_REPACKAGE);

    (void) rpmtsSetFlags(ts, ia->transFlags);
    (void) rpmtsSetDFlags(ts, ia->depFlags);

    /* Display and set autorollback goal. */
    if (rpmExpandNumeric("%{?_rollback_transaction_on_failure}")) {
	if (ia->arbtid) {
	    time_t ttid = (time_t)ia->arbtid;
	    rpmlog(RPMLOG_DEBUG, D_("Autorollback Goal: %-24.24s (0x%08x)\n"),
		ctime(&ttid), ia->arbtid);
	    rpmtsSetARBGoal(ts, ia->arbtid);
	}
    }

    if (ia->installInterfaceFlags & INSTALL_UPGRADE)
	vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_erase}");
    else
	vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_install}");
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
    VSF_SET(vsflags, NEEDPAYLOAD);
#endif
    ovsflags = rpmtsSetVSFlags(ts, vsflags);

    {	int notifyFlags;
	notifyFlags = ia->installInterfaceFlags | (rpmIsVerbose() ? INSTALL_LABEL : 0 );
	xx = rpmtsSetNotifyCallback(ts,
			rpmShowProgress, (void *) ((long)notifyFlags));
    }

    if ((relocations = ia->relocations) != NULL) {
	while (relocations->oldPath)
	    relocations++;
	if (relocations->newPath == NULL)
	    relocations = NULL;
    }

 {	/* start-of-transaction-build */
    int tag = (ia->qva_source == RPMQV_FTSWALK)
	? RPMDBI_FTSWALK : RPMDBI_ARGLIST;
    rpmgi gi = rpmgiNew(ts, tag, NULL, 0);
    rpmgiFlags _giFlags = RPMGI_NONE;
    const char * fn = NULL;;

/*@-mods@*/
    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
/*@=mods@*/
    rc = rpmgiSetArgs(gi, argv, rpmioFtsOpts, _giFlags);
    while ((rpmrc = rpmgiNext(gi)) == RPMRC_OK) {
	Header h;

	fn = _free(fn);
	fn = xstrdup(rpmgiHdrPath(gi));

        /* === Check for "+bing" lookaside paths within install transaction. */
        if (fn[0] == '+') {
	    const char * nfn;
	    addMacro(NULL, "NEVRA", NULL, &fn[1], RMIL_GLOBAL);
	    nfn = rpmcliInstallElementPath(ts, &fn[1]);
	    delMacro(NULL, "NEVRA");
	    if (nfn == NULL) {
		rpmlog(RPMLOG_ERR, _("package \"%s\" cannot be found\n"), fn);
		numFailed++;	/* XXX multiple erasures? */
		continue;
	    }
	    fn = _free(fn);
	    fn = nfn;
	    /* XXX hack into rpmgi innards for now ... */
	    h = rpmgiReadHeader(gi, fn);
	    if (h != NULL)
		gi->h = headerLink(h);
	    (void)headerFree(h);
	    h = NULL;
        }

	/* === Check for "-bang" erasures within install transaction. */
	if (fn[0] == '-') {
	    switch (rpmcliEraseElement(ts, &fn[1])) {
	    case RPMRC_OK:
		numRPMS++;	/* XXX multiple erasures? */
		/*@switchbreak@*/ break;
	    case RPMRC_NOTFOUND:
	    default:
		rpmlog(RPMLOG_ERR, _("package \"%s\" cannot be erased\n"), fn);
		numFailed++;	/* XXX multiple erasures? */
		fn = _free(fn);		/* XXX coverity #1035884 */
		goto exit;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    }
	    continue;
	}

	h = rpmgiHeader(gi);
	if (h == NULL) {
	    numFailed++;
	    continue;
	}

	/* === Check for relocatable package. */
	if (relocations) {
	    he->tag = RPMTAG_PREFIXES;
	    xx = headerGet(h, he, 0);
	    if (xx && he->c == 1) {
		relocations->oldPath = xstrdup(he->p.argv[0]);
		he->p.ptr = _free(he->p.ptr);
	    } else {
		he->p.ptr = _free(he->p.ptr);
		he->tag = RPMTAG_NVRA;
		xx = headerGet(h, he, 0);
		rpmlog(RPMLOG_ERR,
			       _("package %s is not relocatable\n"), he->p.str);
		he->p.ptr = _free(he->p.ptr);
		numFailed++;
		fn = _free(fn);		/* XXX coverity #1035884 */
		goto exit;
		/*@notreached@*/
	    }
	}

	/* === On --freshen, verify package is installed and newer. */
	if (ia->installInterfaceFlags & INSTALL_FRESHEN) {
	    rpmmi mi;
	    Header oldH;
	    int count;

	    he->tag = RPMTAG_NAME;
	    xx = headerGet(h, he, 0);
assert(xx != 0 && he->p.str != NULL);
	    mi = rpmtsInitIterator(ts, RPMTAG_NAME, he->p.str, 0);
	    he->p.ptr = _free(he->p.ptr);
	    count = rpmmiCount(mi);
	    while ((oldH = rpmmiNext(mi)) != NULL) {
		if (rpmVersionCompare(oldH, h) < 0)
		    /*@innercontinue@*/ continue;
		/* same or newer package already installed */
		count = 0;
		/*@innerbreak@*/ break;
	    }
	    mi = rpmmiFree(mi);
	    if (count == 0)
		continue;
	    /* Package is newer than those currently installed. */
	}

	/* === Add binary package to transaction set. */
	xx = argvAdd(&avfn, fn);
	rc = rpmtsAddInstallElement(ts, h, (fnpyKey)avfn[acfn++],
			(ia->installInterfaceFlags & INSTALL_UPGRADE) != 0,
			ia->relocations);

	if (relocations)
	    relocations->oldPath = _free(relocations->oldPath);

	numRPMS++;
    }

    /* XXX disambiguate end-of-iteration from item failures. */
    if (rpmrc == RPMRC_NOTFOUND)
	rpmrc = (rpmRC) rpmgiRc(gi);

    fn = _free(fn);
    gi = rpmgiFree(gi);

 }	/* end-of-transaction-build */

    /* XXX exit if the iteration failed. */
    if (rpmrc != RPMRC_OK)
	numFailed = (numRPMS ? numRPMS : 1); /* XXX error on no-op. */
    if (numFailed) goto exit;

    if (numRPMS) {
	if (!(ia->installInterfaceFlags & INSTALL_NODEPS)
	 && (rc = rpmcliInstallCheck(ts)) != 0) {
	    numFailed = numRPMS;
	    (void) rpmcliInstallSuggests(ts);
	}

	if (!(ia->installInterfaceFlags & INSTALL_NOORDER)
	 && (rc = rpmcliInstallOrder(ts)) != 0)
	    numFailed = (numRPMS ? numRPMS : 1); /* XXX error on no-op. */

	/* Drop added/available package indices and dependency sets. */
	rpmtsClean(ts);

	/* XXX Avoid empty transaction msg, run iff there are elements. */
	if (numFailed == 0 && rpmtsNElements(ts) > 0
	 && (rc = rpmcliInstallRun(ts, NULL, ia->probFilter)) != 0) {
	    if (rc > 0)
		numFailed = rc;
	    else
		numFailed = (numRPMS ? numRPMS : 1); /* XXX error on no-op. */
	}
    }

    if (numFailed) goto exit;

exit:
    avfn = argvFree(avfn);

#ifdef	NOTYET	/* XXX grrr, segfault in selabel_close */
    if (!(ia->transFlags & RPMTRANS_FLAG_NOCONTEXTS))
	matchpathcon_fini();
#endif

    rpmtsEmpty(ts);

    return numFailed;
}

int rpmErase(rpmts ts, QVA_t ia, const char ** argv)
{
    const char ** arg;
    int numFailed = 0;
    int numRPMS = 0;
    rpmVSFlags vsflags, ovsflags;
    int rc;

    if (argv == NULL) return 0;

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
    VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif
    ovsflags = rpmtsSetVSFlags(ts, vsflags);

    if (rpmExpandNumeric("%{?_repackage_all_erasures}"))
	ia->transFlags = (rpmtransFlags)
		(ia->transFlags | RPMTRANS_FLAG_REPACKAGE);

    (void) rpmtsSetFlags(ts, ia->transFlags);
    (void) rpmtsSetDFlags(ts, ia->depFlags);

    /* Display and set autorollback goal. */
    if (rpmExpandNumeric("%{?_rollback_transaction_on_failure}")) {
	if (ia->arbtid) {
	    time_t ttid = (time_t)ia->arbtid;
	    rpmlog(RPMLOG_DEBUG, D_("Autorollback Goal: %-24.24s (0x%08x)\n"),
		ctime(&ttid), ia->arbtid);
	    rpmtsSetARBGoal(ts, ia->arbtid);
	}
    }

#ifdef	NOTYET	/* XXX no callbacks on erase yet */
    {	int notifyFlags;
	notifyFlags = ia->installInterfaceFlags | (rpmIsVerbose() ? INSTALL_LABEL : 0 );
	xx = rpmtsSetNotifyCallback(ts,
			rpmShowProgress, (void *) ((long)notifyFlags));
    }
#endif

    (void) rpmtsSetGoal(ts, TSM_ERASE);

    for (arg = argv; *arg; arg++) {
	rpmmi mi;

	/* XXX HACK to get rpmdbFindByLabel out of the API */
	mi = rpmtsInitIterator(ts, RPMTAG_NVRA, *arg, 0);
	if (mi == NULL) {
	    rpmlog(RPMLOG_ERR, _("package %s is not installed\n"), *arg);
	    numFailed++;
	} else {
	    Header h;	/* XXX iterator owns the reference */
	    int count = 0;
	    while ((h = rpmmiNext(mi)) != NULL) {
		uint32_t hdrNum = rpmmiInstance(mi);

		if (!(count++ == 0 || (ia->installInterfaceFlags & INSTALL_ALLMATCHES))) {
		    rpmlog(RPMLOG_ERR, _("\"%s\" specifies multiple packages\n"),
			*arg);
		    numFailed++;
		    /*@innerbreak@*/ break;
		}
		if (hdrNum) {
		    (void) rpmtsAddEraseElement(ts, h, hdrNum);
		    numRPMS++;
		}
	    }
	}
	mi = rpmmiFree(mi);
    }

    if (numFailed == 0 && numRPMS > 0) {
	if (!(ia->installInterfaceFlags & INSTALL_NODEPS)
	 && (rc = rpmcliInstallCheck(ts)) != 0)
	    numFailed = numRPMS;

	if (numFailed == 0
	 && !(ia->installInterfaceFlags & INSTALL_NOORDER)
	 && (rc = rpmcliInstallOrder(ts)) != 0)
	    numFailed = numRPMS;

	/* Drop added/available package indices and dependency sets. */
	rpmtsClean(ts);

	if (numFailed == 0
	 && (rc = rpmcliInstallRun(ts, NULL, (rpmprobFilterFlags) (ia->probFilter & (RPMPROB_FILTER_DISKSPACE|RPMPROB_FILTER_DISKNODES)))) != 0)
	    numFailed += (rc < 0 ? numRPMS : rc);

    }

    rpmtsEmpty(ts);

    return numFailed;
}

int rpmInstallSource(rpmts ts, const char * arg,
		const char ** specFilePtr, const char ** cookie)
{
    FD_t fd = Fopen(arg, "r%{?_rpmgio}");
    rpmRC rc = RPMRC_FAIL;	/* assume failure */

    if (fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("cannot open %s: %s\n"), arg, Fstrerror(fd));
	goto exit;
    }

    if (rpmIsVerbose())
	fprintf(stdout, _("Installing %s\n"), arg);

    {	
	rpmVSFlags nvsflags, ovsflags;
	nvsflags = rpmtsVSFlags(ts);
	VSF_SET(nvsflags, NEEDPAYLOAD);
	ovsflags = rpmtsSetVSFlags(ts, nvsflags);
	rc = rpmInstallSourcePackage(ts, fd, specFilePtr, cookie);
	ovsflags = rpmtsSetVSFlags(ts, ovsflags);
    }
    if (rc != RPMRC_OK)
	rpmlog(RPMLOG_ERR, _("%s cannot be installed\n"), arg);

exit:
    if (fd != NULL) (void) Fclose(fd);

    return (rc == RPMRC_OK ? 0 : 1);
}
