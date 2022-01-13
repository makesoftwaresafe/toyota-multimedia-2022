/** \ingroup rpmbuild
 * \file build/spec.c
 * Handle spec data structure.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmpgp.h>

#include "buildio.h"
#include "rpmds.h"
#include "rpmfi.h"
#include "rpmts.h"

#include "rpmlua.h"

#include "debug.h"

/*@unchecked@*/
int _pkg_debug;

/*@unchecked@*/
int _spec_debug;

/*@-redecl@*/
extern int specedit;
/*@=redecl@*/

#define SKIPWHITE(_x)	{while(*(_x) && (xisspace(*_x) || *(_x) == ',')) (_x)++;}
#define SKIPNONWHITE(_x){while(*(_x) &&!(xisspace(*_x) || *(_x) == ',')) (_x)++;}

/*@access rpmluav @*/

/**
 * @param p		trigger entry chain
 * @return		NULL always
 */
static inline
/*@null@*/ struct TriggerFileEntry * freeTriggerFiles(/*@only@*/ /*@null@*/ struct TriggerFileEntry * p)
	/*@modifies p @*/
{
    struct TriggerFileEntry *o, *q = p;
    
    while (q != NULL) {
	o = q;
	q = q->next;
	o->fileName = _free(o->fileName);
	o->script = _free(o->script);
	o->prog = _free(o->prog);
	o = _free(o);
    }
    return NULL;
}

/**
 * Destroy source component chain.
 * @param s		source component chain
 * @return		NULL always
 */
static inline
/*@null@*/ struct Source * freeSources(/*@only@*/ /*@null@*/ struct Source * s)
	/*@modifies s @*/
{
    struct Source *r, *t = s;

    while (t != NULL) {
	r = t;
	t = t->next;
	r->fullSource = _free(r->fullSource);
	r = _free(r);
    }
    return NULL;
}

rpmRC lookupPackage(Spec spec, const char *name, int flag, /*@out@*/Package *pkgp)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    char *NV = NULL;
    char *N = NULL;
    char *V = NULL;
    Package p;
    Package lastp = spec->packages;
    rpmRC rc = RPMRC_OK;
    int xx;
    
    if (lastp == NULL)	/* XXX segfault avoidance */
	goto exit;
    /* "main" package */
    if (name == NULL)
	goto exit;

    /* Construct package name */
    if (flag == PART_SUBNAME) {
	he->tag = RPMTAG_NAME;
	xx = headerGet(spec->packages->header, he, 0);
assert(xx != 0 && he->p.str != NULL);
	N = rpmExpand(he->p.str, "-", name, NULL);
	he->p.ptr = _free(he->p.ptr);
    } else {
	N = xstrdup(name);
	/* XXX restrict V to leading digit to prevent NV split ambiguity. */
	if ((V = strrchr(N, '-')) != NULL && xisdigit(V[1])) {
	    NV = xstrdup(N);
	    *V++ = '\0';
	} else
	    V = NULL;
    }

    /* Match last package with same N or same {N,V} */
    lastp = NULL;
    for (p = spec->packages; p != NULL; p = p->next) {
	char *nv, *n, *v;
	nv = n = v = NULL;
	he->tag = RPMTAG_NAME;
	xx = headerGet(p->header, he, 0);
	if (xx && he->p.str != NULL) {
	    n = (char *) he->p.str;
	    he->p.str = NULL;
	}
	if (NV != NULL) {
	    he->tag = RPMTAG_VERSION;
	    xx = headerGet(p->header, he, 0);
	    if (xx && he->p.str != NULL) {
		v = (char *) he->p.str;
		he->p.str = NULL;
		nv = rpmExpand(n, "-", v, NULL);
	    }
	}

	if (NV == NULL) {
	    if ((n && !strcmp(N, n)))
		lastp = p;
	} else {
	    if ((nv && !strcmp(NV, nv)) || (n && !strcmp(NV, n))
	    || ((n && !strcmp(N, n)) && (V == NULL || (v && !strcmp(V, v)))))
		lastp = p;
	}
/*@-usereleased@*/
	n = _free(n);
	v = _free(v);
	nv = _free(nv);
/*@=usereleased@*/
    }
    rc = (lastp == NULL ? RPMRC_FAIL : RPMRC_OK);
    NV = _free(NV);
    N = _free(N);

