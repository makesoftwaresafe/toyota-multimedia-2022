#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/sha384.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_provider_SHA384.h"

jlong JNICALL Java_beecrypt_provider_SHA384_allocParam(JNIEnv* env, jclass dummy)
{
	jlong param = (jlong) malloc(sizeof(sha384Param));
	if (param == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	sha384Reset((sha384Param*) param);

	return param;
}

jlong JNICALL Java_beecrypt_provider_SHA384_cloneParam(JNIEnv* env, jclass dummy, jlong param) 
{
	jlong clone = (jlong) malloc(sizeof(sha384Param));
	if (clone == 0)
	{
		jclass ex = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (ex)
			(*env)->ThrowNew(env, ex, (const char*) 0);
	}

	memcpy((void*) clone, (void*) param, sizeof(sha384Param));

	return clone;
}

void JNICALL Java_beecrypt_provider_SHA384_freeParam(JNIEnv* env, jclass dummy, jlong param)
{
	if (param)
		free((void*) param);
}

jbyteArray JNICALL Java_beecrypt_provider_SHA384_digest__J(JNIEnv* env, jclass dummy, jlong param)
{
	jbyte digest[48];
	jbyteArray result = (*env)->NewByteArray(env, 48);

	sha384Digest((sha384Param*) param, (byte *)digest);

	(*env)->SetByteArrayRegion(env, result, 0, 48, digest);

	return result;
}

jint JNICALL Java_beecrypt_provider_SHA384_digest__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray buf, jint off, jint len)
{
	if (len < 48)
	{
		jclass ex = (*env)->FindClass(env, "java/security/DigestException");
		if (ex)
			(*env)->ThrowNew(env, ex, "len must be at least 48");
	}
	else
	{
		jbyte digest[48];

		sha384Digest((sha384Param*) param, (byte *)digest);

		(*env)->SetByteArrayRegion(env, buf, off, 48, digest);

		return 48;
	}
	return 0;
}

void JNICALL Java_beecrypt_provider_SHA384_reset(JNIEnv* env, jclass dummy, jlong param)
{
	sha384Reset((sha384Param*) param);
}

void JNICALL Java_beecrypt_provider_SHA384_update__JB(JNIEnv* env, jclass dummy, jlong param, jbyte input)
{
	sha384Update((sha384Param*) param, (byte *)&input, 1);
}

void JNICALL Java_beecrypt_provider_SHA384_update__J_3BII(JNIEnv* env, jclass dummy, jlong param, jbyteArray input, jint off, jint len)
{
	jbyte* data = (*env)->GetByteArrayElements(env, input, 0);
	if (data)
	{
		sha384Update((sha384Param*) param, (byte *)data+off, len);

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
