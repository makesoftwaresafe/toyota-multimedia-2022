#include "system.h"

#include <signal.h>	/* getOutputFrom() */

#include <rpmio.h>
#include <rpmiotypes.h>		/* XXX fnpyKey */
#include <rpmlog.h>
#include <rpmurl.h>
#include <rpmmg.h>
#include <argv.h>
#define	_MIRE_INTERNAL
#include <mire.h>

#include <rpmtag.h>
#define	_RPMEVR_INTERNAL
#include <rpmbuild.h>

#define	_RPMNS_INTERNAL
#include <rpmns.h>

#define	_RPMFC_INTERNAL
#include <rpmfc.h>

#define	_RPMDS_INTERNAL
#include <rpmds.h>
#include <rpmfi.h>

#include "debug.h"

/*@access rpmds @*/
/*@access miRE @*/

#ifdef __cplusplus
GENfree(rpmuint16_t *)
GENfree(rpmuint32_t *)
#endif	/* __cplusplus */

/*@unchecked@*/
static int _filter_values = 1;
/*@unchecked@*/
static int _filter_execs = 1;

/**
 */
static int rpmfcExpandAppend(/*@out@*/ ARGV_t * argvp, const ARGV_t av)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies *argvp, rpmGlobalMacroContext, internalState @*/
	/*@requires maxRead(argvp) >= 0 @*/
{
    ARGV_t argv = *argvp;
    int argc = argvCount(argv);
    int ac = argvCount(av);
    int i;

    argv = (ARGV_t) xrealloc(argv, (argc + ac + 1) * sizeof(*argv));
    for (i = 0; i < ac; i++)
	argv[argc + i] = rpmExpand(av[i], NULL);
    argv[argc + ac] = NULL;
    *argvp = argv;
    return 0;
}

/* XXX FIXME: more AutoFu testing needed? */
#if defined(HAVE_SIG_T) && !defined(SIGHANDLER_T)
typedef sig_t sighandler_t;
#endif

/**
 * Return output from helper script.
 * @todo Use poll(2) rather than select(2), if available.
 * @param dir		directory to run in (or NULL)
 * @param argv		program and arguments to run
 * @param writePtr	bytes to feed to script on stdin (or NULL)
 * @param writeBytesLeft no. of bytes to feed to script on stdin
 * @param failNonZero	is script failure an error?
 * @return		buffered stdout from script, NULL on error
 */     
/*@null@*/
static rpmiob getOutputFrom(/*@null@*/ const char * dir, ARGV_t argv,
			const char * writePtr, size_t writeBytesLeft,
			int failNonZero)
	/*@globals h_errno, fileSystem, internalState@*/
	/*@modifies fileSystem, internalState@*/
{
    pid_t child, reaped;
    int toProg[2];
    int fromProg[2];
    int status;
    sighandler_t oldhandler = signal(SIGPIPE, SIG_IGN);
    rpmiob iob = NULL;
    int done;

    toProg[0] = toProg[1] = 0;
    fromProg[0] = fromProg[1] = 0;
    if (pipe(toProg) < 0 || pipe(fromProg) < 0) {
	rpmlog(RPMLOG_ERR, _("Couldn't create pipe for %s: %m\n"), argv[0]);
	return NULL;
    }
    
    if (!(child = fork())) {
	(void) close(toProg[1]);
	(void) close(fromProg[0]);
	
	(void) dup2(toProg[0], STDIN_FILENO);   /* Make stdin the in pipe */
	(void) dup2(fromProg[1], STDOUT_FILENO); /* Make stdout the out pipe */

	(void) close(toProg[0]);
	(void) close(fromProg[1]);

	if (dir) {
	    (void) Chdir(dir);
	}
	
	rpmlog(RPMLOG_DEBUG, D_("\texecv(%s) pid %d\n"),
			argv[0], (unsigned)getpid());

	unsetenv("MALLOC_CHECK_");
	(void) execvp(argv[0], (char *const *)argv);
	/* XXX this error message is probably not seen. */
	rpmlog(RPMLOG_ERR, _("Couldn't exec %s: %s\n"),
		argv[0], strerror(errno));
	_exit(EXIT_FAILURE);
    }
    if (child < 0) {
	rpmlog(RPMLOG_ERR, _("Couldn't fork %s: %s\n"),
		argv[0], strerror(errno));
	return NULL;
    }

    (void) close(toProg[0]);
    (void) close(fromProg[1]);

    /* Do not block reading or writing from/to prog. */
    (void) fcntl(fromProg[0], F_SETFL, O_NONBLOCK);
    (void) fcntl(toProg[1], F_SETFL, O_NONBLOCK);
    
    iob = rpmiobNew(0);

    do {
	fd_set ibits, obits;
	struct timeval tv;
	int nfd;
	ssize_t nbr;
	ssize_t nbw;
	int rc;

	done = 0;
top:
	FD_ZERO(&ibits);
	FD_ZERO(&obits);
	if (fromProg[0] >= 0) {
	    FD_SET(fromProg[0], &ibits);
	}
	if (toProg[1] >= 0) {
	    FD_SET(toProg[1], &obits);
	}
	/* XXX values set to limit spinning with perl doing ~100 forks/sec. */
	tv.tv_sec = 0;
	tv.tv_usec = 10000;
	nfd = ((fromProg[0] > toProg[1]) ? fromProg[0] : toProg[1]);
	if ((rc = select(nfd, &ibits, &obits, NULL, &tv)) < 0) {
	    if (errno == EINTR)
		goto top;
	    break;
	}

	/* Write any data to program */
	if (toProg[1] >= 0 && FD_ISSET(toProg[1], &obits)) {
	  if (writePtr && writeBytesLeft > 0) {
	    if ((nbw = write(toProg[1], writePtr,
		    ((size_t)1024<writeBytesLeft) ? (size_t)1024 : writeBytesLeft)) < 0)
	    {
	        if (errno != EAGAIN) {
		    perror("getOutputFrom()");
	            exit(EXIT_FAILURE);
		}
	        nbw = 0;
	    }
	    writeBytesLeft -= nbw;
	    writePtr += nbw;
	  } else if (toProg[1] >= 0) {	/* close write fd */
	    (void) close(toProg[1]);
	    toProg[1] = -1;
	  }
	}
	
	/* Read any data from prog */
	{   char buf[BUFSIZ+1];
	    while ((nbr = read(fromProg[0], buf, sizeof(buf)-1)) > 0) {
		buf[nbr] = '\0';
		iob = rpmiobAppend(iob, buf, 0);
	    }
	}

	/* terminate on (non-blocking) EOF or error */
	done = (nbr == 0 || (nbr < 0 && errno != EAGAIN));

    } while (!done);

    /* Clean up */
    if (toProg[1] >= 0)
    	(void) close(toProg[1]);
    if (fromProg[0] >= 0)
	(void) close(fromProg[0]);
    (void) signal(SIGPIPE, oldhandler);

    /* Collect status from prog */
    reaped = waitpid(child, &status, 0);
    rpmlog(RPMLOG_DEBUG, D_("\twaitpid(%d) rc %d status %x\n"),
	(unsigned)child, (unsigned)reaped, status);

    if (failNonZero && (!WIFEXITED(status) || WEXITSTATUS(status))) {
	const char *cmd = argvJoin(argv, ' ');
	int rc = (WIFEXITED(status) ? WEXITSTATUS(status) : -1);

	rpmlog(RPMLOG_ERR, _("Command \"%s\" failed, exit(%d)\n"), cmd, rc);
	cmd = _free(cmd);
	iob = rpmiobFree(iob);
	return NULL;
    }
    if (writeBytesLeft) {
	rpmlog(RPMLOG_ERR, _("failed to write all data to %s\n"), argv[0]);
	iob = rpmiobFree(iob);
	return NULL;
    }
    return iob;
}

int rpmfcExec(ARGV_t av, rpmiob iob_stdin, rpmiob * iob_stdoutp,
		int failnonzero)
{
    const char * s = NULL;
    ARGV_t xav = NULL;
    ARGV_t pav = NULL;
    int pac = 0;
    int ec = -1;
    rpmiob iob = NULL;
    const char * buf_stdin = NULL;
    size_t buf_stdin_len = 0;
    int xx;

    if (iob_stdoutp)
	*iob_stdoutp = NULL;
    if (!(av && *av))
	goto exit;

    /* Find path to executable with (possible) args. */
    s = rpmExpand(av[0], NULL);
    if (!(s && *s))
	goto exit;

    /* Parse args buried within expanded executable. */
    pac = 0;
    xx = poptParseArgvString(s, &pac, (const char ***)&pav);
    if (!(xx == 0 && pac > 0 && pav != NULL))
	goto exit;

    /* Build argv, appending args to the executable args. */
    xav = NULL;
    xx = argvAppend(&xav, pav);
    if (av[1])
	xx = rpmfcExpandAppend(&xav, av + 1);

    if (iob_stdin != NULL) {
	buf_stdin = rpmiobStr(iob_stdin);
	buf_stdin_len = rpmiobLen(iob_stdin);
    }

    /* Read output from exec'd helper. */
    iob = getOutputFrom(NULL, xav, buf_stdin, buf_stdin_len, failnonzero);

    if (iob_stdoutp != NULL) {
	*iob_stdoutp = iob;
	iob = NULL;	/* XXX don't free */
    }

    ec = 0;

exit:
    iob = rpmiobFree(iob);
    xav = argvFree(xav);
    pav = _free(pav);	/* XXX popt mallocs in single blob. */
    s = _free(s);
    return ec;
}

