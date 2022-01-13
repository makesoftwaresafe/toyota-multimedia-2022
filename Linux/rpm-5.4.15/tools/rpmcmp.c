#include "system.h"

#include <popt.h>
#include <rpmio_internal.h>
#include <rpmcb.h>		/* XXX fnpyKey */
#include <argv.h>
#include <rpmtypes.h>
#include <rpmtag.h>

#define	_RPMDS_INTERNAL
#define	_RPMEVR_INTERNAL
#include <rpmds.h>

#include "debug.h"

const char *__progname;
#define	progname	__progname

static int pointRpmEVR(ARGV_t av)
{
    EVR_t a = memset(alloca(sizeof(*a)), 0, sizeof(*a));
    EVR_t b = memset(alloca(sizeof(*b)), 0, sizeof(*b));
    int rc;

    (void) rpmEVRparse(av[0], a);
    (void) rpmEVRparse(av[2], b);

    rc = rpmEVRcompare(a, b);
    if (rc < 0)
	rc = !(av[1][0] == 'l');
    else if (rc > 0)
	rc = !(av[1][0] == 'g');
    else
	rc = !(av[1][0] == 'e' || av[1][1] == 'e');

    a->str = _free(a->str);
    b->str = _free(b->str);
    return rc;
}

static struct poptOption optionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL,	&_rpmevr_debug, -1,		NULL, NULL },
  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon;
    ARGV_t av;
    int ac;
    const char * arg;
    int ec = 0;
    int rc;
    int xx;

    if ((progname = strrchr(argv[0], '/')) != NULL)
	progname++;
    else
	progname = argv[0];

    optCon = poptGetContext(argv[0], argc, (const char **)argv, optionsTable, 0);
    while ((rc = poptGetNextOpt(optCon)) > 0)
	;

    av = poptGetArgs(optCon);
    ac = argvCount(av);

    if (ac == 0 || !strcmp(*av, "-")) {
	av = NULL;
	xx = argvFgets(&av, NULL);
	ac = argvCount(av);
    }
    
    if (av != NULL)
    while ((arg = *av++) != NULL) {
	ARGV_t rav = NULL;
	int rac = 0;
	if (poptParseArgvString(arg, &rac, &rav) || rac != 3) {
	    fprintf(stderr, _("skipping malformed comparison: \"%s\"\n"), arg);
	    continue;
	}
	rc = pointRpmEVR(rav);
	free(rav);
	rav = NULL;
	rac = 0;
    }

    optCon = poptFreeContext(optCon);

    return ec;
}
