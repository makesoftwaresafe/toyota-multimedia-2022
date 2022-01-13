#ifndef H_RPMBC_JS
#define H_RPMBC_JS

/**
 * \file js/rpmbc-js.h
 */

#include "rpm-js.h"

extern JSClass rpmbcClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitBcClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewBcObject(JSContext *cx);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMBC_JS */
