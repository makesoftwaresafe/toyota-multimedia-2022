/** \ingroup rpmbuild
 * \file build/parseSpec.c
 *  Top level dispatcher for spec file parsing.
 */

#include "system.h"

#include <rpmio_internal.h>	/* XXX fdGetFp */
#include <rpmcb.h>
#include <argv.h>

#define	_RPMTAG_INTERNAL	/* XXX rpmTags->aTags */
#include <rpmbuild.h>
#include "rpmds.h"
#include "rpmts.h"
#include "debug.h"

/*@access headerTagIndices @*/
/*@access FD_t @*/	/* compared with NULL */

/**
 */
/*@unchecked@*/
static struct PartRec {
    rpmParseState part;
    size_t len;
/*@observer@*/ /*@null@*/
    const char * token;
} partList[] = {
    { PART_PREAMBLE,      0, "%package"},
    { PART_PREP,          0, "%prep"},
    { PART_BUILD,         0, "%build"},
    { PART_INSTALL,       0, "%install"},
    { PART_CHECK,         0, "%check"},
    { PART_CLEAN,         0, "%clean"},
    { PART_PREUN,         0, "%preun"},
    { PART_POSTUN,        0, "%postun"},
    { PART_PRETRANS,      0, "%pretrans"},
    { PART_POSTTRANS,     0, "%posttrans"},
    { PART_PRE,           0, "%pre"},
    { PART_POST,          0, "%post"},
    { PART_FILES,         0, "%files"},
    { PART_CHANGELOG,     0, "%changelog"},
    { PART_DESCRIPTION,   0, "%description"},
    { PART_TRIGGERPOSTUN, 0, "%triggerpostun"},
    { PART_TRIGGERPREIN,  0, "%triggerprein"},
    { PART_TRIGGERUN,     0, "%triggerun"},
    { PART_TRIGGERIN,     0, "%triggerin"},
    { PART_TRIGGERIN,     0, "%trigger"},
    { PART_VERIFYSCRIPT,  0, "%verifyscript"},
    { PART_SANITYCHECK,	  0, "%sanitycheck"},	/* support "%sanitycheck" scriptlet */
    {0, 0, NULL}
};

/**
 */
static inline void initParts(struct PartRec *p)
	/*@modifies p->len @*/
{
    for (; p->token != NULL; p++)
	p->len = strlen(p->token);
}

rpmParseState isPart(Spec spec)
{
    const char * line = spec->line;
    struct PartRec *p;
    rpmParseState nextPart = PART_NONE;	/* assume plain text */

    if (partList[0].len == 0)
	initParts(partList);
    
    for (p = partList; p->token != NULL; p++) {
	char c;
	if (xstrncasecmp(line, p->token, p->len))
	    continue;
	c = *(line + p->len);
	if (c == '\0' || xisspace(c)) {
	    nextPart = p->part;
	    goto exit;
	}
    }

    /* If %foo is not found explicitly, check for an arbitrary %foo tag. */
    if (line[0] == '%') {
	ARGV_t aTags = NULL;
	const char * s;
/*@-noeffect@*/
        (void) tagName(0); /* XXX force arbitrary tags to be initialized. */
/*@=noeffect@*/
        aTags = rpmTags->aTags;
        if (aTags != NULL && aTags[0] != NULL) {
            ARGV_t av;
            s = tagCanonicalize(line+1);	/* XXX +1 to skip leading '%' */
#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
            av = argvSearchLinear(aTags, s, argvFnmatchCasefold);
#else
            av = argvSearch(aTags, s, argvStrcasecmp);
#endif
            if (av != NULL) {
		spec->foo = xrealloc(spec->foo, (spec->nfoo + 1) * sizeof(*spec->foo));
		spec->foo[spec->nfoo].str = xstrdup(s);
		spec->foo[spec->nfoo].tag = tagGenerate(s);
		spec->foo[spec->nfoo].iob = NULL;
		spec->nfoo++;
                nextPart = PART_ARBITRARY;
	    }
            s = _free(s);
        }
    }

exit:
    return nextPart;
}

/**
 */
static int matchTok(const char *token, const char *line)
	/*@*/
{
    const char *b, *be = line;
    size_t toklen = strlen(token);
    int rc = 0;

    while ( *(b = be) != '\0' ) {
	SKIPSPACE(b);
	be = b;
	SKIPNONSPACE(be);
	if (be == b)
	    break;
	if (toklen != (size_t)(be-b) || xstrncasecmp(token, b, (be-b)))
	    continue;
	rc = 1;
	break;
    }

    return rc;
}

