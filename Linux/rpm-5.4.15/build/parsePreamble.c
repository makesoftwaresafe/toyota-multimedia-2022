/** \ingroup rpmbuild
 * \file build/parsePreamble.c
 *  Parse tags in global section from spec file.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <rpmurl.h>
#include <argv.h>
#include <mire.h>

#define	_RPMEVR_INTERNAL
#define	_RPMTAG_INTERNAL	/* XXX rpmTags->aTags */
#include <rpmbuild.h>
#include "debug.h"

/*@access FD_t @*/	/* compared with NULL */
/*@access headerTagIndices @*/	/* rpmTags->aTags */

/**
 */
/*@observer@*/ /*@unchecked@*/
static rpmTag copyTagsDuringParse[] = {
    RPMTAG_EPOCH,
    RPMTAG_VERSION,
    RPMTAG_RELEASE,
    RPMTAG_DISTEPOCH,
    RPMTAG_LICENSE,
    RPMTAG_GROUP,		/* XXX permissive. */
    RPMTAG_SUMMARY,		/* XXX permissive. */
    RPMTAG_DESCRIPTION,		/* XXX permissive. */
    RPMTAG_PACKAGER,
    RPMTAG_DISTRIBUTION,
    RPMTAG_DISTURL,
    RPMTAG_VENDOR,
    RPMTAG_ICON,
    RPMTAG_GIF,
    RPMTAG_XPM,
    RPMTAG_URL,
    RPMTAG_CHANGELOGTIME,
    RPMTAG_CHANGELOGNAME,
    RPMTAG_CHANGELOGTEXT,
    RPMTAG_PREFIXES,
    RPMTAG_DISTTAG,
    RPMTAG_BUGURL,
    RPMTAG_CVSID,
    RPMTAG_VARIANTS,
    RPMTAG_XMAJOR,
    RPMTAG_XMINOR,
    RPMTAG_REPOTAG,
    RPMTAG_KEYWORDS,
    0
};

/**
 */
/*@observer@*/ /*@unchecked@*/
static rpmTag requiredTags[] = {
    RPMTAG_NAME,
    RPMTAG_VERSION,
    RPMTAG_RELEASE,
    RPMTAG_SUMMARY,
    RPMTAG_GROUP,
    RPMTAG_LICENSE,
    0
};

/**
 */
static void addOrAppendListEntry(Header h, rpmTag tag, char * line)
	/*@modifies h @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int xx;
    int argc;
    const char **argv;

    xx = poptParseArgvString(line, &argc, &argv);
    if (argc) {
	he->tag = tag;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = argv;
	he->c = argc;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
    }
    argv = _free(argv);
}

/* Parse a simple part line that only take -n <pkg> or <pkg> */
/* <pkg> is returned in name as a pointer into malloc'd storage. */

/**
 */
static int parseSimplePart(Spec spec, /*@out@*/char ** Np,
		/*@out@*/rpmParseState *flag)
	/*@globals internalState@*/
	/*@modifies *Np, *flag, internalState, spec->line @*/
{
    char * s, * se;
    int rc = 0;		/* assume failure */

    if (Np)
	*Np = NULL;

    se = strchr(spec->line, '#');
    if (se) {
	*se = '\0';
	while (--se >= spec->line && strchr(" \t\n\r", *se) != NULL)
	    *se = '\0';
    }

    s = xstrdup(spec->line);
    /* Throw away the first token (the %xxxx) */
    (void)strtok(s, " \t\n");
    
    if (!(se = strtok(NULL, " \t\n")))
	goto exit;
    
    if (!strcmp(se, "-n")) {
	if (!(se = strtok(NULL, " \t\n"))) {
	    rc = 1;
	    goto exit;
	}
	*flag = PART_NAME;
    } else
	*flag = PART_SUBNAME;

    if (Np)
	*Np = xstrdup(se);

    rc = (strtok(NULL, " \t\n") ? 1 : 0);

exit:
    s = _free(s);
    return rc;
}

/**
 */
static inline int parseYesNo(const char * s)
	/*@*/
{
    return ((!s || (s[0] == 'n' || s[0] == 'N' || s[0] == '0') ||
	!xstrcasecmp(s, "false") || !xstrcasecmp(s, "off"))
	    ? 0 : 1);
}

typedef struct tokenBits_s {
/*@observer@*/ /*@null@*/
    const char * name;
    rpmsenseFlags bits;
} * tokenBits;

/**
 */
/*@observer@*/ /*@unchecked@*/
static struct tokenBits_s installScriptBits[] = {
    { "interp",		RPMSENSE_INTERP },
    { "preun",		RPMSENSE_SCRIPT_PREUN },
    { "pre",		RPMSENSE_SCRIPT_PRE },
    { "postun",		RPMSENSE_SCRIPT_POSTUN },
    { "post",		RPMSENSE_SCRIPT_POST },
    { "rpmlib",		RPMSENSE_RPMLIB },
    { "verify",		RPMSENSE_SCRIPT_VERIFY },
    { "hint",		RPMSENSE_MISSINGOK },
    { NULL, 0 }
};

/**
 */
/*@observer@*/ /*@unchecked@*/
static struct tokenBits_s buildScriptBits[] = {
    { "prep",		RPMSENSE_SCRIPT_PREP },
    { "build",		RPMSENSE_SCRIPT_BUILD },
    { "install",	RPMSENSE_SCRIPT_INSTALL },
    { "clean",		RPMSENSE_SCRIPT_CLEAN },
    { "hint",		RPMSENSE_MISSINGOK },
    { NULL, 0 }
};