exit:
    if (pkgp)
	/*@-dependenttrans@*/ *pkgp = lastp; /*@=dependenttrans@*/
    return rc;
}

static void pkgFini(void * _pkg)
	/*@modifies _pkg @*/
{
    Package pkg = _pkg;

    if (pkg == NULL) return;	/* XXX assert? */
    
    pkg->preInFile = _free(pkg->preInFile);
    pkg->postInFile = _free(pkg->postInFile);
    pkg->preUnFile = _free(pkg->preUnFile);
    pkg->postUnFile = _free(pkg->postUnFile);
    pkg->verifyFile = _free(pkg->verifyFile);
    pkg->sanityCheckFile = _free(pkg->sanityCheckFile);

    (void)headerFree(pkg->header);
    pkg->header = NULL;
    (void)rpmdsFree(pkg->ds);
    pkg->ds = NULL;
    pkg->fileList = rpmiobFree(pkg->fileList);
    pkg->fileFile = _free(pkg->fileFile);
    if (pkg->fi != NULL) {
	rpmfi fi = pkg->fi;
	pkg->fi = NULL;
	fi = rpmfiFree(fi);
    }

    pkg->specialDoc = rpmiobFree(pkg->specialDoc);
    pkg->triggerFiles = freeTriggerFiles(pkg->triggerFiles);
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _pkgPool;

static Package pkgGetPool(rpmioPool pool)
{
    Package pkg;

    if (_pkgPool == NULL) {
	_pkgPool = rpmioNewPool("pkg", sizeof(*pkg), -1, _pkg_debug,
			NULL, NULL, pkgFini);
	pool = _pkgPool;
    }
    pkg = (Package) rpmioGetPool(pool, sizeof(*pkg));
    memset(((char *)pkg)+sizeof(pkg->_item), 0, sizeof(*pkg)-sizeof(pkg->_item));
    return pkg;
}

Package newPackage(/*@unused@*/ Spec spec)
{
    Package pkg = pkgGetPool(_pkgPool);

    pkg->header = headerNew();
    pkg->ds = NULL;

    pkg->autoProv = ((_rpmbuildFlags & 0x1) != 0);
    pkg->autoReq = ((_rpmbuildFlags & 0x2) != 0);
    
#if 0    
    pkg->reqProv = NULL;
    pkg->triggers = NULL;
    pkg->triggerScripts = NULL;
#endif

    pkg->triggerFiles = NULL;
    
    pkg->fileFile = NULL;
    pkg->fileList = NULL;

    pkg->fi = NULL;

    pkg->preInFile = NULL;
    pkg->postInFile = NULL;
    pkg->preUnFile = NULL;
    pkg->postUnFile = NULL;
    pkg->verifyFile = NULL;
    pkg->sanityCheckFile = NULL;

    pkg->specialDoc = NULL;

    pkg->next = NULL;

    return (Package)rpmioLinkPoolItem((rpmioItem)pkg, __FUNCTION__, __FILE__, __LINE__);
}

Package freePackages(Package packages)
{
    Package p;

    while ((p = packages) != NULL) {
	packages = p->next;
	p->next = NULL;
	p = freePackage(p);
    }
    return NULL;
}

/**
 */
static inline /*@owned@*/ struct Source *findSource(Spec spec, rpmuint32_t num, int flag)
	/*@*/
{
    struct Source *p;

    for (p = spec->sources; p != NULL; p = p->next)
	if ((num == p->num) && (p->flags & flag)) return p;

    return NULL;
}

/**
 */
int SpecSourceCount(Spec spec)
{
    return spec->numSources;
}

/**
 */
SpecSource getSource(Spec spec, int num)
    /* @ */
{
    struct Source *p = spec->sources;
    int i;

    for (i = 0; i < num; i++)
	if ((p = p->next) == NULL) return NULL;

/*@-usereleased@*/
    return p;
/*@=usereleased@*/
}

/**
 */
const char * specSourceName(SpecSource source)
{
    return source->source;
}

/**
 */
const char * specFullSourceName(SpecSource source)
{
    return source->fullSource;
}

/**
 */
int specSourceNum(SpecSource source)
{
    return source->num;
}

/**
 */
int specSourceFlags(SpecSource source)
{
    return source->flags;
}

int parseNoSource(Spec spec, const char * field, rpmTag tag)
{
    const char *f, *fe;
    const char *name;
    rpmuint32_t num, flag;

    if (tag == RPMTAG_NOSOURCE) {
	flag = RPMFILE_SOURCE;
	name = "source";
    } else {
	flag = RPMFILE_PATCH;
	name = "patch";
    }
    
    fe = field;
    for (f = fe; *f != '\0'; f = fe) {
	struct Source *p;

	SKIPWHITE(f);
	if (*f == '\0')
	    break;
	fe = f;
	SKIPNONWHITE(fe);
	if (*fe != '\0') fe++;

	if (parseNum(f, &num)) {
	    rpmlog(RPMLOG_ERR, _("line %d: Bad number: %s\n"),
		     spec->lineNum, f);
	    return RPMRC_FAIL;
	}

	if (! (p = findSource(spec, num, flag))) {
	    rpmlog(RPMLOG_ERR, _("line %d: Bad no%s number: %d\n"),
		     spec->lineNum, name, num);
	    return RPMRC_FAIL;
	}

	p->flags |= RPMFILE_GHOST;

    }

    return RPMRC_OK;
}

int addSource(Spec spec, /*@unused@*/ Package pkg,
		const char *field, rpmTag tag)
{
    struct Source *p;
#if defined(RPM_VENDOR_OPENPKG) /* regular-ordered-sources */
    struct Source *p_last;
#endif
    int flag = 0;
    const char *name = NULL;
    const char *mdir = NULL;
    const char *fieldp = NULL;
    char buf[BUFSIZ];
    uint32_t num = 0;

    buf[0] = '\0';
    switch (tag) {
    case RPMTAG_SOURCE:
	flag = RPMFILE_SOURCE;
	name = "source";
	fieldp = spec->line + strlen(name);
	break;
    case RPMTAG_PATCH:
	flag = RPMFILE_PATCH;
	name = "patch";
	fieldp = spec->line + strlen(name);
	break;
    case RPMTAG_ICON:
	flag = RPMFILE_ICON;
	name = "icon";
	fieldp = NULL;
	break;
    default:
assert(0);
	/*@notreached@*/ break;
    }
#if !defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    mdir = getSourceDir(flag);
assert(mdir != NULL);
#endif

    /* Get the number */
    if (fieldp != NULL) {
	char * end = NULL;

	num = strtoul(fieldp, &end, 10);
	SKIPSPACE(end);
	if (*end != ':') {
	    rpmlog(RPMLOG_ERR, _("line %d: No ':' terminator: %s\n"),
			 spec->lineNum, spec->line);
	    return RPMRC_FAIL;
	}
    }

    /* Check whether tags of the same number haven't already been defined */
    for (p = spec->sources; p != NULL; p = p->next) {
	if ( p->num != num ) continue;
	if ((tag == RPMTAG_SOURCE && p->flags == RPMFILE_SOURCE) ||
	    (tag == RPMTAG_PATCH  && p->flags == RPMFILE_PATCH)) {
		rpmlog(RPMLOG_ERR, _("%s %d defined multiple times\n"), name, num);
		return RPMRC_FAIL;
	    }
    }

    /* Create the entry and link it in */
    p = xmalloc(sizeof(*p));
    p->num = num;
    p->fullSource = xstrdup(field);
    p->flags = flag;
    p->source = strrchr(p->fullSource, '/');
    if (p->source)
	p->source++;
    else
	p->source = p->fullSource;

#if defined(RPM_VENDOR_OPENPKG) /* regular-ordered-sources */
    p->next = NULL;
    p_last = spec->sources;
    while (p_last != NULL && p_last->next != NULL)
        p_last = p_last->next;
    if (p_last != NULL)
        p_last->next = p;
    else
        spec->sources = p;
#else
    p->next = spec->sources;
    spec->sources = p;
#endif

    spec->numSources++;

    /* XXX FIXME: need to add ICON* macros. */
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    mdir = getSourceDir(flag, p->source);
#endif
    if (tag != RPMTAG_ICON) {
	const char *body = rpmGenPath(NULL, mdir, p->source);

	sprintf(buf, "%s%d",
		(flag & RPMFILE_PATCH) ? "PATCH" : "SOURCE", num);
	addMacro(spec->macros, buf, NULL, body, RMIL_SPEC);
	sprintf(buf, "%sURL%d",
		(flag & RPMFILE_PATCH) ? "PATCH" : "SOURCE", num);
	addMacro(spec->macros, buf, NULL, p->fullSource, RMIL_SPEC);
#ifdef WITH_LUA
	if (!spec->recursing) {
	    rpmlua lua = NULL; /* global state */
	    const char * what = (flag & RPMFILE_PATCH) ? "patches" : "sources";
	    rpmluav var = rpmluavNew();

	    rpmluaPushTable(lua, what);
	    rpmluavSetListMode(var, 1);
	    rpmluavSetValue(var, RPMLUAV_STRING, body);
	    rpmluaSetVar(lua, var);
/*@-moduncon@*/
	    var = (rpmluav) rpmluavFree(var);
/*@=moduncon@*/
	    rpmluaPop(lua);
	}
#endif
	body = _free(body);
    }
    
    return RPMRC_OK;
}

/**
 */
static inline /*@only@*/ /*@null@*/ speclines newSl(void)
	/*@*/
{
    speclines sl = NULL;
    if (specedit) {
	sl = xmalloc(sizeof(*sl));
	sl->sl_lines = NULL;
	sl->sl_nalloc = 0;
	sl->sl_nlines = 0;
    }
    return sl;
}

/**
 */
static inline /*@null@*/ speclines freeSl(/*@only@*/ /*@null@*/ speclines sl)
	/*@modifies sl @*/
{
    int i;
    if (sl == NULL) return NULL;
    for (i = 0; i < sl->sl_nlines; i++)
	/*@-unqualifiedtrans@*/
	sl->sl_lines[i] = _free(sl->sl_lines[i]);
	/*@=unqualifiedtrans@*/
    sl->sl_lines = _free(sl->sl_lines);
    return _free(sl);
}

/**
 */
static inline /*@only@*/ /*@null@*/ spectags newSt(void)
	/*@*/
{
    spectags st = NULL;
    if (specedit) {
	st = xmalloc(sizeof(*st));
	st->st_t = NULL;
	st->st_nalloc = 0;
	st->st_ntags = 0;
    }
    return st;
}

/**
 */
static inline /*@null@*/ spectags freeSt(/*@only@*/ /*@null@*/ spectags st)
	/*@modifies st @*/
{
    int i;
    if (st == NULL) return NULL;
    for (i = 0; i < st->st_ntags; i++) {
	spectag t = st->st_t + i;
	t->t_lang = _free(t->t_lang);
	t->t_msgid = _free(t->t_msgid);
    }
    st->st_t = _free(st->st_t);
    return _free(st);
}

static void specFini(void * _spec)
	/*@modifies _spec @*/
{
    Spec spec = _spec;
    struct ReadLevelEntry *rl;

    if (spec == NULL) return;	/* XXX assert? */

    spec->lbuf = _free(spec->lbuf);

    spec->sl = freeSl(spec->sl);
    spec->st = freeSt(spec->st);

    spec->prep = rpmiobFree(spec->prep);
    spec->build = rpmiobFree(spec->build);
    spec->install = rpmiobFree(spec->install);
    spec->check = rpmiobFree(spec->check);
    spec->clean = rpmiobFree(spec->clean);
    spec->foo = tagStoreFree(spec->foo, spec->nfoo);
    spec->nfoo = 0;

    spec->buildSubdir = _free(spec->buildSubdir);
    spec->rootURL = _free(spec->rootURL);
    spec->specFile = _free(spec->specFile);

    closeSpec(spec);

    while (spec->readStack) {
	rl = spec->readStack;
	/*@-dependenttrans@*/
	spec->readStack = rl->next;
	/*@=dependenttrans@*/
	rl->next = NULL;
	rl = _free(rl);
    }
    
    spec->sourceRpmName = _free(spec->sourceRpmName);
    spec->sourcePkgId = _free(spec->sourcePkgId);
    spec->sourceHeader = headerFree(spec->sourceHeader);

    if (spec->fi != NULL) {
	rpmfi fi = spec->fi;
	spec->fi = NULL;
	fi = rpmfiFree(fi);
    }
    
    if (!spec->recursing) {
	if (spec->BASpecs != NULL)
	while (spec->BACount--) {
	    /*@-unqualifiedtrans@*/
	    spec->BASpecs[spec->BACount] =
			freeSpec(spec->BASpecs[spec->BACount]);
	    /*@=unqualifiedtrans@*/
	}
	/*@-compdef@*/
	spec->BASpecs = _free(spec->BASpecs);
	/*@=compdef@*/
    }
    spec->BANames = _free(spec->BANames);

    spec->passPhrase = _free(spec->passPhrase);
    spec->cookie = _free(spec->cookie);

#ifdef WITH_LUA
    {	rpmlua lua = NULL; /* global state */
	rpmluaDelVar(lua, "patches");
	rpmluaDelVar(lua, "sources");	
    }
#endif

    spec->sources = freeSources(spec->sources);

    spec->dig = pgpDigFree(spec->dig);
    spec->packages = freePackages(spec->packages);
    
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _specPool;

static Spec specGetPool(rpmioPool pool)
{
    Spec spec;

    if (_specPool == NULL) {
	_specPool = rpmioNewPool("spec", sizeof(*spec), -1, _spec_debug,
			NULL, NULL, specFini);
	pool = _specPool;
    }
    spec = (Spec) rpmioGetPool(pool, sizeof(*spec));
    memset(((char *)spec)+sizeof(spec->_item), 0, sizeof(*spec)-sizeof(spec->_item));
    return spec;
}

Spec newSpec(void)
{
    static const char _spec_line_buffer_size[] =
	"%{?_spec_line_buffer_size}%{!?_spec_line_buffer_size:100000}";
    Spec spec = specGetPool(_specPool);
    
    spec->specFile = NULL;

    spec->sl = newSl();
    spec->st = newSt();

    spec->fileStack = NULL;
    spec->lbuf_len = (size_t)rpmExpandNumeric(_spec_line_buffer_size);
    spec->lbuf = (char *)xmalloc(spec->lbuf_len);
    spec->lbuf[0] = '\0';
    spec->line = spec->lbuf;
    spec->nextline = NULL;
    spec->nextpeekc = '\0';
    spec->lineNum = 0;
    spec->readStack = xmalloc(sizeof(*spec->readStack));
    spec->readStack->next = NULL;
    spec->readStack->reading = 1;

    spec->rootURL = NULL;

    memset(&spec->sstates, 0, sizeof(spec->sstates));
    memset(&spec->smetrics, 0, sizeof(spec->smetrics));

    spec->prep = NULL;
    spec->build = NULL;
    spec->install = NULL;
    spec->check = NULL;
    spec->clean = NULL;
    spec->foo = NULL;
    spec->nfoo = 0;

    spec->dig = NULL;

    spec->sources = NULL;
    spec->packages = NULL;
    spec->noSource = 0;
    spec->numSources = 0;

    spec->sourceRpmName = NULL;
    spec->sourcePkgId = NULL;
    spec->sourceHeader = headerNew();
    spec->fi = NULL;
    
    spec->buildSubdir = NULL;

    spec->passPhrase = NULL;
    spec->timeCheck = 0;
    spec->cookie = NULL;

    spec->BANames = NULL;
    spec->BACount = 0;
    spec->recursing = 0;
    spec->toplevel = 1;
    spec->BASpecs = NULL;

    spec->force = 0;
    spec->anyarch = 0;

/*@i@*/	spec->macros = rpmGlobalMacroContext;

    spec->_parseRCPOT = parseRCPOT;	/* XXX hack around backward linkage. */
    
    return (Spec)rpmioLinkPoolItem((rpmioItem)spec, __FUNCTION__, __FILE__, __LINE__);
}

/*@only@*/
struct OpenFileInfo * newOpenFileInfo(void)
{
    struct OpenFileInfo *ofi;

    ofi = xmalloc(sizeof(*ofi));
    ofi->fd = NULL;
    ofi->fileName = NULL;
    ofi->lineNum = 0;
    ofi->readBuf[0] = '\0';
    ofi->readPtr = NULL;
    ofi->next = NULL;

    return ofi;
}

/**
 * Print copy of spec file, filling in Group/Description/Summary from specspo.
 * @param spec		spec file control structure
 */
static void
printNewSpecfile(Spec spec)
	/*@globals fileSystem, internalState @*/
	/*@modifies spec->sl->sl_lines[], spec->packages->header,
		fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Header h;
    speclines sl = spec->sl;
    spectags st = spec->st;
    const char * msgstr = NULL;
    int i, j;
    int xx;

    if (sl == NULL || st == NULL)
	return;

    for (i = 0; i < st->st_ntags; i++) {
	spectag t = st->st_t + i;
	const char * tn = tagName(t->t_tag);
	const char * errstr;
	char fmt[1024];

	fmt[0] = '\0';
	if (t->t_msgid == NULL)
	    h = spec->packages->header;
	else {
	    Package pkg;
	    char *fe;

	    strncpy(fmt, t->t_msgid, sizeof(fmt)-1);
	    fmt[sizeof(fmt)-1] = '\0';
	    for (fe = fmt; *fe && *fe != '('; fe++)
		{} ;
	    if (*fe == '(') *fe = '\0';
	    h = NULL;
	    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
		h = pkg->header;
		he->tag = RPMTAG_NAME;
		xx = headerGet(h, he, 0);
		if (!strcmp(he->p.str, fmt)) {
		    he->p.ptr = _free(he->p.ptr);
		    /*@innerbreak@*/ break;
		}
		he->p.ptr = _free(he->p.ptr);
	    }
	    if (pkg == NULL || h == NULL)
		h = spec->packages->header;
	}

	if (h == NULL)
	    continue;

	fmt[0] = '\0';
	(void) stpcpy( stpcpy( stpcpy( fmt, "%{"), tn), "}");
	msgstr = _free(msgstr);

	/* XXX this should use queryHeader(), but prints out tn as well. */
	msgstr = headerSprintf(h, fmt, NULL, rpmHeaderFormats, &errstr);
	if (msgstr == NULL) {
	    rpmlog(RPMLOG_ERR, _("can't query %s: %s\n"), tn, errstr);
	    return;
	}

	switch(t->t_tag) {
	case RPMTAG_SUMMARY:
	case RPMTAG_GROUP:
	    /*@-unqualifiedtrans@*/
	    sl->sl_lines[t->t_startx] = _free(sl->sl_lines[t->t_startx]);
	    /*@=unqualifiedtrans@*/
	    if (t->t_lang && strcmp(t->t_lang, RPMBUILD_DEFAULT_LANG))
		continue;
	    {   char *buf = xmalloc(strlen(tn) + sizeof(": ") + strlen(msgstr));
		(void) stpcpy( stpcpy( stpcpy(buf, tn), ": "), msgstr);
		sl->sl_lines[t->t_startx] = buf;
	    }
	    /*@switchbreak@*/ break;
	case RPMTAG_DESCRIPTION:
	    for (j = 1; j < t->t_nlines; j++) {
		if (*sl->sl_lines[t->t_startx + j] == '%')
		    /*@innercontinue@*/ continue;
		/*@-unqualifiedtrans@*/
		sl->sl_lines[t->t_startx + j] =
			_free(sl->sl_lines[t->t_startx + j]);
		/*@=unqualifiedtrans@*/
	    }
	    if (t->t_lang && strcmp(t->t_lang, RPMBUILD_DEFAULT_LANG)) {
		sl->sl_lines[t->t_startx] = _free(sl->sl_lines[t->t_startx]);
		continue;
	    }
	    sl->sl_lines[t->t_startx + 1] = xstrdup(msgstr);
	    if (t->t_nlines > 2)
		sl->sl_lines[t->t_startx + 2] = xstrdup("\n\n");
	    /*@switchbreak@*/ break;
	}
    }
    msgstr = _free(msgstr);

    for (i = 0; i < sl->sl_nlines; i++) {
	const char * s = sl->sl_lines[i];
	if (s == NULL)
	    continue;
	printf("%s", s);
	if (strchr(s, '\n') == NULL && s[strlen(s)-1] != '\n')
	    printf("\n");
    }
}

