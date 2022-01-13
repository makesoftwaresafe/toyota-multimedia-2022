#ifndef H_RPMGI_JS
#define H_RPMGI_JS

/**
 * \file js/rpmgi-js.h
 */

#include "rpm-js.h"

extern JSClass rpmgiClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitGiClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewGiObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMGI_JS */