/**
 */
static int parseBits(const char * s, const tokenBits tokbits,
		/*@out@*/ rpmsenseFlags * bp)
	/*@modifies *bp @*/
{
    tokenBits tb;
    const char * se;
    rpmsenseFlags bits = RPMSENSE_ANY;
    int c = 0;

    if (s) {
	while (*s != '\0') {
	    while ((c = *s) && xisspace(c)) s++;
	    se = s;
	    while ((c = *se) && xisalpha(c)) se++;
	    if (s == se)
		break;
	    for (tb = tokbits; tb->name; tb++) {
		if (tb->name != NULL &&
		    strlen(tb->name) == (size_t)(se-s) && !strncmp(tb->name, s, (se-s)))
		    /*@innerbreak@*/ break;
	    }
	    if (tb->name == NULL)
		break;
	    bits |= tb->bits;
	    while ((c = *se) && xisspace(c)) se++;
	    if (c != ',')
		break;
	    s = ++se;
	}
    }
    if (c == 0 && bp) *bp = bits;
    return (c ? RPMRC_FAIL : RPMRC_OK);
}

/**
 */
/*@null@*/
static inline char * findLastChar(char * s)
	/*@modifies *s @*/
{
    char *se = s + strlen(s);

    /* Right trim white space. */
    while (--se > s && strchr(" \t\n\r", *se) != NULL)
	*se = '\0';
    /* Truncate comments. */
    if ((se = strchr(s, '#')) != NULL) {
	*se = '\0';
	while (--se > s && strchr(" \t\n\r", *se) != NULL)
	    *se = '\0';
    }
/*@-temptrans -retalias @*/
    return se;
/*@=temptrans =retalias @*/
}

/**
 */
static int isMemberInEntry(Header h, const char *name, rpmTag tag)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    int rc = -1;
    int xx;

    he->tag = tag;
    xx = headerGet(h, he, 0);
    if (!xx)
	return rc;
    rc = 0;
    while (he->c) {
	he->c--;
	if (xstrcasecmp(he->p.argv[he->c], name))
	    continue;
	rc = 1;
	break;
    }
    he->p.ptr = _free(he->p.ptr);
    return rc;
}

/**
 */
static int checkForValidArchitectures(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/
{
    const char *arch = rpmExpand("%{_target_cpu}", NULL);
    const char *os = rpmExpand("%{_target_os}", NULL);
    int rc = RPMRC_FAIL;	/* assume failure. */
    
    if (isMemberInEntry(spec->sourceHeader, arch, RPMTAG_EXCLUDEARCH) == 1) {
	rpmlog(RPMLOG_ERR, _("Architecture is excluded: %s\n"), arch);
	goto exit;
    }
    if (isMemberInEntry(spec->sourceHeader, arch, RPMTAG_EXCLUSIVEARCH) == 0) {
	rpmlog(RPMLOG_ERR, _("Architecture is not included: %s\n"), arch);
	goto exit;
    }
    if (isMemberInEntry(spec->sourceHeader, os, RPMTAG_EXCLUDEOS) == 1) {
	rpmlog(RPMLOG_ERR, _("OS is excluded: %s\n"), os);
	goto exit;
    }
    if (isMemberInEntry(spec->sourceHeader, os, RPMTAG_EXCLUSIVEOS) == 0) {
	rpmlog(RPMLOG_ERR, _("OS is not included: %s\n"), os);
	goto exit;
    }
    rc = 0;
exit:
    arch = _free(arch);
    os = _free(os);
    return rc;
}

/**
 * Check that required tags are present in header.
 * @param h		header
 * @param NVR		package name-version-release
 * @return		RPMRC_OK if OK
 */
static rpmRC checkForRequired(Header h, const char * NVR)
	/*@*/
{
    rpmTag * p;
    rpmRC rc = RPMRC_OK;

    for (p = requiredTags; *p != 0; p++) {
	if (!headerIsEntry(h, *p)) {
	    rpmlog(RPMLOG_ERR,
			_("%s field must be present in package: %s\n"),
			tagName(*p), NVR);
	    rc = RPMRC_FAIL;
	}
    }

    return rc;
}

/**
 * Check that no duplicate tags are present in header.
 * @param h		header
 * @param NVR		package name-version-release
 * @return		RPMRC_OK if OK
 */
static rpmRC checkForDuplicates(Header h, const char * NVR)
	/*@globals internalState @*/
	/*@modifies h, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HeaderIterator hi;
    rpmTag lastTag = 0;
    rpmRC rc = RPMRC_OK;
    
    for (hi = headerInit(h);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
	if (he->tag != lastTag) {
	    lastTag = he->tag;
	    continue;
	}
	rpmlog(RPMLOG_ERR, _("Duplicate %s entries in package: %s\n"),
		     tagName(he->tag), NVR);
	rc = RPMRC_FAIL;
    }
    hi = headerFini(hi);

    return rc;
}

/**
 */
/*@observer@*/ /*@unchecked@*/
static struct optionalTag {
    rpmTag	ot_tag;
/*@observer@*/ /*@null@*/
    const char * ot_mac;
} optionalTags[] = {
    { RPMTAG_VENDOR,		"%{vendor}" },
    { RPMTAG_PACKAGER,		"%{packager}" },
    { RPMTAG_DISTEPOCH,		"%{distepoch}" },
    { RPMTAG_DISTRIBUTION,	"%{distribution}" },
    { RPMTAG_DISTTAG,		"%{disttag}" },
    { RPMTAG_DISTURL,		"%{disturl}" },
    { RPMTAG_BUGURL,		"%{bugurl}" },
    { 0xffffffff,		"%{class}" },
    { -1, NULL }
};

/**
 */
static void fillOutMainPackage(Header h)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies h, rpmGlobalMacroContext, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct optionalTag *ot;
    int xx;

    for (ot = optionalTags; ot->ot_mac != NULL; ot++) {
	const char * val;
	rpmTag tag;

	tag = ot->ot_tag;

	/* Generate arbitrary tag (if necessary). */
	if (tag == 0xffffffff) {
	    val = tagCanonicalize(ot->ot_mac + (sizeof("%{")-1));
	    tag = tagGenerate(val);
	    val = _free(val);
	}

	if (headerIsEntry(h, tag))
	    continue;
	val = rpmExpand(ot->ot_mac, NULL);
	if (val && *val != '%') {
		he->tag = tag;
		he->t = RPM_STRING_TYPE;
		he->p.str = val;
		he->c = 1;
		xx = headerPut(h, he, 0);
	}
	val = _free(val);
    }
}

