#include "system.h"

#include <rpmio.h>
#include <rpmdir.h>
#include <rpmdav.h>
#include <poptIO.h>

#include "debug.h"

static int my_Glob_error(const char *epath, int eerrno)
{
fprintf(stderr, "*** glob_error(%p,%d) path %s\n", epath, eerrno, epath);
    return 1;
}

static int printGlob(const char * path)
{
    rpmop op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
    glob_t gl = { .gl_pathc = 0, .gl_pathv = NULL, .gl_offs = 0 };
    int rc;
    int xx;

fprintf(stderr, "===== %s\n", path);
    xx = rpmswEnter(op, 0);
    gl.gl_pathc = 0;
    gl.gl_pathv = NULL;
    gl.gl_offs = 0;
    rc = Glob(path, 0, my_Glob_error, &gl);
    if (rc != 0) {
fprintf(stderr, "*** Glob rc %d\n", rc);
    } else
    if (rpmIsVerbose()) {
	int i;
	for (i = 0; i < (int)gl.gl_pathc; i++)
	    fprintf(stderr, "%5d %s\n", i, gl.gl_pathv[i]);
    }
    Globfree(&gl);
    xx = rpmswExit(op, 0);

    if (_rpmsw_stats)
	rpmswPrint("glob:", op, NULL);
    return rc;
}

static struct poptOption optionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL,	&__debug, -1,		NULL, NULL },

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
    ARGV_t av;
    int ac;
    const char * dn;
    int rc = 0;

    if (__debug) {
_av_debug = -1;
_dav_debug = -1;
_ftp_debug = -1;
_url_debug = -1;
_rpmio_debug = -1;
    }

    av = poptGetArgs(optCon);
    ac = argvCount(av);
    if (ac < 1) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    while (rc == 0 && (dn = *av++) != NULL)
	rc = printGlob(dn);

exit:

    optCon = rpmioFini(optCon);

    return rc;
}
