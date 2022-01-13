/** \ingroup header
 * \file rpmdb/header.c
 */

/* RPM - Copyright (C) 1995-2002 Red Hat Software */

/* Data written to file descriptors is in network byte order.    */
/* Data read from file descriptors is expected to be in          */
/* network byte order and is converted on the fly to host order. */

#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>		/* XXX for rpmioPool et al */
#define	_RPMTAG_INTERNAL
#include <header_internal.h>

#include "debug.h"

/*@access Header @*/
/*@access HeaderIterator @*/
/*@access headerSprintfExtension @*/
/*@access headerTagTableEntry @*/

/*@access entryInfo @*/
/*@access indexEntry @*/

#ifdef __cplusplus
GENfree(rpmuint32_t *)
GENfree(indexEntry)
GENfree(HeaderIterator)
#endif	/* __cplusplus */

#if defined(SUPPORT_IMPLICIT_TAG_DATA_TYPES)
extern void tagTypeValidate(HE_t he)
	/*@*/;
#endif

/*@unchecked@*/
int _hdr_debug = 0;

/** \ingroup header
 */
/*@-type@*/
/*@observer@*/ /*@unchecked@*/
static unsigned char header_magic[8] = {
	0x8e, 0xad, 0xe8, 0x01, 0x00, 0x00, 0x00, 0x00
};
/*@=type@*/

/** \ingroup header
 * Size of header data types.
 */
/*@observer@*/ /*@unchecked@*/
static int typeSizes[16] =  { 
    0,	/*!< (unused) RPM_NULL_TYPE */
    1,	/*!< (unused) RPM_CHAR_TYPE */
    1,	/*!< RPM_UINT8_TYPE */
    2,	/*!< RPM_UINT16_TYPE */
    4,	/*!< RPM_UINT32_TYPE */
    8,	/*!< RPM_UINT64_TYPE */
    -1,	/*!< RPM_STRING_TYPE */
    1,	/*!< RPM_BIN_TYPE */
    -1,	/*!< RPM_STRING_ARRAY_TYPE */
    -1,	/*!< RPM_I18NSTRING_TYPE */
    0,  /*!< (unused) RPM_ASN1_TYPE */
    0,  /*!< (unused) RPM_OPENPGP_TYPE */
    0,
    0,
    0,
    0
};

/** \ingroup header
 * Maximum no. of bytes permitted in a header.
 */
/*@unchecked@*/
static size_t headerMaxbytes = (1024*1024*1024);

/**
 * Global header stats enabler.
 */
/*@unchecked@*/
int _hdr_stats = 0;

/*@-compmempass@*/
/*@unchecked@*/
static struct rpmop_s hdr_loadops;
/*@unchecked@*/ /*@relnull@*/
rpmop _hdr_loadops = &hdr_loadops;
/*@unchecked@*/
static struct rpmop_s hdr_getops;
/*@unchecked@*/ /*@relnull@*/
rpmop _hdr_getops = &hdr_getops;
/*@=compmempass@*/

void * headerGetStats(Header h, int opx)
{
    rpmop op = NULL;
    if (_hdr_stats)
    switch (opx) {
    case 18:	op = &h->h_loadops;	break;	/* RPMTS_OP_HDRLOAD */
    case 19:	op = &h->h_getops;	break;	/* RPMTS_OP_HDRGET */
    }
    return op;
}

/*@-mustmod@*/
static void headerScrub(void * _h)	/* XXX headerFini already in use */
	/*@modifies *_h @*/
{
    Header h = (Header) _h;

    if (h->index != NULL) {
	int mask = (HEADERFLAG_ALLOCATED | HEADERFLAG_MAPPED);
	indexEntry entry = h->index;
	size_t i;
	for (i = 0; i < h->indexUsed; i++, entry++) {
	    if ((h->flags & mask) && ENTRY_IS_REGION(entry)) {
		if (entry->length > 0) {
		    rpmuint32_t * ei = (rpmuint32_t *) entry->data;
		    if ((ei - 2) == h->blob) {
			if (h->flags & HEADERFLAG_MAPPED) {
			    if (munmap(h->blob, h->bloblen) != 0)
				fprintf(stderr,
					"==> munmap(%p[%u]) error(%d): %s\n",
					h->blob, (unsigned)h->bloblen,
					errno, strerror(errno));
			    h->blob = NULL;
			} else
			    h->blob = _free(h->blob);
			h->bloblen = 0;
		    }
		    entry->data = NULL;
		}
	    } else if (!ENTRY_IN_REGION(entry)) {
		entry->data = _free(entry->data);
	    }
	    entry->data = NULL;
	    entry->length = 0;
	}
	h->index = _free(h->index);
    }
    h->origin = _free(h->origin);
    h->baseurl = _free(h->baseurl);
    h->digest = _free(h->digest);
    h->parent = _free(h->parent);

/*@-nullstate@*/
    if (_hdr_stats) {
	if (_hdr_loadops)	/* RPMTS_OP_HDRLOAD */
	    (void) rpmswAdd(_hdr_loadops, (rpmop) headerGetStats(h, 18));
	if (_hdr_getops)	/* RPMTS_OP_HDRGET */
	    (void) rpmswAdd(_hdr_getops, (rpmop) headerGetStats(h, 19));
    }
/*@=nullstate@*/
}
/*@=mustmod@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _headerPool;

static Header headerGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _headerPool, fileSystem @*/
	/*@modifies pool, _headerPool, fileSystem @*/
{
    Header h;

    if (_headerPool == NULL) {
ANNOTATE_BENIGN_RACE(&_headerPool, "");
	_headerPool = rpmioNewPool("h", sizeof(*h), -1, _hdr_debug,
			NULL, NULL, headerScrub);
	pool = _headerPool;
    }
    h = (Header) rpmioGetPool(pool, sizeof(*h));
    memset(((char *)h)+sizeof(h->_item), 0, sizeof(*h)-sizeof(h->_item));
    return h;
}

Header headerNew(void)
{
    Header h = headerGetPool(_headerPool);

    (void) memcpy(h->magic, header_magic, sizeof(h->magic));
    h->blob = NULL;
    h->bloblen = 0;
    h->origin = NULL;
    h->baseurl = NULL;
    h->digest = NULL;
    h->parent = NULL;
    h->rpmdb = NULL;
    memset(&h->sb, 0, sizeof(h->sb));
    h->instance = 0;
    h->startoff = 0;
    h->endoff = 0;
    memset(&h->h_loadops, 0, sizeof(h->h_loadops));
    memset(&h->h_getops, 0, sizeof(h->h_getops));
    h->indexAlloced = INDEX_MALLOC_SIZE;
    h->indexUsed = 0;
    h->flags = HEADERFLAG_SORTED;

    h->index = (indexEntry) (h->indexAlloced
	? xcalloc(h->indexAlloced, sizeof(*h->index))
	: NULL);

/*@-globstate -nullret -observertrans @*/
    return headerLink(h);
/*@=globstate =nullret =observertrans @*/
}

/**
 */
static int indexCmp(const void * avp, const void * bvp)
	/*@*/
{
    /*@-castexpose@*/
    indexEntry ap = (indexEntry) avp, bp = (indexEntry) bvp;
    /*@=castexpose@*/
    return ((int)ap->info.tag - (int)bp->info.tag);
}

/** \ingroup header
 * Sort tags in header.
 * @param h		header
 */
static
void headerSort(Header h)
	/*@modifies h @*/
{
    if (!(h->flags & HEADERFLAG_SORTED)) {
	qsort(h->index, h->indexUsed, sizeof(*h->index), indexCmp);
	h->flags |= HEADERFLAG_SORTED;
    }
}

/**
 */
static int offsetCmp(const void * avp, const void * bvp) /*@*/
{
    /*@-castexpose@*/
    indexEntry ap = (indexEntry) avp, bp = (indexEntry) bvp;
    /*@=castexpose@*/
    int rc = ((int)ap->info.offset - (int)bp->info.offset);

    if (rc == 0) {
	/* Within a region, entries sort by address. Added drips sort by tag. */
	if (ap->info.offset < 0)
	    rc = (((char *)ap->data) - ((char *)bp->data));
	else
	    rc = ((int)ap->info.tag - (int)bp->info.tag);
    }
    return rc;
}

/** \ingroup header
 * Restore tags in header to original ordering.
 * @param h		header
 */
static
void headerUnsort(Header h)
	/*@modifies h @*/
{
    qsort(h->index, h->indexUsed, sizeof(*h->index), offsetCmp);
}

