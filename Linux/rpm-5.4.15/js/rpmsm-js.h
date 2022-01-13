#ifndef H_RPMSM_JS
#define H_RPMSM_JS

/**
 * \file js/rpmsm-js.h
 */

#include "rpm-js.h"

extern JSClass rpmsmClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSmClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSmObject(JSContext *cx, const char * _fn, unsigned int _flags);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSM_JS */
