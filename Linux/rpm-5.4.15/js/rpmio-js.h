#ifndef H_RPMIO_JS
#define H_RPMIO_JS

/**
 * \file js/rpmio-js.h
 */

#include "rpm-js.h"

extern JSClass rpmioClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitIoClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewIoObject(JSContext *cx, const char * _fn, const char * _fmode);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMIO_JS */
