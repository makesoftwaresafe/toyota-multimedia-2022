#ifndef RPMJNI_H
#define RPMJNI_H

/** \ingroup rpmio
 * \file rpmio/rpmjni.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmjni_s * rpmjni;

/*@unchecked@*/
extern int _rpmjni_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmjni _rpmjniI;

#if defined(_RPMJNI_INTERNAL)
struct rpmjni_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    void * ENV;			/* JNIEnv* */
    void * VM;			/* JavaVM* */
    void * C;			/* jclass FindClass("bsh/Interpreter") */
    void * mid_main;	/* ("main", "([Ljava/lang/String;)V") */
    void * mid_init;	/* ("<init>", "()V") */
    void * I;			/* jobject NewObject(C, mid_init) */
    void * IC;			/* jclass GetObjectClass(I) */
    void * mid_eval;	/* ("eval", "(Ljava/lang/String;)Ljava/lang/Object;") */
    void * String;		/* jclass */
    void * Integer;			/* jclass */
    void * Float;		/* jclass */
    void * Double;		/* jclass */
    void * mid_intValue;	/* jmethodID */
    void * mid_floatValue;	/* jmethodID */
    void * mid_doubleValue;	/* jmethodID */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMJNI_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a jni interpreter instance.
 * @param jni	jni interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmjni rpmjniUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmjni jni)
	/*@modifies jni @*/;
#define	rpmjniUnlink(_jni)	\
    ((rpmjni)rpmioUnlinkPoolItem((rpmioItem)(_jni), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a jni interpreter instance.
 * @param jni	jni interpreter
 * @return		new jni interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmjni rpmjniLink (/*@null@*/ rpmjni jni)
	/*@modifies jni @*/;
#define	rpmjniLink(_jni)	\
    ((rpmjni)rpmioLinkPoolItem((rpmioItem)(_jni), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a jni interpreter.
 * @param jni	jni interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmjni rpmjniFree(/*@killref@*/ /*@null@*/rpmjni jni)
	/*@globals fileSystem @*/
	/*@modifies jni, fileSystem @*/;
#define	rpmjniFree(_jni)	\
    ((rpmjni)rpmioFreePoolItem((rpmioItem)(_jni), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a jni interpreter.
 * @param av		jni interpreter args (or NULL)
 * @param flags		jni interpreter flags ((1<<31): use global interpreter)
 * @return		new jni interpreter
 */
/*@newref@*/ /*@null@*/
rpmjni rpmjniNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Execute jni string.
 * @param jni	jni interpreter (NULL uses global interpreter)
 * @param str		jni string to execute (NULL returns RPMRC_FAIL)
 * @param *resultp	jni exec result
 * @return		RPMRC_OK on success
 */
rpmRC rpmjniRun(rpmjni jni, /*@null@*/ const char * str,
		/*@null@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies jni, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMJNI_H */
