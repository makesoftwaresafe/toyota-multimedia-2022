/** \ingroup rpmcli
 * \file lib/query.c
 * Display tag values from package metadata.
 */

#include "system.h"

#ifndef PATH_MAX
/*@-incondefs@*/	/* FIX: long int? */
# define PATH_MAX 255
/*@=incondefs@*/
#endif

#include <rpmio.h>
#include <rpmiotypes.h>
#include <poptIO.h>

#include <rpmtag.h>
#include "rpmdb.h"

#include "rpmfi.h"
#define	_RPMTS_INTERNAL		/* XXX for ts->rdb */
#include "rpmts.h"
#include "rpmgi.h"

#include "manifest.h"
#include "misc.h"	/* XXX for currentDirectory() */

#include <rpmcli.h>

#include "debug.h"

static int _jbj;
#define	JBJDEBUG(_list)	if (_jbj) fprintf _list

/*@access rpmts @*/	/* XXX cast */

#ifdef __cplusplus

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (QUERY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define FF_ISSET(_fflags, _FLAG)	((_fflags) & (RPMFILE_##_FLAG))

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (QUERY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

/**
 */
static void printFileInfo(char * te, const char * name,
			  size_t size, unsigned short mode,
			  unsigned int mtime,
			  unsigned short rdev, unsigned int nlink,
			  const char * owner, const char * group,
			  const char * linkto)
	/*@modifies *te @*/
{
    char sizefield[15];
#if defined(RPM_VENDOR_OPENPKG) /* adjust-verbose-listing */
    /* In verbose file listing output, give the owner and group fields
       more width and at the same time reduce the nlink and size fields
       more to typical sizes within OpenPKG. */
    char ownerfield[13+1], groupfield[13+1];
#else
    char ownerfield[8+1], groupfield[8+1];
#endif
    char timefield[100];
    time_t when = mtime;  /* important if sizeof(rpmuint32_t) ! sizeof(time_t) */
    struct tm * tm;
    static time_t now;
    static struct tm nowtm;
    const char * namefield = name;
    char * perms = rpmPermsString(mode);

    /* On first call, grab snapshot of now */
    if (now == 0) {
	now = time(NULL);
	tm = localtime(&now);
	if (tm) nowtm = *tm;	/* structure assignment */
    }

    strncpy(ownerfield, owner, sizeof(ownerfield));
    ownerfield[sizeof(ownerfield)-1] = '\0';

    strncpy(groupfield, group, sizeof(groupfield));
    groupfield[sizeof(groupfield)-1] = '\0';

    /* this is normally right */
#if defined(RPM_VENDOR_OPENPKG) /* adjust-verbose-listing */
    /* In verbose file listing output, give the owner and group fields
       more width and at the same time reduce the nlink and size fields
       more to typical sizes within OpenPKG. */
    sprintf(sizefield, "%8u", (unsigned)size);
#else
    sprintf(sizefield, "%12u", (unsigned)size);
#endif

    /* this knows too much about dev_t */

    if (S_ISLNK(mode)) {
	char *nf = (char *)
		alloca(strlen(name) + sizeof(" -> ") + strlen(linkto));
	sprintf(nf, "%s -> %s", name, linkto);
	namefield = nf;
    } else if (S_ISCHR(mode)) {
	perms[0] = 'c';
	sprintf(sizefield, "%3u, %3u", ((unsigned)(rdev >> 8) & 0xff),
			((unsigned)rdev & 0xff));
    } else if (S_ISBLK(mode)) {
	perms[0] = 'b';
	sprintf(sizefield, "%3u, %3u", ((unsigned)(rdev >> 8) & 0xff),
			((unsigned)rdev & 0xff));
    }

    /* Convert file mtime to display format */
    tm = localtime(&when);
    timefield[0] = '\0';
    if (tm != NULL)
    {	const char *fmt;
	if (now > when + 6L * 30L * 24L * 60L * 60L ||	/* Old. */
	    now < when - 60L * 60L)			/* In the future.  */
	{
	/* The file is fairly old or in the future.
	 * POSIX says the cutoff is 6 months old;
	 * approximate this by 6*30 days.
	 * Allow a 1 hour slop factor for what is considered "the future",
	 * to allow for NFS server/client clock disagreement.
	 * Show the year instead of the time of day.
	 */        
	    fmt = "%b %e  %Y";
	} else {
	    fmt = "%b %e %H:%M";
	}
	(void)strftime(timefield, sizeof(timefield) - 1, fmt, tm);
    }

#if defined(RPM_VENDOR_OPENPKG) /* adjust-verbose-listing */
    /* In verbose file listing output, give the owner and group fields
       more width and at the same time reduce the nlink and size fields
       more to typical sizes within OpenPKG. */
    sprintf(te, "%s %d %-13s %-13s %8s %s %s", perms,
	(int)nlink, ownerfield, groupfield, sizefield, timefield, namefield);
#else
    sprintf(te, "%s %4d %-7s %-8s %10s %s %s", perms,
	(int)nlink, ownerfield, groupfield, sizefield, timefield, namefield);
#endif
    perms = _free(perms);
}

/**
 */
static inline /*@null@*/ const char * queryHeader(Header h, const char * qfmt)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/
{
    const char * errstr = "(unkown error)";
    const char * str;

/*@-modobserver@*/
    str = headerSprintf(h, qfmt, NULL, rpmHeaderFormats, &errstr);
/*@=modobserver@*/
    if (str == NULL)
	rpmlog(RPMLOG_ERR, _("incorrect format: %s\n"), errstr);
    return str;
}

/**
 */
static void flushBuffer(char ** tp, char ** tep, int nonewline)
	/*@modifies *tp, **tp, *tep, **tep @*/
{
    char *t, *te;

    t = *tp;
    te = *tep;
    if (te > t) {
	if (!nonewline) {
	    *te++ = '\n';
	    *te = '\0';
	}
	rpmlog(RPMLOG_NOTICE, "%s", t);
	te = t;
	*t = '\0';
    }
    *tp = t;
    *tep = te;
}

int showQueryPackage(QVA_t qva, rpmts ts, Header h)
{
    int scareMem = 0;
    rpmfi fi = NULL;
    size_t tb = 2 * BUFSIZ;
    size_t sb;
    char * t, * te;
    char * prefix = NULL;
    int rc = 0;		/* XXX FIXME: need real return code */
    int i;

JBJDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, qva, ts, h));
    te = t = (char *) xmalloc(tb);
    *te = '\0';

    if (qva->qva_queryFormat != NULL) {
	const char * str;
/*@-type@*/	/* FIX rpmtsGetRDB()? */
	(void) headerSetRpmdb(h, ts->rdb);
/*@=type@*/
	str = queryHeader(h, qva->qva_queryFormat);
	(void) headerSetRpmdb(h, NULL);
	if (str) {
	    size_t tx = (te - t);

	    sb = strlen(str);
	    if (sb) {
		tb += sb;
		t = (char *) xrealloc(t, tb);
		te = t + tx;
	    }
	    /*@-usereleased@*/
	    te = stpcpy(te, str);
	    /*@=usereleased@*/
	    str = _free(str);
	    flushBuffer(&t, &te, 1);
	}
    }

    if (!QVA_ISSET(qva->qva_flags, FOR_LIST))
	goto exit;

    fi = rpmfiNew(ts, h, RPMTAG_BASENAMES, scareMem);
    if (rpmfiFC(fi) <= 0) {
	te = stpcpy(te, _("(contains no files)"));
	goto exit;
    }

    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while ((i = rpmfiNext(fi)) >= 0) {
	rpmfileAttrs fflags;
	unsigned short fmode;
 	unsigned short frdev;
	unsigned int fmtime;
	rpmfileState fstate;
	size_t fsize;
	const char * fn;
	const char * fdigest;
	const char * fuser;
	const char * fgroup;
	const char * flink;
	rpmuint32_t fnlink;

	fflags = (rpmfileAttrs) rpmfiFFlags(fi);
	fmode = rpmfiFMode(fi);
	frdev = rpmfiFRdev(fi);
	fmtime = rpmfiFMtime(fi);
	fstate = rpmfiFState(fi);
	fsize = rpmfiFSize(fi);
	fn = rpmfiFN(fi);
	{   static char hex[] = "0123456789abcdef";
	    int dalgo = 0;
	    size_t dlen = 0;
	    const unsigned char * digest = rpmfiDigest(fi, &dalgo, &dlen);
	    char * p;
	    size_t j;
	    fdigest = p = (char *) xcalloc(1, ((2 * dlen) + 1));
	    for (j = 0; j < dlen; j++) {
		unsigned k = *digest++;
		*p++ = hex[ (k >> 4) & 0xf ];
		*p++ = hex[ (k     ) & 0xf ];
	    }
	    *p = '\0';
	}
	fuser = rpmfiFUser(fi);
	fgroup = rpmfiFGroup(fi);
	flink = rpmfiFLink(fi);
	fnlink = rpmfiFNlink(fi);
assert(fn != NULL);
assert(fdigest != NULL);

	/* If querying only docs, skip non-doc files. */
	if (QVA_ISSET(qva->qva_flags, FOR_DOCS) && !FF_ISSET(fflags, DOC)) {
	    fdigest = _free(fdigest);
	    continue;
	}

	/* If querying only configs, skip non-config files. */
	if (QVA_ISSET(qva->qva_flags, FOR_CONFIG) && !FF_ISSET(fflags, CONFIG)){
	    fdigest = _free(fdigest);
	    continue;
	}

	/* If not querying %config, skip config files. */
	if (FF_ISSET(qva->qva_fflags, CONFIG) && FF_ISSET(fflags, CONFIG)) {
	    fdigest = _free(fdigest);
	    continue;
	}

	/* If not querying %doc, skip doc files. */
	if (FF_ISSET(qva->qva_fflags, DOC) && FF_ISSET(fflags, DOC)) {
	    fdigest = _free(fdigest);
	    continue;
	}

	/* If not querying %ghost, skip ghost files. */
	if (FF_ISSET(qva->qva_fflags, GHOST) && FF_ISSET(fflags, GHOST)) {
	    fdigest = _free(fdigest);
	    continue;
	}

	/* Insure space for header derived data */
	sb = 0;
	if (fn)		sb += strlen(fn);
	if (fdigest)	sb += strlen(fdigest);
	if (fuser)	sb += strlen(fuser);
	if (fgroup)	sb += strlen(fgroup);
	if (flink)	sb += strlen(flink);
	if ((sb + BUFSIZ) > tb) {
	    size_t tx = (te - t);
	    tb += sb + BUFSIZ;
	    t = (char *) xrealloc(t, tb);
	    te = t + tx;
	}

	if (!rpmIsVerbose() && prefix)
	    te = stpcpy(te, prefix);

	if (QVA_ISSET(qva->qva_flags, FOR_STATE)) {
	    switch (fstate) {
	    case RPMFILE_STATE_NORMAL:
		te = stpcpy(te, _("normal        "));
		/*@switchbreak@*/ break;
	    case RPMFILE_STATE_REPLACED:
		te = stpcpy(te, _("replaced      "));
		/*@switchbreak@*/ break;
	    case RPMFILE_STATE_NOTINSTALLED:
		te = stpcpy(te, _("not installed "));
		/*@switchbreak@*/ break;
	    case RPMFILE_STATE_NETSHARED:
		te = stpcpy(te, _("net shared    "));
		/*@switchbreak@*/ break;
	    case RPMFILE_STATE_WRONGCOLOR:
		te = stpcpy(te, _("wrong color   "));
		/*@switchbreak@*/ break;
	    case RPMFILE_STATE_MISSING:
		te = stpcpy(te, _("(no state)    "));
		/*@switchbreak@*/ break;
	    default:
		sprintf(te, _("(unknown %3d) "), fstate);
		te += strlen(te);
		/*@switchbreak@*/ break;
	    }
	}

	if (QVA_ISSET(qva->qva_flags, FOR_DUMPFILES)) {
	    sprintf(te, "%s %d %d %s 0%o ",
				fn, (int)fsize, fmtime, fdigest, fmode);
	    te += strlen(te);

	    if (fuser && fgroup) {
/*@-nullpass@*/
		sprintf(te, "%s %s", fuser, fgroup);
/*@=nullpass@*/
		te += strlen(te);
	    } else {
		rpmlog(RPMLOG_CRIT, _("package without owner/group tags\n"));
	    }

	    sprintf(te, " %s %s %u ", 
				 FF_ISSET(fflags, CONFIG) ? "1" : "0",
				 FF_ISSET(fflags, DOC) ? "1" : "0",
				 frdev);
	    te += strlen(te);

	    sprintf(te, "%s", (flink && *flink ? flink : "X"));
	    te += strlen(te);
	} else
	if (!rpmIsVerbose()) {
	    te = stpcpy(te, fn);
	}
	else {

	    /* XXX Adjust directory link count and size for display output. */
	    if (S_ISDIR(fmode)) {
		fnlink++;
		fsize = 0;
	    }

	    if (fuser && fgroup) {
/*@-nullpass@*/
		printFileInfo(te, fn, fsize, fmode, fmtime, frdev, fnlink,
					fuser, fgroup, flink);
/*@=nullpass@*/
		te += strlen(te);
	    } else {
		rpmlog(RPMLOG_CRIT, _("package without owner/group tags\n"));
	    }
	}
	flushBuffer(&t, &te, 0);
	fdigest = _free(fdigest);
    }
	    
    rc = 0;

exit:
    flushBuffer(&t, &te, 0);
    t = _free(t);

    fi = rpmfiFree(fi);
JBJDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, qva, ts, h, rc));
    return rc;
}

