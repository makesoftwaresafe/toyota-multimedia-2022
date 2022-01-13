/**
 * \file rpmio/rpmgenpkglist.c
 */

#include "system.h"

#include <poptIO.h>

#include "debug.h"

/*@access FD_t @*/
/*@access miRE @*/

/*@unchecked@*/
static const char * indexfn;
/*@unchecked@*/
static const char * infofn;
/*@unchecked@*/
static const char * metasuffix;
/*@unchecked@*/
static int bloat;
/*@unchecked@*/
static int append;
/*@unchecked@*/
static int progress;
/*@unchecked@*/
static const char * cachedir;

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {

 { "index", '\0', POPT_ARG_STRING,		&indexfn, 0,
	N_("<file> to write srpm index data to"), N_("<file>") },
 { "info", '\0', POPT_ARG_STRING,		&infofn, 0,
	N_("<file> to read update info from"), N_("<file>") },
 { "meta", '\0', POPT_ARG_STRING,		&metasuffix, 0,
	N_("create package file list with given <suffix>"), N_("<suffix>") },
 { "bloat", '\0', POPT_ARG_VAL,			&bloat, 1,
	N_("do not strip the package file list"), NULL },
 { "append", '\0', POPT_ARG_VAL,		&append, 1,
	N_("append to the package file list, don't overwrite"), NULL },
 { "progress", '\0', POPT_ARG_VAL,		&progress, 1,
	N_("show a progress bar"), NULL },
 { "cachedir", '\0', POPT_ARG_STRING,		&cachedir, 1,
	N_("use a custom directory for package md5sum cache"), N_("<dir>") },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: genpkglist [<options>] <dir> <suffix>\n\
", NULL },

  POPT_TABLEEND

};

int
main(int argc, char *argv[])
	/*@globals __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    const char ** av = NULL;
    int rc = 1;		/* assume failure. */

/*@-observertrans -readonlytrans @*/
    __progname = "rpmgenpkglist";
/*@=observertrans =readonlytrans @*/

    av = poptGetArgs(optCon);
    if (av == NULL || av[0] == NULL) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    rc = 0;

exit:
    optCon = rpmioFini(optCon);

    return rc;
}
