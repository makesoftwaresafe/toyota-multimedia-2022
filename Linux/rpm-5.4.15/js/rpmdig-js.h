#ifndef H_RPMDIG_JS
#define H_RPMDIG_JS

/**
 * \file js/rpmdig-js.h
 */

#include "rpm-js.h"

extern JSClass rpmdigClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitDigClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewDigObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDIG_JS */
