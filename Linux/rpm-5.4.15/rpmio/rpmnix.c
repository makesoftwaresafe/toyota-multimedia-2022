#include "system.h"

#define	_RPMNIX_INTERNAL
#include <rpmnix.h>
#include <rpmdir.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <ugid.h>
#include <poptIO.h>

#include "debug.h"

/*@unchecked@*/
int _rpmnix_debug = -1;

/*@unchecked@*/ /*@relnull@*/
rpmnix _rpmnixI = NULL;

/*@unchecked@*/
struct rpmnix_s _nix;		/* XXX static */

/*==============================================================*/

static int rpmnixBuildInstantiate(rpmnix nix, const char * expr, ARGV_t * drvPathsP)
	/*@*/
{
    ARGV_t argv = NULL;
    const char * argv0 = rpmGetPath(nix->binDir, "/nix-instantiate", NULL);
    const char * cmd;
    int rc = 1;		/* assume failure */
    int xx;

assert(drvPathsP);
    *drvPathsP = NULL;

argvPrint(__FUNCTION__, nix->instArgs, NULL);

    /* Construct the nix-instantiate command to run. */
    xx = argvAdd(&argv, argv0);
    argv0 = _free(argv0);

    xx = argvAdd(&argv, "--add-root");
    xx = argvAdd(&argv, nix->drvLink);
    xx = argvAdd(&argv, "--indirect");
    xx = argvAppend(&argv, nix->instArgs);
    xx = argvAdd(&argv, expr);
    cmd = argvJoin(argv, ' ');

    /* Chomp nix-instantiate spewage into *drvPathsP */
    argv0 = rpmExpand("%(", cmd, ")", NULL);
    if (argv0 && *argv0) {
	xx = argvSplit(drvPathsP, argv0, NULL);
	rc = 0;
    } else
	rc = 1;
    argv0 = _free(argv0);

    cmd = _free(cmd);
    argv = argvFree(argv);

NIXDBG((stderr, "<-- %s(%p, \"%s\", %p) rc %d\n", __FUNCTION__, nix, expr, drvPathsP, rc));
    return rc;
}

static int rpmnixBuildStore(rpmnix nix, ARGV_t drvPaths, ARGV_t * outPathsP)
	/*@*/
{
    ARGV_t argv = NULL;
    const char * argv0 = rpmGetPath(nix->binDir, "/nix-store", NULL);
    const char * cmd;
    int rc = 1;		/* assume failure */
    int xx;

assert(outPathsP);
    *outPathsP = NULL;

    /* Construct the nix-store command to run. */
    xx = argvAdd(&argv, argv0);
    argv0 = _free(argv0);

    xx = argvAdd(&argv, "--add-root");
    xx = argvAdd(&argv, nix->outLink);
    xx = argvAdd(&argv, "--indirect");
    xx = argvAdd(&argv, "-rv");
    xx = argvAppend(&argv, nix->buildArgs);
    xx = argvAppend(&argv, drvPaths);
    cmd = argvJoin(argv, ' ');

    /* Chomp nix-store spewage into *outPathsP */
    argv0 = rpmExpand("%(", cmd, ")", NULL);
    if (argv0 && *argv0) {
	xx = argvSplit(outPathsP, argv0, NULL);
	rc = 0;
    } else
	rc = 1;

    argv0 = _free(argv0);
    cmd = _free(cmd);
    argv = argvFree(argv);

NIXDBG((stderr, "<-- %s(%p, %p, %p) rc %d\n", __FUNCTION__, nix, drvPaths, outPathsP, rc));
    return rc;
}

int rpmnixBuild(rpmnix nix)
	/*@*/
{
    static const char _default_nix[] = "./default.nix";
    static const char _build_tmp[] = ".nix-build-tmp-";
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    ARGV_t drvPaths = NULL;
    int ndrvPaths = 0;
    ARGV_t outPaths = NULL;
    int noutPaths = 0;
    int nexpr = 0;
    int ec = 1;		/* assume failure */
    int xx;
    int i;

#ifdef	REFERENCE
sub intHandler {
    exit 1;
}
$SIG{'INT'} = 'intHandler';
#endif

    if (ac == 0)
	xx = argvAdd(&nix->exprs, _default_nix);
    else
	xx = argvAppend(&nix->exprs, av);

    if (nix->drvLink == NULL) {
	const char * _pre = !F_ISSET(nix, ADDDRVLINK) ? _build_tmp : "";
	nix->drvLink = rpmExpand(_pre, "derivation", NULL);
    }
    if (nix->outLink == NULL) {
	const char * _pre = !F_ISSET(nix, NOOUTLINK) ? _build_tmp : "";
	nix->outLink = rpmExpand(_pre, "result", NULL);
    }

    /* Loop over all the Nix expression arguments. */
    nexpr = argvCount(nix->exprs);
    for (i = 0; i < nexpr; i++) {
	const char * expr = nix->exprs[i];

	/* Instantiate. */
	if (rpmnixBuildInstantiate(nix, expr, &drvPaths))
	    goto exit;

	/* Verify that the derivations exist, print if verbose. */
	ndrvPaths = argvCount(drvPaths);
	for (i = 0; i < ndrvPaths; i++) {
	    const char * drvPath = drvPaths[i];
	    char target[BUFSIZ];
	    ssize_t nb = Readlink(drvPath, target, sizeof(target));
	    if (nb < 0) {
		fprintf(stderr, _("%s: cannot read symlink `%s'\n"),
			__progname, drvPath);
		goto exit;
	    }
	    target[nb] = '\0';
	    if (nix->verbose)
		fprintf(stderr, "derivation is %s\n", target);
	}

	/* Build. */
	if (rpmnixBuildStore(nix, drvPaths, &outPaths))
	    goto exit;

	if (F_ISSET(nix, DRYRUN))
	    continue;

	noutPaths = argvCount(outPaths);
	for (i = 0; i < noutPaths; i++) {
	    const char * outPath = outPaths[i];
	    char target[BUFSIZ];
	    ssize_t nb = Readlink(outPath, target, sizeof(target));
	    if (nb < 0) {
		fprintf(stderr, _("%s: cannot read symlink `%s'\n"),
			__progname, outPath);
		goto exit;
	    }
	    target[nb] = '\0';
	    fprintf(stdout, "%s\n", target);
	}

    }

    ec = 0;	/* XXX success */

exit:
    /* Clean up any generated files. */
    av = NULL;
    ac = 0;
    if (!rpmGlob(".nix-build-tmp-*", &ac, &av)) {
	for (i = 0; i < ac; i++)
	    xx = Unlink(av[i]);
	av = argvFree(av);
	ac = 0;
    }

    nix = rpmnixFree(nix);

    return ec;
}

#define	_BASE	0x40000000	/* XXX uniqify nix popt callback args */

enum {
    NIX_ADD_DRV_LINK	= _BASE + 0,	/*    --add-drv-link */
    NIX_DRV_LINK	= _BASE + 1,	/*    --drv-link NAME */
    NIX_NO_OUT_LINK	= _BASE + 2,	/*    --no-out-link */
    NIX_ATTR		= _BASE + 3,	/*    --attr ATTR */

    NIX_ARG		= _BASE + 4,	/*    --arg ARG1 ARG2 */
    NIX_LOG_TYPE	= _BASE + 5,	/*    --log-type TYPE */
    NIX_OPTION		= _BASE + 6,	/*    --option OPT1 OPT2 */

    NIX_MAX_JOBS	= _BASE + 7,	/* -j,--max-jobs JOBS */
    NIX_DRY_RUN		= _BASE + 8,	/*X   --dry-run */
    NIX_SHOW_TRACE	= _BASE + 9,	/*X   --show-trace */
    NIX_VERBOSE		= _BASE + 10,	/* -v,--verbose */

/* XXX from both nix-instatiate and nix-store: */
    NIX_ADD_ROOT	= _BASE + 20,	/*X   --add-root */
    NIX_VERSION		= _BASE + 21,	/*    --version */

/* XXX from nix-instatiate: */
    NIX_EVAL_ONLY	= _BASE + 22,	/*    --eval-only */
    NIX_PARSE_ONLY	= _BASE + 23,	/*    --parse-only */
    NIX_XML		= _BASE + 24,	/*    --xml */
    NIX_STRICT		= _BASE + 25,	/*    --strict */

/* XXX from nix-store: */
    NIX_REALISE		= _BASE + 30,	/* -r,--realise */
    NIX_ADD		= _BASE + 31,	/* -A,--add */
    NIX_DELETE		= _BASE + 32,	/*    --delete */

    NIX_READ_LOG	= _BASE + 33,	/* -l,--read-log */
    NIX_REGISTER_VALIDITY = _BASE + 34,	/*    --register-validity */
    NIX_CHECK_VALIDITY	= _BASE + 35,	/*    --check-validity */

    NIX_DUMP		= _BASE + 36,	/*    --dump */
    NIX_RESTORE		= _BASE + 37,	/*    --restore */
    NIX_EXPORT		= _BASE + 38,	/*    --export */
    NIX_IMPORT		= _BASE + 39,	/*    --import */
    NIX_VERIFY		= _BASE + 40,	/*    --verify */
    NIX_OPTIMISE	= _BASE + 41,	/*    --optimise */

    NIX_QUERY		= _BASE + 256,	/* -q,--query */
    NIX_QUERY_OUTPUTS	= _BASE + 256+1,	/*    --outputs */
    NIX_QUERY_REQUISITES = _BASE + 256+2,	/*    --requisites */
    NIX_QUERY_REFERENCES = _BASE + 256+3,	/*    --references */
    NIX_QUERY_REFERRERS = _BASE + 256+4,	/*    --referrers */
    NIX_QUERY_REFERRERS_CLOSURE = _BASE+256+5,	/*    --referrers-closure */
    NIX_QUERY_TREE	= _BASE + 256+6,	/*    --tree */
    NIX_QUERY_GRAPH	= _BASE + 256+7,	/*    --graph */
    NIX_QUERY_HASH	= _BASE + 256+8,	/*    --hash */
    NIX_QUERY_ROOTS	= _BASE + 256+9,	/*    --roots */
    NIX_QUERY_USE_OUTPUT = _BASE + 256+10,	/*    --use-output */
    NIX_QUERY_FORCE_REALISE = _BASE + 256+11,	/*    --force-realise */
    NIX_QUERY_INCLUDE_OUTPUTS = _BASE + 256+12,	/*    --include-outputs */

    NIX_GC		= _BASE + 512,	/*    --gc */
    NIX_GC_PRINT_ROOTS	= _BASE + 512+1,	/*    --print-roots */
    NIX_GC_PRINT_LIVE	= _BASE + 512+2,	/*    --print-live */
    NIX_GC_PRINT_DEAD	= _BASE + 512+3,	/*    --print-dead */
    NIX_GC_DELETE	= _BASE + 512+4,	/*    --delete */

};
#undef	_BASE

static void rpmnixBuildInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;
    const char * s = NULL;
    int xx;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_ATTR:			/*    --attr ATTR */
    case NIX_LOG_TYPE:			/*    --log-type TYPE */
	s = rpmExpand("--", opt->longName, NULL);
	xx = argvAdd(&nix->instArgs, s);
	xx = argvAdd(&nix->instArgs, arg);
	s = _free(s);
	break;
    case NIX_ARG:			/*    --arg ARG1 ARG2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->instArgs, arg1);
	xx = argvAdd(&nix->instArgs, arg2);
#endif
	break;
    case NIX_OPTION:			/*    --option OPT1 OPT2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->instArgs, opt1);
	xx = argvAdd(&nix->instArgs, opt2);
#endif
	break;
    case NIX_VERBOSE:			/* -v,--verbose */
	xx = argvAdd(&nix->instArgs, "-v");
	break;

    case NIX_SHOW_TRACE:		/*    --show-trace */

    case NIX_ADD_ROOT:			/*    --add-root */
    case NIX_VERSION:			/*    --version */

    case NIX_EVAL_ONLY:			/*    --eval-only */
    case NIX_PARSE_ONLY:		/*    --parse-only */
    case NIX_XML:			/*    --xml */
    case NIX_STRICT:			/*    --strict */
	s = rpmExpand("--", opt->longName, NULL);
	xx = argvAdd(&nix->instArgs, s);
	s = _free(s);
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

static struct poptOption rpmnixBuildInstantiateOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixBuildInstantiateArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "eval-only", '\0', POPT_ARG_NONE,			0, NIX_EVAL_ONLY,
	N_("evaluate and print resulting term; do not instantiate"), NULL },
 { "parse-only", '\0', POPT_ARG_NONE,			0, NIX_PARSE_ONLY,
	N_("parse and print abstract syntax tree"), NULL },
 { "attr", 'A', POPT_ARG_STRING,			0, NIX_ATTR,
	N_("select a specific ATTR from the Nix expression"), N_("ATTR") },
 { "add-root", '\0', POPT_ARG_NONE,			0, NIX_ADD_ROOT,
	N_("add garbage collector roots for the result"), NULL },

 { "xml", '\0', POPT_ARG_NONE,				0, NIX_XML,
	N_("print an XML representation of the abstract syntax tree"), NULL },
 { "strict", '\0', POPT_ARG_NONE,			0, NIX_STRICT,
	N_("compute attributes and list elements, rather than being lazy"), NULL },

 { "log-type", '\0', POPT_ARG_STRING,			0, NIX_LOG_TYPE,
	N_("FIXME"), N_("TYPE") },
/* XXX needs next 2 args */
 { "arg", '\0', POPT_ARG_NONE,				0, NIX_ARG,
	N_("FIXME"), N_("ARG1 ARG2") },
 { "argstr", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, 0, NIX_ARG,
	N_("FIXME"), N_("ARG1 ARG2") },
/* XXX needs next 2 args */
 { "option", '\0', POPT_ARG_NONE,			0, NIX_OPTION,
	N_("FIXME"), N_("OPT1 OPT2") },
 { "show-trace", '\0', POPT_ARG_NONE,			0, NIX_SHOW_TRACE,
	N_("FIXME"), NULL },

 { "verbose", 'v', POPT_ARG_NONE,			0, NIX_VERBOSE,
	N_("verbose operation (may be repeated)"), NULL },

 { "version", '\0', POPT_ARG_NONE,			0, NIX_VERSION,
	N_("output version information"), NULL },

  POPT_TABLEEND
};

