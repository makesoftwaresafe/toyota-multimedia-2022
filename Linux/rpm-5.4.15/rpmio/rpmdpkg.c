#include "system.h"

#include <rpmio.h>
#ifdef	DYING
#include <poptIO.h>
#define	rpmcliInit		rpmioInit
#define	rpmcliFini		rpmioFini
#define	rpmcliAllPoptTable	rpmioAllPoptTable
#
#else
#include <argv.h>
#include <rpmcli.h>
#endif

#include "debug.h"

/*==============================================================*/

#define _KFB(n) (1U << (n))
#define _DFB(n) (_KFB(n) | 0x40000000)

/**
 * Bit field enum for rpmdpkg CLI options.
 */
enum dpkgFlags_e {
    DPKG_FLAGS_NONE		= 0,

    DPKG_FLAGS_SELECTEDONLY	= _DFB( 0), /*!< -O,--selected-only ... */
    DPKG_FLAGS_SKIPSAMEVERSION	= _DFB( 1), /*!< -E,--skip-same-version ... */
    DPKG_FLAGS_NODOWNGRADE	= _DFB( 2), /*!< -G,--refuse-downgrade ... */
    DPKG_FLAGS_DECONFIGURE	= _DFB( 3), /*!< -B,--auto-deconfigure ... */
    DPKG_FLAGS_NOTRIGGERS	= _DFB( 4), /*!<    --no-triggers ... */
    DPKG_FLAGS_NODEBSIG		= _DFB( 5), /*!<    --no-debsig ... */
    DPKG_FLAGS_DRYRUN		= _DFB( 6), /*!<    --dry-run ... */
    DPKG_FLAGS_FORCE		= _DFB( 7), /*!<    --force ... */
    DPKG_FLAGS_PENDING		= _DFB( 8), /*!< -a,--pending ... */
    DPKG_FLAGS_NOSELECTALSO	= _DFB( 9), /*!<    --no-also-select ... */

};

/**
 */
typedef struct rpmdpkg_s * rpmdpkg;

/**
 */
struct rpmdpkg_s {
    const char * admindir;	/*!<    --admindir ... */
    const char * rootdir;	/*!<    --root ... */
    const char * instdir;	/*!<    --instdir ... */

    int debug;			/*!< -D,--debug ... */
    int command_fd;		/*!< -c,--command-fd ... */
    int status_fd;		/*!<    --status-fd ... */
    const char * log_file;	/*!<    --log ... */
    const char **ignore_depends;/*!<    --ignore-depends ... */
    int errabort;		/*!<    --abort-after ... */

    int verbose;		/*!< -v,--verbose ... */

    const char ** recursedirs;	/*!< -R,--recursive ... */

};

#define	ADMINDIR	"?ADMINDIR?"
/*@unchecked@*/
static struct rpmdpkg_s __rpmdpkg = {
    .admindir	= ADMINDIR
};
/*@unchecked@*/
static rpmdpkg _rpmdpkg = &__rpmdpkg;

/*@unchecked@*/
enum dpkgFlags_e dpkgFlags = DPKG_FLAGS_NONE;

/*==============================================================*/
#define POPT_DPKG_VERYVERBOSE		-1500
#define	POPT_DPKG_UNPACK		 1
#define	POPT_DPKG_CONFIGURE		 2
#define	POPT_DPKG_TRIGGERS_ONLY		 3
#define	POPT_DPKG_GET_SELECTIONS	 4
#define	POPT_DPKG_SET_SELECTIONS	 5
#define	POPT_DPKG_CLEAR_SELECTIONS	 6
#define	POPT_DPKG_UPDATE_AVAIL		 7
#define	POPT_DPKG_MERGE_AVAIL		 8
#define	POPT_DPKG_CLEAR_AVAIL		 9
#define	POPT_DPKG_FORGET_OLD_UNAVAIL	10
#define	POPT_DPKG_YET_TO_UNPACK		11
#define	POPT_DPKG_ASSERT_SUPPORT_PREDEPENDS	12
#define	POPT_DPKG_ASSERT_WORKING_EPOCH	13
#define	POPT_DPKG_ASSERT_LONG_FILENAMES	14
#define	POPT_DPKG_ASSERT_MULTI_CONREP	15
#define	POPT_DPKG_PRINT_ARCH		16
#define	POPT_DPKG_PRINT_INSTALL_ARCH	17
#define	POPT_DPKG_PREDEP_PACKAGE	18
#define	POPT_DPKG_COMPARE_VERSIONS	19
#define	POPT_DPKG_LICENSE		20

