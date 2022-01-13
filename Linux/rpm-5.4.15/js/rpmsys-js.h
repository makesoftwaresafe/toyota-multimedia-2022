#ifndef H_RPMSYS_JS
#define H_RPMSYS_JS

/**
 * \file js/rpmsys-js.h
 */

#include "rpm-js.h"

extern JSClass rpmsysClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSysClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSysObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSYS_JS */
