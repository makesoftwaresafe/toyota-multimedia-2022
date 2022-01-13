#ifndef H_RPMSEQ_JS
#define H_RPMSEQ_JS

/**
 * \file js/rpmseq-js.h
 */

#include "rpm-js.h"

extern JSClass rpmseqClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject *
rpmjs_InitSeqClass(JSContext *cx, JSObject* obj);

extern JSObject *
rpmjs_NewSeqObject(JSContext *cx, void * _db, uint32_t _flags);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMSEQ_JS */
