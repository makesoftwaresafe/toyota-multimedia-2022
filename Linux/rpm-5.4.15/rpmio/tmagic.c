#include "system.h"

#include "magic.h"

#include <rpmio.h>
#include <rpmmacro.h>
#include <rpmcb.h>
#include <poptIO.h>

#define	_RPMMG_INTERNAL
#include <rpmmg.h>

#include "debug.h"

#define        FNPATH          "tmagic"
static char * fnpath = FNPATH;

static
void readFile(rpmmg mg, const char * path)
{
    FD_t fd;

fprintf(stderr, "===== readFile(%p, %s)\n", mg, path);
    fd = Fopen(path, "r");
    if (fd != NULL) {
	char buf[BUFSIZ];
	size_t len = Fread(buf, 1, sizeof(buf), fd);
	(void) Fclose(fd);

	if (len > 0) {
	    const char * magic = rpmmgBuffer(mg, buf, len);
	    fprintf(stderr, "==> magic \"%s\"\n", magic);
	    magic = _free(magic);
	}
    }
}

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
    rpmmg mg;
    int rc = 0;

_rpmmg_debug = -1;
    if (_rpmmg_debug) {
	rpmIncreaseVerbosity();
	rpmIncreaseVerbosity();
    }

    mg = rpmmgNew(NULL, 0);
    readFile(mg, fnpath);
    mg = rpmmgFree(mg);

    optCon = rpmioFini(optCon);

    return rc;
}
