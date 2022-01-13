#include "system.h"

#include <rpmio.h>
#include <poptIO.h>

#include "debug.h"

/*==============================================================*/

#define _KFB(n) (1U << (n))
#define _BFB(n) (_KFB(n) | 0x40000000)

/**
 * Bit field enum for rpmborg CLI options.
 */
enum borgFlags_e {
    BORG_FLAGS_NONE		= 0,
    BORG_FLAGS_DEB		= _BFB( 0), /*!< -d,--to-deb ... */
    BORG_FLAGS_RPM		= _BFB( 1), /*!< -r,--to-rpm ... */
    BORG_FLAGS_SLP		= _BFB( 2), /*!<    --to-slp ... */
    BORG_FLAGS_LSB		= _BFB( 3), /*!< -l,--to-lsb ... */
    BORG_FLAGS_TGZ		= _BFB( 4), /*!< -t,--to-tgz ... */
    BORG_FLAGS_PKG		= _BFB( 5), /*!< -p,--to-pkg ... */
    BORG_FLAGS_XAR		= _BFB( 6), /*!< -x,--to-xar ... */
        /* 7-15 unused */
    BORG_FLAGS_INSTALL		= _BFB(16), /*!< -i,--install ... */
    BORG_FLAGS_GENERATE		= _BFB(17), /*!< -g,--generate ... */
    BORG_FLAGS_SCRIPTS		= _BFB(18), /*!< -c,--scripts ... */
    BORG_FLAGS_KEEPVERSION	= _BFB(19), /*!< -k,--keep-version ... */
    BORG_FLAGS_NOPATCH		= _BFB(20), /*!<    --nopatch ... */
    BORG_FLAGS_ANYPATCH		= _BFB(21), /*!<    --anypatch ... */
    BORG_FLAGS_SINGLE		= _BFB(22), /*!< -s,--single ... */
    BORG_FLAGS_FIXPERMS		= _BFB(23), /*!<    --fixperms ... */
    BORG_FLAGS_TEST		= _BFB(24), /*!<    --test ... */
};

/**
 */
typedef struct rpmborg_s * rpmborg;

/**
 */
struct rpmborg_s {
    const char * patch;		/*!<    --patch ... */
    const char * description;	/*!<    --description ... */
    const char * version;	/*!<    --version ... */
    int bump;			/*!<    --bump ... */
    int verbose;		/*!< -v,--verbose ... */
};

/*@unchecked@*/
static struct rpmborg_s __rpmborg;
/*@unchecked@*/
static rpmborg _rpmborg = &__rpmborg;

/*@unchecked@*/
enum borgFlags_e borgFlags = BORG_FLAGS_NONE;

/*==============================================================*/
#define POPT_BORG_VERYVERBOSE	-1500

/**
 */
static void rpmborgArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmborg borg = _rpmborg;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case POPT_BORG_VERYVERBOSE:	/*    --veryverbose */
	borg->verbose++;	
        borg->verbose++;
        break;
    case 'v':                   /* -v,--verbose */
        borg->verbose++;
        break;
    case '?':
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/

#define	POPT_XXX		0

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmborgCommandsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmborgArgCallback, 0, NULL, NULL },
/*@=type@*/

	/* XXX this bit is (default). */
  { "to-deb", 'd', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_DEB,
        N_("Generate a Debian deb package."), NULL },

#ifdef	NOTYET	/* XXX need means for immediate --help display. */
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("    Enables these options:"), NULL },
#endif
  { "patch", '\0', POPT_ARG_STRING,	&__rpmborg.patch, 0,
        N_("Specify patch file to use instead of automatically looking for patch in /var/lib/borg."), N_("<patch>") },
  { "nopatch", '\0', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_NOPATCH,
        N_("Do not use patches."), NULL },
  { "anypatch", '\0', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_ANYPATCH,
        N_("Use even old version os patches."), NULL },
  { "single", 's', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_SINGLE,
        N_("Like --generate, but do not create .orig directory."), NULL },
  { "fixperms", '\0', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_FIXPERMS,
        N_("Munge/fix permissions and owners."), NULL },
  { "test", '\0', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_TEST,
        N_("Test generated packages with lintian."), NULL },

  { "to-rpm", 'r', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_RPM,
        N_("Generate a RPM rpm package."), NULL },
  { "to-slp", '\0', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_SLP,
        N_("Generate a Stampede slp package."), NULL },
  { "to-lsb", 'l', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_LSB,
        N_("Generate a LSB package."), NULL },
  { "to-tgz", 't', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_TGZ,
        N_("Generate a Slackware tgz package."), NULL },
