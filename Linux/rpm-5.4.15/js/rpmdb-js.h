#ifndef H_RPMDB_JS
#define H_RPMDB_JS


/**
 * \file js/rpmdb-js.h
 */

#include "rpm-js.h"

extern JSClass rpmdbClass;
#define	OBJ_IS_RPMDB(_cx, _o)	(JS_GET_CLASS(_cx, _o) == &rpmdbClass)

#if defined(_RPMDB_JS_INTERNAL)
#include <db.h>

/**
 * A type punned data structure to avoid allocating int32/int64 keys/values.
 */
typedef struct _RPMDBT_s {
    DBT dbt;
    union {
	int32_t _i;
	int64_t _ll;
    } _u;
} _RPMDBT;
#define	_RPMDBT_INIT	{{0}}
#define	_RPMDBT_PTR(_p)	(&(_p).dbt)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitDbClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewDbObject(JSContext *cx, void * _dbenv, uint32_t _flags);

#if defined(_RPMDB_JS_INTERNAL)
extern int rpmdb_v2dbt(JSContext *cx, jsval v, _RPMDBT * p);
#endif

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDB_JS */