static void rpmnixBuildStoreArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;
    const char * s = NULL;
    int xx;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_LOG_TYPE:			/*    --log-type TYPE */
    case NIX_MAX_JOBS:			/*    --max-jobs JOBS */
	s = rpmExpand("--", opt->longName, NULL);
	xx = argvAdd(&nix->buildArgs, s);
	xx = argvAdd(&nix->buildArgs, arg);
	s = _free(s);
	break;
    case NIX_OPTION:			/*    --option OPT1 OPT2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->buildArgs, opt1);
	xx = argvAdd(&nix->buildArgs, opt2);
#endif
	break;

    case NIX_VERBOSE:			/* -v,--verbose */
	xx = argvAdd(&nix->buildArgs, "-v");
	break;
    case NIX_DRY_RUN:			/*    --dry-run */
    case NIX_ADD_ROOT:			/*    --add-root */
    case NIX_VERSION:			/*    --version */
    case NIX_REALISE:			/* -r,--realise */
    case NIX_ADD:			/* -A,--add */
    case NIX_DELETE:			/*    --delete */
    case NIX_READ_LOG:			/* -l,--read-log */
    case NIX_REGISTER_VALIDITY:		/*    --register-validity */
    case NIX_CHECK_VALIDITY:		/*    --check-validity */
    case NIX_DUMP:			/*    --dump */
    case NIX_RESTORE:			/*    --restore */
    case NIX_EXPORT:			/*    --export */
    case NIX_IMPORT:			/*    --import */
    case NIX_VERIFY:			/*    --verify */
    case NIX_OPTIMISE:			/*    --optimise */
    case NIX_QUERY:			/* -q,--query */
    case NIX_QUERY_OUTPUTS:		/*    --outputs */
    case NIX_QUERY_REQUISITES:		/*    --requisites */
    case NIX_QUERY_REFERENCES:		/*    --references */
    case NIX_QUERY_REFERRERS:		/*    --referrers */
    case NIX_QUERY_REFERRERS_CLOSURE:	/*    --referrers-closure */
    case NIX_QUERY_TREE:		/*    --tree */
    case NIX_QUERY_GRAPH:		/*    --graph */
    case NIX_QUERY_HASH:		/*    --hash */
    case NIX_QUERY_ROOTS:		/*    --roots */
    case NIX_QUERY_USE_OUTPUT:		/*    --use-output */
    case NIX_QUERY_FORCE_REALISE:	/*    --force-realise */
    case NIX_QUERY_INCLUDE_OUTPUTS:	/*    --include-outputs */
    case NIX_GC:			/*    --gc */
    case NIX_GC_PRINT_ROOTS:		/*    --print-roots */
    case NIX_GC_PRINT_LIVE:		/*    --print-live */
    case NIX_GC_PRINT_DEAD:		/*    --print-dead */
    case NIX_GC_DELETE:			/*    --delete */
	s = rpmExpand("--", opt->longName, NULL);
	xx = argvAdd(&nix->buildArgs, s);
	s = _free(s);
	break;

    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

static struct poptOption rpmnixBuildStoreOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixBuildStoreArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "log-type", '\0', POPT_ARG_STRING,			0, NIX_LOG_TYPE,
	N_("FIXME"), N_("TYPE") },
/* XXX needs next 2 args */
 { "option", '\0', POPT_ARG_NONE,			0, NIX_OPTION,
	N_("FIXME"), N_("OPT1 OPT2") },
 { "max-jobs", 'j', POPT_ARG_STRING,			0, NIX_MAX_JOBS,
	N_("FIXME"), N_("JOBS") },

 { "verbose", 'v', POPT_ARG_NONE,			0, NIX_VERBOSE,
	N_("verbose operation (may be repeated)"), NULL },
 { "dry-run", '\0', POPT_ARG_NONE,			0, NIX_DRY_RUN,
	N_("FIXME"), NULL },

 { "add-root", '\0', POPT_ARG_NONE,			0, NIX_ADD_ROOT,
	N_("add garbage collector roots for the result"), NULL },
 { "version", '\0', POPT_ARG_NONE,			0, NIX_VERSION,
	N_("output version information"), NULL },

 { "realise", 'r', POPT_ARG_NONE,			0, NIX_REALISE,
	N_("ensure path validity; if a derivation, ensure the validity of the outputs"), NULL },
/* XXX conflict with -A,--attr otherwise -A,--add */
 { "add", '\0', POPT_ARG_NONE,			0, NIX_ADD,
	N_("copy a path to the Nix store"), NULL },
 { "delete", '\0', POPT_ARG_NONE,			0, NIX_DELETE,
	N_("safely delete paths from the Nix store"), NULL },
 { "read-log", 'l', POPT_ARG_NONE,			0, NIX_READ_LOG,
	N_("print build log of given store paths"), NULL },
 { "register-validity", '\0', POPT_ARG_NONE,			0, NIX_REGISTER_VALIDITY,
	N_("register path validity (dangerous!)"), NULL },
 { "check-validity", '\0', POPT_ARG_NONE,			0, NIX_CHECK_VALIDITY,
	N_("check path validity"), NULL },
 { "dump", '\0', POPT_ARG_NONE,			0, NIX_DUMP,
	N_("dump a path as a Nix archive, forgetting dependencies"), NULL },
 { "restore", '\0', POPT_ARG_NONE,			0, NIX_RESTORE,
	N_("restore a path from a Nix archive, without registering validity"), NULL },
 { "export", '\0', POPT_ARG_NONE,			0, NIX_EXPORT,
	N_("export a path as a Nix archive, marking dependencies"), NULL },
 { "import", '\0', POPT_ARG_NONE,			0, NIX_IMPORT,
	N_("import a path from a Nix archive, and register as valid"), NULL },
 { "verify", '\0', POPT_ARG_NONE,			0, NIX_VERIFY,
	N_("verify Nix structures"), NULL },
 { "optimise", '\0', POPT_ARG_NONE,			0, NIX_OPTIMISE,
	N_("optimise the Nix store by hard-linking identical files"), NULL },
 { "query", 'q', POPT_ARG_NONE,			0, NIX_QUERY,
	N_("query information"), NULL },
 { "outputs", '\0', POPT_ARG_NONE,			0, NIX_QUERY_OUTPUTS,
	N_("query the output paths of a Nix derivation (default)"), NULL },
 { "requisites", '\0', POPT_ARG_NONE,			0, NIX_QUERY_REQUISITES,
	N_("print all paths necessary to realise the path"), NULL },
 { "references", '\0', POPT_ARG_NONE,			0, NIX_QUERY_REFERENCES,
	N_("print all paths referenced by the path"), NULL },
 { "referrers", '\0', POPT_ARG_NONE,			0, NIX_QUERY_REFERRERS,
	N_("print all paths directly referring to the path"), NULL },
 { "referrers-closure", '\0', POPT_ARG_NONE,			0, NIX_QUERY_REFERRERS_CLOSURE,
	N_("print all paths (in)directly referring to the path"), NULL },
 { "tree", '\0', POPT_ARG_NONE,			0, NIX_QUERY_TREE,
	N_("print a tree showing the dependency graph of the path"), NULL },
 { "graph", '\0', POPT_ARG_NONE,			0, NIX_QUERY_GRAPH,
	N_("print a dot graph rooted at given path"), NULL },
 { "hash", '\0', POPT_ARG_NONE,			0, NIX_QUERY_HASH,
	N_("print the SHA-256 hash of the contents of the path"), NULL },
 { "roots", '\0', POPT_ARG_NONE,			0, NIX_QUERY_ROOTS,
	N_("print the garbage collector roots that point to the path"), NULL },
 { "use-output", '\0', POPT_ARG_NONE,			0, NIX_QUERY_USE_OUTPUT,
	N_("perform query on output of derivation, not derivation itself"), NULL },
 { "force-realise", '\0', POPT_ARG_NONE,			0, NIX_QUERY_FORCE_REALISE,
	N_("realise the path before performing the query"), NULL },
 { "include-outputs", '\0', POPT_ARG_NONE,			0, NIX_QUERY_INCLUDE_OUTPUTS,
	N_("in `-R' on a derivation, include requisites of outputs"), NULL },
 { "gc", '\0', POPT_ARG_NONE,			0, NIX_GC,
	N_("run the garbage collector"), NULL },
 { "print-roots", '\0', POPT_ARG_NONE,			0, NIX_GC_PRINT_ROOTS,
	N_("print GC roots and exit"), NULL },
 { "print-live", '\0', POPT_ARG_NONE,			0, NIX_GC_PRINT_LIVE,
	N_("print live paths and exit"), NULL },
 { "print-dead", '\0', POPT_ARG_NONE,			0, NIX_GC_PRINT_DEAD,
	N_("print dead paths and exit"), NULL },
/* XXX conflict with --delete */
 { "gcdelete", '\0', POPT_ARG_NONE,			0, NIX_GC_DELETE,
	N_("delete dead paths (default)"), NULL },

  POPT_TABLEEND
};

static void rpmnixBuildArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
/* XXX must be callback to continue through tables. */
    case NIX_DRY_RUN:			/*    --dry-run */
	nix->flags |= RPMNIX_FLAGS_DRYRUN;
	break;
    case NIX_VERBOSE:			/* -v,--verbose */
	nix->verbose++;
	break;

    /* XXX Collect and filter unknown options for pass through. */
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

struct poptOption _rpmnixBuildOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixBuildArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "add-drv-link", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_ADDDRVLINK,
	N_("create a symlink `derivation' to the store derivation"), NULL },
 { "no-out-link", '\0', POPT_BIT_CLR,	&_nix.flags, RPMNIX_FLAGS_NOOUTLINK,
	N_("do not create the `result' symlink"), NULL },
 { "no-link", '\0', POPT_BIT_CLR|POPT_ARGFLAG_DOC_HIDDEN,	&_nix.flags, RPMNIX_FLAGS_NOOUTLINK,
	N_("do not create the `result' symlink"), NULL },

 { "drv-link", '\0', POPT_ARG_STRING,	&_nix.drvLink, 0,
	N_("create symlink NAME instead of `derivation'"), N_("NAME") },
 { "out-link", 'o', POPT_ARG_STRING,	&_nix.outLink, 0,
	N_("create symlink NAME instead of `result'"), N_("NAME") },

/* XXX must be callback to continue through tables. */
 { "dry-run", '\0', POPT_ARG_NONE,			0, NIX_DRY_RUN,
	N_("FIXME"), NULL },
 { "verbose", 'v', POPT_ARG_NONE,			0, NIX_VERBOSE,
	N_("verbose operation (may be repeated)"), NULL },

#ifdef	NOTYET
 { "version", '\0', POPT_ARG_NONE,			0, NIX_VERSION,
	N_("output version information"), NULL },
#endif

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmnixBuildInstantiateOptions, 0,
        N_("Options passed to nix-instantiate:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmnixBuildStoreOptions, 0,
        N_("Options passed to nix-store:"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-build [OPTION]... [FILE]...\n\
\n\
`nix-build' builds the given Nix expressions (which\n\
default to ./default.nix if none are given).  A symlink called\n\
`result' is placed in the current directory.\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

/* Reads the list of channels from the file $channelsList. */
static void rpmnixReadChannels(rpmnix nix)
	/*@*/
{
    FD_t fd;
    struct stat sb;
    int xx;

NIXDBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));

#ifdef	REFERENCE
/*
    return if (!-f $channelsList);
*/
#endif
    if (nix->channelsList == NULL || Stat(nix->channelsList, &sb) < 0)
	return;

#ifdef	REFERENCE
/*
    open CHANNELS, "<$channelsList" or die "cannot open `$channelsList': $!";
    while (<CHANNELS>) {
        chomp;
        next if /^\s*\#/;
        push @channels, $_;
    }
    close CHANNELS;
*/
#endif
    fd = Fopen(nix->channelsList, "r.fpio");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"r\") failed.\n", nix->channelsList);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    /* XXX skip comments todo++ */
    nix->channels = argvFree(nix->channels);
    xx = argvFgets(&nix->channels, fd);
    xx = Fclose(fd);
}

/* Writes the list of channels to the file $channelsList */
static void rpmnixWriteChannels(rpmnix nix)
	/*@*/
{
    FD_t fd;
    int ac = argvCount(nix->channels);
    ssize_t nw;
    int xx;
    int i;

NIXDBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));
    if (Access(nix->channelsList, W_OK)) {
	fprintf(stderr, "file %s is not writable.\n", nix->channelsList);
	return;
    }
    fd = Fopen(nix->channelsList, "w");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"w\") failed.\n", nix->channelsList);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    for (i = 0; i < ac; i++) {
	const char * url = nix->channels[i];
	nw = Fwrite(url, 1, strlen(url), fd);
	nw = Fwrite("\n", 1, sizeof("\n")-1, fd);
    }
    xx = Fclose(fd);
}

/* Adds a channel to the file $channelsList */
static void rpmnixAddChannel(rpmnix nix, const char * url)
	/*@*/
{
    int ac;
    int xx;
    int i;

NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, url));
    rpmnixReadChannels(nix);
    ac = argvCount(nix->channels);
    for (i = 0; i < ac; i++) {
	if (!strcmp(nix->channels[i], url))
	    return;
    }
    xx = argvAdd(&nix->channels, url);
    rpmnixWriteChannels(nix);
}

/* Remove a channel from the file $channelsList */
static void rpmnixRemoveChannel(rpmnix nix, const char * url)
	/*@*/
{
    const char ** nchannels = NULL;
    int ac;
    int xx;
    int i;

NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, url));
    rpmnixReadChannels(nix);
    ac = argvCount(nix->channels);
    for (i = 0; i < ac; i++) {
	if (!strcmp(nix->channels[i], url))
	    continue;
	xx = argvAdd(&nchannels, nix->channels[i]);
    }
    nix->channels = argvFree(nix->channels);
    nix->channels = nchannels;
    rpmnixWriteChannels(nix);
}

/*
 * Fetch Nix expressions and pull cache manifests from the subscribed
 * channels.
 */
static void rpmnixUpdateChannels(rpmnix nix)
	/*@*/
{
    uid_t uid = getuid();
    struct stat sb;
    const char * userName = uidToUname(uid);
    const char * rootFile;
    const char * outPath;
    const char * rval;
    const char * cmd;
    const char * fn;

const char * inputs = "[]";	/* XXX FIXME */
    int xx;

NIXDBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));

    rpmnixReadChannels(nix);

    /* Create the manifests directory if it doesn't exist. */
    xx = rpmioMkpath(nix->manifestsPath, (mode_t)0755, (uid_t)-1, (gid_t)-1);

    /*
     * Do we have write permission to the manifests directory?  If not,
     * then just skip pulling the manifest and just download the Nix
     * expressions.  If the user is a non-privileged user in a
     * multi-user Nix installation, he at least gets installation from
     * source.
     */
    if (!Access(nix->manifestsPath, W_OK)) {
	int ac = argvCount(nix->channels);
	int i;

	/* Pull cache manifests. */
#ifdef	REFERENCE
/*
        foreach my $url (@channels) {
            #print "pulling cache manifest from `$url'\n";
            system("/usr/bin/nix-pull", "--skip-wrong-store", "$url/MANIFEST") == 0
                or die "cannot pull cache manifest from `$url'";
        }

*/
#endif
	for (i = 0; i < ac; i++) {
	    const char * url = nix->channels[i];
            cmd = rpmExpand(nix->binDir, "/nix-pull --skip-wrong-store ",
			url, "/MANIFEST", "; echo $?", NULL);
	    rval = rpmExpand("%(", cmd, ")", NULL);
	    if (strcmp(rval, "0")) {
		fprintf(stderr, "cannot pull cache manifest from `%s'\n", url);
		exit(1);
	    }
	    rval = _free(rval);
	    cmd = _freeCmd(cmd);
	}
    }

    /*
     * Create a Nix expression that fetches and unpacks the channel Nix
     * expressions.
     */
#ifdef	REFERENCE
/*
    my $inputs = "[";
    foreach my $url (@channels) {
        $url =~ /\/([^\/]+)\/?$/;
        my $channelName = $1;
        $channelName = "unnamed" unless defined $channelName;

        my $fullURL = "$url/nixexprs.tar.bz2";
        print "downloading Nix expressions from `$fullURL'...\n";
        $ENV{"PRINT_PATH"} = 1;
        $ENV{"QUIET"} = 1;
        my ($hash, $path) = `/usr/bin/nix-prefetch-url '$fullURL'`;
        die "cannot fetch `$fullURL'" if $? != 0;
        chomp $path;
        $inputs .= '"' . $channelName . '"' . " " . $path . " ";
    }
    $inputs .= "]";
*/
#endif

    /* Figure out a name for the GC root. */
    rootFile = rpmGetPath(nix->rootsPath,
			"/per-user/", userName, "/channels", NULL);
    
    /* Build the Nix expression. */
    fprintf(stdout, "unpacking channel Nix expressions...\n");
