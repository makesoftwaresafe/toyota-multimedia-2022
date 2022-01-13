#include "system.h"

#include <rpmio.h>
#include <rpmdir.h>
#include <rpmdav.h>
#include <rpmmacro.h>
#include <rpmcb.h>
#include <poptIO.h>
#include <argv.h>

#include "debug.h"

static int _debug = 0;

int noNeon;

#define	HTTPSPATH	"https://localhost/rawhide/toad/tput.txt"
#define	HTTPPATH	"http://localhost/rawhide/toad/tput.txt"
#define	FTPPATH		"ftp://localhost/home/test/tput.txt"
#define	DIRPATH		"file://localhost/var/ftp/tput.txt"
#if 0
static char * httpspath = HTTPSPATH;
#endif
static char * httppath = HTTPPATH;
#if 0
static char * ftppath = FTPPATH;
static char * dirpath = DIRPATH;
#endif

#if 0
static size_t readFile(const char * path)
{
    char buf[BUFSIZ];
    size_t len = 0;
    FD_t fd;
    int xx;

    buf[0] = '\0';
fprintf(stderr, "===== Fread %s\n", path);
    fd = Fopen(path, "r");
    if (fd != NULL) {

	len = Fread(buf, 1, sizeof(buf), fd);
	xx = Fclose(fd);
    }

    if (len > 0)
	fwrite(buf, 1, strlen(buf), stderr);

    return len;
}
#endif

static size_t writeFile(const char * path)
{
    char buf[BUFSIZ];
    size_t len = 0;
    FD_t fd;
    int xx;

    strcpy(buf, "Hello World!\n");
fprintf(stderr, "===== Fwrite %s\n", path);
    fd = Fopen(path, "w");
    if (fd != NULL) {
	len = Fwrite(buf, 1, strlen(buf), fd);
	xx = Fclose(fd);
if (xx)
fprintf(stderr, "===> Fclose rc %d\n", xx);
    }

    if (len > 0)
	fwrite(buf, 1, strlen(buf), stderr);

    return len;
}

#if 0
static int unlinkFile(const char * path)
{
fprintf(stderr, "===== Unlink %s\n", path);
    return Unlink(path);
}
#endif

static void doFile(const char * path)
{
    int xx;

fprintf(stderr, "===== %s\n", path);
#if 0
    xx = unlink("/home/toad/tput.txt");
    xx = unlink("/var/ftp/tput.txt");
    xx = unlink("/var/www/html/tput.txt");
#endif

#if 0
    xx = unlinkFile(path);
#endif
    xx = writeFile(path);
#if 0
    xx = readFile(path);
    xx = Unlink(path);

    xx = unlink("/home/toad/tput.txt");
    xx = unlink("/var/ftp/tput.txt");
    xx = unlink("/var/www/html/tput.txt");
#endif
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
    int rc = 0;

_av_debug = -1;
_ftp_debug = -1;
_dav_debug = -1;
#if 0
    doFile(dirpath);
    doFile(ftppath);
#endif
    doFile(httppath);
#if 0
    doFile(httpspath);
#endif

    optCon = rpmioFini(optCon);

    return 0;
}
