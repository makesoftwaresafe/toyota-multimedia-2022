/** \ingroup rpmrc rpmio
 * \file rpmio/macro.c
 */

#include "system.h"
#include <stdarg.h>

#if !defined(isblank)
#define	isblank(_c)	((char)(_c) == ' ' || (char)(_c) == '\t')
#endif
#define	iseol(_c)	((char)(_c) == '\n' || (char)(_c) == '\r')

#define	STREQ(_t, _f, _fn)	((_fn) == (sizeof(_t)-1) && !strncmp((_t), (_f), (_fn)))

#ifdef DEBUG_MACROS
#undef	WITH_LUA	/* XXX fixme */
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <popt.h>

#define rpmlog fprintf
#define RPMLOG_ERR stderr
#define RPMLOG_WARNING stderr
#undef	_
#define	_(x)	x

#define	vmefail(_nb)		(exit(1), NULL)
#define	URL_IS_DASH		1
#define	URL_IS_PATH		2
#define	urlPath(_xr, _r)	(*(_r) = (_xr), URL_IS_PATH)
#define	xisalnum(_c)		isalnum(_c)
#define	xisalpha(_c)		isalpha(_c)
#define	xisdigit(_c)		isdigit(_c)
#define	xisspace(_c)		isspace(_c)

typedef	FILE * FD_t;
#define Fopen(_path, _fmode)	fopen(_path, "r");
#define	Ferror			ferror
#define Fstrerror(_fd)		strerror(errno)
#define	Fread			fread
#define	Fclose			fclose

#define	fdGetFILE(_fd)		(_fd)

/*@unused@*/ static inline /*@null@*/ void *
_free(/*@only@*/ /*@null@*/ const void * p)
	/*@modifies p@*/
{
    if (p != NULL)	free((void *)p);
    return NULL;
}

#else

/*@observer@*/ /*@checked@*/
const char * rpmMacrofiles = MACROFILES;

#include <rpmio_internal.h>
#include <rpmlog.h>
#include <mire.h>

#ifdef	WITH_LUA
#define	_RPMLUA_INTERNAL	/* XXX lua->printbuf access */
#include <rpmlua.h>
#endif

#define	_RPMAUG_INTERNAL	/* XXX for _rpmaugFoo globals */
#include <rpmaug.h>
#include <rpmficl.h>
#include <rpmgit.h>
#include <rpmjs.h>

#if defined(WITH_NIX)
#define _RPMNIX_INTERNAL
#include <rpmnix.h>
#endif

#include <rpmjni.h>
#include <rpmjs.h>
#include <rpmmrb.h>
#include <rpmperl.h>
#include <rpmpython.h>
#include <rpmruby.h>
#include <rpmsm.h>
#include <rpmsquirrel.h>
#include <rpmsql.h>
#include <rpmtcl.h>

#endif

#include <rpmuuid.h>

#define	_MACRO_INTERNAL
#include <rpmmacro.h>

#include "debug.h"

/*@unchecked@*/
#if defined(WITH_AUGEAS) || defined(WITH_FICL) || defined(WITH_GPSEE) || defined(WITH_NIX) || defined(WITH_PERLEMBED) || defined(WITH_PYTHONEMBED) || defined(WITH_RUBYEMBED) || defined(WITH_SQLITE) || defined(WITH_SQUIRREL) || defined(WITH_TCL)
static int _globalI = 0x80000000;
#endif

#if defined(__LCLINT__)
/*@-exportheader@*/
extern const unsigned short int **__ctype_b_loc (void) /*@*/;
/*@=exportheader@*/
#endif

/*@access FD_t @*/		/* XXX compared with NULL */
/*@access miRE @*/		/* XXX cast */
/*@access MacroContext @*/
/*@access MacroEntry@ */
/*@access rpmlua @*/
/*@access rpmtcl @*/

static struct MacroContext_s rpmGlobalMacroContext_s;
/*@-compmempass@*/
MacroContext rpmGlobalMacroContext = &rpmGlobalMacroContext_s;
/*@=compmempass@*/

static struct MacroContext_s rpmCLIMacroContext_s;
/*@-compmempass@*/
MacroContext rpmCLIMacroContext = &rpmCLIMacroContext_s;
/*@=compmempass@*/

/**
 * Macro expansion state.
 */
typedef /*@abstract@*/ struct MacroBuf_s {
/*@kept@*/ /*@exposed@*/
    const char * s;		/*!< Text to expand. */
/*@shared@*/
    char * t;			/*!< Expansion buffer. */
    size_t nb;			/*!< No. bytes remaining in expansion buffer. */
    int depth;			/*!< Current expansion depth. */
    int macro_trace;		/*!< Pre-print macro to expand? */
    int expand_trace;		/*!< Post-print macro expansion? */
/*@kept@*/ /*@exposed@*/ /*@null@*/
    void * spec;		/*!< (future) %file expansion info?. */
/*@kept@*/ /*@exposed@*/
    MacroContext mc;
} * MacroBuf;

#define SAVECHAR(_mb, _c) { *(_mb)->t = (char) (_c), (_mb)->t++, (_mb)->nb--; }

/*@-exportlocal -exportheadervar@*/

#define	_MAX_MACRO_DEPTH	16
/*@unchecked@*/
int max_macro_depth = _MAX_MACRO_DEPTH;

#define	_PRINT_MACRO_TRACE	0
/*@unchecked@*/
int print_macro_trace = _PRINT_MACRO_TRACE;

#define	_PRINT_EXPAND_TRACE	0
/*@unchecked@*/
int print_expand_trace = _PRINT_EXPAND_TRACE;

#define	_MAX_LOAD_DEPTH		2
/*@unchecked@*/
int _max_load_depth = _MAX_LOAD_DEPTH;
/*@=exportlocal =exportheadervar@*/

#define	MACRO_CHUNK_SIZE	16

/* Size of expansion buffers. */
/*@unchecked@*/
static size_t _macro_BUFSIZ = 16 * 1024;

/* forward ref */
static int expandMacro(MacroBuf mb)
	/*@globals rpmGlobalMacroContext,
		print_macro_trace, print_expand_trace, h_errno,
		fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext,
		print_macro_trace, print_expand_trace,
		fileSystem, internalState @*/;

/* =============================================================== */

/**
 * Compare macro entries by name (qsort/bsearch).
 * @param ap		1st macro entry
 * @param bp		2nd macro entry
 * @return		result of comparison
 */
static int
compareMacroName(const void * ap, const void * bp)
	/*@*/
{
    MacroEntry ame = *((MacroEntry *)ap);
    MacroEntry bme = *((MacroEntry *)bp);

    if (ame == NULL && bme == NULL)
	return 0;
    if (ame == NULL)
	return 1;
    if (bme == NULL)
	return -1;
    return strcmp(ame->name, bme->name);
}

/**
 * Enlarge macro table.
 * @param mc		macro context
 */
static void
expandMacroTable(MacroContext mc)
	/*@modifies mc @*/
{
    if (mc->macroTable == NULL) {
	mc->macrosAllocated = MACRO_CHUNK_SIZE;
	mc->macroTable = (MacroEntry *)
	    xmalloc(sizeof(*mc->macroTable) * mc->macrosAllocated);
	mc->firstFree = 0;
    } else {
	mc->macrosAllocated += MACRO_CHUNK_SIZE;
	mc->macroTable = (MacroEntry *)
	    xrealloc(mc->macroTable, sizeof(*mc->macroTable) *
			mc->macrosAllocated);
    }
    memset(&mc->macroTable[mc->firstFree], 0, MACRO_CHUNK_SIZE * sizeof(*(mc->macroTable)));
}

/**
 * Sort entries in macro table.
 * @param mc		macro context
 */
static void
sortMacroTable(MacroContext mc)
	/*@modifies mc @*/
{
    int i;

    if (mc == NULL || mc->macroTable == NULL)
	return;

    qsort(mc->macroTable, mc->firstFree, sizeof(mc->macroTable[0]),
		compareMacroName);

    /* Empty pointers are now at end of table. Reset first free index. */
    for (i = 0; i < mc->firstFree; i++) {
	if (mc->macroTable[i] != NULL)
	    continue;
	mc->firstFree = i;
	break;
    }
}

#if !defined(DEBUG_MACROS)
/*@only@*/
static char * dupMacroEntry(MacroEntry me)
	/*@*/
{
    char * t, * te;
    size_t nb;

assert(me != NULL);
    nb = strlen(me->name) + sizeof("%") - 1;
    if (me->opts)
	nb += strlen(me->opts) + sizeof("()") - 1;
    if (me->body)
	nb += strlen(me->body) + sizeof("\t") - 1;
    nb++;

    t = te = (char *) xmalloc(nb);
    *te = '\0';
    te = stpcpy( stpcpy(te, "%"), me->name);
    if (me->opts)
	te = stpcpy( stpcpy( stpcpy(te, "("), me->opts), ")");
    if (me->body)
	te = stpcpy( stpcpy(te, "\t"), me->body);
    *te = '\0';

    return t;
}
#endif

void
rpmDumpMacroTable(MacroContext mc, FILE * fp)
{
    int nempty = 0;
    int nactive = 0;

    if (mc == NULL) mc = rpmGlobalMacroContext;
    if (fp == NULL) fp = stderr;
    
    fprintf(fp, "========================\n");
    if (mc->macroTable != NULL) {
	int i;
	for (i = 0; i < mc->firstFree; i++) {
	    MacroEntry me;
	    if ((me = mc->macroTable[i]) == NULL) {
		/* XXX this should never happen */
		nempty++;
		continue;
	    }
	    fprintf(fp, "%3d%c %s", me->level,
			(me->used > 0 ? '=' : ':'), me->name);
	    if (me->opts && *me->opts)
		    fprintf(fp, "(%s)", me->opts);
	    if (me->body && *me->body)
		    fprintf(fp, "\t%s", me->body);
	    fprintf(fp, "\n");
	    nactive++;
	}
    }
    fprintf(fp, _("======================== active %d empty %d\n"),
		nactive, nempty);
}

#if !defined(DEBUG_MACROS)
int
rpmGetMacroEntries(MacroContext mc, void * _mire, int used,
		const char *** avp)
{
/*@-assignexpose -castexpose @*/
    miRE mire = (miRE) _mire;
/*@=assignexpose =castexpose @*/
    const char ** av;
    int ac = 0;
    int i;

    if (mc == NULL)
	mc = rpmGlobalMacroContext;

    if (avp == NULL)
	return mc->firstFree;

    av = (const char **) xcalloc( (mc->firstFree+1), sizeof(mc->macroTable[0]));
    if (mc->macroTable != NULL)
    for (i = 0; i < mc->firstFree; i++) {
	MacroEntry me;
	me = mc->macroTable[i];
	if (used > 0 && me->used < used)
	    continue;
	if (used == 0 && me->used != 0)
	    continue;
#if !defined(DEBUG_MACROS)	/* XXX preserve standalone build */
	if (mire != NULL && mireRegexec(mire, me->name, 0) < 0)
	    continue;
#endif
	av[ac++] = dupMacroEntry(me);
    }
    av[ac] = NULL;
    *avp = av = (const char **) xrealloc(av, (ac+1) * sizeof(*av));
    
    return ac;
}
#endif

/**
 * Find entry in macro table.
 * @param mc		macro context
 * @param name		macro name
 * @param namelen	no. of bytes
 * @return		address of slot in macro table with name (or NULL)
 */
/*@dependent@*/ /*@null@*/
static MacroEntry *
findEntry(MacroContext mc, const char * name, size_t namelen)
	/*@*/
{
    MacroEntry key, *ret;

/*@-globs@*/
    if (mc == NULL) mc = rpmGlobalMacroContext;
/*@=globs@*/
    if (mc->macroTable == NULL || mc->firstFree == 0)
	return NULL;

    if (namelen > 0) {
	char * t = strncpy((char *)alloca(namelen + 1), name, namelen);
	t[namelen] = '\0';
	name = t;
    }
    
    key = (MacroEntry) memset(alloca(sizeof(*key)), 0, sizeof(*key));
    /*@-temptrans -assignexpose@*/
    key->name = (char *)name;
    /*@=temptrans =assignexpose@*/
    ret = (MacroEntry *) bsearch(&key, mc->macroTable, mc->firstFree,
			sizeof(*(mc->macroTable)), compareMacroName);
    /* XXX TODO: find 1st empty slot and return that */
    return ret;
}

/* =============================================================== */

/**
 * fgets(3) analogue that reads \ continuations. Last newline always trimmed.
 * @param buf		input buffer
 * @param size		inbut buffer size (bytes)
 * @param fd		file handle
 * @return		buffer, or NULL on end-of-file
 */