#ifdef	REFERENCE
/*
    my $outPath = `\\
        /usr/bin/nix-build --out-link '$rootFile' --drv-link '$rootFile'.tmp \\
        /usr/share/nix/corepkgs/channels/unpack.nix \\
        --argstr system i686-linux --arg inputs '$inputs'`
        or die "cannot unpack the channels";
    chomp $outPath;
*/
#endif
    fn = rpmGetPath(rootFile, ".tmp", NULL);
    cmd = rpmExpand(nix->binDir, "/nix-build --out-link '", rootFile, "'",
		" --drv-link '", fn, "'", 
		"/usr/share/nix/corepkgs/channels/unpack.nix --argstr system i686-linux --arg inputs '", inputs, "'", NULL);
    outPath = rpmExpand("%(", cmd, ")", NULL);
    cmd = _freeCmd(cmd);

    xx = Unlink(fn);
    fn = _free(fn);

    /* Make the channels appear in nix-env. */
#ifdef	REFERENCE
/*
    unlink $nixDefExpr if -l $nixDefExpr; # old-skool ~/.nix-defexpr
    mkdir $nixDefExpr or die "cannot create directory `$nixDefExpr'" if !-e $nixDefExpr;
    my $channelLink = "$nixDefExpr/channels";
    unlink $channelLink; # !!! not atomic
    symlink($outPath, $channelLink) or die "cannot symlink `$channelLink' to `$outPath'";
*/
#endif
    if (Lstat(nix->nixDefExpr, &sb) == 0 && S_ISLNK(sb.st_mode))
	xx = Unlink(nix->nixDefExpr);
    if (Lstat(nix->nixDefExpr, &sb) < 0 && errno == ENOENT) {
	mode_t dmode = 0755;
	if (Mkdir(nix->nixDefExpr, dmode)) {
	    fprintf(stderr, "Mkdir(%s, 0%o) failed\n", nix->nixDefExpr, dmode);
	    exit(1);
	}
    }
    fn = rpmGetPath(nix->nixDefExpr, "/channels", NULL);
    xx = Unlink(fn);	/* !!! not atomic */
    if (Symlink(outPath, fn)) {
	fprintf(stderr, "Symlink(%s, %s) failed\n", outPath, fn);
	exit(1);
    }
    fn = _free(fn);

    rootFile = _free(rootFile);
}

int rpmnixChannel(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 1;		/* assume failure */
    int xx;

    /* Turn on caching in nix-prefetch-url. */
    nix->channelCache = rpmGetPath(nix->stateDir, "/channel-cache", NULL);
    xx = rpmioMkpath(nix->channelCache, 0755, (uid_t)-1, (gid_t)-1);
    if (!Access(nix->channelCache, W_OK))
	xx = setenv("NIX_DOWNLOAD_CACHE", nix->channelCache, 0);

    /* Figure out the name of the `.nix-channels' file to use. */
    nix->channelsList = rpmGetPath(nix->homeDir, "/.nix-channels", NULL);
    nix->nixDefExpr = rpmGetPath(nix->homeDir, "/.nix-defexpr", NULL);

    if (nix->op == 0 || (av && av[0] != NULL) || ac != 0) {
	poptPrintUsage(nix->con, stderr, 0);
	goto exit;
    }

    switch (nix->op) {
    case NIX_CHANNEL_ADD:
assert(nix->url != NULL);	/* XXX proper exit */
	rpmnixAddChannel(nix, nix->url);
	break;
    case NIX_CHANNEL_REMOVE:
assert(nix->url != NULL);	/* XXX proper exit */
	rpmnixRemoveChannel(nix, nix->url);
	break;
    case NIX_CHANNEL_LIST:
	rpmnixReadChannels(nix);
	argvPrint(nix->channelsList, nix->channels, NULL);
	break;
    case NIX_CHANNEL_UPDATE:
	rpmnixUpdateChannels(nix);
	break;
    }

    ec = 0;	/* XXX success */

exit:

    return ec;
}

static void _rpmnixChannelArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_CHANNEL_ADD:
    case NIX_CHANNEL_REMOVE:
	nix->url = xstrdup(arg);
	/*@fallthrough@*/
    case NIX_CHANNEL_LIST:
    case NIX_CHANNEL_UPDATE:
	nix->op = opt->val;
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

struct poptOption _rpmnixChannelOptions[] = {
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	_rpmnixChannelArgCallback, 0, NULL, NULL },

 { "add", '\0', POPT_ARG_STRING,		0, NIX_CHANNEL_ADD,
        N_("subscribe to a Nix channel"), N_("URL") },
 { "remove", '\0', POPT_ARG_STRING,		0, NIX_CHANNEL_REMOVE,
        N_("unsubscribe from a Nix channel"), N_("URL") },
 { "list", '\0', POPT_ARG_NONE,			0, NIX_CHANNEL_LIST,
        N_("list subscribed channels"), NULL },
 { "update", '\0', POPT_ARG_NONE,		0, NIX_CHANNEL_UPDATE,
        N_("download latest Nix expressions"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

#ifdef	DYING
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage:\n\
  nix-channel --add URL\n\
  nix-channel --remove URL\n\
  nix-channel --list\n\
  nix-channel --update\n\
"), NULL },
#endif

  POPT_TABLEEND
};

/*==============================================================*/

/*
 * If `-d' was specified, remove all old generations of all profiles.
 * Of course, this makes rollbacks to before this point in time
 * impossible.
 */
static int rpmnixRemoveOldGenerations(rpmnix nix, const char * dn)
	/*@*/
{
    DIR * dir;
    struct dirent * dp;
    struct stat sb;
    int xx;

#ifdef	REFERENCE
/*
    my $dh;
    opendir $dh, $dir or die;

    foreach my $name (sort (readdir $dh)) {
        next if $name eq "." || $name eq "..";
        $name = $dir . "/" . $name;
        if (-l $name && (readlink($name) =~ /link/)) {
            print STDERR "removing old generations of profile $name\n";
            system("$binDir/nix-env", "-p", $name, "--delete-generations", "old");
        }
        elsif (! -l $name && -d $name) {
            rpmnixRemoveOldGenerations $name;
        }
    }
    
    closedir $dh or die;
*/
#endif
    dir = Opendir(dn);
    if (dn == NULL) {
	fprintf(stderr, "Opendir(%s) failed\n", dn);
	exit(1);
    }
    while ((dp = Readdir(dir)) != NULL) {
	const char * fn;
	if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
	    continue;
	fn = rpmGetPath(dn, "/", dp->d_name, NULL);
	if (Lstat(fn, &sb) < 0) {
	    fn = _free(fn);
	    continue;
	}
	/* Recurse on sub-directories. */
	if (S_ISDIR(sb.st_mode)) {
	    xx = rpmnixRemoveOldGenerations(nix, fn);
	    fn = _free(fn);
	    continue;
	}
	if (S_ISLNK(sb.st_mode)) {
	    char buf[BUFSIZ];
	    ssize_t nr = Readlink(fn, buf, sizeof(buf));
	    if (nr >= 0)
		buf[nr] = '\0';
	    if (!strcmp(buf, "link")) {	/* XXX correct test? */
		const char * cmd;
		const char * rval;

		fprintf(stderr, "removing old generations of profile %s\n", fn);
		cmd = rpmExpand(nix->binDir, "/nix-env -p ", fn,
				" --delete-generations old", NULL);
		rval = rpmExpand("%(", cmd, ")", NULL);
		rval = _free(rval);
		cmd = _freeCmd(cmd);
	    }
	    fn = _free(fn);
	    continue;
	}
	fn = _free(fn);
    }
    xx = Closedir(dir);

    return 0;
}

int
rpmnixCollectGarbage(rpmnix nix)
{
    ARGV_t av = rpmnixArgv(nix, NULL);
    int ec = 1;		/* assume failure */
    const char * rval;
    const char * cmd;
    int xx;

    if (F_ISSET(nix, DELETEOLD))
	xx = rpmnixRemoveOldGenerations(nix, nix->profilesPath);

#ifdef	REFERENCE
/*
# Run the actual garbage collector.
exec "$binDir/nix-store", "--gc", @args;
*/
#endif
    rval = argvJoin(av, ' ');
    cmd = rpmExpand(nix->binDir, "/nix-store --gc ", rval, "; echo $?", NULL);
    rval = _free(rval);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (!strcmp(rval, "0"))
	ec = 0;	/* XXX success */
    rval = _free(rval);
    cmd = _freeCmd(cmd);

    return ec;
}

struct poptOption _rpmnixCollectGarbageOptions[] = {

 { "delete-old", 'd', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_DELETEOLD,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP
  POPT_TABLEEND
};

/*==============================================================*/

int
rpmnixCopyClosure(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 1;		/* assume failure */
    const char * s;
    const char * cmd;
    const char * rval;
    const char * sshOpts = "";			/* XXX HACK */
    const char * compressor = "";
    const char * decompressor = "";
    const char * extraOpts = "";
    int nac;
    int xx;

    if (av == NULL || av[0] == NULL || ac < 1) {
	poptPrintUsage(nix->con, stderr, 0);
	goto exit;
    }

    if (nix->op == 0)
	nix->op = NIX_COPY_CLOSURE_TO_HOST;

    xx = argvAppend(&nix->storePaths, av);
    if (F_ISSET(nix, GZIP)) {
	compressor = "| gzip";
	decompressor = "gunzip |";
    }
    if (F_ISSET(nix, SIGN)) {
	extraOpts = " --sign";
    }

#ifdef	REFERENCE
/*
openSSHConnection $sshHost or die "$0: unable to start SSH\n";
*/
#endif


    switch (nix->op) {
    case NIX_COPY_CLOSURE_TO_HOST:	/* Copy TO the remote machine. */

	/* Get the closure of this path. */
#ifdef	REFERENCE
/*
    my $pid = open(READ, "$binDir/nix-store --query --requisites @storePaths|") or die;
    
    while (<READ>) {
        chomp;
        die "bad: $_" unless /^\//;
        push @allStorePaths, $_;
    }

    close READ or die "nix-store failed: $?";
*/
#endif
	s = argvJoin(nix->storePaths, ' ');
        cmd = rpmExpand(nix->binDir, "/nix-store --query --requisites ", s, NULL);
	s = _free(s);
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->allStorePaths, rval, NULL);
        rval = _free(rval);
        cmd = _freeCmd(cmd);

	/* Ask the remote host which paths are invalid. */
#ifdef	REFERENCE
/*
    open(READ, "ssh $sshHost @sshOpts nix-store --check-validity --print-invalid @allStorePaths|");
    my @missing = ();
    while (<READ>) {
        chomp;
        push @missing, $_;
    }
    close READ or die;
*/
#endif
	s = argvJoin(nix->allStorePaths, ' ');
        cmd = rpmExpand("ssh ", nix->sshHost, " ", sshOpts, " nix-store --check-validity --print-invalid ", s, NULL);
	s = _free(s);
#ifdef	NOTYET
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
#else
nix->missing = NULL;
fprintf(stderr, "<-- missing assumed NULL\n");
#endif
        cmd = _freeCmd(cmd);

	/* Export the store paths and import them on the remote machine. */
	nac = argvCount(nix->missing);
	if (nac > 0) {
argvPrint("copying these missing paths:", nix->missing, NULL);
#ifdef	REFERENCE
/*
        my $extraOpts = "";
        $extraOpts .= "--sign" if $sign == 1;
        system("nix-store --export $extraOpts @missing $compressor | ssh $sshHost @sshOpts '$decompressor nix-store --import'") == 0
            or die "copying store paths to remote machine `$sshHost' failed: $?";
*/
#endif
	    s = argvJoin(nix->missing, ' ');
	    cmd = rpmExpand(nix->binDir, "/nix-store --export ", extraOpts, " ", s, " ", compressor,
		" | ssh ", nix->sshHost, " ", sshOpts, " '", decompressor, " nix-store --import'", NULL);
	    s = _free(s);
	    cmd = _freeCmd(cmd);
	}
	break;
    case NIX_COPY_CLOSURE_FROM_HOST:	/* Copy FROM the remote machine. */

	/*
	 * Query the closure of the given store paths on the remote
	 * machine.  Paths are assumed to be store paths; there is no
	 * resolution (following of symlinks).
	 */
#ifdef	REFERENCE
/*
    my $pid = open(READ,
        "ssh @sshOpts $sshHost nix-store --query --requisites @storePaths|") or die;
    
    my @allStorePaths;

    while (<READ>) {
        chomp;
        die "bad: $_" unless /^\//;
        push @allStorePaths, $_;
    }

    close READ or die "nix-store on remote machine `$sshHost' failed: $?";
*/
#endif
	s = argvJoin(nix->storePaths, ' ');
        cmd = rpmExpand("ssh ", sshOpts, " ", nix->sshHost, " nix-store --query --requisites ", s, NULL);
	s = _free(s);
#ifdef	NOTYET
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
#else
nix->allStorePaths = NULL;
fprintf(stderr, "<-- allStorePaths assumed NULL\n");
#endif
	cmd = _freeCmd(cmd);

	/* What paths are already valid locally? */
#ifdef	REFERENCE
/*
    open(READ, "/usr/bin/nix-store --check-validity --print-invalid @allStorePaths|");
    my @missing = ();
    while (<READ>) {
        chomp;
        push @missing, $_;
    }
    close READ or die;
*/
#endif
	s = argvJoin(nix->allStorePaths, ' ');
        cmd = rpmExpand(nix->binDir, "/nix-store --check-validity --print-invalid ", s, NULL);
	s = _free(s);
        rval = rpmExpand("%(", cmd, ")", NULL);
        xx = argvSplit(&nix->missing, rval, NULL);
        rval = _free(rval);
	cmd = _freeCmd(cmd);

	/* Export the store paths on the remote machine and import them on locally. */
	nac = argvCount(nix->missing);
	if (nac > 0) {
argvPrint("copying these missing paths:", nix->missing, NULL);
#ifdef	REFERENCE
/*
        system("ssh $sshHost @sshOpts 'nix-store --export $extraOpts @missing $compressor' | $decompressor /usr/bin/nix-store --import") == 0
            or die "copying store paths from remote machine `$sshHost' failed: $?";
*/
#endif
	    s = argvJoin(nix->missing, ' ');
	    cmd = rpmExpand("ssh ", nix->sshHost, " ", sshOpts,
		" 'nix-store --export ", extraOpts, " ", s, " ", compressor,
		"' | ", decompressor, " ", nix->binDir, "/nix-store --import", NULL);
	    s = _free(s);
	    cmd = _freeCmd(cmd);
	}
	break;
    }

    ec = 0;	/* XXX success */

exit:

    return ec;
}

static void rpmnixCopyClosureArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_COPY_CLOSURE_FROM_HOST:
	nix->op = opt->val;
	nix->sshHost = xstrdup(arg);
	break;
    case NIX_COPY_CLOSURE_TO_HOST:
	nix->op = opt->val;
	nix->sshHost = xstrdup(arg);
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

struct poptOption _rpmnixCopyClosureOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixCopyClosureArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "from", '\0', POPT_ARG_STRING,	0, NIX_COPY_CLOSURE_FROM_HOST,
	N_("FIXME"), NULL },
 { "to", '\0', POPT_ARG_STRING,		0, NIX_COPY_CLOSURE_TO_HOST,
	N_("FIXME"), NULL },
 { "gzip", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_GZIP,
	N_("FIXME"), NULL },
 { "sign", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_SIGN,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-copy-closure [--from | --to] HOSTNAME [--sign] [--gzip] PATHS...\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

#ifdef	REFERENCE
/*
sub barf {
    my $msg = shift;
    print "$msg\n";
    <STDIN> if $interactive;
    exit 1;
}
*/
#endif

int
rpmnixInstallPackage(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 1;		/* assume failure */
    const char * rval;
    const char * cmd;
    const char * s;
    int xx;

#ifdef	NOTYET
const char * version	= "?version?";
const char * system	= "?system?";
const char * drvPath	= "?drvPath?";
const char * pkgFile	= "?pkgFile?";
#endif

const char * manifestURL= "?manifestURL?";
const char * drvName	= "?drvName?";
const char * outPath	= "?outPath?";

    const char * source;

const char ** extraNixEnvArgs = NULL;

    /* Parse the command line arguments. */
#ifdef	REFERENCE
/*
my @args = @ARGV;
usageError if scalar @args == 0;

my $source;
my $fromURL = 0;
my @extraNixEnvArgs = ();
my $interactive = 1;

while (scalar @args) {
    my $arg = shift @args;
    if ($arg eq "--help") {
        usageError;
    }
    elsif ($arg eq "--url") {
        $fromURL = 1;
    }
    elsif ($arg eq "--profile" || $arg eq "-p") {
        my $profile = shift @args;
        usageError if !defined $profile;
        push @extraNixEnvArgs, "-p", $profile;
    }
    elsif ($arg eq "--non-interactive") {
        $interactive = 0;
    }
    else {
        $source = $arg;
    }
}

usageError unless defined $source;

*/
#endif
    if (ac != 1) {
	poptPrintUsage(nix->con, stderr, 0);
	goto exit;
    }
    source = av[0];

    /*
     * Re-execute in a terminal, if necessary, so that if we're executed
     * from a web browser, the user gets to see us.
     */
    if (F_ISSET(nix, INTERACTIVE) && (s=getenv("NIX_HAVE_TERMINAL")) == NULL) {
	xx = setenv("NIX_HAVE_TERMINAL", "1", 1);
	xx = setenv("LD_LIBRARY_PATH", "", 1);

#ifdef	REFERENCE
/*
    foreach my $term ("xterm", "konsole", "gnome-terminal", "xterm") {
        exec($term, "-e", "$binDir/nix-install-package", @ARGV);
    }
    die "cannot execute `xterm'";
*/
#endif
    }


#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-install-package.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif
    nix->tmpPath = mkdtemp(rpmGetPath(nix->tmpDir, "/nix-pull.XXXXXX", NULL));
    if (nix->tmpPath == NULL) {
	fprintf(stderr, _("cannot create a temporary directory\n"));
	goto exit;
    }

    /* Download the package description, if necessary. */
#ifdef	REFERENCE
/*
my $pkgFile = $source;
if ($fromURL) {
    $pkgFile = "$tmpDir/tmp.nixpkg";
    system("/usr/bin/curl", "--silent", $source, "-o", $pkgFile) == 0
        or barf "curl failed: $?";
}
*/
#endif


    /* Read and parse the package file. */
#ifdef	REFERENCE
/*
open PKGFILE, "<$pkgFile" or barf "cannot open `$pkgFile': $!";
my $contents = <PKGFILE>;
close PKGFILE;
*/
#endif

#ifdef	REFERENCE
/*
my $urlRE = "(?: [a-zA-Z][a-zA-Z0-9\+\-\.]*\:[a-zA-Z0-9\%\/\?\:\@\&\=\+\$\,\-\_\.\!\~\*\']+ )";
my $nameRE = "(?: [A-Za-z0-9\+\-\.\_\?\=]+ )"; # see checkStoreName()
my $systemRE = "(?: [A-Za-z0-9\+\-\_]+ )";
my $pathRE = "(?: \/ [\/A-Za-z0-9\+\-\.\_\?\=]* )";

# Note: $pathRE doesn't check that whether we're looking at a valid
# store path.  We'll let nix-env do that.

$contents =~
    / ^ \s* (\S+) \s+ ($urlRE) \s+ ($nameRE) \s+ ($systemRE) \s+ ($pathRE) \s+ ($pathRE) /x
    or barf "invalid package contents";
my $version = $1;
my $manifestURL = $2;
my $drvName = $3;
my $system = $4;
my $drvPath = $5;
my $outPath = $6;
*/
#endif

#ifdef	REFERENCE
/*
barf "invalid package version `$version'" unless $version eq "NIXPKG1";
*/
#endif


    if (F_ISSET(nix, INTERACTIVE)) {
	/* Ask confirmation. */
#ifdef	REFERENCE
/*
    print "Do you want to install `$drvName' (Y/N)? ";
    my $reply = <STDIN>;
    chomp $reply;
    exit if $reply ne "y" && $reply ne "Y";
*/
#endif
    }


    /*
     * Store the manifest in the temporary directory so that we don't
     * pollute /nix/var/nix/manifests.
     */
    xx = setenv("NIX_MANIFESTS_DIR", nix->tmpPath, 1);


    fprintf(stdout, "\nPulling manifests...\n");
#ifdef	REFERENCE
/*
system("$binDir/nix-pull", $manifestURL) == 0
    or barf "nix-pull failed: $?";
*/
#endif
    cmd = rpmExpand(nix->binDir, "/nix-pull ", manifestURL, "; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "nix-pull failed: %s\n", rval);
	goto exit;
    }
    rval = _free(rval);
    cmd = _freeCmd(cmd);


    fprintf(stdout, "\nInstalling package...\n");
#ifdef	REFERENCE
/*
system("$binDir/nix-env", "--install", $outPath, "--force-name", $drvName, @extraNixEnvArgs) == 0
    or barf "nix-env failed: $?";
*/
#endif
    s = argvJoin(extraNixEnvArgs, ' ');
    cmd = rpmExpand(nix->binDir, "/nix-env --install ", outPath,
			" --force-name ", drvName, " ", s, "; echo $?", NULL);
    s = _free(s);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "nix-env failed: %s\n", rval);
	goto exit;
    }
    rval = _free(rval);
    cmd = _freeCmd(cmd);


    if (F_ISSET(nix, INTERACTIVE)) {
#ifdef	REFERENCE
/*
    print "\nInstallation succeeded! Press Enter to continue.\n";
    <STDIN>;
*/
#endif
    }

    ec = 0;	/* XXX success */

