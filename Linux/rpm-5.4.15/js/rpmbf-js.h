#ifndef H_RPMBF_JS
#define H_RPMBF_JS

/**
 * \file js/rpmbf-js.h
 */

#include "rpm-js.h"

extern JSClass rpmbfClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitBfClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewBfObject(JSContext *cx, size_t _m, size_t _k, unsigned int _flags);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMBF_JS */