/**
 */
static void rpmdpkgArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmdpkg dpkg = _rpmdpkg;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'v':			/* -v,--verbose */
	dpkg->verbose++;
	break;
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/

#define	POPT_XXX	0

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmdpkgCommandsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmdpkgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "install", 'i', POPT_ARG_STRING, NULL, 'i',
        N_("?install?"), N_("<.deb file name>") },
  { "unpack", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_UNPACK,
        N_("?unpack?"), N_("<.deb file name>") },
  { "record-avail", 'A', POPT_ARG_STRING, NULL, 'A',
        N_("?record-avail?"), N_("<.deb file name>") },
  { "configure", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_CONFIGURE,
        N_("?configure?"), N_("<package>") },
  { "remove", 'r', POPT_ARG_STRING, NULL, 'r',
        N_("?remove?"), N_("<package>") },
  { "purge", 'p', POPT_ARG_STRING, NULL, 'p',
        N_("?purge?"), N_("<package>") },
  { "triggers-only", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_TRIGGERS_ONLY,
        N_("?triggers-only?"), N_("<package>") },

  { "listfiles", 'L', POPT_ARG_STRING, NULL, 'L',
        N_("List files `owned' by package(s)."), N_("<package>") },
  { "status", 's', POPT_ARG_STRING, NULL, 's',
        N_("Display package status details."), N_("<package>") },

 	/* XXX optional pattern */
  { "get-selections", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_GET_SELECTIONS,
        N_("Get list of selections to stdout."), N_("<pattern>") },
  { "set-selections", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_SET_SELECTIONS,
        N_("Set package selections from stdin."), NULL },
  { "clear-selections", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_CLEAR_SELECTIONS,
        N_("Deselect every non-essential package."), NULL },

  { "print-avail", 'p', POPT_ARG_STRING, NULL, 'p',
        N_("Display available version details."), N_("<package>") },
  { "update-avail", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_UPDATE_AVAIL,
        N_("Replace available packages info."), N_("<Packages-file>") },
  { "merge-avail", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_MERGE_AVAIL,
        N_("Merge with info from file."), N_("<Packages-file>") },
  { "clear-avail", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_CLEAR_AVAIL,
        N_("Erase existing available info."), NULL },
  { "forget-old-unavail", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_FORGET_OLD_UNAVAIL,
        N_("Forget uninstalled unavailable pkgs."), NULL },

  { "audit", 'C', POPT_ARG_NONE, NULL, 'C',
        N_("Check for broken package(s)."), NULL },
  { "yet-to-unpack", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_YET_TO_UNPACK,
        N_("?yet-to-unpack?"), NULL },
 	/* XXX optional pattern */
  { "list", 'l', POPT_ARG_STRING, NULL, 'l',
        N_("List packages concisely."), N_("<pattern>") },
  { "search", 'S', POPT_ARG_STRING, NULL, 'S',
        N_("Find package(s) owning file(s)."), N_("<pattern>") },

  { "assert-support-predepends", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_ASSERT_SUPPORT_PREDEPENDS,
        N_("?assert-support-predepends?"), NULL },
  { "assert-working-epoch", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_ASSERT_WORKING_EPOCH,
        N_("?assert-working-epoch?"), NULL },
  { "assert-long-filenames", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_ASSERT_LONG_FILENAMES,
        N_("?assert-long-filenames?"), NULL },
  { "assert-multi-conrep", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_ASSERT_MULTI_CONREP,
        N_("?assert-multi-conrep?"), NULL },
  { "print-architecture", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_PRINT_ARCH,
        N_("Print dpkg architecture."), NULL },
  { "print-installation-architecture", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_PRINT_INSTALL_ARCH,
        N_("Print dpkg installation architecture."), NULL },
  { "predep-package", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_PREDEP_PACKAGE,
        N_("?predep-package?"), NULL },

  { "compare-versions", '\0', POPT_ARG_STRING, NULL, POPT_DPKG_COMPARE_VERSIONS,
	N_("Compare version numbers - see below."), N_("<a> <op> <b>") },