static int rpmgiShowMatches(QVA_t qva, rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
        /*@modifies qva, rpmGlobalMacroContext, h_errno, internalState @*/
{
    rpmgi gi = qva->qva_gi;
    rpmRC rpmrc = RPMRC_NOTFOUND;
    int ec = 0;

JBJDEBUG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, qva, ts));

    while ((rpmrc = rpmgiNext(gi)) == RPMRC_OK) {
	Header h;
	int rc;

#ifdef	NOTYET	/* XXX exiting here will leave stale locks. */
	(void) rpmdbCheckSignals();
#endif

	h = rpmgiHeader(gi);
	if (h == NULL)		/* XXX perhaps stricter break instead? */
	    continue;
	if ((rc = qva->qva_showPackage(qva, ts, h)) != 0)
	    ec = rc;
	if (qva->qva_source == RPMQV_DBOFFSET)
	    break;
    }
    if (ec == 0 && rpmrc == RPMRC_FAIL)
	ec++;

JBJDEBUG((stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, qva, ts, ec));
    return ec;
}

/** \ingroup rpmcli
 * Display query/verify information for each header in iterator.
 *
 * This routine uses:
 *	- qva->qva_mi		rpm database iterator
 *	- qva->qva_showPackage	query/verify display routine
 *
 * @param qva		parsed query/verify options
 * @param ts		transaction set
 * @return		result of last non-zero showPackage() return
 */