/**
 */
static int rpmfcSaveArg(/*@out@*/ ARGV_t * argvp, const char * key)
	/*@modifies *argvp @*/
	/*@requires maxSet(argvp) >= 0 @*/
{
    int rc = 0;

    if (argvSearch(*argvp, key, NULL) == NULL) {
	rc = argvAdd(argvp, key);
	rc = argvSort(*argvp, NULL);
    }
    return rc;
}

/**
 */
static char * rpmfcFileDep(/*@returned@*/ char * buf, size_t ix,
		/*@null@*/ rpmds ds)
	/*@globals internalState @*/
	/*@modifies buf, internalState @*/
	/*@requires maxSet(buf) >= 0 @*/
{
    rpmTag tagN = rpmdsTagN(ds);
    char deptype = 'X';

    buf[0] = '\0';
    switch (tagN) {
    default:
assert(0);
	/*@notreached@*/ break;
    case RPMTAG_PROVIDENAME:
	deptype = 'P';
	break;
    case RPMTAG_REQUIRENAME:
	deptype = 'R';
	break;
    }
/*@-nullpass@*/
    if (ds != NULL)
	sprintf(buf, "%08u%c %s %s 0x%08x", (unsigned)ix, deptype,
		rpmdsN(ds), rpmdsEVR(ds), rpmdsFlags(ds));
/*@=nullpass@*/
    return buf;
};

/*@null@*/
static void * rpmfcExpandRegexps(const char * str, int * nmirep)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies *nmirep, rpmGlobalMacroContext, internalState @*/
{
    ARGV_t av = NULL;
    int ac = 0;
    miRE mire = NULL;
    int nmire = 0;
    const char * s;
    int xx;
    int i;

    s = rpmExpand(str, NULL);
    if (s && *s) {
    	xx = poptParseArgvString(s, &ac, (const char ***)&av);
	s = _free(s);
    }
    if (ac == 0 || av == NULL || *av == NULL) {
	s = _free(s);
	goto exit;
    }

    for (i = 0; i < ac; i++) {
	xx = mireAppend(RPMMIRE_REGEX, 0, av[i], NULL, &mire, &nmire);
	/* XXX add REG_NOSUB? better error msg?  */
	if (xx) {
	    rpmlog(RPMLOG_NOTICE, 
			_("Compilation of pattern '%s'"
		        " (expanded from '%s') failed. Skipping ...\n"),
			av[i], str);
	    nmire--;	/* XXX does this actually skip?!? */
	}
    }
    if (nmire == 0)
	mire = mireFree(mire);

exit:
    av = _free(av);
    if (nmirep)
	*nmirep = nmire;
    return mire;
}

static int rpmfcMatchRegexps(void * _mire, int nmire,
		const char * str, char deptype)
	/*@modifies mires @*/
{
    miRE mire = (miRE) _mire;
    int xx;
    int i;

    for (i = 0; i < nmire; i++) {
#ifdef	DYING	/* XXX noisy. use --miredebug if you need this spewage */
	rpmlog(RPMLOG_DEBUG, D_("Checking %c: '%s'\n"), deptype, str);
#endif
	if ((xx = mireRegexec(mire + i, str, 0)) < 0)
	    continue;
	rpmlog(RPMLOG_NOTICE, _("Skipping %c: '%s'\n"), deptype, str);
	return 1;
    }
    return 0;
}

/*@null@*/
static void * rpmfcFreeRegexps(/*@only@*/ void * _mire, int nmire)
	/*@modifies mires @*/
{
    miRE mire = (miRE) _mire;
/*@-refcounttrans@*/
    return mireFreeAll(mire, nmire);
/*@=refcounttrans@*/
}

/**
 * Run per-interpreter dependency helper.
 * @param fc		file classifier
 * @param deptype	'P' == Provides:, 'R' == Requires:, helper
 * @param nsdep		class name for interpreter (e.g. "perl")
 * @return		0 on success
 */
static int rpmfcHelper(rpmfc fc, unsigned char deptype, const char * nsdep)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fc, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    miRE mire = NULL;
    int nmire = 0;
    const char * fn = fc->fn[fc->ix];
    char buf[BUFSIZ];
    rpmiob iob_stdout = NULL;
    rpmiob iob_stdin;
    const char *av[2];
    rpmds * depsp, ds;
    const char * N;
    const char * EVR;
    rpmTag tagN;
    evrFlags Flags;
    evrFlags dsContext;
    ARGV_t pav;
    const char * s;
    int pac;
    int xx;
    int i;

    switch (deptype) {
    default:
	return -1;
	/*@notreached@*/ break;
    case 'P':
	if (fc->skipProv)
	    return 0;
	xx = snprintf(buf, sizeof(buf), "%%{?__%s_provides}", nsdep);
	depsp = &fc->provides;
	dsContext = RPMSENSE_FIND_PROVIDES;
	tagN = RPMTAG_PROVIDENAME;
	mire = (miRE) fc->Pmires;
	nmire = fc->Pnmire;
	break;
    case 'R':
	if (fc->skipReq)
	    return 0;
	xx = snprintf(buf, sizeof(buf), "%%{?__%s_requires}", nsdep);
	depsp = &fc->requires;
	dsContext = RPMSENSE_FIND_REQUIRES;
	tagN = RPMTAG_REQUIRENAME;
	mire = (miRE) fc->Rmires;
	nmire = fc->Rnmire;
	break;
    }
    buf[sizeof(buf)-1] = '\0';
    av[0] = buf;
    av[1] = NULL;

    iob_stdin = rpmiobNew(0);
    iob_stdin = rpmiobAppend(iob_stdin, fn, 1);
    iob_stdout = NULL;
    xx = rpmfcExec(av, iob_stdin, &iob_stdout, 0);
    iob_stdin = rpmiobFree(iob_stdin);

    if (xx == 0 && iob_stdout != NULL) {
	pav = NULL;
	xx = argvSplit(&pav, rpmiobStr(iob_stdout), " \t\n\r");
	pac = argvCount(pav);
	if (pav)
	for (i = 0; i < pac; i++) {
	    N = pav[i];
	    EVR = "";
	    Flags = dsContext;
	    if (pav[i+1] && strchr("=<>", *pav[i+1])) {
		i++;
		for (s = pav[i]; *s; s++) {
		    switch(*s) {
		    default:
assert(*s != '\0');
			/*@switchbreak@*/ break;
		    case '=':
			Flags = (evrFlags) (Flags | RPMSENSE_EQUAL);
			/*@switchbreak@*/ break;
		    case '<':
			Flags = (evrFlags) (Flags | RPMSENSE_LESS);
			/*@switchbreak@*/ break;
		    case '>':
			Flags = (evrFlags) (Flags | RPMSENSE_GREATER);
			/*@switchbreak@*/ break;
		    }
		}
		i++;
		EVR = pav[i];
assert(EVR != NULL);
	    }

	    if (_filter_values && rpmfcMatchRegexps(mire, nmire, N, deptype))
		continue;

	    /* Add tracking dependency for versioned Provides: */
	    if (!fc->tracked && deptype == 'P' && *EVR != '\0') {
		static evrFlags _Flags = (evrFlags)
			(RPMSENSE_RPMLIB|(RPMSENSE_LESS|RPMSENSE_EQUAL));
		ds = rpmdsSingle(RPMTAG_REQUIRENAME,
			"rpmlib(VersionedDependencies)", "3.0.3-1",
			_Flags);
		xx = rpmdsMerge(&fc->requires, ds);
		(void)rpmdsFree(ds);
		ds = NULL;
		fc->tracked = 1;
	    }

	    ds = rpmdsSingle(tagN, N, EVR, Flags);

#if defined(RPM_VENDOR_MANDRIVA) /* filter-overlapping-dependencies */
	    int overlap = 0;
	    if (*depsp) {
		int ix = rpmdsSearch(*depsp, ds);
		if (ix >= 0) {
		    EVR_t lEVR = rpmEVRnew(RPMSENSE_ANY, 0),
			  rEVR = rpmEVRnew(RPMSENSE_ANY, 0);

		    rpmdsSetIx(*depsp, ix);

		    rpmEVRparse(rpmdsEVR(*depsp), lEVR);
		    rpmEVRparse(EVR, rEVR);
		    lEVR->Flags = rpmdsFlags(*depsp) | RPMSENSE_EQUAL;
		    rEVR->Flags = Flags | RPMSENSE_EQUAL;

		    if (rpmEVRcompare(lEVR, rEVR) < 0) {
			(*depsp)->EVR[(*depsp)->i] = EVR;
			(*depsp)->Flags[(*depsp)->i] = Flags;
			overlap = 1;
		    }
		    lEVR = rpmEVRfree(lEVR);
		    rEVR = rpmEVRfree(rEVR);
		}
	    }
	    if (!overlap)
#endif
	    /* Add to package dependencies. */
	    xx = rpmdsMerge(depsp, ds);

	    /* Add to file dependencies. */
	    xx = rpmfcSaveArg(&fc->ddict, rpmfcFileDep(buf, fc->ix, ds));

	    (void)rpmdsFree(ds);
	    ds = NULL;
	}

	pav = argvFree(pav);
    }
    iob_stdout = rpmiobFree(iob_stdout);

    return 0;
}

