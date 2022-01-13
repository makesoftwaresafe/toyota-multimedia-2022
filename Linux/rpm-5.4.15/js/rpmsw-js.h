#ifndef H_RPMSW_JS
#define H_RPMSW_JS

/**
 * \file js/rpmsw-js.h
 */

#include "rpm-js.h"

extern JSClass rpmswClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSwClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSwObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSW_JS */