#ifdef	NOTYET
  { "help",              'h', 0, NULL,          NULL,      helponly,      0 },
  { "version",           0,   0, NULL,          NULL,      versiononly,   0 },
  /* UK spelling. */
  { "licence",           0,   0, NULL,          NULL,      showcopyright, 0 },
  /* US spelling. */
  { "license",           0,   0, NULL,          NULL,      showcopyright, 0 },
  ACTIONBACKEND( "build",               'b', BACKEND),
  ACTIONBACKEND( "contents",            'c', BACKEND),
  ACTIONBACKEND( "control",             'e', BACKEND),
  ACTIONBACKEND( "info",                'I', BACKEND),
  ACTIONBACKEND( "field",               'f', BACKEND),
  ACTIONBACKEND( "extract",             'x', BACKEND),
  ACTIONBACKEND( "new",                 0,  BACKEND),
  ACTIONBACKEND( "old",                 0,  BACKEND),
  ACTIONBACKEND( "vextract",            'X', BACKEND),
  ACTIONBACKEND( "fsys-tarfile",        0,   BACKEND),
#endif

#ifdef	REFERENCE

  --force-help                     Show help on forcing.\n\
  -Dh|--debug=help                 Show help on debugging.\n\
\n\
  -h|--help                        Show this help message.\n\

