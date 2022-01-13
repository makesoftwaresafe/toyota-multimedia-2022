#ifndef H_SYCK_JS
#define H_SYCK_JS

/**
 * \file js/syck-js.h
 */

#include "rpm-js.h"

extern JSClass syckClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject*
rpmjs_InitSyckClass(JSContext *cx, JSObject* obj);

#ifdef __cplusplus      
}
#endif

#endif	/* H_SYCK_JS */
