#ifndef H_RPMFC_JS
#define H_RPMFC_JS

/**
 * \file js/rpmfc-js.h
 */

#include "rpm-js.h"

extern JSClass rpmfcClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitFcClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewFcObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMFC_JS */
