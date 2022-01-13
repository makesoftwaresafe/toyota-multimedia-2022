#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/rsakp.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_tools.h"
#include "beecrypt/java/beecrypt_provider_RSAKeyPairGenerator.h"

/* need an adapter from SecureRandom to randomGenerator */

void JNICALL Java_beecrypt_provider_RSAKeyPairGenerator_generate(JNIEnv* env, jobject obj)
{
	jclass cls = (*env)->GetObjectClass(env, obj);
	jfieldID sid = (*env)->GetFieldID(env, cls, "_size", "I");
	jfieldID fid;
	if (sid)
	{
		randomGeneratorContext rngc;
		rsakp pair;
		jint keybits = (*env)->GetIntField(env, obj, sid);

		randomGeneratorContextInit(&rngc, randomGeneratorDefault());

		rsakpInit(&pair);

		fid = (*env)->GetFieldID(env, cls, "_e", "[B");
		if (fid)
		{
			mpnsetbigint(&pair.e, env, (*env)->GetObjectField(env, obj, fid));
		}

		rsakpMake(&pair, &rngc, (size_t) keybits);

		if ((fid = (*env)->GetFieldID(env, cls, "_n", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.n.size, pair.n.modl));

		if ((fid = (*env)->GetFieldID(env, cls, "_e", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.e.size, pair.e.data));

		if ((fid = (*env)->GetFieldID(env, cls, "_d", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.d.size, pair.d.data));

		if ((fid = (*env)->GetFieldID(env, cls, "_p", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.p.size, pair.p.modl));

		if ((fid = (*env)->GetFieldID(env, cls, "_q", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.q.size, pair.q.modl));

		if ((fid = (*env)->GetFieldID(env, cls, "_dp", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.dp.size, pair.dp.data));

		if ((fid = (*env)->GetFieldID(env, cls, "_dq", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.dq.size, pair.dq.data));

		if ((fid = (*env)->GetFieldID(env, cls, "_qi", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.qi.size, pair.qi.data));

		rsakpFree(&pair);
		randomGeneratorContextFree(&rngc);
	}
}

#endif
