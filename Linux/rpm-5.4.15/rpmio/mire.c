/** \ingroup rpmds
 * \file lib/rpmds.c
 */
#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>

#define	_MIRE_INTERNAL
#include <mire.h>

#include "debug.h"

/*@access regex_t @*/

/*@unchecked@*/
int _mire_debug = 0;

/*@unchecked@*/
const unsigned char * _mirePCREtables = NULL;

/*@unchecked@*/
mireEL_t _mireEL = EL_LF;

/*@unchecked@*/
int _mireSTRINGoptions = 0;

/*@unchecked@*/
int _mireGLOBoptions = FNM_EXTMATCH | FNM_PATHNAME | FNM_PERIOD;

/*@unchecked@*/
int _mireREGEXoptions = REG_EXTENDED | REG_NEWLINE;

/*@unchecked@*/
int _mirePCREoptions = 0;

int mireClean(miRE mire)
{
    if (mire == NULL) return 0;
    mire->pattern = _free(mire->pattern);
    if (mire->mode == RPMMIRE_REGEX) {
	if (mire->preg != NULL) {
	    regfree(mire->preg);
	    /*@+voidabstract -usereleased @*/ /* LCL: regfree has bogus only */
	    if (mire->preg) free(mire->preg);
	    mire->preg = NULL;
	    /*@=voidabstract =usereleased @*/
	}
    }
    if (mire->mode == RPMMIRE_PCRE) {	/* TODO: (*pcre_free)(_p) override */
	mire->pcre = _free(mire->pcre);
	mire->hints = _free(mire->hints);
    }
    mire->errmsg = NULL;
    mire->erroff = 0;
    mire->errcode = 0;
    mire->fnflags = 0;
    mire->cflags = 0;
    mire->eflags = 0;
    mire->coptions = 0;
    mire->eoptions = 0;
    mire->notmatch = 0;
/*@-modfilesys@*/
if (_mire_debug)
fprintf(stderr, "<-- mireClean(%p)\n", mire);
/*@=modfilesys@*/
    return 0;
}

