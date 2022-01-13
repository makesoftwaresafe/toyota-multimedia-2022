#include "system.h"

#include <argv.h>

#define	_RPMTD_INTERNAL
#define _RPMTAG_INTERNAL	/* XXX rpmHeaderFormatFuncByValue() */
#include <rpmtd.h>
#include <rpmpgp.h>

#include "debug.h"

/*@access rpmtd @*/
/*@access headerSprintfExtension @*/

#ifdef __cplusplus
GENfree(rpmtd)
#endif	/* __cplusplus */

#define RPM_NULL_TYPE		0
#define RPM_INT8_TYPE		RPM_UINT8_TYPE
#define RPM_INT16_TYPE		RPM_UINT16_TYPE
#define RPM_INT32_TYPE		RPM_UINT32_TYPE
#define RPM_INT64_TYPE		RPM_UINT64_TYPE

#define	rpmTagGetType(_tag)	tagType(_tag)

#define	rpmHeaderFormats	headerCompoundFormats

/*@observer@*/ /*@null@*/
static void * rpmHeaderFormatFuncByName(const char *fmt)
	/*@*/
{
    headerSprintfExtension ext;
    void * func = NULL;

    for (ext = rpmHeaderFormats; ext->name != NULL; ext++) {
	if (ext->name == NULL || ext->type != HEADER_EXT_FORMAT)
	    continue;
        if (!strcmp(ext->name, fmt)) {
/*@-castfcnptr @*/
            func = (void *) ext->u.fmtFunction;
/*@=castfcnptr @*/
            break;
        }
    }
    return func;
}

/*@unchecked@*/
static struct rpmtdkey_s {
/*@observer@*/
    const char * str;
    rpmtdFormats fmt;
} keyFormats[] = {
    { "armor",		RPMTD_FORMAT_ARMOR },
#ifdef	NOTYET
    { "arraysize",	RPMTD_FORMAT_ARRAYSIZE },
#endif
    { "base64",		RPMTD_FORMAT_BASE64 },
    { "date",		RPMTD_FORMAT_DATE },
    { "day",		RPMTD_FORMAT_DAY },
    { "depflags",	RPMTD_FORMAT_DEPFLAGS },
    { "fflags",		RPMTD_FORMAT_FFLAGS },
    { "hex",		RPMTD_FORMAT_HEX },
    { "octal",		RPMTD_FORMAT_OCTAL },
    { "permissions",	RPMTD_FORMAT_PERMS },
    { "perms",		RPMTD_FORMAT_PERMS },
    { "pgpsig",		RPMTD_FORMAT_PGPSIG },
    { "shescape",	RPMTD_FORMAT_SHESCAPE },
#ifdef	NOTYET
    { "string",		RPMTD_FORMAT_STRING },
#endif
    { "triggertype",	RPMTD_FORMAT_TRIGGERTYPE },
    { "xml",		RPMTD_FORMAT_XML },
};
/*@unchecked@*/
static size_t nKeyFormats = sizeof(keyFormats) / sizeof(keyFormats[0]);

/*@observer@*/ /*@null@*/
static const char * fmt2name(rpmtdFormats fmt)
	/*@*/
{
    const char * str = NULL;
    int i;

    for (i = 0; i < (int)nKeyFormats; i++) {
	if (fmt != keyFormats[i].fmt)
	    continue;
	str = keyFormats[i].str;
	break;
    }
    return str;
}

/*@observer@*/ /*@null@*/
static void * rpmHeaderFormatFuncByValue(rpmtdFormats fmt)
	/*@*/
{
    const char * name = fmt2name(fmt);
    return (name != NULL ? rpmHeaderFormatFuncByName(name) : NULL);
}

rpmtd rpmtdNew(void)
{
    rpmtd td = (rpmtd) xcalloc(1, sizeof(*td));
    return rpmtdReset(td);
}

rpmtd rpmtdFree(rpmtd td)
{
    /* XXX should we free data too - a flag maybe? */
    td = _free(td);
    return NULL;
}

rpmtd rpmtdReset(rpmtd td)
{
assert(td != NULL);

    memset(td, 0, sizeof(*td));
    td->ix = -1;
    return td;
}

void rpmtdFreeData(rpmtd td)
{
    int i;
assert(td != NULL);

    if (td->flags & RPMTD_ALLOCED) {
	if (td->flags & RPMTD_PTR_ALLOCED) {
	    char ** data = (char **) td->data;
	    assert(td->data != NULL);
	    for (i = 0; i < (int)td->count; i++)
		data[i] = _free(data[i]);
	}
	td->data = _free(td->data);
    }
    td = rpmtdReset(td);
}

rpm_count_t rpmtdCount(rpmtd td)
{
assert(td != NULL);
    /* fix up for binary type abusing count as data length */
    return (td->type == RPM_BIN_TYPE) ? 1 : td->count;
}

rpmTag rpmtdTag(rpmtd td)
{
assert(td != NULL);
    return td->tag;
}

rpmTagType rpmtdType(rpmtd td)
{
assert(td != NULL);
    return td->type;
}

int rpmtdGetIndex(rpmtd td)
{
assert(td != NULL);
    return td->ix;
}

