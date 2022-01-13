#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/md5.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_MD5.h"

jlong JNICALL Java_beecrypt_provider_MD5_allocParam(JNIEnv* env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(md5Param));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	md5Reset((md5Param*) param);

	return param;
}

jlong JNICALL Java_beecrypt_provider_MD5_cloneParam(JNIEnv* env, jclass dummy, jlong param) 
{
	jlong clone = (jlong) malloc(sizeof(md5Param));
	if (clone == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	memcpy((void*) clone, (void*) param, sizeof(md5Param));

	return clone;
}

void JNICALL Java_beecrypt_provider_MD5_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
	if (param)
		free((void*) param);
}

jbyteArray JNICALL Java_beecrypt_provider_MD5_digest__J(JNIEnv* env, jclass dummy, jlong param)
{
	jbyte digest[16];
	jbyteArray result = (*env)->NewByteArray(env, 16);

	md5Digest((md5Param*) param, (byte *)digest);

	(*env)->SetByteArrayRegion(env, result, 0, 16, digest);

	return result;
}

jint JNICALL Java_beecrypt_provider_MD5_digest__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray buf, jint off, jint len)
{
	if (len < 16)
	{
		jclass ex = (*env)->FindClass(env, "java/security/DigestException");
		if (ex)
			(*env)->ThrowNew(env, ex, "len must be at least 16");
	}
	else
	{
		jbyte digest[16];

		md5Digest((md5Param*) param, (byte *)digest);

		(*env)->SetByteArrayRegion(env, buf, off, 16, digest);

		return 16;
	}
	return 0;
}

void JNICALL Java_beecrypt_provider_MD5_reset(JNIEnv* env, jclass dummy, jlong param)
{
	md5Reset((md5Param*) param);
}

void JNICALL Java_beecrypt_provider_MD5_update__JB(JNIEnv* env, jclass dummy, jlong param, jbyte input)
{
	md5Update((md5Param*) param, (byte *)&input, 1);
}

void JNICALL Java_beecrypt_provider_MD5_update__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray input, jint off, jint len)
{
	jbyte* data = (*env)->GetByteArrayElements(env, input, 0);
	if (data)
	{
		md5Update((md5Param*) param, (byte *)data+off, len);

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
