#include "system.h"

#if defined(RPM_VENDOR_MANDRIVA)
#include <rpmio_internal.h>	/* XXX for fdGetFILE() */
#include <argv.h>
#include <rpmlog.h>
#define	_MIRE_INTERNAL
#include <mire.h>
#include <rpmmacro.h>	/* XXX for rpmExpand */
#include <rpmtag.h>
#include <rpmtypes.h>
#include <rpmfi.h>

#include "filetriggers.h" /* mayAddToFilesAwaitingFiletriggers rpmRunFileTriggers */

#include "debug.h"

#define	_FILES_AWAITING_FILETRIGGERS "/var/lib/rpm/files-awaiting-filetriggers"

/*@unchecked@*/ /*@observer@*/
static const char * files_awaiting_filetriggers = _FILES_AWAITING_FILETRIGGERS;

#define FILTER_EXTENSION ".filter"

/*@unchecked@*/ /*@shared@*/ /*@null@*/
static const char * _filetriggers_dir;

/*@null@*/ /*@observer@*/
static const char * filetriggers_dir(void)
	/*@globals _filetriggers_dir, rpmGlobalMacroContext, h_errno, internalState  @*/
	/*@modifies _filetriggers_dir, rpmGlobalMacroContext, internalState @*/
{
    if (_filetriggers_dir == NULL)
	_filetriggers_dir = rpmExpand("%{?_filetriggers_dir}", NULL);
    return (_filetriggers_dir != NULL && _filetriggers_dir[0] != '\0')
	? _filetriggers_dir : NULL;
}

/*@only@*/ /*@null@*/
static char * get_filter_name(/*@returned@*/ const char * fn)
	/*@*/
{
    char * p = strrchr(fn, '/');
    if (p == NULL)
	return NULL;
#if defined(HAVE_STRNDUP) && !defined(__LCLINT__)
    p = strndup(p+1, strlen(p+1) - strlen(FILTER_EXTENSION));
#else
    {	char * p_src = p;
	size_t p_len = strlen(p_src+1) - strlen(FILTER_EXTENSION);
	p = (char *) xmalloc(p_len+1);
	strncpy(p, p_src+1, p_len);
    }
#endif
    return p;
}

int mayAddToFilesAwaitingFiletriggers(const char * rootDir, rpmfi fi,
		int install_or_erase)
{
    const char * fn;
    FILE * fp;
    int rc = RPMRC_FAIL;
    int xx;

    if (filetriggers_dir() == NULL)
	return RPMRC_OK;

    fn = rpmGetPath(rootDir ? rootDir : "/", files_awaiting_filetriggers, NULL);

    fp = fopen(fn, "a");
    if (fp == NULL) {
	rpmlog(RPMLOG_ERR, _("%s: open failed: %s\n"), fn, strerror(errno));
	goto exit;
    }

    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0) {
	xx = fputc(install_or_erase ? '+' : '-', fp);
	xx = fputs(rpmfiFN(fi), fp);
	xx = fputc('\n', fp);
    }
    xx = fclose(fp);
    rc = RPMRC_OK;

exit:
    fn = _free(fn);
    return rc;
}

struct filetrigger_raw {
    char * regexp;
/*@relnull@*/
    char * name;
};

struct filetrigger {
    miRE mire;
    char * name;
    int command_pipe;
    pid_t command_pid;
};

