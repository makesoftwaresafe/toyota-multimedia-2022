/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is PageMail, Inc.
 *
 * Portions created by the Initial Developer are 
 * Copyright (c) 2007-2009, PageMail, Inc. All Rights Reserved.
 *
 * Contributor(s): 
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** 
 */

#include "system.h"

#include "rpmio_internal.h"
#include <argv.h>
#include <popt.h>

#if defined(__APPLE__)
#include <crt_externs.h>
#else
extern char ** environ;
#endif

#if defined(WITH_GPSEE)

#define JS_THREADSAFE   1       /* XXX FIXME: current Fedora package broken. */
#define	XP_UNIX	1		/* XXX needed? */
#undef	DOXYGEN
#include <gpsee.h>

typedef	gpsee_interpreter_t * JSI_t;
#define	_RPMJS_OPTIONS	\
    (JSOPTION_STRICT | JSOPTION_RELIMIT | JSOPTION_ANONFUNFIX | JSOPTION_JIT)

#else	/* WITH_GPSEE */

typedef void * JSI_t;
#define	_RPMJS_OPTIONS	0

#endif	/* WITH_GPSEE */

#define _RPMJS_INTERNAL
#include "rpmjs.h"

#include "debug.h"

#define F_ISSET(_flags, _FLAG) ((_flags) & RPMJS_FLAGS_##_FLAG)

/*@unchecked@*/
int _rpmjs_debug = 0;

#define RPMJSDBG(_t, _l) \
  if ((_t) || _rpmjs_debug) fprintf _l

/*@unchecked@*/ /*@relnull@*/
rpmjs _rpmjsI = NULL;

/*@unchecked@*/
uint32_t _rpmjs_options = _RPMJS_OPTIONS;

/*@unchecked@*/
int _rpmjs_zeal = 2;

struct rpmjs_s _rpmjs;

struct poptOption rpmjsIPoptTable[] = {
  { "allow", 'a', POPT_BIT_SET,		&_rpmjs.flags, RPMJS_FLAGS_ALLOW,
        N_("Allow (read-only) access to caller's environmen"), NULL },
  { "nocache", 'C', POPT_BIT_SET,	&_rpmjs.flags, RPMJS_FLAGS_NOCACHE,
        N_("Disables compiler caching via JSScript XDR serialization"), NULL },
  { "loadrc", 'R', POPT_BIT_SET,	&_rpmjs.flags, RPMJS_FLAGS_LOADRC,
        N_("Load RC file for interpreter based on script filename."), NULL },
  { "nowarn", 'W', POPT_BIT_SET,	&_rpmjs.flags, RPMJS_FLAGS_NOWARN,
        N_("Do not report warnings"), NULL },

  { "norelimit", 'e', POPT_BIT_CLR,	&_rpmjs.flags, RPMJS_FLAGS_RELIMIT,
        N_("Do not limit regexps to n^3 levels of backtracking"), NULL },
  { "nojit", 'J', POPT_BIT_CLR,		&_rpmjs.flags, RPMJS_FLAGS_JIT,
        N_("Disable nanojit"), NULL },
  { "nostrict", 'S', POPT_BIT_CLR,	&_rpmjs.flags, RPMJS_FLAGS_STRICT,
        N_("Disable Strict mode"), NULL },
  { "noutf8", 'U', POPT_BIT_SET,	&_rpmjs.flags, RPMJS_FLAGS_NOUTF8,
        N_("Disable UTF-8 C string processing"), NULL },
  { "xml", 'x', POPT_BIT_SET,		&_rpmjs.flags, RPMJS_FLAGS_XML,
        N_("Parse <!-- comments --> as E4X tokens"), NULL },

  { "anonfunfix", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmjs.flags, RPMJS_FLAGS_ANONFUNFIX,
        N_("Parse //@line number [\"filename\"] for XUL"), NULL },
  { "atline", 'A', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmjs.flags, RPMJS_FLAGS_ATLINE,
        N_("Parse //@line number [\"filename\"] for XUL"), NULL },
  { "werror", 'w', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&_rpmjs.flags, RPMJS_FLAGS_WERROR,
        N_("Convert warnings to errors"), NULL },

  POPT_TABLEEND
};