/*@null@*/
static char *
rdcl(/*@returned@*/ char * buf, size_t size, FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies buf, fileSystem @*/
{
    char *q = buf - 1;		/* initialize just before buffer. */
    size_t nb = 0;
    size_t nread = 0;
    FILE * f = fdGetFILE(fd);
    int pc = 0, bc = 0;
    char *p = buf;

    if (f != NULL)
    do {
	*(++q) = '\0';			/* terminate and move forward. */
	if (fgets(q, (int)size, f) == NULL)	/* read next line. */
	    break;
	nb = strlen(q);
	nread += nb;			/* trim trailing \r and \n */
	for (q += nb - 1; nb > 0 && iseol(*q); q--)
	    nb--;
	for (; p <= q; p++) {
	    switch (*p) {
		case '\\':
		    switch (*(p+1)) {
			case '\r': /*@switchbreak@*/ break;
			case '\n': /*@switchbreak@*/ break;
			case '\0': /*@switchbreak@*/ break;
			default: p++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '%':
		    switch (*(p+1)) {
			case '{': p++, bc++; /*@switchbreak@*/ break;
			case '(': p++, pc++; /*@switchbreak@*/ break;
			case '%': p++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '{': if (bc > 0) bc++; /*@switchbreak@*/ break;
		case '}': if (bc > 0) bc--; /*@switchbreak@*/ break;
		case '(': if (pc > 0) pc++; /*@switchbreak@*/ break;
		case ')': if (pc > 0) pc--; /*@switchbreak@*/ break;
	    }
	}
	if (nb == 0 || (*q != '\\' && !bc && !pc) || *(q+1) == '\0') {
	    *(++q) = '\0';		/* trim trailing \r, \n */
	    break;
	}
	q++; p++; nb++;			/* copy newline too */
	size -= nb;
	if (*q == '\r')			/* XXX avoid \r madness */
	    *q = '\n';
    } while (size > 0);
    return (nread > 0 ? buf : NULL);
}

/**
 * Return text between pl and matching pr characters.
 * @param p		start of text
 * @param pl		left char, i.e. '[', '(', '{', etc.
 * @param pr		right char, i.e. ']', ')', '}', etc.
 * @return		address of last char before pr (or NULL)
 */
/*@null@*/
static const char *
matchchar(const char * p, char pl, char pr)
	/*@*/
{
    int lvl = 0;
    char c;

    while ((c = *p++) != '\0') {
	if (c == '\\') {		/* Ignore escaped chars */
	    p++;
	    continue;
	}
	if (c == pr) {
	    if (--lvl <= 0)	return --p;
	} else if (c == pl)
	    lvl++;
    }
    return (const char *)NULL;
}

/**
 * Pre-print macro expression to be expanded.
 * @param mb		macro expansion state
 * @param s		current expansion string
 * @param se		end of string
 */
static void
printMacro(MacroBuf mb, const char * s, const char * se)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    const char *senl;
    const char *ellipsis;
    int choplen;

    if (s >= se) {	/* XXX just in case */
	fprintf(stderr, _("%3d>%*s(empty)"), mb->depth,
		(2 * mb->depth + 1), "");
	return;
    }

    if (s[-1] == '{')
	s--;

    /* Print only to first end-of-line (or end-of-string). */
    for (senl = se; *senl && !iseol(*senl); senl++)
	{};

    /* Limit trailing non-trace output */
    choplen = 61 - (2 * mb->depth);
    if ((senl - s) > choplen) {
	senl = s + choplen;
	ellipsis = "...";
    } else
	ellipsis = "";

    /* Substitute caret at end-of-macro position */
    fprintf(stderr, "%3d>%*s%%%.*s^", mb->depth,
	(2 * mb->depth + 1), "", (int)(se - s), s);
    if (se[1] != '\0' && (senl - (se+1)) > 0)
	fprintf(stderr, "%-.*s%s", (int)(senl - (se+1)), se+1, ellipsis);
    fprintf(stderr, "\n");
}

/**
 * Post-print expanded macro expression.
 * @param mb		macro expansion state
 * @param t		current expansion string result
 * @param te		end of string
 */
static void
printExpansion(MacroBuf mb, const char * t, const char * te)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    const char *ellipsis;
    int choplen;

    if (!(te > t)) {
	fprintf(stderr, _("%3d<%*s(empty)\n"), mb->depth, (2 * mb->depth + 1), "");
	return;
    }

    /* Shorten output which contains newlines */
    while (te > t && iseol(te[-1]))
	te--;
    ellipsis = "";
    if (mb->depth > 0) {
	const char *tenl;

	/* Skip to last line of expansion */
	while ((tenl = strchr(t, '\n')) && tenl < te)
	    t = ++tenl;

	/* Limit expand output */
	choplen = 61 - (2 * mb->depth);
	if ((te - t) > choplen) {
	    te = t + choplen;
	    ellipsis = "...";
	}
    }

    fprintf(stderr, "%3d<%*s", mb->depth, (2 * mb->depth + 1), "");
    if (te > t)
	fprintf(stderr, "%.*s%s", (int)(te - t), t, ellipsis);
    fprintf(stderr, "\n");
}

#define	SKIPBLANK(_s, _c)	\
	/*@-globs@*/	/* FIX: __ctype_b */ \
	while (((_c) = (int) *(_s)) && isblank(_c)) \
		(_s)++;		\
	/*@=globs@*/

#define	SKIPNONBLANK(_s, _c)	\
	/*@-globs@*/	/* FIX: __ctype_b */ \
	while (((_c) = (int) *(_s)) && !(isblank(_c) || iseol(_c))) \
		(_s)++;		\
	/*@=globs@*/

#define	COPYNAME(_ne, _s, _c)	\
    {	SKIPBLANK(_s,_c);	\
	while(((_c) = (int) *(_s)) && (xisalnum(_c) || (_c) == (int) '_')) \
		*(_ne)++ = *(_s)++; \
	*(_ne) = '\0';		\
    }

#define	COPYOPTS(_oe, _s, _c)	\
    {   while(((_c) = (int) *(_s)) && (_c) != (int) ')') \
		*(_oe)++ = *(_s)++; \
	*(_oe) = '\0';		\
    }

/**
 * Save source and expand field into target.
 * @param mb		macro expansion state
 * @param f		field
 * @param flen		no. bytes in field
 * @return		result of expansion
 */
static int
expandT(MacroBuf mb, const char * f, size_t flen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    char *sbuf;
    const char *s = mb->s;
    int rc;

    sbuf = (char *) alloca(flen + 1);
    memset(sbuf, 0, (flen + 1));

    strncpy(sbuf, f, flen);
    sbuf[flen] = '\0';
    mb->s = sbuf;
    rc = expandMacro(mb);
    mb->s = s;
    return rc;
}

#if 0
/**
 * Save target and expand sbuf into target.
 * @param mb		macro expansion state
 * @param tbuf		target buffer
 * @param tbuflen	no. bytes in target buffer
 * @return		result of expansion
 */
static int
expandS(MacroBuf mb, char * tbuf, size_t tbuflen)
	/*@globals rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies mb, *tbuf, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char *t = mb->t;
    size_t nb = mb->nb;
    int rc;

    mb->t = tbuf;
    mb->nb = tbuflen;
    rc = expandMacro(mb);
    mb->t = t;
    mb->nb = nb;
    return rc;
}
#endif

/**
 * Save source/target and expand macro in u.
 * @param mb		macro expansion state
 * @param u		input macro, output expansion
 * @param ulen		no. bytes in u buffer
 * @return		result of expansion
 */
static int
expandU(MacroBuf mb, char * u, size_t ulen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, *u, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char *s = mb->s;
    char *t = mb->t;
    size_t nb = mb->nb;
    char *tbuf;
    int rc;

    tbuf = (char *) alloca(ulen + 1);
    memset(tbuf, 0, (ulen + 1));

    mb->s = u;
    mb->t = tbuf;
    mb->nb = ulen;
    rc = expandMacro(mb);

    tbuf[ulen] = '\0';	/* XXX just in case */
    if (ulen > mb->nb)
	strncpy(u, tbuf, (ulen - mb->nb + 1));

    mb->s = s;
    mb->t = t;
    mb->nb = nb;

    return rc;
}

/**
 * Expand output of shell command into target buffer.
 * @param mb		macro expansion state
 * @param cmd		shell command
 * @param clen		no. bytes in shell command
 * @return		result of expansion
 */
static int
doShellEscape(MacroBuf mb, const char * cmd, size_t clen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    size_t bufn = _macro_BUFSIZ + clen;
    char * buf = (char *) alloca(bufn);
    FILE *shf;
    int rc;
    int c;

    strncpy(buf, cmd, clen);
    buf[clen] = '\0';
    rc = expandU(mb, buf, bufn);
    if (rc)
	return rc;

    if ((shf = popen(buf, "r")) == NULL)
	return 1;
    while(mb->nb > 0 && (c = fgetc(shf)) != EOF)
	SAVECHAR(mb, c);
    (void) pclose(shf);

    /* XXX delete trailing \r \n */
    while (iseol(mb->t[-1])) {
	*(mb->t--) = '\0';
	mb->nb++;
    }
    return 0;
}

/**
 * Parse (and execute) new macro definition.
 * @param mb		macro expansion state
 * @param se		macro definition to parse
 * @param level		macro recursion level
 * @param expandbody	should body be expanded?
 * @return		address to continue parsing
 */
/*@dependent@*/ static const char *
doDefine(MacroBuf mb, /*@returned@*/ const char * se, int level, int expandbody)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, internalState @*/
{
    const char *s = se;
    size_t bufn = _macro_BUFSIZ;
    char *buf = (char *) alloca(bufn);
    char *n = buf, *ne;
    char *o = NULL, *oe;
    char *b, *be;
    int c;
    int oc = (int) ')';

    SKIPBLANK(s, c);
    if (c == (int) '.')		/* XXX readonly macros */
/*@i@*/	*n++ = c = *s++;
    if (c == (int) '.')		/* XXX readonly macros */
/*@i@*/	*n++ = c = *s++;
    ne = n;

    /* Copy name */
    COPYNAME(ne, s, c);

    /* Copy opts (if present) */
    oe = ne + 1;
    if (*s == '(') {
	s++;	/* skip ( */
	o = oe;
	COPYOPTS(oe, s, oc);
	/* Options must be terminated with ')' */
	if (oc != ')') {
	    rpmlog(RPMLOG_ERR, _("Macro %%%s has unterminated opts\n"), n);
	    se = s;	/* XXX W2DO? */
	    return se;
	}
	s++;	/* skip ) */
    }

    /* Copy body, skipping over escaped newlines */
    b = be = oe + 1;
    SKIPBLANK(s, c);
    if (c == (int) '{') {	/* XXX permit silent {...} grouping */
	if ((se = matchchar(s, (char) c, '}')) == NULL) {
	    rpmlog(RPMLOG_ERR,
		_("Macro %%%s has unterminated body\n"), n);
	    se = s;	/* XXX W2DO? */
	    return se;
	}
	s++;	/* XXX skip { */
	strncpy(b, s, (se - s));
	b[se - s] = '\0';
	be += strlen(b);
	se++;	/* XXX skip } */
	s = se;	/* move scan forward */
    } else {	/* otherwise free-field */
	int bc = 0, pc = 0;
	while (*s && (bc || pc || !iseol(*s))) {
	    switch (*s) {
		case '\\':
		    switch (*(s+1)) {
			case '\0': /*@switchbreak@*/ break;
			default: s++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '%':
		    switch (*(s+1)) {
			case '{': *be++ = *s++; bc++; /*@switchbreak@*/ break;
			case '(': *be++ = *s++; pc++; /*@switchbreak@*/ break;
			case '%': *be++ = *s++; /*@switchbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '{': if (bc > 0) bc++; /*@switchbreak@*/ break;
		case '}': if (bc > 0) bc--; /*@switchbreak@*/ break;
		case '(': if (pc > 0) pc++; /*@switchbreak@*/ break;
		case ')': if (pc > 0) pc--; /*@switchbreak@*/ break;
	    }
	    *be++ = *s++;
	}
	*be = '\0';

	if (bc || pc) {
	    rpmlog(RPMLOG_ERR,
		_("Macro %%%s has unterminated body\n"), n);
	    se = s;	/* XXX W2DO? */
	    return se;
	}

	/* Trim trailing blanks/newlines */
/*@-globs@*/
	while (--be >= b && (c = (int) *be) && (isblank(c) || iseol(c)))
	    {};
/*@=globs@*/
	*(++be) = '\0';	/* one too far */
    }

    /* Move scan over body */
    while (iseol(*s))
	s++;
    se = s;

    /* Names must start with alphabetic or _ and be at least 3 chars */
    if (!((c = (int) *n) && (xisalpha(c) || c == (int) '_') && (ne - n) > 2)) {
	rpmlog(RPMLOG_ERR,
		_("Macro %%%s has illegal name (%%define)\n"), n);
	return se;
    }

    if ((be - b) < 1) {
	rpmlog(RPMLOG_ERR, _("Macro %%%s has empty body\n"), n);
	return se;
    }

/*@-modfilesys@*/
    if (expandbody && expandU(mb, b, (&buf[bufn] - b))) {
	rpmlog(RPMLOG_ERR, _("Macro %%%s failed to expand\n"), n);
	return se;
    }
/*@=modfilesys@*/

    if (n != buf)		/* XXX readonly macros */
	n--;
    if (n != buf)		/* XXX readonly macros */
	n--;
    addMacro(mb->mc, n, o, b, (level - 1));

    return se;
}

/**
 * Parse (and execute) macro undefinition.
 * @param mc		macro context
 * @param se		macro name to undefine
 * @return		address to continue parsing
 */
/*@dependent@*/ static const char *
doUndefine(MacroContext mc, /*@returned@*/ const char * se)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies mc, rpmGlobalMacroContext @*/
{
    const char *s = se;
    char *buf = (char *) alloca(_macro_BUFSIZ);
    char *n = buf, *ne = n;
    int c;

    COPYNAME(ne, s, c);

    /* Move scan over body */
    while (iseol(*s))
	s++;
    se = s;

    /* Names must start with alphabetic or _ and be at least 3 chars */
    if (!((c = (int) *n) && (xisalpha(c) || c == (int) '_') && (ne - n) > 2)) {
	rpmlog(RPMLOG_ERR,
		_("Macro %%%s has illegal name (%%undefine)\n"), n);
	return se;
    }

    delMacro(mc, n);

    return se;
}

#ifdef	DYING
static void
dumpME(const char * msg, MacroEntry me)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (msg)
	fprintf(stderr, "%s", msg);
    fprintf(stderr, "\tme %p", me);
    if (me)
	fprintf(stderr,"\tname %p(%s) prev %p",
		me->name, me->name, me->prev);
    fprintf(stderr, "\n");
}
#endif

/**
 * Push new macro definition onto macro entry stack.
 * @param mep		address of macro entry slot
 * @param n		macro name
 * @param o		macro parameters (NULL if none)
 * @param b		macro body (NULL becomes "")
 * @param level		macro recursion level
 */
static void
pushMacro(/*@out@*/ MacroEntry * mep, const char * n, /*@null@*/ const char * o,
		/*@null@*/ const char * b, int level)
	/*@modifies *mep @*/
{
    MacroEntry prev = (mep && *mep ? *mep : NULL);
    MacroEntry me = (MacroEntry) xmalloc(sizeof(*me));
    const char *name = n;

    if (*name == '.')		/* XXX readonly macros */
	name++;
    if (*name == '.')		/* XXX readonly macros */
	name++;

    /*@-assignexpose@*/
    me->prev = prev;
    /*@=assignexpose@*/
    me->name = (prev ? prev->name : xstrdup(name));
    me->opts = (o ? xstrdup(o) : NULL);
    me->body = xstrdup(b ? b : "");
    me->used = 0;
    me->level = level;
    me->flags = (name != n);
    if (mep)
	*mep = me;
    else {
	if (me) free(me);
	me = NULL;
    }
}

/**
 * Pop macro definition from macro entry stack.
 * @param mep		address of macro entry slot
 */
static void
popMacro(MacroEntry * mep)
	/*@modifies *mep @*/
{
	MacroEntry me = (*mep ? *mep : NULL);

	if (me) {
		/* XXX cast to workaround const */
		/*@-onlytrans@*/
		if ((*mep = me->prev) == NULL)
			me->name = _free(me->name);
		me->opts = _free(me->opts);
		me->body = _free(me->body);
		if (me) free(me);
		me = NULL;
		/*@=onlytrans@*/
	}
}

/**
 * Free parsed arguments for parameterized macro.
 * @param mb		macro expansion state
 */
static void
freeArgs(MacroBuf mb)
	/*@modifies mb @*/
{
    MacroContext mc = mb->mc;
    int ndeleted = 0;
    int i;

    if (mc == NULL || mc->macroTable == NULL)
	return;

    /* Delete dynamic macro definitions */
    for (i = 0; i < mc->firstFree; i++) {
	MacroEntry *mep, me;
	int skiptest = 0;
	mep = &mc->macroTable[i];
	me = *mep;

	if (me == NULL)		/* XXX this should never happen */
	    continue;
	if (me->level < mb->depth)
	    continue;
	if (strlen(me->name) == 1 && strchr("#*0", *me->name)) {
	    if (*me->name == '*' && me->used > 0)
		skiptest = 1; /* XXX skip test for %# %* %0 */
	} else if (!skiptest && me->used <= 0) {
#if NOTYET
	    rpmlog(RPMLOG_ERR,
			_("Macro %%%s (%s) was not used below level %d\n"),
			me->name, me->body, me->level);
#endif
	}
	popMacro(mep);
	if (!(mep && *mep))
	    ndeleted++;
    }

    /* If any deleted macros, sort macro table */
    if (ndeleted)
	sortMacroTable(mc);
}

/**
 * Parse arguments (to next new line) for parameterized macro.
 * @todo Use popt rather than getopt to parse args.
 * @param mb		macro expansion state
 * @param me		macro entry slot
 * @param se		arguments to parse
 * @param lastc		stop parsing at lastc
 * @return		address to continue parsing
 */
/*@dependent@*/ static const char *
grabArgs(MacroBuf mb, const MacroEntry me, /*@returned@*/ const char * se,
		const char * lastc)
	/*@globals rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    poptContext optCon;
    struct poptOption *optTbl;
    size_t bufn = _macro_BUFSIZ;
    char *buf = (char *) alloca(bufn);
    char *b, *be;
    char aname[16];
    const char *opts;
    int argc = 0;
    const char **argv;
    int c;
    unsigned int popt_flags;

    /* Copy macro name as argv[0], save beginning of args.  */
    buf[0] = '\0';
    b = be = stpcpy(buf, me->name);

    addMacro(mb->mc, "0", NULL, buf, mb->depth);
    
    argc = 1;	/* XXX count argv[0] */

    /* Copy args into buf until lastc */
    *be++ = ' ';
    while ((c = (int) *se++) != (int) '\0' && (se-1) != lastc) {
/*@-globs@*/
	if (!isblank(c)) {
	    *be++ = (char) c;
	    continue;
	}
/*@=globs@*/
	/* c is blank */
	if (be[-1] == ' ')
	    continue;
	/* a word has ended */
	*be++ = ' ';
	argc++;
    }
    if (c == (int) '\0') se--;	/* one too far */
    if (be[-1] != ' ')
	argc++, be++;		/* last word has not trailing ' ' */
    be[-1] = '\0';
    if (*b == ' ') b++;		/* skip the leading ' ' */

/*
 * The macro %* analoguous to the shell's $* means "Pass all non-macro
 * parameters." Consequently, there needs to be a macro that means "Pass all
 * (including macro parameters) options". This is useful for verifying
 * parameters during expansion and yet transparently passing all parameters
 * through for higher level processing (e.g. %description and/or %setup).
 * This is the (potential) justification for %{**} ...
 */
    /* Add unexpanded args as macro */
    addMacro(mb->mc, "**", NULL, b, mb->depth);

#ifdef NOTYET
    /* XXX if macros can be passed as args ... */
    expandU(mb, buf, bufn);
#endif

    /* Build argv array */
    argv = (const char **) alloca((argc + 1) * sizeof(*argv));
    be[-1] = ' '; /* assert((be - 1) == (b + strlen(b) == buf + strlen(buf))) */
    be[0] = '\0';

    b = buf;
    for (c = 0; c < argc; c++) {
	argv[c] = b;
	b = strchr(b, ' ');
	*b++ = '\0';
    }
    /* assert(b == be);  */
    argv[argc] = NULL;

    /* '+' as the first character means that options are recognized
     * only before positional arguments, as POSIX requires.
    */
    popt_flags = POPT_CONTEXT_NO_EXEC;
#if defined(RPM_VENDOR_OPENPKG) /* always-strict-posix-option-parsing */
    popt_flags |= POPT_CONTEXT_POSIXMEHARDER;
#endif
    if (me->opts[0] == '+') popt_flags |= POPT_CONTEXT_POSIXMEHARDER;

    /* Count the number of short options. */
    opts = me->opts;
    if (*opts == '+') opts++;
    for (c = 0; *opts != '\0'; opts++)
	if (*opts != ':') c++;

    /* Set up popt option table. */
    optTbl = (struct poptOption *) xcalloc(sizeof(*optTbl), (c + 1));
    opts = me->opts;
    if (*opts == '+') opts++;
    for (c = 0; *opts != '\0'; opts++) {
	if (*opts == ':') continue;
	optTbl[c].shortName = opts[0];
	optTbl[c].val = (int) opts[0];
	if (opts[1] == ':')
	    optTbl[c].argInfo = POPT_ARG_STRING;
	c++;
    }

    /* Parse the options, defining option macros. */
/*@-nullstate@*/
    optCon = poptGetContext(argv[0], argc, argv, optTbl, popt_flags);
/*@=nullstate@*/
    while ((c = poptGetNextOpt(optCon)) > 0) {
	const char * optArg = poptGetOptArg(optCon);
	*be++ = '-';
	*be++ = (char) c;
	if (optArg != NULL) {
	    *be++ = ' ';
	    be = stpcpy(be, optArg);
	}
	*be++ = '\0';
	aname[0] = '-'; aname[1] = (char)c; aname[2] = '\0';
	addMacro(mb->mc, aname, NULL, b, mb->depth);
	if (optArg != NULL) {
	    aname[0] = '-'; aname[1] = (char)c; aname[2] = '*'; aname[3] = '\0';
	    addMacro(mb->mc, aname, NULL, optArg, mb->depth);
	}
	be = b; /* reuse the space */
/*@-dependenttrans -modobserver -observertrans @*/
	optArg = _free(optArg);
/*@=dependenttrans =modobserver =observertrans @*/
    }
    if (c < -1) {
	rpmlog(RPMLOG_ERR, _("Unknown option in macro %s(%s): %s: %s\n"),
		me->name, me->opts,
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(c));
	goto exit;
    }

    argv = poptGetArgs(optCon);
    argc = 0;
    if (argv != NULL)
    for (c = 0; argv[c] != NULL; c++)
	argc++;
    
    /* Add arg count as macro. */
    sprintf(aname, "%d", argc);
    addMacro(mb->mc, "#", NULL, aname, mb->depth);

    /* Add macro for each arg. Concatenate args for %*. */
    if (be) {
	*be = '\0';
	if (argv != NULL)
	for (c = 0; c < argc; c++) {
	    sprintf(aname, "%d", (c + 1));
	    addMacro(mb->mc, aname, NULL, argv[c], mb->depth);
	    if (be != b) *be++ = ' '; /* Add space between args */
	    be = stpcpy(be, argv[c]);
	}
    }

    /* Add unexpanded args as macro. */
    addMacro(mb->mc, "*", NULL, b, mb->depth);

exit:
    optCon = poptFreeContext(optCon);
    if (optTbl) free(optTbl);
    optTbl = NULL;
    return se;
}

/**
 * Perform macro message output
 * @param mb		macro expansion state
 * @param waserror	use rpmError()?
 * @param msg		message to ouput
 * @param msglen	no. of bytes in message
 */
static void
doOutput(MacroBuf mb, int waserror, const char * msg, size_t msglen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    size_t bufn = _macro_BUFSIZ + msglen;
    char *buf = (char *) alloca(bufn);

    strncpy(buf, msg, msglen);
    buf[msglen] = '\0';
    (void) expandU(mb, buf, bufn);
    if (waserror)
	rpmlog(RPMLOG_ERR, "%s\n", buf);
    else
	fprintf(stderr, "%s", buf);
}

/**
 * Execute macro primitives.
 * @param mb		macro expansion state
 * @param negate	should logic be inverted?
 * @param f		beginning of field f
 * @param fn		length of field f
 * @param g		beginning of field g
 * @param gn		length of field g
 */
static void
doFoo(MacroBuf mb, int negate, const char * f, size_t fn,
		/*@null@*/ const char * g, size_t gn)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    size_t bufn = _macro_BUFSIZ + fn + gn;
    char * buf = (char *) alloca(bufn);
    char *b = NULL, *be;
    int c;
    mode_t mode;

    buf[0] = '\0';
    if (g != NULL) {
	strncpy(buf, g, gn);
	buf[gn] = '\0';
	(void) expandU(mb, buf, bufn);
    }
    if (fn > 5 && STREQ("patch", f, 5) && xisdigit((int)f[5])) {
	/* Skip leading zeros */
	for (c = 5; c < (int)(fn-1) && f[c] == '0' && xisdigit((int)f[c+1]);)
	    c++;
	b = buf;
	be = stpncpy( stpcpy(b, "%patch -P "), f+c, fn-c);
	*be = '\0';
    } else
    if (STREQ("basename", f, fn)) {
	if ((b = strrchr(buf, '/')) == NULL)
	    b = buf;
	else
	    b++;
    } else if (STREQ("dirname", f, fn)) {
	if ((b = strrchr(buf, '/')) != NULL)
	    *b = '\0';
	b = buf;
#if !defined(__LCLINT__) /* XXX LCL: realpath(3) annotations are buggy. */
    } else if (STREQ("realpath", f, fn)) {
	char rp[PATH_MAX];
	char *cp;
	size_t l;
	if ((cp = realpath(buf, rp)) != NULL) {
	    l = strlen(cp);
	    if ((size_t)(l+1) <= bufn) {
		memcpy(buf, cp, l+1);
		b = buf;
	    }
	}
#endif
    } else if (STREQ("getenv", f, fn)) {
	char *cp;
	if ((cp = getenv(buf)) != NULL)
	    b = cp;
    } else if (STREQ("shrink", f, fn)) {
	/*
	 * shrink body by removing all leading and trailing whitespaces and
	 * reducing intermediate whitespaces to a single space character.
	 */
	int i, j, k, was_space = 0;
	for (i = 0, j = 0, k = (int)strlen(buf); i < k; ) {
	    if (xisspace((int)(buf[i]))) {
		was_space = 1;
		i++;
		continue;
	    }
	    else if (was_space) {
		was_space = 0;
		if (j > 0) /* remove leading blanks at all */
		    buf[j++] = ' ';
		/* fallthrough */
	    }
	    buf[j++] = buf[i++];
	}
	buf[j] = '\0';
	b = buf;
    } else if (STREQ("suffix", f, fn)) {
	if ((b = strrchr(buf, '.')) != NULL)
	    b++;
    } else if (STREQ("expand", f, fn)) {
	b = buf;
    } else if (STREQ("verbose", f, fn)) {
#if defined(RPMLOG_MASK)
	if (negate)
	    b = ((rpmlogSetMask(0) >= RPMLOG_MASK( RPMLOG_INFO )) ? NULL : buf);
	else
	    b = ((rpmlogSetMask(0) >= RPMLOG_MASK( RPMLOG_INFO )) ? buf : NULL);
#else
	/* XXX assume always verbose when running standalone */
	b = (negate) ? NULL : buf;
#endif
    } else if (STREQ("url2path", f, fn) || STREQ("u2p", f, fn)) {
	int ut = urlPath(buf, (const char **)&b);
	ut = ut;	/* XXX quiet gcc */
	if (*b == '\0') b = (char *) "/";
    } else if (STREQ("uncompress", f, fn)) {
	rpmCompressedMagic compressed = COMPRESSED_OTHER;
/*@-globs@*/
	for (b = buf; (c = (int)*b) && isblank(c);)
	    b++;
	/* XXX FIXME: file paths with embedded white space needs rework. */
	for (be = b; (c = (int)*be) && !isblank(c);)
	    be++;
/*@=globs@*/
	*be++ = '\0';
	(void) isCompressed(b, &compressed);
	switch(compressed) {
	default:
	case 0:	/* COMPRESSED_NOT */
	    sprintf(be, "%%__cat %s", b);
	    break;
	case 1:	/* COMPRESSED_OTHER */
	    sprintf(be, "%%__gzip -dc '%s'", b);
	    break;
	case 2:	/* COMPRESSED_BZIP2 */
	    sprintf(be, "%%__bzip2 -dc '%s'", b);
	    break;
	case 3:	/* COMPRESSED_ZIP */
	    sprintf(be, "%%__unzip -qq '%s'", b);
	    break;
	case 4:	/* COMPRESSED_LZOP */
	    sprintf(be, "%%__lzop -dc '%s'", b);
	    break;
	case 5:	/* COMPRESSED_LZMA */
	    sprintf(be, "%%__lzma -dc '%s'", b);
	    break;
	case 6:	/* COMPRESSED_XZ */
	    sprintf(be, "%%__xz -dc '%s'", b);
	    break;
	case 7: /* COMPRESSED_LRZIP */
	    sprintf(be, "%%__lrzip -dqo- %s", b);
	    break;
	case 8:	/* COMPRESSED_LZIP */
	    sprintf(be, "%%__lzip -dc %s", b);
	    break;
	case 9: /* COMPRESSED_7ZIP */
	    sprintf(be, "%%__7zip x %s", b);
	    break;
	}
	b = be;
    } else if (STREQ("mkstemp", f, fn)) {
/*@-globs@*/
	for (b = buf; (c = (int)*b) && isblank(c);)
	    b++;
	/* XXX FIXME: file paths with embedded white space needs rework. */
	for (be = b; (c = (int)*be) && !isblank(c);)
	    be++;
/*@=globs@*/
#if defined(HAVE_MKSTEMP)
        mode = umask(0077);
	(void) close(mkstemp(b));
        (void) umask(mode);
#else
	(void) mktemp(b);
#endif
    } else if (STREQ("mkdtemp", f, fn)) {
/*@-globs@*/
	for (b = buf; (c = (int)*b) && isblank(c);)
	    b++;
	/* XXX FIXME: file paths with embedded white space needs rework. */
	for (be = b; (c = (int)*be) && !isblank(c);)
	    be++;
/*@=globs@*/
#if defined(HAVE_MKDTEMP) && !defined(__LCLINT__)
	if (mkdtemp(b) == NULL)
	    perror("mkdtemp");
#else
	if ((b = tmpnam(b)) != NULL)
	    (void) mkdir(b, 0700);	/* XXX S_IWRSXU is not included. */
#endif
    } else if (STREQ("uuid", f, fn)) {
	int uuid_version;
	const char *uuid_ns;
	const char *uuid_data;
	char *cp;
	size_t n;

	uuid_version = 1;
	uuid_ns = NULL;
	uuid_data = NULL;
	cp = buf;
	if ((n = strspn(cp, " \t\n")) > 0)
	    cp += n;
	if ((n = strcspn(cp, " \t\n")) > 0) {
	    uuid_version = (int)strtol(cp, (char **)NULL, 10);
	    cp += n;
	    if ((n = strspn(cp, " \t\n")) > 0)
		cp += n;
	    if ((n = strcspn(cp, " \t\n")) > 0) {
		uuid_ns = cp;
		cp += n;
		*cp++ = '\0';
		if ((n = strspn(cp, " \t\n")) > 0)
		    cp += n;
		if ((n = strcspn(cp, " \t\n")) > 0) {
		    uuid_data = cp;
		    cp += n;
		    *cp++ = '\0';
		}
	    }
	}
/*@-nullpass@*/	/* FIX: uuid_ns may be NULL */
	if (rpmuuidMake(uuid_version, uuid_ns, uuid_data, buf, NULL))
	    rpmlog(RPMLOG_ERR, "failed to create UUID\n");
	else
	    b = buf;
/*@=nullpass@*/
    } else if (STREQ("S", f, fn)) {
	for (b = buf; (c = (int)*b) && xisdigit(c);)
	    b++;
	if (!c) {	/* digit index */
	    b++;
	    sprintf(b, "%%SOURCE%s", buf);
	} else
	    b = buf;
    } else if (STREQ("P", f, fn)) {
	for (b = buf; (c = (int) *b) && xisdigit(c);)
	    b++;
	if (!c) {	/* digit index */
	    b++;
	    sprintf(b, "%%PATCH%s", buf);
	} else
	    b = buf;
    } else if (STREQ("F", f, fn)) {
	b = buf + strlen(buf) + 1;
	sprintf(b, "file%s.file", buf);
    }

    if (b) {
	(void) expandT(mb, b, strlen(b));
    }
}