void handleComments(char *s)
{
    SKIPSPACE(s);
    if (*s == '#')
	*s = '\0';
}

/**
 */
static void forceIncludeFile(Spec spec, const char * fileName)
	/*@modifies spec->fileStack @*/
{
    OFI_t * ofi;

    ofi = newOpenFileInfo();
    ofi->fileName = xstrdup(fileName);
    ofi->next = spec->fileStack;
    spec->fileStack = ofi;
}

/**
 */
static int restoreFirstChar(Spec spec)
	/*@modifies spec->nextline, spec->nextpeekc @*/
{
    /* Restore 1st char in (possible) next line */
    if (spec->nextline != NULL && spec->nextpeekc != '\0') {
	*spec->nextline = spec->nextpeekc;
	spec->nextpeekc = '\0';
	return 1;
    }
    return 0;
}

/**
 */
static int copyNextLineFromOFI(Spec spec, OFI_t * ofi, rpmStripFlags strip)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies spec->nextline, spec->lbuf, spec->lbufPtr,
		ofi->readPtr,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    char ch;

    /* Expand next line from file into line buffer */
    if (!(spec->nextline && *spec->nextline)) {
	int pc = 0, bc = 0, nc = 0;
	char *from, *to, *p;
	to = spec->lbufPtr ? spec->lbufPtr : spec->lbuf;
	from = ofi->readPtr;
	ch = ' ';
	while (from && *from && ch != '\n')
	    ch = *to++ = *from++;
/*@-mods@*/
	spec->lbufPtr = to;
/*@=mods@*/
	*to++ = '\0';
	ofi->readPtr = from;

	/* Check if we need another line before expanding the buffer. */
	for (p = spec->lbuf; *p; p++) {
	    switch (*p) {
		case '\\':
		    switch (*(p+1)) {
			case '\n': p++, nc = 1; /*@innerbreak@*/ break;
			case '\0': /*@innerbreak@*/ break;
			default: p++; /*@innerbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '\n': nc = 0; /*@switchbreak@*/ break;
		case '%':
		    switch (*(p+1)) {
			case '{': p++, bc++; /*@innerbreak@*/ break;
			case '(': p++, pc++; /*@innerbreak@*/ break;
			case '%': p++; /*@innerbreak@*/ break;
		    }
		    /*@switchbreak@*/ break;
		case '{': if (bc > 0) bc++; /*@switchbreak@*/ break;
		case '}': if (bc > 0) bc--; /*@switchbreak@*/ break;
		case '(': if (pc > 0) pc++; /*@switchbreak@*/ break;
		case ')': if (pc > 0) pc--; /*@switchbreak@*/ break;
	    }
	}
	
	/* If it doesn't, ask for one more line. We need a better
	 * error code for this. */
	if (pc || bc || nc ) {
/*@-observertrans -readonlytrans@*/
	    spec->nextline = "";
/*@=observertrans =readonlytrans@*/
	    return RPMRC_FAIL;
	}
/*@-mods@*/
	spec->lbufPtr = spec->lbuf;
/*@=mods@*/

	/* Don't expand macros (eg. %define) in false branch of %if clause */
	/* Also don't expand macros in %changelog if STRIP_NOEXPAND is set */
	/* (first line is omitted, so %date macro will be expanded */
      if (!(strip & STRIP_NOEXPAND)) {
	if (spec->readStack->reading &&
	    expandMacros(spec, spec->macros, spec->lbuf, spec->lbuf_len)) {
		rpmlog(RPMLOG_ERR, _("line %d: %s\n"),
			spec->lineNum, spec->lbuf);
		return RPMRC_FAIL;
	}
      }
	spec->nextline = spec->lbuf;
    }
    return 0;
}

/**
 */
static int copyNextLineFinish(Spec spec, int strip)
	/*@modifies spec->line, spec->nextline, spec->nextpeekc @*/
{
    char *last;
    char ch;

    /* Find next line in expanded line buffer */
    spec->line = last = spec->nextline;
    ch = ' ';
    while (*spec->nextline && ch != '\n') {
	ch = *spec->nextline++;
	if (!xisspace(ch))
	    last = spec->nextline;
    }

    /* Save 1st char of next line in order to terminate current line. */
    if (*spec->nextline != '\0') {
	spec->nextpeekc = *spec->nextline;
	*spec->nextline = '\0';
    }
    
    if (strip & STRIP_COMMENTS)
	handleComments(spec->line);
    
    if (strip & STRIP_TRAILINGSPACE)
	*last = '\0';

    return 0;
}