/**
 */
/*@-nullassign@*/
/*@unchecked@*/ /*@observer@*/
static struct rpmfcTokens_s rpmfcTokens[] = {
  { "directory",		RPMFC_DIRECTORY|RPMFC_INCLUDE },

  { " shared object",		RPMFC_LIBRARY },
  { " executable",		RPMFC_EXECUTABLE },
  { " statically linked",	RPMFC_STATIC },
  { " not stripped",		RPMFC_NOTSTRIPPED },
  { " archive",			RPMFC_ARCHIVE },

  { "MIPS, N32 MIPS32",		RPMFC_ELFMIPSN32|RPMFC_INCLUDE },
  { "ELF 32-bit",		RPMFC_ELF32|RPMFC_INCLUDE },
  { "ELF 64-bit",		RPMFC_ELF64|RPMFC_INCLUDE },

  { " script",			RPMFC_SCRIPT },
  { " text",			RPMFC_TEXT },
  { " document",		RPMFC_DOCUMENT },

  { " compressed",		RPMFC_COMPRESSED },

  { "troff or preprocessor input",	RPMFC_MANPAGE|RPMFC_INCLUDE },
  { "GNU Info",			RPMFC_MANPAGE|RPMFC_INCLUDE },

  { "perl script text",		RPMFC_PERL|RPMFC_INCLUDE },
  { "Perl5 module source text", RPMFC_PERL|RPMFC_MODULE|RPMFC_INCLUDE },

  { "PHP script text",		RPMFC_PHP|RPMFC_INCLUDE },
  { "G-IR binary database",	RPMFC_TYPELIB|RPMFC_INCLUDE },

  /* XXX "a /usr/bin/python -t script text executable" */
  /* XXX "python 2.3 byte-compiled" */
  { " /usr/bin/python",		RPMFC_PYTHON|RPMFC_INCLUDE },
  { "python ",			RPMFC_PYTHON|RPMFC_INCLUDE },

  { "libtool library ",		RPMFC_LIBTOOL|RPMFC_INCLUDE },
  { "pkgconfig ",		RPMFC_PKGCONFIG|RPMFC_INCLUDE },

  { "Bourne ",			RPMFC_BOURNE|RPMFC_INCLUDE },
  { "Bourne-Again ",		RPMFC_BOURNE|RPMFC_INCLUDE },

  { "Java ",			RPMFC_JAVA|RPMFC_INCLUDE },

  { "Mono/.Net assembly",	RPMFC_MONO|RPMFC_INCLUDE },

  { "ruby script text",		RPMFC_RUBY|RPMFC_INCLUDE },
  { "Ruby script text",		RPMFC_RUBY|RPMFC_INCLUDE },

  { "current ar archive",	RPMFC_STATIC|RPMFC_LIBRARY|RPMFC_ARCHIVE|RPMFC_INCLUDE },

  { "Zip archive data",		RPMFC_COMPRESSED|RPMFC_ARCHIVE|RPMFC_INCLUDE },
  { "tar archive",		RPMFC_ARCHIVE|RPMFC_INCLUDE },
  { "cpio archive",		RPMFC_ARCHIVE|RPMFC_INCLUDE },
  { "RPM v3",			RPMFC_ARCHIVE|RPMFC_INCLUDE },
  { "RPM v4",			RPMFC_ARCHIVE|RPMFC_INCLUDE },

  { " image",			RPMFC_IMAGE|RPMFC_INCLUDE },
  { " font",			RPMFC_FONT|RPMFC_INCLUDE },
  { " Font",			RPMFC_FONT|RPMFC_INCLUDE },

  { " commands",		RPMFC_SCRIPT|RPMFC_INCLUDE },
  { " script",			RPMFC_SCRIPT|RPMFC_INCLUDE },

  { "empty",			RPMFC_WHITE|RPMFC_INCLUDE },

  { "HTML",			RPMFC_WHITE|RPMFC_INCLUDE },
  { "SGML",			RPMFC_WHITE|RPMFC_INCLUDE },
  { "XML",			RPMFC_WHITE|RPMFC_INCLUDE },

  { " program text",		RPMFC_WHITE|RPMFC_INCLUDE },
  { " source",			RPMFC_WHITE|RPMFC_INCLUDE },
  { "GLS_BINARY_LSB_FIRST",	RPMFC_WHITE|RPMFC_INCLUDE },
  { " DB ",			RPMFC_WHITE|RPMFC_INCLUDE },

  { "ASCII English text",	RPMFC_WHITE|RPMFC_INCLUDE },
  { "ASCII text",		RPMFC_WHITE|RPMFC_INCLUDE },
  { "ISO-8859 text",		RPMFC_WHITE|RPMFC_INCLUDE },

  { "symbolic link to",		RPMFC_SYMLINK },
  { "socket",			RPMFC_DEVICE },
  { "special",			RPMFC_DEVICE },

  { "ASCII",			RPMFC_WHITE },
  { "ISO-8859",			RPMFC_WHITE },

  { "data",			RPMFC_WHITE },

  { "application",		RPMFC_WHITE },
  { "boot",			RPMFC_WHITE },
  { "catalog",			RPMFC_WHITE },
  { "code",			RPMFC_WHITE },
  { "file",			RPMFC_WHITE },
  { "format",			RPMFC_WHITE },
  { "message",			RPMFC_WHITE },
  { "program",			RPMFC_WHITE },

  { "broken symbolic link to ",	RPMFC_WHITE|RPMFC_ERROR },
  { "can't read",		RPMFC_WHITE|RPMFC_ERROR },
  { "can't stat",		RPMFC_WHITE|RPMFC_ERROR },
  { "executable, can't read",	RPMFC_WHITE|RPMFC_ERROR },
  { "core file",		RPMFC_WHITE|RPMFC_ERROR },

  { NULL,			RPMFC_BLACK }
};
/*@=nullassign@*/

int rpmfcColoring(const char * fmstr)
{
    rpmfcToken fct;
    int fcolor = RPMFC_BLACK;

    for (fct = rpmfcTokens; fct->token != NULL; fct++) {
	if (strstr(fmstr, fct->token) == NULL)
	    continue;
	fcolor |= fct->colors;
	if (fcolor & RPMFC_INCLUDE)
	    return fcolor;
    }
    return fcolor;
}

void rpmfcPrint(const char * msg, rpmfc fc, FILE * fp)
{
    int fcolor;
    int ndx;
    int cx;
    int dx;
    size_t fx;

unsigned nprovides;
unsigned nrequires;

    if (fp == NULL) fp = stderr;

    if (msg)
	fprintf(fp, "===================================== %s\n", msg);

nprovides = rpmdsCount(fc->provides);
nrequires = rpmdsCount(fc->requires);

    if (fc)
    for (fx = 0; fx < fc->nfiles; fx++) {
assert(fx < fc->fcdictx->nvals);
	cx = fc->fcdictx->vals[fx];
assert(fx < fc->fcolor->nvals);
	fcolor = fc->fcolor->vals[fx];

	fprintf(fp, "%3d %s", (int)fx, fc->fn[fx]);
	if (fcolor != RPMFC_BLACK)
		fprintf(fp, "\t0x%x", fc->fcolor->vals[fx]);
	else
		fprintf(fp, "\t%s", fc->cdict[cx]);
	fprintf(fp, "\n");

	if (fc->fddictx == NULL || fc->fddictn == NULL)
	    continue;

assert(fx < fc->fddictx->nvals);
	dx = fc->fddictx->vals[fx];
assert(fx < fc->fddictn->nvals);
	ndx = fc->fddictn->vals[fx];

	while (ndx-- > 0) {
	    const char * depval;
	    unsigned char deptype;
	    unsigned ix;

	    ix = fc->ddictx->vals[dx++];
	    deptype = ((ix >> 24) & 0xff);
	    ix &= 0x00ffffff;
	    depval = NULL;
	    switch (deptype) {
	    default:
assert(depval != NULL);
		/*@switchbreak@*/ break;
	    case 'P':
		if (nprovides > 0) {
assert(ix < nprovides);
		    (void) rpmdsSetIx(fc->provides, ix-1);
		    if (rpmdsNext(fc->provides) >= 0)
			depval = rpmdsDNEVR(fc->provides);
		}
		/*@switchbreak@*/ break;
	    case 'R':
		if (nrequires > 0) {
assert(ix < nrequires);
		    (void) rpmdsSetIx(fc->requires, ix-1);
		    if (rpmdsNext(fc->requires) >= 0)
			depval = rpmdsDNEVR(fc->requires);
		}
		/*@switchbreak@*/ break;
	    }
	    if (depval)
		fprintf(fp, "\t%s\n", depval);
	}
    }
}

/**
 * Extract script dependencies.
 * @param fc		file classifier
 * @return		0 on success
 */
