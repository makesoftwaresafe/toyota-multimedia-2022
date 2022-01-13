#ifndef H_RPMMI_JS
#define H_RPMMI_JS

/**
 * \file js/rpmmi-js.h
 */

#include "rpm-js.h"

extern JSClass rpmmiClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitMiClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewMiObject(JSContext *cx, void * _ts, int _tag, jsval kv);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMI_JS */
