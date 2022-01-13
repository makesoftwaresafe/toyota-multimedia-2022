/** \ingroup rpmbuild
 * \file build/reqprov.c
 *  Add dependency tags to package header(s).
 */

#include "system.h"

#include <rpmio.h>
#include <rpmiotypes.h>
#define	_RPMEVR_INTERNAL
#include "rpmbuild.h"
#include "debug.h"

int addReqProv(/*@unused@*/ Spec spec, Header h,
		/*@unused@*/ rpmTag tagN,
		const char * N, const char * EVR, rpmsenseFlags Flags,
		rpmuint32_t index)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    const char ** names;
    rpmTag nametag = 0;
    rpmTag versiontag = 0;
    rpmTag flagtag = 0;
    rpmTag indextag = 0;
    int len;
    rpmsenseFlags extra = RPMSENSE_ANY;
    int xx;

    if (Flags & RPMSENSE_PROVIDES) {
	nametag = RPMTAG_PROVIDENAME;
	versiontag = RPMTAG_PROVIDEVERSION;
	flagtag = RPMTAG_PROVIDEFLAGS;
	extra = Flags & RPMSENSE_FIND_PROVIDES;
    } else if (Flags & RPMSENSE_OBSOLETES) {
	nametag = RPMTAG_OBSOLETENAME;
	versiontag = RPMTAG_OBSOLETEVERSION;
	flagtag = RPMTAG_OBSOLETEFLAGS;
    } else if (Flags & RPMSENSE_CONFLICTS) {
	nametag = RPMTAG_CONFLICTNAME;
	versiontag = RPMTAG_CONFLICTVERSION;
	flagtag = RPMTAG_CONFLICTFLAGS;
    } else if (Flags & RPMSENSE_TRIGGER) {
	nametag = RPMTAG_TRIGGERNAME;
	versiontag = RPMTAG_TRIGGERVERSION;
	flagtag = RPMTAG_TRIGGERFLAGS;
	indextag = RPMTAG_TRIGGERINDEX;
	extra = Flags & RPMSENSE_TRIGGER;
    } else {
	nametag = RPMTAG_REQUIRENAME;
	versiontag = RPMTAG_REQUIREVERSION;
	flagtag = RPMTAG_REQUIREFLAGS;
	extra = Flags & _ALL_REQUIRES_MASK;
    }

    Flags = (Flags & RPMSENSE_SENSEMASK) | extra;

    if (EVR == NULL)
	EVR = "";
#if defined(RPM_VENDOR_MANDRIVA)
    /* Check that provide isn't duplicate of package */
    else if (nametag == RPMTAG_PROVIDENAME) {
	const char *NEVR;
	size_t len;
	int duplicate;

	len = strlen(N);
	NEVR = headerSprintf(h, "%{NAME}-%|EPOCH?{%{EPOCH}:}|%{VERSION}-%{RELEASE}", NULL, NULL, NULL);
	duplicate = !strncmp(NEVR, N, len) && !strcmp(NEVR+len+1, EVR);

	_free(NEVR);

	if (duplicate)
	    return 0;
    }
#endif

    /* Check for duplicate dependencies. */
    he->tag = nametag;
    xx = headerGet(h, he, 0);
    names = he->p.argv;
    len = he->c;
    if (xx) {
	const char ** versions = NULL;
	rpmuint32_t * flags = NULL;
	rpmuint32_t * indexes = NULL;
	int duplicate = 0;

	if (flagtag) {
	    he->tag = versiontag;
	    xx = headerGet(h, he, 0);
	    versions = he->p.argv;
	    he->tag = flagtag;
	    xx = headerGet(h, he, 0);
	    flags = he->p.ui32p;
	}
	if (indextag) {
	    he->tag = indextag;
	    xx = headerGet(h, he, 0);
	    indexes = he->p.ui32p;
	}

	while (len > 0) {
	    len--;
	    if (strcmp(names[len], N))
		continue;

#if defined(RPM_VENDOR_MANDRIVA) /* filter-overlapping-dependencies */
	    /* XXX: Potential drawbacks? Need to study & discuess this one a
	     * bit further, leaving under #ifdef for now...
	     * TODO: auto-generated deps too
	     */
	    if (flagtag && versions != NULL) {
		int overlap;

		if(*EVR && !*versions[len]) {
		    overlap = 1;
		    flags[len] = Flags;
		    he->tag = flagtag;
		    he->t = RPM_UINT32_TYPE;
		    he->p.argv = (void *) &Flags;
		    xx = headerMod(h, he, 0);
		} else {
		    EVR_t lEVR = rpmEVRnew(RPMSENSE_ANY, 0),
			  rEVR = rpmEVRnew(RPMSENSE_ANY, 0);

		    rpmEVRparse(EVR, lEVR);
		    rpmEVRparse(versions[len], rEVR);
		    lEVR->Flags = Flags | RPMSENSE_EQUAL;
		    rEVR->Flags = flags[len] | RPMSENSE_EQUAL;
		    overlap = rpmEVRoverlap(lEVR, rEVR);
		    if (!overlap)
			if (rpmEVRoverlap(rEVR, lEVR))
    			    duplicate = 1;
		    lEVR = rpmEVRfree(lEVR);
		    rEVR = rpmEVRfree(rEVR);
		}
		if (overlap) {
		    versions[len] = EVR;
		    he->tag = versiontag;
		    he->t = RPM_STRING_ARRAY_TYPE;
		    he->p.argv = versions;
		    xx = headerMod(h, he, 0);
		} else
		    continue;
	    }
#else
	    if (flagtag && versions != NULL &&
		(strcmp(versions[len], EVR) || (rpmsenseFlags)flags[len] != Flags))
		continue;
#endif

	    if (indextag && indexes != NULL && indexes[len] != index)
		continue;

	    /* This is a duplicate dependency. */
	    duplicate = 1;

	    break;
	}
/*@-usereleased@*/
	names = _free(names);
	versions = _free(versions);
	flags = _free(flags);
	indexes = _free(indexes);
/*@=usereleased@*/
	if (duplicate)
	    return 0;
    }

    /* Add this dependency. */
    he->tag = nametag;
    he->t = RPM_STRING_ARRAY_TYPE;
    he->p.argv = &N;
    he->c = 1;
    he->append = 1;
    xx = headerPut(h, he, 0);
    he->append = 0;

    if (flagtag) {
	he->tag = versiontag;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.argv = &EVR;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;

	he->tag = flagtag;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = (void *) &Flags;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
    }
    if (indextag) {
	he->tag = indextag;
	he->t = RPM_UINT32_TYPE;
	he->p.ui32p = &index;
	he->c = 1;
	he->append = 1;
	xx = headerPut(h, he, 0);
	he->append = 0;
    }

    return 0;
}

int rpmlibNeedsFeature(Header h, const char * feature, const char * featureEVR)
{
    char * reqname = alloca(sizeof("rpmlib()") + strlen(feature));

    (void) stpcpy( stpcpy( stpcpy(reqname, "rpmlib("), feature), ")");

    /* XXX 1st arg is unused */
   return addReqProv(NULL, h, RPMTAG_REQUIRENAME, reqname, featureEVR,
	RPMSENSE_RPMLIB|(RPMSENSE_LESS|RPMSENSE_EQUAL), 0);
}