/**
 */
static int doIcon(Spec spec, Header h)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, rpmGlobalMacroContext, fileSystem, internalState  @*/
{
    static size_t iconsize = 0;
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char *fn, *Lurlfn = NULL;
    struct Source *sp;
    size_t nb;
    rpmuint8_t * icon;
    FD_t fd = NULL;
    int rc = RPMRC_FAIL;	/* assume error */
    int urltype;
    int xx;

    if (iconsize == 0) {
	iconsize = rpmExpandNumeric("%{?_build_iconsize}");
	if (iconsize < 2048)
	    iconsize = 2048;
    }
    icon = alloca(iconsize+1);

    for (sp = spec->sources; sp != NULL; sp = sp->next) {
	if (sp->flags & RPMFILE_ICON)
	    break;
    }
    if (sp == NULL) {
	rpmlog(RPMLOG_ERR, _("No icon file in sources\n"));
	goto exit;
    }

#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    /* support splitted source directories, i.e., source files which
       are alternatively placed into the .spec directory and picked
       up from there, too. */
    Lurlfn = rpmGenPath(NULL, "%{_specdir}/", sp->source);
    if (access(Lurlfn, F_OK) == -1) {
        Lurlfn = _free(Lurlfn);
        Lurlfn = rpmGenPath(NULL, "%{_icondir}/", sp->source);
    }
#else
    Lurlfn = rpmGenPath(NULL, "%{_icondir}/", sp->source);
#endif

    fn = NULL;
    urltype = urlPath(Lurlfn, &fn);
    switch (urltype) {  
    case URL_IS_HTTPS: 
    case URL_IS_HTTP:
    case URL_IS_FTP:
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
	rpmlog(RPMLOG_ERR, _("Invalid icon URL: %s\n"), Lurlfn);
	goto exit;
	/*@notreached@*/ break;
    }

    fd = Fopen(fn, "r%{?_rpmgio}");
    if (fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("Unable to open icon %s: %s\n"),
		fn, Fstrerror(fd));
	rc = RPMRC_FAIL;
	goto exit;
    }

    *icon = '\0';
    nb = Fread(icon, sizeof(icon[0]), iconsize, fd);
    if (Ferror(fd) || nb == 0) {
	rpmlog(RPMLOG_ERR, _("Unable to read icon %s: %s\n"),
		fn, Fstrerror(fd));
	goto exit;
    }
    if (nb >= iconsize) {
	rpmlog(RPMLOG_ERR, _("Icon %s is too big (max. %d bytes)\n"),
		fn, (int)iconsize);
	goto exit;
    }

    if (icon[0] == 'G' && icon[1] == 'I' && icon[2] == 'F')
	he->tag = RPMTAG_GIF;
    else
    if (icon[0] == '/' && icon[1] == '*' && icon[2] == ' '
     && icon[3] == 'X' && icon[4] == 'P' && icon[5] == 'M')
	he->tag = RPMTAG_XPM;
    else
	he->tag = tagValue("Icon");
    he->t = RPM_BIN_TYPE;
    he->p.ui8p = icon;
    he->c = (rpmTagCount)nb;
    xx = headerPut(h, he, 0);
    rc = 0;
    
exit:
    if (fd) {
	(void) Fclose(fd);
	fd = NULL;
    }
    Lurlfn = _free(Lurlfn);
    return rc;
}

spectag stashSt(Spec spec, Header h, rpmTag tag, const char * lang)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    spectag t = NULL;
    int xx;

    if (spec->st) {
	spectags st = spec->st;
	if (st->st_ntags == st->st_nalloc) {
	    st->st_nalloc += 10;
	    st->st_t = xrealloc(st->st_t, st->st_nalloc * sizeof(*(st->st_t)));
	}
	t = st->st_t + st->st_ntags++;
	t->t_tag = tag;
	t->t_startx = spec->lineNum - 1;
	t->t_nlines = 1;
	t->t_lang = xstrdup(lang);
	t->t_msgid = NULL;
	if (!(t->t_lang && strcmp(t->t_lang, RPMBUILD_DEFAULT_LANG))) {
	    he->tag = RPMTAG_NAME;
	    xx = headerGet(h, he, 0);
	    if (xx) {
		char buf[1024];
		sprintf(buf, "%s(%s)", he->p.str, tagName(tag));
		t->t_msgid = xstrdup(buf);
	    }
	    he->p.ptr = _free(he->p.ptr);
	}
    }
    /*@-usereleased -compdef@*/
    return t;
    /*@=usereleased =compdef@*/
}