static void mireFini(void * _mire)
	/*@modifies _mire @*/
{
    miRE mire = (miRE) _mire;
    (void) mireClean(mire);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _mirePool;

miRE mireGetPool(rpmioPool pool)
{
    miRE mire;

    if (_mirePool == NULL) {
	_mirePool = rpmioNewPool("mire", sizeof(*mire), -1, _mire_debug,
			NULL, NULL, mireFini);
	pool = _mirePool;
    }
    mire = (miRE) rpmioGetPool(pool, sizeof(*mire));
    memset(((char *)mire)+sizeof(mire->_item), 0, sizeof(*mire)-sizeof(mire->_item));
    return mire;
}

/*@-onlytrans@*/	/* XXX miRE array, not refcounted. */
void * mireFreeAll(miRE mire, int nmire)
{
    if (mire != NULL) {
	while (--nmire > 0)
	    (void) mireClean(mire + nmire);
	/* XXX rpmgrep doesn't use mire pools yet. retrofit a fix. */
	if (mire->_item.use != NULL && mire->_item.pool != NULL)
	    mire = (miRE)rpmioFreePoolItem((rpmioItem)mire, __FUNCTION__, __FILE__, __LINE__);
	else {
	    if (mire) free(mire);
	    mire = NULL;
	}
    }
    return NULL;
}
/*@=onlytrans@*/

miRE mireNew(rpmMireMode mode, int tag)
{
    miRE mire = mireGetPool(_mirePool);
    mire->mode = mode;
    mire->tag = tag;
    return mireLink(mire);
}

int mireSetCOptions(miRE mire, rpmMireMode mode, int tag, int options,
		const unsigned char * table)
{
    int rc = 0;
    mire->mode = mode;
    mire->tag = tag;
    switch (mire->mode) {
    case RPMMIRE_DEFAULT:
	break;
    case RPMMIRE_STRCMP:
	/* XXX strcasecmp? */
	break;
    case RPMMIRE_GLOB:
	if (options == 0)
	    options = _mireGLOBoptions;
	mire->fnflags = options;
	break;
    case RPMMIRE_REGEX:
	if (options == 0)
	    options = _mireREGEXoptions;
	mire->cflags = options;
	break;
    case RPMMIRE_PCRE:
	if (options == 0)
	    options = _mirePCREoptions;
	/* XXX check default compile options? */
	mire->coptions = options;
/*@-assignexpose -temptrans @*/
	mire->table = table;
/*@=assignexpose =temptrans @*/
	break;
    }
    return rc;
}

int mireSetEOptions(miRE mire, int * offsets, int noffsets)
{
    int rc = 0;
    if (mire->mode == RPMMIRE_PCRE) {
	mire->startoff = 0;
	mire->eoptions = 0;
/*@-assignexpose@*/
	mire->offsets = offsets;
/*@=assignexpose@*/
	mire->noffsets = noffsets;
    } else
    if (mire->mode == RPMMIRE_REGEX) {
	mire->startoff = 0;
	mire->eoptions = 0;
/*@-assignexpose@*/
	mire->offsets = offsets;
/*@=assignexpose@*/
	mire->noffsets = noffsets;
    } else
	rc = -1;

    return rc;
}

int mireSetGOptions(const char * newline, int caseless, int multiline, int utf8)
{
    int rc = 0;

    if (caseless) {
#if defined(PCRE_CASELESS)
	_mirePCREoptions |= PCRE_CASELESS;
#endif
	_mireREGEXoptions |= REG_ICASE;
#if defined(FNM_CASEFOLD)
	_mireGLOBoptions |= FNM_CASEFOLD;
#endif
    } else {
#if defined(PCRE_CASELESS)
	_mirePCREoptions &= ~PCRE_CASELESS;
#endif
	_mireREGEXoptions &= ~REG_ICASE;
#if defined(FNM_CASEFOLD)
	_mireGLOBoptions &= ~FNM_CASEFOLD;
#endif
    }

    if (multiline) {
#if defined(PCRE_MULTILINE)
	_mirePCREoptions |= PCRE_MULTILINE|PCRE_FIRSTLINE;
#endif
    } else {
#if defined(PCRE_MULTILINE)
	_mirePCREoptions &= ~(PCRE_MULTILINE|PCRE_FIRSTLINE);
#endif
    }

    if (utf8) {
#if defined(PCRE_UTF8)
	_mirePCREoptions |= PCRE_UTF8;
#endif
    } else {
#if defined(PCRE_UTF8)
	_mirePCREoptions &= ~PCRE_UTF8;
#endif
    }

    /*
     * Set the default line ending value from the default in the PCRE library;
     * "lf", "cr", "crlf", and "any" are supported. Anything else is treated
     * as "lf".
     */
    if (newline == NULL) {
	int val = 0;
#if defined(PCRE_CONFIG_NEWLINE)
/*@-modunconnomods@*/
	(void)pcre_config(PCRE_CONFIG_NEWLINE, &val);
/*@=modunconnomods@*/
#endif
	switch (val) {
	default:	newline = "lf";		break;
	case '\r':	newline = "cr";		break;
/*@-shiftimplementation@*/
	case ('\r' << 8) | '\n': newline = "crlf"; break;
/*@=shiftimplementation@*/
	case -1:	newline = "any";	break;
	case -2:	newline = "anycrlf";	break;
	}
    }

    /* Interpret the newline type; the default settings are Unix-like. */
    if (!strcasecmp(newline, "cr")) {
#if defined(PCRE_NEWLINE_CR)
	_mirePCREoptions |= PCRE_NEWLINE_CR;
#endif
	_mireEL = EL_CR;
    } else if (!strcasecmp(newline, "lf")) {
#if defined(PCRE_NEWLINE_LF)
	_mirePCREoptions |= PCRE_NEWLINE_LF;
#endif
	_mireEL = EL_LF;
    } else if (!strcasecmp(newline, "crlf")) {
#if defined(PCRE_NEWLINE_CRLF)
	_mirePCREoptions |= PCRE_NEWLINE_CRLF;
#endif
	_mireEL = EL_CRLF;
    } else if (!strcasecmp(newline, "any")) {
#if defined(PCRE_NEWLINE_ANY)
	_mirePCREoptions |= PCRE_NEWLINE_ANY;
#endif
	_mireEL = EL_ANY;
    } else if (!strcasecmp(newline, "anycrlf")) {
#if defined(PCRE_NEWLINE_ANYCRLF)
	_mirePCREoptions |= PCRE_NEWLINE_ANYCRLF;
#endif
	_mireEL = EL_ANYCRLF;
    } else {
	rc = -1;
    }

    return rc;
}

int mireSetLocale(/*@unused@*/ miRE mire, const char * locale)
{
    const char * locale_from = NULL;
    int rc = -1;	/* assume failure */

    /* XXX TODO: --locale jiggery-pokery should be done env LC_ALL=C rpmgrep */
    if (locale != NULL)
	locale_from = "--locale";
    else {
	/*
	 * If a locale has not been provided as an option, see if the
	 * LC_CTYPE or LC_ALL environment variable is set, and if so,
	 * use it.
	 */
/*@-dependenttrans -observertrans@*/
	if ((locale = getenv("LC_ALL")) != NULL)
	    locale_from = "LC_ALL";
	else if ((locale = getenv("LC_CTYPE")) != NULL)
	    locale_from = "LC_CTYPE";
	else {
	    locale_from = "glibc";
	    locale = "";
	}
/*@=dependenttrans =observertrans@*/
    }

    /*
    * If a locale has been provided, set it, and generate the tables PCRE
    * needs. Otherwise, _mirePCREtables == NULL, which uses default tables.
    */
    if (locale != NULL) {
	const char * olocale = setlocale(LC_CTYPE, locale);
	if (olocale == NULL) {
/*@-modfilesys@*/
 	    fprintf(stderr,
		_("%s: Failed to set locale %s (obtained from %s)\n"),
		__progname, locale, locale_from);
/*@=modfilesys@*/
	    goto exit;
	}
#if defined(WITH_PCRE)
/*@-evalorderuncon -onlytrans @*/
	_mirePCREtables = pcre_maketables();
/*@=evalorderuncon =onlytrans @*/
#endif
	if (setlocale(LC_CTYPE, olocale) == NULL)
	    goto exit;
    }
    rc = 0;

exit:
    return rc;
}

int mireRegcomp(miRE mire, const char * pattern)
{
    int rc = 0;

    mire->pattern = xstrdup(pattern);

    switch (mire->mode) {
    case RPMMIRE_STRCMP:
	break;
    case RPMMIRE_PCRE:
#ifdef	WITH_PCRE
	mire->errcode = 0;
	mire->errmsg = NULL;
	mire->erroff = 0;
	mire->pcre = pcre_compile2(mire->pattern, mire->coptions,
		&mire->errcode, &mire->errmsg, &mire->erroff, mire->table);
	if (mire->pcre == NULL) {
	    if (_mire_debug)
		rpmlog(RPMLOG_ERR,
		    _("pcre_compile2 failed: %s(%d) at offset %d of \"%s\"\n"),
		    mire->errmsg, mire->errcode, mire->erroff, mire->pattern);
	    rc = -1;
	    goto exit;	/* XXX HACK: rpmgrep is not expecting mireClean. */
	}
#else
	rc = -99;
#endif
	break;
    case RPMMIRE_DEFAULT:
    case RPMMIRE_REGEX:
	mire->preg = (regex_t *) xcalloc(1, sizeof(*mire->preg));
	if (mire->cflags == 0)
	    mire->cflags = _mireREGEXoptions;
	rc = regcomp(mire->preg, mire->pattern, mire->cflags);
	if (rc) {
	    char msg[256];
	    (void) regerror(rc, mire->preg, msg, sizeof(msg)-1);
	    msg[sizeof(msg)-1] = '\0';
	    rpmlog(RPMLOG_ERR, _("%s: regcomp failed: %s\n"),
			mire->pattern, msg);
	}
	break;
    case RPMMIRE_GLOB:
	if (mire->fnflags == 0)
	    mire->fnflags = _mireGLOBoptions;
	break;
    default:
	rc = -1;
	break;
    }

    if (rc)
	(void) mireClean(mire);

#ifdef	WITH_PCRE
exit:
#endif
/*@-modfilesys@*/
if (_mire_debug)
fprintf(stderr, "<-- mireRegcomp(%p, \"%s\") rc %d\n", mire, pattern, rc);
/*@=modfilesys@*/
    return rc;
}

int mireRegexec(miRE mire, const char * val, size_t vallen)
{
    int rc = -1;	/* assume failure */

    switch (mire->mode) {
    case RPMMIRE_STRCMP:
	if (mire->pattern == NULL)
	    break;
	if (vallen == 0)
	    vallen = strlen(val);
	/* XXX strcasecmp? */
	rc = strncmp(mire->pattern, val, vallen);
	if (rc) rc = -1;
	break;
    case RPMMIRE_DEFAULT:
    case RPMMIRE_REGEX:
	if (mire->preg == NULL)
	    break;
	/* XXX rpmgrep: ensure that the string is NUL terminated. */
	if (vallen > 0) {
	    if (val[vallen] != '\0') {
		char * t = strncpy((char *)alloca(vallen+1), val, vallen);
		t[vallen] = '\0';
		val = t;
	    }
	} else
	if (vallen == 0)
	    vallen = strlen(val);
/*@-nullpass@*/
	/* XXX HACK: PCRE returns 2/3 of array, POSIX dimensions regmatch_t. */
	rc = regexec(mire->preg, val,
		mire->noffsets/3, (regmatch_t *)mire->offsets, mire->eflags);
/*@=nullpass@*/
	switch (rc) {
	case 0:			rc = 0;	/*@innerbreak@*/ break;
	case REG_NOMATCH:	rc = -1;/*@innerbreak@*/ break;
	default:
	  { char msg[256];
	    (void) regerror(rc, mire->preg, msg, sizeof(msg)-1);
	    msg[sizeof(msg)-1] = '\0';
	    rpmlog(RPMLOG_ERR, _("%s: regexec failed: %s(%d)\n"),
			mire->pattern, msg, rc);
	    if (rc < 0) rc -= 1;	/* XXX ensure -1 is nomatch. */
	    if (rc > 0)	rc = -(rc+1);	/* XXX ensure errors are negative. */
	  } /*@innerbreak@*/ break;
	}
	break;
    case RPMMIRE_PCRE:
#ifdef	WITH_PCRE
	if (mire->pcre == NULL)
	    break;
	if (vallen == 0)
	    vallen = strlen(val);
	rc = pcre_exec((pcre *)mire->pcre, (pcre_extra *)mire->hints,
		val, (int)vallen, mire->startoff,
		mire->eoptions, mire->offsets, mire->noffsets);
	switch (rc) {
	case 0:				rc = 0;	/*@innerbreak@*/ break;
	case PCRE_ERROR_NOMATCH:	rc = -1;/*@innerbreak@*/ break;
	default:
	    if (_mire_debug && rc < 0)
		rpmlog(RPMLOG_ERR, _("pcre_exec failed: return %d\n"), rc);
	    /*@innerbreak@*/ break;
	}
#else
	rc = -99;
#endif
	break;
    case RPMMIRE_GLOB:
	if (mire->pattern == NULL)
	    break;
	rc = fnmatch(mire->pattern, val, mire->fnflags);
	switch (rc) {
	case 0:			rc = 0;	/*@innerbreak@*/ break;
	case FNM_NOMATCH:	rc = -1;/*@innerbreak@*/ break;
	default:
	    if (_mire_debug)
		rpmlog(RPMLOG_ERR, _("fnmatch failed: return %d\n"), rc);
	    if (rc < 0) rc -= 1;	/* XXX ensure -1 is nomatch. */
	    if (rc > 0)	rc = -(rc+1);	/* XXX ensure errors are negative. */
	    /*@innerbreak@*/ break;
	}
	break;
    default:
	break;
    }

/*@-modfilesys@*/
if (_mire_debug)
fprintf(stderr, "<-- mireRegexec(%p, %p[%u]) rc %d mode %d \"%.*s\"\n", mire, val, (unsigned)vallen, rc, mire->mode, (int)(vallen < 20 ? vallen : 20), val);
/*@=modfilesys@*/
    return rc;
}

/*@-onlytrans@*/	/* XXX miRE array, not refcounted. */
int mireAppend(rpmMireMode mode, int tag, const char * pattern,
		const unsigned char * table, miRE * mirep, int * nmirep)
{
    miRE mire;
    int xx;

    if (*mirep == NULL) {
	(*mirep) = mireGetPool(_mirePool);
	mire = (*mirep);
    } else {
	yarnLock use =  (*mirep)->_item.use;
	void *pool = (*mirep)->_item.pool;

	/* XXX only the 1st element in the array has a usage mutex. */
	(*mirep) = (miRE) xrealloc((*mirep), ((*nmirep) + 1) * sizeof(*mire));
	mire = (*mirep) + (*nmirep);
	memset(mire, 0, sizeof(*mire));
        /* XXX ensure no segfault, copy the use/pool from 1st item. */
/*@-assignexpose@*/
        mire->_item.use = use;
        mire->_item.pool = pool;
/*@=assignexpose@*/
    }

    (*nmirep)++;
    xx = mireSetCOptions(mire, mode, tag, 0, table);
/*@-usereleased@*/
    return mireRegcomp(mire, pattern);
/*@=usereleased@*/
}
/*@=onlytrans@*/

int mireLoadPatterns(rpmMireMode mode, int tag, const char ** patterns,
		const unsigned char * table, miRE * mirep, int * nmirep)
{
    const char *pattern;
    int rc = -1;	/* assume failure */

    if (patterns != NULL)	/* note rc=0 return with no patterns to load. */
    while ((pattern = *patterns++) != NULL) {
	/* XXX pcre_options is not used. should it be? */
	/* XXX more realloc's than necessary. */
	int xx = mireAppend(mode, tag, pattern, table, mirep, nmirep);
	if (xx) {
	    rc = xx;
	    goto exit;
	}
    }
    rc = 0;

exit:
    return rc;
}

int mireApply(miRE mire, int nmire, const char *s, size_t slen, int rc)
{
    int i;

    if (slen == 0)
	slen = strlen(s);

    if (mire)
    for (i = 0; i < nmire; mire++, i++) {
	int xx = mireRegexec(mire, s, slen);

	/* Check if excluding or including condition applies. */
	if (rc < 0 && xx < 0)
	    continue;	/* excluding: continue on negative matches. */
	if (rc > 0 && xx >= 0)
	    continue;	/* including: continue on positive matches. */
	/* Save 1st found termination condition and exit. */
	rc = xx;
	break;
    }
    return rc;
}

int mireStudy(miRE mire, int nmires)
{
    int rc = -1;	/* assume failure */
    int j;

    /* Study the PCRE regex's, as we will be running them many times */
    if (mire)		/* note rc=0 return with no mire's. */
    for (j = 0; j < nmires; mire++, j++) {
	if (mire->mode != RPMMIRE_PCRE)
	    continue;
#if defined(WITH_PCRE)
      {	const char * error;
	mire->hints = pcre_study((pcre *)mire->pcre, 0, &error);
	if (error != NULL) {
	    char s[32];
	    if (nmires == 1) s[0] = '\0'; else sprintf(s, _(" number %d"), j);
	    rpmlog(RPMLOG_ERR, _("%s: Error while studying regex%s: %s\n"),
		__progname, s, error);
	    goto exit;
	}
      }
#endif
    }
    rc = 0;

#if defined(WITH_PCRE)
exit:
#endif
    return rc;
}