#endif

  { "license", '\0', POPT_ARG_NONE, NULL, POPT_DPKG_LICENSE,
	N_("Show the copyright licensing terms."), NULL },
  { "licence", '\0', POPT_ARG_NONE|POPT_ARGFLAG_DOC_HIDDEN, NULL, POPT_DPKG_LICENSE,
	N_("Show the copyright licensing terms."), NULL },
  { "version", '\0', POPT_ARG_NONE, NULL, 'v',
	N_("Show the version."), NULL },

  POPT_TABLEEND

};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmdpkgOptionsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmdpkgArgCallback, 0, NULL, NULL },
/*@=type@*/

	/* XXX default value */
  { "admindir", '\0', POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &__rpmdpkg.admindir, 0,
        N_("Use <directory> for ADMINDIR."), N_("<directory>") },
  { "root", '\0', POPT_ARG_STRING, &__rpmdpkg.rootdir, 0,
        N_("Install on a different root directory."), N_("<directory>") },
  { "instdir", '\0', POPT_ARG_STRING, &__rpmdpkg.instdir, 0,
        N_("Change installation dir without changing ADMINDIR."), N_("<directory>") },

  { "selected-only", 'O', POPT_BIT_SET,	&dpkgFlags, DPKG_FLAGS_SELECTEDONLY,
        N_("Skip packages not selected for install/upgrade."), NULL },
  { "skip-same-version", 'E', POPT_BIT_SET, &dpkgFlags, DPKG_FLAGS_SKIPSAMEVERSION,
        N_("Skip packages whose same version is installed."), NULL },
	/* XXX Alias ('G') for --refuse. */
  { "refuse-downgrade", 'G', POPT_BIT_SET, &dpkgFlags, DPKG_FLAGS_NODOWNGRADE,
        N_("Skip packages with earlier version than installed."), NULL },
  { "auto-deconfigure", 'B', POPT_BIT_SET, &dpkgFlags, DPKG_FLAGS_DECONFIGURE,
        N_("Install even if it would break some other package."), NULL },
	/* XXX enabler needed too. */
  { "no-triggers", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE, &dpkgFlags, DPKG_FLAGS_NOTRIGGERS,
        N_("Skip or force consequential trigger processing."), NULL },
	/* XXX FIXME: Remove ('N') sometime. */
  { "no-also-select", 'N', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &dpkgFlags, DPKG_FLAGS_NOSELECTALSO,
        N_("?no-also-select?"), NULL },
  { "no-debsig", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE, &dpkgFlags, DPKG_FLAGS_NODEBSIG,
        N_("Do not try to verify package signatures."), NULL },
  { "dryrun", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,	&dpkgFlags, DPKG_FLAGS_DRYRUN,
        N_("Just say what we would do - don't do it."), NULL },
  { "no-act", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN, &dpkgFlags, DPKG_FLAGS_DRYRUN,
        N_("Just say what we would do - don't do it."), NULL },
  { "simulate", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &dpkgFlags, DPKG_FLAGS_DRYRUN,
        N_("Just say what we would do - don't do it."), NULL },
  { "debug", 'D', POPT_ARG_INT,		&__rpmdpkg.debug, 0,
        N_("Enable debugging (see -Dhelp or --debug=help)."), N_("<octal>") },
  { "command-fd", 'c', POPT_ARG_INT|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmdpkg.command_fd, 0,
        N_("?command-fd?"), N_("<n>") },
  { "status-fd", '\0', POPT_ARG_INT,	&__rpmdpkg.status_fd, 0,
        N_("Send status change updates to file descriptor <n>."), N_("<n>") },
  { "log", '\0', POPT_ARG_STRING,	&__rpmdpkg.log_file, 0,
        N_("Log status changes and actions to <filename>."), N_("<filename>") },
	/* XXX dpkg uses comma separated list. */
  { "ignore-depends", '\0', POPT_ARG_ARGV, &__rpmdpkg.ignore_depends, 0,
        N_("Ignore dependencies involving <package>."), N_("<package>") },
	/* XXX disabler needed too. POPT_ARGFLAG_TOGGLE? */
  { "force", '\0', POPT_BIT_SET,	&dpkgFlags, DPKG_FLAGS_FORCE,
        N_("Override problems (see --force-help)."), NULL },
	/* XXX disablers needed too. */
  { "no-force", '\0', POPT_BIT_CLR,	&dpkgFlags, DPKG_FLAGS_FORCE,
        N_("Stop when problems encountered."), NULL },
	/* XXX Alias ('G') for --refuse. */
  { "refuse", '\0', POPT_BIT_CLR|POPT_ARGFLAG_DOC_HIDDEN, &dpkgFlags, DPKG_FLAGS_FORCE,
        N_("Stop when problems encountered."), NULL },
  { "abort-after", 'B', POPT_ARG_INT,		&__rpmdpkg.errabort, 0,
        N_("Send status change updates to file descriptor <n>."), N_("<n>") },

  { "recursive", 'R', POPT_ARG_ARGV, &__rpmdpkg.recursedirs, 0,
        N_("?recursive?"), N_("<directory>") },
  { "pending", 'a', POPT_BIT_SET, &dpkgFlags, DPKG_FLAGS_PENDING,
        N_("?pending?"), NULL },

  POPT_TABLEEND

};

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmdpkgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmdpkgCommandsPoptTable, 0,
        N_("Commands:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmdpkgOptionsPoptTable, 0,
        N_("Options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options for all executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Commands:\n\
  -i|--install       <.deb file name> ... | -R|--recursive <directory> ...\n\
  --unpack           <.deb file name> ... | -R|--recursive <directory> ...\n\
  -A|--record-avail  <.deb file name> ... | -R|--recursive <directory> ...\n\
  --configure        <package> ... | -a|--pending\n\
  --triggers-only    <package> ... | -a|--pending\n\
  -r|--remove        <package> ... | -a|--pending\n\
  -P|--purge         <package> ... | -a|--pending\n\
  --get-selections [<pattern> ...] Get list of selections to stdout.\n\
  --set-selections                 Set package selections from stdin.\n\
  --clear-selections               Deselect every non-essential package.\n\
  --update-avail <Packages-file>   Replace available packages info.\n\
  --merge-avail <Packages-file>    Merge with info from file.\n\
  --clear-avail                    Erase existing available info.\n\
  --forget-old-unavail             Forget uninstalled unavailable pkgs.\n\
  -s|--status <package> ...        Display package status details.\n\
  -p|--print-avail <package> ...   Display available version details.\n\
  -L|--listfiles <package> ...     List files `owned' by package(s).\n\
  -l|--list [<pattern> ...]        List packages concisely.\n\
  -S|--search <pattern> ...        Find package(s) owning file(s).\n\
  -C|--audit                       Check for broken package(s).\n\
  --print-architecture             Print dpkg architecture.\n\
  --compare-versions <a> <op> <b>  Compare version numbers - see below.\n\
  --force-help                     Show help on forcing.\n\
  -Dh|--debug=help                 Show help on debugging.\n\
\n\
  -h|--help                        Show this help message.\n\
  --version                        Show the version.\n\
  --license|--licence              Show the copyright licensing terms.\n\
\n\
Use dpkg -b|--build|-c|--contents|-e|--control|-I|--info|-f|--field|\n\
 -x|--extract|-X|--vextract|--fsys-tarfile  on archives (type %s --help).\n\
\n\
For internal use: dpkg --assert-support-predepends | --predep-package |\n\
  --assert-working-epoch | --assert-long-filenames | --assert-multi-conrep.\n\
\n\
Options:\n\
  --admindir=<directory>     Use <directory> instead of %s.\n\
  --root=<directory>         Install on a different root directory.\n\
  --instdir=<directory>      Change installation dir without changing admin dir.\n\
  -O|--selected-only         Skip packages not selected for install/upgrade.\n\
  -E|--skip-same-version     Skip packages whose same version is installed.\n\
  -G|--refuse-downgrade      Skip packages with earlier version than installed.\n\
  -B|--auto-deconfigure      Install even if it would break some other package.\n\
  --[no-]triggers            Skip or force consequential trigger processing.\n\
  --no-debsig                Do not try to verify package signatures.\n\
  --no-act|--dry-run|--simulate\n\
                             Just say what we would do - don't do it.\n\
  -D|--debug=<octal>         Enable debugging (see -Dhelp or --debug=help).\n\
  --status-fd <n>            Send status change updates to file descriptor <n>.\n\
  --log=<filename>           Log status changes and actions to <filename>.\n\
  --ignore-depends=<package>,...\n\
                             Ignore dependencies involving <package>.\n\
  --force-...                Override problems (see --force-help).\n\
  --no-force-...|--refuse-...\n\
                             Stop when problems encountered.\n\
  --abort-after <n>          Abort after encountering <n> errors.\n\
\n\
Comparison operators for --compare-versions are:\n\
  lt le eq ne ge gt       (treat empty version as earlier than any version);\n\
  lt-nl le-nl ge-nl gt-nl (treat empty version as later than any version);\n\
  < << <= = >= >> >       (only for compatibility with control file syntax).\n\
\n\
Use `dselect' or `aptitude' for user-friendly package management.\n\
\n\
Type dpkg --help for help about installing and deinstalling packages [*];\n\
Use `dselect' or `aptitude' for user-friendly package management;\n\
Type dpkg -Dhelp for a list of dpkg debug flag values;\n\
Type dpkg --force-help for a list of forcing options;\n\
Type dpkg-deb --help for help about manipulating *.deb files;\n\
Type dpkg --license for copyright license and lack of warranty (GNU GPL) [*].\n\
\n\
Options marked [*] produce a lot of output - pipe it through `less' or `more' !\n\
"), NULL },

  POPT_TABLEEND

};

int
main(int argc, char *argv[])
	/*@globals __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name, _rpmrepo,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    const char ** av = NULL;
    int ac;
    int rc = 1;		/* assume failure. */
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmdpkg";
/*@=observertrans =readonlytrans @*/

    av = poptGetArgs(optCon);
    if (av == NULL || av[0] == NULL) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }
    ac = argvCount(av);

    if (av != NULL)
    for (i = 0; i < ac; i++) {
    }
    rc = 0;

exit:
    optCon = rpmcliFini(optCon);

    return rc;
}