size_t headerSizeof(Header h)
{
    indexEntry entry;
    size_t size = 0;
    size_t pad = 0;
    size_t i;

    if (h == NULL)
	return size;

    headerSort(h);

    size += sizeof(header_magic);	/* XXX HEADER_MAGIC_YES */

    /*@-sizeoftype@*/
    size += 2 * sizeof(rpmuint32_t);	/* count of index entries */
    /*@=sizeoftype@*/

    for (i = 0, entry = h->index; i < h->indexUsed; i++, entry++) {
	size_t diff;
	rpmTagType type;

	/* Regions go in as is ... */
        if (ENTRY_IS_REGION(entry)) {
	    size += entry->length;
	    /* XXX Legacy regions do not include the region tag and data. */
	    /*@-sizeoftype@*/
	    if (i == 0 && (h->flags & HEADERFLAG_LEGACY))
		size += sizeof(struct entryInfo_s) + entry->info.count;
	    /*@=sizeoftype@*/
	    continue;
        }

	/* ... and region elements are skipped. */
	if (entry->info.offset < 0)
	    continue;

	/* Alignment */
	type = entry->info.type;
	if (typeSizes[type] > 1) {
	    diff = typeSizes[type] - (size % typeSizes[type]);
	    if ((int)diff != typeSizes[type]) {
		size += diff;
		pad += diff;
	    }
	}

	/*@-sizeoftype@*/
	size += sizeof(struct entryInfo_s) + entry->length;
	/*@=sizeoftype@*/
    }

    return size;
}

/**
 * Return length of entry data.
 * @param type		entry data type
 * @param *p		tag container data
 * @param count		entry item count
 * @param onDisk	data is concatenated strings (with NUL's))?
 * @param *pend		pointer to end of tag container data (or NULL)
 * @return		no. bytes in data, 0 on failure
 */
static size_t dataLength(rpmTagType type, rpmTagData * p, rpmTagCount count,
		int onDisk, /*@null@*/ rpmTagData * pend)
	/*@*/
{
    const unsigned char * s = (unsigned char *) (*p).ui8p;
    const unsigned char * se = (unsigned char *) (pend ? (*pend).ui8p : NULL);
    size_t length = 0;

    switch (type) {
#if !defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
assert(0);
#endif
    case RPM_STRING_TYPE:
	if (count != 1)
	    return 0;
	while (*s++ != '\0') {
	    if (se && s > se)
		return 0;
	    length++;
	}
	length++;	/* count nul terminator too. */
	break;
	/* These are like RPM_STRING_TYPE, except they're *always* an array */
	/* Compute sum of length of all strings, including nul terminators */
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_STRING_ARRAY_TYPE:
	if (onDisk) {
	    while (count--) {
		length++;       /* count nul terminator too */
		while (*s++ != '\0') {
		    if (se && s > se)
			return 0;
		    length++;
		}
	    }
	} else {
	    const char ** av = (*p).argv;
	    while (count--) {
		/* add one for null termination */
		length += strlen(*av++) + 1;
	    }
	}
	break;
    default:
	if (typeSizes[type] == -1)
	    return 0;
	length = typeSizes[(type & 0xf)] * count;
	if ((se && (s + length) > se))
	    return 0;
	break;
    }

    return length;
}

/** \ingroup header
 * Swab rpmuint64_t/rpmuint32_t/rpmuint16_t arrays within header region.
 *
 */
static unsigned char * tagSwab(/*@out@*/ /*@returned@*/ unsigned char * t,
		const HE_t he, size_t nb)
	/*@modifies *t @*/
{
    rpmuint32_t i;

    switch (he->t) {
    case RPM_UINT64_TYPE:
    {	rpmuint32_t * tt = (rpmuint32_t *)t;
assert(nb == (2 * he->c * sizeof(*tt)));
	for (i = 0; i < he->c; i++) {
	    rpmuint32_t j = 2 * i;
	    rpmuint32_t b = (rpmuint32_t) htonl(he->p.ui32p[j]);
	    tt[j  ] = (rpmuint32_t) htonl(he->p.ui32p[j+1]);
	    tt[j+1] = b;
	}
    }   break;
    case RPM_UINT32_TYPE:
    {	rpmuint32_t * tt = (rpmuint32_t *)t;
assert(nb == (he->c * sizeof(*tt)));
	for (i = 0; i < he->c; i++)
	    tt[i] = (rpmuint32_t) htonl(he->p.ui32p[i]);
    }   break;
    case RPM_UINT16_TYPE:
    {	rpmuint16_t * tt = (rpmuint16_t *)t;
assert(nb == (he->c * sizeof(*tt)));
	for (i = 0; i < he->c; i++)
	    tt[i] = (rpmuint16_t) htons(he->p.ui16p[i]);
    }   break;
    default:
assert(he->p.ptr != NULL);
	if ((void *)t != he->p.ptr && nb)
	    memcpy(t, he->p.ptr, nb);
	t += nb;
	break;
    }
/*@-compdef@*/
    return t;
/*@=compdef@*/
}

/**
 * Always realloc HE_t memory.
 * @param he		tag container
 * @return		1 on success, 0 on failure
 */
static int rpmheRealloc(HE_t he)
	/*@modifies he @*/
{
    size_t nb = 0;
    int rc = 1;		/* assume success */

    switch (he->t) {
    default:
assert(0);	/* XXX stop unimplemented oversights. */
	break;
    case RPM_BIN_TYPE:
	he->freeData = 1;	/* XXX RPM_BIN_TYPE is malloc'd */
	/*@fallthrough@*/
    case 1:
    case RPM_UINT8_TYPE:
	nb = he->c * sizeof(*he->p.ui8p);
	break;
    case RPM_UINT16_TYPE:
	nb = he->c * sizeof(*he->p.ui16p);
	break;
    case RPM_UINT32_TYPE:
	nb = he->c * sizeof(*he->p.ui32p);
	break;
    case RPM_UINT64_TYPE:
	nb = he->c * sizeof(*he->p.ui64p);
	break;
#if !defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
assert(0);
#endif
    case RPM_STRING_TYPE:
	if (he->p.str)
	    nb = strlen(he->p.str) + 1;
	else
	    rc = 0;
	break;
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_STRING_ARRAY_TYPE:
	break;
    }

    /* Allocate all returned storage (if not already). */
    if (he->p.ptr && nb && !he->freeData) {
	void * ptr = (void *) DRD_xmalloc(nb);
	if (tagSwab((unsigned char *)ptr, he, nb) != NULL)
	    he->p.ptr = ptr;
	else {
/*@-dependenttrans@*/
	    ptr = _free(ptr);
/*@=dependenttrans@*/
	    rc = 0;
	}
    }

    if (rc)
	he->freeData = 1;

    return rc;
}

/** \ingroup header
 * Swab rpmuint64_t/rpmuint32_t/rpmuint16_t arrays within header region.
 *
 * This code is way more twisty than I would like.
 *
 * A bug with RPM_I18NSTRING_TYPE in rpm-2.5.x (fixed in August 1998)
 * causes the offset and length of elements in a header region to disagree
 * regarding the total length of the region data.
 *
 * The "fix" is to compute the size using both offset and length and
 * return the larger of the two numbers as the size of the region.
 * Kinda like computing left and right Riemann sums of the data elements
 * to determine the size of a data structure, go figger :-).
 *
 * There's one other twist if a header region tag is in the set to be swabbed,
 * as the data for a header region is located after all other tag data.
 *
 * @param entry		header entry
 * @param il		no. of entries
 * @param dl		start no. bytes of data
 * @param pe		header physical entry pointer (swapped)
 * @param dataStart	header data start
 * @param dataEnd	header data end
 * @param regionid	region offset
 * @return		no. bytes of data in region, 0 on error
 */
/*@-globs@*/	/* XXX rpm_typeAlign usage */
static rpmuint32_t regionSwab(/*@null@*/ indexEntry entry, rpmuint32_t il, rpmuint32_t dl,
		entryInfo pe,
		unsigned char * dataStart,
		/*@null@*/ const unsigned char * dataEnd,
		rpmint32_t regionid)
	/*@modifies *entry @*/
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTagData p;
    rpmTagData pend;
    unsigned char * tprev = NULL;
    unsigned char * t = NULL;
    size_t tdel = 0;
    size_t tl = dl;
    struct indexEntry_s ieprev;

