#include "system.h"
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>

#if defined(WITH_JNIEMBED)
#include <jni.h>
#endif

#define _RPMJNI_INTERNAL 1
#include "rpmjni.h"

#include "debug.h"

/*@unchecked@*/
int _rpmjni_debug;

/*@unchecked@*/ /*@relnull@*/
rpmjni _rpmjniI = NULL;

/**
 * Finalizes a JNI interpreter instance/pool item
 */
static void rpmjniFini(void *_jni)
        /*@globals fileSystem @*/
        /*@modifies *_jni, fileSystem @*/
{
    rpmjni jni = (rpmjni) _jni;

#if defined(WITH_JNIEMBED)
    JavaVM * jvm = (JavaVM *) jni->VM;
    jvm->DestroyJavaVM();
#endif
    jni->String = NULL; 
    jni->Integer = NULL;
    jni->Float = NULL;
    jni->Double = NULL;
    jni->mid_intValue = NULL;
    jni->mid_floatValue = NULL;
    jni->mid_doubleValue = NULL;

    jni->mid_eval = NULL;
    jni->IC = NULL;
    jni->I = NULL;
    jni->mid_init = NULL;
    jni->mid_main = NULL;
    jni->C = NULL;
    jni->VM = NULL;
    jni->ENV = NULL;

}

/**
* The pool of JNI interpreter instances
* @see rpmioPool
*/
/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmjniPool;

/**
 * Returns the current rpmio pool responsible for JNI interpreter instances
 *
 * This is a wrapper function that returns the current rpmio pool responsible
 * for embedded JNI interpreters. It creates and initializes a new pool when
 * there is no current pool.
 *
 * @return The current pool
 * @see rpmioNewPool
 */
static rpmjni rpmjniGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _rpmjniPool, fileSystem @*/
        /*@modifies pool, _rpmjniPool, fileSystem @*/
{
    rpmjni jni;

    if (_rpmjniPool == NULL) {
        _rpmjniPool = rpmioNewPool("jni", sizeof(*jni), -1, _rpmjni_debug,
            NULL, NULL, rpmjniFini);
        pool = _rpmjniPool;
    } 

    return (rpmjni) rpmioGetPool(pool, sizeof(*jni));
}

#if defined(WITH_JNIEMBED)
/** Initializes JNI's StringIO for storing output in a string. */
/*@unchecked@*/
static const char * rpmjniInitStringIO = "\
require 'stringio'\n\
$stdout = StringIO.new($result, \"w+\")\n\
";
#endif

static rpmjni rpmjniI(void)
        /*@globals _rpmjniI @*/
        /*@modifies _rpmjniI @*/
{
    if (_rpmjniI == NULL)
        _rpmjniI = rpmjniNew(NULL, 0);
    return _rpmjniI;
}

#if defined(WITH_JNIEMBED)
static jint JNICALL rpmjniVfprintf(FILE *fp, const char *fmt, va_list ap)
{
    jint rc = JNI_OK;
    int msglvl = (fp == stderr ? RPMLOG_ERR : RPMLOG_DEBUG);
    vrpmlog(msglvl, fmt, ap);
    return rc;
}

static void JNICALL rpmjniExit(jint ec)
{
    int msglvl = (ec ? RPMLOG_ERR : RPMLOG_DEBUG);
    rpmlog(msglvl, "%s: JVM exit(%d).\n", __FUNCTION__, ec);
}

static void JNICALL rpmjniAbort(void)
{
    int msglvl = RPMLOG_ERR;
    rpmlog(msglvl, "%s: JVM abort.\n", __FUNCTION__);
}

static int rpmjniCheck(rpmjni jni)
{
    JNIEnv * env = (JNIEnv *) jni->ENV;
    if (env->ExceptionOccurred()) {
	env->ExceptionDescribe();
	return 1;
    }
    return 0;
}
#endif	/* WITH_JNIEMBED */

