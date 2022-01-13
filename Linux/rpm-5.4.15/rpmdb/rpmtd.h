#ifndef _RPMTD_H
#define _RPMTD_H

#include <rpmtag.h>

typedef rpmuint32_t	rpm_count_t;

typedef void *		rpm_data_t;
typedef const void *	rpm_constdata_t;

typedef /*@abstract@*/ struct rpmtd_s * rpmtd;

typedef enum rpmtdFlags_e {
    RPMTD_NONE		= 0,
    RPMTD_ALLOCED	= (1 << 0),	/* was memory allocated? */
    RPMTD_PTR_ALLOCED	= (1 << 1),	/* were array pointers allocated? */
    RPMTD_IMMUTABLE	= (1 << 2),	/* header data or modifiable? */
} rpmtdFlags;

#if defined(_RPMTD_INTERNAL)
/** \ingroup rpmtd
 * Container for rpm tag data (from headers or extensions).
 * @todo		Make this opaque (at least outside rpm itself)
 */
struct rpmtd_s {
    rpmTag tag;		/* rpm tag of this data entry*/
    rpmTagType type;	/* data type */
    rpm_count_t count;	/* number of entries */
/*@relnull@*/
    rpm_data_t data;	/* pointer to actual data */
    rpmtdFlags flags;	/* flags on memory allocation etc */
    int ix;		/* iteration index */
};
#endif	/* _RPMTD_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmtd
 * Create new tag data container
 * @return		New, initialized tag data container.
 */
rpmtd rpmtdNew(void)
	/*@*/;

/** \ingroup rpmtd
 * Destroy tag data container.
 * @param td		Tag data container
 * @return		NULL always
 */
/*@null@*/
rpmtd rpmtdFree(/*@only@*/ rpmtd td)
	/*@modifies td @*/;
 
/** \ingroup rpmtd
 * (Re-)initialize tag data container. Contents will be zeroed out
 * and iteration index reset.
 * @param td		Tag data container
 */