assert(dataEnd != NULL);
assert(entry != NULL);

    memset(&ieprev, 0, sizeof(ieprev));
    for (; il > 0; il--, pe++) {
	struct indexEntry_s ie;
	rpmTagType type;

	ie.info.tag = (rpmTag) ntohl(pe->tag);
	ie.info.type = (rpmTagType) ntohl(pe->type);
	ie.info.count = (rpmuint32_t) ntohl(pe->count);
	ie.info.offset = (rpmint32_t) ntohl(pe->offset);
assert(ie.info.offset >= 0);	/* XXX insurance */

	if (hdrchkType(ie.info.type))
	    return 0;
	if (hdrchkData(ie.info.count))
	    return 0;
	if (hdrchkData(ie.info.offset))
	    return 0;
	if (hdrchkAlign(ie.info.type, ie.info.offset))
	    return 0;

	ie.data = t = dataStart + ie.info.offset;
	if (dataEnd && t >= dataEnd)
	    return 0;

	p.ptr = ie.data;
	pend.ui8p = (rpmuint8_t *) dataEnd;

	/* Compute the tag data store length using offsets. */
	if (il > 1)
	    ie.length = ((rpmuint32_t)ntohl(pe[1].offset) - ie.info.offset);
	else {
	    /* XXX (dataEnd - t) +/- REGION_TAG_COUNT forces dataLength() */
	    ie.length = dataLength(ie.info.type, &p, ie.info.count, 1, &pend);
	}

	if (ie.length == 0 || hdrchkData(ie.length))
	    return 0;

	ie.rdlen = 0;

	if (entry) {
	    ie.info.offset = regionid;
/*@-kepttrans@*/	/* entry->data is kept */
	    *entry = ie;	/* structure assignment */
/*@=kepttrans@*/
	    entry++;
	}

	/* Alignment */
	type = ie.info.type;
	if (typeSizes[type] > 1) {
	    size_t diff = typeSizes[type] - (dl % typeSizes[type]);
	    if ((int)diff != typeSizes[type]) {
		dl += diff;
#ifdef	DYING
		if (ieprev.info.type == RPM_I18NSTRING_TYPE)
		    ieprev.length += diff;
#endif
	    }
	}
	tdel = (tprev ? (t - tprev) : 0);
#ifdef	DYING
	if (ieprev.info.type == RPM_I18NSTRING_TYPE)
	    tdel = ieprev.length;
#endif

	if (ie.info.tag >= HEADER_I18NTABLE) {
	    tprev = t;
	} else {
	    tprev = dataStart;
	    /* XXX HEADER_IMAGE tags don't include region sub-tag. */
	    /*@-sizeoftype@*/
	    if (ie.info.tag == HEADER_IMAGE)
		tprev -= REGION_TAG_COUNT;
	    /*@=sizeoftype@*/
	}

	t += ie.length;

	dl += ie.length;
	if (dataEnd && (dataStart + dl) > dataEnd) return 0;
	tl += tdel;
	ieprev = ie;	/* structure assignment */

    }
    tdel = (tprev ? (t - tprev) : 0);
    tl += tdel;

    /* XXX
     * There are two hacks here:
     *	1) tl is 16b (i.e. REGION_TAG_COUNT) short while doing headerReload().
     *	2) the 8/98 rpm bug with inserting i18n tags needs to use tl, not dl.
     */
    /*@-sizeoftype@*/
    if (tl+REGION_TAG_COUNT == dl)
	tl += REGION_TAG_COUNT;
    /*@=sizeoftype@*/

    return dl;
}
/*@=globs@*/

void * headerUnload(Header h, size_t * lenp)
{
    void * sw;
    rpmuint32_t * ei = NULL;
    entryInfo pe;
    unsigned char * dataStart;
    unsigned char * te;
    unsigned pad;
    size_t len = 0;
    rpmuint32_t il = 0;
    rpmuint32_t dl = 0;
    indexEntry entry; 
    rpmTagType type;
    size_t i;
    size_t drlen;
    size_t ndribbles;
    size_t driplen;
    size_t ndrips;
    int legacy = 0;

    if ((sw = headerGetStats(h, 18)) != NULL)	/* RPMTS_OP_HDRLOAD */
	(void) rpmswEnter((rpmop)sw, 0);

    /* Sort entries by (offset,tag). */
    headerUnsort(h);

    /* Compute (il,dl) for all tags, including those deleted in region. */
    pad = 0;
    drlen = ndribbles = driplen = ndrips = 0;
    for (i = 0, entry = h->index; i < h->indexUsed; i++, entry++) {
	if (ENTRY_IS_REGION(entry)) {
	    rpmuint32_t rdl;
	    rpmuint32_t ril;
	    rpmint32_t rid;

assert(entry->info.offset <= 0);	/* XXX insurance */
	    rdl = (rpmuint32_t)-entry->info.offset;	/* negative offset */
	    ril = (rpmuint32_t)(rdl/sizeof(*pe));
	    rid = (rpmuint32_t)entry->info.offset;

	    il += ril;
	    dl += entry->rdlen + entry->info.count;
	    /* XXX Legacy regions do not include the region tag and data. */
	    if (i == 0 && (h->flags & HEADERFLAG_LEGACY))
		il += 1;

	    /* Skip rest of entries in region, but account for dribbles. */
	    for (; i < h->indexUsed && entry->info.offset <= rid+1; i++, entry++) {
		if (entry->info.offset <= rid)
		    /*@innercontinue@*/ continue;

		/* Alignment */
		type = entry->info.type;
		if (typeSizes[type] > 1) {
		    size_t diff = typeSizes[type] - (dl % typeSizes[type]);
		    if ((int)diff != typeSizes[type]) {
			drlen += diff;
			pad += diff;
			dl += diff;
		    }
		}

		ndribbles++;
		il++;
		drlen += entry->length;
		dl += entry->length;
	    }
	    i--;
	    entry--;
	    continue;
	}

	/* Ignore deleted drips. */
	if (entry->data == NULL || entry->length == 0)
	    continue;

	/* Alignment */
	type = entry->info.type;
	if (typeSizes[type] > 1) {
	    size_t diff = typeSizes[type] - (dl % typeSizes[type]);
	    if ((int)diff != typeSizes[type]) {
		driplen += diff;
		pad += diff;
		dl += diff;
	    } else
		diff = 0;
	}

	ndrips++;
	il++;
	driplen += entry->length;
	dl += entry->length;
    }

    /* Sanity checks on header intro. */
    if (hdrchkTags(il) || hdrchkData(dl))
	goto errxit;

    len = sizeof(il) + sizeof(dl) + (il * sizeof(*pe)) + dl;

    ei = (rpmuint32_t *) xmalloc(len);
    ei[0] = (rpmuint32_t) htonl(il);
    ei[1] = (rpmuint32_t) htonl(dl);

    pe = (entryInfo) &ei[2];
    dataStart = te = (unsigned char *) (pe + il);

    pad = 0;
    for (i = 0, entry = h->index; i < h->indexUsed; i++, entry++) {
	const char * src;
	unsigned char *t;
	size_t rdlen;

	if (entry->data == NULL || entry->length == 0)
	    continue;

	t = te;
	pe->tag = (rpmTag) htonl(entry->info.tag);
	pe->type = (rpmTagType) htonl(entry->info.type);
	pe->count = (rpmuint32_t) htonl(entry->info.count);

	if (ENTRY_IS_REGION(entry)) {
	    rpmuint32_t rdl;
	    rpmuint32_t ril;
	    rpmint32_t rid;

assert(entry->info.offset <= 0);	/* XXX insurance */

	    rdl = (rpmuint32_t)-entry->info.offset;	/* negative offset */
	    ril = (rpmuint32_t)(rdl/sizeof(*pe) + ndribbles);
	    rid = (rpmuint32_t)entry->info.offset;

	    src = (char *)entry->data;
	    rdlen = entry->rdlen;

	    /* XXX Legacy regions do not include the region tag and data. */
	    if (i == 0 && (h->flags & HEADERFLAG_LEGACY)) {
		rpmuint32_t stei[4];

		legacy = 1;
		memcpy(pe+1, src, rdl);
		memcpy(te, src + rdl, rdlen);
		te += rdlen;

		pe->offset = (rpmint32_t) htonl(te - dataStart);
		stei[0] = (rpmuint32_t) pe->tag;
		stei[1] = (rpmuint32_t) pe->type;
		stei[2] = (rpmuint32_t) htonl(-rdl-entry->info.count);
		stei[3] = (rpmuint32_t) pe->count;
		memcpy(te, stei, entry->info.count);
		te += entry->info.count;
		ril++;
		rdlen += entry->info.count;

	    } else {

		memcpy(pe+1, src + sizeof(*pe), ((ril-1) * sizeof(*pe)));
		memcpy(te, src + (ril * sizeof(*pe)), rdlen+entry->info.count+drlen);
		te += rdlen;
		{   /*@-castexpose@*/
		    entryInfo se = (entryInfo)src;
		    /*@=castexpose@*/
		    rpmint32_t off = (rpmint32_t) ntohl(se->offset);
		    pe->offset = (rpmint32_t)((off)
			? htonl(te - dataStart) : htonl(off));
		}
		te += entry->info.count + drlen;

	    }

	    /* Skip rest of entries in region. */
	    while (i < h->indexUsed && entry->info.offset <= rid+1) {
		i++;
		entry++;
	    }
	    i--;
	    entry--;
	    pe += ril;
	    continue;
	}

	/* Ignore deleted drips. */
	if (entry->data == NULL || entry->length == 0)
	    continue;

	/* Alignment */
	type = entry->info.type;
	if (typeSizes[type] > 1) {
	    size_t diff;
	    diff = typeSizes[type] - ((te - dataStart) % typeSizes[type]);
	    if ((int)diff != typeSizes[type]) {
		memset(te, 0, diff);
		te += diff;
		pad += diff;
	    }
	}

	/* Move tag data into header data store. */
	pe->offset = (rpmint32_t) htonl(te - dataStart);
	memcpy(te, entry->data, entry->length);
	te += entry->length;
	pe++;
    }
   
    /* Insure that there are no memcpy underruns/overruns. */
    if (((unsigned char *)pe) != dataStart)
	goto errxit;
    if ((((unsigned char *)ei)+len) != te)
	goto errxit;

    if (lenp)
	*lenp = len;

    h->flags &= ~HEADERFLAG_SORTED;
    headerSort(h);

    if (sw != NULL)	(void) rpmswExit((rpmop)sw, len);

    return (void *) ei;

errxit:
    if (sw != NULL)	(void) rpmswExit((rpmop)sw, len);
    /*@-usereleased@*/
    ei = _free(ei);
    /*@=usereleased@*/
    return (void *) ei;
}

