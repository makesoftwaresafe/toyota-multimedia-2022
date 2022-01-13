#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <poptIO.h>

#define	_RPMJS_INTERNAL
#include <rpmjs.h>

#include "gpsee.h"

#define xstr(s) str(s)
#define str(s) #s

#include "debug.h"

extern const char * __progname;

/*==============================================================*/

#define F_ISSET(_flags, _FLAG) ((_flags) & RPMJS_FLAGS_##_FLAG)

static const char * Icode;	/* String with JavaScript program in it */
static const char * Ifn;	/* Filename with JavaScript program in it */
static int verbosity;		/* 0 = no debug, bigger = more debug */
static int gcZeal;

/*==============================================================*/

/** GPSEE uses panic() to panic, expects embedder to provide */
void __attribute__ ((noreturn)) panic(const char *message)
{
    rpmlog(RPMLOG_EMERG, "Fatal Error: %s\n", message);
    exit(EXIT_FAILURE);
}

/*==============================================================*/
static void rpmjsArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, /*@unused@*/ const char * arg,
                /*@unused@*/ void * data)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmjs js = &_rpmjs;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'F':
	js->flags |= RPMJS_FLAGS_SKIPSHEBANG;
	Ifn = xstrdup(arg);
	break;
    case 'z':
        gcZeal++;
        break;
    case 'd':
        rpmIncreaseVerbosity();
        break;
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	exit(EXIT_FAILURE);
	/*@notreached@*/ break;
    }
}

static struct poptOption _gsrOptionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmjsArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, 'c', POPT_ARG_STRING,      &Icode, 0,
        N_("Specifies literal JavaScript code to execute"), N_("code") },
  { NULL, 'f', POPT_ARG_STRING,      &Ifn, 0,
        N_("Specifies the filename containing code to run"), N_("filename") },
  { NULL, 'F', POPT_ARG_STRING,      NULL, 'F',
        N_("Like -f, but skip shebang if present"), N_("filename") },
  { NULL, 'h', POPT_ARG_NONE,      NULL, 'h',
        N_("Display this help"), NULL },
  { "noexec", 'n', POPT_BIT_SET,	&_rpmjs.flags, RPMJS_FLAGS_NOEXEC,
	N_("Engine will load and parse, but not run, the script"), NULL },
  { NULL, 'z', 0, NULL, (int)'z',
	N_("Increase GC Zealousness"), NULL},
  { NULL, 'd', 0, NULL, (int)'d',
	N_("Provide more detailed output"), NULL},

  POPT_TABLEEND
};

extern struct poptOption rpmjsIPoptTable[];	/* XXX in rpmjs.h? */

