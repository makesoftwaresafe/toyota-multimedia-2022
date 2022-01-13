#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/dlkp.h"

#if JAVAGLUE

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MALLOC_H
# include <malloc.h>
#endif

#include "beecrypt/java/beecrypt_tools.h"
#include "beecrypt/java/beecrypt_provider_DHKeyPairGenerator.h"

/* need an adapter from SecureRandom to randomGenerator */

void JNICALL Java_beecrypt_provider_DHKeyPairGenerator_generate(JNIEnv* env, jobject obj)
{
	jclass cls = (*env)->GetObjectClass(env, obj);
	jfieldID sid = (*env)->GetFieldID(env, cls, "_size", "I");
	jfieldID fid;
	if (sid)
	{
		randomGeneratorContext rngc;
		dlkp_p pair;
		jint keybits = (*env)->GetIntField(env, obj, sid);
		jint pribits = keybits;

		randomGeneratorContextInit(&rngc, randomGeneratorDefault());
		dlkp_pInit(&pair);

		fid = (*env)->GetFieldID(env, cls, "_p", "[B");
		if (fid)
		{
			mpbsetbigint(&pair.param.p, env, (*env)->GetObjectField(env, obj, fid));
		}
		fid = (*env)->GetFieldID(env, cls, "_g", "[B");
		if (fid)
		{
			mpnsetbigint(&pair.param.g, env, (*env)->GetObjectField(env, obj, fid));
		}
		fid = (*env)->GetFieldID(env, cls, "_l", "I");
		if (fid)
		{
			pribits = (*env)->GetIntField(env, obj, fid);
		}

		if (mpz(pair.param.p.size, pair.param.p.modl) || mpz(pair.param.g.size, pair.param.g.data))
			dldp_pgonMakeSafe(&pair.param, &rngc, keybits);

		dldp_pPair_s(&pair.param, &rngc, &pair.x, &pair.y, pribits);

		if ((fid = (*env)->GetFieldID(env, cls, "_p", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.param.p.size, pair.param.p.modl));
		if ((fid = (*env)->GetFieldID(env, cls, "_g", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.param.g.size, pair.param.g.data));
		if ((fid = (*env)->GetFieldID(env, cls, "_x", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.x.size, pair.x.data));
		if ((fid = (*env)->GetFieldID(env, cls, "_y", "[B")))
			(*env)->SetObjectField(env, obj, fid, mp_to_bigint(env, pair.y.size, pair.y.data));

		dlkp_pFree(&pair);

		randomGeneratorContextFree(&rngc);
	}
}

#endif
