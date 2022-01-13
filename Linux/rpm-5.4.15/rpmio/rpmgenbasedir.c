/**
 * \file rpmio/rpmgenbasedir.c
 */

#include "system.h"

#include <poptIO.h>

#include "debug.h"

/*@access FD_t @*/
/*@access miRE @*/

/*@unchecked@*/
static int signit;
/*@unchecked@*/
static const char * defaultkey;
/*@unchecked@*/
static int hashonly;
/*@unchecked@*/
static int listonly;
/*@unchecked@*/
static int partial;
/*@unchecked@*/
static int oldhashfile;
/*@unchecked@*/
static int bz2only;
/*@unchecked@*/
static int progress;
/*@unchecked@*/
static const char * infofn;
/*@unchecked@*/
static int flat;
/*@unchecked@*/
static int bloat;
/*@unchecked@*/
static const char * metasuffix;
/*@unchecked@*/
static int compresslevel;
/*@unchecked@*/
static const char * cachedir;

/*@unchecked@*/
static const char * indexfn;
/*@unchecked@*/
static int append;

/*==============================================================*/

/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {

 { "sign", 's', POPT_ARG_VAL,			&signit, 1,
	N_("Generate and sign hashfile"), NULL },
 { "default-key", 's', POPT_ARG_VAL,		&defaultkey, 1,
	N_("Use <ID> as gnupg secret key"), N_("<ID>") },
 { "hashonly", '\0', POPT_ARG_VAL,		&hashonly, 1,
	N_("Do hash stuff only"), NULL },
 { "listonly", '\0', POPT_ARG_VAL,		&listonly, 1,
	N_("Generate pkglists/srclists and quit"), NULL },
 { "partial", '\0', POPT_ARG_VAL,		&partial, 1,
	N_("Update just some of the already existent components"), NULL },
 { "oldhashfile", '\0', POPT_ARG_VAL,		&oldhashfile, 1,
	N_("Enable generation of old hashfile"), NULL },
 { "bz2only", '\0', POPT_ARG_VAL,		&bz2only, 1,
	N_("Generate only compressed lists"), NULL },
 { "progress", '\0', POPT_ARG_VAL,		&progress, 1,
	N_("Show progress bars for genpkglist/gensrclist"), NULL },
 { "updateinfo", '\0', POPT_ARG_STRING,		&infofn, 0,
	N_("Update information file"), N_("<file>") },
 { "flat", '\0', POPT_ARG_VAL,			&flat, 1,
	N_("use a flat directory structure with RPMS/SRPMS in the same directory"), NULL },
 { "bloat", '\0', POPT_ARG_VAL,			&bloat, 1,
	N_("do not strip the package file list"), NULL },
 { "meta", '\0', POPT_ARG_STRING,		&metasuffix, 0,
	N_("create package file list with given <suffix>"), N_("<suffix>") },
 { "compresslevel", '\0', POPT_ARG_INT,		&compresslevel, 0,
	N_("Set bzip2 compress level (1-9)"), NULL },
 { "cachedir", '\0', POPT_ARG_STRING,		&cachedir, 1,
	N_("use a custom directory for package md5sum cache"), N_("<dir>") },

 { "index", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN,		&indexfn, 0,
	N_("<file> to write srpm index data to"), N_("<file>") },
 { "info", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN,		&infofn, 0,
	N_("<file> to read update info from"), N_("<file>") },
 { "append", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,		&append, 1,
	N_("append to the package file list, don't overwrite"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: genbasedir [<options>] <topdir> [<comp1> [<comp2> ...]]\n\
\n\
Examples:\n\
\n\
   genbasedir /home/ftp/pub/conectiva\n\
   genbasedir /home/ftp/pub/conectiva main extra devel\n\
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
