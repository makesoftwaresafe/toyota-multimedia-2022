/*
 * Copyright © 2009 Per Øyvind Karlsen <peroyvind@mandriva.org>
 *
 * $Id: rpm46compat.h,v 2.2 2010/10/03 06:12:32 pkarlsen Exp $
 */

#ifndef	H_RPM46COMPAT
#define	H_RPM46COMPAT		1

#include <rpm/rpm4compat.h>
#include <rpmbuild.h>

typedef rpmuint64_t rpm_loff_t;

typedef enum rpmtdFlags_e {
    RPMTD_NONE		= 0,
    RPMTD_ALLOCED	= (1 << 0),	/* was memory allocated? */
    RPMTD_PTR_ALLOCED	= (1 << 1),	/* were array pointers allocated? */
    RPMTD_IMMUTABLE	= (1 << 2),	/* header data or modifiable? */
} rpmtdFlags;

typedef struct rpmtd_s * rpmtd;

typedef rpmTagCount	rpm_count_t;

typedef void *		rpm_data_t;

/*
 * Notice that the layout of this struct has been modified to
 * fit HE_s and is different from the "original" rpmtd_s.
 * This way you can safely cast from 'rpmtd' to 'HE_t' while providing
 * the same member and typedef names for rpm >= 4.6 compatibility.
 */
struct rpmtd_s {
    rpmTag tag;		/* rpm tag of this data entry*/
    rpmTagType type;	/* data type */
    rpm_data_t data;	/* pointer to actual data */
    rpm_count_t count;	/* number of entries */
    int ix;		/* iteration index */
    unsigned int freeData	: 1;
    unsigned int avail		: 1;
    unsigned int append		: 1;
    rpmtdFlags flags;	/* flags on memory allocation etc */
};

typedef enum headerGetFlags_e {
    HEADERGET_DEFAULT	= 0,	    /* legacy headerGetEntry() behavior */
    HEADERGET_MINMEM 	= (1 << 0), /* pointers can refer to header memory */
    HEADERGET_EXT 	= (1 << 1), /* lookup extension types too */
    HEADERGET_RAW 	= (1 << 2), /* return raw contents (no i18n lookups) */
    HEADERGET_ALLOC	= (1 << 3), /* always allocate memory for all data */
    HEADERGET_ARGV	= (1 << 4), /* return string arrays NULL-terminated */
} headerGetFlags;

typedef enum headerPutFlags_e {
    HEADERPUT_DEFAULT	= 0,
    HEADERPUT_APPEND 	= (1 << 0),
} headerPutFlags;

#ifdef __cplusplus
extern "C" {
#endif

static inline rpmTagCount rpmtdCount(rpmtd td)
{
    assert(td != NULL);
    /* fix up for binary type abusing count as data length */
    return (td->type == RPM_BIN_TYPE) ? 1 : td->count;
}

static inline rpmtd rpmtdReset(rpmtd td)
{
    memset(td, 0, sizeof(*td));
    td->ix = -1;
    return td;
}

static inline void rpmtdFreeData(rpmtd td)
{
    HE_t he = (HE_t)td;

    if (he && he->freeData)
	he->p.ptr = _free(he->p.ptr);
    td = rpmtdReset(td);
}

static inline rpmuint8_t * rpmtdGetUint8(rpmtd td)
{
    HE_t he = (HE_t)td;
    rpmuint8_t *res = NULL;

    assert(he != NULL);
    if (he->t == RPM_UINT8_TYPE)
	res = &(he->p.ui8p[td->ix >= 0 ? td->ix : 0]);

    return res;
}

static inline rpmuint16_t * rpmtdGetUint16(rpmtd td)
{
    HE_t he = (HE_t)td;
    rpmuint16_t *res = NULL;

    assert(he != NULL);
    if (he->t == RPM_UINT16_TYPE)
	res = &(he->p.ui16p[td->ix >= 0 ? td->ix : 0]);

    return res;
}

static inline rpmuint32_t * rpmtdGetUint32(rpmtd td)
{
    uint32_t *res = NULL;

    assert(td != NULL);

    if (td->type == RPM_INT32_TYPE) {
	int ix = (td->ix >= 0 ? td->ix : 0);
	res = (uint32_t *) td->data + ix;
    } 
    return res;
    
}

static inline rpmuint64_t * rpmtdGetUint64(rpmtd td)
{
    HE_t he = (HE_t)td;
    rpmuint64_t *res = NULL;

    assert(he != NULL);
    if (he->t == RPM_UINT64_TYPE)
	res = &(he->p.ui64p[td->ix >= 0 ? td->ix : 0]);

    return res;
}

static inline const char * rpmtdGetString(rpmtd td)
{
    HE_t he = (HE_t)td;
    const char *str = NULL;

    assert(he != NULL);
    if (he->t == RPM_STRING_TYPE)
       str = (const char *) he->p.str;
    else if(he->t == RPM_STRING_ARRAY_TYPE ||
	    he->t == RPM_I18NSTRING_TYPE)
	str = he->p.argv[td->ix >= 0 ? td->ix : 0];
    return str;
}

static inline int rpmtdNext(rpmtd td)
{
    int i = -1;

    assert(td != NULL);

    if (++td->ix >= 0) {
	if (td->ix < (int)rpmtdCount(td)) {
	    i = td->ix;
	} else {
	    td->ix = i;
	}
    }
    return i;
}

static inline rpmuint32_t *rpmtdNextUint32(rpmtd td)
{
    rpmuint32_t *res = NULL;
    assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetUint32(td);
    }
    return res;
}