int rpmtdSetIndex(rpmtd td, int index)
{
assert(td != NULL);

    if (index < 0 || index >= (int)rpmtdCount(td)) {
	return -1;
    }
    td->ix = index;
    return td->ix;
}

int rpmtdInit(rpmtd td)
{
assert(td != NULL);

    /* XXX check that this is an array type? */
    td->ix = -1;
    return 0;
}

int rpmtdNext(rpmtd td)
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

rpmuint32_t *rpmtdNextUint32(rpmtd td)
{
    rpmuint32_t *res = NULL;
assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetUint32(td);
    }
    return res;
}

rpmuint64_t *rpmtdNextUint64(rpmtd td)
{
    rpmuint64_t *res = NULL;
assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetUint64(td);
    }
    return res;
}

const char *rpmtdNextString(rpmtd td)
{
    const char *res = NULL;
assert(td != NULL);
    if (rpmtdNext(td) >= 0) {
	res = rpmtdGetString(td);
    }
    return res;
}

rpmuint8_t * rpmtdGetUint8(rpmtd td)
{
    rpmuint8_t *res = NULL;

assert(td != NULL);
    if (td->type == RPM_INT8_TYPE) {
	int ix = (td->ix >= 0 ? td->ix : 0);
	res = (rpmuint8_t *) td->data + ix;
    } 
    return res;
}

rpmuint16_t * rpmtdGetUint16(rpmtd td)
{
    rpmuint16_t *res = NULL;

assert(td != NULL);
    if (td->type == RPM_INT16_TYPE) {
	int ix = (td->ix >= 0 ? td->ix : 0);
	res = (rpmuint16_t *) td->data + ix;
    } 
    return res;
}

rpmuint32_t * rpmtdGetUint32(rpmtd td)
{
    rpmuint32_t *res = NULL;

assert(td != NULL);
    if (td->type == RPM_INT32_TYPE) {
	int ix = (td->ix >= 0 ? td->ix : 0);
	res = (rpmuint32_t *) td->data + ix;
    } 
    return res;
}

rpmuint64_t * rpmtdGetUint64(rpmtd td)
{
    rpmuint64_t *res = NULL;

assert(td != NULL);
    if (td->type == RPM_INT64_TYPE) {
	int ix = (td->ix >= 0 ? td->ix : 0);
	res = (rpmuint64_t *) td->data + ix;
    } 
    return res;
}

const char * rpmtdGetString(rpmtd td)
{
    const char *str = NULL;

assert(td != NULL);
    if (td->type == RPM_STRING_TYPE) {
	str = (const char *) td->data;
    } else if (td->type == RPM_STRING_ARRAY_TYPE ||
	       td->type == RPM_I18NSTRING_TYPE) {
	/* XXX TODO: check for array bounds */
	int ix = (td->ix >= 0 ? td->ix : 0);
	str = *((const char**) td->data + ix);
    } 
    return str;
}

char * rpmtdFormat(/*@unused@*/ rpmtd td, rpmtdFormats fmt, const char * errmsg)
{
/*@-castfcnptr@*/
    headerTagFormatFunction func = (headerTagFormatFunction) rpmHeaderFormatFuncByValue(fmt);
/*@=castfcnptr@*/
    const char *err = NULL;
    char *str = NULL;

    if (func) {
#ifdef	NOTYET	/* XXX different prototypes, wait for rpmhe coercion. */
	char fmtbuf[50]; /* yuck, get rid of this */
	strcpy(fmtbuf, "%");
	str = func(td, fmtbuf);
#else
	err = _("Unknown format");
#endif
    } else {
	err = _("Unknown format");
    }
    
    if (err && errmsg) {
	errmsg = err;
    }

    return str;
}

int rpmtdSetTag(rpmtd td, rpmTag tag)
{
    rpmTagType newtype = (rpmTagType) rpmTagGetType(tag);
    int rc = 0;

assert(td != NULL);

    /* 
     * Sanity checks: 
     * - is the new tag valid at all
     * - if changing tag of non-empty container, require matching type 
     */
    if (newtype == RPM_NULL_TYPE)
	goto exit;

    if (td->data || td->count > 0) {
	if (rpmTagGetType(td->tag) != rpmTagGetType(tag)) {
	    goto exit;
	}
    } 

    td->tag = tag;
    td->type = (rpmTagType) (newtype & RPM_MASK_TYPE);
    rc = 1;
    
exit:
    return rc;
}

static inline int rpmtdSet(rpmtd td, rpmTag tag, rpmTagType type, 
			    rpm_constdata_t data, rpm_count_t count)
	/*@modifies td @*/
{
    td = rpmtdReset(td);
    td->tag = tag;
    td->type = type;
    td->count = count;
    /* 
     * Discards const, but we won't touch the data (even rpmtdFreeData()
     * wont free it as allocation flags aren't set) so it's "ok". 
     * XXX: Should there be a separate RPMTD_FOO flag for "user data"?
     */
/*@-assignexpose -temptrans @*/
    td->data = (void *) data;
/*@=assignexpose =temptrans @*/
    return 1;
}

