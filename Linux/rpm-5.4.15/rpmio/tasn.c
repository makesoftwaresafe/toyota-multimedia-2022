
/** \ingroup rpmio signature
 * \file rpmio/tkey.c
 * Routines to handle RFC-2440 detached signatures.
 */

#include "system.h"

#include <rpmio.h>
#define	_RPMASN_INTERNAL
#include <rpmasn.h>
#include <poptIO.h>

#include "debug.h"

static struct poptOption rpmasnOptionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_(" Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
"), NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, rpmasnOptionsTable);
    static const char * _fn = "rpm.asn";
    rpmasn asn = rpmasnNew(_fn, 0);
    int ec = 0;

    asn1_print_structure(stdout, asn->tree, "RPM.DSASignature", ASN1_PRINT_ALL);

    asn = rpmasnFree(asn);

    con = rpmioFini(con);

    return ec;
}