static int rpmcliShowMatches(QVA_t qva, rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies qva, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    Header h;
    int ec = 1;

JBJDEBUG((stderr, "--> %s(%p,%p)\n", __FUNCTION__, qva, ts));
    qva->qva_showFAIL = qva->qva_showOK = 0;
    while ((h = rpmmiNext(qva->qva_mi)) != NULL) {
	ec = qva->qva_showPackage(qva, ts, h);
	if (ec)
	    qva->qva_showFAIL++;
	else
	    qva->qva_showOK++;
	if (qva->qva_source == RPMQV_DBOFFSET)
	    break;
    }
    qva->qva_mi = rpmmiFree(qva->qva_mi);
JBJDEBUG((stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, qva, ts, ec));
    return ec;
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static inline unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (c - '0');
    if (c >= 'A' && c <= 'F')
	return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f')
	return (c - 'a') + 10;
    return 0;
}

int rpmQueryVerify(QVA_t qva, rpmts ts, const char * arg)
{
    int res = 0;
    const char * s;
    int i;
    int provides_checked = 0;

JBJDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, qva, ts, arg));

    (void) rpmdbCheckSignals();

    if (qva->qva_showPackage == NULL)
	return 1;

    switch (qva->qva_source) {
#ifdef	NOTYET
    default:
#endif
    case RPMQV_GROUP:
    case RPMQV_TRIGGEREDBY:
    case RPMQV_WHATCONFLICTS:
    case RPMQV_WHATOBSOLETES:
	qva->qva_mi = rpmtsInitIterator(ts, (rpmTag) qva->qva_source, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("key \"%s\" not found in %s table\n"),
			arg, tagName((rpmTag)qva->qva_source));
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
	break;

    case RPMQV_RPM:
	res = rpmgiShowMatches(qva, ts);
	break;

    case RPMQV_ALL:
	res = rpmgiShowMatches(qva, ts);
	break;

    case RPMQV_HDLIST:
	res = rpmgiShowMatches(qva, ts);
	break;

    case RPMQV_FTSWALK:
	res = rpmgiShowMatches(qva, ts);
	break;

    case RPMQV_SPECSRPM:
    case RPMQV_SPECFILE:
	res = ((qva->qva_specQuery != NULL)
		? qva->qva_specQuery(ts, qva, arg) : 1);
	break;

#ifdef	DYING
    case RPMQV_GROUP:
	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_GROUP, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_ERR,
		_("group %s does not contain any packages\n"), arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
	break;

    case RPMQV_TRIGGEREDBY:
	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_TRIGGERNAME, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package triggers %s\n"), arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
	break;
#endif

    case RPMQV_SOURCEPKGID:
    case RPMQV_PKGID:
#define	MD5len	128/8	/* XXX coverity #1035850 */
    {	unsigned char MD5[MD5len+1];
	unsigned char * t;
	rpmuint32_t tag;

	for (i = 0, s = arg; *s && isxdigit(*s); s++, i++)
	    {};
	if (i != 32) {
	    rpmlog(RPMLOG_NOTICE, _("malformed %s: %s\n"), "pkgid", arg);
	    return 1;
	}

	MD5[0] = '\0';
        for (i = 0, t = MD5, s = arg; i < MD5len; i++, t++, s += 2)
            *t = (nibble(s[0]) << 4) | nibble(s[1]);
	
	tag = (qva->qva_source == RPMQV_PKGID
		? RPMTAG_SOURCEPKGID : RPMTAG_PKGID);
	qva->qva_mi = rpmtsInitIterator(ts, (rpmTag) tag, MD5, MD5len);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package matches %s: %s\n"),
			"pkgid", arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
    }	break;
#undef	MD5len

    case RPMQV_HDRID:
	for (i = 0, s = arg; *s && isxdigit(*s); s++, i++)
	    {};
	if (i != 40) {
	    rpmlog(RPMLOG_NOTICE, _("malformed %s: %s\n"), "hdrid", arg);
	    return 1;
	}

	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_SHA1HEADER, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package matches %s: %s\n"),
			"hdrid", arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
	break;

    case RPMQV_FILEID:
    {	unsigned char * t;
	unsigned char * digest;
	size_t dlen;

	/* Insure even no. of digits and at least 8 digits. */
	for (dlen = 0, s = arg; *s && isxdigit(*s); s++, dlen++)
	    {};
	if ((dlen & 1) || dlen < 8) {
	    rpmlog(RPMLOG_ERR, _("malformed %s: %s\n"), "fileid", arg);
	    return 1;
	}

	dlen /= 2;
	digest = (unsigned char *) memset(alloca(dlen), 0, dlen);
        for (t = digest, s = arg; *s; t++, s += 2)
            *t = (nibble(s[0]) << 4) | nibble(s[1]);

	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_FILEDIGESTS, digest, dlen);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package matches %s: %s\n"),
			"fileid", arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
    }	break;

    case RPMQV_TID:
    {	int mybase = 10;
	const char * myarg = arg;
	char * end = NULL;
	unsigned iid;

	/* XXX should be in strtoul */
	if (*myarg == '0') {
	    myarg++;
	    mybase = 8;
	    if (*myarg == 'x') {
		myarg++;
		mybase = 16;
	    }
	}
	iid = (unsigned) strtoul(myarg, &end, mybase);
	if ((*end) || (end == arg) || (iid == UINT_MAX)) {
	    rpmlog(RPMLOG_ERR, _("malformed %s: %s\n"), "tid", arg);
	    return 1;
	}
	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_INSTALLTID, &iid, sizeof(iid));
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package matches %s: %s\n"),
			"tid", arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
    }	break;

    case RPMQV_WHATNEEDS:
    case RPMQV_WHATREQUIRES:
	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_REQUIRENAME, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("no package requires %s\n"), arg);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
	break;

    case RPMQV_WHATPROVIDES:
	if (arg[0] != '/') {
	    provides_checked = 1;
	    qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, arg, 0);
	    if (qva->qva_mi == NULL) {
		rpmlog(RPMLOG_NOTICE, _("no package provides %s\n"), arg);
		res = 1;
	    } else
		res = rpmcliShowMatches(qva, ts);
	    break;
	}
	/*@fallthrough@*/
    case RPMQV_PATH:
    {	int gotpattern = 0;
	char * fn;

	if (arg[0] == '^' || arg[strlen(arg)-1] == '$') {
	    fn = xstrdup(arg);
	    gotpattern++;
	} else
#ifdef	NOTYET
	if (arg[0] == '/' && Glob_pattern_p(arg, 1)) {
	    fn = xstrdup(arg);
	    gotpattern++;
	} else
#endif
	{
	    for (s = arg; *s != '\0'; s++) {
		if (!(*s == '.' || *s == '/'))
		    /*@loopbreak@*/ break;
	    }

	    if (*s == '\0') {
		char fnbuf[PATH_MAX];
		fn = Realpath(arg, fnbuf);
		fn = xstrdup( (fn != NULL ? fn : arg) );
	    } else if (*arg != '/') {
		const char *curDir = currentDirectory();
		fn = (char *) rpmGetPath(curDir, "/", arg, NULL);
		curDir = _free(curDir);
	    } else
		fn = xstrdup(arg);
	    (void) rpmCleanPath(fn);
	}

	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_FILEPATHS, fn, 0);
	if (qva->qva_mi == NULL && !provides_checked && !gotpattern) {
	    qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, fn, 0);