/**
 * Find matching (tag,type) entry in header.
 * @param h		header
 * @param tag		entry tag
 * @param type		entry type
 * @return 		header entry
 */
static /*@null@*/
indexEntry findEntry(/*@null@*/ Header h, rpmTag tag, rpmTagType type)
	/*@modifies h @*/
{
    indexEntry entry, entry2, last;
    struct indexEntry_s key;

    if (h == NULL) return NULL;
    if (!(h->flags & HEADERFLAG_SORTED)) headerSort(h);

    key.info.tag = tag;

    entry2 = entry = (indexEntry)
	bsearch(&key, h->index, h->indexUsed, sizeof(*h->index), indexCmp);
    if (entry == NULL)
	return NULL;

    if (type == 0)
	return entry;

    /* look backwards */
    while (entry->info.tag == tag && entry->info.type != type &&
	   entry > h->index) entry--;

    if (entry->info.tag == tag && entry->info.type == type)
	return entry;

    last = h->index + h->indexUsed;
    /*@-usereleased@*/ /* FIX: entry2 = entry. Code looks bogus as well. */
    while (entry2->info.tag == tag && entry2->info.type != type &&
	   entry2 < last) entry2++;
    /*@=usereleased@*/

    if (entry->info.tag == tag && entry->info.type == type)
	return entry;

    return NULL;
}

/** \ingroup header
 * Delete tag in header.
 * Removes all entries of type tag from the header, returns 1 if none were
 * found.
 *
 * @param h		header
 * @param tag		tag
 * @return		0 on success, 1 on failure (INCONSISTENT)
 */
static
int headerRemoveEntry(Header h, rpmTag tag)
	/*@modifies h @*/
{
    indexEntry last = h->index + h->indexUsed;
    indexEntry entry, first;
    int ne;

    entry = findEntry(h, tag, (rpmTagType)0);
    if (!entry) return 1;

    /* Make sure entry points to the first occurrence of this tag. */
    while (entry > h->index && (entry - 1)->info.tag == tag)  
	entry--;

    /* Free data for tags being removed. */
    for (first = entry; first < last; first++) {
	void * data;
	if (first->info.tag != tag)
	    break;
	data = first->data;
	first->data = NULL;
	first->length = 0;
	if (ENTRY_IN_REGION(first))
	    continue;
	data = _free(data);
    }

    ne = (first - entry);
    if (ne > 0) {
	h->indexUsed -= ne;
	ne = last - first;
	if (ne > 0)
	    memmove(entry, first, (ne * sizeof(*entry)));
    }

    return 0;
}

Header headerLoad(void * uh)
{
    void * sw = NULL;
    rpmuint32_t * ei = (rpmuint32_t *) uh;
    rpmuint32_t il = (rpmuint32_t) ntohl(ei[0]);		/* index length */
    rpmuint32_t dl = (rpmuint32_t) ntohl(ei[1]);		/* data length */
    /*@-sizeoftype@*/
    size_t pvlen = sizeof(il) + sizeof(dl) +
               (il * sizeof(struct entryInfo_s)) + dl;
    /*@=sizeoftype@*/
    void * pv = uh;
    Header h = NULL;
    entryInfo pe;
    unsigned char * dataStart;
    unsigned char * dataEnd;
    indexEntry entry; 
    rpmuint32_t rdlen;
    int i;

    /* Sanity checks on header intro. */
    if (hdrchkTags(il) || hdrchkData(dl))
	goto errxit;

    ei = (rpmuint32_t *) pv;
    /*@-castexpose@*/
    pe = (entryInfo) &ei[2];
    /*@=castexpose@*/
    dataStart = (unsigned char *) (pe + il);
    dataEnd = dataStart + dl;

    h = headerGetPool(_headerPool);
    memset(&h->h_loadops, 0, sizeof(h->h_loadops));
    if ((sw = headerGetStats(h, 18)) != NULL)	/* RPMTS_OP_HDRLOAD */
	(void) rpmswEnter((rpmop)sw, 0);
    {	unsigned char * hmagic = header_magic;
	(void) memcpy(h->magic, hmagic, sizeof(h->magic));
    }
    /*@-assignexpose -kepttrans@*/
    h->blob = uh;
    h->bloblen = pvlen;
    /*@=assignexpose =kepttrans@*/
    h->origin = NULL;
    h->baseurl = NULL;
    h->digest = NULL;
    h->parent = NULL;
    h->rpmdb = NULL;
    memset(&h->sb, 0, sizeof(h->sb));
    h->instance = 0;
    h->startoff = 0;
    h->endoff = (rpmuint32_t) pvlen;
    memset(&h->h_getops, 0, sizeof(h->h_getops));
    h->indexAlloced = il + 1;
    h->indexUsed = il;
    h->index = (indexEntry) DRD_xcalloc(h->indexAlloced, sizeof(*h->index));
    h->flags = HEADERFLAG_SORTED;
    h = headerLink(h);
assert(h != NULL);

    entry = h->index;
    i = 0;
    if (!(htonl(pe->tag) < HEADER_I18NTABLE)) {
	h->flags |= HEADERFLAG_LEGACY;
	entry->info.type = REGION_TAG_TYPE;
	entry->info.tag = (rpmTag) HEADER_IMAGE;
	/*@-sizeoftype@*/
	entry->info.count = (rpmTagCount)REGION_TAG_COUNT;
	/*@=sizeoftype@*/
	entry->info.offset = ((unsigned char *)pe - dataStart); /* negative offset */

	/*@-assignexpose@*/
	entry->data = pe;
	/*@=assignexpose@*/
	entry->length = pvlen - sizeof(il) - sizeof(dl);
	rdlen = regionSwab(entry+1, il, 0, pe, dataStart, dataEnd, entry->info.offset);
#if 0	/* XXX don't check, the 8/98 i18n bug fails here. */
	if (rdlen != dl)
	    goto errxit;
#endif
	entry->rdlen = rdlen;
	entry++;
	h->indexUsed++;
    } else {
	rpmuint32_t rdl;
	rpmuint32_t ril;

	h->flags &= ~HEADERFLAG_LEGACY;

	entry->info.type = (rpmTagType) htonl(pe->type);
	entry->info.count = (rpmuint32_t) htonl(pe->count);
	entry->info.tag = (rpmTag) htonl(pe->tag);

	if (!ENTRY_IS_REGION(entry))
	    goto errxit;
	if (entry->info.type != REGION_TAG_TYPE)
	    goto errxit;
	if (entry->info.count != REGION_TAG_COUNT)
	    goto errxit;

	{   rpmint32_t off = (rpmint32_t) ntohl(pe->offset);

	    if (hdrchkData(off))
		goto errxit;
	    if (off) {
/*@-sizeoftype@*/
		size_t nb = REGION_TAG_COUNT;
/*@=sizeoftype@*/
		rpmuint32_t * stei = (rpmuint32_t *)
			memcpy(alloca(nb), dataStart + off, nb);
		rdl = (rpmuint32_t)-ntohl(stei[2]);	/* negative offset */
assert((rpmint32_t)rdl >= 0);	/* XXX insurance */
		ril = (rpmuint32_t)(rdl/sizeof(*pe));
		if (hdrchkTags(ril) || hdrchkData(rdl))
		    goto errxit;
	    } else {
		ril = il;
		/*@-sizeoftype@*/
		rdl = (rpmuint32_t)(ril * sizeof(struct entryInfo_s));
		/*@=sizeoftype@*/
		entry->info.tag = (rpmTag) HEADER_IMAGE;
	    }
	}
	entry->info.offset = (rpmint32_t) -rdl;	/* negative offset */

	/*@-assignexpose@*/
	entry->data = pe;
	/*@=assignexpose@*/
	entry->length = pvlen - sizeof(il) - sizeof(dl);
	rdlen = regionSwab(entry+1, (ril-1), 0, pe+1, dataStart, dataEnd, entry->info.offset);
	if (rdlen == 0)
	    goto errxit;
	entry->rdlen = rdlen;

	if (ril < (rpmuint32_t)h->indexUsed) {
	    indexEntry newEntry = entry + ril;
	    size_t ne = (h->indexUsed - ril);
	    rpmint32_t rid = entry->info.offset+1;

	    /* Load dribble entries from region. */
	    rdlen = regionSwab(newEntry, (rpmuint32_t)ne, rdlen, pe+ril,
				dataStart, dataEnd, rid);
	    if (rdlen == 0)
		goto errxit;

	  { indexEntry firstEntry = newEntry;
	    size_t save = h->indexUsed;
	    size_t j;

	    /* Dribble entries replace duplicate region entries. */
	    h->indexUsed -= ne;
	    for (j = 0; j < ne; j++, newEntry++) {
		(void) headerRemoveEntry(h, newEntry->info.tag);
		if (newEntry->info.tag == HEADER_BASENAMES)
		    (void) headerRemoveEntry(h, (rpmTag) HEADER_OLDFILENAMES);
	    }

	    /* If any duplicate entries were replaced, move new entries down. */
	    if (h->indexUsed < (save - ne)) {
		memmove(h->index + h->indexUsed, firstEntry,
			(ne * sizeof(*entry)));
	    }
	    h->indexUsed += ne;
	  }
	}

	rdlen += REGION_TAG_COUNT;
	if (rdlen != dl)
	    goto errxit;
    }

    h->flags &= ~HEADERFLAG_SORTED;
    headerSort(h);

    if (sw != NULL)	(void) rpmswExit((rpmop)sw, pvlen);

    /*@-globstate -observertrans @*/
    return h;
    /*@=globstate =observertrans @*/

errxit:
    if (sw != NULL)	(void) rpmswExit((rpmop)sw, pvlen);
    /*@-usereleased@*/
    if (h) {
	h->index = _free(h->index);
	yarnPossess(h->_item.use);	/* XXX rpmioPutItem expects locked. */
	h = (Header) rpmioPutPool((rpmioItem)h);
    }
    /*@=usereleased@*/
    /*@-refcounttrans -globstate@*/
    return h;
    /*@=refcounttrans =globstate@*/
}

