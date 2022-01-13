/** \ingroup rpmbuild
 * \file build/parsePrep.c
 *  Parse %prep section from spec file.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <argv.h>
#include <rpmcb.h>
#include <rpmurl.h>
#ifdef	NOTYET
#include <rpmmg.h>
#endif

#include <rpmbuild.h>

#include "misc.h"	/* XXX rpmMkdirPath */
#include "debug.h"

/* These have to be global to make up for stupid compilers */
/*@unchecked@*/
    static int leaveDirs, skipDefaultAction;
/*@unchecked@*/
    static int createDir, quietly;
/*@unchecked@*/ /*@observer@*/ /*@null@*/
    static const char * dirName = NULL;
/*@unchecked@*/ /*@observer@*/
    static struct poptOption optionsTable[] = {
	    { NULL, 'a', POPT_ARG_STRING, NULL, 'a',	NULL, NULL},
	    { NULL, 'b', POPT_ARG_STRING, NULL, 'b',	NULL, NULL},
	    { NULL, 'c', 0, &createDir, 0,		NULL, NULL},
	    { NULL, 'D', 0, &leaveDirs, 0,		NULL, NULL},
	    { NULL, 'n', POPT_ARG_STRING, &dirName, 0,	NULL, NULL},
	    { NULL, 'T', 0, &skipDefaultAction, 0,	NULL, NULL},
	    { NULL, 'q', 0, &quietly, 0,		NULL, NULL},
	    { 0, 0, 0, 0, 0,	NULL, NULL}
    };

/**
 * Check that file owner and group are known.
 * @param urlfn		file url
 * @return		RPMRC_OK on success
 */
static rpmRC checkOwners(const char * urlfn)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    struct stat sb;

    if (Lstat(urlfn, &sb)) {
	rpmlog(RPMLOG_ERR, _("Bad source: %s: %s\n"),
		urlfn, strerror(errno));
	return RPMRC_FAIL;
    }
    if (!getUname(sb.st_uid) || !getGname(sb.st_gid)) {
	rpmlog(RPMLOG_ERR, _("Bad owner/group: %s\n"), urlfn);
	return RPMRC_FAIL;
    }

    return RPMRC_OK;
}

#ifndef	DYING
/**
 * Expand %patchN macro into %prep scriptlet.
 * @param spec		build info
 * @param c		patch index
 * @param strip		patch level (i.e. patch -p argument)
 * @param db		saved file suffix (i.e. patch --suffix argument)
 * @param reverse	include -R?
 * @param removeEmpties	include -E?
 * @param fuzz		include -F? (fuzz<0 means no)
 * @param subdir	sub-directory (i.e patch -d argument);
 * @return		expanded %patch macro (NULL on error)
 */
/*@observer@*/
static char *doPatch(Spec spec, rpmuint32_t c, int strip, const char *db,
		     int reverse, int removeEmpties, int fuzz, const char *subdir)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char *fn, *Lurlfn;
    static char buf[BUFSIZ];
    char args[BUFSIZ], *t = args;
    struct Source *sp;
    rpmCompressedMagic compressed = COMPRESSED_NOT;
    int urltype;
    const char *patch, *flags;

    *t = '\0';
    if (db)
	t = stpcpy( stpcpy(t, "-b --suffix "), db);
#if defined(RPM_VENDOR_OPENPKG) /* always-backup-on-patching */
    /* always create backup files in OpenPKG */
    else
	t = stpcpy(t, "-b --suffix .orig ");
