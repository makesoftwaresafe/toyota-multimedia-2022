#ifndef RPMMRB_H
#define RPMMRB_H

/**
 * \file rpmio/rpmmrb.h
 * \ingroup rpmio
 *
 * Embedded MRuby interpreter
 *
 * This allows the embedding of the MRuby interpreter into RPM5. It can be
 * used, for example, for expanding in the fashion of
 * <tt>%%expand{mrb: ...}</tt>.
 *
 * Currently (as of mrb-1.9.2), the MRuby interpreter allows only one instance
 * of itself per process. As such, rpmio's pooling mechanism will also always
 * contain only one ::rpmmrb instance. Calling rpmmrbNew() will initialize
 * a new interpreter, while rpmmrbFree() beds this instance. Make sure you
 * keep things in order, that is, call rpmmrbNew() exactly once, and don't
 * forget to call rpmmrbFree() when you're done. Repeatedly calling
 * rpmmrbNew will have no effect while an interpreter is allocated.
 *
 * You can use rpmmrbRun() to evaluate MRuby code and get the result back.
 */

#include <rpmiotypes.h>
#include <rpmio.h>

/** Triggers printing of debugging information */
/*@unchecked@*/
extern int _rpmmrb_debug;

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmmrb_s* rpmmrb;

/**
 * Current (global) interpreter instance
 *
 * At the moment, this variable is merely a safeguard against initializing the
 * MRuby interpreter over and over again. In the future, when there is MRuby
 * support for multiple interpreter instances, a flag given to rpmmrbNew()
 * will use this variable and return the global interpreter instance.
 */
/*@unchecked@*/ /*@relnull@*/
extern rpmmrb _rpmmrbI;

#if defined(_RPMMRB_INTERNAL)
/**
 * Instances of this struct carry an embedded MRuby interpreter and the state
 * associated with it.
 */
struct rpmmrb_s {

    /**
     * Pool identifier, including usage mutex, etc.
     * \see rpmioItem_s
     */
    struct rpmioItem_s _item;

    /** The actual interpreter instance */
    void *I;

    /** The interpreter context */
    void *C;

#if defined(__LCLINT__)
    /** Reference count: Currently unused, only to keep splint happy. */
/*@refs@*/
    int nrefs;
#endif
};
#endif /* _RPMMRB_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dereferences a MRuby interpreter instance.
 *
 * @param mrb		MRuby interpreter pool item
 * @return		NULL on last dereference
 * @see rpmUnlinkPoolItem
 */
/*@unused@*/ /*@null@*/
rpmmrb rpmmrbUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmmrb mrb)
	/*@modifies mrb @*/;
#define	rpmmrbUnlink(_mrb) \
    ((rpmmrb)rpmioUnlinkPoolItem((rpmioItem)(_mrb), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * References a MRuby interpreter instance.
 *
 * @param mrb		MRuby interpreter
 * @return		A new mrb interpreter reference
 * @see rpmioLinkPoolItem
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmmrb rpmmrbLink (/*@null@*/ rpmmrb mrb)
	/*@modifies mrb @*/;
#define	rpmmrbLink(_mrb) \
    ((rpmmrb)rpmioLinkPoolItem((rpmioItem)(_mrb), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * Destroys a MRuby interpreter instance.
 *
 * @param mrb		The MRuby interpreter to be destroyed
 * @return		NULL on last dereference
 * @see rpmioFreePoolItem
 */
/*@null@*/
rpmmrb rpmmrbFree(/*@killref@*/ /*@null@*/ rpmmrb mrb)
	/*@globals fileSystem @*/
	/*@modifies mrb, fileSystem @*/;
#define	rpmmrbFree(_mrb)	\
    ((rpmmrb)rpmioFreePoolItem((rpmioItem)(_mrb), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * Creates and initializes a MRuby interpreter.
 *
 * @param av		Arguments to the MRuby interpreter (may be NULL)
 * @param flags		MRuby interpreter flags: ((1<<31): use global interpreter)
 * @return		A new MRuby interpreter
 */
/*@newref@*/ /*@null@*/
rpmmrb rpmmrbNew(/*@null@*/ char **av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Evaluates MRuby code stored in a string.
 *
 * @param mrb		The MRuby interpreter that is to be used 
 *			(NULL uses global interpreter)
 * @param str		MRuby code to evaluate (NULL forces return of RPMRC_FAIL)
 * @param *resultp	Result of the evaluation
 * @return		RPMRC_OK on success
 */
rpmRC rpmmrbRun(rpmmrb mrb, /*@null@*/ const char *str,
		/*@null@*/ const char **resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies mrb, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMMRB_H */
