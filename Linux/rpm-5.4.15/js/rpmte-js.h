#ifndef H_RPMTE_JS
#define H_RPMTE_JS

/**
 * \file js/rpmte-js.h
 */

#include "rpm-js.h"

extern JSClass rpmteClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitTeClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewTeObject(JSContext *cx, void * _ts, void * _hdro);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMDS_JS */
