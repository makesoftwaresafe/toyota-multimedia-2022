#ifndef H_RPMMPF_JS
#define H_RPMMPF_JS

/**
 * \file js/rpmmpf-js.h
 */

#include "rpm-js.h"

extern JSClass rpmmpfClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitMpfClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewMpfObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMPF_JS */