int headerGetMagic(Header h, unsigned char ** magicp, size_t * nmagicp)
{
    unsigned char * hmagic = header_magic;
    if (magicp)
	*magicp = (h ? h->magic : hmagic);
    if (nmagicp)
	*nmagicp = (h ? sizeof(h->magic) : sizeof(header_magic));
    return 0;
}

int headerSetMagic(Header h, unsigned char * magic, size_t nmagic)
{
    if (nmagic > sizeof(h->magic))
	nmagic = sizeof(h->magic);
    if (h) {
	memset(h->magic, 0, sizeof(h->magic));
	if (nmagic > 0)
	    memmove(h->magic, magic, nmagic);
    }
    return 0;
}

const char * headerGetOrigin(Header h)
{
    return (h != NULL ? h->origin : NULL);
}

int headerSetOrigin(Header h, const char * origin)
{
    if (h != NULL) {
	h->origin = _free(h->origin);
	h->origin = xstrdup(origin);
    }
    return 0;
}

const char * headerGetParent(Header h)
{
    return (h != NULL ? h->parent : NULL);
}

int headerSetParent(Header h, const char * parent)
{
    if (h != NULL) {
	h->parent = _free(h->parent);
	h->parent = xstrdup(parent);
    }
    return 0;
}

const char * headerGetBaseURL(Header h)
{
/*@-retexpose@*/
    return (h != NULL ? h->baseurl : NULL);
/*@=retexpose@*/
}

int headerSetBaseURL(Header h, const char * baseurl)
{
    if (h != NULL) {
	h->baseurl = _free(h->baseurl);
	h->baseurl = xstrdup(baseurl);
    }
    return 0;
}

struct stat * headerGetStatbuf(Header h)
{
/*@-immediatetrans -retexpose@*/
    return (h != NULL ? &h->sb : NULL);
/*@=immediatetrans =retexpose@*/
}

int headerSetStatbuf(Header h, struct stat * st)
{
    if (h != NULL && st != NULL)
	memcpy(&h->sb, st, sizeof(h->sb));
    return 0;
}

const char * headerGetDigest(Header h)
{
/*@-compdef -retexpose -usereleased @*/
    return (h != NULL ? h->digest : NULL);
/*@=compdef =retexpose =usereleased @*/
}

int headerSetDigest(Header h, const char * digest)
{
    if (h != NULL) {
	h->digest = _free(h->digest);
	if (digest)
	    h->digest = xstrdup(digest);
    }
    return 0;
}

void * headerGetRpmdb(Header h)
{
/*@-compdef -retexpose -usereleased @*/
    return (h != NULL ? h->rpmdb : NULL);
/*@=compdef =retexpose =usereleased @*/
}

void * headerSetRpmdb(Header h, void * rpmdb)
{
/*@-assignexpose -temptrans @*/
    if (h != NULL)
	h->rpmdb = rpmdb;
/*@=assignexpose =temptrans @*/
    return NULL;
}

uint32_t headerGetInstance(Header h)
{
    return (h != NULL ? h->instance : 0);
}

uint32_t headerSetInstance(Header h, uint32_t instance)
{
    uint32_t oinstance = 0;
    if (h != NULL) {
	oinstance = h->instance;
	h->instance = instance;
    }
    return oinstance;
}

rpmuint32_t headerGetStartOff(Header h)
{
    return (h != NULL ? h->startoff : 0);
}

rpmuint32_t headerSetStartOff(Header h, rpmuint32_t startoff)
{
    if (h != NULL)
	h->startoff = startoff;
    return 0;
}

rpmuint32_t headerGetEndOff(Header h)
{
    return (h != NULL ? h->endoff : 0);
}

rpmuint32_t headerSetEndOff(Header h, rpmuint32_t endoff)
{
    if (h != NULL)
	h->endoff = endoff;
    return 0;
}

Header headerReload(Header h, rpmTag tag)
{
    Header nh;
    void * uh;
    const char * origin = (h->origin != NULL ? xstrdup(h->origin) : NULL);
    const char * parent = (h->parent != NULL ? xstrdup(h->parent) : NULL);
    const char * baseurl = (h->baseurl != NULL ? xstrdup(h->baseurl) : NULL);
    const char * digest = (h->digest != NULL ? xstrdup(h->digest) : NULL);
    struct stat sb = h->sb;	/* structure assignment */
    void * rpmdb = h->rpmdb;
    rpmuint32_t instance = headerGetInstance(h);
    int xx;

/*@-onlytrans@*/
    uh = headerUnload(h, NULL);
    (void)headerFree(h);
    h = NULL ;
/*@=onlytrans@*/
    if (uh == NULL)
	goto errxit;
    nh = headerLoad(uh);
    if (nh == NULL) {
	uh = _free(uh);
	goto errxit;
    }
    nh->flags &= ~(HEADERFLAG_MAPPED|HEADERFLAG_RDONLY); /* XXX unnecessary */
    nh->flags |= HEADERFLAG_ALLOCATED;
    if (ENTRY_IS_REGION(nh->index)) {
	if (tag == HEADER_SIGNATURES || tag == HEADER_IMMUTABLE)
	    nh->index[0].info.tag = tag;
    }
    if (origin != NULL) {
	xx = headerSetOrigin(nh, origin);
	origin = _free(origin);
    }
    if (parent != NULL) {
	xx = headerSetParent(nh, parent);
	parent = _free(parent);
    }
    if (baseurl != NULL) {
	xx = headerSetBaseURL(nh, baseurl);
	baseurl = _free(baseurl);
    }
    if (digest != NULL) {
	xx = headerSetDigest(nh, digest);
	digest = _free(digest);
    }
/*@-assignexpose@*/
    nh->sb = sb;	/* structure assignment */
/*@=assignexpose@*/
    (void) headerSetRpmdb(nh, rpmdb);
    xx = (int) headerSetInstance(nh, instance);
if (_hdr_debug)
fprintf(stderr, "--> h %p ==== %s: blob %p[%u] flags 0x%x\n", nh, __FUNCTION__, nh->blob, (unsigned)nh->bloblen, nh->flags);
    return nh;

errxit:
    digest = _free(digest);
    baseurl = _free(baseurl);
    parent = _free(parent);
    origin = _free(origin);
    return NULL;
}

static Header headerMap(const void * uh, int map)
	/*@*/
{
    rpmuint32_t * ei = (rpmuint32_t *) uh;
    rpmuint32_t il = (rpmuint32_t) ntohl(ei[0]);	/* index length */
    rpmuint32_t dl = (rpmuint32_t) ntohl(ei[1]);	/* data length */
    /*@-sizeoftype@*/
    size_t pvlen = sizeof(il) + sizeof(dl) +
			(il * sizeof(struct entryInfo_s)) + dl;
    /*@=sizeoftype@*/
    void * nuh = NULL;
    Header nh = NULL;

    /* Sanity checks on header intro. */
    if (hdrchkTags(il) || hdrchkData(dl) || pvlen >= headerMaxbytes)
	return NULL;

    if (map) {
	static const int prot = PROT_READ | PROT_WRITE;
	static const int flags = MAP_PRIVATE| MAP_ANONYMOUS;
	static const int fdno = -1;
	static const off_t off = 0;
	nuh = mmap(NULL, pvlen, prot, flags, fdno, off);
assert(nuh != NULL && nuh != (void *)-1);	/* coverity #1214080 */
	if (nuh == NULL || nuh == (void *)-1)
	    fprintf(stderr,
		"==> mmap(%p[%u], 0x%x, 0x%x, %d, 0x%x) error(%d): %s\n",
		NULL, (unsigned)pvlen, prot, flags, fdno, (unsigned)off,
		errno, strerror(errno));
	memcpy(nuh, uh, pvlen);
	if (mprotect(nuh, pvlen, PROT_READ) != 0)
	    fprintf(stderr, "==> mprotect(%p[%u],0x%x) error(%d): %s\n",
			nuh, (unsigned)pvlen, PROT_READ,
			errno, strerror(errno));
	nh = headerLoad(nuh);
	if (nh != NULL) {
assert(nh->bloblen == pvlen);
	    nh->flags |= HEADERFLAG_MAPPED;
	    nh->flags |= HEADERFLAG_RDONLY;
	} else {
	    if (munmap(nuh, pvlen) != 0)
		fprintf(stderr, "==> munmap(%p[%u]) error(%d): %s\n",
			nuh, (unsigned)pvlen, errno, strerror(errno));
	}
    } else {
	nuh = memcpy(xmalloc(pvlen), uh, pvlen);
	if ((nh = headerLoad(nuh)) != NULL)
	    nh->flags |= HEADERFLAG_ALLOCATED;
	else
	    nuh = _free(nuh);
    }

    return nh;
}