#define SINGLE_TOKEN_ONLY \
if (multiToken) { \
    rpmlog(RPMLOG_ERR, _("line %d: Tag takes single token only: %s\n"), \
	     spec->lineNum, spec->line); \
    return RPMRC_FAIL; \
}

/*@-redecl@*/
extern int noLang;
/*@=redecl@*/

static rpmRC tagValidate(Spec spec, rpmTag tag, const char * value)
	/*@*/
{
    const char * tagN = tagName(tag);
    const char * pattern = rpmExpand("%{?pattern_", tagN, "}", NULL);
    rpmRC ec = RPMRC_OK;

    if (pattern && *pattern) {
	miRE mire;
	int xx;

	mire = mireNew(RPMMIRE_REGEX, tag);
	xx = mireSetCOptions(mire, RPMMIRE_REGEX, 0, 0, NULL);
	if (!xx)
	    xx = mireRegcomp(mire, pattern);
	if (!xx)
	    xx = mireRegexec(mire, value, strlen(value));
	if (!xx)
	    ec = RPMRC_OK;
	else {
	    rpmlog(RPMLOG_ERR, _("line %d: invalid tag value(\"%s\") %s: %s\n"),
		    spec->lineNum, pattern, tagN, spec->line);
	    ec = RPMRC_FAIL;
	}

	mire = mireFree(mire);
    }

    pattern = _free(pattern);

    return ec;
}

/**
 */
static rpmRC handlePreambleTag(Spec spec, Package pkg, rpmTag tag,
		const char *macro, const char *lang)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->macros, spec->st,
		spec->sources, spec->numSources, spec->noSource,
		spec->sourceHeader, spec->BANames, spec->BACount,
		spec->line,
		pkg->header, pkg->autoProv, pkg->autoReq, pkg->noarch,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    char * field = spec->line;
    char * end;
    int multiToken = 0;
    rpmsenseFlags tagflags;
    int len;
    rpmuint32_t num;
    int rc;
    int xx;
    
    if (field == NULL) return RPMRC_FAIL;	/* XXX can't happen */
    /* Find the start of the "field" and strip trailing space */
    while ((*field) && (*field != ':'))
	field++;
    if (*field != ':') {
	rpmlog(RPMLOG_ERR, _("line %d: Malformed tag: %s\n"),
		 spec->lineNum, spec->line);
	return RPMRC_FAIL;
    }
    field++;
    SKIPSPACE(field);
    if (!*field) {
	/* Empty field */
	rpmlog(RPMLOG_ERR, _("line %d: Empty tag: %s\n"),
		 spec->lineNum, spec->line);
	return RPMRC_FAIL;
    }
    end = findLastChar(field);

    /* Validate tag data content. */
    if (tagValidate(spec, tag, field) != RPMRC_OK)
	return RPMRC_FAIL;

    /* See if this is multi-token */
    end = field;
    SKIPNONSPACE(end);
    if (*end != '\0')
	multiToken = 1;

    switch (tag) {
    case RPMTAG_NAME:
    case RPMTAG_VERSION:
    case RPMTAG_RELEASE:
    case RPMTAG_DISTEPOCH:
    case RPMTAG_URL:
    case RPMTAG_DISTTAG:
    case RPMTAG_REPOTAG:
    case RPMTAG_CVSID:
    case RPMTAG_BUGURL:
	SINGLE_TOKEN_ONLY;
	/* These macros are for backward compatibility */
	if (tag == RPMTAG_VERSION) {
	    if (strchr(field, '-') != NULL) {
		rpmlog(RPMLOG_ERR, _("line %d: Illegal char '-' in %s: %s\n"),
		    spec->lineNum, "version", spec->line);
		return RPMRC_FAIL;
	    }
	    addMacro(spec->macros, "PACKAGE_VERSION", NULL, field, RMIL_OLDSPEC);
	} else if (tag == RPMTAG_RELEASE) {
	    if (strchr(field, '-') != NULL) {
		rpmlog(RPMLOG_ERR, _("line %d: Illegal char '-' in %s: %s\n"),
		    spec->lineNum, "release", spec->line);
		return RPMRC_FAIL;
	    }
	    addMacro(spec->macros, "PACKAGE_RELEASE", NULL, field, RMIL_OLDSPEC-1);
	}
	he->tag = tag;
	he->t = RPM_STRING_TYPE;
	he->p.str = field;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);
	break;
    case RPMTAG_GROUP:
    case RPMTAG_SUMMARY:
#if defined(RPM_VENDOR_OPENPKG) /* make-class-available-as-macro */
    case RPMTAG_CLASS:
#endif
	(void) stashSt(spec, pkg->header, tag, lang);
	/*@fallthrough@*/
    case RPMTAG_DISTRIBUTION:
    case RPMTAG_VENDOR:
    case RPMTAG_LICENSE:
    case RPMTAG_PACKAGER:
	if (!*lang) {
	    he->tag = tag;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = field;
	    he->c = 1;
	    xx = headerPut(pkg->header, he, 0);
	} else if (!(noLang && strcmp(lang, RPMBUILD_DEFAULT_LANG))) {
#if defined(SUPPORT_I18NSTRING_TYPE)
	    (void) headerAddI18NString(pkg->header, tag, field, lang);
#else
	    if (!strcmp(lang, RPMBUILD_DEFAULT_LANG)) {
		he->tag = tag;
		he->t = RPM_STRING_TYPE;
		he->p.str = field;
		he->c = 1;
		xx = headerPut(pkg->header, he, 0);
	    }
#endif
	}
	break;
    /* XXX silently ignore BuildRoot: */
    case RPMTAG_BUILDROOT:
	SINGLE_TOKEN_ONLY;
	macro = NULL;
#ifdef	DYING
	buildRootURL = rpmGenPath(spec->rootURL, "%{?buildroot}", NULL);
	(void) urlPath(buildRootURL, &buildRoot);
	if (*buildRoot == '\0') buildRoot = "/";
	if (!strcmp(buildRoot, "/")) {
	    rpmlog(RPMLOG_ERR,
		     _("BuildRoot can not be \"/\": %s\n"), spec->buildRootURL);
	    buildRootURL = _free(buildRootURL);
	    return RPMRC_FAIL;
	}
	buildRootURL = _free(buildRootURL);
#endif
	break;
    case RPMTAG_KEYWORDS:
    case RPMTAG_VARIANTS:
    case RPMTAG_PREFIXES:
	addOrAppendListEntry(pkg->header, tag, field);
	he->tag = tag;
	xx = headerGet(pkg->header, he, 0);
	if (tag == RPMTAG_PREFIXES)
	while (he->c--) {
	    if (he->p.argv[he->c][0] != '/') {
		rpmlog(RPMLOG_ERR,
			 _("line %d: Prefixes must begin with \"/\": %s\n"),
			 spec->lineNum, spec->line);
		he->p.ptr = _free(he->p.ptr);
		return RPMRC_FAIL;
	    }
	    len = (int)strlen(he->p.argv[he->c]);
	    if (he->p.argv[he->c][len - 1] == '/' && len > 1) {
		rpmlog(RPMLOG_ERR,
			 _("line %d: Prefixes must not end with \"/\": %s\n"),
			 spec->lineNum, spec->line);
		he->p.ptr = _free(he->p.ptr);
		return RPMRC_FAIL;
	    }
	}
	he->p.ptr = _free(he->p.ptr);
	break;
    case RPMTAG_DOCDIR:
	SINGLE_TOKEN_ONLY;
	if (field[0] != '/') {
	    rpmlog(RPMLOG_ERR,
		     _("line %d: Docdir must begin with '/': %s\n"),
		     spec->lineNum, spec->line);
	    return RPMRC_FAIL;
	}
	macro = NULL;
	delMacro(NULL, "_docdir");
	addMacro(NULL, "_docdir", NULL, field, RMIL_SPEC);
	break;
    case RPMTAG_XMAJOR:
    case RPMTAG_XMINOR:
    case RPMTAG_EPOCH:
	SINGLE_TOKEN_ONLY;
	if (parseNum(field, &num)) {
	    rpmlog(RPMLOG_ERR,
		     _("line %d: %s takes an integer value: %s\n"),
		     spec->lineNum, tagName(tag), spec->line);
	    return RPMRC_FAIL;
	}
	he->tag = tag;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &num;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);
	break;
    case RPMTAG_AUTOREQPROV:
	pkg->autoReq = parseYesNo(field);
	pkg->autoProv = pkg->autoReq;
	break;
    case RPMTAG_AUTOREQ:
	pkg->autoReq = parseYesNo(field);
	break;
    case RPMTAG_AUTOPROV:
	pkg->autoProv = parseYesNo(field);
	break;
    case RPMTAG_SOURCE:
    case RPMTAG_PATCH:
	SINGLE_TOKEN_ONLY;
	macro = NULL;
	if ((rc = addSource(spec, pkg, field, tag)))
	    return rc;
	break;
    case RPMTAG_ICON:
	SINGLE_TOKEN_ONLY;
	macro = NULL;
	if ((rc = addSource(spec, pkg, field, tag)))
	    return rc;
	/* XXX the fetch/load of icon needs to be elsewhere. */
	if ((rc = doIcon(spec, pkg->header)))
	    return rc;
	break;
    case RPMTAG_NOSOURCE:
    case RPMTAG_NOPATCH:
	spec->noSource = 1;
	if ((rc = parseNoSource(spec, field, tag)))
	    return rc;
	break;
    case RPMTAG_BUILDPREREQ:
    case RPMTAG_BUILDREQUIRES:
	if ((rc = parseBits(lang, buildScriptBits, &tagflags))) {
	    rpmlog(RPMLOG_ERR,
		     _("line %d: Bad %s: qualifiers: %s\n"),
		     spec->lineNum, tagName(tag), spec->line);
	    return rc;
	}
	if ((rc = parseRCPOT(spec, pkg, field, tag, 0, tagflags)))
	    return rc;
	break;
    case RPMTAG_PREREQ:
    case RPMTAG_REQUIREFLAGS:
	if ((rc = parseBits(lang, installScriptBits, &tagflags))) {
	    rpmlog(RPMLOG_ERR,
		     _("line %d: Bad %s: qualifiers: %s\n"),
		     spec->lineNum, tagName(tag), spec->line);
	    return rc;
	}
	if ((rc = parseRCPOT(spec, pkg, field, tag, 0, tagflags)))
	    return rc;
	break;
    /* Aliases for BuildRequires(hint): */
    case RPMTAG_BUILDSUGGESTS:
    case RPMTAG_BUILDENHANCES:
	tagflags = RPMSENSE_MISSINGOK;
	if ((rc = parseRCPOT(spec, pkg, field, tag, 0, tagflags)))
	    return rc;
	break;
    /* Aliases for Requires(hint): */
    case RPMTAG_SUGGESTSFLAGS:
    case RPMTAG_ENHANCESFLAGS:
	tag = RPMTAG_REQUIREFLAGS;
	tagflags = RPMSENSE_MISSINGOK;
	if ((rc = parseRCPOT(spec, pkg, field, tag, 0, tagflags)))
	    return rc;
	break;
    case RPMTAG_BUILDOBSOLETES:
    case RPMTAG_BUILDPROVIDES:
    case RPMTAG_BUILDCONFLICTS:
    case RPMTAG_CONFLICTFLAGS:
    case RPMTAG_OBSOLETEFLAGS:
    case RPMTAG_PROVIDEFLAGS:
	tagflags = RPMSENSE_ANY;
	if ((rc = parseRCPOT(spec, pkg, field, tag, 0, tagflags)))
	    return rc;
	break;
    case RPMTAG_BUILDPLATFORMS:		/* XXX needs pattern parsing */
    case RPMTAG_EXCLUDEARCH:
    case RPMTAG_EXCLUSIVEARCH:
    case RPMTAG_EXCLUDEOS:
    case RPMTAG_EXCLUSIVEOS:
	addOrAppendListEntry(spec->sourceHeader, tag, field);
	break;

    case RPMTAG_BUILDARCHS:
    {	const char ** BANames = NULL;
	int BACount = 0;
	if ((rc = poptParseArgvString(field, &BACount, &BANames))) {
	    rpmlog(RPMLOG_ERR,
		     _("line %d: Bad BuildArchitecture format: %s\n"),
		     spec->lineNum, spec->line);
	    return RPMRC_FAIL;
	}
	if (spec->toplevel) {
	    if (BACount > 0 && BANames != NULL) {
		spec->BACount = BACount;
		spec->BANames = BANames;
		BANames = NULL;		/* XXX don't free. */
	    }
	} else {
	    if (BACount != 1 || strcmp(BANames[0], "noarch")) {
		rpmlog(RPMLOG_ERR,
		     _("line %d: Only \"noarch\" sub-packages are supported: %s\n"),
		     spec->lineNum, spec->line);
		BANames = _free(BANames);
		return RPMRC_FAIL;
	    }
	    pkg->noarch = 1;
	}
	BANames = _free(BANames);
    }	break;

    default:
	macro = NULL;
	he->tag = tag;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv= (const char **) &field;	/* XXX NOCAST */
	he->c = 1;
	he->append = 1;
	xx = headerPut(pkg->header, he, 0);
	he->append = 0;
	break;
    }

