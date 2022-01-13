#include "system.h"

#include <rpmio.h>
#include <rpmmacro.h>
#include <argv.h>
#include <mire.h>
#include <rpmcb.h>

#include <rpmtag.h>
#include <rpmdb.h>

#include <rpmgi.h>
#include <rpmcli.h>
#include <poptIO.h>

#include <rpmte.h>

#include <popt.h>

#include "debug.h"

#ifdef __cplusplus

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) |= (RPMVSF_##_FLAG))
#define VSF_CLR(_vsflags, _FLAG)	\
	(*((unsigned *)&(_vsflags)) &= ~(RPMVSF_##_FLAG))

#else	/* __cplusplus */

#define QVA_ISSET(_qvaflags, _FLAG)	((_qvaflags) & (VERIFY_##_FLAG))

#define VSF_ISSET(_vsflags, _FLAG)	((_vsflags) & (RPMVSF_##_FLAG))
#define VSF_SET(_vsflags, _FLAG)	(_vsflags) |= (RPMVSF_##_FLAG)
#define VSF_CLR(_vsflags, _FLAG)	(_vsflags) &= ~(RPMVSF_##_FLAG)

#endif	/* __cplusplus */

static const char * gitagstr = NULL;
static const char * gikeystr = NULL;
static rpmtransFlags transFlags = 0;
static rpmdepFlags depFlags = 0;

static const char * queryFormat = NULL;
static const char * defaultQueryFormat =
	"%{name}-%{version}-%{release}.%|SOURCERPM?{%{arch}.rpm}:{%|ARCH?{src.rpm}:{pubkey}|}|";

/*@only@*/ /*@null@*/
static const char * rpmgiPathOrQF(const rpmgi gi)
	/*@*/
{
    const char * fmt = ((queryFormat != NULL)
	? queryFormat : defaultQueryFormat);
    const char * val = NULL;
    Header h = rpmgiHeader(gi);

    if (h != NULL)
	val = headerSprintf(h, fmt, rpmTagTable, rpmHeaderFormats, NULL);
    else {
	const char * fn = rpmgiHdrPath(gi);
	val = (fn != NULL ? xstrdup(fn) : NULL);
    }

    return val;
}

static rpmRC rpmcliEraseElement(rpmts ts, const char * arg)
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

    return 0;
}

static const char * rpmcliInstallElementPath(rpmts ts, const char * arg)
{
    static const char * pkgpat = "-[^-]*-[^-]*.[^.]*.rpm$";
    const char * mirePattern = rpmExpand(&arg[1], pkgpat, NULL);
    miRE mire = mireNew(RPMMIRE_REGEX, 0);
    const char * fn = NULL;
    ARGV_t av = NULL;
    int ac = 0;
    int xx = mireRegcomp(mire, mirePattern);
    int i;

    /* Get list of candidate package paths. */
    /* XXX note the added "-*.rpm" to force globbing on '-' boundaries. */
    fn = rpmGetPath("%{?_rpmgi_prefix:%{?_rpmgi_prefix}/}", arg, "-*.rpm", NULL);
    xx = rpmGlob(fn, &ac, &av);
    fn = _free(fn);

    /* Filter out glibc <-> glibc-common confusions. */
    for (i = 0; i < ac; i++) {
	if (mireRegexec(mire, av[i], 0) < 0)
	    continue;
	fn = xstrdup(av[0]);
	break;
    }

    av = argvFree(av);
    mire = mireFree(mire);
    mirePattern = _free(mirePattern);

    return fn;
}

static struct poptOption optionsTable[] = {
 { "rpmgidebug", 'd', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_rpmgi_debug, -1,
	N_("debug generalized iterator"), NULL},

 { "tag", '\0', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &gitagstr, 0,
	N_("iterate tag index"), NULL },
 { "key", '\0', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &gikeystr, 0,
	N_("tag value key"), NULL },

 { "transaction", 'T', POPT_BIT_SET, &giFlags, (RPMGI_TSADD|RPMGI_TSORDER),
	N_("create transaction set"), NULL},
 { "noorder", '\0', POPT_BIT_CLR, &giFlags, RPMGI_TSORDER,
	N_("do not order transaction set"), NULL},
 { "noglob", '\0', POPT_BIT_SET, &giFlags, RPMGI_NOGLOB,
	N_("do not glob arguments"), NULL},
 { "nomanifest", '\0', POPT_BIT_SET, &giFlags, RPMGI_NOMANIFEST,
	N_("do not process non-package files as manifests"), NULL},
 { "noheader", '\0', POPT_BIT_SET, &giFlags, RPMGI_NOHEADER,
	N_("do not read headers"), NULL},

 { "qf", '\0', POPT_ARG_STRING, &queryFormat, 0,
        N_("use the following query format"), "QUERYFORMAT" },
 { "queryformat", '\0', POPT_ARG_STRING, &queryFormat, 0,
        N_("use the following query format"), "QUERYFORMAT" },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioFtsPoptTable, 0,
        N_("File tree walk options for fts(3):"),
        NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
        N_("Common options for all rpm modes and executables:"),
        NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *const argv[])
{
    poptContext optCon;
    rpmts ts = NULL;
    rpmVSFlags vsflags;
    int numRPMS = 0;
    int numFailed = 0;
    rpmgi gi = NULL;
    int gitag = RPMDBI_ARGLIST;
    const char * fn = NULL;
    ARGV_t av;
    int ac;
    int rc = 0;

    optCon = rpmcliInit(argc, argv, optionsTable);
    if (optCon == NULL)
        exit(EXIT_FAILURE);

    if (rpmioFtsOpts == 0)
	rpmioFtsOpts = (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOSTAT);

    if (gitagstr != NULL) {
	gitag = tagValue(gitagstr);
	if (gitag < 0) {
	    fprintf(stderr, _("unknown --tag argument: %s\n"), gitagstr);
	    exit(EXIT_FAILURE);
	}
    }

    av = poptGetArgs(optCon);

    /* XXX ftswalk segfault with no args. */

    ts = rpmtsCreate();
    (void) rpmtsSetFlags(ts, transFlags);
    (void) rpmtsSetDFlags(ts, depFlags);

    vsflags = (rpmVSFlags) rpmExpandNumeric("%{?_vsflags_query}");
    vsflags = (rpmVSFlags) 0;	/* XXX FIXME: ignore default disablers. */
#if defined(SUPPORT_NOSIGNATURES)
    if (!QVA_ISSET(rpmcliQueryFlags, DIGEST)) {
	VSF_SET(vsflags, NOSHA1HEADER);
	VSF_SET(vsflags, NOMD5HEADER);
	VSF_SET(vsflags, NOSHA1);
	VSF_SET(vsflags, NOMD5);
    }
    if (!QVA_ISSET(rpmcliQueryFlags, SIGNATURE)) {
	VSF_SET(vsflags, NODSAHEADER);
	VSF_SET(vsflags, NORSAHEADER);
	VSF_SET(vsflags, NODSA);
	VSF_SET(vsflags, NORSA);
    }
    if (!QVA_ISSET(rpmcliQueryFlags, HDRCHK)) {
	VSF_SET(vsflags, NOHDRCHK);
    }
    VSF_CLR(vsflags, NEEDPAYLOAD);	/* XXX needed? */
#endif

    (void) rpmtsSetVSFlags(ts, vsflags);

    gi = rpmgiNew(ts, gitag, gikeystr, 0);

    (void) rpmgiSetArgs(gi, av, rpmioFtsOpts, giFlags);

#if defined(REFERENCE_FORNOW)
if (fileURL[0] == '=') {
    rpmds this = rpmdsSingle(RPMTAG_REQUIRENAME, fileURL+1, NULL, 0);

    xx = rpmtsSolve(ts, this, NULL);
    if (ts->suggests && ts->nsuggests > 0) {
	fileURL = _free(fileURL);
	fileURL = ts->suggests[0];
	ts->suggests[0] = NULL;
	while (ts->nsuggests-- > 0) {
	    if (ts->suggests[ts->nsuggests] == NULL)
		continue;
	    ts->suggests[ts->nsuggests] = _free(ts->suggests[ts->nsuggests]);
	}
	ts->suggests = _free(ts->suggests);
	rpmlog(RPMLOG_DEBUG, D_("Adding goal: %s\n"), fileURL);
	pkgURL[pkgx] = fileURL;
	fileURL = NULL;
	pkgx++;
    }
    (void)rpmdsFree(this);
    this = NULL;
} else
#endif

    ac = 0;
    while (rpmgiNext(gi) == RPMRC_OK) {

	fn = _free(fn);
	fn = xstrdup(rpmgiHdrPath(gi));

	/* === Check for "+bing" lookaside paths within install transaction. */
	if (fn[0] == '+') {
	    const char * nfn = rpmcliInstallElementPath(ts, &fn[1]);
	    fn = _free(fn);
	    fn = nfn;
	}

	/* === Check for "-bang" erasures within install transaction. */
	if (fn[0] == '-') {
	    switch (rpmcliEraseElement(ts, &fn[1])) {
	    case RPMRC_OK:
		numRPMS++;	/* XXX multiple erasures? */
		break;
	    case RPMRC_NOTFOUND:
	    default:
		rpmlog(RPMLOG_ERR, _("package %s cannot be erased\n"), &fn[1]);
		numFailed++;	/* XXX multiple erasures? */
		break;
	    }
	    continue;
	}

	if (!(giFlags & RPMGI_TSADD)) {
	    const char * arg = rpmgiPathOrQF(gi);
	    fprintf(stdout, "%5d %s\n", ac, fn);
	    fn = _free(arg);
	}
	ac++;
    }
    fn = _free(fn);

    if (giFlags & RPMGI_TSORDER) {
	rpmtsi tsi;
	rpmte q;
	int i;
	
fprintf(stdout, "======================= %d transaction elements\n\
    # Tree Depth Degree Package\n\
=======================\n", rpmtsNElements(ts));

	i = 0;
	tsi = rpmtsiInit(ts);
	while((q = rpmtsiNext(tsi, 0)) != NULL) {
	    char deptypechar;

	    if (i == rpmtsUnorderedSuccessors(ts, -1))
		fprintf(stdout, "======================= leaf nodes only:\n");

	    deptypechar = (rpmteType(q) == TR_REMOVED ? '-' : '+');
	    fprintf(stdout, "%5d%5d%6d%7d %*s%c%s\n",
		i, rpmteTree(q), rpmteDepth(q), rpmteDegree(q),
		(2 * rpmteDepth(q)), "",
		deptypechar, rpmteNEVRA(q));
	    i++;
	}
	tsi = rpmtsiFree(tsi);
    }

    gi = rpmgiFree(gi);
    (void)rpmtsFree(ts); 
    ts = NULL;
    optCon = rpmcliFini(optCon);

    return rc;
}