/**
 */
static int readLineFromOFI(Spec spec, OFI_t *ofi)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies ofi, spec->fileStack, spec->lineNum, spec->sl,
 		fileSystem, internalState @*/
{
retry:
    /* Make sure the current file is open */
    if (ofi->fd == NULL) {
	ofi->fd = Fopen(ofi->fileName, "r.fpio");
	if (ofi->fd == NULL || Ferror(ofi->fd)) {
	    /* XXX Fstrerror */
	    rpmlog(RPMLOG_ERR, _("Unable to open %s: %s\n"),
		     ofi->fileName, Fstrerror(ofi->fd));
	    return RPMRC_FAIL;
	}
	spec->lineNum = ofi->lineNum = 0;
    }

    /* Make sure we have something in the read buffer */
    if (!(ofi->readPtr && *(ofi->readPtr))) {
	/*@-type@*/ /* FIX: cast? */
	FILE * f = fdGetFp(ofi->fd);
	/*@=type@*/
	if (f == NULL || !fgets(ofi->readBuf, (int)sizeof(ofi->readBuf), f)) {
	    /* EOF */
	    if (spec->readStack->next) {
		rpmlog(RPMLOG_ERR, _("Unclosed %%if\n"));
	        return RPMRC_FAIL;
	    }

	    /* remove this file from the stack */
	    spec->fileStack = ofi->next;
	    (void) Fclose(ofi->fd);
	    ofi->fileName = _free(ofi->fileName);
/*@-temptrans@*/
	    ofi = _free(ofi);
/*@=temptrans@*/

	    /* only on last file do we signal EOF to caller */
	    ofi = spec->fileStack;
	    if (ofi == NULL)
		return 1;

	    /* otherwise, go back and try the read again. */
	    goto retry;
	}
	ofi->readPtr = ofi->readBuf;
	ofi->lineNum++;
	spec->lineNum = ofi->lineNum;
	if (spec->sl) {
	    speclines sl = spec->sl;
	    if (sl->sl_nlines == sl->sl_nalloc) {
		sl->sl_nalloc += 100;
		sl->sl_lines = (char **) xrealloc(sl->sl_lines, 
			sl->sl_nalloc * sizeof(*(sl->sl_lines)));
	    }
	    sl->sl_lines[sl->sl_nlines++] = xstrdup(ofi->readBuf);
	}
    }
    return 0;
}

