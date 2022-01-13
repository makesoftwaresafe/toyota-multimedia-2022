#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacmd5.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_HMACMD5.h"

jlong JNICALL Java_beecrypt_provider_HMACMD5_allocParam(JNIEnv* env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(hmacmd5Param));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	md5Reset((md5Param*) param);

	return param;
}

jlong JNICALL Java_beecrypt_provider_HMACMD5_cloneParam(JNIEnv* env, jclass dummy, jlong param) 
{
	jlong clone = (jlong) malloc(sizeof(hmacmd5Param));
	if (clone == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	memcpy((void*) clone, (void*) param, sizeof(md5Param));

	return clone;
}

void JNICALL Java_beecrypt_provider_HMACMD5_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
	if (param)
		free((void*) param);
}

jbyteArray JNICALL Java_beecrypt_provider_HMACMD5_doFinal__J(JNIEnv* env, jclass dummy, jlong param)
{
	jbyte digest[16];
	jbyteArray result = (*env)->NewByteArray(env, 16);

	hmacmd5Digest((hmacmd5Param*) param, (byte *)digest);

	(*env)->SetByteArrayRegion(env, result, 0, 16, digest);

	return result;
}

void JNICALL Java_beecrypt_provider_HMACMD5_init(JNIEnv* env, jclass dummy, jlong param, jbyteArray rawkey)
{
	jbyte* data = (*env)->GetByteArrayElements(env, rawkey, 0);
	if (data)
	{
		jint len = (*env)->GetArrayLength(env, rawkey);

		hmacmd5Setup((hmacmd5Param*) param, (byte *)data, len);

		(*env)->ReleaseByteArrayElements(env, rawkey, data, JNI_ABORT);
	}
	else
	{
		jclass ex = (*env)->FindClass(env, "java/lang/NullPointerException");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}
}

void JNICALL Java_beecrypt_provider_HMACMD5_reset(JNIEnv* env, jclass dummy, jlong param)
{
	hmacmd5Reset((hmacmd5Param*) param);
}

void JNICALL Java_beecrypt_provider_HMACMD5_update__JB(JNIEnv* env, jclass dummy, jlong param, jbyte input)
{
	hmacmd5Update((hmacmd5Param*) param, (byte *)&input, 1);
}

void JNICALL Java_beecrypt_provider_HMACMD5_update__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray input, jint off, jint len)
{
	jbyte* data = (*env)->GetByteArrayElements(env, input, 0);
	if (data)
	{
		hmacmd5Update((hmacmd5Param*) param, (byte *)data+off, len);

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
