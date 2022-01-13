#ifndef H_RPMDC_JS
#define H_RPMDC_JS

/**
 * \file js/rpmdc-js.h
 */

#include "rpm-js.h"

extern JSClass rpmdcClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitDcClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewDcObject(JSContext *cx, unsigned int _dalgo, unsigned int _flags);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDC_JS */
