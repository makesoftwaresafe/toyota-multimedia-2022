#ifndef H_RPMSX_JS
#define H_RPMSX_JS

/**
 * \file js/rpmsx-js.h
 */

#include "rpm-js.h"

extern JSClass rpmsxClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSxClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSxObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSX_JS */
