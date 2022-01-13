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
};

/**
 */
static struct rpmnix_s _nix = {
	.flags = RPMNIX_FLAGS_NOOUTLINK
};

/*==============================================================*/

static int verbose = 0;

#define	_BASE	0x40000000
enum {
    NIX_ARG		= _BASE + 4,	/*    --arg ARG1 ARG2 */
    NIX_LOG_TYPE	= _BASE + 5,	/*    --log-type TYPE */
    NIX_OPTION		= _BASE + 6,	/*    --option OPT1 OPT2 */
    NIX_VERBOSE		= _BASE + 10,	/* -v,--verbose */
    NIX_VERSION		= _BASE + 21,	/*    --version */
};

static void rpmnixInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
#ifdef	UNUSED
    rpmnix nix = &_nix;
#endif

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
	verbose++;
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

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmioInit(argc, argv, _rpmnixInstantiateOptions);
    int ec = 1;		/* assume failure */
#ifdef	UNUSED
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    rpmnix nix = &_nix;
    int xx;
#endif

    ec = 0;	/* XXX success */

    optCon = rpmioFini(optCon);

    return ec;
}
