#include "system.h"

#undef  _	/* XXX everyone gotta be different */

#include <rpmio_internal.h>
#include <poptIO.h>

#define	_RPMPERL_INTERNAL
#include <rpmperl.h>

#include "debug.h"

static struct poptOption optionsTable[] = {

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
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    int perlFlags = 0;
    rpmperl perl = rpmperlNew((char **)av, perlFlags);
#ifdef	NOTYET
    const char * fn;
#endif
    const char * result;
    int rc = 1;		/* assume failure */

#ifdef	NOTYET
    if (ac < 1) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    while ((fn = *av++) != NULL) {
	rpmRC ret;
	result = NULL;
	if ((ret = rpmperlRunFile(perl, fn, &result)) != RPMRC_OK)
	    goto exit;
	if (result != NULL && *result != '\0')
	    fprintf(stdout, "%s\n", result);
    }
    rc = 0;
#else
    result = NULL;
    if (rpmperlRun(perl, "print \"Hello, world!\n\";", &result) != RPMRC_OK)
	goto exit;
    if (result && *result)
	fprintf(stdout, "%s\n", result);
    rc = 0;
#endif

exit:
    perl = rpmperlFree(perl);
    optCon = rpmioFini(optCon);

    return rc;
}
