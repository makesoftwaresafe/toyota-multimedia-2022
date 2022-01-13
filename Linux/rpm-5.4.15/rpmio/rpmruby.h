#ifndef RPMRUBY_H
#define RPMRUBY_H

/**
 * \file rpmio/rpmruby.h
 * \ingroup rpmio
 *
 * Embedded Ruby interpreter
 *
 * This allows the embedding of the Ruby interpreter into RPM5. It can be
 * used, for example, for expanding in the fashion of
 * <tt>%%expand{ruby: ...}</tt>.
 *
 * Currently (as of ruby-1.9.2), the Ruby interpreter allows only one instance
 * of itself per process. As such, rpmio's pooling mechanism will also always
 * contain only one ::rpmruby instance. Calling rpmrubyNew() will initialize
 * a new interpreter, while rpmrubyFree() beds this instance. Make sure you
 * keep things in order, that is, call rpmrubyNew() exactly once, and don't
 * forget to call rpmrubyFree() when you're done. Repeatedly calling
 * rpmrubyNew will have no effect while an interpreter is allocated.
 *
 * You can use rpmrubyRun() to evaluate Ruby code and get the result back.
 */

#include <rpmiotypes.h>
#include <rpmio.h>

/** Triggers printing of debugging information */
/*@unchecked@*/
extern int _rpmruby_debug;

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmruby_s* rpmruby;

/**
 * Current (global) interpreter instance
 *
 * At the moment, this variable is merely a safeguard against initializing the
 * Ruby interpreter over and over again. In the future, when there is Ruby
 * support for multiple interpreter instances, a flag given to rpmrubyNew()
 * will use this variable and return the global interpreter instance.
 */
/*@unchecked@*/ /*@relnull@*/
extern rpmruby _rpmrubyI;

#if defined(_RPMRUBY_INTERNAL)
/**
 * Instances of this struct carry an embedded Ruby interpreter and the state
 * associated with it.
 */
struct rpmruby_s {

    /**
     * Pool identifier, including usage mutex, etc.
     * \see rpmioItem_s
     */
    struct rpmioItem_s _item;

    /** The actual interpreter instance */
    void *I;

    /**
     * Carries the interpreter's current state, e.g. the latest return code.
     */
    unsigned long state;

#if defined(__LCLINT__)
    /** Reference count: Currently unused, only to keep splint happy. */
/*@refs@*/
    int nrefs;
#endif
};
#endif /* _RPMRUBY_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dereferences a Ruby interpreter instance.
 *
 * @param ruby		Ruby interpreter pool item
 * @return		NULL on last dereference
 * @see rpmUnlinkPoolItem
 */
/*@unused@*/ /*@null@*/
rpmruby rpmrubyUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmruby ruby)
	/*@modifies ruby @*/;
#define	rpmrubyUnlink(_ruby) \
    ((rpmruby)rpmioUnlinkPoolItem((rpmioItem)(_ruby), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * References a Ruby interpreter instance.
 *
 * @param ruby		Ruby interpreter
 * @return		A new ruby interpreter reference
 * @see rpmioLinkPoolItem
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmruby rpmrubyLink (/*@null@*/ rpmruby ruby)
	/*@modifies ruby @*/;
#define	rpmrubyLink(_ruby) \
    ((rpmruby)rpmioLinkPoolItem((rpmioItem)(_ruby), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * Destroys a Ruby interpreter instance.
 *
 * @param ruby		The Ruby interpreter to be destroyed
 * @return		NULL on last dereference
 * @see rpmioFreePoolItem
 */
/*@null@*/
rpmruby rpmrubyFree(/*@killref@*/ /*@null@*/ rpmruby ruby)
	/*@globals fileSystem @*/
	/*@modifies ruby, fileSystem @*/;
#define	rpmrubyFree(_ruby)	\
    ((rpmruby)rpmioFreePoolItem((rpmioItem)(_ruby), __FUNCTION__, \
        __FILE__, __LINE__))

/**
 * Creates and initializes a Ruby interpreter.
 *
 * @param av		Arguments to the Ruby interpreter (may be NULL)
 * @param flags		Ruby interpreter flags: ((1<<31): use global interpreter)
 * @return		A new Ruby interpreter
 */
/*@newref@*/ /*@null@*/
rpmruby rpmrubyNew(/*@null@*/ char **av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Evaluates Ruby code stored in a string.
 *
 * @param ruby		The Ruby interpreter that is to be used 
 *			(NULL uses global interpreter)
 * @param str		Ruby code to evaluate (NULL forces return of RPMRC_FAIL)
 * @param *resultp	Result of the evaluation
 * @return		RPMRC_OK on success
 */
rpmRC rpmrubyRun(rpmruby ruby, /*@null@*/ const char *str,
		/*@null@*/ const char **resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies ruby, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMRUBY_H */