exit:

#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-install-package.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif

    return ec;
}

struct poptOption _rpmnixInstallPackageOptions[] = {

 { "profile", 'p', POPT_ARG_ARGV,	&_nix.profiles, 0,
	N_("FIXME"), NULL },
 { "non-interactive", '\0', POPT_BIT_CLR, &_nix.flags, RPMNIX_FLAGS_INTERACTIVE,
	N_("FIXME"), NULL },
 { "url", '\0', POPT_ARG_STRING,	&_nix.url, 0,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-install-package (FILE | --url URL)\n\
\n\
Install a Nix Package (.nixpkg) either directly from FILE or by\n\
downloading it from URL.\n\
\n\
Flags:\n\
  --profile / -p LINK: install into the specified profile\n\
  --non-interactive: don't run inside a new terminal\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

/**
 * Copy source string to target, substituting for URL characters.
 * @param t		target xml string
 * @param s		source string
 * @return		target xml string
 */
static char * urlStrcpy(/*@returned@*/ char * t, const char * s)
	/*@modifies t @*/
{
    char * te = t;
    int c;

/*
 * Handle escaped characters in the URI.  `+', `=' and `?' are the only
 * characters that are valid in Nix store path names but have a special
 * meaning in URIs.
 */
    while ((c = (int) *s++) != (int) '\0') {
	switch (c) {
	case '%':
	    if (s[0] == '2' && s[1] == 'b')	c = (int)'+';
	    if (s[0] == '3' && s[1] == 'd')	c = (int)'=';
	    if (s[0] == '3' && s[1] == 'f')	c = (int)'?';
	    if (c != (int)s[-1])
		s += 2;
	    break;
	}
	*te++ = (char) c;
    }
    *te = '\0';
    return t;
}

static void rpmnixMakeTmpPath(rpmnix nix)
	/*@*/
{
    if (nix->tmpPath == NULL) {
	nix->tmpPath =
		mkdtemp(rpmGetPath(nix->tmpDir, "/nix-prefetch-url-XXXXXX",NULL));
assert(nix->tmpPath != NULL);
    }

NIXDBG((stderr, "<-- %s(%p) tmpPath %s\n", __FUNCTION__, nix, nix->tmpPath));

}

static void rpmnixRemoveTmpPath(rpmnix nix)
	/*@*/
{
    const char * cmd;
    const char * rval;
    struct stat sb;

    if (nix->tmpPath == NULL) return;
    if (Stat(nix->tmpPath, &sb) < 0) return;

    cmd = rpmExpand("/bin/rm -rf '", nix->tmpPath, "'; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "failed to remove %s\n", nix->tmpPath);
    }
    rval = _free(rval);

NIXDBG((stderr, "<-- %s(%p)\n", __FUNCTION__, nix));

    cmd = _freeCmd(cmd);
}

static void rpmnixDoDownload(rpmnix nix)
	/*@*/
{
    const char * cmd;
    const char * rval;

    cmd = rpmExpand("/usr/bin/curl ", nix->cacheFlags,
	" --fail --location --max-redirs 20 --disable-epsv",
	" --cookie-jar ", nix->tmpPath, "/cookies ", nix->url,
	" -o ", nix->tmpFile, NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    rval = _free(rval);

NIXDBG((stderr, "<-- %s(%p)\n", __FUNCTION__, nix));

    cmd = _freeCmd(cmd);
}

int
rpmnixPrefetchURL(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 1;		/* assume failure */
    const char * s;
    char * fn;
    char * cmd;
    struct stat sb;
    FD_t fd;
    int xx;

#ifdef	REFERENCE
/*
    trap rpmnixRemoveTmpPath EXIT SIGINT SIGQUIT
*/
#endif

    switch (ac) {
    default:	poptPrintUsage(nix->con, stderr, 0);	goto exit;
	/*@notreached@*/
    case 2:	nix->expHash = av[1];	/*@fallthrough@*/
    case 1:	nix->url = av[0];	break;
    }

    nix->hashFormat = (strcmp(nix->hashAlgo, "md5") ? "--base32" : "");

nix->cacheFlags = xstrdup("");

    /*
     * Handle escaped characters in the URI.  `+', `=' and `?' are the only
     * characters that are valid in Nix store path names but have a special
     * meaning in URIs.
     */
    fn = xstrdup(nix->url);
    s = basename(fn);
    if (s == NULL || *s == '\0') {
	fprintf(stderr, _("invalid url: %s\n"), nix->url);
	goto exit;
    }
    nix->name = urlStrcpy(xstrdup(s), s);
    fn = _free(fn);

    /* If expected hash specified, check if it already exists in the store. */
    if (nix->expHash != NULL) {

	cmd = rpmExpand(nix->binDir, "/nix-store --print-fixed-path ",
		nix->hashAlgo, " ", nix->expHash, " ", nix->name, NULL);
	nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);

	cmd = rpmExpand(nix->binDir, "/nix-store --check-validity ",
		nix->finalPath, " 2>/dev/null; echo $?", NULL);
	s = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);
	if (strcmp(s, "0"))
	    nix->finalPath = _free(nix->finalPath);
	s = _free(s);
	nix->hash = xstrdup(nix->expHash);
    }

    /* Hack to support the mirror:// scheme from Nixpkgs. */
    if (!strncmp(nix->url, "mirror://", sizeof("mirror://")-1)) {
	ARGV_t expanded = NULL;
	if ((s = getenv("NIXPKGS_ALL")) || *s == '\0') {
            fprintf(stderr, _("Resolving mirror:// URLs requires Nixpkgs.  Please point $NIXPKGS_ALL at a Nixpkgs tree.\n"));
	    goto exit;
	}
	nix->nixPkgs = s;

	rpmnixMakeTmpPath(nix);

	cmd = rpmExpand(nix->binDir, "/nix-build ", nix->nixPkgs,
		" -A resolveMirrorURLs --argstr url ", nix->url,
		" -o ", nix->tmpPath, "/urls > /dev/null", NULL);
	s = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);
	s = _free(s);

	fn = rpmGetPath(nix->tmpPath, "/urls", NULL);
	fd = Fopen(fn, "r.fpio");
	if (fd == NULL || Ferror(fd)) {
	    fprintf(stderr, _("Fopen(%s, \"r\") failed\n"), fn);
	    if (fd) xx = Fclose(fd);
	    fn = _free(fn);
	    goto exit;
	}
	xx = argvFgets(&expanded, fd);
	xx = Fclose(fd);

	fn = _free(fn);

	if (argvCount(expanded) == 0) {
	    fprintf(stderr, _("%s: cannot resolve %s\n"), __progname, nix->url);
	    expanded = argvFree(expanded);
	    goto exit;
	}

	fprintf(stderr, _("url %s replaced with %s\n"), nix->url, expanded[0]);
	nix->url = xstrdup(expanded[0]);
	expanded = argvFree(expanded);
    }

    /*
     * If we don't know the hash or a file with that hash doesn't exist,
     * download the file and add it to the store.
     */
    if (nix->finalPath == NULL) {

	rpmnixMakeTmpPath(nix);

	nix->tmpFile = rpmGetPath(nix->tmpPath, "/", nix->name, NULL);

	/*
	 * Optionally do timestamp-based caching of the download.
	 * Actually, the only thing that we cache in $NIX_DOWNLOAD_CACHE is
	 * the hash and the timestamp of the file at $url.  The caching of
	 * the file *contents* is done in Nix store, where it can be
	 * garbage-collected independently.
	 */
	if (nix->downloadCache) {

	    fn = rpmGetPath(nix->tmpPath, "/url", NULL);
	    fd = Fopen(fn, "w");
	    if (fd == NULL || Ferror(fd)) {
		fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		if (fd) xx = Fclose(fd);
		fn = _free(fn);
		goto exit;
	    }
	    xx = Fwrite(nix->url, 1, strlen(nix->url), fd);
	    xx = Fclose(fd);
	    fn = _free(fn);

	    cmd = rpmExpand(nix->binDir, "/nix-hash --type sha256 --base32 --flat ",
			nix->tmpPath, "/url", NULL);
	    nix->urlHash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    fn = rpmGetPath(nix->downloadCache,"/", nix->urlHash, ".url", NULL);
	    fd = Fopen(fn, "w");
	    if (fd == NULL || Ferror(fd)) {
		fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		if (fd) xx = Fclose(fd);
		fn = _free(fn);
		goto exit;
	    }
	    xx = Fwrite(nix->url, 1, strlen(nix->url), fd);
	    xx = Fwrite("\n", 1, 1, fd);
	    xx = Fclose(fd);
	    fn = _free(fn);

	    nix->cachedHashFN = rpmGetPath(nix->downloadCache, "/",
			nix->urlHash, ".", nix->hashAlgo, NULL);
	    nix->cachedTimestampFN = rpmGetPath(nix->downloadCache, "/",
			nix->urlHash, ".stamp", NULL);
	    nix->cacheFlags = _free(nix->cacheFlags);
	    nix->cacheFlags = xstrdup("--remote-time");

	    if (!Stat(nix->cachedTimestampFN, &sb)
	     && !Stat(nix->cachedHashFN, &sb))
	    {
		/* Only download the file if newer than the cached version. */
		s = nix->cacheFlags;
		nix->cacheFlags = rpmExpand(nix->cacheFlags,
			" --time-cond ", nix->cachedTimestampFN, NULL);
	    }
	}

	/* Perform the download. */
	rpmnixDoDownload(nix);

	if (nix->downloadCache && Stat(nix->tmpFile, &sb) < 0) {
	    /* 
	     * Curl didn't create $tmpFile, so apparently there's no newer
	     * file on the server.
	     */
	    nix->hash = _free(nix->hash);
	    cmd = rpmExpand("cat ", nix->cachedHashFN, NULL);
	    nix->hash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    nix->finalPath = _free(nix->finalPath);
	    cmd = rpmExpand(nix->binDir, "/nix-store --print-fixed-path ",
			nix->hashAlgo, " ", nix->hash, " ", nix->name, NULL);
	    nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    cmd = rpmExpand(nix->binDir, "/nix-store --check-validity ",
			nix->finalPath, " 2>/dev/null; echo $?", NULL);
	    s = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);
	    if (strcmp(s, "0")) {
		fprintf(stderr, _("cached contents of `%s' disappeared, redownloading...\n"), nix->url);
		nix->finalPath = _free(nix->finalPath);
		nix->cacheFlags = _free(nix->cacheFlags);
		nix->cacheFlags = xstrdup("--remote-time");
		rpmnixDoDownload(nix);
	    }
	    s = _free(s);
	}

