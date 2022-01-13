/** \ingroup rpmbuild
 * \file build/build.c
 *  Top-level build dispatcher.
 */

#include "system.h"

#define	_RPMBC_INTERNAL
#include <rpmio_internal.h>	/* XXX fdGetFp */
#include <rpmcb.h>
#include <rpmsq.h>

#define	_RPMTAG_INTERNAL
#include <rpmbuild.h>
#include "signature.h"		/* XXX rpmTempFile */

#include "debug.h"

/**
 */
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
const char * getSourceDir(rpmfileAttrs attr, const char *filename)
#else
const char * getSourceDir(rpmfileAttrs attr)
#endif
{
    const char * dir = NULL;
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
    const char *fn;

    /*	support splitted source directories, i.e., source files which
	are alternatively placed into the .spec directory and picked
	up from there, too. */
    if (attr & (RPMFILE_SOURCE|RPMFILE_PATCH|RPMFILE_ICON) && filename != NULL)
    {
	fn = rpmGetPath("%{_specdir}/", filename, NULL);
	if (access(fn, F_OK) == 0)
	    dir = "%{_specdir}/";
	fn = _free(fn);
    }
    if (dir != NULL) {
    } else
#endif
    if (attr & RPMFILE_SOURCE)
	dir = "%{_sourcedir}/";
    else if (attr & RPMFILE_PATCH)
	dir = "%{_patchdir}/";
    else if (attr & RPMFILE_ICON)
	dir = "%{_icondir}/";

    return dir;
}

/*@access urlinfo @*/		/* XXX compared with NULL */
/*@access FD_t @*/

/**
 */
static void doRmSource(Spec spec)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rpmGlobalMacroContext, fileSystem, internalState  @*/
{
    struct Source *sp;
    int rc;

#if 0
    rc = Unlink(spec->specFile);
#endif

    for (sp = spec->sources; sp != NULL; sp = sp->next) {
	const char *dn, *fn;
	if (sp->flags & RPMFILE_GHOST)
	    continue;
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
	if (! (dn = getSourceDir(sp->flags, sp->source)))
#else
	if (! (dn = getSourceDir(sp->flags)))
#endif
	    continue;
	fn = rpmGenPath(NULL, dn, sp->source);
	rc = Unlink(fn);
	fn = _free(fn);
    }
}

/*
 * @todo Single use by %%doc in files.c prevents static.
 */
