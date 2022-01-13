/** \ingroup rpmbuild
 * \file build/parseBuildInstallClean.c
 *  Parse %build/%install/%clean section from spec file.
 */
#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#define	_RPMTAG_INTERNAL
#include "rpmbuild.h"
#include "debug.h"

int parseBuildInstallClean(Spec spec, rpmParseState parsePart)
{
    rpmParseState nextPart;
    rpmiob *iobp = NULL;
    const char *name = NULL;
    rpmRC rc;

    switch (parsePart) {
    case PART_BUILD:
	iobp = &spec->build;
	name = "build";
	break;
    case PART_INSTALL:
	iobp = &spec->install;
	name = "install";
	break;
    case PART_CHECK:
	iobp = &spec->check;
	name = "check";
	break;
    case PART_CLEAN:
	iobp = &spec->clean;
	name = "clean";
	break;
    case PART_ARBITRARY:
assert(spec->nfoo > 0);
	iobp = &spec->foo[spec->nfoo-1].iob;
	name = spec->foo[spec->nfoo-1].str;
	break;
    default:
	rpmlog(RPMLOG_ERR, _("line %d: unknown specfile section\n"),
		spec->lineNum);
	return RPMRC_FAIL;
	break;
    }
    
    if (*iobp != NULL) {
	rpmlog(RPMLOG_ERR, _("line %d: second %%%s section\n"),
		spec->lineNum, name);
	return RPMRC_FAIL;
    }
    
    *iobp = rpmiobNew(0);

    /* Make sure the buildroot is removed where needed. */
    if (parsePart == PART_INSTALL) {
	const char * s = rpmExpand("%{!?__spec_install_pre:%{?buildroot:%{__rm} -rf '%{buildroot}'\n%{__mkdir_p} '%{buildroot}'\n}}\n", NULL);
	if (s && *s)
	    *iobp = rpmiobAppend(*iobp, s, 0);
	s = _free(s);
    } else if (parsePart == PART_CLEAN) {
	const char * s = rpmExpand("%{?__spec_clean_body}%{!?__spec_clean_body:%{?buildroot:rm -rf '%{buildroot}'\n}}\n", NULL);
	if (s && *s)
	    *iobp = rpmiobAppend(*iobp, s, 0);
	s = _free(s);
#if !defined(RPM_VENDOR_OPENPKG) /* still-support-section-clean */
	/* OpenPKG still wishes to use "%clean" script/section */
	iobp = NULL;	/* XXX skip %clean from spec file. */
#endif
    }

    /* There are no options to %build, %install, %check, or %clean */
    if ((rc = readLine(spec, STRIP_NOTHING)) > 0)
	return PART_NONE;
    if (rc != RPMRC_OK)
	return rc;
    
    while ((nextPart = isPart(spec)) == PART_NONE) {
	if (iobp)
	    *iobp = rpmiobAppend(*iobp, spec->line, 0);
	if ((rc = readLine(spec, STRIP_NOTHING)) > 0)
	    return PART_NONE;
	if (rc)
	    return rc;
    }

    return nextPart;
}
