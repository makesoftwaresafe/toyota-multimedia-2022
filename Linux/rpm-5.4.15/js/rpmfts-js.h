#ifndef H_RPMFTS_JS
#define H_RPMFTS_JS

/**
 * \file js/rpmfts-js.h
 */

#include "rpm-js.h"

extern JSClass rpmftsClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitFtsClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewFtsObject(JSContext *cx, JSObject *dno, int _options);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMFTS_JS */
