/** \ingroup rpmbuild
 * \file build/parseReqs.c
 *  Parse dependency tag from spec file or from auto-dependency generator.
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#include <rpmlog.h>
#define	_RPMEVR_INTERNAL
#include "rpmbuild.h"
#include "debug.h"

/*@access EVR_t @*/

#define	SKIPWHITE(_x)	{while(*(_x) && (xisspace(*_x) || *(_x) == ',')) (_x)++;}
#define	SKIPNONWHITE(_x){while(*(_x) &&!(xisspace(*_x) || *(_x) == ',')) (_x)++;}

rpmRC parseRCPOT(Spec spec, Package pkg, const char *field, rpmTag tagN,
	       rpmuint32_t index, rpmsenseFlags tagflags)
{
    EVR_t evr = alloca(sizeof(*evr));
    const char *r, *re, *v, *ve;
    char * N = NULL;
    char * EVR = NULL;
    rpmsenseFlags Flags;
    Header h;
    rpmRC rc = RPMRC_FAIL;	/* assume failure */
    int ix;

    switch (tagN) {
    case RPMTAG_PROVIDEFLAGS:
	tagflags |= RPMSENSE_PROVIDES;
	h = pkg->header;
	break;
    case RPMTAG_OBSOLETEFLAGS:
	tagflags |= RPMSENSE_OBSOLETES;
	h = pkg->header;
	break;
    case RPMTAG_CONFLICTFLAGS:
	tagflags |= RPMSENSE_CONFLICTS;
	h = pkg->header;
	break;
    case RPMTAG_BUILDCONFLICTS:
	tagflags |= RPMSENSE_CONFLICTS;
	h = spec->sourceHeader;
	break;
    case RPMTAG_PREREQ:
	tagflags |= RPMSENSE_ANY;
	h = pkg->header;
	break;
    case RPMTAG_TRIGGERPREIN:
	tagflags |= RPMSENSE_TRIGGERPREIN;
	h = pkg->header;
	break;
    case RPMTAG_TRIGGERIN:
	tagflags |= RPMSENSE_TRIGGERIN;
	h = pkg->header;
	break;
    case RPMTAG_TRIGGERPOSTUN:
	tagflags |= RPMSENSE_TRIGGERPOSTUN;
	h = pkg->header;
	break;
    case RPMTAG_TRIGGERUN:
	tagflags |= RPMSENSE_TRIGGERUN;
	h = pkg->header;
	break;
    case RPMTAG_BUILDSUGGESTS:
    case RPMTAG_BUILDENHANCES:
	tagflags |= RPMSENSE_MISSINGOK;
	h = spec->sourceHeader;
	break;
    case RPMTAG_BUILDPREREQ:
    case RPMTAG_BUILDREQUIRES:
	tagflags |= RPMSENSE_ANY;
	h = spec->sourceHeader;
	break;
    case RPMTAG_BUILDPROVIDES:
	tagflags |= RPMSENSE_PROVIDES;
	h = spec->sourceHeader;
	break;
    case RPMTAG_BUILDOBSOLETES:
	tagflags |= RPMSENSE_OBSOLETES;
	h = spec->sourceHeader;
	break;
    default:
    case RPMTAG_REQUIREFLAGS:
	tagflags |= RPMSENSE_ANY;
	h = pkg->header;
	break;
    }

    for (r = field; *r != '\0'; r = re) {
	size_t nr;
	SKIPWHITE(r);
	if (*r == '\0')
	    break;

	Flags = (tagflags & ~RPMSENSE_SENSEMASK);

	re = r;
	SKIPNONWHITE(re);
	N = xmalloc((re-r) + 1);
	strncpy(N, r, (re-r));
	N[re-r] = '\0';

	/* N must begin with alphanumeric, _, or /, or a macro. */
	nr = strlen(N);
	ix = 0;
	if (N[ix] == '!')
	    ix++;
	if (!(xisalnum(N[ix]) || N[ix] == '_' || N[ix] == '/'
	 || (nr > 5 && N[ix] == '%' && N[ix+1] == '{' && N[nr-1] == '}')))
	{
	    rpmlog(RPMLOG_ERR,
		     _("line %d: Dependency \"%s\" must begin with alpha-numeric, '_' or '/': %s\n"),
		     spec->lineNum, N, spec->line);
	    goto exit;
	}

	/* Parse EVR */
	v = re;
	SKIPWHITE(v);
	ve = v;
	SKIPNONWHITE(ve);

	re = v;	/* ==> next token (if no EVR found) starts here */

	/* Check for possible logical operator */
	if (ve > v) {
/*@-mods@*/
	    rpmsenseFlags F = rpmEVRflags(v, &ve);
/*@=mods@*/
	    if (F && r[0] == '/') {
		rpmlog(RPMLOG_ERR,
			 _("line %d: Versioned file name not permitted: %s\n"),
			 spec->lineNum, spec->line);
		goto exit;
	    }
	    if (F) {
		/* now parse EVR */
		v = ve;
		SKIPWHITE(v);
		ve = v;
		SKIPNONWHITE(ve);
	    }
	    Flags &= ~RPMSENSE_SENSEMASK;
	    Flags |= F;
	}

 	if (Flags & RPMSENSE_SENSEMASK) {
	    char * t;

	    EVR = t = xmalloc((ve-v) + 1);
	    nr = 0;
	    while (v < ve && *v != '\0')
	    switch ((int)*v) {
	    case '-':   nr++;	/*@fallthrough@*/
	    default:    *t++ = *v++;    break;
	    }
	    *t = '\0';

	    if (*EVR == '\0') {
		rpmlog(RPMLOG_ERR, _("line %d: %s must be specified: %s\n"),
			spec->lineNum, "EVR", spec->line);
		goto exit;
	    }
	    if (nr > 1) {
		rpmlog(RPMLOG_ERR, _("line %d: Illegal char '-' in %s: %s\n"),
			spec->lineNum, "EVR", spec->line);
		goto exit;
            }
	    /* EVR must be parseable (or a macro). */
	    ix = 0;
	    nr = strlen(EVR);
	    if (!(nr > 3 && EVR[0] == '%' && EVR[1] == '{' && EVR[nr-1] == '}'))
	    {
		memset(evr, 0, sizeof(*evr));
		ix = rpmEVRparse(xstrdup(EVR), evr);
		evr->str = _free(evr->str);
	    }
	    if (ix != 0) {
		rpmlog(RPMLOG_ERR, _("line %d: %s does not parse: %s\n"),
			 spec->lineNum, "EVR", spec->line);
		goto exit;
	    }
	    re = ve;	/* ==> next token after EVR string starts here */
	} else
	    EVR = NULL;

	(void) addReqProv(spec, h, tagN, N, EVR, Flags, index);

	N = _free(N);
	EVR = _free(EVR);

    }
    rc = RPMRC_OK;

exit:
    N = _free(N);
    EVR = _free(EVR);
    return rc;
}