/*@-usereleased@*/
    if (macro)
	addMacro(spec->macros, macro, NULL, field, RMIL_SPEC);
/*@=usereleased@*/
    
    return RPMRC_OK;
}

/* This table has to be in a peculiar order.  If one tag is the */
/* same as another, plus a few letters, it must come first.     */

/**
 */
typedef struct PreambleRec_s {
    rpmTag tag;
    int multiLang;
    int obsolete;
/*@observer@*/ /*@null@*/
    const char * token;
} * PreambleRec;

/*@unchecked@*/
static struct PreambleRec_s preambleList[] = {
    {RPMTAG_NAME,		0, 0, "name"},
    {RPMTAG_VERSION,		0, 0, "version"},
    {RPMTAG_RELEASE,		0, 0, "release"},
    {RPMTAG_DISTEPOCH,		0, 0, "distepoch"},
    {RPMTAG_EPOCH,		0, 0, "epoch"},
    {RPMTAG_EPOCH,		0, 1, "serial"},
    {RPMTAG_SUMMARY,		1, 0, "summary"},
    {RPMTAG_LICENSE,		0, 0, "copyright"},
    {RPMTAG_LICENSE,		0, 0, "license"},
    {RPMTAG_DISTRIBUTION,	0, 0, "distribution"},
    {RPMTAG_DISTURL,		0, 0, "disturl"},
    {RPMTAG_VENDOR,		0, 0, "vendor"},
    {RPMTAG_GROUP,		1, 0, "group"},
    {RPMTAG_PACKAGER,		0, 0, "packager"},
    {RPMTAG_URL,		0, 0, "url"},
    {RPMTAG_SOURCE,		0, 0, "source"},
    {RPMTAG_PATCH,		0, 0, "patch"},
    {RPMTAG_NOSOURCE,		0, 0, "nosource"},
    {RPMTAG_NOPATCH,		0, 0, "nopatch"},
    {RPMTAG_EXCLUDEARCH,	0, 0, "excludearch"},
    {RPMTAG_EXCLUSIVEARCH,	0, 0, "exclusivearch"},
    {RPMTAG_EXCLUDEOS,		0, 0, "excludeos"},
    {RPMTAG_EXCLUSIVEOS,	0, 0, "exclusiveos"},
    {RPMTAG_ICON,		0, 0, "icon"},
    {RPMTAG_PROVIDEFLAGS,	0, 0, "provides"},
    {RPMTAG_REQUIREFLAGS,	1, 0, "requires"},
    {RPMTAG_PREREQ,		1, 0, "prereq"},
    {RPMTAG_CONFLICTFLAGS,	0, 0, "conflicts"},
    {RPMTAG_OBSOLETEFLAGS,	0, 0, "obsoletes"},
    {RPMTAG_PREFIXES,		0, 0, "prefixes"},
    {RPMTAG_PREFIXES,		0, 0, "prefix"},
    {RPMTAG_BUILDROOT,		0, 0, "buildroot"},
    {RPMTAG_BUILDARCHS,		0, 0, "buildarchitectures"},
    {RPMTAG_BUILDARCHS,		0, 0, "buildarch"},
    {RPMTAG_BUILDCONFLICTS,	0, 0, "buildconflicts"},
    {RPMTAG_BUILDOBSOLETES,	0, 0, "buildobsoletes"},
    {RPMTAG_BUILDPREREQ,	1, 0, "buildprereq"},
    {RPMTAG_BUILDPROVIDES,	0, 0, "buildprovides"},
    {RPMTAG_BUILDREQUIRES,	1, 0, "buildrequires"},
    {RPMTAG_AUTOREQPROV,	0, 0, "autoreqprov"},
    {RPMTAG_AUTOREQ,		0, 0, "autoreq"},
    {RPMTAG_AUTOPROV,		0, 0, "autoprov"},
    {RPMTAG_DOCDIR,		0, 0, "docdir"},
    {RPMTAG_DISTTAG,		0, 0, "disttag"},
    {RPMTAG_BUGURL,		0, 0, "bugurl"},
    {RPMTAG_CVSID,		0, 0, "cvsid"},
    {RPMTAG_SVNID,		0, 0, "svnid"},
    {RPMTAG_SUGGESTSFLAGS,	0, 0, "suggests"},
    {RPMTAG_ENHANCESFLAGS,	0, 0, "enhances"},
    {RPMTAG_BUILDSUGGESTS,	0, 0, "buildsuggests"},
    {RPMTAG_BUILDENHANCES,	0, 0, "buildenhances"},
    {RPMTAG_VARIANTS,		0, 0, "variants"},
    {RPMTAG_VARIANTS,		0, 0, "variant"},
    {RPMTAG_XMAJOR,		0, 0, "xmajor"},
    {RPMTAG_XMINOR,		0, 0, "xminor"},
    {RPMTAG_REPOTAG,		0, 0, "repotag"},
    {RPMTAG_KEYWORDS,		0, 0, "keywords"},
    {RPMTAG_KEYWORDS,		0, 0, "keyword"},
    {RPMTAG_BUILDPLATFORMS,	0, 0, "buildplatforms"},
#if defined(RPM_VENDOR_OPENPKG) /* make-class-available-as-macro */
    {RPMTAG_CLASS,		0, 0, "class"},
#endif
    /*@-nullassign@*/	/* LCL: can't add null annotation */
    {0, 0, 0, 0}
    /*@=nullassign@*/
};

