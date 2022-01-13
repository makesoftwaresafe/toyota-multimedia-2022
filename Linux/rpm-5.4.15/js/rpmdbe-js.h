#ifndef H_RPMDBE_JS
#define H_RPMDBE_JS

/**
 * \file js/rpmdbe-js.h
 */

#include "rpm-js.h"

extern JSClass rpmdbeClass;
#define	OBJ_IS_RPMDBE(_cx, _o)	(JS_GET_CLASS(_cx, _o) == &rpmdbeClass)

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitDbeClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewDbeObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDBE_JS */
