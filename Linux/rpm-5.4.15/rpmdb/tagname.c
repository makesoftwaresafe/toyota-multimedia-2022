/**
 * \file rpmdb/tagname.c
 */

#include "system.h"

#include <rpmio_internal.h>	/* XXX DIGEST_CTX, xtolower, xstrcasecmp */
#include <rpmmacro.h>
#include <argv.h>
#define	_RPMTAG_INTERNAL
#include <rpmtag.h>
#include "debug.h"

/*@access headerTagTableEntry @*/
/*@access headerTagIndices @*/

#ifdef __cplusplus
GENfree(tagStore_t)
GENfree(headerTagTableEntry *)
#endif	/* __cplusplus */

/**
 * Load/sort arbitrary tags.
 * @retval *argvp	arbitrary tag array
 * @return		0 always
 */
static int tagLoadATags(/*@null@*/ ARGV_t * argvp,
		int (*cmp) (const void * avp, const void * bvp))
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies *argvp, rpmGlobalMacroContext, internalState @*/
{
    ARGV_t aTags = NULL;
    char * s = rpmExpand("%{?_arbitrary_tags}", NULL);

    if (s && *s)
	(void) argvSplit(&aTags, s, ":");
    else
	aTags = (ARGV_t) xcalloc(1, sizeof(*aTags));
    if (aTags && aTags[0] && aTags[1])
	(void) argvSort(aTags, (int (*) (const char **, const char **))cmp);
    s = _free(s);

    if (argvp)
	*argvp = aTags;
    else
	aTags = argvFree(aTags);
/*@-nullstate@*/	/* *argvp may be NULL */
    return 0;
/*@=nullstate@*/
}

/**
 * Compare tag table entries by name.
 * @param *avp		tag table entry a
 * @param *bvp		tag table entry b
 * @return		comparison
 */
static int tagCmpName(const void * avp, const void * bvp)
        /*@*/
{
    headerTagTableEntry a = *(headerTagTableEntry *) avp;
    headerTagTableEntry b = *(headerTagTableEntry *) bvp;
    return strcmp(a->name, b->name);
}

/**
 * Compare tag table entries by value.
 * @param *avp		tag table entry a
 * @param *bvp		tag table entry b
 * @return		comparison
 */
static int tagCmpValue(const void * avp, const void * bvp)
        /*@*/
{
    headerTagTableEntry a = *(headerTagTableEntry *) avp;
    headerTagTableEntry b = *(headerTagTableEntry *) bvp;
    int ret = ((int)a->val - (int)b->val);
    /* Make sure that sort is stable, longest name first. */
    if (ret == 0)
	ret = ((int)strlen(b->name) - (int)strlen(a->name));
    return ret;
}

/**
 * Load/sort a tag index.
 * @retval *ipp		tag index
 * @retval *np		no. of tags
 * @param cmp		sort compare routine
 * @return		0 always
 */
static int tagLoadIndex(headerTagTableEntry ** ipp, size_t * np,
		int (*cmp) (const void * avp, const void * bvp))
	/*@modifies *ipp, *np @*/
{
    headerTagTableEntry tte, *ip;
    size_t n = 0;

    ip = (headerTagTableEntry *) xcalloc(rpmTagTableSize, sizeof(*ip));
    n = 0;
/*@-dependenttrans@*/ /*@-observertrans@*/ /*@-castexpose@*/ /*@-mods@*/ /*@-modobserver@*/
    for (tte = rpmTagTable; tte->name != NULL; tte++) {
	ip[n] = tte;
	n++;
    }
assert(n == (size_t)rpmTagTableSize);
/*@=dependenttrans@*/ /*@=observertrans@*/ /*@=castexpose@*/ /*@=mods@*/ /*@=modobserver@*/

    if (n > 1)
	qsort(ip, n, sizeof(*ip), cmp);
    *ipp = ip;
    *np = n;
    return 0;
}

static char * _tagCanonicalize(const char * s)
	/*@*/
{
    const char * se;
    size_t nb = 0;
    char * te;
    char * t;
    int c;

    if (!strncasecmp(s, "RPMTAG_", sizeof("RPMTAG_")-1))
	s += sizeof("RPMTAG_") - 1;
    se = s;
    while ((c = (int)*se++) && xisalnum(c))
	nb++;

    te = t = (char *) xmalloc(nb+1);
    if (*s != '\0' && nb > 0) {
	*te++ = (char) xtoupper((int)*s++);
	nb--;
    }
    while (nb--)
	*te++ = (char) xtolower((int)*s++);
    *te = '\0';

    return t;
}

