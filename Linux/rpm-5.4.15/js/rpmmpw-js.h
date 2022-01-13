#ifndef H_RPMMPW_JS
#define H_RPMMPW_JS

/**
 * \file js/rpmmpw-js.h
 */

#include "rpm-js.h"

extern JSClass rpmmpwClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitMpwClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewMpwObject(JSContext *cx, jsval v);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMPW_JS */