#endif
    if (subdir)
	t = stpcpy( stpcpy(t, "-d "), subdir);
    if (fuzz >= 0) {
	t = stpcpy(t, "-F ");
	sprintf(t, "%10.10d", fuzz);
	t += strlen(t);
    }
    if (reverse)
	t = stpcpy(t, " -R");
    if (removeEmpties)
	t = stpcpy(t, " -E");

    for (sp = spec->sources; sp != NULL; sp = sp->next) {
	if ((sp->flags & RPMFILE_PATCH) && (sp->num == c))
	    break;
    }
    if (sp == NULL) {
	rpmlog(RPMLOG_ERR, _("No patch number %d\n"), c);
	return NULL;
    }

    Lurlfn = rpmGenPath(NULL, "%{_patchdir}/", sp->source);

    /* XXX On non-build parse's, file cannot be stat'd or read */
    if (!spec->force && (isCompressed(Lurlfn, &compressed) || checkOwners(Lurlfn))) {
	Lurlfn = _free(Lurlfn);
	return NULL;
    }

    fn = NULL;
    urltype = urlPath(Lurlfn, &fn);
    switch (urltype) {
    case URL_IS_HTTPS:	/* XXX WRONG WRONG WRONG */
    case URL_IS_HTTP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_FTP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_HKP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_MONGO:	/* XXX FIXME */
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
	Lurlfn = _free(Lurlfn);
	return NULL;
	/*@notreached@*/ break;
    }

    patch = rpmGetPath("%{?__patch}", NULL);
    if (!(patch && *patch != '\0')) {
	patch = _free(patch);
        patch = xstrdup("patch");
    }

    flags = rpmExpand("%{?_default_patch_flags}%{!?_default_patch_flags:-s}", NULL);

    if (compressed) {
	const char *zipper;

	switch (compressed) {
	default:
	case COMPRESSED_NOT:	/* XXX can't happen */
	case COMPRESSED_OTHER:
	case COMPRESSED_ZIP:	/* XXX wrong */
	    zipper = "%{__gzip}";
	    break;
	case COMPRESSED_BZIP2:
	    zipper = "%{__bzip2}";
	    break;
	case COMPRESSED_LZOP:
	    zipper = "%{__lzop}";
	    break;
	case COMPRESSED_LZMA:
	    zipper = "%{__lzma}";
	    break;
	case COMPRESSED_XZ:
	    zipper = "%{__xz}";
	    break;
	}
	zipper = rpmGetPath(zipper, NULL);

	sprintf(buf,
		"echo \"Patch #%d (%s):\"\n"
		"%s -d < '%s' | %s -p%d %s %s\n"
		"STATUS=$?\n"
		"if [ $STATUS -ne 0 ]; then\n"
		"  exit $STATUS\n"
		"fi",
		c,
/*@-moduncon@*/
		(const char *) basename((char *)fn),
/*@=moduncon@*/
		zipper,
		fn, patch, strip, args, flags);
	zipper = _free(zipper);
    } else {
	sprintf(buf,
		"echo \"Patch #%d (%s):\"\n"
		"%s -p%d %s %s < '%s'", c,
/*@-moduncon@*/
		(const char *) basename((char *)fn),
/*@=moduncon@*/
		patch, strip, args, flags, fn);
    }

    patch = _free(patch);
    flags = _free(flags);
    Lurlfn = _free(Lurlfn);
    return buf;
}
#endif

/**
 * Expand %setup macro into %prep scriptlet.
 * @param spec		build info
 * @param c		source index
 * @param quietly	should -vv be omitted from tar?
 * @return		expanded %setup macro (NULL on error)
 */
/*@observer@*/
static const char *doUntar(Spec spec, rpmuint32_t c, int quietly)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
    const char *fn, *Lurlfn;
    static char buf[BUFSIZ];
    char taropts[8];
    char *t = NULL;
    struct Source *sp;
    rpmCompressedMagic compressed = COMPRESSED_NOT;
    int urltype;
    const char *tar;
    int rubygem = 0;

    for (sp = spec->sources; sp != NULL; sp = sp->next) {
	if ((sp->flags & RPMFILE_SOURCE) && (sp->num == c)) {
	    break;
	}
    }
    if (sp == NULL) {
	rpmlog(RPMLOG_ERR, _("No source number %d\n"), c);
	return NULL;
    }

    t = strrchr(sp->source, '.');
    if(t && !strcasecmp(t, ".gem"))
	rubygem = 1;

    t = stpcpy(taropts, "-x");
    /*@-internalglobs@*/ /* FIX: shrug */
    if(rpmIsVerbose() && !quietly)
	t = stpcpy(t, "vv");
    if(rubygem)
	t = stpcpy(t, "m");

    t = stpcpy(t, "f");
    /*@=internalglobs@*/

