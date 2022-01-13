#ifndef H_RPMIOB_JS
#define H_RPMIOB_JS

/**
 * \file js/rpmiob-js.h
 */

#include "rpm-js.h"

extern JSClass rpmiobClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitIobClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewIobObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMIOB_JS */