rpmRC doScript(Spec spec, int what, const char *name, rpmiob iob, int test)
{
    const char * rootURL = spec->rootURL;
    const char * rootDir;
    const char * scriptName = NULL;
    const char * buildDirURL = rpmGenPath(rootURL, "%{_builddir}", "");
    const char * buildScript;
    const char * buildCmd = NULL;
    const char * buildTemplate = NULL;
    const char * buildPost = NULL;
    const char * mTemplate = NULL;
    const char * mCmd = NULL;
    const char * mPost = NULL;
    int argc = 0;
    const char **argv = NULL;
    FILE * fp = NULL;
    urlinfo u = NULL;
    rpmop op = NULL;
    int ix = -1;

    FD_t fd;
    FD_t xfd;
    int status;
    rpmRC rc;
    size_t i;

    switch (what) {
    case RPMBUILD_PREP:
	name = "%prep";
	iob = spec->prep;
	op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
	ix = RPMSCRIPT_PREP;
	mTemplate = "%{__spec_prep_template}";
	mPost = "%{__spec_prep_post}";
	mCmd = "%{__spec_prep_cmd}";
	break;
    case RPMBUILD_BUILD:
	name = "%build";
	iob = spec->build;
	op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
	ix = RPMSCRIPT_BUILD;
	mTemplate = "%{__spec_build_template}";
	mPost = "%{__spec_build_post}";
	mCmd = "%{__spec_build_cmd}";
	break;
    case RPMBUILD_INSTALL:
	name = "%install";
	iob = spec->install;
	op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
	ix = RPMSCRIPT_INSTALL;
	mTemplate = "%{__spec_install_template}";
	mPost = "%{__spec_install_post}";
	mCmd = "%{__spec_install_cmd}";
	break;
    case RPMBUILD_CHECK:
	name = "%check";
	iob = spec->check;
	op = memset(alloca(sizeof(*op)), 0, sizeof(*op));
	ix = RPMSCRIPT_CHECK;
	mTemplate = "%{__spec_check_template}";
	mPost = "%{__spec_check_post}";
	mCmd = "%{__spec_check_cmd}";
	break;
    case RPMBUILD_CLEAN:
	name = "%clean";
	iob = spec->clean;
	mTemplate = "%{__spec_clean_template}";
	mPost = "%{__spec_clean_post}";
	mCmd = "%{__spec_clean_cmd}";
	break;
    case RPMBUILD_RMBUILD:
	name = "--clean";
	mTemplate = "%{__spec_clean_template}";
	mPost = "%{__spec_clean_post}";
	mCmd = "%{__spec_clean_cmd}";
	break;
    /* support "%track" script/section */
    case RPMBUILD_TRACK:
	name = "%track";
	iob = NULL;
	if (spec->foo)
	for (i = 0; i < spec->nfoo; i++) {
	    if (spec->foo[i].str == NULL || spec->foo[i].iob == NULL)
		continue;
	    if (xstrcasecmp(spec->foo[i].str, "track"))
		continue;
	    iob = spec->foo[i].iob;
	    /*@loopbreak@*/ break;
	}
	mTemplate = "%{__spec_track_template}";
	mPost = "%{__spec_track_post}";
	mCmd = "%{__spec_track_cmd}";
	break;
    case RPMBUILD_STRINGBUF:
    default:
	mTemplate = "%{___build_template}";
	mPost = "%{___build_post}";
	mCmd = "%{___build_cmd}";
	break;
    }

assert(name != NULL);

    if ((what != RPMBUILD_RMBUILD) && iob == NULL) {
	rc = RPMRC_OK;
	goto exit;
    }

    if (rpmTempFile(rootURL, &scriptName, &fd) || fd == NULL || Ferror(fd)) {
	rpmlog(RPMLOG_ERR, _("Unable to open temp file.\n"));
	rc = RPMRC_FAIL;
	goto exit;
    }

    if (fdGetFp(fd) == NULL)
	xfd = Fdopen(fd, "w.fpio");
    else
	xfd = fd;

    /*@-type@*/ /* FIX: cast? */
    if ((fp = fdGetFp(xfd)) == NULL) {
	rc = RPMRC_FAIL;
	goto exit;
    }
    /*@=type@*/

    (void) urlPath(rootURL, &rootDir);
    if (*rootDir == '\0') rootDir = "/";

    (void) urlPath(scriptName, &buildScript);

    buildTemplate = rpmExpand(mTemplate, NULL);
    buildPost = rpmExpand(mPost, NULL);

    (void) fputs(buildTemplate, fp);

    /* support "%track" script/section */
    if (what != RPMBUILD_PREP && what != RPMBUILD_RMBUILD && spec->buildSubdir && what != RPMBUILD_TRACK)
	fprintf(fp, "cd '%s'\n", spec->buildSubdir);

    if (what == RPMBUILD_RMBUILD) {
	if (spec->buildSubdir)
	    fprintf(fp, "rm -rf '%s'\n", spec->buildSubdir);
    } else if (iob != NULL)
	fprintf(fp, "%s", rpmiobStr(iob));

    (void) fputs(buildPost, fp);

    (void) Fclose(xfd);

    if (test) {
	rc = RPMRC_OK;
	goto exit;
    }

    if (buildDirURL && buildDirURL[0] != '/' &&
	(urlSplit(buildDirURL, &u) != 0)) {
	rc = RPMRC_FAIL;
	goto exit;
    }

    switch (urlType(u)) {
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_FTP:
	addMacro(spec->macros, "_remsh", NULL, "%{__remsh}", RMIL_SPEC);
	addMacro(spec->macros, "_remhost", NULL, u->host, RMIL_SPEC);
	if (strcmp(rootDir, "/"))
	    addMacro(spec->macros, "_remroot", NULL, rootDir, RMIL_SPEC);
	break;
    case URL_IS_UNKNOWN:
    case URL_IS_DASH:
    case URL_IS_PATH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	break;
    }

    buildCmd = rpmExpand(mCmd, " ", buildScript, NULL);
    (void) poptParseArgvString(buildCmd, &argc, &argv);

    if (what != RPMBUILD_TRACK)		/* support "%track" script/section */
	rpmlog(RPMLOG_NOTICE, _("Executing(%s): %s\n"), name, buildCmd);

    /* Run the script with a stopwatch. */
    if (op != NULL)
	(void) rpmswEnter(op, 0);

    status = rpmsqExecve(argv);
    if (ix >= 0 && ix < RPMSCRIPT_MAX)
	spec->sstates[ix] =
	    (RPMSCRIPT_STATE_EXEC | RPMSCRIPT_STATE_REAPED) | (status & 0xffff);

    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
	rpmlog(RPMLOG_ERR, _("Bad exit status from %s (%s)\n"),
		 scriptName, name);
	rc = RPMRC_FAIL;
    } else
	rc = RPMRC_OK;

    if (op != NULL) {
	static unsigned int scale = 1000;
	(void) rpmswExit(op, 0);
	if (ix >= 0 && ix < RPMSCRIPT_MAX)
	    spec->smetrics[ix] += op->usecs / scale;
    }