static rpmTag _tagGenerate(const char *s)
	/*@*/
{
    DIGEST_CTX ctx = rpmDigestInit(PGPHASHALGO_SHA1, RPMDIGEST_NONE);
    const char * digest = NULL;
    size_t digestlen = 0;
    size_t nb = strlen(s);
    rpmTag tag = (rpmTag)0;
    int xx;

    xx = rpmDigestUpdate(ctx, s, nb);
    xx = rpmDigestFinal(ctx, &digest, &digestlen, 0);
    if (digest && digestlen > 4) {
	memcpy(&tag, digest + (digestlen - 4), 4);
	tag = (rpmTag) (tag & 0x3fffffff);
	tag = (rpmTag) (tag | 0x40000000);
    }
    digest = _free(digest);
    return tag;
}

/* forward refs */
static const char * _tagName(rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;
static unsigned int _tagType(rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;
static rpmTag _tagValue(const char * tagstr)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/*@unchecked@*/
static struct headerTagIndices_s _rpmTags = {
    tagLoadIndex,
    NULL, 0, tagCmpName, _tagValue,
    NULL, 0, tagCmpValue, _tagName, (rpmTag (*)(rpmTag)) _tagType,
    256, NULL, NULL, _tagCanonicalize, _tagGenerate
};

/*@-compmempass@*/
/*@unchecked@*/
headerTagIndices rpmTags = &_rpmTags;
/*@=compmempass@*/

/*@-mods@*/
static const char * _tagName(rpmTag tag)
{
    char * nameBuf;
    size_t nameBufLen = 0;
    headerTagTableEntry t;
    size_t i, l, u;
    int comparison;
    int xx;
    char *s;

    if (_rpmTags.aTags == NULL)
	xx = tagLoadATags(&_rpmTags.aTags, NULL);
    if (_rpmTags.byValue == NULL)
	xx = tagLoadIndex(&_rpmTags.byValue, &_rpmTags.byValueSize,
		tagCmpValue);
    if (_rpmTags.nameBufLen == 0)
	_rpmTags.nameBufLen = 256;
    if (_rpmTags.nameBuf == NULL)
	_rpmTags.nameBuf = (char *) xcalloc(1, _rpmTags.nameBufLen);
    nameBuf = _rpmTags.nameBuf;
    nameBuf[0] = nameBuf[1] = '\0';
    nameBufLen = _rpmTags.nameBufLen;

    switch (tag) {
    case RPMDBI_PACKAGES:
	strncpy(nameBuf, "Packages", nameBufLen);
	break;
    case RPMDBI_DEPCACHE:
	strncpy(nameBuf, "Depcache", nameBufLen);
	break;
    case RPMDBI_ADDED:
	strncpy(nameBuf, "Added", nameBufLen);
	break;
    case RPMDBI_REMOVED:
	strncpy(nameBuf, "Removed", nameBufLen);
	break;
    case RPMDBI_AVAILABLE:
	strncpy(nameBuf, "Available", nameBufLen);
	break;
    case RPMDBI_HDLIST:
	strncpy(nameBuf, "Hdlist", nameBufLen);
	break;
    case RPMDBI_ARGLIST:
	strncpy(nameBuf, "Arglist", nameBufLen);
	break;
    case RPMDBI_FTSWALK:
	strncpy(nameBuf, "Ftswalk", nameBufLen);
	break;
    case RPMDBI_SEQNO:
	strncpy(nameBuf, "Seqno", nameBufLen);
	break;
    case RPMDBI_BTREE:
	strncpy(nameBuf, "Btree", nameBufLen);
	break;
    case RPMDBI_HASH:
	strncpy(nameBuf, "Hash", nameBufLen);
	break;
    case RPMDBI_QUEUE:
	strncpy(nameBuf, "Queue", nameBufLen);
	break;
    case RPMDBI_RECNO:
	strncpy(nameBuf, "Recno", nameBufLen);
	break;
    case RPMDBI_HEAP:
	strncpy(nameBuf, "Heap", nameBufLen);
	break;

    /* XXX make sure rpmdb indices are identically named. */
    case RPMTAG_CONFLICTS:
	strncpy(nameBuf, "Conflictname", nameBufLen);
	break;
    case RPMTAG_HDRID:
	strncpy(nameBuf, "Sha1header", nameBufLen);
	break;

    /* XXX make sure that h.['filenames'] in python "works". */
    case 0x54aafb71:
	strncpy(nameBuf, "Filenames", nameBufLen);
	break;

    default:
	if (_rpmTags.byValue == NULL)
	    break;
	l = 0;
	u = _rpmTags.byValueSize;
	while (l < u) {
	    i = (l + u) / 2;
	    t = _rpmTags.byValue[i];
	
	    comparison = ((int)tag - (int)t->val);

	    if (comparison < 0)
		u = i;
	    else if (comparison > 0)
		l = i + 1;
	    else {
		/* Make sure that the bsearch retrieve is stable. */
		while (i > 0 && tag == _rpmTags.byValue[i-1]->val) {
		    i--;
		    t = _rpmTags.byValue[i];
		}
		s = (*_rpmTags.tagCanonicalize) (t->name);
		strncpy(nameBuf, s, nameBufLen);
		s = _free(s);
		/*@loopbreak@*/ break;
	    }
	}
	break;
    }
    if (nameBuf[0] == '\0')
	xx = snprintf(nameBuf, nameBufLen, "Tag_0x%08x", (unsigned) tag);
    nameBuf[nameBufLen-1] = '\0';
/*@-globstate@*/	/* _rpmTags.nameBuf reachable. */
    return nameBuf;
/*@=globstate@*/
}
/*@=mods@*/

static unsigned int _tagType(rpmTag tag)
{
    headerTagTableEntry t;
    size_t i, l, u;
    int comparison;
    int xx;

    if (_rpmTags.aTags == NULL)
	xx = tagLoadATags(&_rpmTags.aTags, NULL);
    if (_rpmTags.byValue == NULL)
	xx = tagLoadIndex(&_rpmTags.byValue, &_rpmTags.byValueSize, tagCmpValue);

    switch (tag) {
    case RPMDBI_PACKAGES:
    case RPMDBI_DEPCACHE:
    case RPMDBI_ADDED:
    case RPMDBI_REMOVED:
    case RPMDBI_AVAILABLE:
    case RPMDBI_HDLIST:
    case RPMDBI_ARGLIST:
    case RPMDBI_FTSWALK:
    case RPMDBI_SEQNO:
    case RPMDBI_BTREE:
    case RPMDBI_HASH:
    case RPMDBI_QUEUE:
    case RPMDBI_RECNO:
    case RPMDBI_HEAP:
	break;
    default:
	if (_rpmTags.byValue == NULL)
	    break;
	l = 0;
	u = _rpmTags.byValueSize;
	while (l < u) {
	    i = (l + u) / 2;
	    t = _rpmTags.byValue[i];
	
	    comparison = ((int)tag - (int)t->val);

	    if (comparison < 0)
		u = i;
	    else if (comparison > 0)
		l = i + 1;
	    else {
		/* Make sure that the bsearch retrieve is stable. */
		while (i > 0 && t->val == _rpmTags.byValue[i-1]->val) {
		    i--;
		    t = _rpmTags.byValue[i];
		}
		return t->type;
	    }
	}
	break;
    }
    return 0;
}

static rpmTag _tagValue(const char * tagstr)
{
    headerTagTableEntry t;
    int comparison;
    size_t i, l, u;
    const char * s;
    rpmTag tag;
    int xx;

    /* XXX headerSprintf looks up by "RPMTAG_FOO", not "FOO". */
    if (!strncasecmp(tagstr, "RPMTAG_", sizeof("RPMTAG_")-1))
	tagstr += sizeof("RPMTAG_") - 1;

    if (!xstrcasecmp(tagstr, "Packages"))
	return RPMDBI_PACKAGES;
    if (!xstrcasecmp(tagstr, "Depcache"))
	return RPMDBI_DEPCACHE;
    if (!xstrcasecmp(tagstr, "Added"))
	return RPMDBI_ADDED;
    if (!xstrcasecmp(tagstr, "Removed"))
	return RPMDBI_REMOVED;
    if (!xstrcasecmp(tagstr, "Available"))
	return RPMDBI_AVAILABLE;
    if (!xstrcasecmp(tagstr, "Hdlist"))
	return RPMDBI_HDLIST;
    if (!xstrcasecmp(tagstr, "Arglist"))
	return RPMDBI_ARGLIST;
    if (!xstrcasecmp(tagstr, "Ftswalk"))
	return RPMDBI_FTSWALK;
    if (!xstrcasecmp(tagstr, "Seqno"))
	return RPMDBI_SEQNO;
    if (!xstrcasecmp(tagstr, "Btree"))
	return RPMDBI_BTREE;
    if (!xstrcasecmp(tagstr, "Hash"))
	return RPMDBI_HASH;
    if (!xstrcasecmp(tagstr, "Queue"))
	return RPMDBI_QUEUE;
    if (!xstrcasecmp(tagstr, "Recno"))
	return RPMDBI_RECNO;
    if (!xstrcasecmp(tagstr, "Heap"))
	return RPMDBI_HEAP;

    if (_rpmTags.aTags == NULL)
	xx = tagLoadATags(&_rpmTags.aTags, NULL);
    if (_rpmTags.byName == NULL)
	xx = tagLoadIndex(&_rpmTags.byName, &_rpmTags.byNameSize, tagCmpName);
    if (_rpmTags.byName == NULL)
	goto exit;

    l = 0;
    u = _rpmTags.byNameSize;
    while (l < u) {
	i = (l + u) / 2;
	t = _rpmTags.byName[i];
	
	comparison = xstrcasecmp(tagstr, t->name + (sizeof("RPMTAG_")-1));

	if (comparison < 0)
	    u = i;
	else if (comparison > 0)
	    l = i + 1;
	else
	    return t->val;
    }

exit:
    /* Generate an arbitrary tag string. */
    s = _tagCanonicalize(tagstr);
    tag = _tagGenerate(s);
    s = _free(s);
    return tag;
}

const char * tagName(rpmTag tag)
{
    return ((*rpmTags->tagName)(tag));
}

unsigned int tagType(rpmTag tag)
{
    return ((*rpmTags->tagType)(tag));
}

rpmTag tagValue(const char * tagstr)
{
    return ((*rpmTags->tagValue)(tagstr));
}

char * tagCanonicalize(const char * s)
{
    return ((*rpmTags->tagCanonicalize)(s));
}

rpmTag tagGenerate(const char * s)
{
    return ((*rpmTags->tagGenerate)(s));
}

void tagClean(headerTagIndices _rpmTags)
{
    if (_rpmTags == NULL)
	_rpmTags = rpmTags;
   if (_rpmTags) {
	_rpmTags->nameBuf = _free(_rpmTags->nameBuf);
	_rpmTags->byName = _free(_rpmTags->byName);
	_rpmTags->byValue = _free(_rpmTags->byValue);
	_rpmTags->aTags = argvFree(_rpmTags->aTags);
    }
}

tagStore_t tagStoreFree(tagStore_t dbiTags, size_t dbiNTags)
{
    if (dbiTags != NULL) {
	size_t i;
	for (i = 0; i < dbiNTags; i++) {
	    dbiTags[i].str = _free(dbiTags[i].str);
	    dbiTags[i].iob = rpmiobFree(dbiTags[i].iob);
	}
	dbiTags = _free(dbiTags);
    }
    return NULL;
}

#if defined(SUPPORT_IMPLICIT_TAG_DATA_TYPES)
/**
 * Validate that implicit and explicit types are identical.
 * @param he		tag container
 */
void tagTypeValidate(HE_t he);
void tagTypeValidate(HE_t he)
{
    /* XXX Re-map RPM_I18NSTRING_TYPE -> RPM_STRING_TYPE */
    if (he->t == RPM_I18NSTRING_TYPE)
	he->t = RPM_STRING_TYPE;

    /* XXX Arbitrary tags are always strings. */
    if ((he->tag & 0x40000000)
     && (he->t == RPM_STRING_TYPE || he->t == RPM_STRING_ARRAY_TYPE))
	return;

    /* XXX Make 0x3fffffff disappear for now. Signature? */
    if (he->tag == 0x3fffffff && he->t == RPM_BIN_TYPE)
	return;

/* XXX hack around known borkage for now. */
if (!(he->tag == 62))
if (!(he->tag == 261 || he->tag == 269))
if (!(he->tag == 1000 || he->tag == 1004 || he->tag == 1007))
if (!(he->tag == 1086 || he->tag == 1087))
if (he->t != (tagType(he->tag) & 0xffff))
fprintf(stderr, "==> warning: tag %u type(0x%x) != implicit type(0x%x)\n", (unsigned) he->tag, he->t, tagType(he->tag));
}
#endif
