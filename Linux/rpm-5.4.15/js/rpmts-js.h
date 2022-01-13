#ifndef H_RPMTS_JS
#define H_RPMTS_JS

/**
 * \file js/rpmts-js.h
 */

#include "rpm-js.h"

extern JSClass rpmtsClass;
extern JSClass rpmtsiClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitTsClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewTsObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMTS_JS */
