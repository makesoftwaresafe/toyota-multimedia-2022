#include "system.h"

#include <poptIO.h>

#include <rpmsvn.h>

#include "debug.h"

static struct poptOption rpmsvnOptionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL,	&_rpmsvn_debug, -1,		NULL, NULL },
  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, rpmsvnOptionsTable);
    rpmsvn svn;
    int rc = 0;

_rpmsvn_debug = -1;

    svn = rpmsvnNew(NULL, 0);

    svn = rpmsvnFree(svn);

/*@i@*/ urlFreeCache();

    return rc;
}
