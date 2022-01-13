/*****************************************************************************/
/*  libCUDF - CUDF (Common Upgrade Description Format) manipulation library  */
/*  Copyright (C) 2009  Stefano Zacchiroli <zack@pps.jussieu.fr>             */
/*                                                                           */
/*  This library is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU Lesser General Public License as           */
/*  published by the Free Software Foundation, either version 3 of the       */
/*  License, or (at your option) any later version.  A special linking       */
/*  exception to the GNU Lesser General Public License applies to this       */
/*  library, see the COPYING file for more information.                      */
/*****************************************************************************/

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <argv.h>
#include <poptIO.h>

#define	_RPMCUDF_INTERNAL
#include <rpmcudf.h>

#include "debug.h"

const char *__progname;

static struct poptOption optionsTable[] = {
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        "\
Usage: cudftool CUDF_FILE [ SOLUTION_FILE ]\n\
", NULL },
  POPT_TABLEEND
};

int main(int argc, char **argv)
{
    poptContext optCon = rpmioInit(argc, argv, optionsTable);
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    rpmcudf X = NULL;
    int ec = 1;		/* assume failure */

    if (!(ac == 1 || ac == 2))
	goto exit;

    {	char * _av[] = { (char *)av[0], NULL };
	X = rpmcudfNew(_av, RPMCUDV_CUDFDOC);
    }

    rpmcudfPrintPreamble(X);
    rpmcudfPrintRequest(X);
    rpmcudfPrintUniverse(X);
fprintf(stdout, "%s", rpmiobStr(X->iob));
fflush(stdout);
    X = rpmcudfFree(X);

    {	char * _av[] = { (char *)av[0], NULL };
	X = rpmcudfNew(_av, RPMCUDV_CUDF);
    }

    {	char t [256];
	snprintf(t, sizeof(t), "Universe size: %d/%d (installed/total)",
	       rpmcudfInstalledSize(X), rpmcudfUniverseSize(X));
	rpmiobAppend(X->iob, t, 1);
	snprintf(t, sizeof(t), "Universe consistent: %s",
		(rpmcudfIsConsistent(X) ? "yes" : "no"));
	rpmiobAppend(X->iob, t, 1);
	if (ac >= 2) {
	    char * _av[] = { (char *)av[1], NULL };
	    rpmcudf Y = rpmcudfNew(_av, RPMCUDV_CUDF);
	    snprintf(t, sizeof(t), "Is solution: %s",
		rpmcudfIsSolution(X,Y) ? "yes" : "no");
	    rpmiobAppend(X->iob, t, 1);
	    Y = rpmcudfFree(Y);
	}
    }
fprintf(stdout, "%s", rpmiobStr(X->iob));
fflush(stdout);
    X = rpmcudfFree(X);
    ec = 0;

exit:
    optCon = rpmioFini(optCon);

    return ec;
}
