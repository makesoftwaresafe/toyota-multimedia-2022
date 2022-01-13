#ifndef H_RPMFI_JS
#define H_RPMFI_JS

/**
 * \file js/rpmfi-js.h
 */

#include "rpm-js.h"

extern JSClass rpmfiClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitFiClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewFiObject(JSContext *cx, void * _h, int _tagN);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMFI_JS */
