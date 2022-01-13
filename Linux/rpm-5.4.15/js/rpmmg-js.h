#ifndef H_RPMMG_JS
#define H_RPMMG_JS

/**
 * \file js/rpmmg-js.h
 */

#include "rpm-js.h"

extern JSClass rpmmgClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitMgClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewMgObject(JSContext *cx, const char * _magicfile, int _flags);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMMG_JS */