/**
 * Add expanded build scriptlet to srpm header.
 * @param h		srpm header
 * @param progTag	interpreter tag (0 disables)
 * @param scriptTag	script tag (0 disables)
 * @param iob		script body sting buf (NULL disables)
 * @return		0 always
 */
static int initSourceHeaderScriptlet(Header h,
		rpmTag progTag, rpmTag scriptTag, rpmiob iob)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int xx;

    if (progTag !=(rpmTag) 0) {
	static const char prog[] = "/bin/sh";	/* XXX FIXME */
	he->tag = progTag;
	he->t = RPM_STRING_TYPE;
	he->p.str = prog;
	he->c = 1;
	xx = headerPut(h, he, 0);
    }

    if (scriptTag != (rpmTag)0 && iob != NULL) {
	he->tag = scriptTag;
	he->t = RPM_STRING_TYPE;
	he->p.str = rpmiobStr(iob);
	he->c = 1;
	xx = headerPut(h, he, 0);
    }
    return 0;
}

/**
 * Add expanded build scriptlets to srpm header.
 * @param spec		spec file control structure
 * @return		0 always
 */
static int initSourceHeaderScriptlets(Spec spec)
	/*@modifies spec->sourceHeader @*/
{
    int xx;

    if (spec->prep != NULL)
	xx = initSourceHeaderScriptlet(spec->sourceHeader,
	    tagValue("Buildprepprog"), tagValue("Buildprep"), spec->prep);
    if (spec->build != NULL)
	xx = initSourceHeaderScriptlet(spec->sourceHeader,
	    tagValue("Buildbuildprog"), tagValue("Buildbuild"), spec->build);
    if (spec->install != NULL)
	xx = initSourceHeaderScriptlet(spec->sourceHeader,
	    tagValue("Buildinstallprog"), tagValue("Buildinstall"), spec->install);
    if (spec->check != NULL)
	xx = initSourceHeaderScriptlet(spec->sourceHeader,
	    tagValue("Buildcheckprog"), tagValue("Buildcheck"), spec->check);
    if (spec->clean != NULL)
	xx = initSourceHeaderScriptlet(spec->sourceHeader,
	    tagValue("Buildcleanprog"), tagValue("Buildclean"), spec->clean);

   return 0;
}

