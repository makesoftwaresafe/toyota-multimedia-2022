/**
 * \file lib/tpgp.c
 * Test RFC-4880 clearsigned and detached signatures.
 */

extern int _pgp_debug;
extern int _pgp_print;

#include "system.h"

#include <rpmio.h>
#include <rpmns.h>
#include <rpmcli.h>

#include "genssl.h"

#include "debug.h"

static
rpmRC doit(rpmts ts, const char * sigtype)
{
    rpmRC rc = RPMRC_FAIL;

#ifdef	NOTYET
    if (!strcmp("DSA", sigtype)) {
	rc = rpmnsProbeSignature(ts, DSApem, NULL, DSApub, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, DSApub, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, DSApubpem, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, DSApub, DSApubid, 0);

	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, DSApubpem, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsig, NULL, DSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, DSAsigpem, NULL, DSApubid, 0);
    }

    if (!strcmp("RSA", sigtype)) {
	rc = rpmnsProbeSignature(ts, RSApem, NULL, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsig, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApubpem, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApub, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, RSApubpem, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsig, NULL, RSApubid, 0);
	rc = rpmnsProbeSignature(ts, plaintextfn, RSAsigpem, NULL, RSApubid, 0);
    }
#endif
    
    return rc;
}

static struct poptOption optionsTable[] = {
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
    rpmts ts = rpmtsCreate();
    int rc = rpmtsOpenDB(ts, O_RDONLY);

fprintf(stderr, " DSA");
    rc = doit(ts, "DSA");
fprintf(stderr, " RSA");
    rc = doit(ts, "RSA");
fprintf(stderr, "\n");

    (void) rpmtsFree(ts); 
    ts = NULL;

    optCon = rpmcliFini(optCon);

    return rc;
}