	if (nix->finalPath == NULL) {

	    /* Compute the hash. */
	    nix->hash = _free(nix->hash);
	    cmd = rpmExpand(nix->binDir, "/nix-hash --type ", nix->hashAlgo,
			" ", nix->hashFormat, " --flat ", nix->tmpFile, NULL);
	    nix->hash = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    if (!nix->quiet)
		fprintf(stderr, _("hash is %s\n"), nix->hash);

	    if (nix->downloadCache) {
		struct timeval times[2];

		fn = rpmGetPath(nix->cachedHashFN, NULL);
		fd = Fopen(fn, "w");
		if (fd == NULL || Ferror(fd)) {
		    fprintf(stderr, _("Fopen(%s, \"w\") failed\n"), fn);
		    if (fd) xx = Fclose(fd);
		    fn = _free(fn);
		    goto exit;
		}
		xx = Fwrite(nix->hash, 1, strlen(nix->hash), fd);
		xx = Fwrite("\n", 1, 1, fd);
		xx = Fclose(fd);
		fn = _free(fn);

		if (Stat(nix->tmpFile, &sb)) {
		    fprintf(stderr, _("Stat(%s) failed\n"), nix->tmpFile);
		    goto exit;
		}
		times[1].tv_sec  = times[0].tv_sec  = sb.st_mtime;
		times[1].tv_usec = times[0].tv_usec = 0;
		xx = Utimes(nix->cachedTimestampFN, times);

	    }

	    /* Add the downloaded file to the Nix store. */
	    nix->finalPath = _free(nix->finalPath);
	    cmd = rpmExpand(nix->binDir, "/nix-store --add-fixed ", nix->hashAlgo,
			" ", nix->tmpFile, NULL);
	    nix->finalPath = rpmExpand("%(", cmd, ")", NULL);
	    cmd = _freeCmd(cmd);

	    if (nix->expHash && *nix->expHash
	     && strcmp(nix->expHash, nix->hash))
	    {
		fprintf(stderr, "hash mismatch for URL `%s'\n", nix->url);
		goto exit;
	    }
	}
    }

    if (!nix->quiet)
	fprintf(stderr, _("path is %s\n"), nix->finalPath);
    fprintf(stdout, "%s\n", nix->hash);
    if (nix->print)
	fprintf(stdout, _("%s\n"), nix->finalPath);

    ec = 0;	/* XXX success */

exit:

    rpmnixRemoveTmpPath(nix);

    return ec;
}

struct poptOption _rpmnixPrefetchUrlOptions[] = {

    /* XXX add --quiet and --print options. */

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
syntax: nix-prefetch-url URL [EXPECTED-HASH]\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

#ifdef	REFERENCE
static int rpmnixAddPatch(rpmnix nix, const char * storePath, const char * patch)
	/*@*/
{
    int found = 0;

NIXDBG((stderr, "--> %s(%p, \"%s\", \"%s\")\n", __FUNCTION__, nix, storePath, patch));
#ifdef	REFERENCE
/*
    my ($patches, $storePath, $patch) = @_;
*/
#endif

#ifdef	REFERENCE
/*
    $$patches{$storePath} = []
        unless defined $$patches{$storePath};
*/
#endif

#ifdef	REFERENCE
/*
    my $patchList = $$patches{$storePath};
*/
#endif

#ifdef	REFERENCE
/*
    foreach my $patch2 (@{$patchList}) {
        $found = 1 if
            $patch2->{url} eq $patch->{url} &&
            $patch2->{basePath} eq $patch->{basePath};
    }
*/
#endif
    
#ifdef	REFERENCE
/*
    push @{$patchList}, $patch if !$found;
*/
#endif

    return !found;
}
#endif	/* REFERENCE */


static int rpmnixReadManifest(rpmnix nix, const char * manifest)
	/*@*/
{

#ifdef	REFERENCE
/*
    my ($manifest, $narFiles, $localPaths, $patches) = @_;
*/
#endif
    int inside = 0;
    int manifestVersion = 2;
    int xx;

#ifdef	NOTYET
    const char * type;
    const char * storePath = NULL;
    const char * url = NULL;
    const char * hash = NULL;
    const char * size = NULL;
    const char * basePath = NULL;
    const char * baseHash = NULL;
    const char * patchType = NULL;
    const char * narHash = NULL;
    const char * references = NULL;
    const char * deriver = NULL;
    const char * hashAlgo = NULL;
    const char * copyFrom = NULL;
#endif

    FD_t fd = Fopen(manifest, "r");

NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, manifest));
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"r\") failed\n", manifest);
	if (fd) xx = Fclose(fd);
	exit(1);
    }

#ifdef	REFERENCE
/*
    while (<MANIFEST>) {
*/
#endif
#ifdef	REFERENCE
/*
        chomp;
        s/\#.*$//g;
        next if (/^$/);
*/
#endif

	if (!inside) {
#ifdef	REFERENCE
/*
            if (/^\s*(\w*)\s*\{$/) {
                $type = $1;
                $type = "narfile" if $type eq "";
                $inside = 1;
                undef $storePath;
                undef $url;
                undef $hash;
                undef $size;
                undef $narHash;
                undef $basePath;
                undef $baseHash;
                undef $patchType;
                $references = "";
                $deriver = "";
                $hashAlgo = "md5";
	    }
*/
#endif

        } else {
            
#ifdef	REFERENCE
/*
            if (/^\}$/) {
                $inside = 0;

                if ($type eq "narfile") {

                    $$narFiles{$storePath} = []
                        unless defined $$narFiles{$storePath};

                    my $narFileList = $$narFiles{$storePath};

                    my $found = 0;
                    foreach my $narFile (@{$narFileList}) {
                        $found = 1 if $narFile->{url} eq $url;
                    }
                    if (!$found) {
                        push @{$narFileList},
                            { url => $url, hash => $hash, size => $size
                            , narHash => $narHash, references => $references
                            , deriver => $deriver, hashAlgo => $hashAlgo
                            };
                    }
                
                }

                elsif ($type eq "patch") {
                    rpmnixAddPatch $patches, $storePath,
                        { url => $url, hash => $hash, size => $size
                        , basePath => $basePath, baseHash => $baseHash
                        , narHash => $narHash, patchType => $patchType
                        , hashAlgo => $hashAlgo
                        };
                }

                elsif ($type eq "localPath") {

                    $$localPaths{$storePath} = []
                        unless defined $$localPaths{$storePath};

                    my $localPathsList = $$localPaths{$storePath};

                    # !!! remove duplicates
                    
                    push @{$localPathsList},
                        { copyFrom => $copyFrom, references => $references
                        , deriver => ""
                        };
                }

            }
            
            elsif (/^\s*StorePath:\s*(\/\S+)\s*$/) { $storePath = $1; }
            elsif (/^\s*CopyFrom:\s*(\/\S+)\s*$/) { $copyFrom = $1; }
            elsif (/^\s*Hash:\s*(\S+)\s*$/) { $hash = $1; }
            elsif (/^\s*URL:\s*(\S+)\s*$/) { $url = $1; }
            elsif (/^\s*Size:\s*(\d+)\s*$/) { $size = $1; }
            elsif (/^\s*SuccOf:\s*(\/\S+)\s*$/) { } # obsolete
            elsif (/^\s*BasePath:\s*(\/\S+)\s*$/) { $basePath = $1; }
            elsif (/^\s*BaseHash:\s*(\S+)\s*$/) { $baseHash = $1; }
            elsif (/^\s*Type:\s*(\S+)\s*$/) { $patchType = $1; }
            elsif (/^\s*NarHash:\s*(\S+)\s*$/) { $narHash = $1; }
            elsif (/^\s*References:\s*(.*)\s*$/) { $references = $1; }
            elsif (/^\s*Deriver:\s*(\S+)\s*$/) { $deriver = $1; }
            elsif (/^\s*ManifestVersion:\s*(\d+)\s*$/) { $manifestVersion = $1; }

            # Compatibility;
            elsif (/^\s*NarURL:\s*(\S+)\s*$/) { $url = $1; }
            elsif (/^\s*MD5:\s*(\S+)\s*$/) { $hash = "md5:$1"; }

*/
#endif
        }
#ifdef	REFERENCE
/*
    }
*/
#endif

    if (fd) xx = Fclose(fd);

    return manifestVersion;
}

static char * rpmnixDownloadFile(rpmnix nix, const char * url)
	/*@*/
{
    const char * cmd;
    const char * rval;
    char * path;
    int xx;

    xx = setenv("PRINT_PATH", "1", 0);
    xx = setenv("QUIET", "1", 0);
    cmd = rpmExpand(nix->binDir, "/nix-prefetch-url '", url, "'", NULL);

    rval = rpmExpand("%(", cmd, ")", NULL);
    /* XXX The 1st line is the hash, the 2nd line is the path ... */
    if ((path = strchr(rval, '\n')) == NULL) {
	fprintf(stderr, "nix-prefetch-url did not return a path");
	exit(1);
    }
#ifdef	REFERENCE
/*
    chomp $path;
*/
#endif
    path = xstrdup(path+1);
NIXDBG((stderr, "<-- %s(%p, \"%s\") path %s\n", __FUNCTION__, nix, url, path));
    rval = _free(rval);
    cmd = _freeCmd(cmd);
    return path;
}

static int processURL(rpmnix nix, const char * url)
	/*@*/
{
    const char * cmd;
    const char * rval;
    int version;
    const char * baseName;
    const char * urlFile;
    const char * finalPath;
    const char * hash;

    FD_t fd;
    char * globpat;
    char * fn;
    char * manifest;
    struct stat sb;
    ARGV_t gav;
    int gac;
    int xx;
    int i;

#ifdef	REFERENCE
/*
    my $url = shift;

    $url =~ s/\/$//;

    my $manifest;
*/
#endif

NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, url));
    /* First see if a bzipped manifest is available. */
    fn = rpmGetPath(url, ".bz2", NULL);
#ifdef	REFERENCE
/*
    if (system("/usr/bin/curl --fail --silent --head '$url'.bz2 > /dev/null") == 0)
*/
#else
    if (!Stat(fn, &sb))
#endif
    {
	const char * bzipped;

        fprintf(stdout, _("fetching list of Nix archives at `%s'...\n"), fn);

	bzipped = rpmnixDownloadFile(nix, fn);

	manifest = rpmExpand(nix->tmpPath, "/MANIFEST", NULL);

	cmd = rpmExpand("/usr/libexec/nix/bunzip2 < ", bzipped,
		" > ", manifest, "; echo $?", NULL);
	rval = rpmExpand("%(", cmd, ")", NULL);
	if (strcmp(rval, "0")) {
	    fprintf(stderr, "cannot decompress manifest\n");
	    exit(1);
	}
	rval = _free(rval);
	cmd = _freeCmd(cmd);

	cmd = rpmExpand(nix->binDir, "/nix-store --add ", manifest, NULL);
	manifest = _free(manifest);
	manifest = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);

#ifdef	REFERENCE
/*
        chomp $manifest;
*/
#endif

    } else {	/* Otherwise, just get the uncompressed manifest. */
        fprintf(stdout, _("obtaining list of Nix archives at `%s'...\n"), url);
	manifest = rpmnixDownloadFile(nix, url);
    }
    fn = _free(fn);

    version = rpmnixReadManifest(nix, manifest);
    if (version < 3) {
	fprintf(stderr, "`%s' is not a manifest or it is too old (i.e., for Nix <= 0.7)\n", url);
	exit (1);
    }
    if (version >= 5) {
	fprintf(stderr, "manifest `%s' is too new\n", url);
	exit (1);
    }

    if (F_ISSET(nix, SKIPWRONGSTORE)) {
	size_t ns = strlen(nix->storeDir);
	int nac = argvCount(nix->narFiles);
	int j;
	for (j = 0; j < nac; j++) {
	    const char * path = nix->narFiles[j];
	    size_t np = strlen(path);

	    if (np > ns && !strncmp(path, nix->storeDir, ns) && path[ns] == '/')
		continue;
	    fprintf(stderr, "warning: manifest `%s' assumes a Nix store at a different location than %s, skipping...\n", url, nix->storeDir);
	    exit(0);
	}
    }

    fn = xstrdup(url);
    baseName = xstrdup(basename(fn));
    fn = _free(fn);

    cmd = rpmExpand(nix->binDir, "/nix-hash --flat ", manifest, NULL);
    hash = rpmExpand("%(", cmd, ")", NULL);
    cmd = _freeCmd(cmd);
    if (hash == NULL) {
	fprintf(stderr, "cannot hash `%s'\n", manifest);
	exit(1);
    }
#ifdef	REFERENCE
/*
    chomp $hash;
*/
#endif

    urlFile = rpmGetPath(nix->manifestsPath, "/",
			baseName, "-", hash, ".url", NULL);

    fd = Fopen(urlFile, "w");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "cannot create `%s'\n", urlFile);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    (void) Fwrite(url, 1, strlen(url), fd);
    xx = Fclose(fd);
    
    finalPath = rpmGetPath(nix->manifestsPath, "/",
			baseName, "-", hash, ".nixmanifest", NULL);

    if (!Lstat(finalPath, &sb))
	xx = Unlink(finalPath);
        
    if (Symlink(manifest, finalPath)) {
	fprintf(stderr, _("cannot link `%s' to `%s'\n"), finalPath, manifest);
	exit(1);
    }
    finalPath = _free(finalPath);

    /* Delete all old manifests downloaded from this URL. */
    globpat = rpmGetPath(nix->manifestsPath, "/*.url", NULL);
    gav = NULL;
    gac = 0;
    if (!rpmGlob(globpat, &gac, &gav)) {
	for (i = 0; i < gac; i++) {
	    const char * urlFile2 = gav[i];
	    char * base, * be;
	    ARGV_t uav;
	    const char * url2;

	    if (!strcmp(urlFile, urlFile2))
		continue;

	    uav = NULL;
	    fd = Fopen(urlFile2, "r.fpio");
	    if (fd == NULL || Ferror(fd)) {
		fprintf(stderr, "cannot create `%s'\n", urlFile2);
		if (fd) xx = Fclose(fd);
		exit(1);
	    }
	    xx = argvFgets(&uav, fd);
	    xx = Fclose(fd);
	    url2 = xstrdup(uav[0]);
	    uav = argvFree(uav);

	    if (strcmp(url, url2))
		continue;

	    base = xstrdup(urlFile2);
	    be = base + strlen(base) - sizeof(".url") + 1;
	    if (be > base && !strcmp(be, ".url")) *be = '\0';

	    fn = rpmGetPath(base, ".url", NULL);
	    xx = Unlink(fn);
	    fn = _free(fn);

	    fn = rpmGetPath(base, ".nixmanifest", NULL);
	    xx = Unlink(fn);
	    fn = _free(fn);

	    base = _free(base);
	}
    }
    gav = argvFree(gav);
    globpat = _free(globpat);

    return 0;
}