exit:
    if (scriptName) {
#if defined(RPM_VENDOR_OPENPKG) /* always-remove-tempfiles */
	/* Unconditionally remove temporary files ("rpm-tmp.XXXXX") which
	   were generated for the executed scripts. In OpenPKG we run the
	   scripts in debug mode ("set -x") anyway, so we never need to
	   see the whole generated script -- not even if it breaks.  Instead
	   we would just have temporary files staying around forever. */
#else
	if (rc == RPMRC_OK)
#endif
	    (void) Unlink(scriptName);
	scriptName = _free(scriptName);
    }

    switch (urlType(u)) {
    case URL_IS_HTTPS:
    case URL_IS_HTTP:
    case URL_IS_FTP:
	delMacro(spec->macros, "_remsh");
	delMacro(spec->macros, "_remhost");
	if (strcmp(rootDir, "/"))
	    delMacro(spec->macros, "_remroot");
	break;
    case URL_IS_UNKNOWN:
    case URL_IS_DASH:
    case URL_IS_PATH:
    case URL_IS_HKP:
    case URL_IS_MONGO:	/* XXX FIXME */
    default:
	break;
    }

    argv = _free(argv);
    buildCmd = _free(buildCmd);
    buildTemplate = _free(buildTemplate);
    buildPost = _free(buildPost);
    buildDirURL = _free(buildDirURL);
    return rc;
}

rpmRC buildSpec(rpmts ts, Spec spec, int what, int test)
{
    rpmRC rc = RPMRC_OK;

    /* Generate a keypair lazily. */
assert(spec->dig == NULL);
    spec->dig = pgpDigNew(RPMVSF_DEFAULT, PGPPUBKEYALGO_UNKNOWN);
    {	int xx;
	xx = pgpDigSetAlgos(spec->dig);
	xx = pgpImplGenerate(spec->dig);
assert(xx == 1);
	xx = pgpExportPubkey(spec->dig);
    }

    if (!spec->recursing && spec->BACount) {
	int x;
	/* When iterating over BANames, do the source    */
	/* packaging on the first run, and skip RMSOURCE altogether */
	if (spec->BASpecs != NULL)
	for (x = 0; x < spec->BACount; x++) {
	    if ((rc = buildSpec(ts, spec->BASpecs[x],
				(what & ~RPMBUILD_RMSOURCE) |
				(x ? 0 : (what & RPMBUILD_PACKAGESOURCE)),
				test))) {
		goto exit;
	    }
	}
    } else {
	/* support "%track" script/section */
	if ((what & RPMBUILD_TRACK) &&
	    (rc = doScript(spec, RPMBUILD_TRACK, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_PREP) &&
	    (rc = doScript(spec, RPMBUILD_PREP, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_BUILD) &&
	    (rc = doScript(spec, RPMBUILD_BUILD, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_INSTALL) &&
	    (rc = doScript(spec, RPMBUILD_INSTALL, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_CHECK) &&
	    (rc = doScript(spec, RPMBUILD_CHECK, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_PACKAGESOURCE) &&
	    (rc = processSourceFiles(spec)))
		goto exit;

	if (((what & RPMBUILD_INSTALL) || (what & RPMBUILD_PACKAGEBINARY) ||
	    (what & RPMBUILD_FILECHECK)) &&
	    (rc = processBinaryFiles(spec, what & RPMBUILD_INSTALL, test)))
		goto exit;

	if (((what & RPMBUILD_PACKAGESOURCE) && !test) &&
	    (rc = packageSources(spec)))
		return rc;

	if (((what & RPMBUILD_PACKAGEBINARY) && !test) &&
	    (rc = packageBinaries(spec)))
		goto exit;
	
	if ((what & RPMBUILD_CLEAN) &&
	    (rc = doScript(spec, RPMBUILD_CLEAN, NULL, NULL, test)))
		goto exit;

	if ((what & RPMBUILD_RMBUILD) &&
	    (rc = doScript(spec, RPMBUILD_RMBUILD, NULL, NULL, test)))
		goto exit;
    }

    if (what & RPMBUILD_RMSOURCE)
	doRmSource(spec);

    if (what & RPMBUILD_RMSPEC)
	(void) Unlink(spec->specFile);

#if defined(RPM_VENDOR_OPENPKG) /* auto-remove-source-directories */
    /* In OpenPKG we use per-package %{_sourcedir} and %{_specdir}
       definitions (macros have trailing ".../%{name}"). On removal of
       source(s) and .spec file, this per-package directory would be kept
       (usually <prefix>/RPM/SRC/<name>/), because RPM does not know about
       this OpenPKG convention. So, let RPM try(!) to remove the two
       directories (if they are empty) and just ignore removal failures
       (if they are still not empty). */
    if (what & RPMBUILD_RMSOURCE) {
	const char *pn;
	pn = rpmGetPath("%{_sourcedir}", NULL);
	Rmdir(pn); /* ignore error, it is ok if it fails (usually with ENOTEMPTY) */
	pn = _free(pn);
    }
    if (what & RPMBUILD_RMSPEC) {
	const char *pn;
	pn = rpmGetPath("%{_specdir}", NULL);
	Rmdir(pn); /* ignore error, it is ok if it fails (usually with ENOTEMPTY) */
	pn = _free(pn);
    }
#endif

exit:
    if (rc != RPMRC_OK && rpmlogGetNrecs() > 0) {
	rpmlog(RPMLOG_NOTICE, _("\n\nRPM build errors:\n"));
	rpmlogPrint(NULL);
    }

    return rc;
}