Header headerCopyLoad(const void * uh)
{
    static const int map = 1;
    return headerMap(uh, map);
}

int headerIsEntry(Header h, rpmTag tag)
{
    /*@-mods@*/		/*@ FIX: h modified by sort. */
    return (findEntry(h, tag, (rpmTagType)0) != NULL ? 1 : 0);
    /*@=mods@*/	
}

/** \ingroup header
 * Retrieve data from header entry.
 * @todo Permit retrieval of regions other than HEADER_IMUTABLE.
 * @param entry		header entry
 * @retval *he		tag container
 * @param minMem	string pointers refer to header memory?
 * @return		1 on success, otherwise error.
 */
static int copyEntry(const indexEntry entry, HE_t he, int minMem)
	/*@modifies he @*/
{
    rpmTagCount count = entry->info.count;
    int rc = 1;		/* XXX 1 on success. */

    switch (entry->info.type) {
    case RPM_BIN_TYPE:
	/*
	 * XXX This only works for
	 * XXX 	"sealed" HEADER_IMMUTABLE/HEADER_SIGNATURES/HEADER_IMAGE.
	 * XXX This will *not* work for unsealed legacy HEADER_IMAGE (i.e.
	 * XXX a legacy header freshly read, but not yet unloaded to the rpmdb).
	 */
	if (ENTRY_IS_REGION(entry)) {
	    rpmuint32_t * ei = ((rpmuint32_t *)entry->data) - 2;
	    /*@-castexpose@*/
	    entryInfo pe = (entryInfo) (ei + 2);
	    /*@=castexpose@*/
	    unsigned char * dataStart = (unsigned char *) (pe + ntohl(ei[0]));
	    rpmuint32_t rdl;
	    rpmuint32_t ril;

assert(entry->info.offset <= 0);		/* XXX insurance */
	    rdl = (rpmuint32_t)-entry->info.offset;	/* negative offset */
	    ril = (rpmuint32_t)(rdl/sizeof(*pe));
	    /*@-sizeoftype@*/
	    rdl = (rpmuint32_t)entry->rdlen;
	    count = 2 * sizeof(*ei) + (ril * sizeof(*pe)) + rdl;
	    if (entry->info.tag == HEADER_IMAGE) {
		ril -= 1;
		pe += 1;
	    } else {
		count += REGION_TAG_COUNT;
		rdl += REGION_TAG_COUNT;
	    }

	    he->p.ui32p = ei = (rpmuint32_t *) DRD_xmalloc(count);
	    ei[0] = (rpmuint32_t)htonl(ril);
	    ei[1] = (rpmuint32_t)htonl(rdl);

	    /*@-castexpose@*/
	    pe = (entryInfo) memcpy(ei + 2, pe, (ril * sizeof(*pe)));
	    /*@=castexpose@*/

	    (void) memcpy(pe + ril, dataStart, rdl);
	} else {
	    count = (rpmTagCount)entry->info.count;
	    he->p.ptr = (!minMem
		? memcpy(DRD_xmalloc(count), entry->data, count)
		: entry->data);
	}
	break;
#if !defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
assert(0);
#endif
    case RPM_STRING_TYPE:
	if (count == 1) {
	    he->p.str = (char *) entry->data;
	    break;
	}
	/*@fallthrough@*/
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_STRING_ARRAY_TYPE:
    {	const char ** argv;
	size_t nb = count * sizeof(*argv);
	char * t;
	unsigned i;

	/*@-mods@*/
	if (minMem) {
	    he->p.argv = argv = (const char **) DRD_xmalloc(nb);
	    t = (char *) entry->data;
	} else {
	    he->p.argv = argv = (const char **) DRD_xmalloc(nb + entry->length);
	    t = (char *) &argv[count];
	    memcpy(t, entry->data, entry->length);
	}
	/*@=mods@*/
	for (i = 0; i < (unsigned) count; i++) {
	    argv[i] = t;
	    t = strchr(t, 0);
	    t++;
	}
    }	break;

    default:
	he->p.ptr = entry->data;
	break;
    }
    he->t = entry->info.type;
    he->c = count;
    return rc;
}

#if defined(SUPPORT_I18NSTRING_TYPE)
/**
 * Does locale match entry in header i18n table?
 * 
 * \verbatim
 * The range [l,le) contains the next locale to match:
 *    ll[_CC][.EEEEE][@dddd]
 * where
 *    ll	ISO language code (in lowercase).
 *    CC	(optional) ISO coutnry code (in uppercase).
 *    EEEEE	(optional) encoding (not really standardized).
 *    dddd	(optional) dialect.
 * \endverbatim
 *
 * @param td		header i18n table data, NUL terminated
 * @param l		start of locale	to match
 * @param le		end of locale to match
 * @return		1 on match, 0 on no match
 */
static int headerMatchLocale(const char *td, const char *l, const char *le)
	/*@*/
{
    const char *fe;


#if 0
  { const char *s, *ll, *CC, *EE, *dd;
    char *lbuf, *t.

    /* Copy the buffer and parse out components on the fly. */
    lbuf = (char *) alloca(le - l + 1);
    for (s = l, ll = t = lbuf; *s; s++, t++) {
	switch (*s) {
	case '_':
	    *t = '\0';
	    CC = t + 1;
	    break;
	case '.':
	    *t = '\0';
	    EE = t + 1;
	    break;
	case '@':
	    *t = '\0';
	    dd = t + 1;
	    break;
	default:
	    *t = *s;
	    break;
	}
    }

    if (ll)	/* ISO language should be lower case */
	for (t = ll; *t; t++)	*t = tolower(*t);
    if (CC)	/* ISO country code should be upper case */
	for (t = CC; *t; t++)	*t = toupper(*t);

    /* There are a total of 16 cases to attempt to match. */
  }
#endif

    /* First try a complete match. */
    if (strlen(td) == (size_t)(le - l) && !strncmp(td, l, (size_t)(le - l)))
	return 1;

    /* Next, try stripping optional dialect and matching.  */
    for (fe = l; fe < le && *fe != '@'; fe++)
	{};
    if (fe < le && !strncmp(td, l, (fe - l)))
	return 1;

    /* Next, try stripping optional codeset and matching.  */
    for (fe = l; fe < le && *fe != '.'; fe++)
	{};
    if (fe < le && !strncmp(td, l, (fe - l)))
	return 1;

    /* Finally, try stripping optional country code and matching. */
    for (fe = l; fe < le && *fe != '_'; fe++)
	{};
    if (fe < le && !strncmp(td, l, (fe - l)))
	return 2;

    return 0;
}

/**
 * Return i18n string from header that matches locale.
 * @param h		header
 * @param entry		i18n string data
 * @return		matching i18n string (or 1st string if no match)
 */
/*@dependent@*/ /*@exposed@*/ static char *
headerFindI18NString(Header h, indexEntry entry)
	/*@*/
{
    const char *lang, *l, *le;
    indexEntry table;

    /* XXX Drepper sez' this is the order. */
    if ((lang = getenv("LANGUAGE")) == NULL &&
	(lang = getenv("LC_ALL")) == NULL &&
	(lang = getenv("LC_MESSAGES")) == NULL &&
	(lang = getenv("LANG")) == NULL)
	    return (char *) entry->data;
    
    /*@-mods@*/
    if ((table = findEntry(h, (rpmTag)HEADER_I18NTABLE, RPM_STRING_ARRAY_TYPE)) == NULL)
	return (char *) entry->data;
    /*@=mods@*/

    for (l = lang; *l != '\0'; l = le) {
	const char *td;
	char *ed, *ed_weak = NULL;
	rpmuint32_t langNum;

	while (*l && *l == ':')			/* skip leading colons */
	    l++;
	if (*l == '\0')
	    break;
	for (le = l; *le && *le != ':'; le++)	/* find end of this locale */
	    {};

	/* For each entry in the header ... */
	for (langNum = 0, td = (const char *)table->data, ed = (char *)entry->data;
	     langNum < entry->info.count;
	     langNum++, td += strlen(td) + 1, ed += strlen(ed) + 1)
	{
		int match = headerMatchLocale(td, l, le);
		if (match == 1) return (char *) ed;
		else if (match == 2) ed_weak = ed;
	}
	if (ed_weak) return (char *) ed_weak;
    }

    return (char *) entry->data;
}
#endif

/**
 * Retrieve tag data from header.
 * @param h		header
 * @param he		tag container
 * @param flags		headerGet flags
 * @return		1 on success, 0 on not found
 */