static struct poptOption _optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmjsArgCallback, 0, NULL, NULL },
/*@=type@*/

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, _gsrOptionsTable, 0,
        N_("Command options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmjsIPoptTable, 0,
        N_("JS interpreter options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

static struct poptOption *optionsTable = &_optionsTable[0];
/*==============================================================*/
static int _gsrExitCode(rpmjs js)
{
    gpsee_interpreter_t *I = js->I;
    const char *reason = NULL;
    int ec = 1;

    if (I->grt->exitType & et_successMask) {
	ec = I->grt->exitCode;
	goto exit;
    }

    switch (I->grt->exitType) {
    case et_successMask:
    case et_errorMask:
    case et_requested:
    case et_finished:
    case et_dummy:
    default:
	reason = N_("impossible");
	GPSEE_NOT_REACHED(reason);
	break;
    case et_execFailure:
	reason = N_("execFailure - probable native module error, returning JS_FALSE without setting exception");
	break;
    case et_compileFailure:
	reason = N_("script could not be compiled");
	break;
    case et_unknown:
	reason = N_("unknown - probable native module error, returning JS_FALSE without setting exception");
	break;
    case et_exception:
	reason = NULL;
	goto exit;
	/*@notreached@*/ break;
    }

    rpmlog(RPMLOG_NOTICE, _("Unexpected interpreter shutdown: %s(%m)"), reason);

exit:
    return ec;
}

static int _main(int argc, char *argv[])
{
    poptContext optCon;
    rpmjs js = NULL;
    char *const * Iargv = NULL;	/* Becomes arguments array in JS program */
    const char * result = NULL;
    int ac = 0;
    int ec = 1;		/* assume failure */
rpmRC ret;

    void *stackBasePtr = NULL;
    gpsee_interpreter_t * I = NULL;
#ifdef GPSEE_DEBUGGER
JSDContext *jsdc;
#endif

    _rpmjs.flags = _rpmjs_options;	/* XXX necessary? */

    gpsee_setVerbosity(isatty(STDERR_FILENO) ? GSR_PREPROGRAM_TTY_VERBOSITY
		       : GSR_PREPROGRAM_NOTTY_VERBOSITY);

#ifdef	NOTYET	/* XXX original gsr is solaris -> *BSDish */
    _rpmio_popt_context_flags = POPT_CONTEXT_POSIXMEHARDER;
#endif
    optCon = rpmioInit(argc, argv, optionsTable);

    /* Add files from CLI. */
    Iargv = (char *const *)poptGetArgs(optCon);
    ac = argvCount((ARGV_t)Iargv);

    switch (ac) {
    case 0:
	if (Icode || Ifn)
 	    break;
	/*@fallthrough@*/
    default:
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
	/*@notreached@*/ break;
    }

    if (F_ISSET(_rpmjs.flags, NOUTF8) || getenv("GPSEE_NO_UTF8_C_STRINGS")) {
	JS_DestroyRuntime(JS_NewRuntime(1024));
	putenv((char *) "GPSEE_NO_UTF8_C_STRINGS=1");
    }

    js = rpmjsNew((char **)Iargv, _rpmjs.flags);
    I = js->I;

#if defined(GPSEE_DEBUGGER)
    jsdc = gpsee_initDebugger(I->cx, I->realm, DEBUGGER_JS);
#endif
    /* XXX set from macro? */
    gpsee_setThreadStackLimit(I->cx, &stackBasePtr, strtol("0x80000", NULL, 0));

    if (verbosity < GSR_MIN_TTY_VERBOSITY && isatty(STDERR_FILENO))
	verbosity = GSR_MIN_TTY_VERBOSITY;

    if (gpsee_verbosity(0) < verbosity)
	gpsee_setVerbosity(verbosity);

    /* XXX set from macro? */
#ifdef	REFERENCE
#ifdef JSFEATURE_GC_ZEAL
    if (JS_HasFeature(JSFEATURE_GC_ZEAL) == JS_TRUE)
	JS_SetGCZeal(I->cx, gcZeal);
#else
# ifdef JS_GC_ZEAL
    JS_SetGCZeal(I->cx, gcZeal);
# else
#  warning JS_SetGCZeal not available when building with this version of SpiderMonkey (try a debug build?)
# endif
#endif
#endif	/* REFERENCE */

    /* Run JavaScript specified with -c */
    if (Icode) {
	const char * result = NULL;
	rpmRC rc = rpmjsRun(js, Icode, &result);
	ec = (rc == RPMRC_OK ? 0 : 1);
	goto finish;
    }

    /* Pre-compile JavaScript specified with -f */
#if !defined(SYSTEM_GSR)
#define	SYSTEM_GSR	"/usr/bin/gsr"
#endif
    if (argv[0][0] == '/' && strcmp(argv[0], SYSTEM_GSR)) {
	const char * preloadfn = rpmGetPath(dirname(argv[0]),
				"/.", basename(argv[0]), "_preload", NULL);

	/* XXX assert? */
	if (!(preloadfn && *preloadfn)) {
	    rpmlog(RPMLOG_EMERG,
		"%s: Unable to create preload script filename!\n", __progname);
	    preloadfn = _free(preloadfn);
	    goto finish;
	}

	errno = 0;
	if (Access(preloadfn, F_OK) == 0) {
	    jsval v;
	    JSScript *script;
	    JSObject *scrobj;

	    if (!gpsee_compileScript(I->cx, preloadfn,
			NULL, NULL, &script, I->realm->globalObject, &scrobj))
	    {
		I->grt->exitType = et_compileFailure;
		rpmlog(RPMLOG_EMERG,
			  "%s: Unable to compile preload script '%s'\n",
			  __progname, preloadfn);
		preloadfn = _free(preloadfn);
		goto finish;
	    }
	    preloadfn = _free(preloadfn);

	    if (!script || !scrobj)
		goto finish;

	    if (!F_ISSET(js->flags, NOEXEC)) {
		JS_AddNamedObjectRoot(I->cx, &scrobj, "preload_scrobj");
		I->grt->exitType = et_execFailure;
		if (JS_ExecuteScript(I->cx, I->realm->globalObject, script, &v) == JS_FALSE)
		    I->grt->exitType = et_finished;
		else {
		    if (JS_IsExceptionPending(I->cx))
			I->grt->exitType = et_exception;
		    JS_ReportPendingException(I->cx);
		}
		JS_RemoveObjectRoot(I->cx, &scrobj);
		v = JSVAL_NULL;
	    }
	}

	if (I->grt->exitType & et_exception)
	    goto finish;
    }

    /* Setup for main-script running -- cancel preprogram verbosity and use our own error reporting system in gsr that does not use error reporter */
    gpsee_setVerbosity(verbosity);
    JS_SetOptions(I->cx, JS_GetOptions(I->cx) | JSOPTION_DONT_REPORT_UNCAUGHT);

    if (Ifn == NULL) {
	if (Icode == NULL)
	    goto finish;	/* XXX gsr does usage() here */

	if (I->grt->exitType == et_unknown)
	    I->grt->exitType = et_finished;
	goto finish;
    }

    /* Run (pre-compiled) JavaScript specified with -f */
    if (ac != 0)	/* XXX FIXME */
	js->flags |= RPMJS_FLAGS_SKIPSHEBANG;

    ret = rpmjsRunFile(js, Ifn, Iargv, &result);
    switch (ret) {
    default:
    {	int code = gpsee_getExceptionExitCode(I->cx);
	if (code >= 0) {	/** e.g. throw 6 */
assert(I->grt->exitType == et_requested);
assert(I->grt->exitCode == code);
	    I->grt->exitType = et_requested;
	    I->grt->exitCode = code;
	} else
	if (JS_IsExceptionPending(I->cx)) {
assert(I->grt->exitType == et_exception);
	    I->grt->exitType = et_exception;
	    gpsee_reportUncaughtException(I->cx, JSVAL_NULL,
                                        (gpsee_verbosity(0) >= GSR_FORCE_STACK_DUMP_VERBOSITY) ||
                                        ((gpsee_verbosity(0) >= GPSEE_ERROR_OUTPUT_VERBOSITY) && isatty(STDERR_FILENO)));
	} else {
assert(I->grt->exitType == et_execFailure);
	    I->grt->exitType = et_execFailure;
	    rpmlog(RPMLOG_NOTICE, "%s: Unable to open' script '%s'! (%s)\n",
		      __progname, Ifn, result);
	}
	result = _free(result);
    }	break;
    case RPMRC_OK:
#ifdef	NOTYET		/* XXX FIXME */
assert(I->grt->exitType == et_finished);
#endif
	I->grt->exitType = et_finished;
	/* XXX print result? */
	break;
    }

finish:
#ifdef GPSEE_DEBUGGER
    gpsee_finiDebugger(jsdc);
#endif

    /* XXX Map the exit code identically to gsr. */
    ec = _gsrExitCode(js);

exit:
    js = rpmjsFree(js);

    Icode = _free(Icode);
    Ifn = _free(Ifn);

    optCon = rpmioFini(optCon);

    return ec;
}

int main(int argc, char *argv[])
{
  int ec = PR_Initialize(_main, argc, argv, 0);
  (void) PR_Cleanup();
  return ec;
}

#ifdef GPSEE_DEBUGGER
/**
 *  Entry point for native-debugger debugging. Set this function as a break point
 *  in your native-language debugger debugging gsrdb, and you can set breakpoints
 *  on JSNative (and JSFastNative) functions from the gsrdb user interface.
 */
extern void __attribute__ ((noinline)) gpsee_breakpoint(void)
{
    return;
}
#endif