static int rpmfcSCRIPT(rpmfc fc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fc, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * fn = fc->fn[fc->ix];
    const char * bn;
    rpmds ds;
    char buf[BUFSIZ];
    FILE * fp;
    char * s, * se;
    int i;
    int is_executable;
    int xx;
    const char * defaultdocdir = NULL;

    /* Extract dependencies only from files with executable bit set. */
    {	struct stat sb, * st = &sb;
	if (stat(fn, st) != 0)
	    return -1;
	is_executable = (int)(st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH));
    }

    fp = fopen(fn, "r");
    if (fp == NULL || ferror(fp)) {
	if (fp) (void) fclose(fp);
	return -1;
    }

    /* Look for #! interpreter in first 10 lines. */
    for (i = 0; i < 10; i++) {

	s = fgets(buf, sizeof(buf) - 1, fp);
	if (s == NULL || ferror(fp) || feof(fp))
	    break;
	s[sizeof(buf)-1] = '\0';
	if (!(s[0] == '#' && s[1] == '!'))
	    continue;
	s += 2;

	while (*s && strchr(" \t\n\r", *s) != NULL)
	    s++;
	if (*s == '\0')
	    continue;
	if (*s != '/')
	    continue;

	for (se = s+1; *se; se++) {
	    if (strchr(" \t\n\r", *se) != NULL)
		/*@innerbreak@*/ break;
	}
	*se = '\0';
	se++;

	if (!_filter_values
	 || (!fc->skipReq
	  && !rpmfcMatchRegexps(fc->Rmires, fc->Rnmire, s, 'R')))
	if (is_executable) {
	    /* Add to package requires. */
	    ds = rpmdsSingle(RPMTAG_REQUIRENAME, s, "", RPMSENSE_FIND_REQUIRES);
	    xx = rpmdsMerge(&fc->requires, ds);

	    /* Add to file requires. */
	    xx = rpmfcSaveArg(&fc->ddict, rpmfcFileDep(se, fc->ix, ds));

	    (void)rpmdsFree(ds);
	    ds = NULL;
	}

	/* Set color based on interpreter name. */
	/* XXX magic token should have already done this?!? */
/*@-moduncon@*/
	bn = basename(s);
/*@=moduncon@*/
	if (!strcmp(bn, "perl"))
	    fc->fcolor->vals[fc->ix] |= RPMFC_PERL;
	else if (!strncmp(bn, "python", sizeof("python")-1))
	    fc->fcolor->vals[fc->ix] |= RPMFC_PYTHON;
	else if (!strncmp(bn, "php", sizeof("php")-1))
	    fc->fcolor->vals[fc->ix] |= RPMFC_PHP;
	else if (!strncmp(bn, "ruby", sizeof("ruby")-1))
	    fc->fcolor->vals[fc->ix] |= RPMFC_RUBY;

	break;
    }

    (void) fclose(fp);

    if (fc->fcolor->vals[fc->ix] & RPMFC_PERL) {
    	defaultdocdir = rpmExpand("%{?_defaultdocdir}", NULL);
	/* XXX coverity #1035723 */
	/* XXX coverity #1035883 */
    	if (defaultdocdir == NULL || *defaultdocdir == '\0')  {
            defaultdocdir = _free(defaultdocdir);
            defaultdocdir = xstrdup("/usr/share/doc");
	}

	if (strncmp(fn, defaultdocdir, strlen(defaultdocdir))) {
	    if (fc->fcolor->vals[fc->ix] & RPMFC_MODULE)
		xx = rpmfcHelper(fc, 'P', "perl");
	    if (is_executable || (fc->fcolor->vals[fc->ix] & RPMFC_MODULE))
		xx = rpmfcHelper(fc, 'R', "perl");
	}
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_PYTHON) {
	xx = rpmfcHelper(fc, 'P', "python");
#ifdef	NOTYET
	if (is_executable)
#endif
	    xx = rpmfcHelper(fc, 'R', "python");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_LIBTOOL) {
	xx = rpmfcHelper(fc, 'P', "libtool");
#ifdef	NOTYET
	if (is_executable)
#endif
	    xx = rpmfcHelper(fc, 'R', "libtool");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_PKGCONFIG) {
	xx = rpmfcHelper(fc, 'P', "pkgconfig");
#ifdef	NOTYET
	if (is_executable)
#endif
	    xx = rpmfcHelper(fc, 'R', "pkgconfig");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_BOURNE) {
#ifdef	NOTYET
	xx = rpmfcHelper(fc, 'P', "executable");
#endif
	if (is_executable)
	    xx = rpmfcHelper(fc, 'R', "executable");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_PHP) {
	xx = rpmfcHelper(fc, 'P', "php");
	if (is_executable)
	    xx = rpmfcHelper(fc, 'R', "php");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_MONO) {
	xx = rpmfcHelper(fc, 'P', "mono");
	if (is_executable)
	    xx = rpmfcHelper(fc, 'R', "mono");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_RUBY) {
	xx = rpmfcHelper(fc, 'P', "ruby");
#ifdef	NOTYET
	if (is_executable)
#endif
	    xx = rpmfcHelper(fc, 'R', "ruby");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_FONT) {
       xx = rpmfcHelper(fc, 'P', "font");
       /* XXX: currently of no use, but for the sake of consistency... */
       xx = rpmfcHelper(fc, 'R', "font");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_HASKELL) {
       xx = rpmfcHelper(fc, 'P', "haskell");
       xx = rpmfcHelper(fc, 'R', "haskell");
    } else
    if (fc->fcolor->vals[fc->ix] & RPMFC_TYPELIB) {
	xx = rpmfcHelper(fc, 'P', "typelib");
#ifdef	NOTYET
	if (is_executable)
#endif
	    xx = rpmfcHelper(fc, 'R', "typelib");
    } else
    if ((fc->fcolor->vals[fc->ix] & (RPMFC_MODULE|RPMFC_LIBRARY)) &&
	    strstr(fn, "/gstreamer")) {
	xx = rpmfcHelper(fc, 'P', "gstreamer");
	/* XXX: currently of no use, but for the sake of consistency... */
	xx = rpmfcHelper(fc, 'R', "gstreamer");
    }

/*@-observertrans@*/
    defaultdocdir = _free(defaultdocdir) ;
/*@=observertrans@*/
    return 0;
}

/**
 * Merge provides/requires dependencies into a rpmfc container.
 * @param context	merge dependency set(s) container
 * @param ds		dependency set to merge
 * @return		0 on success
 */
static int rpmfcMergePR(void * context, rpmds ds)
	/*@globals fileSystem, internalState @*/
	/*@modifies ds, fileSystem, internalState @*/
{
    rpmfc fc = (rpmfc) context;
    char buf[BUFSIZ];
    int rc = 0;

if (_rpmfc_debug < 0)
fprintf(stderr, "*** rpmfcMergePR(%p, %p) %s\n", context, ds, tagName(rpmdsTagN(ds)));
    switch(rpmdsTagN(ds)) {
    default:
	rc = -1;
	break;
    case RPMTAG_PROVIDENAME:
	if (!_filter_values
	 || (!fc->skipProv
	  && !rpmfcMatchRegexps(fc->Pmires, fc->Pnmire, ds->N[0], 'P')))
	{
	    /* Add to package provides. */
	    rc = rpmdsMerge(&fc->provides, ds);

	    /* Add to file dependencies. */
	    buf[0] = '\0';
	    rc = rpmfcSaveArg(&fc->ddict, rpmfcFileDep(buf, fc->ix, ds));
	}
	break;
    case RPMTAG_REQUIRENAME:
	if (!_filter_values
	 || (!fc->skipReq
	  && !rpmfcMatchRegexps(fc->Rmires, fc->Rnmire, ds->N[0], 'R')))
	{
	    /* Add to package requires. */
	    rc = rpmdsMerge(&fc->requires, ds);

	    /* Add to file dependencies. */
	    buf[0] = '\0';
	    rc = rpmfcSaveArg(&fc->ddict, rpmfcFileDep(buf, fc->ix, ds));
	}
	break;
    }
    return rc;
}

/**
 * Extract Elf dependencies.
 * @param fc		file classifier
 * @return		0 on success
 */
static int rpmfcELF(rpmfc fc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * fn = fc->fn[fc->ix];
    int flags = 0;

    if (fc->skipProv)
	flags |= RPMELF_FLAG_SKIPPROVIDES;
    if (fc->skipReq)
	flags |= RPMELF_FLAG_SKIPREQUIRES;

    return rpmdsELF(fn, flags, rpmfcMergePR, fc);
}

#if defined(RPM_VENDOR_MANDRIVA)
/** \ingroup rpmds
 * Extract dependencies from a symlink.
 * XXX Prototype added to keep GCC quite and avoid adding a symbol.
 * @param fn		file name
 * @param flags		1: skip provides 2: skip requires
 * @param *add		add(arg, ds) saves next provide/require symlink dependency.
 * @param context	add() callback context
 * @return		0 on success
 */
extern int rpmdsSymlink(const char * fn, int flags,
		int (*add) (void * context, rpmds ds), void * context)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/;