static inline rpmuint64_t *rpmtdNextUint64(rpmtd td)
{
    rpmuint64_t *res = NULL;
    assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetUint64(td);
    }
    return res;
}

static inline const char *rpmtdNextString(rpmtd td)
{
    const char *res = NULL;
    assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetString(td);
    }
    return res;
}

static inline rpmTagType rpmtdType(rpmtd td)
{
    assert(td != NULL);
    return td->type;
}

static inline char * headerFormat(Header h, const char * fmt, errmsg_t * errmsg) {
    return headerSprintf(h, fmt, NULL, NULL, errmsg);
}

static inline int headerGet_RPMorg(Header h, rpmTag tag, rpmtd td, headerGetFlags flags)
{
    int rc;
    unsigned int rflags = 0;
    HE_t he = (HE_t)rpmtdReset(td);

    /* XXX: Ehhr..? */
    he->tag = (rpmTag)tag;
    if(flags & HEADERGET_EXT)
	rflags &= ~HEADERGET_NOEXTENSION;
    else
	rflags |= HEADERGET_NOEXTENSION;
    if(flags & HEADERGET_RAW)
	rflags |= HEADERGET_NOI18NSTRING;
    else
	rflags &= ~HEADERGET_NOI18NSTRING;

    rc = headerGet(h, (HE_t)he, rflags);

    return rc;
}

static inline int headerPut_RPMorg(Header h, rpmtd td, headerPutFlags flags)
{
    HE_t he = (HE_t)td;
    if (flags & HEADERPUT_APPEND)
	he->append = 1;
    return headerPut(h, he, 0);
}

static inline int headerPutString(Header h, rpmTag tag, const char *val)
{
    HE_t he = (HE_t)memset(alloca(sizeof(*he)), 0, sizeof(*he));

    he->tag = tag;
    he->p.str = val;
    he->c = 1;
    return headerPut(h, he, 0);
}

static inline int headerMod_RPMorg(Header h, rpmtd td)
{
    HE_t he = (HE_t)td;
    return headerMod(h, he, 0);
}

static inline int headerNext_RPMorg(HeaderIterator hi, rpmtd td)
{
    HE_t he = (HE_t)rpmtdReset(td);
    return headerNext(hi, he, 0);
}

static inline rpmProblem rpmpsGetProblem_RPMorg(rpmpsi psi)
{
    return rpmpsGetProblem(psi->ps, psi->ix);
}

typedef Spec rpmSpec;

static inline void initSourceHeader_RPMorg(rpmSpec spec)
{
    initSourceHeader(spec, NULL);
}

static inline int parseSpec_RPMorg(rpmts ts, const char * specFile,
		const char * rootDir,
		__attribute__((unused)) const char * buildRoot,
		int recursing,
		const char * passPhrase,
		const char * cookie,
		int anyarch, int force)
{
    return parseSpec(ts, specFile, rootDir, recursing, passPhrase, cookie,
	    anyarch, force, 0);
}

#ifdef __cplusplus
}
#endif

/*
 * This hack will make the conflictiong functions work with the same number
 * of arguments and behavior as in the rpm >= 4.6 API, providing API
 * compatibility by redefining the functions to use the *_RPMorg wrapper
 * versions.
 * I have no idea if this might break with other compilers, and it will also
 * for sure break the "real" versions of these functions provided with this
 * rpm version, ie. you cannot mix functions from the different APIs.
 * To prevent this you can define RPM46COMPAT_NO_CONFLICT_HACK.
 */
#ifndef RPM46COMPAT_NO_CONFLICT_HACK
#define headerDel(h, tag) headerRemoveEntry(h, tag)
#define headerGet(h, tag, he, flags) headerGet_RPMorg(h, tag, he, flags)
#define headerMod(h, td) headerMod_RPMorg(h, td)
#define headerNext(hi, td) headerNext_RPMorg(hi, td)
#define headerPut(h, td, flags) headerPut_RPMorg(h, td, flags)
#define initSourceHeader(spec) initSourceHeader_RPMorg(spec)
#define parseSpec(ts, specFile, rootDir, buildRoot, recursing, passPhrase, cookie, anyarch, force) \
    parseSpec_RPMorg(ts, specFile, rootDir, buildRoot, recursing, passPhrase, cookie, anyarch, force)
#define rpmpsGetProblem(psi) rpmpsGetProblem_RPMorg(psi)
#endif

#endif /* H_RPM46COMPAT */
