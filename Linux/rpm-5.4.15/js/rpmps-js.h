#ifndef H_RPMPS_JS
#define H_RPMPS_JS

/**
 * \file js/rpmps-js.h
 */

#include "rpm-js.h"

extern JSClass rpmpsClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitPsClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewPsObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMPS_JS */
