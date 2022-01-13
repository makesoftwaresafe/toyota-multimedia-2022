#ifndef H_RPMDIR_JS
#define H_RPMDIR_JS

/**
 * \file js/rpmdir-js.h
 */

#include "rpm-js.h"

extern JSClass rpmdirClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitDirClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewDirObject(JSContext *cx, const char * _dn);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDIR_JS */
