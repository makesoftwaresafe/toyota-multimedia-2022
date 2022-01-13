/*@-sizeoftype@*/
/** \ingroup header
 * \file rpmdb/header_internal.c
 */

#include "system.h"

#define	_RPMTAG_INTERNAL
#include <header_internal.h>

#include "debug.h"

/**
 * Alignment needs (and sizeof scalars types) for internal rpm data types.
 */
/*@observer@*/ /*@unchecked@*/
int rpm_typeAlign[16] =  {
    1,	/*!< RPM_NULL_TYPE */
    1,	/*!< RPM_CHAR_TYPE */
    1,	/*!< RPM_UINT8_TYPE */
    2,	/*!< RPM_UINT16_TYPE */
    4,	/*!< RPM_UINT32_TYPE */
    8,	/*!< RPM_UINT64_TYPE */
    1,	/*!< RPM_STRING_TYPE */
    1,	/*!< RPM_BIN_TYPE */
    1,	/*!< RPM_STRING_ARRAY_TYPE */
    1,	/*!< RPM_I18NSTRING_TYPE */
    0,
    0,
    0,
    0,
    0,
    0
};

int headerVerifyInfo(rpmuint32_t il, rpmuint32_t dl, const void * pev, void * iv, int negate)
{
/*@-castexpose@*/
    entryInfo pe = (entryInfo) pev;
/*@=castexpose@*/
    entryInfo info = (entryInfo) iv;
    rpmuint32_t i;

    for (i = 0; i < il; i++) {
	info->tag = (rpmTag) ntohl(pe[i].tag);
	info->type = (rpmTagType) ntohl(pe[i].type);
	/* XXX Convert RPMTAG_FILESTATE to RPM_UINT8_TYPE. */
	if (info->tag == 1029 && info->type == 1) {
	    info->type = RPM_UINT8_TYPE;
	}
	info->offset = (rpmint32_t) ntohl(pe[i].offset);
assert(negate || info->offset >= 0);	/* XXX insurance */
	if (negate)
	    info->offset = -info->offset;
	info->count = (rpmuint32_t) ntohl(pe[i].count);

	if (hdrchkType(info->type))
	    return (int)i;
	if (hdrchkAlign(info->type, info->offset))
	    return (int)i;
	if (hdrchkRange((rpmint32_t)dl, info->offset))
	    return (int)i;
	if (hdrchkData(info->count))
	    return (int)i;

    }
    return -1;
}
/*@=sizeoftype@*/