#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    Lurlfn = rpmGenPath(NULL, getSourceDir(sp->flags, sp->source), sp->source);
#else
    Lurlfn = rpmGenPath(NULL, getSourceDir(sp->flags), sp->source);
#endif

    /* XXX On non-build parse's, file cannot be stat'd or read */
    if (!spec->force && (isCompressed(Lurlfn, &compressed) || checkOwners(Lurlfn))) {
	Lurlfn = _free(Lurlfn);
	return NULL;
    }

    fn = NULL;
    urltype = urlPath(Lurlfn, &fn);
    switch (urltype) {
    case URL_IS_HTTPS:	/* XXX WRONG WRONG WRONG */
    case URL_IS_HTTP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_FTP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_HKP:	/* XXX WRONG WRONG WRONG */
    case URL_IS_MONGO:	/* XXX FIXME */
    case URL_IS_PATH:
    case URL_IS_UNKNOWN:
	break;
    case URL_IS_DASH:
	Lurlfn = _free(Lurlfn);
	return NULL;
	/*@notreached@*/ break;
    }
#ifdef	NOTYET
    {	rpmmg mg;
	
_rpmmg_debug = 1;
	mg = rpmmgNew(NULL, 0);
	t = (char *) rpmmgFile(mg, fn);
	mg = rpmmgFree(mg);
fprintf(stderr, "==> %s: %s\n", fn, t);
	t = _free(t);
_rpmmg_debug = 0;
    }
#endif

    tar = rpmGetPath("%{?__tar}", NULL);
    if (!(tar && *tar != '\0')) {
	tar = _free(tar);
        tar = xstrdup("tar");
    }

#if defined(RPM_VENDOR_ARK) /* use-gnu-tar-compression-detection */
/* We leave compression handling for all tar based files up to GNU tar */
    if (compressed == COMPRESSED_ZIP)
#else
    if (compressed != COMPRESSED_NOT)
#endif
    {
	const char *zipper;
	int needtar = 1;

	switch (compressed) {
	case COMPRESSED_NOT:	/* XXX can't happen */
	case COMPRESSED_OTHER:
	    t = "%{__gzip} -dc";
	    break;
	case COMPRESSED_BZIP2:
	    t = "%{__bzip2} -dc";
	    break;
	case COMPRESSED_LZOP:
	    t = "%{__lzop} -dc";
	    break;
	case COMPRESSED_LZMA:
	    t = "%{__lzma} -dc";
	    break;
	case COMPRESSED_XZ:
	    t = "%{__xz} -dc";
	    break;
	case COMPRESSED_LZIP:
	    t = "%{__lzip} -dc";
	    break;
	case COMPRESSED_LRZIP:
	    t = "%{__lrzip} -dqo-";
	    break;
	case COMPRESSED_7ZIP:
	    t = "%{__7zip} x";
	    needtar = 0;
	    break;
	case COMPRESSED_ZIP:
#if defined(RPM_VENDOR_OPENPKG) /* use-bsdtar-for-zip-files */
	    t = "%{__bsdtar} -x -f";
#else
	    if (rpmIsVerbose() && !quietly)
		t = "%{__unzip}";
	    else
		t = "%{__unzip} -qq";
#endif
	    needtar = 0;
	    break;
	}
	zipper = rpmGetPath(t, NULL);
	buf[0] = '\0';
	t = stpcpy(buf, zipper);
	zipper = _free(zipper);
	*t++ = ' ';
	*t++ = '\'';
	t = stpcpy(t, fn);
	*t++ = '\'';
	if (needtar) {
	    t = stpcpy(t, " | ");
	    t = stpcpy(t, tar);
	    t = stpcpy(t, " ");
	    t = stpcpy(t, taropts);
	    t = stpcpy(t, " -");
        }
	t = stpcpy(t,
		"\n"
		"STATUS=$?\n"
		"if [ $STATUS -ne 0 ]; then\n"
		"  exit $STATUS\n"
		"fi");
    } else {
	buf[0] = '\0';
	t = stpcpy(buf, tar);
	t = stpcpy(t, " ");
	t = stpcpy(t, taropts);
	*t++ = ' ';
	t = stpcpy(t, fn);
	if(rubygem) {
	    t = stpcpy(t,
		    "\n"
		    "if [ -f data.tar.gz ]; then\n"
		    "  tar ");
	    t = stpcpy(t, taropts);
	    t = stpcpy(t,
		    " data.tar.gz\n"
		    "fi");
	}
    }

    tar = _free(tar);
    Lurlfn = _free(Lurlfn);
    return buf;
}