/**
 * Parse a spec file, and query the resultant header.
 * @param ts		rpm transaction
 * @param qva		query args
 * @param specName	specfile to parse
 * @param target	cpu-vender-os platform for query (NULL is current)
 * @return              0 on success
 */
static int _specQuery(rpmts ts, QVA_t qva, const char *specName,
		/*@null@*/ const char *target) 
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    Spec spec = NULL;
    Package pkg;
    int res = 1;	/* assume error */
    int anyarch = (target == NULL) ? 1 : 0;
    char * passPhrase = "";
    int recursing = 0;
    char *cookie = NULL;
    int verify = 0;
    int xx;

    /*@-mods@*/ /* FIX: make spec abstract */
    if (parseSpec(ts, specName, "/", recursing, passPhrase,
		cookie, anyarch, 1, verify)
      || (spec = rpmtsSetSpec(ts, NULL)) == NULL)
    {
	rpmlog(RPMLOG_ERR,
	    _("query of specfile %s failed, can't parse\n"), 
	    specName);
	goto exit;
    }
    /*@=mods@*/

    res = 0;
    if (specedit) {
	printNewSpecfile(spec);
	goto exit;
    }

    switch (qva->qva_source) {
    case RPMQV_SPECSRPM:
	xx = initSourceHeader(spec, NULL);
	xx = initSourceHeaderScriptlets(spec);
	xx = qva->qva_showPackage(qva, ts, spec->sourceHeader);
	break;
    default:
    case RPMQV_SPECFILE:
	for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
	    /* If no target was specified, display all packages.
	     * Packages with empty file lists are not produced.
	     */
	    /* XXX DIEDIEDIE: this logic looks flawed. */
	    if (target == NULL || pkg->fileList != NULL) 
		xx = qva->qva_showPackage(qva, ts, pkg->header);
	}
	break;
    }