rpmjni rpmjniNew(char **av, uint32_t flags)
{
    static const char *_av[] = { "rpmjni", NULL };
    int ac;
    
    /* XXX FIXME: recurse like every other embedded interpreter. */
    if (_rpmjniI)
        return _rpmjniI;

    rpmjni jni = rpmjniGetPool(_rpmjniPool);

    if (av == NULL)
        av = (char **) _av;
    ac = argvCount((ARGV_t)av);

#if defined(WITH_JNIEMBED)
    static int oneshot = 0;
    static const char * _jvm_options =
	"%{?_jvm_options}"
	"%{!?_jvm_options:-Xmx128m vfprintf exit abort}";
    const char * optstr = rpmExpand(_jvm_options, NULL);
    ARGV_t opts = NULL;
    int xx = argvSplit(&opts, optstr, NULL);
    int nopts = argvCount(opts);
    size_t nOptions = nopts;
    JavaVMOption * options =
        (JavaVMOption *) xmalloc(nOptions * sizeof(*options));
    JavaVMOption * o = options;
    JavaVMInitArgs vm_args;
    size_t i;

    /* Set up JVM options from %_jvm_options macro.. */
    for (i = 0, o = options; i < nOptions; i++, o++) {
        o->optionString = (char *) opts[i];
        if (!strcmp(o->optionString, "vfprintf"))
            o->extraInfo = (void *) rpmjniVfprintf;
        else if (!strcmp(o->optionString, "exit"))
            o->extraInfo = (void *) rpmjniExit;
        else if (!strcmp(o->optionString, "abort"))
            o->extraInfo = (void *) rpmjniAbort;
        else
            o->extraInfo = NULL;
    }

    /* Fill in default JVM version et al from -ljvm. */
    vm_args.version = JNI_VERSION_1_2;
    xx = JNI_GetDefaultJavaVMInitArgs(&vm_args);
assert(xx == JNI_OK);
    vm_args.options = options;
    vm_args.nOptions = nOptions;
    vm_args.ignoreUnrecognized = JNI_FALSE;

    /* Print out the args for debugging. */
    if (!oneshot) {
	size_t i;
	int msglvl = RPMLOG_DEBUG;
	rpmlog(msglvl, "---------- JVM %d.%d options:\n",
		(vm_args.version >> 16), (vm_args.version & 0xffff));
	for (i = 0; i < nOptions; i++) {
	    JavaVMOption * o = options + i;
	    if (o->extraInfo == NULL)
		rpmlog(msglvl, "\t%s\n", o->optionString);
	    else
		rpmlog(msglvl, "\t%s\t(%p)\n", o->optionString, o->extraInfo);
	}
	rpmlog(msglvl, "----------\n");
	oneshot++;
    }

    JNIEnv * env = NULL;
    JavaVM * jvm = NULL;
    xx = JNI_CreateJavaVM(&jvm, reinterpret_cast<void**>(&env), &vm_args);
assert(xx == JNI_OK);
    jni->ENV = (void *) env;
    jni->VM = (void *) jvm;

    if (options)
	free(options);
    options = NULL;
    opts = argvFree(opts);
    optstr = _free(optstr);

    /* Cache some java.lang.* basic data types */
    jclass String = env->FindClass("java/lang/String");
assert(String != NULL);
    jni->String = (void *) String;

    jclass Integer = env->FindClass("java/lang/Integer");
assert(Integer != NULL);
    jni->Integer = (void *) Integer;
    jmethodID mid_intValue = env->GetMethodID(Integer, "intValue", "()I");
assert(mid_intValue != NULL);
    jni->mid_intValue = (void *) mid_intValue;

    jclass Float = env->FindClass("java/lang/Float");
assert(Float != NULL);
    jni->Float = (void *) Float;
    jmethodID mid_floatValue = env->GetMethodID(Float, "floatValue", "()F");
assert(mid_floatValue != NULL);
    jni->mid_floatValue = (void *) mid_floatValue;

    jclass Double = env->FindClass("java/lang/Double");
assert(Double != NULL);
    jni->Double = (void *) Double;
    jmethodID mid_doubleValue = env->GetMethodID(Double, "doubleValue", "()D");
assert(mid_doubleValue != NULL);
    jni->mid_doubleValue = (void *) mid_doubleValue;

    jclass C = env->FindClass("bsh/Interpreter");
assert(C != NULL);
    jni->C = (void *) C;

#ifdef	DYING	/* XXX incompatible with bsh.eval()? */
    jmethodID mid_main = env->GetStaticMethodID(C,
		"main", "([Ljava/lang/String;)V");
assert(mid_main != NULL);
    jni->mid_main = (void *) mid_main;

    jmethodID mid_main = env->GetStaticMethodID(C,
		"main", "([Ljava/lang/String;)V");
assert(mid_main != NULL);
    jni->mid_main = (void *) mid_main;
#endif	/* DYING */

    /* Initialize and instantiate a bsh.eval() object. */
    jmethodID mid_init = env->GetMethodID(C, "<init>", "()V");
assert(mid_init != NULL);
    jni->mid_init = (void *) mid_init;

    jobject I = env->NewObject(C, mid_init);
assert(I != NULL);
    jni->I = (void *) I;

    jclass IC = env->GetObjectClass(I);
assert(IC != NULL);
    jni->IC = (void *) IC;

    jmethodID mid_eval =
	env->GetMethodID(C, "eval", "(Ljava/lang/String;)Ljava/lang/Object;");
assert(mid_eval != NULL);
    jni->mid_eval = (void *) mid_eval;

#ifdef	DYING	/* XXX incompatible with bsh.eval()? */
    jint jac = 1;
    jobjectArray jav = env->NewObjectArray(jac,
		String, (jobject)NULL);

    /* Interactive shells read ~/.bshrc. */
    jstring js = env->NewStringUTF("/home/jbj/.bshrc");
    env->SetObjectArrayElement(jav, 0, js);
    env->DeleteLocalRef(js);

    env->CallStaticVoidMethod(C, mid_main, jav);
    xx = rpmjniCheck(jni);
    env->DeleteLocalRef(jav);
#endif	/* DYING */

#ifdef	NOISY
    /* Print the banner. */
    jstring js = env->NewStringUTF("printBanner();");
    jobject jo = env->CallObjectMethod(I, mid_eval, js);
    xx = rpmjniCheck(jni);
    env->DeleteLocalRef(js);
#endif

#endif	/* WITH_JNIEMBED */

    return rpmjniLink(jni);
}

