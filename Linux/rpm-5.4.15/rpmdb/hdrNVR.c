/** \ingroup rpmdb
 * \file rpmdb/hdrNVR.c
 */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmmacro.h>

#define	_RPMTAG_INTERNAL
#include "header_internal.h"		/* XXX hdrchkType(), hdrchkData() */

#include "debug.h"

/**
 * Macros to be defined from per-header tag values.
 * @todo Should other macros be added from header when installing a package?
 */
/*@observer@*/ /*@unchecked@*/
static struct tagMacro {
/*@observer@*/ /*@null@*/
    const char *macroname;	/*!< Macro name to define. */
    rpmTag	tag;		/*!< Header tag to use for value. */
} tagMacros[] = {
    { "name",		RPMTAG_NAME },
    { "version",	RPMTAG_VERSION },
    { "release",	RPMTAG_RELEASE },
    { "distepoch",	RPMTAG_DISTEPOCH },
    { "epoch",		RPMTAG_EPOCH },
    { "arch",		RPMTAG_ARCH },
    { "os",		RPMTAG_OS },
    { NULL, (rpmTag) 0 }
};

/*@-globs -mods -incondefs@*/
int headerMacrosLoad(Header h)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct tagMacro * tagm;
    char numbuf[64];
    const char * val;
    rpmuint64_t ival;
    int xx;

    numbuf[0] = '\0';
    /* XXX pre-expand %{buildroot} (if any) */
    {   const char *s = rpmExpand("%{?buildroot}", NULL);
	if (s && *s) 
	    (void) addMacro(NULL, "..buildroot", NULL, s, -1);
	s = _free(s);
    }
    {   const char *s = rpmExpand("%{?_builddir}", NULL);
	if (s && *s) 
	    (void) addMacro(NULL, ".._builddir", NULL, s, -1);
	s = _free(s);
    }

    for (tagm = tagMacros; tagm->macroname != NULL; tagm++) {
	he->tag = tagm->tag;
	xx = headerGet(h, he, 0);
	if (!xx)
	    continue;
	val = NULL;
	ival = 0;
	switch (he->t) {
	case RPM_UINT8_TYPE:
	    ival = (rpmuint64_t)he->p.ui8p[0];
	    val = numbuf;
	    /*@switchbreak@*/ break;
	case RPM_UINT16_TYPE:
	    ival = (rpmuint64_t)he->p.ui16p[0];
	    val = numbuf;
	    /*@switchbreak@*/ break;
	case RPM_UINT32_TYPE:
	    ival = (rpmuint64_t)he->p.ui32p[0];
	    val = numbuf;
	    /*@switchbreak@*/ break;
	case RPM_UINT64_TYPE:
	    ival = he->p.ui64p[0];
	    val = numbuf;
	    /*@switchbreak@*/ break;
	case RPM_STRING_TYPE:
	    val = he->p.str;
	    /*@switchbreak@*/ break;
	case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	case RPM_STRING_ARRAY_TYPE:
	case RPM_BIN_TYPE:
	default:
	    /*@switchbreak@*/ break;
	}
	
	if (val) {
/*@-duplicatequals@*/
	    if (val == numbuf)
		sprintf(numbuf, "%llu", (unsigned long long)ival);
/*@=duplicatequals@*/
	    addMacro(NULL, tagm->macroname, NULL, val, -1);
	}
	he->p.ptr = _free(he->p.ptr);
    }
    return 0;
}
/*@=globs =mods =incondefs@*/

