#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacsha512.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_HMACSHA512.h"

jlong JNICALL Java_beecrypt_provider_HMACSHA512_allocParam(JNIEnv* env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(hmacsha512Param));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	sha512Reset((sha512Param*) param);

	return param;
}

jlong JNICALL Java_beecrypt_provider_HMACSHA512_cloneParam(JNIEnv* env, jclass dummy, jlong param) 
{
	jlong clone = (jlong) malloc(sizeof(hmacsha512Param));
	if (clone == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	memcpy((void*) clone, (void*) param, sizeof(sha512Param));

	return clone;
}

void JNICALL Java_beecrypt_provider_HMACSHA512_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
	if (param)
		free((void*) param);
}

jbyteArray JNICALL Java_beecrypt_provider_HMACSHA512_doFinal__J(JNIEnv* env, jclass dummy, jlong param)
{
	jbyte digest[64];
	jbyteArray result = (*env)->NewByteArray(env, 64);

	hmacsha512Digest((hmacsha512Param*) param, (byte *)digest);

	(*env)->SetByteArrayRegion(env, result, 0, 64, digest);

	return result;
}

void JNICALL Java_beecrypt_provider_HMACSHA512_init(JNIEnv* env, jclass dummy, jlong param, jbyteArray rawkey)
{
	jbyte* data = (*env)->GetByteArrayElements(env, rawkey, 0);
	if (data)
	{
		jint len = (*env)->GetArrayLength(env, rawkey);

		hmacsha512Setup((hmacsha512Param*) param, (byte *)data, len);

		(*env)->ReleaseByteArrayElements(env, rawkey, data, JNI_ABORT);
	}
	else
	{
		jclass ex = (*env)->FindClass(env, "java/lang/NullPointerException");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}
}

void JNICALL Java_beecrypt_provider_HMACSHA512_reset(JNIEnv* env, jclass dummy, jlong param)
{
	hmacsha512Reset((hmacsha512Param*) param);
}

void JNICALL Java_beecrypt_provider_HMACSHA512_update__JB(JNIEnv* env, jclass dummy, jlong param, jbyte input)
{
	hmacsha512Update((hmacsha512Param*) param, (byte *)&input, 1);
}

void JNICALL Java_beecrypt_provider_HMACSHA512_update__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray input, jint off, jint len)
{
	jbyte* data = (*env)->GetByteArrayElements(env, input, 0);
	if (data)
	{
		hmacsha512Update((hmacsha512Param*) param, (byte *)data+off, len);

		(*env)->ReleaseByteArrayElements(env, input, data, JNI_ABORT);
	}
	else
	{
		jclass ex = (*env)->FindClass(env, "java/lang/NullPointerException");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}
}

#endif