#ifdef	NOTYET	/* XXX need means for immediate --help display. */
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("    Enables these options:"), NULL },
#endif
  { "description", '\0', POPT_ARG_STRING,	&__rpmborg.description, 0,
        N_("Specify package description."), N_("<desc>") },
	/*XXX collides with -V,--version */
  { "version", '\0', POPT_ARG_STRING,	&__rpmborg.version, 0,
        N_("Specify package version."), N_("<version>") },

  { "to-pkg", 'p', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_PKG,
        N_("Generate a Solaris pkg package."), NULL },
  { "to-xar", 'x', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_XAR,
        N_("Generate a XARv1 package."), NULL },

  { "install", 'i', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_INSTALL,
        N_("Install generated package."), NULL },
  { "generate", 'g', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_GENERATE,
        N_("Generate build tree, but do not build package."), NULL },
  { "scripts", 'c', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_SCRIPTS,
        N_("Include scripts in package."), NULL },
  { "keep-version", 'k', POPT_BIT_SET,	&borgFlags, BORG_FLAGS_KEEPVERSION,
        N_("Do not change version of generated package."), NULL },

  { "bump", '\0', POPT_ARG_INT,		&__rpmborg.bump, 0,
        N_("Increment package version by this number."), N_("number") },

 { "verbose", 'v', 0,			NULL, (int)'v',
        N_("Display each command the Borg runs."), NULL },
 { "veryverbose", '\0', 0,		NULL, POPT_BORG_VERYVERBOSE,
        N_("Display each command the Borg runs."), NULL },

#ifdef	REFERENCE
  -d, --to-deb              Generate a Debian deb package (default).
     Enables these options:
       --patch=<patch>      Specify patch file to use instead of automatically
                            looking for patch in /var/lib/borg.
       --nopatch	    Do not use patches.
       --anypatch           Use even old version os patches.
       -s, --single         Like --generate, but do not create .orig
                            directory.
       --fixperms           Munge/fix permissions and owners.
       --test               Test generated packages with lintian.
  -r, --to-rpm              Generate a RPM rpm package.
      --to-slp              Generate a Stampede slp package.
  -l, --to-lsb              Generate a LSB package.
  -t, --to-tgz              Generate a Slackware tgz package.
     Enables these options:
       --description=<desc> Specify package description.
       --version=<version>  Specify package version.
  -p, --to-pkg              Generate a Solaris pkg package.
  -i, --install             Install generated package.
  -g, --generate            Generate build tree, but do not build package.
  -c, --scripts             Include scripts in package.
  -v, --verbose             Display each command the Borg runs.
      --veryverbose         Be verbose, and also display output of run commands.
  -k, --keep-version        Do not change version of generated package.
      --bump=number         Increment package version by this number.
  -h, --help                Display this help message.
  -V, --version		    Display the Borg version number.
#endif

  POPT_TABLEEND

};

#ifdef	NOTYET
/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmborgOptionsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmborgArgCallback, 0, NULL, NULL },
/*@=type@*/

  POPT_TABLEEND

};
#endif

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmborgArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmborgCommandsPoptTable, 0,
        N_("Commands:"), NULL },

#ifdef	NOTYET
  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmborgOptionsPoptTable, 0,
        N_("Options:"), NULL },
#endif

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmborg [options] file [...]\n\
  file [...]                Package file or files to convert.\n\
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
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    const char ** av = NULL;
    int ac;
    int rc = 1;		/* assume failure. */
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmborg";
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
    optCon = rpmioFini(optCon);

    return rc;
}
