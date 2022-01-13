#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>
#include <poptIO.h>

#include "debug.h"

#define _KFB(n) (1U << (n))
#define _DFB(n) (_KFB(n) | 0x40000000)

#define F_ISSET(_nix, _FLAG) ((_nix)->flags & ((RPMNIX_FLAGS_##_FLAG) & ~0x40000000))

/**
 * Bit field enum for rpmdigest CLI options.
 */
enum nixFlags_e {
    RPMNIX_FLAGS_NONE		= 0,
    RPMNIX_FLAGS_ADDDRVLINK	= _DFB(0),	/*    --add-drv-link */
    RPMNIX_FLAGS_NOOUTLINK	= _DFB(1),	/* -o,--no-out-link */
    RPMNIX_FLAGS_DRYRUN		= _DFB(2),	/*    --dry-run */

    RPMNIX_FLAGS_EVALONLY	= _DFB(16),	/*    --eval-only */
    RPMNIX_FLAGS_PARSEONLY	= _DFB(17),	/*    --parse-only */
    RPMNIX_FLAGS_ADDROOT	= _DFB(18),	/*    --add-root */
    RPMNIX_FLAGS_XML		= _DFB(19),	/*    --xml */
    RPMNIX_FLAGS_STRICT		= _DFB(20),	/*    --strict */
    RPMNIX_FLAGS_SHOWTRACE	= _DFB(21),	/*    --show-trace */
};

enum nixQF_e {
    NIX_QF_OUTPUTS		= (1 <<  1),	/*    --outputs */
    NIX_QF_REQUISITES		= (1 <<  2),	/*    --requisites */
    NIX_QF_REFERENCES		= (1 <<  3),	/*    --references */
    NIX_QF_REFERRERS		= (1 <<  4),	/*    --referrers */
    NIX_QF_REFERRERS_CLOSURE	= (1 <<  5),	/*    --referrers-closure */
    NIX_QF_TREE			= (1 <<  6),	/*    --tree */
    NIX_QF_GRAPH		= (1 <<  7),	/*    --graph */
    NIX_QF_HASH			= (1 <<  8),	/*    --hash */
    NIX_QF_ROOTS		= (1 <<  9),	/*    --roots */
    NIX_QF_USE_OUTPUT		= (1 << 10),	/*    --use-output */
    NIX_QF_FORCE_REALISE	= (1 << 11),	/*    --force-realise */
    NIX_QF_INCLUDE_OUTPUTS	= (1 << 12),	/*    --include-outputs */
};

enum nixGC_e {
    NIX_GC_PRINT_ROOTS		= (1 <<  1),	/*    --print-roots */
    NIX_GC_PRINT_LIVE		= (1 <<  2),	/*    --print-live */
    NIX_GC_PRINT_DEAD		= (1 <<  3),	/*    --print-dead */
    NIX_GC_DELETE		= (1 <<  4),	/*    --delete */
};

/**
 */
typedef struct rpmnix_s * rpmnix;

/**
 */
struct rpmnix_s {
    enum nixFlags_e flags;	/*!< rpmnix control bits. */

    const char * outLink;
    const char * drvLink;

    const char ** instArgs;
    const char ** buildArgs;
    const char ** exprs;

    const char * attr;

    int op;
    int jobs;
    enum nixQF_e qf;
    enum nixGC_e gc;
};

/**
 */
static struct rpmnix_s _nix = {
	.flags = RPMNIX_FLAGS_NOOUTLINK
};

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
 
/*==============================================================*/

static int verbose = 0;

#define	_BASE	0x40000000
enum {
    NIX_LOG_TYPE	= _BASE + 5,	/*    --log-type TYPE */
    NIX_OPTION		= _BASE + 6,	/*    --option OPT1 OPT2 */
    NIX_VERBOSE		= _BASE + 10,	/* -v,--verbose */
    NIX_VERSION		= _BASE + 21,	/*    --version */
};

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
    case NIX_LOG_TYPE:			/*    --log-type TYPE */
#ifdef	DYING
	s = rpmExpand("--", opt->longName, NULL);
	xx = argvAdd(&nix->buildArgs, s);
	xx = argvAdd(&nix->buildArgs, arg);
	s = _free(s);
#endif
	break;

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
 { "outputs", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_OUTPUTS,
	N_("query the output paths of a Nix derivation (default)"), NULL },
 { "requisites", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_REQUISITES,
	N_("print all paths necessary to realise the path"), NULL },
 { "references", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_REFERENCES,
	N_("print all paths referenced by the path"), NULL },
 { "referrers", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_REFERRERS,
	N_("print all paths directly referring to the path"), NULL },
 { "referrers-closure", '\0', POPT_BIT_SET,	&_nix.qf, NIX_QF_REFERRERS_CLOSURE,
	N_("print all paths (in)directly referring to the path"), NULL },
 { "tree", '\0', POPT_BIT_SET,			&_nix.qf, NIX_QF_TREE,
	N_("print a tree showing the dependency graph of the path"), NULL },
 { "graph", '\0', POPT_BIT_SET,			&_nix.qf, NIX_QF_GRAPH,
	N_("print a dot graph rooted at given path"), NULL },
 { "hash", '\0', POPT_BIT_SET,			&_nix.qf, NIX_QF_HASH,
	N_("print the SHA-256 hash of the contents of the path"), NULL },
 { "roots", '\0', POPT_BIT_SET,			&_nix.qf, NIX_QF_ROOTS,
	N_("print the garbage collector roots that point to the path"), NULL },
 { "use-output", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_USE_OUTPUT,
	N_("perform query on output of derivation, not derivation itself"), NULL },
 { "force-realise", '\0', POPT_BIT_SET,		&_nix.qf, NIX_QF_FORCE_REALISE,
	N_("realise the path before performing the query"), NULL },
 { "include-outputs", '\0', POPT_BIT_SET,	&_nix.qf, NIX_QF_INCLUDE_OUTPUTS,
	N_("in `-R' on a derivation, include requisites of outputs"), NULL },

 { "gc", '\0', POPT_ARG_VAL,		&_nix.op, opGC,
	N_("run the garbage collector"), NULL },
 { "print-roots", '\0', POPT_BIT_SET,		&_nix.gc, NIX_GC_PRINT_ROOTS,
	N_("print GC roots and exit"), NULL },
 { "print-live", '\0', POPT_BIT_SET,		&_nix.gc, NIX_GC_PRINT_LIVE,
	N_("print live paths and exit"), NULL },
 { "print-dead", '\0', POPT_BIT_SET,		&_nix.gc, NIX_GC_PRINT_DEAD,
	N_("print dead paths and exit"), NULL },
/* XXX conflict with --delete */
 { "gcdelete", '\0', POPT_BIT_SET,		&_nix.gc, NIX_GC_DELETE,
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

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmioInit(argc, argv, _rpmnixStoreOptions);
    int ec = 1;		/* assume failure */
#ifdef	UNUSED
    rpmnix nix = &_nix;
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    int xx;
    int i;
#endif

    ec = 0;	/* XXX success */

    optCon = rpmioFini(optCon);

    return ec;
}