static int rpmfcSYMLINK(rpmfc fc)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char * fn = fc->fn[fc->ix];
    int flags = 0;

    if (fc->skipProv)
	flags |= RPMELF_FLAG_SKIPPROVIDES;
    if (fc->skipReq)
	flags |= RPMELF_FLAG_SKIPREQUIRES;

    return rpmdsSymlink(fn, flags, rpmfcMergePR, fc);
}
#endif	/* RPM_VENDOR_MANDRIVA */

typedef struct rpmfcApplyTbl_s {
    int (*func) (rpmfc fc);
    int colormask;
} * rpmfcApplyTbl;

/**
 * XXX Having two entries for rpmfcSCRIPT may be unnecessary duplication.
 */
/*@-nullassign@*/
/*@unchecked@*/
static struct rpmfcApplyTbl_s rpmfcApplyTable[] = {
    { rpmfcELF,		RPMFC_ELF },
    { rpmfcSCRIPT,	(RPMFC_SCRIPT|RPMFC_FONT|RPMFC_HASKELL|RPMFC_PERL|RPMFC_PYTHON|RPMFC_LIBTOOL|RPMFC_PKGCONFIG|RPMFC_BOURNE|RPMFC_JAVA|RPMFC_PHP|RPMFC_MONO|RPMFC_TYPELIB) },
#if defined(RPM_VENDOR_MANDRIVA)
    { rpmfcSYMLINK,	RPMFC_SYMLINK },
#endif
    { NULL, 0 }
};
/*@=nullassign@*/

