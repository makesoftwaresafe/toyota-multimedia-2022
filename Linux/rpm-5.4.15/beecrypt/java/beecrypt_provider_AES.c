#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/aes.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_AES.h"

jlong JNICALL Java_beecrypt_provider_AES_allocParam(JNIEnv *env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(aesParam));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	return param;
}

void JNICALL Java_beecrypt_provider_AES_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
    if (param)
        free((void*) param);
}

#endif
