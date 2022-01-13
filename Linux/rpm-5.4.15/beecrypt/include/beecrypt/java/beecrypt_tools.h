#ifndef _BEECRYPT_JAVA_TOOLS_H
#define _BEECRYPT_JAVA_TOOLS_H

#include "beecrypt/api.h"
#include "beecrypt/mpbarrett.h"

#ifdef __cplusplus
extern "C" {
#endif

jbyteArray mp_to_bigint(JNIEnv* env, size_t size, mpw* data);

void mpnsetbigint(mpnumber* n, JNIEnv* env, jbyteArray val);
void mpbsetbigint(mpbarrett* b, JNIEnv* env, jbyteArray val);

#ifdef __cplusplus
}
#endif

#endif