int
rpmnixPull(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 1;		/* assume failure */
    int xx;
    int i;

    nix->tmpPath = mkdtemp(rpmGetPath(nix->tmpDir, "/nix-pull.XXXXXX", NULL));
    if (nix->tmpPath == NULL) {
	fprintf(stderr, _("cannot create a temporary directory\n"));
	goto exit;
    }

    /* Prevent access problems in shared-stored installations. */
    xx = umask(0022);

    /* Create the manifests directory if it doesn't exist. */
    if (rpmioMkpath(nix->manifestsPath, (mode_t)0755, (uid_t)-1, (gid_t)-1)) {
	fprintf(stderr, _("cannot create directory `%s'\n"), nix->manifestsPath);
	goto exit;
    }

    for (i = 0; i < ac; i++) {
	xx = processURL(nix, av[i]);
    }

    fprintf(stdout, "%d store paths in manifest\n", 
		argvCount(nix->narFiles) + argvCount(nix->localPaths));

    ec = 0;	/* XXX success */

exit:
#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-pull.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif

    return ec;
}

struct poptOption _rpmnixPullOptions[] = {

 { "skip-wrong-store", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_SKIPWRONGSTORE,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

#ifdef	DYING
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
"), NULL },
#endif

  POPT_TABLEEND
};

/*==============================================================*/

static void rpmnixWriteManifest(rpmnix nix, const char * manifest)
	/*@*/
{
    char * fn = rpmGetPath(manifest, NULL);
    char * tfn = rpmGetPath(fn, ".tmp", NULL);
    const char * rval;
    const char * cmd;
    const char * s;
    FD_t fd;
    ssize_t nw;
    int xx;

NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, manifest));

#ifdef	REFERENCE
/*
    my ($manifest, $narFiles, $patches) = @_;
*/
#endif

    fd = Fopen(tfn, "w");	/* XXX check exclusive */
    if (fd == NULL || Ferror(fd)) {
	if (fd) xx = Fclose(fd);
	goto exit;
    }
    
    s = "\
version {\n\
  ManifestVersion: 3\n\
}\n\
";
    nw = Fwrite(s, 1, strlen(s), fd);

#ifdef	REFERENCE
/*
    foreach my $storePath (sort (keys %{$narFiles})) {
        my $narFileList = $$narFiles{$storePath};
        foreach my $narFile (@{$narFileList}) {
            print MANIFEST "{\n";
            print MANIFEST "  StorePath: $storePath\n";
            print MANIFEST "  NarURL: $narFile->{url}\n";
            print MANIFEST "  Hash: $narFile->{hash}\n" if defined $narFile->{hash};
            print MANIFEST "  NarHash: $narFile->{narHash}\n";
            print MANIFEST "  Size: $narFile->{size}\n" if defined $narFile->{size};
            print MANIFEST "  References: $narFile->{references}\n"
                if defined $narFile->{references} && $narFile->{references} ne "";
            print MANIFEST "  Deriver: $narFile->{deriver}\n"
                if defined $narFile->{deriver} && $narFile->{deriver} ne "";
            print MANIFEST "}\n";
        }
    }
*/
#endif
    
#ifdef	REFERENCE
/*
    foreach my $storePath (sort (keys %{$patches})) {
        my $patchList = $$patches{$storePath};
        foreach my $patch (@{$patchList}) {
            print MANIFEST "patch {\n";
            print MANIFEST "  StorePath: $storePath\n";
            print MANIFEST "  NarURL: $patch->{url}\n";
            print MANIFEST "  Hash: $patch->{hash}\n";
            print MANIFEST "  NarHash: $patch->{narHash}\n";
            print MANIFEST "  Size: $patch->{size}\n";
            print MANIFEST "  BasePath: $patch->{basePath}\n";
            print MANIFEST "  BaseHash: $patch->{baseHash}\n";
            print MANIFEST "  Type: $patch->{patchType}\n";
            print MANIFEST "}\n";
        }
    }
*/
#endif

    if (fd)
	xx = Fclose(fd);

    if (Rename(tfn, fn) < 0) {
	fprintf(stderr, "Rename(%s, %s) failed\n", tfn, fn);
	exit(1);
    }
    tfn = _free(tfn);
    tfn = rpmGetPath(manifest, ".bz2.tmp", NULL);

    /* Create a bzipped manifest. */
    cmd = rpmExpand("/usr/libexec/nix/bzip2 < ", fn,
                " > ", tfn, "; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    cmd = _free(cmd);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "cannot compress manifest\n");
	exit(1);
    }
    rval = _free(rval);

    fn = _free(fn);
    fn = rpmGetPath(manifest, ".bz2", NULL);

    if (Rename(tfn, fn) < 0) {
	fprintf(stderr, "Rename(%s, %s) failed\n", tfn, fn);
	exit(1);
    }

exit:
    tfn = _free(tfn);
    fn = _free(fn);
    return;
}

static int rpmnixCopyFile(const char * src, const char * dst)
	/*@*/
{
    const char * tfn = rpmGetPath(dst, ".tmp", NULL);
    const char * rval;
    const char * cmd;

NIXDBG((stderr, "--> %s(\"%s\", \"%s\")\n", __FUNCTION__, src, dst));
    /* XXX Ick. */
    cmd = rpmExpand("/bin/cp '", src, "' '", tfn, "'; echo $?", NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    cmd = _free(cmd);
    if (strcmp(rval, "0")) {
	fprintf(stderr, "cannot copy file\n");
	exit(1);
    }
    rval = _free(rval);

    if (Rename(tfn, dst) < 0) {
	fprintf(stderr, "Rename(%s, %s) failed\n", tfn, dst);
	exit(1);
    }

    tfn = _free(tfn);
    return 0;
}

static int rpmnixArchiveExists(rpmnix nix, const char * name)
	/*@*/
{
NIXDBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, name));
#ifdef	REFERENCE
/*
    my $name = shift;
    print STDERR "  HEAD on $archivesGetURL/$name\n";
    return system("$curl --head $archivesGetURL/$name > /dev/null") == 0;
*/
#endif
    const char * fn = rpmGetPath(nix->archivesGetURL, "/", name, NULL);
    struct stat sb;
    int rc = Stat(fn, &sb);
    fn = _free(fn);
    return (rc != 0);	/* XXX 0 on success */
}

int
rpmnixPush(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    const char * s;
    int ec = 1;		/* assume failure */
    int xx;
    int i;

    const char * cmd;
    const char * rval;
    FD_t fd;
    ssize_t nw;
    ARGV_t tav;

    const char * curlDefault = "/usr/bin/curl --fail --silent";
    int localCopy;
    int nac;

    nix->tmpPath = mkdtemp(rpmGetPath(nix->tmpDir, "/nix-push.XXXXXX", NULL));
    if (nix->tmpPath == NULL) {
	fprintf(stderr, _("cannot create a temporary directory\n"));
	goto exit;
    }

    nix->nixExpr = rpmGetPath(nix->tmpPath, "/create-nars.nix", NULL);
    nix->manifest = rpmGetPath(nix->tmpPath, "/MANIFEST", NULL);

#ifdef	REFERENCE
/*
my $curl = "/usr/bin/curl --fail --silent";
my $extraCurlFlags = ${ENV{'CURL_FLAGS'}};
$curl = "$curl $extraCurlFlags" if defined $extraCurlFlags;
*/
#endif
    if ((s = getenv("CURL_FLAGS")))
	nix->curl = rpmExpand(curlDefault, " ", s, NULL);
    else
	nix->curl = rpmExpand(curlDefault, NULL);

    /* Parse the command line. */
    if (ac < 1) {
	poptPrintUsage(nix->con, stderr, 0);
	goto exit;
    }

    if (F_ISSET(nix, COPY)) {
	if (ac < 2) {
	    poptPrintUsage(nix->con, stderr, 0);
	    goto exit;
	}
	localCopy = 1;
	nix->localArchivesDir = av[0];
	nix->localManifestFile = av[1];
	if (nix->targetArchivesUrl == NULL)
	    nix->targetArchivesUrl = rpmExpand("file://", nix->localArchivesDir, NULL);
    } else {
	if (ac < 3) {
	    poptPrintUsage(nix->con, stderr, 0);
	    goto exit;
	}
	localCopy = 0;
	nix->archivesPutURL = av[0];
	nix->archivesGetURL = av[1];
	nix->manifestPutURL = av[2];
    }

    /*
     * From the given store paths, determine the set of requisite store
     * paths, i.e, the paths required to realise them.
     */
    for (i = 0; i < ac; i++) {
	const char * path = av[i];

assert(*path == '/');
	/*
	 * Get all paths referenced by the normalisation of the given 
	 * Nix expression.
	 */
#ifdef	REFERENCE
/*
    my $pid = open(READ,
        "$binDir/nix-store --query --requisites --force-realise " .
        "--include-outputs '$path'|") or die;
    
    while (<READ>) {
        chomp;
        die "bad: $_" unless /^\//;
        $storePaths{$_} = "";
    }

    close READ or die "nix-store failed: $?";
*/
#endif
	cmd = rpmExpand(nix->binDir, "/nix-store --query --requisites --force-realise --include-outputs '", path, "'", NULL);
	rval = rpmExpand("%(", cmd, ")", NULL);
	xx = argvSplit(&nix->storePaths, rval, NULL);
	rval = _free(rval);
	cmd = _freeCmd(cmd);
    }

    /*
     * For each path, create a Nix expression that turns the path into
     * a Nix archive.
     */
#ifdef	REFERENCE
/*
my @storePaths = keys %storePaths;
*/
#endif

#ifdef	REFERENCE
/*
open NIX, ">$nixExpr";
print NIX "[";
*/
#endif
    fd = Fopen(nix->nixExpr, "w");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"w\") failed.\n", nix->nixExpr);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    nw = Fwrite("[\n", 1, sizeof("[\n")-1, fd);

#ifdef	REFERENCE
/*
foreach my $storePath (@storePaths) {
    die unless ($storePath =~ /\/[0-9a-z]{32}[^\"\\\$]*$/);

    # Construct a Nix expression that creates a Nix archive.
    my $nixexpr = 
        "((import $dataDir/nix/corepkgs/nar/nar.nix) " .
        "{storePath = builtins.storePath \"$storePath\"; system = \"i686-linux\"; hashAlgo = \"$hashAlgo\";}) ";
    
    print NIX $nixexpr;
}
*/
#endif
    nac = argvCount(nix->storePaths);
    for (i = 0; i < nac; i++) {
	const char * storePath = nix->storePaths[i];
	const char * nixexpr = rpmExpand("(",
		"(import ", nix->dataDir, "/nix/corepkgs/nar/nar.nix)",
		" {",
		    "storePath = builtins.storePath \"", storePath, "\";",
		    "system = \"i686-linux\";",
		    "hashAlgo = \"", nix->hashAlgo, "\";",
		" }", ")", NULL);
	nw = Fwrite(nixexpr, 1, strlen(nixexpr), fd);
	nw = Fwrite("\n", 1, sizeof("\n")-1, fd);
	nixexpr = _free(nixexpr);
    }

#ifdef	REFERENCE
/*
print NIX "]";
close NIX;
*/
#endif
    nw = Fwrite("]\n", 1, sizeof("]\n")-1, fd);
    xx = Fclose(fd);

    /* Instantiate store derivations from the Nix expression. */
    fprintf(stderr, "instantiating store derivations...\n");

#ifdef	REFERENCE
/*
my @storeExprs;
my $pid = open(READ, "$binDir/nix-instantiate $nixExpr|")
    or die "cannot run nix-instantiate";
while (<READ>) {
    chomp;
    die unless /^\//;
    push @storeExprs, $_;
}
close READ or die "nix-instantiate failed: $?";
*/
#endif
    cmd = rpmExpand(nix->binDir, "/nix-instantiate ", nix->nixExpr, NULL);
    rval = rpmExpand("%(", cmd, ")", NULL);
    xx = argvSplit(&nix->storeExprs, rval, NULL);
    rval = _free(rval);
    cmd = _freeCmd(cmd);

    /* Build the derivations. */
    fprintf(stderr, "creating archives...\n");

#ifdef	REFERENCE
/*
my @tmp = @storeExprs;
*/
#endif
    tav = nix->storeExprs;

#ifdef	REFERENCE
/*
while (scalar @tmp > 0)
*/
#endif
    nac = argvCount(tav);
    for (i = 0; i < nac; i++) {
	const char * tmp2;
	
	/* XXX this appears to attempt chunks < 256 for --realise */
#ifdef	REFERENCE
/*
    my $n = scalar @tmp;
    if ($n > 256) { $n = 256 };
    my @tmp2 = @tmp[0..$n - 1];
    @tmp = @tmp[$n..scalar @tmp - 1];
*/
#endif
	/* XXX HACK: do one-by-one for now. */
	tmp2 = tav[i];

#ifdef	REFERENCE
/*
    my $pid = open(READ, "$binDir/nix-store --realise @tmp2|")
        or die "cannot run nix-store";
    while (<READ>) {
        chomp;
        die unless (/^\//);
        push @narPaths, "$_";
    }
    close READ or die "nix-store failed: $?";
*/
#endif
	cmd = rpmExpand(nix->binDir, "/nix-store --realise ", tmp2, NULL);
	rval = rpmExpand("%(", cmd, ")", NULL);
	xx = argvSplit(&nix->narPaths, rval, NULL);
	rval = _free(rval);
	cmd = _freeCmd(cmd);
    }

    /* Create the manifest. */
    fprintf(stderr, "creating manifest...\n");

#ifdef	REFERENCE
/*
for (my $n = 0; $n < scalar @storePaths; $n++)
*/
#endif
    nac = argvCount(nix->storePaths);
    for (i = 0; i < nac; i++) {
	const char * storePath = nix->storePaths[i];
	const char * narDir = nix->narPaths[i];

	struct stat sb;
	const char * references;
	const char * deriver;
	const char * url;
	const char * narbz2Hash;
	const char * narHash;
	const char * narName;
	const char * narFile;
	off_t narbz2Size;

#ifdef	REFERENCE
/*
    open HASH, "$narDir/narbz2-hash" or die "cannot open narbz2-hash";
    my $narbz2Hash = <HASH>;
    chomp $narbz2Hash;
    $narbz2Hash =~ /^[0-9a-z]+$/ or die "invalid hash";
    close HASH;
*/
#endif
	/* XXX Ick. */
	cmd = rpmExpand("/bin/cat ", narDir, "/narbz2-hash", NULL);
	narbz2Hash = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);

#ifdef	REFERENCE
/*
    open HASH, "$narDir/nar-hash" or die "cannot open nar-hash";
    my $narHash = <HASH>;
    chomp $narHash;
    $narHash =~ /^[0-9a-z]+$/ or die "invalid hash";
    close HASH;
*/
#endif
	/* XXX Ick. */
	cmd = rpmExpand("/bin/cat ", narDir, "/nar-hash", NULL);
	narHash = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);
    
	narName = rpmExpand(narbz2Hash, ".nar.bz2", NULL);
	narFile = rpmGetPath(narDir, "/", narName, NULL);
	if (Lstat(narFile, &sb) < 0) {
	    fprintf(stderr, "narfile for %s not found\n", storePath);
	    exit(1);
	}
	xx = argvAdd(&nix->narArchives, narFile);

	xx = Stat(narFile, &sb);
	narbz2Size = sb.st_size;

#ifdef	REFERENCE
/*
    my $references = `$binDir/nix-store --query --references '$storePath'`;
    die "cannot query references for `$storePath'" if $? != 0;
    $references = join(" ", split(" ", $references));
*/
#endif
	cmd = rpmExpand(nix->binDir, "/nix-store --query --references '",
		storePath, "'", NULL);
	references = rpmExpand("%(", cmd, ")", NULL);
	cmd = _freeCmd(cmd);