rpmtd rpmtdReset(/*@returned@*/ rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Free contained data. This is always safe to call as the container knows 
 * if data was malloc'ed or not. Container is reinitialized.
 * @param td		Tag data container
 */
void rpmtdFreeData(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Retrieve array size of the container. For non-array types this is always 1.
 * @param td		Tag data container
 * @return		Number of entries in contained data.
 */
rpm_count_t rpmtdCount(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Retrieve tag of the container.
 * @param td		Tag data container
 * @return		Rpm tag.
 */
rpmTag rpmtdTag(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Retrieve type of the container.
 * @param td		Tag data container
 * @return		Rpm tag type.
 */
rpmTagType rpmtdType(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Retrieve current iteration index of the container.
 * @param td		Tag data container
 * @return		Iteration index (or -1 if not iterating)
 */
int rpmtdGetIndex(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Set iteration index of the container.
 * If new index is out of bounds for the container, -1 is returned and
 * iteration index is left untouched. 
 * @param td		Tag data container
 * @param index		New index
 * @return		New index, or -1 if index out of bounds
 */
int rpmtdSetIndex(rpmtd td, int index)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Initialize tag container for iteration
 * @param td		Tag data container
 * @return		0 on success
 */
int rpmtdInit(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Iterate over tag data container.
 * @param td		Tag data container
 * @return		Tag data container iterator index, -1 on termination
 */
int rpmtdNext(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Iterate over rpmuint32_t type tag data container.
 * @param td		Tag data container
 * @return		Pointer to next value, NULL on termination or error
 */
/*@observer@*/ /*@null@*/
rpmuint32_t * rpmtdNextUint32(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Iterate over rpmuint64_t type tag data container.
 * @param td		Tag data container
 * @return		Pointer to next value, NULL on termination or error
 */
/*@observer@*/ /*@null@*/
rpmuint64_t * rpmtdNextUint64(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Iterate over string / string array type tag data container.
 * @param td		Tag data container
 * @return		Pointer to next value, NULL on termination or error
 */
/*@observer@*/ /*@null@*/
const char * rpmtdNextString(rpmtd td)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Return rpmuint8_t data from tag container.
 * For scalar return type, just return pointer to the integer. On array
 * types, return pointer to current iteration index. If the tag container
 * is not for char type, NULL is returned.
 * @param td		Tag data container
 * @return		Pointer to rpmuint16_t, NULL on error
 */
/*@observer@*/ /*@null@*/
rpmuint8_t * rpmtdGetUint8(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Return rpmuint16_t data from tag container.
 * For scalar return type, just return pointer to the integer. On array
 * types, return pointer to current iteration index. If the tag container
 * is not for int16 type, NULL is returned.
 * @param td		Tag data container
 * @return		Pointer to rpmuint16_t, NULL on error
 */
/*@observer@*/ /*@null@*/
rpmuint16_t * rpmtdGetUint16(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Return rpmuint32_t data from tag container.
 * For scalar return type, just return pointer to the integer. On array
 * types, return pointer to current iteration index. If the tag container
 * is not for int32 type, NULL is returned.
 * @param td		Tag data container
 * @return		Pointer to rpmuint32_t, NULL on error
 */
/*@observer@*/ /*@null@*/
rpmuint32_t * rpmtdGetUint32(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Return rpmuint64_t data from tag container.
 * For scalar return type, just return pointer to the integer. On array
 * types, return pointer to current iteration index. If the tag container
 * is not for int64 type, NULL is returned.
 * @param td		Tag data container
 * @return		Pointer to rpmuint64_t, NULL on error
 */
/*@observer@*/ /*@null@*/
rpmuint64_t * rpmtdGetUint64(rpmtd td)
	/*@*/;

/** \ingroup rpmtd
 * Return string data from tag container.
 * For string types, just return the string. On string array types,
 * return the string from current iteration index. If the tag container
 * is not for a string type, NULL is returned.
 * @param td		Tag data container
 * @return		String constant from container, NULL on error
 */
/*@observer@*/ /*@null@*/
const char * rpmtdGetString(rpmtd td)
	/*@*/;

typedef enum rpmtdFormats_e {
    RPMTD_FORMAT_STRING		= 0,	/* plain string (any type) */
    RPMTD_FORMAT_ARMOR		= 1,	/* ascii armor format (bin types) */
    RPMTD_FORMAT_BASE64		= 2,	/* base64 encoding (bin types) */
    RPMTD_FORMAT_PGPSIG		= 3,	/* pgp/gpg signature (bin types) */
    RPMTD_FORMAT_DEPFLAGS	= 4,	/* dependency flags (int32 types) */
    RPMTD_FORMAT_FFLAGS		= 5,	/* file flags (int32 types) */
    RPMTD_FORMAT_PERMS		= 6,	/* permission string (int32 types) */
    RPMTD_FORMAT_TRIGGERTYPE	= 7,	/* trigger types */
    RPMTD_FORMAT_XML		= 8,	/* xml format (any type) */
    RPMTD_FORMAT_OCTAL		= 9,	/* octal format (int32 types) */
    RPMTD_FORMAT_HEX		= 10,	/* hex format (int32 types) */
    RPMTD_FORMAT_DATE		= 11,	/* date format (int32 types) */
    RPMTD_FORMAT_DAY		= 12,	/* day format (int32 types) */
    RPMTD_FORMAT_SHESCAPE	= 13,	/* shell escaped (any type) */
    RPMTD_FORMAT_ARRAYSIZE	= 14,	/* size of contained array (any type) */
} rpmtdFormats;

/** \ingroup rpmtd
 * Format data from tag container to string presentation of given format.
 * Return malloced string presentation of current data in container,
 * converting from integers etc as necessary. On array types, data from
 * current iteration index is used for formatting.
 * @param td		Tag data container
 * @param fmt		Format to apply
 * @param errmsg	Error message from conversion (or NULL)
 * @return		String representation of current data (malloc'ed), 
 * 			NULL on error
 */
/*@null@*/
char * rpmtdFormat(rpmtd td, rpmtdFormats fmt, const char * errmsg)
	/*@*/;

/** \ingroup rpmtd
 * Set container tag and type.
 * For empty container, any valid tag can be set. If the container has
 * data, changing is only permitted to tag of same type. 
 * @param td		Tag data container
 * @param tag		New tag
 * @return		1 on success, 0 on error
 */
int rpmtdSetTag(rpmtd td, rpmTag tag)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from rpmuint8_t pointer.
 * Tag type is checked to be of compatible type (UINT8 or BIN). 
 * For non-array types (BIN is a special case of UINT8 array) 
 * count must be exactly 1.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		Pointer to rpmuint8_t (value or array)
 * @param count		Number of entries
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromUint8(rpmtd td, rpmTag tag, rpmuint8_t *data, rpm_count_t count)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from rpmuint16_t pointer.
 * Tag type is checked to be of UINT16 type. For non-array types count
 * must be exactly 1.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		Pointer to rpmuint16_t (value or array)
 * @param count		Number of entries
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromUint16(rpmtd td, rpmTag tag, rpmuint16_t *data, rpm_count_t count)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from rpmuint32_t pointer.
 * Tag type is checked to be of UINT32 type. For non-array types count
 * must be exactly 1.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		Pointer to rpmuint32_t (value or array)
 * @param count		Number of entries
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromUint32(rpmtd td, rpmTag tag, rpmuint32_t *data, rpm_count_t count)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from rpmuint64_t pointer.
 * Tag type is checked to be of UINT64 type. For non-array types count
 * must be exactly 1.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		Pointer to rpmuint64_t (value or array)
 * @param count		Number of entries
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromUint64(rpmtd td, rpmTag tag, rpmuint64_t *data, rpm_count_t count)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from a string.
 * Tag type is checked to be of string type. 
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		String to use
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromString(rpmtd td, rpmTag tag, const char * data)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from a string array.
 * Tag type is checked to be of string or string array type. For non-array
 * types count must be exactly 1.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param data		Pointer to string array
 * @param count		Number of entries
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromStringArray(rpmtd td, rpmTag tag,
		const char ** data, rpm_count_t count)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from const char ** array.
 * Tag type is checked to be of string array type and array is checked
 * to be non-empty.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param argv		const char ** array
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromArgv(rpmtd td, rpmTag tag, const char ** argv)
	/*@modifies td @*/;

/** \ingroup rpmtd
 * Construct tag container from ARGI_t array.
 * Tag type is checked to be of integer array type and array is checked
 * to be non-empty.
 * @param td		Tag data container
 * @param tag		Rpm tag to construct
 * @param _argi		ARGI array
 * @return		1 on success, 0 on error (eg wrong type)
 */
int rpmtdFromArgi(rpmtd td, rpmTag tag, const void * _argi)
	/*@modifies td @*/;

/* \ingroup rpmtd
 * Perform deep copy of container.
 * Create a modifiable copy of tag data container (on string arrays each
 * string is separately allocated)
 * @todo		Only string arrays types are supported currently
 * @param td		Container to copy
 * @return		New container or NULL on error
 */
/*@null@*/
rpmtd rpmtdDup(rpmtd td)
	/*@modifies td @*/;

#ifdef __cplusplus
}
#endif

#endif /* _RPMTD_H */
