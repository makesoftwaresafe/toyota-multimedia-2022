#ifndef H_RPMXAR_JS
#define H_RPMXAR_JS

/**
 * \file js/rpmxar-js.h
 */

#include "rpm-js.h"

extern JSClass rpmxarClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitXarClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewXarObject(JSContext *cx, const char * _fn, const char *_fmode);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMXAR_JS */