int readLine(Spec spec, rpmStripFlags strip)
{
    char  *s;
    int match;
    struct ReadLevelEntry *rl;
    OFI_t *ofi = spec->fileStack;
    int rc;

    if (ofi == NULL)	/* XXX segfault avoidance */
	return 1;
    if (!restoreFirstChar(spec)) {
    retry:
      if ((rc = readLineFromOFI(spec, ofi)) != 0)
        return rc;

      /* Copy next file line into the spec line buffer */

      if ((rc = copyNextLineFromOFI(spec, ofi, strip)) != 0) {
	if (rc == RPMRC_FAIL)
	    goto retry;
	return rc;
      }
    }

    (void) copyNextLineFinish(spec, strip);

    s = spec->line;
    SKIPSPACE(s);

    match = -1;
  if (!(strip & STRIP_NOEXPAND)) {
    if (!spec->readStack->reading && !strncmp("%if", s, sizeof("%if")-1)) {
	match = 0;
    } else if (! strncmp("%ifarch", s, sizeof("%ifarch")-1)) {
	const char *arch = rpmExpand("%{_target_cpu}", NULL);
	s += 7;
	match = matchTok(arch, s);
	arch = _free(arch);
    } else if (! strncmp("%ifnarch", s, sizeof("%ifnarch")-1)) {
	const char *arch = rpmExpand("%{_target_cpu}", NULL);
	s += 8;
	match = !matchTok(arch, s);
	arch = _free(arch);
    } else if (! strncmp("%ifos", s, sizeof("%ifos")-1)) {
	const char *os = rpmExpand("%{_target_os}", NULL);
	s += 5;
	match = matchTok(os, s);
	os = _free(os);
    } else if (! strncmp("%ifnos", s, sizeof("%ifnos")-1)) {
	const char *os = rpmExpand("%{_target_os}", NULL);
	s += 6;
	match = !matchTok(os, s);
	os = _free(os);
    } else if (! strncmp("%if", s, sizeof("%if")-1)) {
	s += 3;
        match = parseExpressionBoolean(spec, s);
	if (match < 0) {
	    rpmlog(RPMLOG_ERR,
			_("%s:%d: parseExpressionBoolean returns %d\n"),
			ofi->fileName, ofi->lineNum, match);
	    return RPMRC_FAIL;
	}
    } else if (! strncmp("%else", s, sizeof("%else")-1)) {
	s += 5;
	if (! spec->readStack->next) {
	    /* Got an else with no %if ! */
	    rpmlog(RPMLOG_ERR,
			_("%s:%d: Got a %%else with no %%if\n"),
			ofi->fileName, ofi->lineNum);
	    return RPMRC_FAIL;
	}
	spec->readStack->reading =
	    spec->readStack->next->reading && ! spec->readStack->reading;
	spec->line[0] = '\0';
    } else if (! strncmp("%endif", s, sizeof("%endif")-1)) {
	s += 6;
	if (! spec->readStack->next) {
	    /* Got an end with no %if ! */
	    rpmlog(RPMLOG_ERR,
			_("%s:%d: Got a %%endif with no %%if\n"),
			ofi->fileName, ofi->lineNum);
	    return RPMRC_FAIL;
	}
	rl = spec->readStack;
	spec->readStack = spec->readStack->next;
	free(rl);
	spec->line[0] = '\0';
    } else if (spec->readStack->reading && ! strncmp("%include", s, sizeof("%include")-1)) {
	char *fileName, *endFileName, *p;

	s += 8;
	fileName = s;
	if (! xisspace(*fileName)) {
	    rpmlog(RPMLOG_ERR, _("malformed %%include statement\n"));
	    return RPMRC_FAIL;
	}
	SKIPSPACE(fileName);
	endFileName = fileName;
	SKIPNONSPACE(endFileName);
	p = endFileName;
	SKIPSPACE(p);
	if (*p != '\0') {
	    rpmlog(RPMLOG_ERR, _("malformed %%include statement\n"));
	    return RPMRC_FAIL;
	}
	*endFileName = '\0';

	forceIncludeFile(spec, fileName);

	ofi = spec->fileStack;
	goto retry;
    }
  }

    if (match != -1) {
	rl = xmalloc(sizeof(*rl));
	rl->reading = spec->readStack->reading && match;
	rl->next = spec->readStack;
	spec->readStack = rl;
	spec->line[0] = '\0';
    }

    if (! spec->readStack->reading) {
	spec->line[0] = '\0';
    }

    /*@-compmempass@*/ /* FIX: spec->readStack->next should be dependent */
    return 0;
    /*@=compmempass@*/
}

void closeSpec(Spec spec)
{
    OFI_t *ofi;

    while (spec->fileStack) {
	ofi = spec->fileStack;
	spec->fileStack = spec->fileStack->next;
	if (ofi->fd) (void) Fclose(ofi->fd);
	ofi->fileName = _free(ofi->fileName);
	ofi = _free(ofi);
    }
}

/**
 */
static inline int genSourceRpmName(Spec spec)
	/*@globals internalState @*/
	/*@modifies spec->sourceRpmName, spec->packages->header,
		internalState @*/
{
    if (spec->sourceRpmName == NULL) {
	const char *N, *V, *R;
	char fileName[BUFSIZ];

	(void) headerNEVRA(spec->packages->header, &N, NULL, &V, &R, NULL);
	(void) snprintf(fileName, sizeof(fileName), "%s-%s-%s.%ssrc.rpm",
			N, V, R, spec->noSource ? "no" : "");
	fileName[sizeof(fileName)-1] = '\0';
	N = _free(N);
	V = _free(V);
	R = _free(R);
	spec->sourceRpmName = xstrdup(fileName);
    }

    return 0;
}

/*@-redecl@*/
/*@unchecked@*/
extern int noLang;		/* XXX FIXME: pass as arg */
/*@=redecl@*/