/*@-globs -mods -incondefs@*/
int headerMacrosUnload(Header h)
	/*@globals rpmGlobalMacroContext @*/
	/*@modifies rpmGlobalMacroContext @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    struct tagMacro * tagm;
    int xx;

    for (tagm = tagMacros; tagm->macroname != NULL; tagm++) {
	he->tag = tagm->tag;
	xx = headerGet(h, he, 0);
	if (!xx)
	    continue;
	switch (he->t) {
	case RPM_UINT32_TYPE:
	    delMacro(NULL, tagm->macroname);
	    /*@switchbreak@*/ break;
	case RPM_STRING_TYPE:
	    delMacro(NULL, tagm->macroname);
	    /*@switchbreak@*/ break;
	case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	case RPM_STRING_ARRAY_TYPE:
	case RPM_BIN_TYPE:
	case RPM_UINT8_TYPE:
	case RPM_UINT16_TYPE:
	case RPM_UINT64_TYPE:
	default:
	    /*@switchbreak@*/ break;
	}
	he->p.ptr = _free(he->p.ptr);
    }

    /* XXX restore previous %{buildroot} (if any) */
    {   const char *s = rpmExpand("%{?_builddir}", NULL);
	if (s && *s) 
	    (void) delMacro(NULL, "_builddir");
	s = _free(s);
    }
    {   const char *s = rpmExpand("%{?buildroot}", NULL);
	if (s && *s) 
	    (void) delMacro(NULL, "buildroot");
	s = _free(s);
    }

    return 0;
}
/*@=globs =mods =incondefs@*/

int headerNEVRA(Header h, const char **np, /*@unused@*/ const char **ep,
		const char **vp, const char **rp, const char **ap)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));

/*@-onlytrans@*/
    if (np) {
	he->tag = RPMTAG_NAME;
	if (headerGet(h, he, 0)
	 && he->t == RPM_STRING_TYPE && he->c == 1)
	    *np = xstrdup(he->p.str);
	else
	    *np = NULL;
	he->p.ptr = _free(he->p.ptr);
    }
    if (vp) {
	he->tag = RPMTAG_VERSION;
	if (headerGet(h, he, 0)
	 && he->t == RPM_STRING_TYPE && he->c == 1)
	    *vp = xstrdup(he->p.str);
	else
	    *vp = NULL;
	he->p.ptr = _free(he->p.ptr);
    }
    if (rp) {
	he->tag = RPMTAG_RELEASE;
	if (headerGet(h, he, 0)
	 && he->t == RPM_STRING_TYPE && he->c == 1)
	    *rp = xstrdup(he->p.str);
	else
	    *rp = NULL;
	he->p.ptr = _free(he->p.ptr);
    }
    if (ap) {
#if !defined(RPM_VENDOR_OPENPKG) /* no-architecture-expose */
        /* do not expose the architecture as this is too less
           information, as in OpenPKG the "platform" is described by the
           architecture+operating-system combination. But as the whole
           "platform" information is actually overkill, just revert to the
           RPM 4 behaviour and do not expose any such information at all. */
	he->tag = RPMTAG_ARCH;
/*@-observertrans -readonlytrans@*/
	if (!headerIsEntry(h, he->tag))
	    *ap = xstrdup("pubkey");
	else
	if (!headerIsEntry(h, RPMTAG_SOURCERPM))
	    *ap = xstrdup("src");
/*@=observertrans =readonlytrans@*/
	else
	if (headerGet(h, he, 0)
	 && he->t == RPM_STRING_TYPE && he->c == 1)
	    *ap = xstrdup(he->p.str);
	else
#endif
	    *ap = NULL;
	he->p.ptr = _free(he->p.ptr);
    }
/*@=onlytrans@*/
    return 0;
}

rpmuint32_t hGetColor(Header h)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmuint32_t hcolor = 0;
    int xx;

    he->tag = RPMTAG_FILECOLORS;
    xx = headerGet(h, he, 0);
    if (xx && he->p.ptr != NULL && he->c > 0) {
	unsigned i;
	for (i = 0; i < (unsigned) he->c; i++)
	    hcolor |= he->p.ui32p[i];
    }
    he->p.ptr = _free(he->p.ptr);
    hcolor &= 0x0f;

    return hcolor;
}

