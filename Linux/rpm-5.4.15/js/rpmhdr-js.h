#ifndef H_RPMHDR_JS
#define H_RPMHDR_JS

/**
 * \file js/rpmhdr-js.h
 */

#include "rpm-js.h"

extern JSClass rpmhdrClass;
extern JSClass rpmhiClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject*
rpmjs_InitHdrClass(JSContext *cx, JSObject* obj);

extern JSObject*
rpmjs_NewHdrObject(JSContext *cx, void * _h);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMHDR_JS */