rpmRC rpmjniRun(rpmjni jni, const char *str, const char **resultp)
{
    rpmRC rc = RPMRC_FAIL;

if (_rpmjni_debug)
fprintf(stderr, "==> %s(%p,%s,%p)\n", __FUNCTION__, jni, str, resultp);

    if (jni == NULL)
        jni = rpmjniI();

#if defined(WITH_JNIEMBED)
    if (str) {
	JNIEnv * env = (JNIEnv *) jni->ENV;
	jobject I = (jobject) jni->I;
	jmethodID mid_eval = (jmethodID) jni->mid_eval;
	jstring js = env->NewStringUTF(str);
	jobject jo;
	jthrowable jt;

	int xx;

	env->ExceptionClear();

	jo = env->CallObjectMethod(I, mid_eval, js);
	env->DeleteLocalRef(js);

	jt = env->ExceptionOccurred();
	if (jt) {
	    env->ExceptionClear();

	    jclass jtC = env->GetObjectClass(jt);
	    jmethodID mid_msg = env->GetMethodID(jtC, "getMessage",
				"()Ljava/lang/String;");
	    jstring jts = (jstring) env->CallObjectMethod(jt, mid_msg, NULL);
	    jboolean isCopy = JNI_FALSE;
	    const char * s = (jts != NULL
		? env->GetStringUTFChars(jts, &isCopy) : "(null)");

	    *resultp = rpmExpand("EXCEPTION: ", s, NULL);
	    if (jts)
		env->ReleaseStringUTFChars(jts, s);
	    rpmlog(RPMLOG_ERR, "%s\n", s);
	    env->ExceptionDescribe();
	    env->ExceptionClear();
	    rc = RPMRC_FAIL;
	} else
	if (jo) {
	    char b[BUFSIZ];
	    int nb = sizeof(b) - 1;
	    if (env->IsInstanceOf(jo, (jclass) jni->Integer)) {
		jmethodID mid_intValue = (jmethodID) jni->mid_intValue;
		long val = (long) env->CallIntMethod(jo, mid_intValue);
		nb = snprintf(b, nb, "%ld", val);
	    } else
	    if (env->IsInstanceOf(jo, (jclass) jni->Float)) {
		jmethodID mid_floatValue = (jmethodID) jni->mid_floatValue;
		double val = (double) env->CallFloatMethod(jo, mid_floatValue);
		nb = snprintf(b, nb, "%g", val);
	    } else
	    if (env->IsInstanceOf(jo, (jclass) jni->Double)) {
		jmethodID mid_doubleValue = (jmethodID) jni->mid_doubleValue;
		double val = (double) env->CallDoubleMethod(jo,mid_doubleValue);
		nb = snprintf(b, nb, "%g", val);
	    } else
	    if (env->IsInstanceOf(jo, (jclass) jni->String)) {
		jstring js = (jstring) js;
		jboolean isCopy = JNI_FALSE;
		const char * val = env->GetStringUTFChars(js, &isCopy);
		nb = snprintf(b, nb, "%s", val);
		env->ReleaseStringUTFChars(js, val);
	    } else {
		nb = snprintf(b, nb, "%s", "UNKNOWN_OBJECT");
	    }
	    b[nb] = '\0';
	    *resultp = xstrdup(b);
	    rc = RPMRC_OK;
	} else {
	    *resultp = xstrdup("");
	    rc = RPMRC_OK;
	}
	env->DeleteLocalRef(jo);
    }
#endif

    return rc;
}

