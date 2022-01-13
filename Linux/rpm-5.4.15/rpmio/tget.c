#include "system.h"

#include <rpmio.h>
#include <rpmdir.h>
#include <rpmdav.h>
#include <poptIO.h>
#include <mire.h>

#include "debug.h"

typedef struct rpmtget_s * rpmtget;

struct rpmtget_s {
    const char * pattern;
    miRE mires;
    int nmires;

    rpmop sop;
    rpmop gop;

    const char * uri;
    struct stat sb;
    FD_t fd;
    char * buf;
    size_t nbuf;
    char * b;
    size_t nb;
    ARGV_t  av;
};

static char * rpmPermsString(mode_t st_mode)
{
    char *perms = xstrdup("----------");
   
    if (S_ISREG(st_mode)) 
	perms[0] = '-';
    else if (S_ISDIR(st_mode)) 
	perms[0] = 'd';
    else if (S_ISLNK(st_mode))
	perms[0] = 'l';
    else if (S_ISFIFO(st_mode)) 
	perms[0] = 'p';
    /*@-unrecog@*/
    else if (S_ISSOCK(st_mode)) 
	perms[0] = 's';
    /*@=unrecog@*/
    else if (S_ISCHR(st_mode))
	perms[0] = 'c';
    else if (S_ISBLK(st_mode))
	perms[0] = 'b';
    else
	perms[0] = '?';

    if (st_mode & S_IRUSR) perms[1] = 'r';
    if (st_mode & S_IWUSR) perms[2] = 'w';
    if (st_mode & S_IXUSR) perms[3] = 'x';
 
    if (st_mode & S_IRGRP) perms[4] = 'r';
    if (st_mode & S_IWGRP) perms[5] = 'w';
    if (st_mode & S_IXGRP) perms[6] = 'x';

    if (st_mode & S_IROTH) perms[7] = 'r';
    if (st_mode & S_IWOTH) perms[8] = 'w';
    if (st_mode & S_IXOTH) perms[9] = 'x';

    if (st_mode & S_ISUID)
	perms[3] = ((st_mode & S_IXUSR) ? 's' : 'S'); 

    if (st_mode & S_ISGID)
	perms[6] = ((st_mode & S_IXGRP) ? 's' : 'S'); 

    if (st_mode & S_ISVTX)
	perms[9] = ((st_mode & S_IXOTH) ? 't' : 'T');

    return perms;
}

static void printStat(rpmtget tget)
{
    struct stat * st = &tget->sb;
    size_t nt = 100;
    char * t = alloca(nt);
    time_t when = st->st_mtime;
    struct tm * tm = localtime(&when);
    size_t nb = strftime(t, nt, "%F %T", tm);
    const char * perms = rpmPermsString(st->st_mode);

    t[nb] = '\0';
    /* XXX Note st->st_blocks to convert to linux "ls -island" spew. */
    fprintf(stderr, "%u %u %s %u %u %u %u %s %s",
	(unsigned) st->st_ino, (unsigned)st->st_blocks/2,
	perms, (unsigned)st->st_nlink,
	(unsigned)st->st_uid, (unsigned)st->st_gid,
	(unsigned)st->st_size, t, tget->uri);
    fprintf(stderr, "\n");
    perms = _free(perms);
}

static int tgetFini(rpmtget tget)
{
    int rc = 0;

    if (tget->sop) {
	rpmswPrint("stat:", tget->sop, NULL);
	tget->sop = _free(tget->sop);
    }
    if (tget->gop) {
	rpmswPrint(" get:", tget->gop, NULL);
	tget->gop = _free(tget->gop);
    }

    tget->buf = _free(tget->buf);
    tget->nbuf = 0;
    if (tget->fd) (void) Fclose(tget->fd);
    tget->fd = NULL;

argvPrint(tget->uri, tget->av, NULL);
    tget->av = argvFree(tget->av);

    return rc;
}

static int tgetInit(rpmtget tget, size_t nbuf)
{
    int rc;
    int xx;

    if (_rpmsw_stats) {
	tget->sop = xcalloc(1, sizeof(*tget->sop));
	tget->gop = xcalloc(1, sizeof(*tget->gop));
    }

fprintf(stderr, "===== %s\n", tget->uri);
    xx = rpmswEnter(tget->sop, 0);
    rc = Stat(tget->uri, &tget->sb);
    xx = rpmswExit(tget->sop, 1);
    if (rc < 0)
	goto exit;

    printStat(tget);

    if (nbuf == 0 && tget->sb.st_size > 0)
	nbuf = tget->sb.st_size;

    tget->fd = Fopen(tget->uri, "r.ufdio");
    if (tget->fd == NULL || Ferror(tget->fd)) {
	rc = -1;
	goto exit;
    }
    tget->nbuf = nbuf;
    tget->buf = xmalloc(tget->nbuf + 2);
    tget->buf[0] = '\0';

    tget->b = NULL;
    tget->nb = 0;

    rc = 0;

exit:
    if (rc)
	(void) tgetFini(tget);
    return rc;
}