static int intGetEntry(Header h, HE_t he, int flags)
	/*@modifies he @*/
{
    int minMem = 0;
    indexEntry entry;
    int rc;

    /* First find the tag */
/*@-mods@*/		/*@ FIX: h modified by sort. */
    entry = findEntry(h, he->tag, (rpmTagType)0);
/*@=mods@*/
    if (entry == NULL) {
	he->t = (rpmTagType)0;
	he->p.ptr = NULL;
	he->c = 0;
	return 0;
    }

    switch (entry->info.type) {
#if defined(SUPPORT_I18NSTRING_TYPE)
    case RPM_I18NSTRING_TYPE:
	if (!(flags & HEADERGET_NOI18NSTRING)) {
	    rc = 1;
	    he->t = RPM_STRING_TYPE;
	    he->c = 1;
/*@-dependenttrans@*/
	    he->p.str = headerFindI18NString(h, entry);
/*@=dependenttrans@*/
	    break;
	}
	/*@fallthrough@*/
#endif
    default:
	rc = copyEntry(entry, he, minMem);
	break;
    }

    /* XXX 1 on success */
    return (rc == 1 ? 1 : 0);
}

/**
 * Copy (swab'd) data into store.
 * @param t		data store
 * @param he		tag container
 * @param nb		no. bytes in store
 * @return 		0 on success
 */
static int copyData(char * t, const HE_t he, size_t nb)
	/*@modifies *t @*/
{
    int rc = 0;		/* assume success */

    switch (he->t) {
#if defined(SUPPORT_I18NSTRING_TYPE)	/* XXX used while reloading? */
    case RPM_I18NSTRING_TYPE:
#endif
    case RPM_STRING_ARRAY_TYPE:
    {	const char ** av = he->p.argv;
	rpmTagCount cnt = he->c;
	const char * s;

	while (cnt-- > 0 && nb > 0) {
	    if ((s = *av++) != NULL)
	    do {
		*t++ = *s++;
	    } while (s[-1] && --nb > 0);
	}
    }	break;
    default:
	if (tagSwab((unsigned char *)t, he, nb) == NULL)
	    rc = 1;
	break;
    }
    return rc;
}

/**
 * Return (malloc'ed) copy of (swab'd) entry data.
 * @param he		tag container
 * @retval *lenp	no. bytes in returned data
 * @return 		(malloc'ed) copy of entry data, NULL on error
 */
/*@null@*/
static void *
grabData(HE_t he, /*@out@*/ size_t * lenp)
	/*@modifies *lenp @*/
{
    size_t nb = dataLength(he->t, &he->p, he->c, 0, NULL);
    char * t = NULL;

    if (nb > 0) {
	t = (char *) xmalloc(nb);
	if (copyData(t, he, nb)) {
	    t = _free(t);
	    nb = 0;
	}
    }
    if (lenp)
	*lenp = nb;
    return t;
}

/** \ingroup header
 * Add tag to header.
 * Duplicate tags are okay, but only defined for iteration (with the
 * exceptions noted below). While you are allowed to add i18n string
 * arrays through this function, you probably don't mean to. See
 * headerAddI18NString() instead.
 *
 * @param h		header
 * @param he		tag container
 * @return		1 on success, 0 on failure
 */
static
int headerAddEntry(Header h, HE_t he)
	/*@modifies h @*/
{
    indexEntry entry;
    rpmTagData data;
    size_t length = 0;
    int rc = 0;		/* assume failure */

    /* Count must always be >= 1 for headerAddEntry. */
    if (he->c == 0)
	return rc;

    if (hdrchkType(he->t))
	return rc;
    if (hdrchkData(he->c))
	return rc;

    data.ptr = grabData(he, &length);
    if (data.ptr == NULL || length == 0)
	return rc;

    /* Allocate more index space if necessary */
    if (h->indexUsed == h->indexAlloced) {
	h->indexAlloced += INDEX_MALLOC_SIZE;
	h->index = (indexEntry) xrealloc(h->index, h->indexAlloced * sizeof(*h->index));
    }

    /* Fill in the index */
    entry = h->index + h->indexUsed;
    entry->info.tag = he->tag;
    entry->info.type = he->t;
    entry->info.count = he->c;
    entry->info.offset = 0;
    entry->data = data.ptr;
    entry->length = length;

    if (h->indexUsed > 0 && he->tag < h->index[h->indexUsed-1].info.tag)
	h->flags &= ~HEADERFLAG_SORTED;
    h->indexUsed++;
    rc = 1;

    return rc;
}

/** \ingroup header
 * Append element to tag array in header.
 * Appends item p to entry w/ tag and type as passed. Won't work on
 * RPM_STRING_TYPE.
 *
 * @param h		header
 * @param he		tag container
 * @return		1 on success, 0 on failure
 */
static
int headerAppendEntry(Header h, HE_t he)
	/*@modifies h @*/
{
    rpmTagData src = { he->p.ptr };
    char * t;
    indexEntry entry;
    size_t length;
    int rc = 0;		/* assume failure */

#if !defined(SUPPORT_I18NSTRING_TYPE)
assert(he->t != RPM_I18NSTRING_TYPE);
#endif
    if (he->t == RPM_STRING_TYPE
#if defined(SUPPORT_I18NSTRING_TYPE)
     || he->t == RPM_I18NSTRING_TYPE
#endif
    ) {
	/* we can't do this */
	return rc;
    }

    /* Find the tag entry in the header. */
    entry = findEntry(h, he->tag, he->t);
    if (!entry)
	return rc;

    length = dataLength(he->t, &src, he->c, 0, NULL);
    if (length == 0)
	return rc;

    if (ENTRY_IN_REGION(entry)) {
	char * t = (char *) xmalloc(entry->length + length);
	memcpy(t, entry->data, entry->length);
	entry->data = t;
	entry->info.offset = 0;
    } else
	entry->data = xrealloc(entry->data, entry->length + length);

    t = ((char *) entry->data) + entry->length;
    if (!copyData(t, he, length))
	rc = 1;

    entry->length += length;

    entry->info.count += he->c;

    return rc;
}

/** \ingroup header
 * Add or append element to tag array in header.
 * @param h		header
 * @param he		tag container
 * @return		1 on success, 0 on failure
 */
static
int headerAddOrAppendEntry(Header h, HE_t he)
	/*@modifies h @*/
{
    return (findEntry(h, he->tag, he->t)
	? headerAppendEntry(h, he)
	: headerAddEntry(h, he));
}

#if defined(SUPPORT_I18NSTRING_TYPE)
int headerAddI18NString(Header h, rpmTag tag, const char * string,
		const char * lang)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    indexEntry table, entry;
    rpmTagData p;
    size_t length;
    size_t ghosts;
    rpmuint32_t i;
    rpmuint32_t langNum;
    char * buf;
    int xx;

    table = findEntry(h, (rpmTag)HEADER_I18NTABLE, RPM_STRING_ARRAY_TYPE);
    entry = findEntry(h, tag, RPM_I18NSTRING_TYPE);

    if (!table && entry)
	return 0;		/* this shouldn't ever happen!! */

    if (!table && !entry) {
	const char * argv[2];
	int count = 0;
	p.argv = argv;
	if (!lang || (lang[0] == 'C' && lang[1] == '\0')) {
	    /*@-observertrans -readonlytrans@*/
	    p.argv[count++] = "C";
	    /*@=observertrans =readonlytrans@*/
	} else {
	    /*@-observertrans -readonlytrans@*/
	    p.argv[count++] = "C";
	    /*@=observertrans =readonlytrans@*/
	    p.argv[count++] = lang;
	}
	he->tag = (rpmTag) HEADER_I18NTABLE;
	he->t = RPM_STRING_ARRAY_TYPE;
	he->p.ptr = p.ptr;
	he->c = count;
	xx = headerAddEntry(h, he);
	if (!xx)
	    return 0;
	table = findEntry(h, he->tag, he->t);
    }

    if (!table)
	return 0;
    if (!lang) lang = "C";

    {	const char * l = (const char *) table->data;
	for (langNum = 0; langNum < table->info.count; langNum++) {
	    if (!strcmp(l, lang)) break;
	    l += strlen(l) + 1;
	}
    }

    if (langNum >= table->info.count) {
	length = strlen(lang) + 1;
	if (ENTRY_IN_REGION(table)) {
	    char * t = (char *) xmalloc(table->length + length);
	    memcpy(t, table->data, table->length);
	    table->data = t;
	    table->info.offset = 0;
	} else
	    table->data = xrealloc(table->data, table->length + length);
	memmove(((char *)table->data) + table->length, lang, length);
	table->length += length;
	table->info.count++;
    }

    if (!entry) {
	p.argv = (const char **) alloca(sizeof(*p.argv) * (langNum + 1));
/*@-observertrans -readonlytrans@*/
	for (i = 0; i < langNum; i++)
	    p.argv[i] = "";
/*@=observertrans =readonlytrans@*/
	p.argv[langNum] = string;
	he->tag = tag;
	he->t = RPM_I18NSTRING_TYPE;
	he->p.ptr = p.ptr;
	he->c = langNum + 1;
/*@-compmempass@*/
	xx = headerAddEntry(h, he);
/*@=compmempass@*/
	return xx;
    } else if (langNum >= entry->info.count) {
	ghosts = langNum - entry->info.count;
	
	length = strlen(string) + 1 + ghosts;
	if (ENTRY_IN_REGION(entry)) {
	    char * t = (char *) xmalloc(entry->length + length);
	    memcpy(t, entry->data, entry->length);
	    entry->data = t;
	    entry->info.offset = 0;
	} else
	    entry->data = xrealloc(entry->data, entry->length + length);

	memset(((char *)entry->data) + entry->length, 0, ghosts);
	memmove(((char *)entry->data) + entry->length + ghosts, string, strlen(string)+1);

	entry->length += length;
	entry->info.count = langNum + 1;
    } else {
	char *b, *be, *e, *ee, *t;
	size_t bn, sn, en;

	/* Set beginning/end pointers to previous data */
	b = be = e = ee = (char *) entry->data;
	for (i = 0; i < table->info.count; i++) {
	    if (i == langNum)
		be = ee;
	    ee += strlen(ee) + 1;
	    if (i == langNum)
		e  = ee;
	}

	/* Get storage for new buffer */
	bn = (be-b);
	sn = strlen(string) + 1;
	en = (ee-e);
	length = bn + sn + en;
	t = buf = (char *) xmalloc(length);

	/* Copy values into new storage */
	memcpy(t, b, bn);
	t += bn;
/*@-mayaliasunique@*/
	memcpy(t, string, sn);
	t += sn;
	memcpy(t, e, en);
	t += en;
/*@=mayaliasunique@*/

	/* Replace i18N string array */
	entry->length -= strlen(be) + 1;
	entry->length += sn;
	
	if (ENTRY_IN_REGION(entry)) {
	    entry->info.offset = 0;
	} else
	    entry->data = _free(entry->data);
	/*@-dependenttrans@*/
	entry->data = buf;
	/*@=dependenttrans@*/
    }

    return 0;
}
#endif

