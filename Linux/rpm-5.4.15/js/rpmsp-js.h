#ifndef H_RPMSP_JS
#define H_RPMSP_JS

/**
 * \file js/rpmsp-js.h
 */

#include "rpm-js.h"

extern JSClass rpmspClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSpClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSpObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSP_JS */