/**
 */
static int findPreambleTag(Spec spec, /*@out@*/rpmTag * tagp,
		/*@null@*/ /*@out@*/ const char ** macro, /*@out@*/ char * lang)
	/*@modifies *tagp, *macro, *lang @*/
{
    PreambleRec p;
    char *s;
    size_t len = 0;

    /* Search for defined tags. */
    for (p = preambleList; p->token != NULL; p++) {
	len = strlen(p->token);
	if (!(p->token && !xstrncasecmp(spec->line, p->token, len)))
	    continue;
	if (p->obsolete) {
	    rpmlog(RPMLOG_ERR, _("Legacy syntax is unsupported: %s\n"),
			p->token);
	    p = NULL;
	}
	break;
    }
    if (p == NULL)
	return 1;

    /* Search for arbitrary tags. */
    if (tagp && p->token == NULL) {
	ARGV_t aTags = NULL;
	int rc = 1;	/* assume failure */

/*@-noeffect@*/
	(void) tagName(0); /* XXX force arbitrary tags to be initialized. */
/*@=noeffect@*/
	aTags = rpmTags->aTags;
	if (aTags != NULL && aTags[0] != NULL) {
	    ARGV_t av;
	    s = tagCanonicalize(spec->line);
#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
	    av = argvSearchLinear(aTags, s, argvFnmatchCasefold);
#else
	    av = argvSearch(aTags, s, argvStrcasecmp);
#endif
	    if (av != NULL) {
		*tagp = tagGenerate(s);
		rc = 0;
	    }
	    s = _free(s);
	}
	return rc;
    }

    s = spec->line + len;
    SKIPSPACE(s);

    switch (p->multiLang) {
    default:
    case 0:
	/* Unless this is a source or a patch, a ':' better be next */
	if (p->tag != RPMTAG_SOURCE && p->tag != RPMTAG_PATCH) {
	    if (*s != ':') return 1;
	}
	*lang = '\0';
	break;
    case 1:	/* Parse optional ( <token> ). */
	if (*s == ':') {
	    strcpy(lang, RPMBUILD_DEFAULT_LANG);
	    break;
	}
	if (*s != '(') return 1;
	s++;
	SKIPSPACE(s);
	while (!xisspace(*s) && *s != ')')
	    *lang++ = *s++;
	*lang = '\0';
	SKIPSPACE(s);
	if (*s != ')') return 1;
	s++;
	SKIPSPACE(s);
	if (*s != ':') return 1;
	break;
    }

    if (tagp)
	*tagp = p->tag;
    if (macro)
	/*@-onlytrans -observertrans -dependenttrans@*/	/* FIX: double indirection. */
	*macro = p->token;
	/*@=onlytrans =observertrans =dependenttrans@*/
    return 0;
}