/** \ingroup header
 * Modify tag in header.
 * If there are multiple entries with this tag, the first one gets replaced.
 * @param h		header
 * @param he		tag container
 * @return		1 on success, 0 on failure
 */
static
int headerModifyEntry(Header h, HE_t he)
	/*@modifies h @*/
{
    indexEntry entry;
    rpmTagData oldData;
    rpmTagData newData;
    size_t length = 0;

    /* First find the tag */
    entry = findEntry(h, he->tag, he->t);
    if (!entry)
	return 0;

    newData.ptr = grabData(he, &length);
    if (newData.ptr == NULL || length == 0)
	return 0;

    /* make sure entry points to the first occurrence of this tag */
    while (entry > h->index && (entry - 1)->info.tag == he->tag)  
	entry--;

    /* free after we've grabbed the new data in case the two are intertwined;
       that's a bad idea but at least we won't break */
    oldData.ptr = entry->data;

    entry->info.count = he->c;
    entry->info.type = he->t;
    entry->data = newData.ptr;
    entry->length = length;

    if (ENTRY_IN_REGION(entry)) {
	entry->info.offset = 0;
    } else
	oldData.ptr = _free(oldData.ptr);

    return 1;
}

/**
 * Header tag iterator data structure.
 */
struct headerIterator_s {
    Header h;		/*!< Header being iterated. */
    size_t next_index;	/*!< Next tag index. */
};

HeaderIterator headerFini(/*@only@*/ HeaderIterator hi)
{
    if (hi != NULL) {
	(void)headerFree(hi->h);
        hi->h = NULL;
	hi = _free(hi);
    }
    return hi;
}

HeaderIterator headerInit(Header h)
{
    HeaderIterator hi = (HeaderIterator) xmalloc(sizeof(*hi));

    headerSort(h);

/*@-assignexpose -castexpose @*/
    hi->h = headerLink(h);
/*@=assignexpose =castexpose @*/
assert(hi->h != NULL);
    hi->next_index = 0;
    return hi;
}

int headerNext(HeaderIterator hi, HE_t he, /*@unused@*/ unsigned int flags)
{
    void * sw;
    Header h = hi->h;
    size_t slot = hi->next_index;
    indexEntry entry = NULL;
    int rc;

    /* Ensure that *he is reliably initialized. */
    memset(he, 0, sizeof(*he));

    for (slot = hi->next_index; slot < h->indexUsed; slot++) {
	entry = h->index + slot;
	if (!ENTRY_IS_REGION(entry))
	    break;
    }
    hi->next_index = slot;
    if (entry == NULL || slot >= h->indexUsed)
	return 0;

    hi->next_index++;

    if ((sw = headerGetStats(h, 19)) != NULL)	/* RPMTS_OP_HDRGET */
	(void) rpmswEnter((rpmop)sw, 0);

    he->tag = entry->info.tag;
    rc = copyEntry(entry, he, 0);
    if (rc)
	rc = rpmheRealloc(he);

    if (sw != NULL)	(void) rpmswExit((rpmop)sw, 0);

    /* XXX 1 on success */
    return ((rc == 1) ? 1 : 0);
}

Header headerCopy(Header h)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    Header nh = headerNew();
    HeaderIterator hi;
   
    for (hi = headerInit(h);
	headerNext(hi, he, 0);
	he->p.ptr = _free(he->p.ptr))
    {
	if (he->p.ptr) (void) headerAddEntry(nh, he);
    }
    hi = headerFini(hi);

    return headerReload(nh, (rpmTag)HEADER_IMAGE);
}

void headerCopyTags(Header headerFrom, Header headerTo, rpmTag * tagstocopy)
{
    HE_t he = (HE_t) memset(alloca(sizeof(*he)), 0, sizeof(*he));
    rpmTag * tagno;
    int xx;

    if (headerFrom == headerTo)
	return;

    for (tagno = tagstocopy; *tagno != 0; tagno++) {
	if (headerIsEntry(headerTo, *tagno))
	    continue;
	he->tag = *tagno;
	if (!headerGet(headerFrom, he, 0))
	    continue;
	xx = headerPut(headerTo, he, 0);
	he->p.ptr = _free(he->p.ptr);
    }
}

int headerGet(Header h, HE_t he, unsigned int flags)
{
    void * sw;
    const char * name;
    headerSprintfExtension exts = headerCompoundFormats;
    headerSprintfExtension ext = NULL;
    int extNum;
    int rc;

    if (h == NULL || he == NULL)	return 0;	/* XXX this is nutty. */

    /* Ensure that *he is reliably initialized. */
    {	rpmTag tag = he->tag;
	memset(he, 0, sizeof(*he));
	he->tag = tag;
    }
    name = tagName(he->tag);

    if ((sw = headerGetStats(h, 19)) != NULL)	/* RPMTS_OP_HDRGET */
	(void) rpmswEnter((rpmop)sw, 0);

    /* Search extensions for specific tag override. */
    if (!(flags & HEADERGET_NOEXTENSION))
    for (ext = exts, extNum = 0; ext != NULL && ext->type != HEADER_EXT_LAST;
	ext = (ext->type == HEADER_EXT_MORE ? *ext->u.more : ext+1), extNum++)
    {
	if (ext->name == NULL || ext->type != HEADER_EXT_TAG)
	    continue;
	if (!xstrcasecmp(ext->name + (sizeof("RPMTAG_")-1), name))
	    break;
    }

    if (ext && ext->name != NULL && ext->type == HEADER_EXT_TAG) {
	rc = ext->u.tagFunction(h, he);
	rc = (rc == 0);		/* XXX invert extension return. */
    } else
	rc = intGetEntry(h, he, flags);

    if (rc)
	rc = rpmheRealloc(he);

    if (sw != NULL)	(void) rpmswExit((rpmop)sw, 0);

#if defined(SUPPORT_IMPLICIT_TAG_DATA_TYPES)
/*@-modfilesys@*/
    /* XXX verify that explicit and implicit types are identical. */
    if (rc)
	tagTypeValidate(he);
/*@=modfilesys@*/
#endif

/*@-modfilesys@*/
    if (!((rc == 0 && he->freeData == 0 && he->p.ptr == NULL) ||
	  (rc == 1 && he->freeData == 1 && he->p.ptr != NULL)))
    {
if (_hdr_debug)
fprintf(stderr, "==> %s(%u) %u %p[%u] free %u rc %d\n", name, (unsigned) he->tag, (unsigned) he->t, he->p.ptr, (unsigned) he->c, he->freeData, rc);
    }
/*@=modfilesys@*/

    return rc;
}

int headerPut(Header h, HE_t he, /*@unused@*/ unsigned int flags)
{
    int rc;

#if defined(SUPPORT_IMPLICIT_TAG_DATA_TYPES)
/*@-modfilesys@*/
    /* XXX verify that explicit and implicit types are identical. */
    tagTypeValidate(he);
/*@=modfilesys@*/
#endif

    if (he->append)
	rc = headerAddOrAppendEntry(h, he);
    else
	rc = headerAddEntry(h, he);

    return rc;
}

int headerDel(Header h, HE_t he, /*@unused@*/ unsigned int flags)
	/*@modifies h @*/
{
    return headerRemoveEntry(h, he->tag);
}

int headerMod(Header h, HE_t he, /*@unused@*/ unsigned int flags)
	/*@modifies h @*/
{

#if defined(SUPPORT_IMPLICIT_TAG_DATA_TYPES)
/*@-modfilesys@*/
    /* XXX verify that explicit and implicit types are identical. */
    tagTypeValidate(he);
/*@=modfilesys@*/
#endif

    return headerModifyEntry(h, he);
}