#define	_TagType(_tag)	\
	((rpmTagType)(rpmTagGetType(tag) & RPM_MASK_TYPE))
#define	_TagReturn(_tag)	\
	((rpmTagReturnType)(rpmTagGetType(tag) & RPM_MASK_RETURN_TYPE))

int rpmtdFromUint8(rpmtd td, rpmTag tag, rpmuint8_t *data, rpm_count_t count)
{
    rpmTagType type = _TagType(tag);
    rpmTagReturnType retype = _TagReturn(tag);
    
    if (count < 1)
	return 0;

    /*
     * BIN type is really just an rpmuint8_t array internally, it's just
     * treated specially otherwise.
     */
    switch (type) {
    case RPM_INT8_TYPE:
	if (retype != RPM_ARRAY_RETURN_TYPE && count > 1) 
	    return 0;
	/*@fallthrough@*/
    case RPM_BIN_TYPE:
	break;
    default:
	return 0;
    }
    
    return rpmtdSet(td, tag, type, data, count);
}

int rpmtdFromUint16(rpmtd td, rpmTag tag, rpmuint16_t *data, rpm_count_t count)
{
    rpmTagType type = _TagType(tag);
    rpmTagReturnType retype = _TagReturn(tag);

    if (type != RPM_INT16_TYPE || count < 1) 
	return 0;
    if (retype != RPM_ARRAY_RETURN_TYPE && count > 1) 
	return 0;
    
    return rpmtdSet(td, tag, type, data, count);
}

int rpmtdFromUint32(rpmtd td, rpmTag tag, rpmuint32_t *data, rpm_count_t count)
{
    rpmTagType type = _TagType(tag);
    rpmTagReturnType retype = _TagReturn(tag);

    if (type != RPM_INT32_TYPE || count < 1) 
	return 0;
    if (retype != RPM_ARRAY_RETURN_TYPE && count > 1) 
	return 0;
    
    return rpmtdSet(td, tag, type, data, count);
}

int rpmtdFromUint64(rpmtd td, rpmTag tag, rpmuint64_t *data, rpm_count_t count)
{
    rpmTagType type = _TagType(tag);
    rpmTagReturnType retype = _TagReturn(tag);

    if (type != RPM_INT64_TYPE || count < 1) 
	return 0;
    if (retype != RPM_ARRAY_RETURN_TYPE && count > 1) 
	return 0;
    
    return rpmtdSet(td, tag, type, data, count);
}

int rpmtdFromString(rpmtd td, rpmTag tag, const char *data)
{
    rpmTagType type = _TagType(tag);
    int rc = 0;

    if (type == RPM_STRING_TYPE)
	rc = rpmtdSet(td, tag, type, data, 1);
    else if (type == RPM_STRING_ARRAY_TYPE)
	rc = rpmtdSet(td, tag, type, &data, 1);

    return rc;
}

int rpmtdFromStringArray(rpmtd td, rpmTag tag, const char **data, rpm_count_t count)
{
    rpmTagType type = _TagType(tag);

    if (type != RPM_STRING_ARRAY_TYPE || count < 1)
	return 0;
    if (type == RPM_STRING_TYPE && count != 1)
	return 0;

    return rpmtdSet(td, tag, type, data, count);
}

int rpmtdFromArgv(rpmtd td, rpmTag tag, const char ** argv)
{
    rpmTagType type = _TagType(tag);
    int count = argvCount(argv);

    if (type != RPM_STRING_ARRAY_TYPE || count < 1)
	return 0;

    return rpmtdSet(td, tag, type, argv, count);
}

int rpmtdFromArgi(rpmtd td, rpmTag tag, const void * _argi)
{
    ARGI_t argi = (ARGI_t) _argi;
    ARGint_t data = argiData(argi);
    int count = argiCount(argi);
    rpmTagType type = _TagType(tag);
    rpmTagReturnType retype = _TagReturn(tag);

    if (type != RPM_INT32_TYPE || retype != RPM_ARRAY_RETURN_TYPE)
	return 0;
    if (count < 1 || data == NULL)
	return 0;

    return rpmtdSet(td, tag, type, data, count);
}

rpmtd rpmtdDup(rpmtd td)
{
    rpmtd newtd = NULL;
    char **data = NULL;
    int i;
    
assert(td != NULL);
    /* TODO: permit other types too */
    if (td->type != RPM_STRING_ARRAY_TYPE && td->type != RPM_I18NSTRING_TYPE) {
	return NULL;
    }

    /* deep-copy container and data, drop immutable flag */
    newtd = rpmtdNew();
    memcpy(newtd, td, sizeof(*td));
    newtd->flags = (rpmtdFlags)
	(newtd->flags & ~(RPMTD_IMMUTABLE));

    newtd->flags = (rpmtdFlags)
	(newtd->flags | (RPMTD_ALLOCED | RPMTD_PTR_ALLOCED));
    newtd->data = data = (char **) xmalloc(td->count * sizeof(*data));
    while ((i = rpmtdNext(td)) >= 0) {
	data[i] = xstrdup(rpmtdGetString(td));
    }

/*@-compdef@*/
    return newtd;
/*@=compdef@*/
}