static int expandFIFO(MacroBuf mb, MacroEntry me, const char *g, size_t gn)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext, fileSystem, internalState @*/
{
    int rc = 0;

    if (me) {
	if (me->prev) {
	    rc = expandFIFO(mb, me->prev, g, gn);
	    rc = expandT(mb, g, gn);
	}
	rc = expandT(mb, me->body, strlen(me->body));
    }
    return rc;
}

#if !defined(DEBUG_MACROS)
/* =============================================================== */
/* XXX dupe'd to avoid change in linkage conventions. */

#define POPT_ERROR_NOARG        -10     /*!< missing argument */
#define POPT_ERROR_BADQUOTE     -15     /*!< error in paramter quoting */
#define POPT_ERROR_MALLOC       -21     /*!< memory allocation failed */

#define POPT_ARGV_ARRAY_GROW_DELTA 5

static int XpoptDupArgv(int argc, char **argv,
		int * argcPtr, char *** argvPtr)
	/*@modifies *argcPtr, *argvPtr @*/
{
    size_t nb = (argc + 1) * sizeof(*argv);
    char ** argv2;
    char * dst;
    int i;

    if (argc <= 0 || argv == NULL)	/* XXX can't happen */
	return POPT_ERROR_NOARG;
    for (i = 0; i < argc; i++) {
	if (argv[i] == NULL)
	    return POPT_ERROR_NOARG;
	nb += strlen(argv[i]) + 1;
    }
	
    dst = (char *) xmalloc(nb);
    if (dst == NULL)			/* XXX can't happen */
	return POPT_ERROR_MALLOC;
    argv2 = (char **) dst;
    dst += (argc + 1) * sizeof(*argv);

    for (i = 0; i < argc; i++) {
	argv2[i] = dst;
	dst += strlen(strcpy(dst, argv[i])) + 1;
    }
    argv2[argc] = NULL;

    if (argvPtr) {
	*argvPtr = argv2;
    } else {
	free(argv2);
	argv2 = NULL;
    }
    if (argcPtr)
	*argcPtr = argc;
    return 0;
}

