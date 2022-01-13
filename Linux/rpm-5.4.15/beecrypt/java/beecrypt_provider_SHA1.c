#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/sha1.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_SHA1.h"

jlong JNICALL Java_beecrypt_provider_SHA1_allocParam(JNIEnv* env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(sha1Param));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	sha1Reset((sha1Param*) param);

	return param;
}

jlong JNICALL Java_beecrypt_provider_SHA1_cloneParam(JNIEnv* env, jclass dummy, jlong param) 
{
	jlong clone = (jlong) malloc(sizeof(sha1Param));
	if (clone == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	memcpy((void*) clone, (void*) param, sizeof(sha1Param));

	return clone;
}

void JNICALL Java_beecrypt_provider_SHA1_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
	if (param)
		free((void*) param);
}

jbyteArray JNICALL Java_beecrypt_provider_SHA1_digest__J(JNIEnv* env, jclass dummy, jlong param)
{
	jbyte digest[20];
	jbyteArray result = (*env)->NewByteArray(env, 20);

	sha1Digest((sha1Param*) param, (byte *)digest);

	(*env)->SetByteArrayRegion(env, result, 0, 20, digest);

	return result;
}

jint JNICALL Java_beecrypt_provider_SHA1_digest__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray buf, jint off, jint len)
{
	if (len < 20)
	{
		jclass ex = (*env)->FindClass(env, "java/security/DigestException");
		if (ex)
			(*env)->ThrowNew(env, ex, "len must be at least 20");
	}
	else
	{
		jbyte digest[20];

		sha1Digest((sha1Param*) param, (byte *)digest);

		(*env)->SetByteArrayRegion(env, buf, off, 20, digest);

		return 20;
	}
	return 0;
}

void JNICALL Java_beecrypt_provider_SHA1_reset(JNIEnv* env, jclass dummy, jlong param)
{
	sha1Reset((sha1Param*) param);
}

void JNICALL Java_beecrypt_provider_SHA1_update__JB(JNIEnv* env, jclass dummy, jlong param, jbyte input)
{
	sha1Update((sha1Param*) param, (byte *)&input, 1);
}

void JNICALL Java_beecrypt_provider_SHA1_update__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray input, jint off, jint len)
{
	jbyte* data = (*env)->GetByteArrayElements(env, input, 0);
	if (data)
	{
		sha1Update((sha1Param*) param, (byte *)data+off, len);

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
