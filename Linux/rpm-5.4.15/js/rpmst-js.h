#ifndef H_RPMST_JS
#define H_RPMST_JS

/**
 * \file js/rpmst-js.h
 */

#include "rpm-js.h"

extern JSClass rpmstClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitStClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewStObject(JSContext *cx, jsval fnv);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMST_JS */