rpmRC rpmfcApply(rpmfc fc)
{
    rpmfcApplyTbl fcat;
    const char * s;
    char * se;
    rpmds ds;
    const char * fn;
    const char * N;
    const char * EVR;
    evrFlags Flags;
    unsigned char deptype;
    int nddict;
    int previx;
    unsigned int val;
    int dix;
    int ix;
    int i;
    int xx;
    int skipping;

    miRE mire;
    int skipProv = fc->skipProv;
    int skipReq = fc->skipReq;
    int j;

    if (_filter_execs) {
	fc->PFnmire = 0;
	fc->PFmires = rpmfcExpandRegexps("%{?__noautoprovfiles}", &fc->PFnmire);
	if (fc->PFnmire > 0)
	    rpmlog(RPMLOG_DEBUG, D_("added %d %%__noautoprovfiles patterns.\n"),
			fc->PFnmire);
	fc->RFnmire = 0;
	fc->RFmires = rpmfcExpandRegexps("%{?__noautoreqfiles}", &fc->RFnmire);
	if (fc->RFnmire > 0)
	    rpmlog(RPMLOG_DEBUG, D_("added %d %%__noautoreqfiles patterns.\n"),
			fc->RFnmire);
	fc->Pnmire = 0;
	fc->Pmires = rpmfcExpandRegexps("%{?__noautoprov}", &fc->Pnmire);
	if (fc->Pnmire > 0)
	    rpmlog(RPMLOG_DEBUG, D_("added %d %%__noautoprov patterns.\n"),
			fc->Pnmire);
	fc->Rnmire = 0;
	fc->Rmires = rpmfcExpandRegexps("%{?__noautoreq}", &fc->Rnmire);
	if (fc->Rnmire > 0)
	    rpmlog(RPMLOG_DEBUG, D_("added %d %%__noautoreq patterns.\n"),
			fc->Rnmire);
    }

/* Make sure something didn't go wrong previously! */
assert(fc->fn != NULL);
    /* Generate package and per-file dependencies. */
    for (fc->ix = 0; fc->fn[fc->ix] != NULL; fc->ix++) {

	/* XXX Insure that /usr/lib{,64}/python files are marked RPMFC_PYTHON */
	/* XXX HACK: classification by path is intrinsically stupid. */
	{   fn = strstr(fc->fn[fc->ix], "/usr/lib");
	    if (fn) {
		fn += sizeof("/usr/lib")-1;
		if ((fn[0] == '3' && fn[1] == '2') || 
			(fn[0] == '6' && fn[1] == '4'))
		    fn += 2;
		if (!strncmp(fn, "/python", sizeof("/python")-1))
		    fc->fcolor->vals[fc->ix] |= RPMFC_PYTHON;
		else if (!strncmp(fn, "/ruby", sizeof("/ruby")-1)) {
		    fc->fcolor->vals[fc->ix] |= RPMFC_RUBY;
		    if ((fn = strstr(fn, "/specifications/")) &&
			(fn = strrchr(fn, '.')) && !strcmp(fn, ".gemspec"))
			fc->fcolor->vals[fc->ix] |= RPMFC_MODULE;
		}
		/* XXX: lacking better, more generic classifier... */
		else if (!strncmp(fn, "/gstreamer", sizeof("/gstreamer")-1) &&
			fc->fcolor->vals[fc->ix] & RPMFC_LIBRARY)
		    fc->fcolor->vals[fc->ix] |= (RPMFC_MODULE|RPMFC_SCRIPT);
	    }
	}

       /* XXX ugly quick & dirty integration of haskell() dependencies */
       {   fn = strstr(fc->fn[fc->ix], "/usr/share/haskell-deps");
           if (fn)
               fc->fcolor->vals[fc->ix] |= RPMFC_HASKELL;
       }

	if (fc->fcolor->vals[fc->ix])
	for (fcat = rpmfcApplyTable; fcat->func != NULL; fcat++) {
	    if (!(fc->fcolor->vals[fc->ix] & fcat->colormask))
		/*@innercontinue@*/ continue;

	    if (_filter_execs) {
		fc->skipProv = skipProv;
		fc->skipReq = skipReq;
		if ((mire = (miRE)fc->PFmires) != NULL)
		for (j = 0; j < fc->PFnmire; j++, mire++) {
		    fn = fc->fn[fc->ix] + fc->brlen;
		    if ((xx = mireRegexec(mire, fn, 0)) < 0)
			/*@innercontinue@*/ continue;
		    rpmlog(RPMLOG_NOTICE, _("skipping %s provides detection\n"),
				fn);
		    fc->skipProv = 1;
		    /*@innerbreak@*/ break;
		}
		if ((mire = (miRE)fc->RFmires) != NULL)
		for (j = 0; j < fc->RFnmire; j++, mire++) {
		    fn = fc->fn[fc->ix] + fc->brlen;
		    if ((xx = mireRegexec(mire, fn, 0)) < 0)
			/*@innercontinue@*/ continue;
		    rpmlog(RPMLOG_NOTICE, _("skipping %s requires detection\n"),
				fn);
		    fc->skipReq = 1;
		    /*@innerbreak@*/ break;
		}
	    }

	    xx = (*fcat->func) (fc);
	}
    }

    if (_filter_execs) {
	fc->PFmires = rpmfcFreeRegexps(fc->PFmires, fc->PFnmire);
	fc->RFmires = rpmfcFreeRegexps(fc->RFmires, fc->RFnmire);
	fc->Pmires = rpmfcFreeRegexps(fc->Pmires, fc->Pnmire);
	fc->Rmires = rpmfcFreeRegexps(fc->Rmires, fc->Rnmire);
    }
    fc->skipProv = skipProv;
    fc->skipReq = skipReq;

    /* Generate per-file indices into package dependencies. */
    nddict = argvCount(fc->ddict);
    previx = -1;
    for (i = 0; i < nddict; i++) {
	s = fc->ddict[i];

	/* Parse out (file#,deptype,N,EVR,Flags) */
	ix = strtol(s, &se, 10);
assert(se != NULL);
	deptype = *se++;
	se++;
	N = se;
	while (*se && *se != ' ')
	    se++;
	*se++ = '\0';
	EVR = se;
	while (*se && *se != ' ')
	    se++;
	*se++ = '\0';
	Flags = (evrFlags) strtol(se, NULL, 16);

	dix = -1;
	skipping = 0;
	switch (deptype) {
	default:
	    /*@switchbreak@*/ break;
	case 'P':	
	    skipping = fc->skipProv;
	    ds = rpmdsSingle(RPMTAG_PROVIDENAME, N, EVR, Flags);
	    dix = rpmdsFind(fc->provides, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	    /*@switchbreak@*/ break;
	case 'R':
	    skipping = fc->skipReq;
	    ds = rpmdsSingle(RPMTAG_REQUIRENAME, N, EVR, Flags);
	    dix = rpmdsFind(fc->requires, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	    /*@switchbreak@*/ break;
	}

/* XXX assertion incorrect while generating -debuginfo deps. */
#if 0
assert(dix >= 0);
#else
	if (dix < 0)
	    continue;
#endif

	val = (deptype << 24) | (dix & 0x00ffffff);
	xx = argiAdd(&fc->ddictx, -1, val);

	if (previx != ix) {
	    previx = ix;
	    xx = argiAdd(&fc->fddictx, ix, argiCount(fc->ddictx)-1);
	}
	if (fc->fddictn && fc->fddictn->vals && !skipping)
	    fc->fddictn->vals[ix]++;
    }

    return RPMRC_OK;
}

rpmRC rpmfcClassify(rpmfc fc, ARGV_t argv, rpmuint16_t * fmode)
{
    ARGV_t fcav = NULL;
    ARGV_t dav;
    rpmmg mg = NULL;
    const char * s, * se;
    size_t slen;
    int fcolor;
    int xx;
    const char * magicfile = NULL;

    if (fc == NULL || argv == NULL)
	return RPMRC_OK;

    magicfile = rpmExpand("%{?_rpmfc_magic_path}", NULL);
    if (magicfile == NULL || *magicfile == '\0')
	magicfile = _free(magicfile);

    mg = rpmmgNew(magicfile, 0);
assert(mg != NULL);	/* XXX figger a proper return path. */

    fc->nfiles = argvCount(argv);

    /* Initialize the per-file dictionary indices. */
    xx = argiAdd(&fc->fddictx, fc->nfiles-1, 0);
    xx = argiAdd(&fc->fddictn, fc->nfiles-1, 0);

    /* Build (sorted) file class dictionary. */
    xx = argvAdd(&fc->cdict, "");
    xx = argvAdd(&fc->cdict, "directory");

    for (fc->ix = 0; fc->ix < fc->nfiles; fc->ix++) {
	const char * ftype;
	int freeftype;
	rpmuint16_t mode = (fmode ? fmode[fc->ix] : 0);
	int urltype;

	ftype = "";	freeftype = 0;
	urltype = urlPath(argv[fc->ix], &s);
assert(s != NULL && *s == '/');
	slen = strlen(s);

	switch (mode & S_IFMT) {
	case S_IFCHR:	ftype = "character special";	/*@switchbreak@*/ break;
	case S_IFBLK:	ftype = "block special";	/*@switchbreak@*/ break;
#if defined(S_IFIFO)
	case S_IFIFO:	ftype = "fifo (named pipe)";	/*@switchbreak@*/ break;
#endif
#if defined(S_IFSOCK)
/*@-unrecog@*/
	case S_IFSOCK:	ftype = "socket";		/*@switchbreak@*/ break;
/*@=unrecog@*/
#endif
	case S_IFDIR:
	case S_IFLNK:
	case S_IFREG:
	default:

#define	_suffix(_s, _x) \
    (slen >= sizeof(_x) && !strcmp((_s)+slen-(sizeof(_x)-1), (_x)))

	    /* XXX all files with extension ".pm" are perl modules for now. */
	    if (_suffix(s, ".pm"))
		ftype = "Perl5 module source text";

	    /* XXX all files with extension ".jar" are java archives for now. */
	    else if (_suffix(s, ".jar"))
		ftype = "Java archive file";

	    /* XXX all files with extension ".class" are java class files for now. */
	    else if (_suffix(s, ".class"))
		ftype = "Java class file";

	    /* XXX all files with extension ".la" are libtool for now. */
	    else if (_suffix(s, ".la"))
		ftype = "libtool library file";

	    /* XXX all files with extension ".pc" are pkgconfig for now. */
	    else if (_suffix(s, ".pc"))
		ftype = "pkgconfig file";

	    /* XXX all files with extension ".php" are PHP for now. */
	    else if (_suffix(s, ".php"))
		ftype = "PHP script text";

	    /* XXX files with extension ".typelib" are GNOME typelib for now. */
	    else if (_suffix(s, ".typelib"))
		ftype = "G-IR binary database";

	    /* XXX files with extension ".js" have GNOME typelib requires for now */
	    else if (_suffix(s, ".js"))
		ftype = "G-IR binary database";

	    /* XXX skip all files in /dev/ which are (or should be) %dev dummies. */
	    else if (slen >= fc->brlen+sizeof("/dev/") && !strncmp(s+fc->brlen, "/dev/", sizeof("/dev/")-1))
		ftype = "";
	    else if (magicfile) {
		ftype = rpmmgFile(mg, s);
assert(ftype != NULL);	/* XXX never happens, rpmmgFile() returns "" */
		freeftype = 1;
	    }
	    /*@switchbreak@*/ break;
	}

	se = ftype;

if (_rpmfc_debug)	/* XXX noisy */
	rpmlog(RPMLOG_DEBUG, "%s: %s\n", s, se);

	/* Save the path. */
	xx = argvAdd(&fc->fn, s);

	/* Save the file type string. */
	xx = argvAdd(&fcav, se);

	/* Add (filtered) entry to sorted class dictionary. */
	fcolor = rpmfcColoring(se);
	xx = argiAdd(&fc->fcolor, (int)fc->ix, fcolor);

	if (fcolor != RPMFC_WHITE && (fcolor & RPMFC_INCLUDE))
	    xx = rpmfcSaveArg(&fc->cdict, se);

/*@-modobserver -observertrans @*/	/* XXX mixed types in variable */
	if (freeftype)
	    ftype = _free(ftype);
/*@=modobserver =observertrans @*/
    }

    /* Build per-file class index array. */
    fc->fknown = 0;
    for (fc->ix = 0; fc->ix < fc->nfiles; fc->ix++) {
	se = fcav[fc->ix];
assert(se != NULL);

	dav = argvSearch(fc->cdict, se, NULL);
	if (dav) {
	    xx = argiAdd(&fc->fcdictx, (int)fc->ix, (dav - fc->cdict));
	    fc->fknown++;
	} else {
	    xx = argiAdd(&fc->fcdictx, (int)fc->ix, 0);
	    fc->fwhite++;
	}
    }

    fcav = argvFree(fcav);

    mg = rpmmgFree(mg);
    rpmlog(RPMLOG_DEBUG,
		D_("categorized %d files into %u classes (using %s).\n"),
		(unsigned)fc->nfiles, argvCount(fc->cdict), magicfile);
    magicfile = _free(magicfile);

    return RPMRC_OK;
}

/**
 */
typedef struct DepMsg_s * DepMsg_t;

/**
 */
struct DepMsg_s {
/*@observer@*/ /*@null@*/
    const char * msg;
/*@observer@*/
    const char * argv[4];
    rpmTag ntag;
    rpmTag vtag;
    rpmTag ftag;
    int mask;
    int toggle;
};

/**
 */
/*@-nullassign@*/
/*@unchecked@*/
static struct DepMsg_s depMsgs[] = {
  { "Provides",		{ "%{?__find_provides}", NULL, NULL, NULL },
	RPMTAG_PROVIDENAME, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS,
	0, -1 },
  { "Requires(interp)",	{ NULL, "interp", NULL, NULL },
	RPMTAG_REQUIRENAME, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_INTERP), 0 },
  { "Requires(rpmlib)",	{ NULL, "rpmlib", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_RPMLIB), 0 },
  { "Requires(verify)",	{ NULL, "verify", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	RPMSENSE_SCRIPT_VERIFY, 0 },
  { "Requires(pre)",	{ NULL, "pre", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_SCRIPT_PRE), 0 },
  { "Requires(post)",	{ NULL, "post", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_SCRIPT_POST), 0 },
  { "Requires(preun)",	{ NULL, "preun", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_SCRIPT_PREUN), 0 },
  { "Requires(postun)",	{ NULL, "postun", NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,
	_notpre(RPMSENSE_SCRIPT_POSTUN), 0 },
  { "Requires",		{ "%{?__find_requires}", NULL, NULL, NULL },
	(rpmTag)-1, (rpmTag)-1, RPMTAG_REQUIREFLAGS,	/* XXX inherit name/version arrays */
	RPMSENSE_FIND_REQUIRES|RPMSENSE_TRIGGERIN|RPMSENSE_TRIGGERUN|RPMSENSE_TRIGGERPOSTUN|RPMSENSE_TRIGGERPREIN, 0 },
  { "Conflicts",	{ "%{?__find_conflicts}", NULL, NULL, NULL },
	RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS,
	0, -1 },
  { "Obsoletes",	{ "%{?__find_obsoletes}", NULL, NULL, NULL },
	RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS,
	0, -1 },
  { NULL,		{ NULL, NULL, NULL, NULL },	(rpmTag)0, (rpmTag)0, (rpmTag)0, 0, 0 }
};
/*@=nullassign@*/

/*@unchecked@*/
static DepMsg_t DepMsgs = depMsgs;

/**
 * Print dependencies in a header.
 * @param h		header
 */
static void printDeps(Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    DepMsg_t dm;
    rpmds ds = NULL;
    int flags = 0x2;	/* XXX no filtering, !scareMem */
    const char * DNEVR;
    evrFlags Flags;
    int bingo = 0;

    for (dm = DepMsgs; dm->msg != NULL; dm++) {
	if ((int)dm->ntag != -1) {
	    (void)rpmdsFree(ds);
	    ds = NULL;
	    ds = rpmdsNew(h, dm->ntag, flags);
	}
	if (dm->ftag == 0)
	    continue;

	ds = rpmdsInit(ds);
	if (ds == NULL)
	    continue;	/* XXX can't happen */

	bingo = 0;
	while (rpmdsNext(ds) >= 0) {

	    Flags = rpmdsFlags(ds);
	
	    if (!((Flags & dm->mask) ^ (dm->toggle)))
		/*@innercontinue@*/ continue;
	    if (bingo == 0) {
		rpmlog(RPMLOG_NOTICE, "%s:", (dm->msg ? dm->msg : ""));
		bingo = 1;
	    }
	    if ((DNEVR = rpmdsDNEVR(ds)) == NULL)
		/*@innercontinue@*/ continue;	/* XXX can't happen */
	    rpmlog(RPMLOG_NOTICE, " %s", DNEVR+2);
	}
	if (bingo)
	    rpmlog(RPMLOG_NOTICE, "\n");
    }
    (void)rpmdsFree(ds);
    ds = NULL;
}

/**
 */
static rpmRC rpmfcGenerateDependsHelper(const Spec spec, Package pkg, rpmfi fi)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies fi, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmiob iob_stdin;
    rpmiob iob_stdout;
    DepMsg_t dm;
    int failnonzero = 0;
    rpmRC rc = RPMRC_OK;

    /*
     * Create file manifest buffer to deliver to dependency finder.
     */
    iob_stdin = rpmiobNew(0);
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while (rpmfiNext(fi) >= 0)
	iob_stdin = rpmiobAppend(iob_stdin, rpmfiFN(fi), 1);

    for (dm = DepMsgs; dm->msg != NULL; dm++) {
	rpmTag tag;
	rpmsenseFlags tagflags;
	char * s;
	int xx;

	tag = (dm->ftag > 0) ? dm->ftag : dm->ntag;
	tagflags = (rpmsenseFlags) 0;
	s = NULL;

	switch(tag) {
	case RPMTAG_PROVIDEFLAGS:
	    if (!pkg->autoProv)
		continue;
	    failnonzero = 1;
	    tagflags = RPMSENSE_FIND_PROVIDES;
	    /*@switchbreak@*/ break;
	case RPMTAG_REQUIREFLAGS:
	    if (!pkg->autoReq)
		continue;
	    failnonzero = 0;
	    tagflags = RPMSENSE_FIND_REQUIRES;
	    /*@switchbreak@*/ break;
	default:
	    continue;
	    /*@notreached@*/ /*@switchbreak@*/ break;
	}

	xx = rpmfcExec(dm->argv, iob_stdin, &iob_stdout, failnonzero);
	if (xx == -1)
	    continue;

	s = rpmExpand(dm->argv[0], NULL);
	rpmlog(RPMLOG_NOTICE, _("Finding  %s: %s\n"), dm->msg,
		(s ? s : ""));
	s = _free(s);

	if (iob_stdout == NULL) {
	    rpmlog(RPMLOG_ERR, _("Failed to find %s:\n"), dm->msg);
	    rc = RPMRC_FAIL;
	    break;
	}

	/* Parse dependencies into header */
	if (spec->_parseRCPOT)
	    rc = spec->_parseRCPOT(spec, pkg, rpmiobStr(iob_stdout), tag,
				0, tagflags);
	iob_stdout = rpmiobFree(iob_stdout);

	if (rc) {
	    rpmlog(RPMLOG_ERR, _("Failed to find %s:\n"), dm->msg);
	    break;
	}
    }

    iob_stdin = rpmiobFree(iob_stdin);

    return rc;
}

/**
 */
/*@-nullassign@*/
/*@unchecked@*/
static struct DepMsg_s scriptMsgs[] = {
  { "Requires(pre)",	{ "%{?__scriptlet_requires}", NULL, NULL, NULL },
	RPMTAG_PREINPROG, RPMTAG_PREIN, RPMTAG_REQUIREFLAGS,
	RPMSENSE_SCRIPT_PRE, 0 },
  { "Requires(post)",	{ "%{?__scriptlet_requires}", NULL, NULL, NULL },
	RPMTAG_POSTINPROG, RPMTAG_POSTIN, RPMTAG_REQUIREFLAGS,
	RPMSENSE_SCRIPT_POST, 0 },
  { "Requires(preun)",	{ "%{?__scriptlet_requires}", NULL, NULL, NULL },
	RPMTAG_PREUNPROG, RPMTAG_PREUN, RPMTAG_REQUIREFLAGS,
	RPMSENSE_SCRIPT_PREUN, 0 },
  { "Requires(postun)",	{ "%{?__scriptlet_requires}", NULL, NULL, NULL },
	RPMTAG_POSTUNPROG, RPMTAG_POSTUN, RPMTAG_REQUIREFLAGS,
	RPMSENSE_SCRIPT_POSTUN, 0 },
  { NULL,		{ NULL, NULL, NULL, NULL },	(rpmTag)0, (rpmTag)0, (rpmTag)0, 0, 0 }
};
/*@=nullassign@*/

/*@unchecked@*/
static DepMsg_t ScriptMsgs = scriptMsgs;

/**
 */
static int rpmfcGenerateScriptletDeps(const Spec spec, Package pkg)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmiob iob_stdin = rpmiobNew(0);
    rpmiob iob_stdout = NULL;
    DepMsg_t dm;
    int failnonzero = 0;
    int rc = 0;
    int xx;

    for (dm = ScriptMsgs; dm->msg != NULL; dm++) {
	rpmTag tag;
	rpmsenseFlags tagflags;
	char * s;

	tag = dm->ftag;
	tagflags = (rpmsenseFlags) (RPMSENSE_FIND_REQUIRES | dm->mask);

	/* Retrieve scriptlet interpreter. */
	he->tag = dm->ntag;
	xx = headerGet(pkg->header, he, 0);
	if (!xx || he->p.str == NULL)
	    continue;
	xx = strcmp(he->p.str, "/bin/sh") && strcmp(he->p.str, "/bin/bash");
	he->p.ptr = _free(he->p.ptr);
	if (xx)
	    continue;

	/* Retrieve scriptlet body. */
	he->tag = dm->vtag;
	xx = headerGet(pkg->header, he, 0);
	if (!xx || he->p.str == NULL)
	    continue;
	iob_stdin = rpmiobEmpty(iob_stdin);
	iob_stdin = rpmiobAppend(iob_stdin, he->p.str, 1);
	iob_stdin = rpmiobRTrim(iob_stdin);
	he->p.ptr = _free(he->p.ptr);

	xx = rpmfcExec(dm->argv, iob_stdin, &iob_stdout, failnonzero);
	if (xx == -1)
	    continue;

	/* Parse dependencies into header */
	s = rpmiobStr(iob_stdout);
	if (s != NULL && *s != '\0') {
	    char * se = s;
	    /* XXX Convert "executable(/path/to/file)" to "/path/to/file". */
	    while ((se = strstr(se, "executable(/")) != NULL) {
/*@-modobserver@*/	/* FIX: rpmiobStr should not be observer */
		se = stpcpy(se,     "           ");
		*se = '/';	/* XXX stpcpy truncates the '/' */
/*@=modobserver@*/
		se = strchr(se, ')');
		if (se == NULL)
		    /*@innerbreak@*/ break;
		*se++ = ' ';
	    }
	    if (spec->_parseRCPOT)
		rc = spec->_parseRCPOT(spec, pkg, s, tag, 0, tagflags);
	}
	iob_stdout = rpmiobFree(iob_stdout);

    }

    iob_stdin = rpmiobFree(iob_stdin);

    return rc;
}

rpmRC rpmfcGenerateDepends(void * _spec, void * _pkg)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const Spec spec = (Spec) _spec;
    Package pkg = (Package) _pkg;
    rpmfi fi = pkg->fi;
    rpmfc fc = NULL;
    rpmds ds;
    int flags = 0x2;	/* XXX no filtering, !scareMem */
    ARGV_t av;
    rpmuint16_t * fmode;
    int ac = rpmfiFC(fi);
    char buf[BUFSIZ];
    const char * N;
    const char * EVR;
    int genConfigDeps, internaldeps;
    rpmRC rc = RPMRC_OK;
    int i;
    int xx;

    /* Skip packages with no files. */
    if (ac <= 0)
	return RPMRC_OK;

    /* Skip packages that have dependency generation disabled. */
    if (! (pkg->autoReq || pkg->autoProv))
	return RPMRC_OK;

    /* If new-fangled dependency generation is disabled ... */
    internaldeps = rpmExpandNumeric("%{?_use_internal_dependency_generator}");
    if (internaldeps == 0) {
	/* ... then generate dependencies using %{__find_requires} et al. */
	rc = rpmfcGenerateDependsHelper(spec, pkg, fi);
	printDeps(pkg->header);
	return rc;
    }

    /* Generate scriptlet Dependencies. */
    if (internaldeps > 1)
	xx = rpmfcGenerateScriptletDeps(spec, pkg);

    /* Extract absolute file paths in argv format. */
    /* XXX TODO: should use argvFoo ... */
    av = (ARGV_t) xcalloc(ac+1, sizeof(*av));
    fmode = (rpmuint16_t *) xcalloc(ac+1, sizeof(*fmode));

    genConfigDeps = 0;
    fi = rpmfiInit(fi, 0);
    if (fi != NULL)
    while ((i = rpmfiNext(fi)) >= 0) {
	rpmfileAttrs fileAttrs;

	/* Does package have any %config files? */
	fileAttrs = (rpmfileAttrs) rpmfiFFlags(fi);
	genConfigDeps |= (fileAttrs & RPMFILE_CONFIG);

	av[i] = xstrdup(rpmfiFN(fi));
	fmode[i] = rpmfiFMode(fi);
    }
    av[ac] = NULL;

    fc = rpmfcNew();
    fc->skipProv = !pkg->autoProv;
    fc->skipReq = !pkg->autoReq;
    fc->tracked = 0;

    {	const char * buildRootURL;
	const char * buildRoot;
	buildRootURL = rpmGenPath(spec->rootURL, "%{?buildroot}", NULL);
	(void) urlPath(buildRootURL, &buildRoot);
	if (buildRoot && !strcmp(buildRoot, "/")) buildRoot = NULL;
	fc->brlen = (buildRoot ? strlen(buildRoot) : 0);
	buildRootURL = _free(buildRootURL);
    }

    /* Copy (and delete) manually generated dependencies to dictionary. */
    if (!fc->skipProv) {
	ds = rpmdsNew(pkg->header, RPMTAG_PROVIDENAME, flags);
	xx = rpmdsMerge(&fc->provides, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
	he->tag = RPMTAG_PROVIDENAME;
	xx = headerDel(pkg->header, he, 0);
	he->tag = RPMTAG_PROVIDEVERSION;
	xx = headerDel(pkg->header, he, 0);
	he->tag = RPMTAG_PROVIDEFLAGS;
	xx = headerDel(pkg->header, he, 0);

	/* Add config dependency, Provides: config(N) = EVR */
	if (genConfigDeps) {
	    static evrFlags _Flags = (evrFlags)(RPMSENSE_EQUAL|RPMSENSE_CONFIG);
	    N = rpmdsN(pkg->ds);
assert(N != NULL);
	    EVR = rpmdsEVR(pkg->ds);
assert(EVR != NULL);
	    sprintf(buf, "config(%s)", N);
	    ds = rpmdsSingle(RPMTAG_PROVIDENAME, buf, EVR, _Flags);
	    xx = rpmdsMerge(&fc->provides, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	}
    }

    if (!fc->skipReq) {
	ds = rpmdsNew(pkg->header, RPMTAG_REQUIRENAME, flags);
	xx = rpmdsMerge(&fc->requires, ds);
	(void)rpmdsFree(ds);
	ds = NULL;
	he->tag = RPMTAG_REQUIRENAME;
	xx = headerDel(pkg->header, he, 0);
	he->tag = RPMTAG_REQUIREVERSION;
	xx = headerDel(pkg->header, he, 0);
	he->tag = RPMTAG_REQUIREFLAGS;
	xx = headerDel(pkg->header, he, 0);

	/* Add config dependency,  Requires: config(N) = EVR */
	if (genConfigDeps) {
	    static evrFlags _Flags = (evrFlags)(RPMSENSE_EQUAL|RPMSENSE_CONFIG);
	    N = rpmdsN(pkg->ds);
assert(N != NULL);
	    EVR = rpmdsEVR(pkg->ds);
assert(EVR != NULL);
	    sprintf(buf, "config(%s)", N);
	    ds = rpmdsSingle(RPMTAG_REQUIRENAME, buf, EVR, _Flags);
	    xx = rpmdsMerge(&fc->requires, ds);
	    (void)rpmdsFree(ds);
	    ds = NULL;
	}
    }

    /* Build file class dictionary. */
    xx = rpmfcClassify(fc, av, fmode);

    /* Build file/package dependency dictionary. */
    xx = rpmfcApply(fc);

    /* Add per-file colors(#files) */
    he->tag = RPMTAG_FILECOLORS;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = argiData(fc->fcolor);
    he->c = argiCount(fc->fcolor);
assert(ac == (int)he->c);
    if (he->p.ptr != NULL && he->c > 0) {
	rpmuint32_t * fcolors = he->p.ui32p;

	/* XXX Make sure only primary (i.e. Elf32/Elf64) colors are added. */
	for (i = 0; i < (int)he->c; i++)
	    fcolors[i] &= 0x0f;

	xx = headerPut(pkg->header, he, 0);
    }

    /* Add classes(#classes) */
    he->tag = RPMTAG_CLASSDICT;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = argvData(fc->cdict);
    he->c = argvCount(fc->cdict);
    if (he->p.ptr != NULL && he->c > 0) {
	xx = headerPut(pkg->header, he, 0);
    }

    /* Add per-file classes(#files) */
    he->tag = RPMTAG_FILECLASS;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = argiData(fc->fcdictx);
    he->c = argiCount(fc->fcdictx);
assert(ac == (int)he->c);
    if (he->p.ptr != NULL && he->c > 0) {
	xx = headerPut(pkg->header, he, 0);
    }

    /* Add Provides: */
    if (fc->provides != NULL && (he->c = rpmdsCount(fc->provides)) > 0
     && !fc->skipProv)
    {
	he->tag = RPMTAG_PROVIDENAME;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = fc->provides->N;
	xx = headerPut(pkg->header, he, 0);

	/* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
/*@-nullpass@*/
	he->tag = RPMTAG_PROVIDEVERSION;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = fc->provides->EVR;
assert(he->p.ptr != NULL);
	xx = headerPut(pkg->header, he, 0);

	he->tag = RPMTAG_PROVIDEFLAGS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = (rpmuint32_t *) fc->provides->Flags;
assert(he->p.ptr != NULL);
	xx = headerPut(pkg->header, he, 0);
/*@=nullpass@*/
    }

    /* Add Requires: */
    if (fc->requires != NULL && (he->c = rpmdsCount(fc->requires)) > 0
     && !fc->skipReq)
    {
	he->tag = RPMTAG_REQUIRENAME;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = fc->requires->N;
assert(he->p.ptr != NULL);
	xx = headerPut(pkg->header, he, 0);

	/* XXX rpm prior to 3.0.2 did not always supply EVR and Flags. */
/*@-nullpass@*/
	he->tag = RPMTAG_REQUIREVERSION;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = fc->requires->EVR;
assert(he->p.ptr != NULL);
	xx = headerPut(pkg->header, he, 0);

	he->tag = RPMTAG_REQUIREFLAGS;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = (rpmuint32_t *) fc->requires->Flags;
assert(he->p.ptr != NULL);
	xx = headerPut(pkg->header, he, 0);
/*@=nullpass@*/
    }

    /* Add dependency dictionary(#dependencies) */
    he->tag = RPMTAG_DEPENDSDICT;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = argiData(fc->ddictx);
    he->c = argiCount(fc->ddictx);
    if (he->p.ptr != NULL) {
	xx = headerPut(pkg->header, he, 0);
    }

    /* Add per-file dependency (start,number) pairs (#files) */
    he->tag = RPMTAG_FILEDEPENDSX;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = argiData(fc->fddictx);
    he->c = argiCount(fc->fddictx);
assert(ac == (int)he->c);
    if (he->p.ptr != NULL) {
	xx = headerPut(pkg->header, he, 0);
    }

    he->tag = RPMTAG_FILEDEPENDSN;
    he->t = RPM_UINT32_TYPE;
    he->p.ui32p = argiData(fc->fddictn);
    he->c = argiCount(fc->fddictn);
assert(ac == (int)he->c);
    if (he->p.ptr != NULL) {
	xx = headerPut(pkg->header, he, 0);
    }

    printDeps(pkg->header);

if (fc != NULL && _rpmfc_debug) {
char msg[BUFSIZ];
sprintf(msg, "final: files %u cdict[%d] %u%% ddictx[%d]", (unsigned int)fc->nfiles, argvCount(fc->cdict), (unsigned int)((100 * fc->fknown)/fc->nfiles), argiCount(fc->ddictx));
rpmfcPrint(msg, fc, NULL);
}

    /* Clean up. */
    fmode = _free(fmode);
    fc = rpmfcFree(fc);
    av = argvFree(av);

    return rc;
}

/*@-mustmod@*/
static void rpmfcFini(void * _fc)
	/*@modifies _fc @*/
{
    rpmfc fc = (rpmfc) _fc;

    fc->fn = argvFree(fc->fn);
    fc->fcolor = argiFree(fc->fcolor);
    fc->fcdictx = argiFree(fc->fcdictx);
    fc->fddictx = argiFree(fc->fddictx);
    fc->fddictn = argiFree(fc->fddictn);
    fc->cdict = argvFree(fc->cdict);
    fc->ddict = argvFree(fc->ddict);
    fc->ddictx = argiFree(fc->ddictx);

    (void)rpmdsFree(fc->provides);
    fc->provides = NULL;
    (void)rpmdsFree(fc->requires);
    fc->requires = NULL;

    fc->iob_java = rpmiobFree(fc->iob_java);
    fc->iob_perl = rpmiobFree(fc->iob_perl);
    fc->iob_python = rpmiobFree(fc->iob_python);
    fc->iob_php = rpmiobFree(fc->iob_php);
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmfcPool = NULL;

static rpmfc rpmfcGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmfcPool, fileSystem, internalState @*/
	/*@modifies pool, _rpmfcPool, fileSystem, internalState @*/
{
    rpmfc fc;

    if (_rpmfcPool == NULL) {
	_rpmfcPool = rpmioNewPool("fc", sizeof(*fc), -1, _rpmfc_debug,
			NULL, NULL, rpmfcFini);
	pool = _rpmfcPool;
    }
    fc = (rpmfc) rpmioGetPool(pool, sizeof(*fc));
    memset(((char *)fc)+sizeof(fc->_item), 0, sizeof(*fc)-sizeof(fc->_item));
    return fc;
}

rpmfc rpmfcNew(void)
{
    rpmfc fc = rpmfcGetPool(_rpmfcPool);
    fc->fn = (ARGV_t) xcalloc(1, sizeof(*fc->fn));
    return rpmfcLink(fc);
}

