/**
 * \file rpmio/rpmkey.c
 */

#include "system.h"
#include <rpmio.h>
#include <rpmcli.h>
#include "debug.h"

static int _debug = 0;

static struct poptOption optionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,		&_debug, 1,
	NULL, NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options:"),
	NULL },

    POPT_AUTOALIAS
    POPT_AUTOHELP
    POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    const char * prompt = "Enter pass phrase: ";
    int ret = 0;

    if (optCon == NULL)
        exit(EXIT_FAILURE);

    fprintf(stderr, "password %s\n", _RequestPass(prompt));

    optCon = rpmcliFini(optCon);
    return ret;
}