void headerMergeLegacySigs(Header h, const Header sigh)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    HeaderIterator hi;
    int xx;

    if (h == NULL || sigh == NULL)
	return;

    for (hi = headerInit(sigh);
        headerNext(hi, he, 0);
        he->p.ptr = _free(he->p.ptr))
    {
	/* XXX Translate legacy signature tag values. */
	switch ((rpmSigTag)he->tag) {
	case RPMSIGTAG_SIZE:
	    he->tag = RPMTAG_SIGSIZE;
	    /*@switchbreak@*/ break;
	case RPMSIGTAG_MD5:
	    he->tag = RPMTAG_SIGMD5;
	    /*@switchbreak@*/ break;
	case RPMSIGTAG_PAYLOADSIZE:
	    he->tag = RPMTAG_ARCHIVESIZE;
	    /*@switchbreak@*/ break;
	case RPMSIGTAG_SHA1:
	case RPMSIGTAG_RSA:
	case RPMSIGTAG_DSA:
	case RPMSIGTAG_ECDSA:
	default:
	    /* Skip all unknown tags that are not in the signature tag range. */
	    if (!(he->tag >= HEADER_SIGBASE && he->tag < HEADER_TAGBASE))
		continue;
	    /*@switchbreak@*/ break;
	}
assert(he->p.ptr != NULL);
	if (!headerIsEntry(h, he->tag)) {
	    if (hdrchkType(he->t))
		continue;
	    if (hdrchkData(he->c))
		continue;
	    switch(he->t) {
	    default:
assert(0);	/* XXX keep gcc quiet */
		/*@switchbreak@*/ break;
	    case RPM_UINT8_TYPE:
	    case RPM_UINT16_TYPE:
	    case RPM_UINT32_TYPE:
	    case RPM_UINT64_TYPE:
		if (he->c != 1)
		    continue;
		/*@switchbreak@*/ break;
	    case RPM_STRING_TYPE:
	    case RPM_BIN_TYPE:
		if (he->c >= 16*1024)
		    continue;
		/*@switchbreak@*/ break;
	    case RPM_I18NSTRING_TYPE:
#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(0);
#endif
	    case RPM_STRING_ARRAY_TYPE:
		continue;
		/*@notreached@*/ /*@switchbreak@*/ break;
	    }
 	    xx = headerPut(h, he, 0);
assert(xx == 1);
	}
    }
    hi = headerFini(hi);
}

Header headerRegenSigHeader(const Header h, int noArchiveSize)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Header sigh = headerNew();
    HeaderIterator hi;
    int xx;

    for (hi = headerInit(h);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
	/* XXX Translate legacy signature tag values. */
	switch (he->tag) {
	case RPMTAG_SIGSIZE:
	    he->tag = (rpmTag) RPMSIGTAG_SIZE;
	    /*@switchbreak@*/ break;
	case RPMTAG_SIGMD5:
	    he->tag = (rpmTag) RPMSIGTAG_MD5;
	    /*@switchbreak@*/ break;
	case RPMTAG_ARCHIVESIZE:
	    /* XXX rpm-4.1 and later has archive size in signature header. */
	    if (noArchiveSize)
		continue;
	    he->tag = (rpmTag) RPMSIGTAG_PAYLOADSIZE;
	    /*@switchbreak@*/ break;
	case RPMTAG_SHA1HEADER:
	case RPMTAG_RSAHEADER:
	case RPMTAG_DSAHEADER:
	case RPMTAG_ECDSAHEADER:
	default:
	    /* Skip all unknown tags that are not in the signature tag range. */
	    if (!(he->tag >= HEADER_SIGBASE && he->tag < HEADER_TAGBASE))
		continue;
	    /*@switchbreak@*/ break;
	}
assert(he->p.ptr != NULL);
	if (!headerIsEntry(sigh, he->tag)) {
	    xx = headerPut(sigh, he, 0);
assert(xx == 1);
	}
    }
    hi = headerFini(hi);
    return sigh;
}
