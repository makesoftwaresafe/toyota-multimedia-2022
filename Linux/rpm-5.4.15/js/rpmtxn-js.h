#ifndef H_RPMTXN_JS
#define H_RPMTXN_JS

/**
 * \file js/rpmtxn-js.h
 */

#include "rpm-js.h"

extern JSClass rpmtxnClass;
#define	OBJ_IS_RPMTXN(_cx, _o)	(JS_GET_CLASS(_cx, _o) == &rpmtxnClass)

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitTxnClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewTxnObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMTXN_JS */
