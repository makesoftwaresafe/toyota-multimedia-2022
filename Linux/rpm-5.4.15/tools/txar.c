
#define	RPM2XAR
#include "system.h"
#include "xar.h"
#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmtag.h>
#include <rpmcli.h>

#define	_RPMWF_INTERNAL
#include <rpmwf.h>

#include "debug.h"

static struct poptOption optionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmwf_debug, 1,
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
    const char ** args;
    const char * sfn;
    int ec = 0;

    if (optCon == NULL)
        exit(EXIT_FAILURE);

    if ((args = poptGetArgs(optCon)) != NULL)
    while ((sfn = *args++) != NULL) {
	rpmwf wf;
	char * tfn;
	size_t nb = strlen(sfn);
	int x = nb - (sizeof(".rpm") - 1);
	rpmRC rc;

	if (x <= 0)
	    rc = RPMRC_FAIL;
	else
	if (!strcmp(&sfn[x], ".rpm")) {
	    tfn = xstrdup(sfn);
	    strcpy(&tfn[x], ".xar");
	    if ((wf = rdRPM(sfn)) != NULL) {
		rc = wrXAR(tfn, wf);
		wf = rpmwfFree(wf);
	    } else
		rc = RPMRC_FAIL;
	    tfn = _free(tfn);
	} else
	if (!strcmp(&sfn[x], ".xar")) {
	    tfn = xstrdup(sfn);
	    strcpy(&tfn[x], ".rpm");
	    if ((wf = rdXAR(sfn)) != NULL) {
		rc = wrRPM(tfn, wf);
		wf = rpmwfFree(wf);
	    } else
		rc = RPMRC_FAIL;
	    tfn = _free(tfn);
	} else
	    rc = RPMRC_FAIL;
	if (rc != RPMRC_OK)
	    ec++;
    }

    optCon = rpmcliFini(optCon);

    return ec;
}