/*@todo Skip parse recursion if os is not compatible. @*/
int parseSpec(rpmts ts, const char *specFile, const char *rootURL,
		int recursing, const char *passPhrase,
		const char *cookie, int anyarch, int force, int verify)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmParseState parsePart = PART_PREAMBLE;
    int initialPackage = 1;
    Package pkg;
    Spec spec;
    int xx;
    
    /* Set up a new Spec structure with no packages. */
    spec = newSpec();

    /*
     * Note: rpmGetPath should guarantee a "canonical" path. That means
     * that the following pathologies should be weeded out:
     *          //bin//sh
     *          //usr//bin/
     *          /.././../usr/../bin//./sh (XXX FIXME: dots not handled yet)
     */
    spec->specFile = rpmGetPath(specFile, NULL);
    spec->fileStack = newOpenFileInfo();
    spec->fileStack->fileName = xstrdup(spec->specFile);

    spec->recursing = recursing;
    spec->toplevel = (!recursing ? 1 : 0);
    spec->anyarch = anyarch;
    spec->force = force;

    if (rootURL)
	spec->rootURL = xstrdup(rootURL);
    if (passPhrase)
	spec->passPhrase = xstrdup(passPhrase);
    if (cookie)
	spec->cookie = xstrdup(cookie);

    spec->timeCheck = rpmExpandNumeric("%{_timecheck}");

    /* XXX %_docdir should be set somewhere else. */
    addMacro(NULL, "_docdir", NULL, "%{_defaultdocdir}", RMIL_SPEC);

    /* All the parse*() functions expect to have a line pre-read */
    /* in the spec's line buffer.  Except for parsePreamble(),   */
    /* which handles the initial entry into a spec file.         */
    
    /*@-infloops@*/	/* LCL: parsePart is modified @*/
    while (parsePart > PART_NONE) {
	int goterror = 0;

	switch (parsePart) {
	default:
	    goterror = 1;
	    /*@switchbreak@*/ break;
	case PART_PREAMBLE:
	    parsePart = parsePreamble(spec, initialPackage);
	    initialPackage = 0;
	    /*@switchbreak@*/ break;
	case PART_PREP:
	    parsePart = parsePrep(spec, verify);
	    /*@switchbreak@*/ break;
	case PART_BUILD:
	case PART_INSTALL:
	case PART_CHECK:
	case PART_CLEAN:
	case PART_ARBITRARY:
	    parsePart = parseBuildInstallClean(spec, parsePart);
	    /*@switchbreak@*/ break;
	case PART_CHANGELOG:
	    parsePart = parseChangelog(spec);
	    /*@switchbreak@*/ break;
	case PART_DESCRIPTION:
	    parsePart = parseDescription(spec);
	    /*@switchbreak@*/ break;

	case PART_PRE:
	case PART_POST:
	case PART_PREUN:
	case PART_POSTUN:
	case PART_PRETRANS:
	case PART_POSTTRANS:
	case PART_VERIFYSCRIPT:
	case PART_SANITYCHECK:
	case PART_TRIGGERPREIN:
	case PART_TRIGGERIN:
	case PART_TRIGGERUN:
	case PART_TRIGGERPOSTUN:
	    parsePart = parseScript(spec, parsePart);
	    /*@switchbreak@*/ break;

	case PART_FILES:
	    parsePart = parseFiles(spec);
	    /*@switchbreak@*/ break;

	case PART_NONE:		/* XXX avoid gcc whining */
	case PART_LAST:
	case PART_BUILDARCHITECTURES:
	    /*@switchbreak@*/ break;
	}

	if (goterror || parsePart >= PART_LAST) {
	    spec = freeSpec(spec);
	    return parsePart;
	}

	/* Detect whether BuildArch: is toplevel or within %package. */
	if (spec->toplevel && parsePart != PART_BUILDARCHITECTURES)
	    spec->toplevel = 0;

	/* Restart parse iff toplevel BuildArch: is encountered. */
	if (spec->toplevel && parsePart == PART_BUILDARCHITECTURES) {
	    int index;
	    int x;

	    closeSpec(spec);

	    /* LCL: sizeof(spec->BASpecs[0]) -nullderef whine here */
	    spec->BASpecs = xcalloc(spec->BACount, sizeof(*spec->BASpecs));
	    index = 0;
	    if (spec->BANames != NULL)
	    for (x = 0; x < spec->BACount; x++) {

		/* XXX DIEDIEDIE: filter irrelevant platforms here. */

		/* XXX there's more to do than set the macro. */
		addMacro(NULL, "_target_cpu", NULL, spec->BANames[x], RMIL_RPMRC);
		spec->BASpecs[index] = NULL;
		if (parseSpec(ts, specFile, spec->rootURL, 1,
				  passPhrase, cookie, anyarch, force, verify)
		 || (spec->BASpecs[index] = rpmtsSetSpec(ts, NULL)) == NULL)
		{
			spec->BACount = index;
/*@-nullstate@*/
			spec = freeSpec(spec);
			return RPMRC_FAIL;
/*@=nullstate@*/
		}

		/* XXX there's more to do than delete the macro. */
		delMacro(NULL, "_target_cpu");
		index++;
	    }

	    spec->BACount = index;
	    if (! index) {
		rpmlog(RPMLOG_ERR,
			_("No compatible architectures found for build\n"));
/*@-nullstate@*/
		spec = freeSpec(spec);
		return RPMRC_FAIL;
/*@=nullstate@*/
	    }

	    /*
	     * Return the 1st child's fully parsed Spec structure.
	     * The restart of the parse when encountering BuildArch
	     * causes problems for "rpm -q --specfile". This is
	     * still a hack because there may be more than 1 arch
	     * specified (unlikely but possible.) There's also the
	     * further problem that the macro context, particularly
	     * %{_target_cpu}, disagrees with the info in the header.
	     */
	    if (spec->BACount >= 1) {
		Spec nspec = spec->BASpecs[0];
		spec->BASpecs = _free(spec->BASpecs);
		spec = freeSpec(spec);
		spec = nspec;
	    }

	    (void) rpmtsSetSpec(ts, spec);
	    return 0;
	}
    }
    /*@=infloops@*/	/* LCL: parsePart is modified @*/

    /* Initialize source RPM name. */
    (void) genSourceRpmName(spec);

    /* Check for description in each package and add arch and os */
  {
    const char *platform = rpmExpand("%{_target_platform}", NULL);
    const char *platformNoarch = NULL;
    const char *arch = rpmExpand("%{_target_cpu}", NULL);
    const char *os = rpmExpand("%{_target_os}", NULL);

    for (pkg = spec->packages; pkg != NULL; pkg = pkg->next) {
	he->tag = RPMTAG_OS;
	he->t = RPM_STRING_TYPE;
	/* XXX todo: really need "noos" like pkg->noarch somewhen. */
	he->p.str = os;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);

	he->tag = RPMTAG_ARCH;
	he->t = RPM_STRING_TYPE;
	he->p.str = (pkg->noarch ? "noarch" : arch);
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);

	/* 
	 * If "noarch" subpackages of different arch, we need
	 * to use a separate platform tag for these (mdvbz#61746).
	 */
	if(pkg->noarch && !platformNoarch && strcmp(arch, "noarch")) {
	    addMacro(NULL, "_target_cpu", NULL, "noarch", RMIL_RPMRC);
	    platformNoarch = rpmExpand("%{_target_platform}", NULL);
	    addMacro(NULL, "_target_cpu", NULL, arch, RMIL_RPMRC);
	}
	he->tag = RPMTAG_PLATFORM;
	he->t = RPM_STRING_TYPE;
	he->p.str = (pkg->noarch && platformNoarch ? platformNoarch : platform);
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);

	he->tag = RPMTAG_SOURCERPM;
	he->t = RPM_STRING_TYPE;
	he->p.str = spec->sourceRpmName;
	he->c = 1;
	xx = headerPut(pkg->header, he, 0);

	if (!headerIsEntry(pkg->header, RPMTAG_DESCRIPTION)) {
	    he->tag = RPMTAG_NVRA;
	    xx = headerGet(pkg->header, he, 0);
	    rpmlog(RPMLOG_ERR, _("Package has no %%description: %s\n"),
			he->p.str);
	    he->p.ptr = _free(he->p.ptr);
	    platform = _free(platform);
	    platformNoarch = _free(platformNoarch);
	    arch = _free(arch);
	    os = _free(os);
	    spec = freeSpec(spec);
	    return RPMRC_FAIL;
	}

	pkg->ds = rpmdsThis(pkg->header, RPMTAG_REQUIRENAME, RPMSENSE_EQUAL);

    }

    platform = _free(platform);
    platformNoarch = _free(platformNoarch);
    arch = _free(arch);
    os = _free(os);
  }

    closeSpec(spec);
    (void) rpmtsSetSpec(ts, spec);

    return 0;
}
