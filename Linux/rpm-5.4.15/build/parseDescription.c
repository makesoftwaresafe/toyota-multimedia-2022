/** \ingroup rpmbuild
 * \file build/parseDescription.c
 *  Parse %description section from spec file.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#include "rpmbuild.h"
#include "debug.h"

/*@-exportheadervar@*/
/*@unchecked@*/
extern int noLang;
/*@=exportheadervar@*/

/* These have to be global scope to make up for *stupid* compilers */
/*@unchecked@*/
    /*@observer@*/ /*@null@*/ static const char *name = NULL;
/*@unchecked@*/
    /*@observer@*/ /*@null@*/ static const char *lang = NULL;

/*@unchecked@*/
    static struct poptOption optionsTable[] = {
	{ NULL, 'n', POPT_ARG_STRING, &name, 0,	NULL, NULL},
	{ NULL, 'l', POPT_ARG_STRING, &lang, 0,	NULL, NULL},
	{ 0, 0, 0, 0, 0,	NULL, NULL}
    };

int parseDescription(Spec spec)
	/*@globals name, lang @*/
	/*@modifies name, lang @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmParseState nextPart = (rpmParseState) RPMRC_FAIL; /* assume error */
    rpmiob iob = NULL;
    int flag = PART_SUBNAME;
    Package pkg;
    int rc, argc;
    int arg;
    const char **argv = NULL;
    poptContext optCon = NULL;
    spectag t = NULL;
    int xx;

    {	char * se = strchr(spec->line, '#');
	if (se) {
	    *se = '\0';
	    while (--se >= spec->line && strchr(" \t\n\r", *se) != NULL)
		*se = '\0';
	}
    }

    if ((rc = poptParseArgvString(spec->line, &argc, &argv))) {
	rpmlog(RPMLOG_ERR, _("line %d: Error parsing %%description: %s\n"),
		 spec->lineNum, poptStrerror(rc));
	goto exit;
    }

    name = NULL;
    lang = RPMBUILD_DEFAULT_LANG;
    optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
    while ((arg = poptGetNextOpt(optCon)) > 0)
	{;}
    if (name != NULL)
	flag = PART_NAME;

    if (arg < -1) {
	rpmlog(RPMLOG_ERR, _("line %d: Bad option %s: %s\n"),
		 spec->lineNum,
		 poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		 spec->line);
	goto exit;
    }

    if (poptPeekArg(optCon)) {
	if (name == NULL)
	    name = poptGetArg(optCon);
	if (poptPeekArg(optCon)) {
	    rpmlog(RPMLOG_ERR, _("line %d: Too many names: %s\n"),
		     spec->lineNum, spec->line);
	    goto exit;
	}
    }

    if (lookupPackage(spec, name, flag, &pkg) != RPMRC_OK) {
	rpmlog(RPMLOG_ERR, _("line %d: Package does not exist: %s\n"),
		 spec->lineNum, spec->line);
	goto exit;
    }

    /* Lose the inheirited %description (if present). */
    if (spec->packages->header != pkg->header) {
	he->tag = RPMTAG_DESCRIPTION;
	xx = headerGet(pkg->header, he, 0);
	he->p.ptr = _free(he->p.ptr);
	if (xx && he->t == RPM_STRING_TYPE)
	    xx = headerDel(pkg->header, he, 0);
    }
    
    t = stashSt(spec, pkg->header, RPMTAG_DESCRIPTION, lang);
    
    iob = rpmiobNew(0);

    if ((rc = readLine(spec, STRIP_TRAILINGSPACE | STRIP_COMMENTS)) > 0) {
	nextPart = PART_NONE;
	goto exit;
    }
    if (rc < 0) {
	nextPart = (rpmParseState) RPMRC_FAIL;
	goto exit;
    }

    while ((nextPart = isPart(spec)) == PART_NONE) {
	iob = rpmiobAppend(iob, spec->line, 1);
	if (t) t->t_nlines++;
	if ((rc = readLine(spec, STRIP_TRAILINGSPACE | STRIP_COMMENTS)) > 0) {
	    nextPart = PART_NONE;
	    break;
	}
	if (rc) {
	    nextPart = (rpmParseState) RPMRC_FAIL;
	    goto exit;
	}
    }
    
    iob = rpmiobRTrim(iob);
    if (!(noLang && strcmp(lang, RPMBUILD_DEFAULT_LANG))) {
#if defined(SUPPORT_I18NSTRING_TYPE)
	const char * s = rpmiobStr(iob);
	(void) headerAddI18NString(pkg->header, RPMTAG_DESCRIPTION, s, lang);
#else
	if (!strcmp(lang, RPMBUILD_DEFAULT_LANG)) {
	    he->tag = RPMTAG_DESCRIPTION;
	    he->t = RPM_STRING_TYPE;
	    he->p.str = rpmiobStr(iob);
	    he->c = 1;
	    xx = headerPut(pkg->header, he, 0);
	}
#endif
    }
    
exit:
    iob = rpmiobFree(iob);
    argv = _free(argv);
    optCon = poptFreeContext(optCon);
    return nextPart;
}
