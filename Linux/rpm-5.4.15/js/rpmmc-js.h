#ifndef H_RPMMC_JS
#define H_RPMMC_JS

/**
 * \file js/rpmmc-js.h
 */

#include "rpm-js.h"

extern JSClass rpmmcClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitMcClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewMcObject(JSContext *cx, jsval v);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMC_JS */