/**
 * Parse %setup macro.
 * @todo FIXME: Option -q broken when not immediately after %setup.
 * @param spec		build info
 * @param line		current line from spec file
 * @return		0 on success
 */
static int doSetupMacro(Spec spec, const char * line)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->buildSubdir, spec->macros, spec->prep,
		spec->packages->header,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    char buf[BUFSIZ];
    rpmiob before = NULL;
    rpmiob after = NULL;
    poptContext optCon;
    int argc;
    const char ** argv;
    int arg;
    const char * optArg;
    int rc;
    rpmuint32_t num;
    rpmRC ec = RPMRC_FAIL;	/* XXX assume failure */

    /*@-mods@*/
    leaveDirs = skipDefaultAction = 0;
    createDir = quietly = 0;
    dirName = NULL;
    /*@=mods@*/

    if ((rc = poptParseArgvString(line, &argc, &argv))) {
	rpmlog(RPMLOG_ERR, _("Error parsing %%setup: %s\n"),
			poptStrerror(rc));
	goto exit;
    }

    before = rpmiobNew(0);
    after = rpmiobNew(0);

    optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
    while ((arg = poptGetNextOpt(optCon)) > 0) {
	optArg = poptGetOptArg(optCon);

	/* We only parse -a and -b here */

	if (parseNum(optArg, &num)) {
	    rpmlog(RPMLOG_ERR, _("line %d: Bad arg to %%setup: %s\n"),
		     spec->lineNum, (optArg ? optArg : "???"));
	    optCon = poptFreeContext(optCon);
	    argv = _free(argv);
	    goto exit;
	}

	{   const char *chptr = doUntar(spec, num, quietly);
	    if (chptr == NULL)
		goto exit;

	    (void) rpmiobAppend((arg == 'a' ? after : before), chptr, 1);
	}
    }

    if (arg < -1) {
	rpmlog(RPMLOG_ERR, _("line %d: Bad %%setup option %s: %s\n"),
		 spec->lineNum,
		 poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		 poptStrerror(arg));
	optCon = poptFreeContext(optCon);
	argv = _free(argv);
	goto exit;
    }

    if (dirName) {
	spec->buildSubdir = xstrdup(dirName);
    } else {
	const char *N, *V;
	(void) headerNEVRA(spec->packages->header, &N, NULL, &V, NULL, NULL);
	(void) snprintf(buf, sizeof(buf), "%s-%s", N, V);
	buf[sizeof(buf)-1] = '\0';
	N = _free(N);
	V = _free(V);
	spec->buildSubdir = xstrdup(buf);
    }
    addMacro(spec->macros, "buildsubdir", NULL, spec->buildSubdir, RMIL_SPEC);

    optCon = poptFreeContext(optCon);
    argv = _free(argv);

    /* cd to the build dir */
    {	const char * buildDirURL = rpmGenPath(spec->rootURL, "%{_builddir}", "");
	const char *buildDir;

	(void) urlPath(buildDirURL, &buildDir);
	rc = rpmioMkpath(buildDir, 0755, -1, -1);
	sprintf(buf, "cd '%s'", buildDir);
	spec->prep = rpmiobAppend(spec->prep, buf, 1);
	buildDirURL = _free(buildDirURL);
    }

    /* delete any old sources */
    if (!leaveDirs) {
	sprintf(buf, "rm -rf '%s'", spec->buildSubdir);
	spec->prep = rpmiobAppend(spec->prep, buf, 1);
    }

    /* check if source is a ruby gem */
    {   struct Source *sp;
	for (sp = spec->sources; sp != NULL; sp = sp->next) {
	    if ((sp->flags & RPMFILE_SOURCE) && (sp->num == 0)) {
		break;
	    }
	}
	if (sp != NULL) {
	    char *t = strrchr(sp->source, '.');
	    if(t && !strcasecmp(t, ".gem"))
		createDir = 1;
	}
    }

    /* if necessary, create and cd into the proper dir */
    if (createDir) {
	char *mkdir_p;
	mkdir_p = rpmExpand("%{?__mkdir_p}%{!?__mkdir_p:mkdir -p}", NULL);
	if (!mkdir_p)
	    mkdir_p = xstrdup("mkdir -p");
	sprintf(buf, "%s '%s'\ncd '%s'",
		mkdir_p, spec->buildSubdir, spec->buildSubdir);
	mkdir_p = _free(mkdir_p);
	spec->prep = rpmiobAppend(spec->prep, buf, 1);
    }

    /* do the default action */
   if (!createDir && !skipDefaultAction) {
	const char *chptr = doUntar(spec, 0, quietly);
	if (chptr == NULL)
	    goto exit;
	spec->prep = rpmiobAppend(spec->prep, chptr, 1);
    }

    spec->prep = rpmiobAppend(spec->prep, rpmiobStr(before), 0);

    if (!createDir) {
	sprintf(buf, "cd '%s'", spec->buildSubdir);
	spec->prep = rpmiobAppend(spec->prep, buf, 1);
    }

    if (createDir && !skipDefaultAction) {
	const char * chptr = doUntar(spec, 0, quietly);
	if (chptr == NULL)
	    goto exit;
	spec->prep = rpmiobAppend(spec->prep, chptr, 1);
    }

    spec->prep = rpmiobAppend(spec->prep, rpmiobStr(after), 0);

    /* XXX FIXME: owner & group fixes were conditioned on !geteuid() */
    /* Fix the owner, group, and permissions of the setup build tree */
    {	/*@observer@*/ static const char *fixmacs[] =
		{ "%{_fixowner}", "%{_fixgroup}", "%{_fixperms}", NULL };
	const char ** fm;

	for (fm = fixmacs; *fm; fm++) {
	    const char *fix;
	    fix = rpmExpand(*fm, " .", NULL);
	    if (fix && *fix != '%')
		spec->prep = rpmiobAppend(spec->prep, fix, 1);
	    fix = _free(fix);
	}
    }
    ec = RPMRC_OK;