#if defined(RPM_VENDOR_MANDRIVA)
	if(rpmmiCount(qva->qva_mi) == 0)
	    qva->qva_mi = rpmmiFree(qva->qva_mi);
#endif
	}

	if (qva->qva_mi == NULL) {
	    struct stat sb;
	    if (!gotpattern && Lstat(fn, &sb) != 0)
		rpmlog(RPMLOG_NOTICE, _("file %s: %s\n"), fn, strerror(errno));
	    else
		rpmlog(RPMLOG_NOTICE,
			_("file %s is not owned by any package\n"), fn);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);

	fn = _free(fn);
    }	break;

    case RPMQV_DBOFFSET:
    {	int mybase = 10;
	const char * myarg = arg;
	char * end = NULL;
	uint32_t hdrNum;

	/* XXX should be in strtoul */
	if (*myarg == '0') {
	    myarg++;
	    mybase = 8;
	    if (*myarg == 'x') {
		myarg++;
		mybase = 16;
	    }
	}
	hdrNum = (uint32_t) strtoul(myarg, &end, mybase);
	if ((*end) || (end == arg) || (hdrNum == UINT_MAX)) {
	    rpmlog(RPMLOG_NOTICE, _("invalid package number: %s\n"), arg);
	    return 1;
	}
	rpmlog(RPMLOG_DEBUG, D_("package record number: %u\n"), (unsigned)hdrNum);
	qva->qva_mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES, &hdrNum, sizeof(hdrNum));
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE,
		_("record %u could not be read\n"), (unsigned)hdrNum);
	    res = 1;
	} else
	    res = rpmcliShowMatches(qva, ts);
    }	break;

    case RPMQV_PACKAGE:
	qva->qva_mi = rpmtsInitIterator(ts, RPMTAG_NVRA, arg, 0);
	if (qva->qva_mi == NULL) {
	    rpmlog(RPMLOG_NOTICE, _("package %s is not installed\n"), arg);
	    res = 1;
	} else {
	    res = rpmcliShowMatches(qva, ts);
	    /* detect foo.bogusarch empty iterations. */
	    if (qva->qva_showOK == 0 && qva->qva_showFAIL == 0) {
		rpmlog(RPMLOG_NOTICE, _("package %s is not installed\n"), arg);
		res = 1;
	    }
	}
	break;
    }
   
JBJDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, qva, ts, arg, res));
    return res;
}

int rpmcliArgIter(rpmts ts, QVA_t qva, ARGV_t argv)
	/*@globals rpmioFtsOpts @*/
	/*@modifies rpmioFtsOpts @*/
{
    rpmRC rpmrc = RPMRC_NOTFOUND;
    int ec = 0;

JBJDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, ts, qva, argv));
    switch (qva->qva_source) {
    case RPMQV_ALL:
	qva->qva_gi = rpmgiNew(ts, RPMDBI_PACKAGES, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts, RPMGI_NONE);

	if (rpmgiGetFlags(qva->qva_gi) & RPMGI_TSADD)	/* Load the ts with headers. */
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK)
	    {};
	if (rpmrc != RPMRC_NOTFOUND)
	    return 1;	/* XXX should be no. of failures. */
	
	/*@-nullpass@*/ /* FIX: argv can be NULL, cast to pass argv array */
	ec = rpmQueryVerify(qva, ts, (const char *) argv);
	/*@=nullpass@*/
	rpmtsEmpty(ts);
	break;
    case RPMQV_RPM:
	qva->qva_gi = rpmgiNew(ts, RPMDBI_ARGLIST, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts, giFlags);

	if (rpmgiGetFlags(qva->qva_gi) & RPMGI_TSADD)	/* Load the ts with headers. */
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK)
	    {};
	if (rpmrc != RPMRC_NOTFOUND)
	    return 1;	/* XXX should be no. of failures. */
	
	/*@-nullpass@*/ /* FIX: argv can be NULL, cast to pass argv array */
	ec = rpmQueryVerify(qva, ts, NULL);
	/*@=nullpass@*/
	rpmtsEmpty(ts);
	break;
    case RPMQV_HDLIST:
	qva->qva_gi = rpmgiNew(ts, RPMDBI_HDLIST, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts, giFlags);

	if (rpmgiGetFlags(qva->qva_gi) & RPMGI_TSADD)	/* Load the ts with headers. */
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK)
	    {};
	if (rpmrc != RPMRC_NOTFOUND)
	    return 1;	/* XXX should be no. of failures. */
	
	/*@-nullpass@*/ /* FIX: argv can be NULL, cast to pass argv array */
	ec = rpmQueryVerify(qva, ts, NULL);
	/*@=nullpass@*/
	rpmtsEmpty(ts);
	break;
    case RPMQV_FTSWALK:
	if (rpmioFtsOpts == 0)
	    rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);
	qva->qva_gi = rpmgiNew(ts, RPMDBI_FTSWALK, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts, giFlags);

	if (rpmgiGetFlags(qva->qva_gi) & RPMGI_TSADD)	/* Load the ts with headers. */
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK)
	    {};
	if (rpmrc != RPMRC_NOTFOUND)
	    return 1;	/* XXX should be no. of failures. */
	
	/*@-nullpass@*/ /* FIX: argv can be NULL, cast to pass argv array */
	ec = rpmQueryVerify(qva, ts, NULL);
	/*@=nullpass@*/
	rpmtsEmpty(ts);
	break;
    default:
      if (giFlags & RPMGI_TSADD) {
	qva->qva_gi = rpmgiNew(ts, RPMTAG_NVRA, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts,
		(rpmgiFlags) (giFlags | (RPMGI_NOGLOB               )));
	if (rpmgiGetFlags(qva->qva_gi) & RPMGI_TSADD)	/* Load the ts with headers. */
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK)
	    {};
	if (rpmrc != RPMRC_NOTFOUND)
	    return 1;	/* XXX should be no. of failures. */
	qva->qva_source = RPMQV_ALL;
	/*@-nullpass@*/ /* FIX: argv can be NULL, cast to pass argv array */
	ec = rpmQueryVerify(qva, ts, NULL);
	/*@=nullpass@*/
	rpmtsEmpty(ts);
      } else {
	qva->qva_gi = rpmgiNew(ts, RPMDBI_ARGLIST, NULL, 0);
	qva->qva_rc = rpmgiSetArgs(qva->qva_gi, argv, rpmioFtsOpts,
		(rpmgiFlags) (giFlags | (RPMGI_NOGLOB|RPMGI_NOHEADER)));
	while ((rpmrc = rpmgiNext(qva->qva_gi)) == RPMRC_OK) {
	    const char * path;
	    path = rpmgiHdrPath(qva->qva_gi);
assert(path != NULL);
	    ec += rpmQueryVerify(qva, ts, path);
	    rpmtsEmpty(ts);
	}
      }
	break;
    }

    qva->qva_gi = rpmgiFree(qva->qva_gi);

JBJDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, ts, qva, argv, ec));
    return ec;
}

int rpmcliQuery(rpmts ts, QVA_t qva, const char ** argv)
{
    rpmdepFlags depFlags = qva->depFlags, odepFlags;
    rpmtransFlags transFlags = qva->transFlags, otransFlags;
    rpmVSFlags vsflags, ovsflags;
    int ec = 0;

JBJDEBUG((stderr, "--> %s(%p,%p,%p)\n", __FUNCTION__, ts, qva, argv));
    if (qva->qva_showPackage == NULL)
	qva->qva_showPackage = showQueryPackage;

    /* If --queryformat unspecified, then set default now. */
    if (qva->qva_queryFormat == NULL
     && !QVA_ISSET(qva->qva_flags, FOR_LIST)
     && !QVA_ISSET(qva->qva_flags, FOR_STATE)
     && !QVA_ISSET(qva->qva_flags, FOR_DOCS)
     && !QVA_ISSET(qva->qva_flags, FOR_CONFIG)
     && !QVA_ISSET(qva->qva_flags, FOR_DUMPFILES)
    ) {
	qva->qva_queryFormat = rpmExpand("%{?_query_all_fmt}\n", NULL);
	if (!(qva->qva_queryFormat != NULL && *qva->qva_queryFormat != '\0')) {
	    qva->qva_queryFormat = _free(qva->qva_queryFormat);
	    qva->qva_queryFormat = xstrdup("%{name}-%{version}-%{release}.%{arch}\n");
	}
    }

    vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_query}");
    vsflags = (rpmVSFlags) 0;	/* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
    if (!QVA_ISSET(qva->qva_flags, DIGEST)) {
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
    }
    if (!QVA_ISSET(qva->qva_flags, SIGNATURE)) {
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    }
    if (!QVA_ISSET(qva->qva_flags, HDRCHK)) {
	VSF_SET(vsflags, NOHDRCHK);
    }
    VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif

    odepFlags = rpmtsSetDFlags(ts, depFlags);
    otransFlags = rpmtsSetFlags(ts, transFlags);
    ovsflags = rpmtsSetVSFlags(ts, vsflags);
    ec = rpmcliArgIter(ts, qva, argv);
    vsflags = rpmtsSetVSFlags(ts, ovsflags);
    transFlags = rpmtsSetFlags(ts, otransFlags);
    depFlags = rpmtsSetDFlags(ts, odepFlags);

    if (qva->qva_showPackage == showQueryPackage)
	qva->qva_showPackage = NULL;

JBJDEBUG((stderr, "<-- %s(%p,%p,%p) rc %d\n", __FUNCTION__, ts, qva, argv, ec));
    return ec;
}
