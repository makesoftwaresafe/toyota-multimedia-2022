#ifndef H_UUID_JS
#define H_UUID_JS

/**
 * \file js/uuid-js.h
 */

#include "rpm-js.h"

extern JSClass uuidClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject*
rpmjs_InitUuidClass(JSContext *cx, JSObject* obj);

#ifdef __cplusplus      
}
#endif

#endif	/* H_UUID_JS */