#ifdef	REFERENCE
/*
    my $deriver = `$binDir/nix-store --query --deriver '$storePath'`;
    die "cannot query deriver for `$storePath'" if $? != 0;
    chomp $deriver;
    $deriver = "" if $deriver eq "unknown-deriver";
*/
#endif
	cmd = rpmExpand(nix->binDir, "/nix-store --query",
		" --deriver '", storePath, "'", NULL);
	deriver = rpmExpand("%(", cmd, ")", NULL);
	if (!strcmp(deriver, "unknown-deriver")) {
	    deriver = _free(deriver);
	    deriver = xstrdup("");
	}
	cmd = _freeCmd(cmd);

	if (localCopy)
	    url = rpmGetPath(nix->targetArchivesUrl, "/", narName, NULL);
	else
	    url = rpmGetPath(nix->archivesGetURL, "/", narName, NULL);

#ifdef	REFERENCE
/*
    $narFiles{$storePath} = [
        { url => $url
        , hash => "$hashAlgo:$narbz2Hash"
        , size => $narbz2Size
        , narHash => "$hashAlgo:$narHash"
        , references => $references
        , deriver => $deriver
        }
    ];
*/
#endif
    }

    rpmnixWriteManifest(nix, nix->manifest);

    /* Upload/copy the archives. */
    fprintf(stderr, "uploading/copying archives...\n");

    nac = argvCount(nix->narArchives);
    for (i = 0; i < nac; i++) {
	char * narArchive = xstrdup(av[i]);
	char * bn = basename(narArchive);

	if (localCopy) {
	    const char * dst = rpmGetPath(nix->localArchivesDir, "/", bn, NULL);
	    struct stat sb;

	    /*
	     * Since nix-push creates $dst atomically, if it exists we
	     * don't have to copy again.
	     */
	    if (Stat(dst, &sb) < 0) {
		fprintf(stderr, "  %s\n", narArchive);
		xx = rpmnixCopyFile(narArchive, dst);
	    }
	} else {
	    if (!rpmnixArchiveExists(nix, bn)) {
		fprintf(stderr, "  %s\n", narArchive);
#ifdef	REFERENCE
/*
            system("$curl --show-error --upload-file " .
                   "'$narArchive' '$archivesPutURL/$basename' > /dev/null") == 0 or
                   die "curl failed on $narArchive: $?";
*/
#endif
		cmd = rpmExpand(nix->curl, " --show-error",
			" --upload-file '", narArchive, "' '", nix->manifestPutURL, "/", bn, "'",
			" > /dev/null; echo $?", NULL);
		rval = rpmExpand("%(", cmd, ")", NULL);
		if (strcmp(rval, "0")) {
		    fprintf(stderr, "curl failed on %s: %s\n", narArchive, rval);
		    exit(1);
		}
		rval = _free(rval);
		cmd = _freeCmd(cmd);
	    }
	}
	narArchive = _free(narArchive);
    }

    /* Upload the manifest. */
    fprintf(stderr, "uploading manifest...\n");

    if (localCopy) {
	const char * bmanifest = rpmGetPath(nix->manifest, ".bz2", NULL);
	const char * blocalManifestFile =
		rpmGetPath(nix->localManifestFile, ".bz2", NULL);

	xx = rpmnixCopyFile(nix->manifest, nix->localManifestFile);
	xx = rpmnixCopyFile(bmanifest, blocalManifestFile);
	bmanifest = _free(bmanifest);
	blocalManifestFile = _free(blocalManifestFile);
    } else {
#ifdef	REFERENCE
/*
    system("$curl --show-error --upload-file " .
           "'$manifest' '$manifestPutURL' > /dev/null") == 0 or
           die "curl failed on $manifest: $?";
*/
#endif
	cmd = rpmExpand(nix->curl, " --show-error",
		" --upload-file '", nix->manifest, "' '", nix->manifestPutURL, "'",
		" > /dev/null; echo $?", NULL);
	rval = rpmExpand("%(", cmd, ")", NULL);
	if (strcmp(rval, "0")) {
	    fprintf(stderr, "curl failed on %s: %s\n", nix->manifest, rval);
	    exit(1);
	}
	rval = _free(rval);
	cmd = _freeCmd(cmd);

#ifdef	REFERENCE
/*
    system("$curl --show-error --upload-file " .
           "'$manifest'.bz2 '$manifestPutURL'.bz2 > /dev/null") == 0 or
           die "curl failed on $manifest: $?";
*/
#endif
	cmd = rpmExpand(nix->curl, " --show-error",
		" --upload-file '", nix->manifest, ".bz2' '", nix->manifestPutURL, ".bz2'",
		" > /dev/null; echo $?", NULL);
	rval = rpmExpand("%(", cmd, ")", NULL);
	if (strcmp(rval, "0")) {
	    fprintf(stderr, "curl failed on %s.bz2: %s\n", nix->manifest, rval);
	    exit(1);
	}
	rval = _free(rval);
	cmd = _freeCmd(cmd);
    }

    ec = 0;	/* XXX success */

exit:
#ifdef	REFERENCE
/*
my $tmpDir = tempdir("nix-push.XXXXXX", CLEANUP => 1, TMPDIR => 1)
    or die "cannot create a temporary directory";
*/
#endif

    return ec;
}

struct poptOption _rpmnixPushOptions[] = {

