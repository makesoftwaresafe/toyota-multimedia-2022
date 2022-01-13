/**
 * \file rpmio/rpmgensrclist.c
 */

#include "system.h"

#include <poptIO.h>

#include "debug.h"

/*@access FD_t @*/
/*@access miRE @*/

/*@unchecked@*/
static int flat;
/*@unchecked@*/
static const char * metasuffix;
/*@unchecked@*/
static int append;
/*@unchecked@*/
static int progress;
/*@unchecked@*/
static const char * cachedir;

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {

 { "flat", '\0', POPT_ARG_VAL,			&flat, 1,
	N_("use a flat directory structure with RPMS/SRPMS in the same directory"), NULL },
 { "meta", '\0', POPT_ARG_STRING,		&metasuffix, 0,
	N_("create source package file list with given <suffix>"), N_("<suffix>") },
 { "append", '\0', POPT_ARG_VAL,		&append, 1,
	N_("append to the source package file list, don't overwrite"), NULL },
 { "progress", '\0', POPT_ARG_VAL,		&progress, 1,
	N_("show a progress bar"), NULL },
 { "cachedir", '\0', POPT_ARG_STRING,		&cachedir, 1,
	N_("use a custom directory for package md5sum cache"), N_("<dir>") },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: gensrclist [<options>] <dir> <suffix> <srpm index>\n\
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