static int XpoptParseArgvString(const char * s, int * argcPtr, char *** argvPtr)
	/*@modifies *argcPtr, *argvPtr @*/
{
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    char ** argv = (char **) xmalloc(sizeof(*argv) * argvAlloced);
    int argc = 0;
    size_t buflen = strlen(s) + 1;
    char * buf = (char *) memset(alloca(buflen), 0, buflen);
    int rc = POPT_ERROR_MALLOC;

    if (argv == NULL) return rc;
    argv[argc] = buf;

    for (src = s; *src != '\0'; src++) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote != '\0') {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    rc = POPT_ERROR_BADQUOTE;
		    goto exit;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (isspace(*src)) {
	    if (*argv[argc] != '\0') {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
		    argv = (char **) realloc(argv, sizeof(*argv) * argvAlloced);
		    if (argv == NULL) goto exit;
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    /*@switchbreak@*/ break;
	  case '\\':
	    src++;
	    if (!*src) {
		rc = POPT_ERROR_BADQUOTE;
		goto exit;
	    }
	    /*@fallthrough@*/
	  default:
	    *buf++ = *src;
	    /*@switchbreak@*/ break;
	}
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    rc = XpoptDupArgv(argc, argv, argcPtr, argvPtr);

exit:
    if (argv) free(argv);
    return rc;
}
#endif	/* !defined(DEBUG_MACROS) */

/**
 * Parse args and string for PHP like %{foo <args> : <string> } syntax.
 * @param s		"{ ... }" construct to parse
 * @param nb		no. of bytes
 * @retval *avp		invocation args
 * @return		script string
 */
#if defined(WITH_AUGEAS) || defined(WITH_FICL) || defined(WITH_GPSEE) || defined(WITH_JNIEMBED) || defined(WITH_PERLEMBED) || defined(WITH_PYTHONEMBED) || defined(WITH_RUBYEMBED) || defined(WITH_MRUBY_EMBED) || defined(WITH_SQLITE) || defined(WITH_SQUIRREL) || defined(WITH_TCL)
static char _FIXME_embedded_interpreter_eval_returned_null[] =
    "FIXME: embedded interpreter eval returned null.";
static char * parseEmbedded(const char * s, size_t nb, char *** avp)
	/*@*/
{
    char * script = NULL;
    const char * se;

    /* XXX FIXME: args might have embedded : too. */
    for (se = s + 1; se < (s+nb); se++)
    switch (*se) {
    default:	continue;	/*@notreached@*/ break;
    case ':':	goto bingo;	/*@notreached@*/ break;
    }

bingo:
    {	size_t na = (size_t)(se-s-1);
	char * args = NULL;
	int ac;
	int rc;

	args = (char *) memcpy(xmalloc(na+1), s+1, na);
	args[na] = '\0';

	ac = 0;
	rc = XpoptParseArgvString(args, &ac, avp);
	args = _free(args);
	nb -= na;
    }

    nb -= (nb >= (sizeof("{:}")-1) ? (sizeof("{:}")-1) : nb);
    script = (char *) memcpy(xmalloc(nb+1), se+1, nb+1);
    script[nb] = '\0';
    return script;
}
#endif

/**
 * The main macro recursion loop.
 * @todo Dynamically reallocate target buffer.
 * @param mb		macro expansion state
 * @return		0 on success, 1 on failure
 */
static int
expandMacro(MacroBuf mb)
	/*@globals rpmGlobalMacroContext,
		print_macro_trace, print_expand_trace, h_errno,
		fileSystem, internalState @*/
	/*@modifies mb, rpmGlobalMacroContext,
		print_macro_trace, print_expand_trace,
		fileSystem, internalState @*/
{
    MacroEntry *mep;
    MacroEntry me;
    const char *s = mb->s, *se;
    const char *f, *fe;
    const char *g, *ge;
    size_t fn, gn;
    char *t = mb->t;	/* save expansion pointer for printExpand */
    int c;
    int rc = 0;
    int negate;
    int stackarray;
    const char * lastc;
    int chkexist;

    if (++mb->depth > max_macro_depth) {
	rpmlog(RPMLOG_ERR,
		_("Recursion depth(%d) greater than max(%d)\n"),
		mb->depth, max_macro_depth);
	mb->depth--;
	mb->expand_trace = 1;
	return 1;
    }

    while (rc == 0 && mb->nb > 0 && (c = (int)*s) != (int)'\0') {
	s++;
	/* Copy text until next macro */
	switch(c) {
	case '%':
		if (*s != '\0') {	/* Ensure not end-of-string. */
		    if (*s != '%')
			/*@switchbreak@*/ break;
		    s++;	/* skip first % in %% */
		}
		/*@fallthrough@*/
	default:
		SAVECHAR(mb, c);
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	}

	/* Expand next macro */
	f = fe = NULL;
	g = ge = NULL;
	if (mb->depth > 1)	/* XXX full expansion for outermost level */
		t = mb->t;	/* save expansion pointer for printExpand */
	stackarray = chkexist = negate = 0;
	lastc = NULL;
	switch ((c = (int) *s)) {
	default:		/* %name substitution */
		while (*s != '\0' && strchr("!?@", *s) != NULL) {
			switch(*s++) {
			case '@':
				stackarray = ((stackarray + 1) % 2);
				/*@switchbreak@*/ break;
			case '!':
				negate = ((negate + 1) % 2);
				/*@switchbreak@*/ break;
			case '?':
				chkexist++;
				/*@switchbreak@*/ break;
			}
		}
		f = se = s;
		if (*se == '-')
			se++;
		while((c = (int) *se) && (xisalnum(c) || c == (int) '_'))
			se++;
		/* Recognize non-alnum macros too */
		switch (*se) {
		case '*':
			se++;
			if (*se == '*') se++;
			/*@innerbreak@*/ break;
		case '#':
			se++;
			/*@innerbreak@*/ break;
		default:
			/*@innerbreak@*/ break;
		}
		fe = se;
		/* For "%name " macros ... */
/*@-globs@*/
		if ((c = (int) *fe) && isblank(c))
			if ((lastc = strchr(fe,'\n')) == NULL)
				lastc = strchr(fe, '\0');
/*@=globs@*/
		/*@switchbreak@*/ break;
	case '(':		/* %(...) shell escape */
		if ((se = matchchar(s, (char)c, ')')) == NULL) {
			rpmlog(RPMLOG_ERR,
				_("Unterminated %c: %s\n"), (char)c, s);
			rc = 1;
			continue;
		}
		if (mb->macro_trace)
			printMacro(mb, s, se+1);

		s++;	/* skip ( */
		rc = doShellEscape(mb, s, (se - s));
		se++;	/* skip ) */

		s = se;
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	case '{':		/* %{...}/%{...:...} substitution */
		if ((se = matchchar(s, (char)c, '}')) == NULL) {
			rpmlog(RPMLOG_ERR,
				_("Unterminated %c: %s\n"), (char)c, s);
			rc = 1;
			continue;
		}
		f = s+1;/* skip { */
		se++;	/* skip } */
		while (strchr("!?@", *f) != NULL) {
			switch(*f++) {
			case '@':
				stackarray = ((stackarray + 1) % 2);
				/*@switchbreak@*/ break;
			case '!':
				negate = ((negate + 1) % 2);
				/*@switchbreak@*/ break;
			case '?':
				chkexist++;
				/*@switchbreak@*/ break;
			}
		}
		/* Find end-of-expansion, handle %{foo:bar} expansions. */
		for (fe = f; (c = (int) *fe) && !strchr(" :}", c);)
			fe++;
		switch (c) {
		case ':':
			g = fe + 1;
			ge = se - 1;
			/*@innerbreak@*/ break;
		case ' ':
			lastc = se-1;
			/*@innerbreak@*/ break;
		default:
			/*@innerbreak@*/ break;
		}
		/*@switchbreak@*/ break;
	}

	/* XXX Everything below expects fe > f */
	fn = (fe - f);
	gn = (ge - g);
	if ((fe - f) <= 0) {
/* XXX Process % in unknown context */
		c = (int) '%';	/* XXX only need to save % */
		SAVECHAR(mb, c);
#if 0
		rpmlog(RPMLOG_ERR,
			_("A %% is followed by an unparseable macro\n"));
#endif
		s = se;
		continue;
	}

	if (mb->macro_trace)
		printMacro(mb, s, se);

	/* Expand builtin macros */
	if (STREQ("load", f, fn)) {
		if (g != NULL) {
		    char * mfn = strncpy((char *) alloca(gn + 1), g, gn);
		    int xx;
		    mfn[gn] = '\0';
		    xx = rpmLoadMacroFile(NULL, mfn, _max_load_depth);
		    /* Print failure iff %{load:...} or %{!?load:...} */
		    if (xx != 0 && chkexist == negate)
			rpmlog(RPMLOG_ERR, _("%s: load macros failed\n"), mfn);
		}
		s = se;
		continue;
	}
	if (STREQ("global", f, fn)) {
		s = doDefine(mb, se, RMIL_GLOBAL, 1);
		continue;
	}
	if (STREQ("define", f, fn)) {
		s = doDefine(mb, se, mb->depth, 0);
		continue;
	}
	if (STREQ("undefine", f, fn)) {
		s = doUndefine(mb->mc, se);
		continue;
	}

	if (STREQ("echo", f, fn) ||
	    STREQ("warn", f, fn) ||
	    STREQ("error", f, fn)) {
		int waserror = 0;
		if (STREQ("error", f, fn))
			waserror = 1, rc = 1;
		if (g != NULL && g < ge)
			doOutput(mb, waserror, g, gn);
		else
			doOutput(mb, waserror, f, fn);
		s = se;
		continue;
	}

	if (STREQ("trace", f, fn)) {
		/* XXX TODO restore expand_trace/macro_trace to 0 on return */
		mb->expand_trace = mb->macro_trace = (negate ? 0 : mb->depth);
		if (mb->depth == 1) {
			print_macro_trace = mb->macro_trace;
			print_expand_trace = mb->expand_trace;
		}
		s = se;
		continue;
	}

	if (STREQ("dump", f, fn)) {
		rpmDumpMacroTable(mb->mc, NULL);
		while (iseol(*se))
			se++;
		s = se;
		continue;
	}

#ifdef	WITH_LUA
	if (STREQ("lua", f, fn)) {
		rpmlua lua = rpmluaGetGlobalState();
		rpmlua olua = (rpmlua) memcpy(alloca(sizeof(*olua)), lua, sizeof(*olua));
		char *scriptbuf = (char *)xmalloc(gn);
		const char *printbuf;

		/* Reset the stateful output buffer before recursing down. */
		lua->storeprint = 1;
		lua->printbuf = NULL;
		lua->printbufsize = 0;
		lua->printbufused = 0;

		if (g != NULL && gn > 0)
		    memcpy(scriptbuf, g, gn);
		scriptbuf[gn] = '\0';
		if (rpmluaRunScript(lua, scriptbuf, NULL) == -1)
		    rc = 1;
		printbuf = rpmluaGetPrintBuffer(lua);
		if (printbuf) {
		    size_t len = strlen(printbuf);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, printbuf, len);
		    mb->t += len;
		    mb->nb -= len;
		}

		/* Restore the stateful output buffer after recursion. */
		lua->storeprint = olua->storeprint;
		lua->printbuf = olua->printbuf;
		lua->printbufsize = olua->printbufsize;
		lua->printbufused = olua->printbufused;

		free(scriptbuf);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_AUGEAS
	if (STREQ("augeas", f, fn)) {
		/* XXX change rpmaugNew() to common embedded interpreter API */
#ifdef	NOTYET
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
#else
		char * script = strndup(g, (size_t)(se-g-1));
#endif
		rpmaug aug = (_globalI ? NULL
		    : rpmaugNew(_rpmaugRoot, _rpmaugLoadpath, _rpmaugFlags));
		const char * result = NULL;

		if (rpmaugRun(aug, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		aug = rpmaugFree(aug);
#ifdef	NOTYET
		av = _free(av);
#endif
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_FICL
	if (STREQ("ficl", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmficl ficl = rpmficlNew(av, _globalI);
		const char * result = NULL;

		if (rpmficlRun(ficl, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		ficl = rpmficlFree(ficl);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_LIBGIT2
	if (STREQ("git", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmgit git = rpmgitNew(av, _globalI, NULL);
		const char * result = NULL;

		if (rpmgitRun(git, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		git = rpmgitFree(git);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_GPSEE
	if (STREQ("js", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmjs js = rpmjsNew(av, _globalI);
		const char * result = NULL;

		if (rpmjsRun(js, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		js = rpmjsFree(js);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_JNIEMBED
	if (STREQ("jni", f, fn) || STREQ("java", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmjni jni = rpmjniNew(av, _globalI);
		const char * result = NULL;

		if (rpmjniRun(jni, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		jni = rpmjniFree(jni);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_MRBEMBED
	if (STREQ("mrb", f, fn) || STREQ("mruby", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmmrb mrb = rpmmrbNew(av, _globalI);
		const char * result = NULL;

		if (rpmmrbRun(mrb, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		mrb = rpmmrbFree(mrb);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_NIX
	if (STREQ("nix", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		int (*_vec) (rpmnix nix) = rpmnixEcho;
		uint32_t _flags = RPMNIX_FLAGS_NONE;
		rpmnix nix;
		const char * result = NULL;
		int xx;

		if (av == NULL || av[0] == NULL || av[1] == NULL
		 || !strcmp(av[0], "echo"))
		{
		    _vec = rpmnixEcho;
		} else
		if (!strcmp(av[1], "build")) {
		    _vec = rpmnixBuild;
		    _flags = RPMNIX_FLAGS_NOOUTLINK;
		} else
		if (!strcmp(av[1], "channel")) {
		    _vec = rpmnixChannel;
		} else
		if (!strcmp(av[1], "collect-garbage")) {
		    _vec = rpmnixCollectGarbage;
		} else
		if (!strcmp(av[1], "copy-closure")) {
		    _vec = rpmnixCopyClosure;
		} else
		if (!strcmp(av[1], "env")) {
		    _vec = rpmnixEnv;
		} else
		if (!strcmp(av[1], "hash")) {
		    _vec = rpmnixHash;
		} else
		if (!strcmp(av[1], "install-package")) {
		    _vec = rpmnixInstallPackage;
		    _flags = RPMNIX_FLAGS_INTERACTIVE;
		} else
		if (!strcmp(av[1], "instantiate")) {
		    _vec = rpmnixInstantiate;
		} else
		if (!strcmp(av[1], "prefetch-url")) {
		    _vec = rpmnixPrefetchURL;
		} else
		if (!strcmp(av[1], "push")) {
		    _vec = rpmnixPush;
		} else
		if (!strcmp(av[1], "pull")) {
		    _vec = rpmnixPull;
		} else
		if (!strcmp(av[1], "store")) {
		    _vec = rpmnixStore;
		} else
		if (!strcmp(av[1], "worker")) {
		    _vec = rpmnixWorker;
		} else
assert(0);

		nix = rpmnixNew(av, _flags, NULL);

#ifdef	NOTYET
		if (rpmnixRun(nix, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
#else
		xx = (*_vec) (nix);
		result = xstrdup("");
#endif	/* NOTYET */

		nix = rpmnixFree(nix);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_PERLEMBED
	if (STREQ("perl", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmperl perl = rpmperlNew(av, _globalI);
		const char * result = NULL;

		if (rpmperlRun(perl, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		perl = rpmperlFree(perl);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_PYTHONEMBED
	if (STREQ("python", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmpython python = rpmpythonNew(av, _globalI);
		const char * result = NULL;

		if (rpmpythonRun(python, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		python = rpmpythonFree(python);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_RUBYEMBED
	if (STREQ("ruby", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmruby ruby = rpmrubyNew(av, _globalI);
		const char * result = NULL;

		if (rpmrubyRun(ruby, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		ruby = rpmrubyFree(ruby);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_SEMANAGE
	if (STREQ("spook", f, fn)) {
		/* XXX change rpmsmNew() to common embedded interpreter API */
#ifdef	NOTYET
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
#else
		/* XXX use xstrndup (which never returns NULL) instead. */
		char * script = strndup(g, (size_t)(se-g-1));
		char * av[2];
		/* XXX FIXME */
		static const char * _rpmsmStore = "targeted";
		static unsigned int _rpmsmFlags = 0;
#endif
		rpmsm sm = (_globalI ? NULL
		    : rpmsmNew(_rpmsmStore, _rpmsmFlags));
		const char * result = NULL;

		/* XXX HACK: use an argv for now. */
		av[0] = script;
		av[1] = NULL;
		if (rpmsmRun(sm, av, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		 }
		}
		sm = rpmsmFree(sm);
#ifdef	NOTYET
		av = _free(av);
#endif
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_SQLITE
	if (STREQ("sql", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmsql sql = rpmsqlNew(av, _globalI);
		const char * result = NULL;

		if (rpmsqlRun(sql, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		sql = rpmsqlFree(sql);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_SQUIRREL
	if (STREQ("squirrel", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmsquirrel squirrel = rpmsquirrelNew(av, _globalI);
		const char * result = NULL;

		if (rpmsquirrelRun(squirrel, script, &result) != RPMRC_OK)
		    rc = 1;
		else {
		  if (result == NULL) result = _FIXME_embedded_interpreter_eval_returned_null;
		  if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		  }
		}
		squirrel = rpmsquirrelFree(squirrel);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

#ifdef	WITH_TCL
	if (STREQ("tcl", f, fn)) {
		char ** av = NULL;
		char * script = parseEmbedded(s, (size_t)(se-s), &av);
		rpmtcl tcl = rpmtclNew(av, _globalI);
		const char * result = NULL;

		if (rpmtclRun(tcl, script, &result) != RPMRC_OK)
		    rc = 1;
		else if (result != NULL && *result != '\0') {
		    size_t len = strlen(result);
		    if (len > mb->nb)
			len = mb->nb;
		    memcpy(mb->t, result, len);
		    mb->t += len;
		    mb->nb -= len;
		}
		tcl = rpmtclFree(tcl);
		av = _free(av);
		script = _free(script);
		s = se;
		continue;
	}
#endif

	/* Rewrite "%patchNN ..." as "%patch -P NN ..." and expand. */
	if (lastc && fn > 5 && STREQ("patch", f, 5) && xisdigit((int)f[5])) {
		/*@-internalglobs@*/ /* FIX: verbose may be set */
		doFoo(mb, negate, f, (lastc - f), NULL, 0);
		/*@=internalglobs@*/
		s = lastc;
		continue;
	}

	/* XXX necessary but clunky */
	if (STREQ("basename", f, fn) ||
	    STREQ("dirname", f, fn) ||
	    STREQ("realpath", f, fn) ||
	    STREQ("getenv", f, fn) ||
	    STREQ("shrink", f, fn) ||
	    STREQ("suffix", f, fn) ||
	    STREQ("expand", f, fn) ||
	    STREQ("verbose", f, fn) ||
	    STREQ("uncompress", f, fn) ||
	    STREQ("mkstemp", f, fn) ||
	    STREQ("mkdtemp", f, fn) ||
	    STREQ("uuid", f, fn) ||
	    STREQ("url2path", f, fn) ||
	    STREQ("u2p", f, fn) ||
	    STREQ("S", f, fn) ||
	    STREQ("P", f, fn) ||
	    STREQ("F", f, fn)) {
		/*@-internalglobs@*/ /* FIX: verbose may be set */
		doFoo(mb, negate, f, fn, g, gn);
		/*@=internalglobs@*/
		s = se;
		continue;
	}

	/* Expand defined macros */
	mep = findEntry(mb->mc, f, fn);
	me = (mep ? *mep : NULL);

	/* XXX Special processing for flags */
	if (*f == '-') {
		if (me)
			me->used++;	/* Mark macro as used */
		if ((me == NULL && !negate) ||	/* Without -f, skip %{-f...} */
		    (me != NULL && negate)) {	/* With -f, skip %{!-f...} */
			s = se;
			continue;
		}

		if (g && g < ge) {		/* Expand X in %{-f:X} */
			rc = expandT(mb, g, gn);
		} else
		if (me && me->body && *me->body) {/* Expand %{-f}/%{-f*} */
			rc = expandT(mb, me->body, strlen(me->body));
		}
		s = se;
		continue;
	}

	/* XXX Special processing for macro existence */
	if (chkexist) {
		if ((me == NULL && !negate) ||	/* Without -f, skip %{?f...} */
		    (me != NULL && negate)) {	/* With -f, skip %{!?f...} */
			s = se;
			continue;
		}
		if (g && g < ge) {		/* Expand X in %{?f:X} */
			rc = expandT(mb, g, gn);
		} else
		if (me && me->body && *me->body) { /* Expand %{?f}/%{?f*} */
			rc = expandT(mb, me->body, strlen(me->body));
		}
		s = se;
		continue;
	}

	if (me == NULL) {	/* leave unknown %... as is */
#if !defined(RPM_VENDOR_WINDRIVER_DEBUG)	/* XXX usually disabled */
#if DEAD
		/* XXX hack to skip over empty arg list */
		if (fn == 1 && *f == '*') {
			s = se;
			continue;
		}
#endif
		/* XXX hack to permit non-overloaded %foo to be passed */
		c = (int) '%';	/* XXX only need to save % */
		SAVECHAR(mb, c);
#else
		if (!strncmp(f, "if", fn) ||
		    !strncmp(f, "else", fn) ||
		    !strncmp(f, "endif", fn)) {
			c = '%';        /* XXX only need to save % */
			SAVECHAR(mb, c);
		} else {
			rpmlog(RPMLOG_ERR,
				_("Macro %%%.*s not found, skipping\n"), fn, f);
			s = se;
		}
#endif
		continue;
	}

	/* XXX Special processing to create a tuple from stack'd values. */
	if (stackarray) {
		if (!(g && g < ge)) {
			g = "\n";
			gn = strlen(g);
		}
		rc = expandFIFO(mb, me, g, gn);
		s = se;
		continue;
	}

	/* Setup args for "%name " macros with opts */
	if (me && me->opts != NULL) {
		if (lastc != NULL) {
			se = grabArgs(mb, me, fe, lastc);
		} else {
			addMacro(mb->mc, "**", NULL, "", mb->depth);
			addMacro(mb->mc, "*", NULL, "", mb->depth);
			addMacro(mb->mc, "#", NULL, "0", mb->depth);
			addMacro(mb->mc, "0", NULL, me->name, mb->depth);
		}
	}

	/* Recursively expand body of macro */
	if (me->body && *me->body) {
		mb->s = me->body;
		rc = expandMacro(mb);
		if (rc == 0)
			me->used++;	/* Mark macro as used */
	}

	/* Free args for "%name " macros with opts */
	if (me->opts != NULL)
		freeArgs(mb);

	s = se;
    }

    *mb->t = '\0';
    mb->s = s;
    mb->depth--;
    if (rc != 0 || mb->expand_trace)
	printExpansion(mb, t, mb->t);
    return rc;
}

#if defined(RPM_VENDOR_OPENPKG) /* stick-with-rpm-file-sanity-checking */ || \
    !defined(POPT_ERROR_BADCONFIG)	/* XXX POPT 1.15 retrofit */
int rpmSecuritySaneFile(const char *filename)
{
    struct stat sb;
    uid_t uid;

    if (stat(filename, &sb) == -1)
	return 1;
    uid = getuid();
    if (sb.st_uid != uid)
	return 0;
    if (!S_ISREG(sb.st_mode))
	return 0;
    if (sb.st_mode & (S_IWGRP|S_IWOTH))
	return 0;
    return 1;
}
#endif

#if !defined(DEBUG_MACROS)
/* =============================================================== */
/*@unchecked@*/
static int _debug = 0;

int rpmGlob(const char * patterns, int * argcPtr, const char *** argvPtr)
{
    int ac = 0;
    char ** av = NULL;
    int argc = 0;
    const char ** argv = NULL;
    char * globRoot = NULL;
#ifdef ENABLE_NLS
    const char * old_collate = NULL;
    const char * old_ctype = NULL;
    const char * t;
#endif
    size_t maxb, nb;
    size_t i;
    int j;
    int rc;

    rc = XpoptParseArgvString(patterns, &ac, &av);
    if (rc)
	return rc;
#ifdef ENABLE_NLS
    t = setlocale(LC_COLLATE, NULL);
    if (t)
	old_collate = xstrdup(t);
    t = setlocale(LC_CTYPE, NULL);
    if (t)
	old_ctype = xstrdup(t);
    (void) setlocale(LC_COLLATE, "C");
    (void) setlocale(LC_CTYPE, "C");
#endif
	
    if (av != NULL)
    for (j = 0; j < ac; j++) {
	const char * globURL;
	const char * path;
	int ut = urlPath(av[j], &path);
	glob_t gl;

	if (!Glob_pattern_p(av[j], 0) && strchr(path, '~') == NULL) {
	    argv = (const char **) xrealloc(argv, (argc+2) * sizeof(*argv));
	    argv[argc] = xstrdup(av[j]);
if (_debug)
fprintf(stderr, "*** rpmGlob argv[%d] \"%s\"\n", argc, argv[argc]);
	    argc++;
	    continue;
	}
	
	gl.gl_pathc = 0;
	gl.gl_pathv = NULL;
	rc = Glob(av[j], GLOB_TILDE, Glob_error, &gl);
	if (rc)
	    goto exit;

	/* XXX Prepend the URL leader for globs that have stripped it off */
	maxb = 0;
	for (i = 0; i < gl.gl_pathc; i++) {
	    if ((nb = strlen(&(gl.gl_pathv[i][0]))) > maxb)
		maxb = nb;
	}
	
	nb = ((ut == URL_IS_PATH) ? (path - av[j]) : 0);
	maxb += nb;
	maxb += 1;
	globURL = globRoot = (char *) xmalloc(maxb);

	switch (ut) {
	case URL_IS_PATH:
	case URL_IS_DASH:
	    strncpy(globRoot, av[j], nb);
	    /*@switchbreak@*/ break;
	case URL_IS_HKP:
	case URL_IS_FTP:
	case URL_IS_HTTP:
	case URL_IS_HTTPS:
	case URL_IS_MONGO:	/* XXX FIXME */
	case URL_IS_UNKNOWN:
	default:
	    /*@switchbreak@*/ break;
	}
	globRoot += nb;
	*globRoot = '\0';
if (_debug)
fprintf(stderr, "*** GLOB maxb %d diskURL %d %*s globURL %p %s\n", (int)maxb, (int)nb, (int)nb, av[j], globURL, globURL);
	
	argv = (const char **) xrealloc(argv, (argc+gl.gl_pathc+1) * sizeof(*argv));

	if (argv != NULL)
	for (i = 0; i < gl.gl_pathc; i++) {
	    const char * globFile = &(gl.gl_pathv[i][0]);
	    if (globRoot > globURL && globRoot[-1] == '/')
		while (*globFile == '/') globFile++;
	    strcpy(globRoot, globFile);
if (_debug)
fprintf(stderr, "*** rpmGlob argv[%d] \"%s\"\n", argc, globURL);
	    argv[argc++] = xstrdup(globURL);
	}
	/*@-immediatetrans@*/
	Globfree(&gl);
	/*@=immediatetrans@*/
	globURL = _free(globURL);
    }

    if (argv != NULL && argc > 0) {
	argv[argc] = NULL;
	if (argvPtr)
	    *argvPtr = argv;
	if (argcPtr)
	    *argcPtr = argc;
	rc = 0;
    } else
	rc = 1;


exit:
#ifdef ENABLE_NLS	
    if (old_collate) {
	(void) setlocale(LC_COLLATE, old_collate);
	old_collate = _free(old_collate);
    }
    if (old_ctype) {
	(void) setlocale(LC_CTYPE, old_ctype);
	old_ctype = _free(old_ctype);
    }
#endif
    av = _free(av);
    if (rc || argvPtr == NULL) {
/*@-dependenttrans -unqualifiedtrans@*/
	if (argv != NULL)
	for (j = 0; j < argc; j++)
	    argv[j] = _free(argv[j]);
	argv = _free(argv);
/*@=dependenttrans =unqualifiedtrans@*/
    }
    return rc;
}
#endif	/* !defined(DEBUG_MACROS) */

/* =============================================================== */

int
expandMacros(void * spec, MacroContext mc, char * sbuf, size_t slen)
{
    MacroBuf mb = (MacroBuf) alloca(sizeof(*mb));
    char *tbuf;
    int rc;

    if (sbuf == NULL || slen == 0)
	return 0;
    if (mc == NULL) mc = rpmGlobalMacroContext;

    tbuf = (char *) alloca(slen + 1);
    tbuf[0] = '\0';

    mb->s = sbuf;
    mb->t = tbuf;
    mb->nb = slen;
    mb->depth = 0;
    mb->macro_trace = print_macro_trace;
    mb->expand_trace = print_expand_trace;

    mb->spec = spec;	/* (future) %file expansion info */
    mb->mc = mc;

    rc = expandMacro(mb);

    tbuf[slen] = '\0';
    if (mb->nb == 0)
	rpmlog(RPMLOG_ERR, _("Macro expansion too big for target buffer\n"));
    else
	strncpy(sbuf, tbuf, (slen - mb->nb + 1));

    return rc;
}

void
addMacro(MacroContext mc,
	const char * n, const char * o, const char * b, int level)
{
    MacroEntry * mep;
    const char * name = n;

    if (*name == '.')		/* XXX readonly macros */
	name++;
    if (*name == '.')		/* XXX readonly macros */
	name++;

    if (mc == NULL) mc = rpmGlobalMacroContext;

    /* If new name, expand macro table */
    if ((mep = findEntry(mc, name, 0)) == NULL) {
	if (mc->firstFree == mc->macrosAllocated)
	    expandMacroTable(mc);
	if (mc->macroTable != NULL)
	    mep = mc->macroTable + mc->firstFree++;
    }

    if (mep != NULL) {
	/* XXX permit "..foo" to be pushed over ".foo" */
	if (*mep && (*mep)->flags && !(n[0] == '.' && n[1] == '.')) {
	    /* XXX avoid error message for %buildroot */
	    if (strcmp((*mep)->name, "buildroot"))
		rpmlog(RPMLOG_ERR, _("Macro '%s' is readonly and cannot be changed.\n"), n);
	    return;
	}
	/* Push macro over previous definition */
	pushMacro(mep, n, o, b, level);

	/* If new name, sort macro table */
	if ((*mep)->prev == NULL)
	    sortMacroTable(mc);
    }
}

void
delMacro(MacroContext mc, const char * n)
{
    MacroEntry * mep;

    if (mc == NULL) mc = rpmGlobalMacroContext;
    /* If name exists, pop entry */
    if ((mep = findEntry(mc, n, 0)) != NULL) {
	popMacro(mep);
	/* If deleted name, sort macro table */
	if (!(mep && *mep))
	    sortMacroTable(mc);
    }
}

/*@-mustmod@*/ /* LCL: mc is modified through mb->mc, mb is abstract */
int
rpmDefineMacro(MacroContext mc, const char * macro, int level)
{
    MacroBuf mb = (MacroBuf) alloca(sizeof(*mb));

    memset(mb, 0, sizeof(*mb));
    /* XXX just enough to get by */
    mb->mc = (mc ? mc : rpmGlobalMacroContext);
    (void) doDefine(mb, macro, level, 0);
    return 0;
}
/*@=mustmod@*/

/*@-mustmod@*/ /* LCL: mc is modified through mb->mc, mb is abstract */
int
rpmUndefineMacro(MacroContext mc, const char * macro)
{
    (void) doUndefine(mc ? mc : rpmGlobalMacroContext, macro);
    return 0;
}
/*@=mustmod@*/

void
rpmLoadMacros(MacroContext mc, int level)
{

    if (mc == NULL || mc == rpmGlobalMacroContext)
	return;

    if (mc->macroTable != NULL) {
	int i;
	for (i = 0; i < mc->firstFree; i++) {
	    MacroEntry *mep, me;
	    mep = &mc->macroTable[i];
	    me = *mep;

	    if (me == NULL)		/* XXX this should never happen */
		continue;
	    addMacro(NULL, me->name, me->opts, me->body, (level - 1));
	}
    }
}

#if defined(RPM_VENDOR_OPENPKG) /* expand-macrosfile-macro */
static void expand_macrosfile_macro(const char *file_name, const char *buf, size_t bufn)
{
    char *cp;
    size_t l, k;
    static const char *macro_name = "%{macrosfile}";

    l = strlen(macro_name);
    k = strlen(file_name);
    while ((cp = strstr(buf, macro_name)) != NULL) {
	if (((strlen(buf) - l) + k) < bufn) {
	    memmove(cp+k, cp+l, strlen(cp+l)+1);
	    memcpy(cp, file_name, k);
	}
    }
    return;
}
#endif

int
rpmLoadMacroFile(MacroContext mc, const char * fn, int nesting)
{
    size_t bufn = _macro_BUFSIZ;
    char *buf = (char *) alloca(bufn);
    int lineno = 0;
    int rc = -1;
    FD_t fd;
    int xx;

    /* XXX TODO: teach rdcl() to read through a URI, eliminate ".fpio". */
    fd = Fopen(fn, "r.fpio");
    if (fd == NULL || Ferror(fd)) {
	if (fd) (void) Fclose(fd);
	return rc;
    }

    /* XXX Assume new fangled macro expansion */
    /*@-mods@*/
    max_macro_depth = _MAX_MACRO_DEPTH;
    /*@=mods@*/

    buf[0] = '\0';
    while(rdcl(buf, bufn, fd) != NULL) {
	char * s;
	int c;

	lineno++;
	s = buf;
	SKIPBLANK(s, c);

	if (c != (int) '%')
	    continue;

	/* Parse %{load:...} immediately recursively. */
	if (s[1] == '{' && !strncmp(s+2, "load:", sizeof("load:")-1)) {
	    char * se = (char *) matchchar(s, '{', '}');
	    if (se == NULL) {
		rpmlog(RPMLOG_WARNING,
		    _("%s:%u Missing '}' in \"%s\", skipping.\n"),
		    fn, lineno, buf);
		continue;
	    }
	    s += sizeof("%{load:") - 1;
	    SKIPBLANK(s, c);
	    *se = '\0';
	    if (nesting <= 0) {
		rpmlog(RPMLOG_WARNING,
		    _("%s:%u load depth exceeded, \"%s\" ignored.\n"),
		    fn, lineno, buf);
		continue;
	    }
	    se = rpmMCExpand(mc, s, NULL);
	    rc = rpmLoadMacroFile(mc, se, nesting - 1);
	    se = _free(se);
	    if (rc != 0)
		goto exit;
	} else {
#if defined(RPM_VENDOR_OPENPKG) /* expand-macro-source */
	    expand_macrosfile_macro(fn, buf, bufn);
#endif
	    if (*s == '%') s++;
	    rc = rpmDefineMacro(mc, s, RMIL_MACROFILES);
	}
    }
    rc = 0;
exit:
    xx = Fclose(fd);
    return rc;
}

void
rpmInitMacros(MacroContext mc, const char * macrofiles)
{
    char *mfiles, *m, *me;

    if (macrofiles == NULL)
	return;
#ifdef	DYING
    if (mc == NULL) mc = rpmGlobalMacroContext;
#endif

    mfiles = xstrdup(macrofiles);
    for (m = mfiles; m && *m != '\0'; m = me) {
	const char ** av;
	int ac;
	int i;

	for (me = m; (me = strchr(me, ':')) != NULL; me++) {
	    /* Skip over URI's. */
	    if (!(me[1] == '/' && me[2] == '/'))
		/*@innerbreak@*/ break;
	}

	if (me && *me == ':')
	    *me++ = '\0';
	else
	    me = m + strlen(m);

	/* Glob expand the macro file path element, expanding ~ to $HOME. */
	ac = 0;
	av = NULL;
#if defined(DEBUG_MACROS)
	ac = 1;
	av = xmalloc((ac + 1) * sizeof(*av));
	av[0] = strdup(m);
	av[1] = NULL;
#else
	i = rpmGlob(m, &ac, &av);
	if (i != 0)
	    continue;
#endif

	/* Read macros from each file. */

	for (i = 0; i < ac; i++) {
	    size_t slen = strlen(av[i]);
	    const char *fn = av[i];

	if (fn[0] == '@' /* attention */) {
	    fn++;
#if defined(RPM_VENDOR_OPENPKG) /* stick-with-rpm-file-sanity-checking */ || \
    !defined(POPT_ERROR_BADCONFIG)	/* XXX POPT 1.15 retrofit */
	    if (!rpmSecuritySaneFile(fn))
#else
	    if (!poptSaneFile(fn))
#endif
	    {
		rpmlog(RPMLOG_WARNING, "existing RPM macros file \"%s\" considered INSECURE -- not loaded\n", fn);
		/*@innercontinue@*/ continue;
	    }
	}

	/* Skip backup files and %config leftovers. */
#define	_suffix(_s, _x) \
    (slen >= sizeof(_x) && !strcmp((_s)+slen-(sizeof(_x)-1), (_x)))
	    if (!(_suffix(fn, "~")
	       || _suffix(fn, ".rpmnew")
	       || _suffix(fn, ".rpmorig")
	       || _suffix(fn, ".rpmsave"))
	       )
		   (void) rpmLoadMacroFile(mc, fn, _max_load_depth);
#undef _suffix

	    av[i] = _free(av[i]);
	}
	av = _free(av);
    }
    mfiles = _free(mfiles);

    /* Reload cmdline macros */
    /*@-mods@*/
    rpmLoadMacros(rpmCLIMacroContext, RMIL_CMDLINE);
    /*@=mods@*/
}

/*@-globstate@*/
void
rpmFreeMacros(MacroContext mc)
{
    
    if (mc == NULL) mc = rpmGlobalMacroContext;

    if (mc->macroTable != NULL) {
	int i;
	for (i = 0; i < mc->firstFree; i++) {
	    MacroEntry me;
	    while ((me = mc->macroTable[i]) != NULL) {
		/* XXX cast to workaround const */
		/*@-onlytrans@*/
		if ((mc->macroTable[i] = me->prev) == NULL)
		    me->name = _free(me->name);
		/*@=onlytrans@*/
		me->opts = _free(me->opts);
		me->body = _free(me->body);
		if (me) free(me);
		me = NULL;
	    }
	}
	free(mc->macroTable);
	mc->macroTable = NULL;
    }
    memset(mc, 0, sizeof(*mc));
}
/*@=globstate@*/

/* =============================================================== */
int isCompressed(const char * file, rpmCompressedMagic * compressed)
{
    FD_t fd;
    ssize_t nb;
    int rc = -1;
    unsigned char magic[13];
#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_FEDORA) || defined(RPM_VENDOR_MANDRIVA) /* extension-based-compression-detection */
    size_t file_len;
#endif

    *compressed = COMPRESSED_NOT;

#if defined(RPM_VENDOR_OPENPKG) || defined(RPM_VENDOR_FEDORA) || defined(RPM_VENDOR_MANDRIVA) /* extension-based-compression-detection */
    file_len = strlen(file);
    if ((file_len > 4 && strcasecmp(file+file_len-4, ".tbz") == 0)
     || (file_len > 4 && strcasecmp(file+file_len-4, ".bz2") == 0)) {
	*compressed = COMPRESSED_BZIP2;
	return 0;
    } else
    if (file_len > 4 && strcasecmp(file+file_len-4, ".zip") == 0) {
	*compressed = COMPRESSED_ZIP;
	return 0;
    } else
    if ((file_len > 4 && strcasecmp(file+file_len-4, ".tlz") == 0)
     || (file_len > 5 && strcasecmp(file+file_len-5, ".lzma") == 0)) {
	*compressed = COMPRESSED_LZMA;
	return 0;
    } else
    if (file_len > 4 && strcasecmp(file+file_len-3, ".xz") == 0) {
	*compressed = COMPRESSED_XZ;
	return 0;
    } else
    if ((file_len > 4 && strcasecmp(file+file_len-4, ".tgz") == 0)
     || (file_len > 3 && strcasecmp(file+file_len-3, ".gz") == 0)
     || (file_len > 2 && strcasecmp(file+file_len-2, ".Z") == 0)) {
	*compressed = COMPRESSED_OTHER;
	return 0;
    } else
    if (file_len > 5 && strcasecmp(file+file_len-5, ".cpio") == 0) {
	*compressed = COMPRESSED_NOT;
	return 0;
    } else
    if (file_len > 4 && strcasecmp(file+file_len-4, ".tar") == 0) {
	*compressed = COMPRESSED_NOT;
	return 0;
    }
#endif

    fd = Fopen(file, "r");
    if (fd == NULL || Ferror(fd)) {
	/* XXX Fstrerror */
	rpmlog(RPMLOG_ERR, _("File %s: %s\n"), file, Fstrerror(fd));
	if (fd) (void) Fclose(fd);
	return 1;
    }
    nb = Fread(magic, sizeof(magic[0]), sizeof(magic), fd);
    if (nb < (ssize_t)0) {
	rpmlog(RPMLOG_ERR, _("File %s: %s\n"), file, Fstrerror(fd));
	rc = 1;
    } else if (nb < (ssize_t)sizeof(magic)) {
	rpmlog(RPMLOG_ERR, _("File %s is smaller than %u bytes\n"),
		file, (unsigned)sizeof(magic));
	rc = 0;
    }
    (void) Fclose(fd);
    if (rc >= 0)
	return rc;

    rc = 0;

    if (magic[0] == 'B' && magic[1] == 'Z')
	*compressed = COMPRESSED_BZIP2;
    else
    if (magic[0] == (unsigned char) 0120 && magic[1] == (unsigned char) 0113
     &&	magic[2] == (unsigned char) 0003 && magic[3] == (unsigned char) 0004)	/* pkzip */
	*compressed = COMPRESSED_ZIP;
    else
    if (magic[0] == (unsigned char) 0x89 && magic[1] == 'L'
     &&	magic[2] == 'Z' && magic[3] == 'O')	/* lzop */
	*compressed = COMPRESSED_LZOP;
    else
#if !defined(RPM_VENDOR_OPENPKG) && !defined(RPM_VENDOR_FEDORA) && !defined(RPM_VENDOR_MANDRIVA) /* extension-based-compression-detection */
    /* XXX Ick, LZMA has no magic. See http://lkml.org/lkml/2005/6/13/285 */
    if (magic[ 9] == (unsigned char) 0x00 && magic[10] == (unsigned char) 0x00 &&
	magic[11] == (unsigned char) 0x00 && magic[12] == (unsigned char) 0x00)	/* lzmash */
	*compressed = COMPRESSED_LZMA;
    else
#endif
#if defined(RPM_VENDOR_OPENSUSE)
    if (magic[0] == 0135 && magic[1] == 0 && magic[2] == 0)		/* lzma */
	*compressed = COMPRESSED_LZMA;
    else
#endif
    if (magic[0] == (unsigned char) 0xFD && magic[1] == 0x37 &&	magic[2] == 0x7A
     && magic[3] == 0x58 && magic[4] == 0x5A && magic[5] == 0x00)		/* xz */
	*compressed = COMPRESSED_XZ;
    else if ((magic[0] == 'L') && (magic[1] == 'Z') &&
	       (magic[2] == 'I') && (magic[3] == 'P'))	/* lzip */
	*compressed = COMPRESSED_LZIP;
    else if ((magic[0] == 'L') && (magic[1] == 'R') &&
	       (magic[2] == 'Z') && (magic[3] == 'I')) 		/* lrzip */
	*compressed = COMPRESSED_LRZIP;
    else if ((magic[0] == '7') && (magic[1] == 'z') &&
               (magic[2] == 0xbc) && (magic[3] == 0xaf) &&
               (magic[4] == 0x27) && (magic[5] == 0x1c))	/* 7zip */
	*compressed = COMPRESSED_7ZIP;
    else
    if ((magic[0] == (unsigned char) 0037 && magic[1] == (unsigned char) 0213)	/* gzip */
     ||	(magic[0] == (unsigned char) 0037 && magic[1] == (unsigned char) 0236)	/* old gzip */
     ||	(magic[0] == (unsigned char) 0037 && magic[1] == (unsigned char) 0036)	/* pack */
     ||	(magic[0] == (unsigned char) 0037 && magic[1] == (unsigned char) 0240)	/* SCO lzh */
     ||	(magic[0] == (unsigned char) 0037 && magic[1] == (unsigned char) 0235))	/* compress */
	*compressed = COMPRESSED_OTHER;

    return rc;
}

/* =============================================================== */

/*@-modfilesys@*/
/* XXX TODO: merge rpmExpand and rpmMCExpand. gud enuf for now ... */
char * 
rpmExpand(const char *arg, ...)
{
    MacroContext mc = NULL;
    const char *s;
    char *t, *te;
    size_t sn, tn;
    size_t bufn = 8 * _macro_BUFSIZ;

    va_list ap;

    if (arg == NULL)
	return xstrdup("");

    t = (char *) xmalloc(bufn + strlen(arg) + 1);
    *t = '\0';
    te = stpcpy(t, arg);

    va_start(ap, arg);
    while ((s = va_arg(ap, const char *)) != NULL) {
	sn = strlen(s);
	tn = (te - t);
	t = (char *) xrealloc(t, tn + sn + bufn + 1);
	te = t + tn;
	te = stpcpy(te, s);
    }
    va_end(ap);

    *te = '\0';
    tn = (te - t);
    (void) expandMacros(NULL, mc, t, tn + bufn + 1);
    t[tn + bufn] = '\0';
    t = (char *) xrealloc(t, strlen(t) + 1);
    
    return t;
}

char * 
rpmMCExpand(MacroContext mc, const char *arg, ...)
{
    const char *s;
    char *t, *te;
    size_t sn, tn;
    size_t bufn = 8 * _macro_BUFSIZ;

    va_list ap;

    if (arg == NULL)
	return xstrdup("");

    t = (char *) xmalloc(bufn + strlen(arg) + 1);
    *t = '\0';
    te = stpcpy(t, arg);

    va_start(ap, arg);
    while ((s = va_arg(ap, const char *)) != NULL) {
	sn = strlen(s);
	tn = (te - t);
	t = (char *) xrealloc(t, tn + sn + bufn + 1);
	te = t + tn;
	te = stpcpy(te, s);
    }
    va_end(ap);

    *te = '\0';
    tn = (te - t);
    (void) expandMacros(NULL, mc, t, tn + bufn + 1);
    t[tn + bufn] = '\0';
    t = (char *) xrealloc(t, strlen(t) + 1);
    
    return t;
}
/*@=modfilesys@*/

int
rpmExpandNumeric(const char *arg)
{
    const char *val;
    int rc;

    if (arg == NULL)
	return 0;

    val = rpmExpand(arg, NULL);
    if (!(val && *val != '%'))
	rc = 0;
    else if (*val == 'Y' || *val == 'y')
	rc = 1;
    else if (*val == 'N' || *val == 'n')
	rc = 0;
    else {
	char *end;
	rc = strtol(val, &end, 0);
	if (!(end && *end == '\0'))
	    rc = 0;
    }
    val = _free(val);

    return rc;
}

/* @todo "../sbin/./../bin/" not correct. */
char *rpmCleanPath(char * path)
{
    const char *s;
    char *se, *t, *te;
    int begin = 1;

    if (path == NULL)
	return NULL;

/*fprintf(stderr, "*** RCP %s ->\n", path); */
    s = t = te = path;
    while (*s != '\0') {
/*fprintf(stderr, "*** got \"%.*s\"\trest \"%s\"\n", (t-path), path, s); */
	switch(*s) {
	case ':':			/* handle url's */
	    if (s[1] == '/' && s[2] == '/') {
		*t++ = *s++;
		*t++ = *s++;
		/* XXX handle "file:///" */
		if (s[0] == '/') *t++ = *s++;
		te = t;
		/*@switchbreak@*/ break;
	    }
	    begin=1;
	    /*@switchbreak@*/ break;
	case '/':
	    /* Move parent dir forward */
	    for (se = te + 1; se < t && *se != '/'; se++)
		{};
	    if (se < t && *se == '/') {
		te = se;
/*fprintf(stderr, "*** next pdir \"%.*s\"\n", (te-path), path); */
	    }
	    while (s[1] == '/')
		s++;
	    while (t > te && t[-1] == '/')
		t--;
	    /*@switchbreak@*/ break;
	case '.':
	    /* Leading .. is special */
 	    /* Check that it is ../, so that we don't interpret */
	    /* ..?(i.e. "...") or ..* (i.e. "..bogus") as "..". */
	    /* in the case of "...", this ends up being processed*/
	    /* as "../.", and the last '.' is stripped.  This   */
	    /* would not be correct processing.                 */
	    if (begin && s[1] == '.' && (s[2] == '/' || s[2] == '\0')) {
/*fprintf(stderr, "    leading \"..\"\n"); */
		*t++ = *s++;
		/*@switchbreak@*/ break;
	    }
	    /* Single . is special */
	    if (begin && s[1] == '\0') {
		/*@switchbreak@*/ break;
	    }
	    if (t > path && t[-1] == '/')
	    switch (s[1]) {
	    case '/':	s++;	/*@fallthrough@*/	/* Trim embedded ./ */
	    case '\0':	s++;	continue;		/* Trim trailing /. */
	    default:	/*@innerbreak@*/ break;
	    }
	    /* Trim embedded /../ and trailing /.. */
	    if (!begin && t > path && t[-1] == '/' && s[1] == '.' && (s[2] == '/' || s[2] == '\0')) {
		t = te;
		/* Move parent dir forward */
		if (te > path)
		    for (--te; te > path && *te != '/'; te--)
			{};
/*fprintf(stderr, "*** prev pdir \"%.*s\"\n", (te-path), path); */
		s++;
		s++;
		continue;
	    }
	    /*@switchbreak@*/ break;
	default:
	    begin = 0;
	    /*@switchbreak@*/ break;
	}
	*t++ = *s++;
    }

    /* Trim trailing / (but leave single / alone) */
    if (t > &path[1] && t[-1] == '/')
	t--;
    *t = '\0';

/*fprintf(stderr, "\t%s\n", path); */
    return path;
}

/* Return concatenated and expanded canonical path. */

char *
rpmGetPath(const char *path, ...)
{
    size_t bufn = _macro_BUFSIZ;
    char *buf = (char *) alloca(bufn);
    const char * s;
    char * t, * te;
    int slashed = 0;
    va_list ap;

    if (path == NULL)
	return xstrdup("");

    buf[0] = '\0';
    t = buf;
    te = stpcpy(t, path);
    *te = '\0';

    va_start(ap, path);
    while ((s = va_arg(ap, const char *)) != NULL) {
	/* Specifically requested pesky trailing '/'? */
	slashed = (s[0] == '/' && s[1] == '\0');
	te = stpcpy(te, s);
    }
    va_end(ap);
    *te = '\0';

/*@-modfilesys@*/
    (void) expandMacros(NULL, NULL, buf, bufn);
/*@=modfilesys@*/

    /* Note: rpmCleanPath will strip pesky trailing '/'. */
    (void) rpmCleanPath(buf);

    /* Re-append specifically requested pesky trailing '/'. */
    if (slashed) {
	size_t nb = strlen(buf);
	if (buf[nb-1] != '/')
	    buf[nb++] = '/';
	buf[nb] = '\0';
    }

    return DRD_xstrdup(buf);	/* XXX xstrdup has side effects. */
}

/* Merge 3 args into path, any or all of which may be a url. */

const char * rpmGenPath(const char * urlroot, const char * urlmdir,
		const char *urlfile)
{
/*@owned@*/ const char * xroot = rpmGetPath(urlroot, NULL);
/*@dependent@*/ const char * root = xroot;
/*@owned@*/ const char * xmdir = rpmGetPath(urlmdir, NULL);
/*@dependent@*/ const char * mdir = xmdir;
/*@owned@*/ const char * xfile = rpmGetPath(urlfile, NULL);
/*@dependent@*/ const char * file = xfile;
    const char * result;
    const char * url = NULL;
    size_t nurl = 0;
    int ut;

#if 0
if (_debug) fprintf(stderr, "*** RGP xroot %s xmdir %s xfile %s\n", xroot, xmdir, xfile);
#endif
    ut = urlPath(xroot, &root);
    if (url == NULL && ut > URL_IS_DASH) {
	url = xroot;
	nurl = strlen(url);
	if (root >= url && root <= url+nurl)
	    nurl -= strlen(root);
#if 0
if (_debug) fprintf(stderr, "*** RGP ut %d root %s nurl %u\n", ut, root, (unsigned)nurl);
#endif
    }
    if (root == NULL || *root == '\0') root = "/";

    ut = urlPath(xmdir, &mdir);
    if (url == NULL && ut > URL_IS_DASH) {
	url = xmdir;
	nurl = strlen(url);
	if (mdir >= url && mdir <= url+nurl)
	    nurl -= strlen(mdir);
#if 0
if (_debug) fprintf(stderr, "*** RGP ut %d mdir %s nurl %u\n", ut, mdir, (unsigned)nurl);
#endif
    }
    if (mdir == NULL || *mdir == '\0') mdir = "/";

    ut = urlPath(xfile, &file);
    if (url == NULL && ut > URL_IS_DASH) {
	url = xfile;
	nurl = strlen(url);
	if (file >= url && file <= url+nurl)
	    nurl -= strlen(file);
#if 0
if (_debug) fprintf(stderr, "*** RGP ut %d file %s nurl %u\n", ut, file, (unsigned)nurl);
#endif
    }

    if (url && nurl > 0) {
	char *t = strncpy((char *)alloca(nurl+1), url, nurl);
	t[nurl] = '\0';
	url = t;
    } else
	url = "";

    result = rpmGetPath(url, root, "/", mdir, "/", file, NULL);

    xroot = _free(xroot);
    xmdir = _free(xmdir);
    xfile = _free(xfile);
#if 0
if (_debug) fprintf(stderr, "*** RGP result %s\n", result);
#endif
    return result;
}

/* =============================================================== */

#if defined(DEBUG_MACROS)

#if defined(EVAL_MACROS)

const char *rpmMacrofiles = MACROFILES;

int
main(int argc, char *argv[])
{
    int c;
    int errflg = 0;
    extern char *optarg;
    extern int optind;

    while ((c = getopt(argc, argv, "f:")) != EOF ) {
	switch (c) {
	case 'f':
	    rpmMacrofiles = optarg;
	    break;
	case '?':
	default:
	    errflg++;
	    break;
	}
    }
    if (errflg || optind >= argc) {
	fprintf(stderr, "Usage: %s [-f macropath ] macro ...\n", argv[0]);
	exit(1);
    }

    rpmInitMacros(NULL, rpmMacrofiles);
    /* XXX getopt(3) also used for parametrized macros, expect scwewiness. */
    for ( ; optind < argc; optind++) {
	const char *val;

	val = rpmExpand(argv[optind], NULL);
	if (val) {
	    fprintf(stdout, "%s:\t%s\n", argv[optind], val);
	    val = _free(val);
	}
    }
    rpmFreeMacros(NULL);
    return 0;
}

#else	/* !EVAL_MACROS */

const char *rpmMacrofiles = "../macros:./testmacros";
const char *testfile = "./test";

int
main(int argc, char *argv[])
{
    size_t bufn = _macro_BUFSIZ;
    char *buf = (char *) alloca(bufn);
    FILE *fp;
    int x;

    rpmInitMacros(NULL, rpmMacrofiles);

    if ((fp = fopen(testfile, "r")) != NULL) {
	while(rdcl(buf, bufn, fp)) {
	    x = expandMacros(NULL, NULL, buf, bufn);
	    fprintf(stderr, "%d->%s\n", x, buf);
	    memset(buf, 0, bufn);
	}
	fclose(fp);
    }

    while(rdcl(buf, bufn, stdin)) {
	x = expandMacros(NULL, NULL, buf, bufn);
	fprintf(stderr, "%d->%s\n <-\n", x, buf);
	memset(buf, 0, bufn);
    }
    rpmFreeMacros(NULL);

    return 0;
}
#endif	/* EVAL_MACROS */
#endif	/* DEBUG_MACROS */