 { "copy", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_COPY,
        N_("FIXME"), NULL },
 { "target", '\0', POPT_ARG_STRING,	&_nix.targetArchivesUrl, 0,
        N_("FIXME"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-push --copy ARCHIVES_DIR MANIFEST_FILE PATHS...\n\
   or: nix-push ARCHIVES_PUT_URL ARCHIVES_GET_URL MANIFEST_PUT_URL PATHS...\n\
\n\
`nix-push' copies or uploads the closure of PATHS to the given\n\
destination.\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

int
rpmnixEcho(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

struct poptOption _rpmnixEchoOptions[] = {

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
        N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP
  POPT_TABLEEND
};

/*==============================================================*/

int
rpmnixEnv(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

/*==============================================================*/

int
rpmnixHash(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

struct poptOption _rpmnixHashOptions[] = {

 { "flat", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_FLAT,
	N_("compute hash of regular file contents, not metadata"), NULL },
 { "base32", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_BASE32,
	N_("print hash in base-32 instead of hexadecimal"), NULL },
 { "truncate", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_TRUNCATE,
	N_("truncate the hash to 160 bits"), NULL },
 { "to-base16", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_TOBASE16,
	N_("convert output in base16"), NULL },
 { "to-base32", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_TOBASE32,
	N_("convert output in base32"), NULL },

 { "type", '\0', POPT_ARG_STRING,	&_nix.hashAlgoName, 0,
	N_("use hash algorithm HASH (\"md5\" (default), \"sha1\", \"sha256\")"),
	N_("HASH") },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioDigestPoptTable, 0,
	N_("Available digests:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-hash [OPTIONS...] [FILES...]\n\
\n\
`nix-hash' computes and prints cryptographic hashes for the specified\n\
files.\n\
\n\
  --flat: compute hash of regular file contents, not metadata\n\
  --base32: print hash in base-32 instead of hexadecimal\n\
  --type HASH: use hash algorithm HASH (\"md5\" (default), \"sha1\", \"sha256\")\n\
  --truncate: truncate the hash to 160 bits\n\
"), NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

/*==============================================================*/

int
rpmnixInstantiate(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

static void rpmnixInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_ARG:			/*    --arg ARG1 ARG2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->instArgs, arg1);
	xx = argvAdd(&nix->instArgs, arg2);
#endif
	break;
    case NIX_OPTION:			/*    --option OPT1 OPT2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->instArgs, opt1);
	xx = argvAdd(&nix->instArgs, opt2);
#endif
	break;

    case NIX_VERBOSE:			/* -v,--verbose */
	nix->verbose++;
	break;
    case NIX_VERSION:			/*    --version */
	/* XXX FIXME */
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

struct poptOption _rpmnixInstantiateOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixInstantiateArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "eval-only", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_EVALONLY,
	N_("evaluate and print resulting term; do not instantiate"), NULL },
 { "parse-only", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_PARSEONLY,
	N_("parse and print abstract syntax tree"), NULL },
 { "attr", 'A', POPT_ARG_STRING,	&_nix.attr, 0,
	N_("select a specific ATTR from the Nix expression"), N_("ATTR") },
 { "add-root", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_ADDROOT,
	N_("add garbage collector roots for the result"), NULL },

 { "indirect", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_INDIRECT,
	N_("print an XML representation of the abstract syntax tree"), NULL },
 { "xml", '\0', POPT_BIT_SET,		&_nix.flags, RPMNIX_FLAGS_XML,
	N_("print an XML representation of the abstract syntax tree"), NULL },
 { "strict", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_STRICT,
	N_("compute attributes and list elements, rather than being lazy"), NULL },

 { "show-trace", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_SHOWTRACE,
	N_("FIXME"), NULL },

#ifdef	NOTYET
 { "log-type", '\0', POPT_ARG_STRING,		&_nix.logType, 0,
	N_("FIXME"), N_("TYPE") },
#endif

/* XXX needs next 2 args */
 { "arg", '\0', POPT_ARG_NONE,				0, NIX_ARG,
	N_("FIXME"), N_("ARG1 ARG2") },
 { "argstr", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, 0, NIX_ARG,
	N_("FIXME"), N_("ARG1 ARG2") },
/* XXX needs next 2 args */
 { "option", '\0', POPT_ARG_NONE,			0, NIX_OPTION,
	N_("FIXME"), N_("OPT1 OPT2") },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

 { "verbose", 'v', POPT_ARG_NONE,			0, NIX_VERBOSE,
	N_("verbose operation (may be repeated)"), NULL },
 { "version", '\0', POPT_ARG_NONE,			0, NIX_VERSION,
	N_("output version information"), NULL },

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-instantiate [OPTIONS...] [FILES...]\n\
\n\
`nix-instantiate' turns Nix expressions into store derivations.\n\
\n\
The argument `-' may be specified to read a Nix expression from\n\
standard input.\n\
\n\
Options:\n\
\n\
  --version: output version information\n\
  --help: display help\n\
\n\
  --verbose / -v: verbose operation (may be repeated)\n\
\n\
  --eval-only: evaluate and print resulting term; do not instantiate\n\
  --parse-only: parse and print abstract syntax tree\n\
\n\
  --attr / -A PATH: select an attribute from the top-level expression\n\
\n\
  --add-root: add garbage collector roots for the result\n\
\n\
For --eval-only / --parse-only:\n\
\n\
  --xml: print an XML representation of the abstract syntax tree\n\
\n\
For --eval-only:\n\
\n\
  --strict: compute attributes and list elements, rather than being\n\
    lazy\n\
"), NULL },

  POPT_TABLEEND
};
/*==============================================================*/

enum {
    opRealise = 1,
    opAdd,
    opAddFixed,
    opPrintFixedPath,
    opDelete,
    opQuery,
    opReadLog,
    opDumpDB,
    opLoadDB,
    opRegisterValidity,
    opCheckValidity,
    opGC,
    opDump,
    opRestore,
    opExport,
    opImport,
    opInit,
    opVerify,
    opOptimise,
};

int
rpmnixStore(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

static void rpmnixStoreArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {

    case NIX_OPTION:			/*    --option OPT1 OPT2 */
#ifdef	NOTYET	/* XXX needs next 2 args */
	xx = argvAdd(&nix->buildArgs, opt1);
	xx = argvAdd(&nix->buildArgs, opt2);
#endif
	break;
    case NIX_VERBOSE:			/* -v,--verbose */
	nix->verbose++;
	break;
    case NIX_VERSION:			/*    --version */
	/* XXX FIXME */
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

static struct poptOption _rpmnixStoreOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmnixStoreArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "max-jobs", 'j', POPT_ARG_INT,	&_nix.jobs, 0,
	N_("FIXME"), N_("JOBS") },

 { "dry-run", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_DRYRUN,
	N_("FIXME"), NULL },
 { "add-root", '\0', POPT_BIT_SET,	&_nix.flags, RPMNIX_FLAGS_ADDROOT,
	N_("add garbage collector roots for the result"), NULL },

 { "realise", 'r', POPT_ARG_VAL,	&_nix.op, opRealise,
	N_("ensure path validity; if a derivation, ensure the validity of the outputs"), NULL },
/* XXX conflict with -A,--attr otherwise -A,--add */
 { "add", '\0', POPT_ARG_VAL,		&_nix.op, opAdd,
	N_("copy a path to the Nix store"), NULL },
 { "add-fixed", '\0', POPT_ARG_VAL,	&_nix.op, opAddFixed,
	N_("FIXME"), NULL },
 { "delete", '\0', POPT_ARG_VAL,	&_nix.op, opDelete,
	N_("safely delete paths from the Nix store"), NULL },
 { "read-log", 'l', POPT_ARG_VAL,	&_nix.op, opReadLog,
	N_("print build log of given store paths"), NULL },
 { "register-validity", '\0', POPT_ARG_VAL,	&_nix.op, opRegisterValidity,
	N_("register path validity (dangerous!)"), NULL },
 { "check-validity", '\0', POPT_ARG_VAL,	&_nix.op, opCheckValidity,
	N_("check path validity"), NULL },
 { "dump", '\0', POPT_ARG_VAL,		&_nix.op, opDump,
	N_("dump a path as a Nix archive, forgetting dependencies"), NULL },
 { "restore", '\0', POPT_ARG_VAL,	&_nix.op, opRestore,
	N_("restore a path from a Nix archive, without registering validity"), NULL },
 { "export", '\0', POPT_ARG_VAL,	&_nix.op, opExport,
	N_("export a path as a Nix archive, marking dependencies"), NULL },
 { "import", '\0', POPT_ARG_VAL,	&_nix.op, opImport,
	N_("import a path from a Nix archive, and register as valid"), NULL },
 { "print-fixed-path", '\0', POPT_ARG_VAL,	&_nix.op, opPrintFixedPath,
	N_("FIXME"), NULL },
 { "dump-db", '\0', POPT_ARG_VAL,	&_nix.op, opDumpDB,
	N_("FIXME"), NULL },
 { "load-db", '\0', POPT_ARG_VAL,	&_nix.op, opLoadDB,
	N_("FIXME"), NULL },
 { "init", '\0', POPT_ARG_VAL,		&_nix.op, opInit,
	N_("FIXME"), NULL },
 { "verify", '\0', POPT_ARG_VAL,	&_nix.op, opVerify,
	N_("verify Nix structures"), NULL },
 { "optimise", '\0', POPT_ARG_VAL,	&_nix.op, opOptimise,
	N_("optimise the Nix store by hard-linking identical files"), NULL },

 { "query", 'q', POPT_ARG_VAL,		&_nix.op, opQuery,
	N_("query information"), NULL },
 { "outputs", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_OUTPUTS,
	N_("query the output paths of a Nix derivation (default)"), NULL },
 { "requisites", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_REQUISITES,
	N_("print all paths necessary to realise the path"), NULL },
 { "references", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_REFERENCES,
	N_("print all paths referenced by the path"), NULL },
 { "referrers", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_REFERRERS,
	N_("print all paths directly referring to the path"), NULL },
 { "referrers-closure", '\0', POPT_BIT_SET,	&_nix.qf, RPMNIX_QF_REFERRERS_CLOSURE,
	N_("print all paths (in)directly referring to the path"), NULL },
 { "tree", '\0', POPT_BIT_SET,			&_nix.qf, RPMNIX_QF_TREE,
	N_("print a tree showing the dependency graph of the path"), NULL },
 { "graph", '\0', POPT_BIT_SET,			&_nix.qf, RPMNIX_QF_GRAPH,
	N_("print a dot graph rooted at given path"), NULL },
 { "hash", '\0', POPT_BIT_SET,			&_nix.qf, RPMNIX_QF_HASH,
	N_("print the SHA-256 hash of the contents of the path"), NULL },
 { "roots", '\0', POPT_BIT_SET,			&_nix.qf, RPMNIX_QF_ROOTS,
	N_("print the garbage collector roots that point to the path"), NULL },
 { "use-output", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_USE_OUTPUT,
	N_("perform query on output of derivation, not derivation itself"), NULL },
 { "force-realise", '\0', POPT_BIT_SET,		&_nix.qf, RPMNIX_QF_FORCE_REALISE,
	N_("realise the path before performing the query"), NULL },
 { "include-outputs", '\0', POPT_BIT_SET,	&_nix.qf, RPMNIX_QF_INCLUDE_OUTPUTS,
	N_("in `-R' on a derivation, include requisites of outputs"), NULL },

 { "gc", '\0', POPT_ARG_VAL,		&_nix.op, opGC,
	N_("run the garbage collector"), NULL },
 { "print-roots", '\0', POPT_BIT_SET,		&_nix.gc, RPMNIX_GC_PRINT_ROOTS,
	N_("print GC roots and exit"), NULL },
 { "print-live", '\0', POPT_BIT_SET,		&_nix.gc, RPMNIX_GC_PRINT_LIVE,
	N_("print live paths and exit"), NULL },
 { "print-dead", '\0', POPT_BIT_SET,		&_nix.gc, RPMNIX_GC_PRINT_DEAD,
	N_("print dead paths and exit"), NULL },
/* XXX conflict with --delete */
 { "gcdelete", '\0', POPT_BIT_SET,		&_nix.gc, RPMNIX_GC_DELETE,
	N_("delete dead paths (default)"), NULL },

#ifdef	NOTYET
 { "log-type", '\0', POPT_ARG_STRING,		&_mix.logType, 0,
	N_("FIXME"), N_("TYPE") },
#endif

/* XXX needs next 2 args */
 { "option", '\0', POPT_ARG_NONE,			0, NIX_OPTION,
	N_("FIXME"), N_("OPT1 OPT2") },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif
 { "verbose", 'v', POPT_ARG_NONE,			0, NIX_VERBOSE,
	N_("verbose operation (may be repeated)"), NULL },
 { "version", '\0', POPT_ARG_NONE,			0, NIX_VERSION,
	N_("output version information"), NULL },

  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: nix-store [OPTIONS...] [ARGUMENTS...]\n\
\n\
`nix-store' is a tool to manipulate the Nix store.\n\
\n\
Operations:\n\
\n\
  --realise / -r: ensure path validity; if a derivation, ensure the\n\
      validity of the outputs\n\
  --add / -A: copy a path to the Nix store\n\
  --delete: safely delete paths from the Nix store\n\
  --query / -q: query information\n\
  --read-log / -l: print build log of given store paths\n\
\n\
  --register-validity: register path validity (dangerous!)\n\
  --check-validity: check path validity\n\
\n\
  --gc: run the garbage collector\n\
\n\
  --dump: dump a path as a Nix archive, forgetting dependencies\n\
  --restore: restore a path from a Nix archive, without\n\
      registering validity\n\
\n\
  --export: export a path as a Nix archive, marking dependencies\n\
  --import: import a path from a Nix archive, and register as \n\
      valid\n\
\n\
  --verify: verify Nix structures\n\
  --optimise: optimise the Nix store by hard-linking identical files\n\
\n\
  --version: output version information\n\
  --help: display help\n\
\n\
Query flags:\n\
\n\
  --outputs: query the output paths of a Nix derivation (default)\n\
  --requisites / -R: print all paths necessary to realise the path\n\
  --references: print all paths referenced by the path\n\
  --referrers: print all paths directly referring to the path\n\
  --referrers-closure: print all paths (in)directly referring to the path\n\
  --tree: print a tree showing the dependency graph of the path\n\
  --graph: print a dot graph rooted at given path\n\
  --hash: print the SHA-256 hash of the contents of the path\n\
  --roots: print the garbage collector roots that point to the path\n\
\n\
Query switches (not applicable to all queries):\n\
\n\
  --use-output: perform query on output of derivation, not derivation itself\n\
  --force-realise: realise the path before performing the query\n\
  --include-outputs: in `-R' on a derivation, include requisites of outputs\n\
\n\
Garbage collector options:\n\
\n\
  --print-roots: print GC roots and exit\n\
  --print-live: print live paths and exit\n\
  --print-dead: print dead paths and exit\n\
  --delete: delete dead paths (default)\n\
    \n\
Options:\n\
\n\
  --verbose / -v: verbose operation (may be repeated)\n\
  --keep-failed / -K: keep temporary directories of failed builds\n\
\n\
  --add-root: add garbage collector roots for the result\n\
"), NULL },

  POPT_TABLEEND
};

/*==============================================================*/

int
rpmnixWorker(rpmnix nix)
{
    int ac = 0;
    ARGV_t av = rpmnixArgv(nix, &ac);
    int ec = 0;

NIXDBG((stderr, "--> %s(%p) argv %p[%u]\n", __FUNCTION__, nix, av, (unsigned)ac));
argvPrint(__FUNCTION__, av, NULL);

    return ec;
}

/*==============================================================*/

static void rpmnixFini(void * _nix)
	/*@globals fileSystem @*/
	/*@modifies *_nix, fileSystem @*/
{
    rpmnix nix = _nix;

NIXDBG((stderr, "==> %s(%p)\n", __FUNCTION__, nix));

    nix->tmpPath = _free(nix->tmpPath);
    nix->manifestsPath = _free(nix->manifestsPath);
    nix->rootsPath = _free(nix->rootsPath);

    nix->url = _free(nix->url);

    /* nix-build */
    nix->outLink = _free(nix->outLink);
    nix->drvLink = _free(nix->drvLink);
    nix->instArgs = argvFree(nix->instArgs);
    nix->buildArgs = argvFree(nix->buildArgs);
    nix->exprs = argvFree(nix->exprs);

    /* nix-channel */
    nix->channels = argvFree(nix->channels);
    nix->nixDefExpr = _free(nix->nixDefExpr);
    nix->channelsList = _free(nix->channelsList);
    nix->channelCache = _free(nix->channelCache);

    /* nix-collect-garbage */
    nix->profilesPath = _free(nix->profilesPath);

    /* nix-copy-closure */
    nix->missing = argvFree(nix->missing);
    nix->allStorePaths = argvFree(nix->allStorePaths);
    nix->storePaths = argvFree(nix->storePaths);
    nix->sshHost = _free(nix->sshHost);

    /* nix-install-package */
    nix->profiles = argvFree(nix->profiles);

    /* nix-prefetch-url */
    nix->cacheFlags = _free(nix->cacheFlags);
    nix->hash = _free(nix->hash);
    nix->finalPath = _free(nix->finalPath);
    nix->tmpFile = _free(nix->tmpFile);
    nix->name = _free(nix->name);

    /* nix-push */
    nix->curl = _free(nix->curl);
    nix->manifest = _free(nix->manifest);
    nix->nixExpr = _free(nix->nixExpr);

    nix->av = argvFree(nix->av);
    if (nix->con)
	nix->con = poptFreeContext(nix->con);
    nix->con = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmnixPool;

static rpmnix rpmnixGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmnixPool, fileSystem @*/
	/*@modifies pool, _rpmnixPool, fileSystem @*/
{
    rpmnix nix;

    if (_rpmnixPool == NULL) {
	_rpmnixPool = rpmioNewPool("nix", sizeof(*nix), -1, _rpmnix_debug,
			NULL, NULL, rpmnixFini);
	pool = _rpmnixPool;
    }
    nix = (rpmnix) rpmioGetPool(pool, sizeof(*nix));
    memset(((char *)nix)+sizeof(nix->_item), 0, sizeof(*nix)-sizeof(nix->_item));
    return nix;
}

#ifdef	NOTYET
static rpmnix rpmnixI(void)
	/*@globals _rpmnixI @*/
	/*@modifies _rpmnixI @*/
{
    if (_rpmnixI == NULL) {
	_rpmnixI = rpmnixNew(NULL, 0, NULL);
    }
NIXDBG((stderr, "<== %s() _rpmnixI %p\n", __FUNCTION__, _rpmnixI));
    return _rpmnixI;
}
#endif

const char ** rpmnixArgv(rpmnix nix, int * argcp)
{
#ifdef	DYING
    const char ** av = (nix->con ? poptGetArgs(nix->con) : NULL);
#else
    const char ** av = nix->av;
#endif

    if (argcp)
	*argcp = argvCount(av);
    return av;
}

static void rpmnixInitPopt(rpmnix nix, int ac, char ** av, poptOption tbl)
	/*@modifies nix @*/
{
    void *use =  nix->_item.use;
    void *pool = nix->_item.pool;
    char * av1 = NULL;
    poptContext con;
    int rc;

NIXDBG((stderr, "==> %s(%p, %p[%u], %p)\n", __FUNCTION__, nix, av, (unsigned)ac, tbl));
    if (av == NULL || av[0] == NULL || av[1] == NULL)
	goto exit;

    /* Transform argv[0] from "nix foo" to "nix-foo" invocation. */
    if (!strcmp(av[0], "nix")) {
	size_t nb = sizeof("nix-") + strlen(av[1]);
	/* Save (and replace) argv[1] on "nix build" invocation. */
	av1 = av[1];
	av[1] = xmalloc(nb);
	(void) stpcpy( stpcpy(av[1], "nix-"), av1);
	av++;
    }

    /* Assign the correct popt table based on "*nix-foo" argv[0] */
    if (tbl == NULL) {
	char * fn = xstrdup(av[0]);
	char * bn = basename(fn);
	if (!strncmp(bn, "lt-", sizeof("lt-")-1))
	    bn += sizeof("lt-") - 1;
	if (!strcmp(bn, "nix-build"))
	    tbl = _rpmnixBuildOptions;
	else if (!strcmp(bn, "nix-channel"))
	    tbl = _rpmnixChannelOptions;
	else if (!strcmp(bn, "nix-collect-garbage"))
	    tbl = _rpmnixCollectGarbageOptions;
	else if (!strcmp(bn, "nix-copy-closure"))
	    tbl = _rpmnixCopyClosureOptions;
	else if (!strcmp(bn, "nix-echo") || !strcmp(bn, "xiu-echo"))
	    tbl = _rpmnixEchoOptions;
	else if (!strcmp(bn, "nix-env"))
	    tbl = _rpmnixEchoOptions;	/* XXX NOTYET */
	else if (!strcmp(bn, "nix-hash") || !strcmp(bn, "xiu-hash"))
	    tbl = _rpmnixHashOptions;
	else if (!strcmp(bn, "nix-install-package"))
	    tbl = _rpmnixInstallPackageOptions;
	else if (!strcmp(bn, "nix-instantiate") || !strcmp(bn, "xiu-instantiate"))
	    tbl = _rpmnixInstantiateOptions;
	else if (!strcmp(bn, "nix-prefetch-url"))
	    tbl = _rpmnixPrefetchUrlOptions;
	else if (!strcmp(bn, "nix-pull"))
	    tbl = _rpmnixPullOptions;
	else if (!strcmp(bn, "nix-push"))
	    tbl = _rpmnixPushOptions;
	else if (!strcmp(bn, "nix-store") || !strcmp(bn, "xiu-store"))
	    tbl = _rpmnixStoreOptions;
	else if (!strcmp(bn, "nix-worker"))
	    tbl = _rpmnixEchoOptions;	/* XXX NOTYET */
	else
	    tbl = _rpmnixEchoOptions;
	fn = _free(fn);
    }

    /* Process all options into _nix, whine if unknown options. */
    con = poptGetContext(av[0], ac, (const char **)av, tbl, 0);
    while ((rc = poptGetNextOpt(con)) > 0) {
	const char * arg = poptGetOptArg(con);
	arg = _free(arg);
	switch (rc) {
	default:
		fprintf(stderr, _("%s: option table misconfigured (%d)\n"),
                __FUNCTION__, rc);
		break;
	}
    }
    /* XXX FIXME: arrange error return iff rc < -1. */
if (rc != 0)
fprintf(stderr, "\tpoptGetNextOpt loop end: rc(%d): %s\n", rc, poptStrerror(rc));

    *nix = _nix;	/* structure assignment */
    memset(&_nix, 0, sizeof(_nix));
    nix->_item.use = use;
    nix->_item.pool = pool;
    rc = argvAppend(&nix->av, poptGetArgs(con));
#ifdef	DYING
    nix->con = (void *) con;
#else
    con = poptFreeContext(con);
#endif

    /* Restore argv[1] (if modified by "nix foo" invocation). */
    if (av1) {
	av--;
	av[1] = _free(av[1]);
	av[1] = av1;
    }

exit:
NIXDBG((stderr, "<== %s(%p, %p[%u], %p)\n", __FUNCTION__, nix, av, (unsigned)ac, tbl));
}

static void rpmnixInitEnv(rpmnix nix)
	/*@modifies nix @*/
{
    const char * s;

    s = getenv("TMPDIR");		nix->tmpDir = (s ? s : "/tmp");

    s = getenv("HOME");			nix->homeDir = (s ? s : "~");
    s = getenv("NIX_BIN_DIR");		nix->binDir = (s ? s : "/usr/bin");
    s = getenv("NIX_DATA_DIR");		nix->dataDir = (s ? s : "/usr/share");
    s = getenv("NIX_LIBEXEC_DIR");	nix->libexecDir = (s ? s : "/usr/libexec");

    s = getenv("NIX_STORE_DIR");	nix->storeDir = (s ? s : "/nix/store");
    s = getenv("NIX_STATE_DIR");	nix->stateDir = (s ? s : "/nix/var/nix");

    s = getenv("NIX_MANIFESTS_DIR");
    if (s)
	nix->manifestsPath = rpmGetPath(s, NULL);
    else
	nix->manifestsPath = rpmGetPath(nix->stateDir, "/manifests", NULL);
    /* XXX NIX_ROOTS_DIR? */
    nix->rootsPath = rpmGetPath(nix->stateDir, "/gcroots", NULL);
    /* XXX NIX_PROFILES_DIR? */
    nix->profilesPath = rpmGetPath(nix->stateDir, "/profiles", NULL);

#ifdef	NOTYET
    s = getenv("NIX_HAVE_TERMINAL");

    /* Hack to support the mirror:// scheme from Nixpkgs. */
    s = getenv("NIXPKGS_ALL");

    s = getenv("CURL_FLAGS");
#endif

    /* XXX nix-prefetch-url */
    s = getenv("QUIET");		nix->quiet = (s && *s ? 1 : 0);
    s = getenv("PRINT_PATHS");		nix->print = (s && *s ? 1 : 0);
    s = getenv("NIX_HASH_ALGO");	nix->hashAlgo = (s ? s : "sha256");
    s = getenv("NIX_DOWNLOAD_CACHE");	nix->downloadCache = (s ? s : NULL);
}

rpmnix rpmnixNew(char ** av, uint32_t flags, void * _tbl)
{
    rpmnix nix = rpmnixGetPool(_rpmnixPool);
    int ac = argvCount((ARGV_t)av);

NIXDBG((stderr, "==> %s(%p[%u], 0x%x, %p)\n", __FUNCTION__, av, (unsigned)ac, flags, _tbl));

    _nix.flags = flags;		/* XXX set command peculier defaults. */
    rpmnixInitPopt(nix, ac, av, (poptOption) _tbl);
    rpmnixInitEnv(nix);

    return rpmnixLink(nix);
}

#ifdef	NOTYET
rpmRC rpmnixRun(rpmnix nix, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

    if (nix == NULL) nix = rpmnixI();

    if (str != NULL) {
    }

NIXDBG((stderr, "<== %s(%p,%p[%u]) rc %d\n", __FUNCTION__, nix, str, (unsigned)(str ? strlen(str) : 0), rc));

    return rc;
}
#endif