exit:
    spec = freeSpec(spec);
    return res;
}

int rpmspecQuery(rpmts ts, QVA_t qva, const char * arg)
{
    int res = 1;
    const char * targets = rpmcliTargets;
    char *target;
    const char * t;
    const char * te;
    int nqueries = 0;

    if (qva->qva_showPackage == NULL)
	goto exit;

    if (targets == NULL) {
	res = _specQuery(ts, qva, arg, NULL); 
	nqueries++;
	goto exit;
    }

    rpmlog(RPMLOG_DEBUG, 
	_("Query specfile for platform(s): %s\n"), targets);
    for (t = targets; *t != '\0'; t = te) {
	/* Parse out next target platform. */ 
	if ((te = strchr(t, ',')) == NULL)
	    te = t + strlen(t);
	target = alloca(te-t+1);
	strncpy(target, t, (te-t));
	target[te-t] = '\0';
	if (*te != '\0')
	    te++;

	/* Query spec for this target platform. */
	rpmlog(RPMLOG_DEBUG, _("    target platform: %s\n"), target);
	/* Read in configuration for target. */
	if (t != targets) {
	    rpmFreeMacros(NULL);
	    rpmFreeRpmrc();
	    (void) rpmReadConfigFiles(NULL, target);
	}
	res = _specQuery(ts, qva, arg, target); 
	nqueries++;
	if (res) break;	
    }
    
exit:
    /* Restore original configuration. */
    if (nqueries > 1) {
	t = targets;
	if ((te = strchr(t, ',')) == NULL)
	    te = t + strlen(t);
	target = alloca(te-t+1);
	strncpy(target, t, (te-t));
	target[te-t] = '\0';
	if (*te != '\0')
	    te++;
	rpmFreeMacros(NULL);
	rpmFreeRpmrc();
	(void) rpmReadConfigFiles(NULL, target);
    }
    return res;
}