exit:
    before = rpmiobFree(before);
    after = rpmiobFree(after);
    return ec;
}

#ifndef	DYING
/**
 * Parse %patch line.
 * @param spec		build info
 * @param line		current line from spec file
 * @return		RPMRC_OK on success
 */
static rpmRC doPatchMacro(Spec spec, const char * line)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies spec->prep, rpmGlobalMacroContext,
		fileSystem, internalState  @*/
{
    char *s;
    char *opt_b;
    char *opt_d;
    rpmuint32_t opt_P, opt_p, opt_R, opt_E, opt_F;
    char buf[BUFSIZ], *bp;
    rpmuint32_t patch_nums[1024];  /* XXX - we can only handle 1024 patches! */
    int patch_index, x;

    memset(patch_nums, 0, sizeof(patch_nums));
    opt_P = opt_p = opt_R = opt_E = 0;
    opt_F = rpmExpandNumeric("%{?_default_patch_fuzz}%{!?_default_patch_fuzz:-1}");
    opt_b = NULL;
    opt_d = NULL;
    patch_index = 0;

    if (! strchr(" \t\n", line[6])) {
	/* %patchN */
	sprintf(buf, "%%patch -P %s", line + 6);
    } else {
	strncpy(buf, line, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';
    }

    /*@-internalglobs@*/	/* FIX: strtok has state */
    for (bp = buf; (s = strtok(bp, " \t\n")) != NULL;) {
	if (bp) {	/* remove 1st token (%patch) */
	    bp = NULL;
	    continue;
	}
	if (!strcmp(s, "-P")) {
	    opt_P = 1;
	} else if (!strcmp(s, "-R")) {
	    opt_R = 1;
	} else if (!strcmp(s, "-E")) {
	    opt_E = 1;
	} else if (!strcmp(s, "-b")) {
	    /* orig suffix */
	    opt_b = strtok(NULL, " \t\n");
	    if (! opt_b) {
		rpmlog(RPMLOG_ERR,
			_("line %d: Need arg to %%patch -b: %s\n"),
			spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	} else if (!strcmp(s, "-z")) {
	    /* orig suffix */
	    opt_b = strtok(NULL, " \t\n");
	    if (! opt_b) {
		rpmlog(RPMLOG_ERR,
			_("line %d: Need arg to %%patch -z: %s\n"),
			spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	} else if (!strcmp(s, "-F")) {
	    /* fuzz factor */
	    const char * fnum = (!strchr(" \t\n", s[2])
				? s+2 : strtok(NULL, " \t\n"));
	    char * end = NULL;

	    opt_F = (fnum ? strtol(fnum, &end, 10) : 0);
	    if (! opt_F || *end) {
		rpmlog(RPMLOG_ERR,
			_("line %d: Bad arg to %%patch -F: %s\n"),
			spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	} else if (!strcmp(s, "-d")) {
	    /* subdirectory */
	    opt_d = strtok(NULL, " \t\n");
	    if (! opt_d) {
		rpmlog(RPMLOG_ERR,
			_("line %d: Need arg to %%patch -d: %s\n"),
			spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	} else if (!strncmp(s, "-p", sizeof("-p")-1)) {
	    /* unfortunately, we must support -pX */
	    if (! strchr(" \t\n", s[2])) {
		s = s + 2;
	    } else {
		s = strtok(NULL, " \t\n");
		if (s == NULL) {
		    rpmlog(RPMLOG_ERR,
			     _("line %d: Need arg to %%patch -p: %s\n"),
			     spec->lineNum, spec->line);
		    return RPMRC_FAIL;
		}
	    }
	    if (parseNum(s, &opt_p)) {
		rpmlog(RPMLOG_ERR,
			_("line %d: Bad arg to %%patch -p: %s\n"),
			spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	} else {
	    /* Must be a patch num */
	    if (patch_index == 1024) {
		rpmlog(RPMLOG_ERR, _("Too many patches!\n"));
		return RPMRC_FAIL;
	    }
	    if (parseNum(s, &(patch_nums[patch_index]))) {
		rpmlog(RPMLOG_ERR, _("line %d: Bad arg to %%patch: %s\n"),
			 spec->lineNum, spec->line);
		return RPMRC_FAIL;
	    }
	    patch_index++;
	}
    }
    /*@=internalglobs@*/

    /* All args processed */

    if (! opt_P) {
	s = doPatch(spec, 0, opt_p, opt_b, opt_R, opt_E, opt_F, opt_d);
	if (s == NULL)
	    return RPMRC_FAIL;
	spec->prep = rpmiobAppend(spec->prep, s, 1);
    }

    for (x = 0; x < patch_index; x++) {
	s = doPatch(spec, patch_nums[x], opt_p, opt_b, opt_R, opt_E, opt_F, opt_d);
	if (s == NULL)
	    return RPMRC_FAIL;
	spec->prep = rpmiobAppend(spec->prep, s, 1);
    }

    return RPMRC_OK;
}
#endif

static void prepFetchVerbose(/*@unused@*/ struct Source *sp,
		/*@unused@*/ struct stat *st)
	/*@globals internalState @*/
	/*@modifies internalState @*/
{
    char *buf;
    size_t buf_len;
    int xx;
    int i;

    if (!(rpmIsVerbose() && !quietly && (rpmBTArgs.buildAmount & RPMBUILD_FETCHSOURCE)))
        return;
    buf_len = 2*80;
    if ((buf = (char *)malloc(buf_len)) == NULL)
        return;
    xx = snprintf(buf, buf_len, "%s%d:", (sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch", sp->num);
    for (i = (int)strlen(buf); i <= 11; i++)
        buf[i] = ' ';
    xx = snprintf(buf+i, buf_len-i, "%-52.52s", sp->source);
    i = (int)strlen(buf);
    if (st != NULL)
        xx = snprintf(buf+i, buf_len-i, " %9lu Bytes\n", (unsigned long)st->st_size);
    else
        xx = snprintf(buf+i, buf_len-i, "      ...MISSING\n");
    rpmlog(RPMLOG_NOTICE, "%s", buf);
    buf = _free(buf);
    return;
}

/**
 * Check that all sources/patches/icons exist locally, fetching if necessary.
 */
static int prepFetch(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState @*/
{
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    const char *Smacro;
#endif
    const char *Lmacro, *Lurlfn = NULL;
    const char *Rmacro, *Rurlfn = NULL;
    struct Source *sp;
    struct stat st;
    rpmRC rpmrc;
    int ec, rc;
    char *cp;

    /* XXX insure that %{_sourcedir} exists */
    rpmrc = RPMRC_OK;
    Lurlfn = rpmGenPath(NULL, "%{?_sourcedir}", NULL);
    if (Lurlfn != NULL && *Lurlfn != '\0')
	rpmrc = rpmMkdirPath(Lurlfn, "_sourcedir");
    Lurlfn = _free(Lurlfn);
    if (rpmrc != RPMRC_OK)
	return -1;

    /* XXX insure that %{_patchdir} exists */
    rpmrc = RPMRC_OK;
    Lurlfn = rpmGenPath(NULL, "%{?_patchdir}", NULL);
    if (Lurlfn != NULL && *Lurlfn != '\0')
	rpmrc = rpmMkdirPath(Lurlfn, "_patchdir");
    Lurlfn = _free(Lurlfn);
    if (rpmrc != RPMRC_OK)
	return -1;

    /* XXX insure that %{_icondir} exists */
    rpmrc = RPMRC_OK;
    Lurlfn = rpmGenPath(NULL, "%{?_icondir}", NULL);
    if (Lurlfn != NULL && *Lurlfn != '\0')
	rpmrc = rpmMkdirPath(Lurlfn, "_icondir");
    Lurlfn = _free(Lurlfn);
    if (rpmrc != RPMRC_OK)
	return -1;

    if (rpmIsVerbose() && !quietly && (rpmBTArgs.buildAmount & RPMBUILD_FETCHSOURCE))
        rpmlog(RPMLOG_NOTICE, "Checking source and patch file(s):\n");

    ec = 0;
    for (sp = spec->sources; sp != NULL; sp = sp->next) {

#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
	Smacro = "%{?_specdir}/";
#endif
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    if (! (Lmacro = getSourceDir(sp->flags, sp->source)))
#else
    if (! (Lmacro = getSourceDir(sp->flags)))
#endif
        continue;
	if (sp->flags & RPMFILE_SOURCE) {
	    Rmacro = "%{?_Rsourcedir}/";
	} else
	if (sp->flags & RPMFILE_PATCH) {
	    Rmacro = "%{?_Rpatchdir}/";
	} else
	if (sp->flags & RPMFILE_ICON) {
	    Rmacro = "%{?_Ricondir}/";
	} else
	    continue;

#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
	/* support splitted source directories, i.e., source files which
	   are alternatively placed into the .spec directory and picked
	   up from there, too. */
	Lurlfn = rpmGenPath(NULL, Smacro, sp->source);
	rc = Lstat(Lurlfn, &st);
	if (rc == 0) {
            prepFetchVerbose(sp, &st);
	    goto bottom;
        }
#endif
	Lurlfn = rpmGenPath(NULL, Lmacro, sp->source);
	rc = Lstat(Lurlfn, &st);
	if (rc == 0) {
/*@-noeffect@*/
            prepFetchVerbose(sp, &st);
/*@=noeffect@*/
	    goto bottom;
        }
/*@-noeffect@*/
        prepFetchVerbose(sp, NULL);
/*@=noeffect@*/
	if (errno != ENOENT) {
	    ec++;
	    rpmlog(RPMLOG_ERR, _("Missing %s%d %s: %s\n"),
		((sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch"),
		sp->num, sp->source, strerror(ENOENT));
	    goto bottom;
	}

        /* try to fetch via macro-controlled remote locations */
        cp = rpmExpand(Rmacro, NULL);
        if (cp != NULL && strcmp(cp, "/") != 0) {
            cp = _free(cp);
            Rurlfn = rpmGenPath(NULL, Rmacro, sp->source);
            if (!(Rurlfn == NULL || Rurlfn[0] == '\0' || !strcmp(Rurlfn, "/") || !strcmp(Lurlfn, Rurlfn))) {
                rpmlog(RPMLOG_NOTICE, _("Fetching(%s%d): %s\n"),
                       (sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch", sp->num, Rurlfn);
                rc = urlGetFile(Rurlfn, Lurlfn);
                if (rc == 0)
                    goto bottom;
                else {
                    rpmlog(RPMLOG_ERR, _("Fetching %s%d failed: %s\n"),
                           (sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch", sp->num, ftpStrerror(rc));
                    ec++;
                }
            }
        }
        cp = _free(cp);

        /* try to fetch from original location */
        rpmlog(RPMLOG_NOTICE, _("Fetching(%s%d): %s\n"),
               (sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch", sp->num, sp->fullSource);
        rc = urlGetFile(sp->fullSource, Lurlfn);
        if (rc == 0)
            goto bottom;
        else {
            rpmlog(RPMLOG_ERR, _("Fetching %s%d failed: %s\n"),
                   (sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch", sp->num, ftpStrerror(rc));
            ec++;
        }

        rpmlog(RPMLOG_ERR, _("Missing %s%d: %s: %s\n"),
            ((sp->flags & RPMFILE_SOURCE) ? "Source" : "Patch"),
            sp->num, sp->source, strerror(ENOENT));
        ec++;

bottom:
	Lurlfn = _free(Lurlfn);
	Rurlfn = _free(Rurlfn);
    }

    return ec;
}

int parsePrep(Spec spec, int verify)
{
    rpmParseState nextPart;
    int res, rc;
    rpmiob iob;
    ARGV_t saveLines = NULL;
    ARGV_t lines;
    const char * cp;
    int xx;

    if (spec->prep != NULL) {
	rpmlog(RPMLOG_ERR, _("line %d: second %%prep\n"), spec->lineNum);
	return RPMRC_FAIL;
    }

    spec->prep = rpmiobNew(0);

    /* There are no options to %prep */
    if ((rc = readLine(spec, STRIP_NOTHING)) > 0)
	return PART_NONE;
    if (rc)
	return rc;

    /* Check to make sure that all sources/patches are present. */
    if (verify) {
	rc = prepFetch(spec);
	if (rc)
	    return RPMRC_FAIL;
    }

    iob = rpmiobNew(0);

    while ((nextPart = isPart(spec)) == PART_NONE) {
	/* Need to expand the macros inline.  That way we  */
	/* can give good line number information on error. */
	iob = rpmiobAppend(iob, spec->line, 0);
	if ((rc = readLine(spec, STRIP_NOTHING)) > 0) {
	    nextPart = PART_NONE;
	    break;
	}
	if (rc)
	    return rc;
    }

    xx = argvSplit(&saveLines, rpmiobStr(iob), "\n");

/*@-usereleased@*/
    for (lines = saveLines; *lines; lines++) {
	res = 0;
	for (cp = *lines; *cp == ' ' || *cp == '\t'; cp++)
	    {};
	if (!strncmp(cp, "%setup", sizeof("%setup")-1)) {
	    res = doSetupMacro(spec, cp);
#ifndef	DYING
	} else if (! strncmp(cp, "%patch", sizeof("%patch")-1)) {
	    res = doPatchMacro(spec, cp);
#endif
	} else {
	    spec->prep = rpmiobAppend(spec->prep, *lines, 1);
	}
	if (res && !spec->force) {
	    saveLines = argvFree(saveLines);
	    iob = rpmiobFree(iob);
	    return res;
	}
    }
/*@=usereleased@*/

    saveLines = argvFree(saveLines);
    iob = rpmiobFree(iob);

    return nextPart;
}