/* XXX should return rpmParseState, but RPMRC_FAIL forces int return. */
int parsePreamble(Spec spec, int initialPackage)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmParseState nextPart;
    int xx;
    char *linep;
    Package pkg;
    char NVR[BUFSIZ];
    char lang[BUFSIZ];
    rpmRC rc;

    strcpy(NVR, "(main package)");

    pkg = newPackage(spec);
    if (spec->packages == NULL) {
	spec->packages = pkg;
assert(initialPackage);
    } else if (! initialPackage) {
	char *name = NULL;
	rpmParseState flag;
	Package lastpkg;

	/* There is one option to %package: <pkg> or -n <pkg> */
	flag = PART_NONE;
	if (parseSimplePart(spec, &name, &flag)) {
	    rpmlog(RPMLOG_ERR, _("Bad package specification: %s\n"),
			spec->line);
	    name = _free(name);
	    pkg = freePackages(pkg);
	    return RPMRC_FAIL;
	}
	
	lastpkg = NULL;
	if (lookupPackage(spec, name, flag, &lastpkg) == RPMRC_OK) {
	    pkg->next = lastpkg->next;
	} else {
	    /* Add package to end of list */
	    for (lastpkg = spec->packages; lastpkg->next != NULL; lastpkg = lastpkg->next)
	    {};
	}
assert(lastpkg != NULL);
	lastpkg->next = pkg;
	
	/* Construct the package */
	if (flag == PART_SUBNAME) {
	    he->tag = RPMTAG_NAME;
	    xx = headerGet(spec->packages->header, he, 0);
	    sprintf(NVR, "%s-%s", he->p.str, name);
	    he->p.ptr = _free(he->p.ptr);
	} else {
	    strncpy(NVR, name, sizeof(NVR)-1);
	    NVR[sizeof(NVR)-1] = '\0';
	}
	name = _free(name);
	he->tag = RPMTAG_NAME;
	he->t = RPM_STRING_TYPE;
	he->p.str = NVR;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);
    }

    if ((rc = readLine(spec, STRIP_TRAILINGSPACE | STRIP_COMMENTS)) > 0) {
	nextPart = PART_NONE;
    } else {
	if (rc)
	    return rc;
	while ((nextPart = isPart(spec)) == PART_NONE) {
	    const char * macro = NULL;
	    rpmTag tag = 0;

	    /* Skip blank lines */
	    linep = spec->line;
	    SKIPSPACE(linep);
	    if (*linep != '\0') {
		if (findPreambleTag(spec, &tag, &macro, lang)) {
		    rpmlog(RPMLOG_ERR, _("line %d: Unknown tag: %s\n"),
				spec->lineNum, spec->line);
		    return RPMRC_FAIL;
		}
		if (handlePreambleTag(spec, pkg, tag, macro, lang))
		    return RPMRC_FAIL;
		if (spec->BANames && !spec->recursing && spec->toplevel)
		    return PART_BUILDARCHITECTURES;
	    }
	    if ((rc =
		 readLine(spec, STRIP_TRAILINGSPACE | STRIP_COMMENTS)) > 0) {
		nextPart = PART_NONE;
		break;
	    }
	    if (rc)
		return rc;
	}
    }

    /* Do some final processing on the header */

    /* 
     * Expand buildroot one more time to get %{version} and the like
     * from the main package.
     */
    if (initialPackage) {
        const char *s = rpmExpand("%{?buildroot}", NULL);
	if (s && *s) 
	    (void) addMacro(NULL, "buildroot", NULL, s, -1);
	s = _free(s);
    }
    
    /* XXX Skip valid arch check if not building binary package */
    if (!spec->anyarch && checkForValidArchitectures(spec))
	return RPMRC_FAIL;

    if (pkg == spec->packages)
	fillOutMainPackage(pkg->header);

    if (checkForDuplicates(pkg->header, NVR) != RPMRC_OK)
	return RPMRC_FAIL;

    if (pkg != spec->packages)
	headerCopyTags(spec->packages->header, pkg->header,
			(void *)copyTagsDuringParse);

#ifdef RPM_VENDOR_PLD /* rpm-epoch0 */
    /* Add Epoch: 0 to package header if it was not set by spec */
    he->tag = RPMTAG_NAME;
    if (headerGet(spec->packages->header, he, 0) == 0) {
    	rpmuint32_t num = 0;

	he->tag = RPMTAG_EPOCH;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &num;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);

	/* also declare %{epoch} to be same */
	addMacro(spec->macros, "epoch", NULL, "0", RMIL_SPEC);
    }
#endif /* RPM_VENDOR_PLD rpm-epoch0 */

    if (checkForRequired(pkg->header, NVR) != RPMRC_OK)
	return RPMRC_FAIL;

    return nextPart;
}