static int getFiletriggers_raw(const char * rootDir, int * nftp,
		struct filetrigger_raw ** list_raw)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies *nftp, *list_raw, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * dn = filetriggers_dir();
    const char * globstr = NULL;
    ARGV_t av = NULL;
    int ac = 0;
    int xx;
    int i;

    if (dn == NULL)
	goto exit;

    globstr = rpmGetPath(rootDir ? rootDir : "/", dn, "/*" FILTER_EXTENSION, NULL);
    xx = rpmGlob(globstr, &ac, &av);
    if (ac == 0)
	goto exit;

    *list_raw = (struct filetrigger_raw *) xcalloc(ac, sizeof(**list_raw));

    for (i = 0; i < ac; i++) {
	const char * fn = av[i];
	int fdno = open(fn, O_RDONLY);
	struct stat sb;

	if (fdno == -1) {
	    rpmlog(RPMLOG_ERR, "opening %s failed: %s\n", fn, strerror(errno));
	    continue;
	}

	if (fstat(fdno, &sb) == 0 && sb.st_size > 0) {
	    size_t bn = sb.st_size;
	    char * b = (char *) xmalloc(bn + 1);

	    if (read(fdno, b, bn) != (ssize_t)bn) {
		rpmlog(RPMLOG_ERR, "reading %s failed: %s\n", fn,
			strerror(errno));
		b = _free(b);
	    } else {
		char * be = b + bn;
		*be = '\0';
		if ((be = strchr(b, '\n')) != NULL) {
		    *be = '\0';
		    while (be > b && xisspace((int)be[-1]))
			*(--be) = '\0';
		}
		(*list_raw)[i].regexp = b;
		(*list_raw)[i].name = get_filter_name(fn);
	    }
	}
	xx = close(fdno);
    }

exit:
    if (nftp != NULL)
	*nftp = ac;
    av = argvFree(av);
    globstr = _free(globstr);
    return RPMRC_OK;
}

static char * computeMatchesAnyFilter(size_t nb,
		struct filetrigger_raw * list_raw)
	/*@*/
{
    char * matches_any;
    char * p;
    size_t regexp_str_size = 0;
    int i;

    for (i = 0; i < (int)nb; i++)
	regexp_str_size += strlen(list_raw[i].regexp) + 1;

    matches_any = (char *) xmalloc(regexp_str_size);
    p = stpcpy(matches_any, list_raw[0].regexp);

    for (i = 1; i < (int)nb; i++) {
	*p++ = '|';
	p = stpcpy(p, list_raw[i].regexp);
    }
    rpmlog(RPMLOG_DEBUG, D_("[filetriggers] matches-any regexp is %s\n"),
		matches_any);
    return matches_any;
}

static void compileFiletriggersRegexp(/*@only@*/ char * raw, miRE mire)
	/*@modifies raw, mire @*/
{
    static int options = REG_NOSUB | REG_EXTENDED | REG_NEWLINE;
    int xx;

    xx = mireSetCOptions(mire, RPMMIRE_REGEX, 0, options, NULL);

    if (mireRegcomp(mire, raw) != 0) {
	rpmlog(RPMLOG_ERR, "failed to compile filetrigger filter: %s\n", raw);
	mire = mireFree(mire);
    }
    raw = _free(raw);
}

static void getFiletriggers(const char * rootDir, miRE matches_any,
		int * nftp, struct filetrigger ** list)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies matches_any, *nftp, *list, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    struct filetrigger_raw * list_raw = NULL;
    int xx;
    int i;

    xx = getFiletriggers_raw(rootDir, nftp, &list_raw);
    if (*nftp == 0) return;

    compileFiletriggersRegexp(computeMatchesAnyFilter(*nftp, list_raw), matches_any);

    *list = (struct filetrigger *) xcalloc(*nftp, sizeof(**list));
    for (i = 0; i < *nftp; i++) {
	(*list)[i].name = list_raw[i].name;
	(*list)[i].mire = mireNew(0, 0);
	compileFiletriggersRegexp(list_raw[i].regexp, (*list)[i].mire);
    }
    list_raw = _free(list_raw);
}

static void freeFiletriggers(/*@only@*/ miRE matches_any,
		int nft, /*@only@*/ struct filetrigger * list)
	/*@modifies matches_any, list @*/
{
    int i;

    matches_any = mireFree(matches_any);
    for (i = 0; i < nft; i++) {
	list[i].mire = mireFree(list[i].mire);
	list[i].name = _free(list[i].name);
    }
    list = _free(list);
}

static int is_regexp_matching(miRE mire, const char * s)
	/*@modifies mire @*/
{
    return mireRegexec(mire, s, (size_t) 0) == 0;
}