static ssize_t tgetFill(rpmtget tget)
{
    char * b = tget->buf;
    size_t nb = tget->nbuf;
    ssize_t rc;

    if (tget->b != NULL && tget->nb > 0 && tget->b > tget->buf) {
	memmove(tget->buf, tget->b, tget->nb);
	b += tget->nb;
	nb -= tget->nb;
    }

    rc = Fread(b, 1, nb, tget->fd);
    if (Ferror(tget->fd))
	rc = -1;
    else if (rc > 0) {
	tget->nb += rc;
	if (rpmIsVerbose()) {
	    int xx;
	    xx = fwrite(b, 1, rc, stderr);
	}
    }
    tget->b = tget->buf;

    return rc;
}

static const char * hrefpat = "(?i)<a(?:\\s+[a-z][a-z0-9_]*(?:=(?:\"[^\"]*\"|\\S+))?)*?\\s+href=(?:\"([^\"]*)\"|(\\S+))";

static int parseHTML(rpmtget tget)
{
    miRE mire = tget->mires;
    int noffsets = 3;
    int offsets[3];
    ssize_t nr = (tget->b != NULL ? (ssize_t)tget->nb : tgetFill(tget));
    int xx;

    xx = mireSetEOptions(mire, offsets, noffsets);

    while (tget->nb > 0) {
	char * gbn, * hbn;
	char * f, * fe;
	char * g, * ge;
	char * h, * he;
	char * t;
	mode_t mode;
	size_t nb;

	offsets[0] = offsets[1] = -1;
	xx = mireRegexec(mire, tget->b, tget->nb);
	if (xx == 0 && offsets[0] != -1 && offsets[1] != -1) {

	    /* [f:fe) contains |<a href="..."| match. */
	    f = tget->b + offsets[0];
	    fe = tget->b + offsets[1];

	    /* [h:he) contains the href basename. */
	    he = fe;
	    if (he[-1] == '"') he--;
	    if (he[-1] == '/') {
		mode = S_IFDIR | 0755;
		he--;
	    } else
		mode = S_IFREG | 0644;
	    h = he;
	    while (h > f && (h[-1] != '"' && h[-1] != '/'))
		h--;
	    nb = (size_t)(he - h);
	    hbn = t = xmalloc(nb + 1 + 1);
	    while (h < he)
		*t++ = *h++;
	    if (S_ISDIR(mode))
		*t++ = '/';
	    *t = '\0';

	    /* [g:ge) contains the URI basename. */
	    g = fe;
	    while (*g != '>')
		g++;
	    ge = ++g;
	    while (*ge != '<')
		ge++;
	    nb = (size_t)(ge - g);
	    gbn = t = xmalloc(nb + 1 + 1);
	    while (g < ge)
		*t++ = *g++;
	    if (S_ISDIR(mode))
		*t++ = '/';
	    *t = '\0';

	    /* Filter out weirdos and "." and "..". */
	    if (!strcmp(gbn, hbn) && strcmp(hbn, "./") && strcmp(hbn, "../")) {
		fprintf(stderr, "\t%s\n", gbn);
		xx = argvAdd(&tget->av, gbn);
	    }

	    gbn = _free(gbn);
	    hbn = _free(hbn);

	    offsets[1] += (ge - fe);
	    tget->b += offsets[1];
	    tget->nb -= offsets[1];
	} else {
	    size_t nb = tget->nb;
	    if (nr > 0) nb -= 1024;	/* XXX overlap a bit if filling. */
	    tget->b += nb;
	    tget->nb -= nb;
	}

	if (nr > 0)
	    nr = tgetFill(tget);
    }

    xx = mireSetEOptions(mire, NULL, 0);

    return 0;
}

static int readFile(rpmtget tget)
{
    int rc;
    int xx;

    xx = tgetInit(tget, 8 * BUFSIZ);

    xx = rpmswEnter(tget->gop, 0);

    if (S_ISDIR(tget->sb.st_mode)) {
	rc = tgetFill(tget);
    } else
    if (S_ISREG(tget->sb.st_mode)) {
	rc = tgetFill(tget);
    } else
	rc = -1;

    xx = rpmswExit(tget->gop, tget->nbuf);

    if (rc < 0)
	goto exit;

    rc = parseHTML(tget);

exit:
    xx = tgetFini(tget);
    return rc;
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
    rpmtget tget = xcalloc(1, sizeof(*tget));
    ARGV_t av = NULL;
    int ac;
    int rc = 0;
    int xx;

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

    tget->pattern = hrefpat;
    xx = mireAppend(RPMMIRE_PCRE, 0, tget->pattern, NULL, &tget->mires, &tget->nmires);

    while (rc == 0 && (tget->uri = *av++) != NULL)
	rc = readFile(tget);

    tget->mires = mireFreeAll(tget->mires, tget->nmires);
    tget->nmires = 0;

exit:

    optCon = rpmioFini(optCon);

    return rc;
}