static void rpmjsFini(void * _js)
	/*@globals fileSystem @*/
	/*@modifies *_js, fileSystem @*/
{
    rpmjs js = _js;

RPMJSDBG(0, (stderr, "==> %s(%p) I %p\n", __FUNCTION__, js, js->I));

#if defined(WITH_GPSEE)
#if defined(XXX_GPSEE_DEBUGGER)
    gpsee_finiDebugger(js->jsdc);
    js->jsdc = NULL;
#endif
    (void) gpsee_destroyInterpreter(js->I);
#endif
    js->I = NULL;
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmjsPool;

static rpmjs rpmjsGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmjsPool, fileSystem @*/
	/*@modifies pool, _rpmjsPool, fileSystem @*/
{
    rpmjs js;

    if (_rpmjsPool == NULL) {
	_rpmjsPool = rpmioNewPool("js", sizeof(*js), -1, _rpmjs_debug,
			NULL, NULL, rpmjsFini);
	pool = _rpmjsPool;
    }
    return (rpmjs) rpmioGetPool(pool, sizeof(*js));
}

static rpmjs rpmjsI(void)
	/*@globals _rpmjsI @*/
	/*@modifies _rpmjsI @*/
{
    if (_rpmjsI == NULL) {
#if defined(WITH_GPSEE)
	gpsee_verbosity(0);	/* XXX hack around syslog(3) in GPSEE */
#endif
	_rpmjsI = rpmjsNew(NULL, 0);
    }
RPMJSDBG(0, (stderr, "<== %s() _rpmjsI %p\n", __FUNCTION__, _rpmjsI));
    return _rpmjsI;
}

/* XXX FIXME: Iargv/Ienviron are now associated with running. */
rpmjs rpmjsNew(char ** av, uint32_t flags)
{
    rpmjs js =
#ifdef	NOTYET
	(flags & 0x80000000) ? rpmjsI() :
#endif
	rpmjsGetPool(_rpmjsPool);
    JSI_t I = NULL;

#if defined(WITH_GPSEE)

#if defined(XXX_GPSEE_DEBUGGER)	/* XXX js->jsdc? */
    JSDContext *jsdc;
#endif

    if (flags == 0)
	flags = _rpmjs_options;

    if (F_ISSET(flags, NOUTF8) || getenv("GPSEE_NO_UTF8_C_STRINGS")) {
	JS_DestroyRuntime(JS_NewRuntime(1024));
	putenv((char *) "GPSEE_NO_UTF8_C_STRINGS=1");
    }

    /* XXX FIXME: js->Iargv/js->Ienviron for use by rpmjsRunFile() */
    I = gpsee_createInterpreter();
#if defined(XXX_GPSEE_DEBUGGER)
    js->jsdc = gpsee_initDebugger(I->cx, I->realm, DEBUGGER_JS);
#endif

#ifdef	NOTYET	/* FIXME: dig out where NOCACHE has moved. */
    if (F_ISSET(flags, NOCACHE))
	I->useCompilerCache = 0;
#endif
    if (F_ISSET(flags, NOWARN)) {
	gpsee_runtime_t * grt = JS_GetRuntimePrivate(JS_GetRuntime(I->cx));
	grt->errorReport |= er_noWarnings;
    }

    JS_SetOptions(I->cx, (flags & 0xffff));
#if defined(JS_GC_ZEAL)
    JS_SetGCZeal(I->cx, _rpmjs_zeal);
#endif
#endif	/* WITH_GPSEE */

    js->flags = flags;
    js->I = I;

    return rpmjsLink(js);
}

#if defined(WITH_GPSEE)
static FILE * rpmjsOpenFile(rpmjs js, const char * fn, const char ** msgp)
	/*@modifies js @*/
{
    FILE * fp = NULL;

    fp = fopen(fn, "r");
    if (fp == NULL || ferror(fp)) {
	if (fp) {
	    if (msgp)
		*msgp = xstrdup(strerror(errno));
	    (void) fclose(fp);
	    fp = NULL;
	} else {
	    if (msgp)	/* XXX FIXME: add __FUNCTION__ identifier? */
		*msgp = xstrdup("unknown error");
	}
	goto exit;
    }

    gpsee_flock(fileno(fp), GPSEE_LOCK_SH);

    if (F_ISSET(js->flags, SKIPSHEBANG)) {
	char buf[BUFSIZ];
	
	if (fgets(buf, sizeof(buf), fp)) {
	    if (!(buf[0] == '#' && buf[1] == '!')) {
		/* XXX FIXME: return through *msgp */
		rpmlog(RPMLOG_WARNING, "%s: %s: no \'#!\' on 1st line\n",
			__FUNCTION__, fn);
		rewind(fp);
	    } else {
#ifdef	NOTYET	/* XXX FIXME */
gpsee_interpreter_t * I = js->I;
		I->linenoOffset += 1;
#endif	/* NOTYET */
		do {	/* consume entire first line, regardless of length */
		    if (strchr(buf, '\n'))
			break;
		} while (fgets(buf, sizeof(buf), fp));
		/*
		 * Make spidermonkey think the script starts with a blank line,
		 * to keep line numbers in sync.
		 */
		ungetc('\n', fp);
	    }
	}
    }

exit:

RPMJSDBG(0, (stderr, "<== %s(%p,%s,%p) fp %p\n", __FUNCTION__, js, fn, msgp, fp));

    return fp;
}

#ifdef	NOTYET	/* XXX FIXME */
static void processInlineFlags(rpmjs js, FILE * fp, signed int *verbosity_p)
{
    char buf[256];
    off_t offset;

    offset = ftello(fp);

    while (fgets(buf, sizeof(buf), fp)) {
	char *s, *e;

	if ((buf[0] != '/') || (buf[1] != '/'))
	    break;

	for (s = buf + 2; *s == ' ' || *s == '\t'; s++);
	if (strncmp(s, "gpsee:", 6) != 0)
	    continue;

	for (s = s + 6; *s == ' ' || *s == '\t'; s++);

	for (e = s; *e; e++) {
	    switch (*e) {
	    case '\r':
	    case '\n':
	    case '\t':
	    case ' ':
		*e = '\0';
		break;
	    }
	}

	if (s[0])
	    processFlags(gsr, s, verbosity_p);
    }

    fseeko(fp, offset, SEEK_SET);
}
#endif	/* NOTYET */
#endif	/* WITH_GPSEE */

rpmRC rpmjsRunFile(rpmjs js, const char * fn,
		char *const * Iargv,
		const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

    if (js == NULL) js = rpmjsI();

    if (fn != NULL) {
#if defined(WITH_GPSEE)
	gpsee_interpreter_t * I = js->I;
	FILE * fp = rpmjsOpenFile(js, fn, resultp);

	if (fp == NULL) {
	    I->grt->exitType = et_execFailure;
	    /* XXX FIXME: strerror in *resultp */
	    goto exit;
	}

#ifdef	NOTYET	/* XXX FIXME */
	processInlineFlags(js, fp, &verbosity);
	gpsee_setVerbosity(verbosity);
#endif

	/* Just compile and exit? */
	if (F_ISSET(js->flags, NOEXEC)) {
	    JSScript *script = NULL;
	    JSObject *scrobj = NULL;

	    if (!gpsee_compileScript(I->cx, fn,
			fp, NULL, &script, I->realm->globalObject, &scrobj))
	    {
		I->grt->exitType = et_exception;
		/* XXX FIXME: isatty(3) */
		gpsee_reportUncaughtException(I->cx, JSVAL_NULL,
			(gpsee_verbosity(0) >= GSR_FORCE_STACK_DUMP_VERBOSITY)
			||
			((gpsee_verbosity(0) >= GPSEE_ERROR_OUTPUT_VERBOSITY)
				&& isatty(STDERR_FILENO)));
	    } else {
		I->grt->exitType = et_finished;
		rc = RPMRC_OK;
	    }
	} else {
	    char *const * Ienviron = NULL;

	    if (F_ISSET(js->flags, ALLOW)) {
#if defined(__APPLE__)
		Ienviron = (char *const *) _NSGetEnviron();
#else
		Ienviron = environ;
#endif
	    }

	    I->grt->exitType = et_execFailure;
	    if (gpsee_runProgramModule(I->cx, fn,
			NULL, fp, Iargv, Ienviron) == JS_FALSE)
	    {
		int code = gpsee_getExceptionExitCode(I->cx);
		if (code >= 0) {
		    I->grt->exitType = et_requested;
		    I->grt->exitCode = code;
		    /* XXX FIXME: format and return code in *resultp. */
		    /* XXX hack tp get code into rc -> ec by negating */
		    rc = -code;
		} else
		if (JS_IsExceptionPending(I->cx)) {
		    /* XXX FIXME: isatty(3) */
		    gpsee_reportUncaughtException(I->cx, JSVAL_NULL,
			(gpsee_verbosity(0) >= GSR_FORCE_STACK_DUMP_VERBOSITY)
			||
			((gpsee_verbosity(0) >= GPSEE_ERROR_OUTPUT_VERBOSITY)
				&& isatty(STDERR_FILENO)));
		}
	    } else {
		I->grt->exitType = et_finished;
		rc = RPMRC_OK;
	    }
	}
	fclose(fp);
	fp = NULL;
#endif	/* WITH_GPSEE */
    }

#if defined(WITH_GPSEE)
exit:
#endif	/* WITH_GPSEE */

RPMJSDBG(0, (stderr, "<== %s(%p,%s) rc %d |%s|\n", __FUNCTION__, js, fn, rc, (resultp ? *resultp :"")));

    return rc;
}

rpmRC rpmjsRun(rpmjs js, const char * str, const char ** resultp)
{
    rpmRC rc = RPMRC_FAIL;

    if (!(str && *str))
	goto exit;

    if (js == NULL) js = rpmjsI();

#if defined(WITH_GPSEE)
    {	gpsee_interpreter_t * I = js->I;
	jsval v = JSVAL_VOID;
	JSBool ok = JS_EvaluateScript(I->cx, I->realm->globalObject,
				str, strlen(str), __FILE__, __LINE__, &v);
	if (!ok)
	    goto exit;
	rc = RPMRC_OK;
	if (resultp && JSVAL_IS_STRING(v)) {
	    JSString * rstr = JSVAL_TO_STRING(v);
	    size_t ns = JS_GetStringEncodingLength(I->cx, rstr);
	    char * s = xmalloc(ns+1);
	    ns = JS_EncodeStringToBuffer(rstr, s, ns);
	    s[ns] = '\0';
	    *resultp = s;
	}
    }
#endif	/* WITH_GPSEE */

exit:
RPMJSDBG(0, (stderr, "<== %s(%p,%p[%u]) rc %d\n", __FUNCTION__, js, str, (unsigned)(str ? strlen(str) : 0), rc));

    return rc;
}
