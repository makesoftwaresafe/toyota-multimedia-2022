#include "system.h"

#define _MIRE_INTERNAL
#include <poptIO.h>

#include "debug.h"

static rpmMireMode mireMode = RPMMIRE_REGEX;
static int invert = 0;

static struct poptOption optionsTable[] = {

 { "strcmp", '\0', POPT_ARG_VAL,	&mireMode, RPMMIRE_STRCMP,
	N_("use strcmp matching"), NULL},
 { "regex", '\0', POPT_ARG_VAL,		&mireMode, RPMMIRE_REGEX,
	N_("use regex matching"), NULL},
 { "fnmatch", '\0', POPT_ARG_VAL,	&mireMode, RPMMIRE_GLOB,
	N_("use fnmatch matching"), NULL},
 { "pcre", '\0', POPT_ARG_VAL,		&mireMode, RPMMIRE_PCRE,
	N_("use pcre matching"), NULL},

 { "invert-match", 'v', POPT_ARG_VAL|POPT_ARGFLAG_XOR,	&invert, 1,
	N_("select non-mtching lines"), NULL},

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    miRE mire = NULL;
    ARGV_t av = NULL;
    int ac = 0;
    int rc = 1;
    int xx;
    int i;

    if (__debug) {
_mire_debug = 1;
    }

    av = poptGetArgs(optCon);
    if ((ac = argvCount(av)) != 1)
	goto exit;

    mire = mireNew(mireMode, 0);
    if ((rc = mireRegcomp(mire, av[0])) != 0)
	goto exit;
    mire->notmatch ^= invert;
    
    av = NULL;
    if ((rc = argvFgets(&av, NULL)) != 0)
	goto exit;

    rc = 1;	/* assume nomatch failure. */
    ac = argvCount(av);
    if (av && *av)
    for (i = 0; i < ac; i++) {
	xx = mireRegexec(mire, av[i], 0);
	if (xx >= 0) {
	    fprintf(stdout, "%s\n", av[i]);
	    rc = 0;
	}
    }
    av = argvFree(av);

exit:
    mire = mireFree(mire);

    optCon = rpmioFini(optCon);

    return rc;
}
