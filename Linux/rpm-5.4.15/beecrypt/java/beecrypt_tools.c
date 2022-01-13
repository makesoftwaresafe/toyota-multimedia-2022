#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/api.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_tools.h"

jbyteArray mp_to_bigint(JNIEnv* env, size_t size, mpw* data)
{
	size_t sigbits = mpbits(size, data);
	size_t req = (sigbits + 8) >> 3;

	jbyteArray tmp = (*env)->NewByteArray(env, req);
	jbyte* tmpdata = (*env)->GetByteArrayElements(env, tmp, (jboolean*) 0);

	if (tmpdata)
	{
		int rc = i2osp((byte *)tmpdata, req, data, size);
		(void)rc;

		(*env)->ReleaseByteArrayElements(env, tmp, tmpdata, 0);

		return tmp;
	}
	return 0;
}

void mpnsetbigint(mpnumber* n, JNIEnv* env, jbyteArray val)
{
	if (val)
	{
		jsize size = (*env)->GetArrayLength(env, val);
		jbyte* data = (*env)->GetByteArrayElements(env, val, (jboolean*) 0);

		mpnsetbin(n, (byte *)data, size);
	}
	else
		mpnzero(n);
}

void mpbsetbigint(mpbarrett* b, JNIEnv* env, jbyteArray val)
{
	if (val)
	{
		jsize size = (*env)->GetArrayLength(env, val);
		jbyte* data = (*env)->GetByteArrayElements(env, val, (jboolean*) 0);

		mpbsetbin(b, (byte *)data, size);
	}
	else
		mpbzero(b);
}

#endif