static int popen_with_root(const char * rootDir, const char * cmd,
		pid_t * pidp)
	/*@globals fileSystem, internalState @*/
	/*@modifies *pidp, fileSystem, internalState @*/
{
    int pipes[2];
    int xx;

    if (pipe(pipes) != 0)
	return 0;

    *pidp = fork();
    if (*pidp == 0) {
	const char * argv[2];

	xx = close(pipes[1]);
	xx = dup2(pipes[0], STDIN_FILENO);
	xx = close(pipes[0]);

	if (rootDir != NULL && strcmp(rootDir, "/") && *rootDir == '/') {
/*@-superuser@*/
	    if (Chroot(rootDir) != 0) {
		rpmlog(RPMLOG_ERR, "chroot to %s failed\n", rootDir);
		_exit(-1);
	    }
/*@=superuser@*/
	    xx = chdir("/");
	}
	argv[0] = cmd;
	argv[1] = NULL;
	xx = execv(argv[0], (char *const *) argv);
	_exit(-1);
    }
    
    xx = close(pipes[0]);

    return pipes[1];   
}

static void mayStartFiletrigger(const char * rootDir,
		struct filetrigger * trigger)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies trigger, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    if (!trigger->command_pipe) {
	const char * dn = filetriggers_dir();
	const char * cmd;

	if (dn == NULL)
	    return;

	cmd = rpmGetPath(dn, "/", trigger->name, ".script", NULL);
	rpmlog(RPMLOG_DEBUG, D_("[filetriggers] spawning %s\n"),
			cmd);
	trigger->command_pipe = popen_with_root(rootDir, cmd,
				    &trigger->command_pid);
	cmd = _free(cmd);
    }
}

void rpmRunFileTriggers(const char * rootDir)
{
    miRE matches_any = mireNew(RPMMIRE_DEFAULT, 0);
    int nft = 0;
    struct filetrigger *list = NULL;
    const char * fn = NULL;
    FD_t fd = NULL;
    FILE * fp = NULL;
    int xx;

    rpmlog(RPMLOG_DEBUG, D_("[filetriggers] starting\n"));

    fn = rpmGenPath(rootDir, files_awaiting_filetriggers, NULL);

    if (!filetriggers_dir())
	goto exit;

    getFiletriggers(rootDir, matches_any, &nft, &list);
    if (nft <= 0)
	goto exit;

    fd = Fopen(fn, "r.fpio");
    if (fd == NULL)
	goto exit;

    fp = fdGetFILE(fd);

    if (fp != NULL) {
	void (*oldhandler)(int) = signal(SIGPIPE, SIG_IGN);
	char tmp[BUFSIZ];
	int i;

	rpmlog(RPMLOG_DEBUG,
		D_("[filetriggers] testing files from list: %s\n"), fn);

	while (fgets(tmp, (int)sizeof(tmp), fp)) {
	    size_t tmplen = strlen(tmp);

	    if (tmplen > 0 && tmp[tmplen-1] == '\n')
		tmp[--tmplen] = '\0';

	    if (!is_regexp_matching(matches_any, tmp))
		continue;

	    rpmlog(RPMLOG_DEBUG,
			D_("[filetriggers] matches-any regexp found %s\n"),
			tmp);

	    for (i = 0; i < nft; i++) {
		ssize_t nw;
		if (!is_regexp_matching(list[i].mire, tmp))
		    /*@innercontinue@*/ continue;

		rpmlog(RPMLOG_DEBUG,
			D_("[filetriggers] file name '%s' matches pattern '%s'\n"),
			tmp, list[i].mire->pattern);

		mayStartFiletrigger(rootDir, &list[i]);
		nw = write(list[i].command_pipe, tmp, tmplen);
		nw = write(list[i].command_pipe, "\n", 1);
	    }
	}

	xx = Fclose(fd);
	fd = NULL;
	fp = NULL;

	for (i = 0; i < nft; i++) {
	    int status;
	    if (list[i].command_pipe) {
		pid_t pid;
		xx = close(list[i].command_pipe);
		rpmlog(RPMLOG_DEBUG,
			D_("[filetriggers] waiting for %s to end\n"),
			list[i].name);
		pid = waitpid(list[i].command_pid, &status, 0);
		list[i].command_pipe = 0;
	    }
	}

	oldhandler = signal(SIGPIPE, oldhandler);
    }

exit:
    freeFiletriggers(matches_any, nft, list);

    if (fn != NULL)
	xx = unlink(fn);
    fn = _free(fn);
}
#endif	/* defined(RPM_VENDOR_MANDRIVA) */
