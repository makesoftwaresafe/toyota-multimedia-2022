#ifndef SET_H
#define SET_H

extern int _rpmset_debug;

typedef /*@refcounted@*/ struct set * rpmset;

#if defined(_RPMSET_INTERNAL)
/* Internally, "struct set" is just a bag of strings and their hash values. */
struct set {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    int c;
    struct sv {
	const char *s;
	unsigned v;
    } *sv;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a set wrapper instance.
 * @param set		set wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmset rpmsetUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmset set)
	/*@modifies set @*/;
#define	rpmsetUnlink(_set)	\
    ((rpmset)rpmioUnlinkPoolItem((rpmioItem)(_set), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a set wrapper instance.
 * @param set		set wrapper
 * @return		new set wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmset rpmsetLink (/*@null@*/ rpmset set)
	/*@modifies set @*/;
#define	rpmsetLink(_set)	\
    ((rpmset)rpmioLinkPoolItem((rpmioItem)(_set), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a set wrapper.
 * @param set		set wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmset rpmsetFree(/*@killref@*/ /*@null@*/rpmset set)
	/*@globals fileSystem @*/
	/*@modifies set, fileSystem @*/;
#define	rpmsetFree(_set)	\
    ((rpmset)rpmioFreePoolItem((rpmioItem)(_set), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a set wrapper.
 * @param fn		set file (unused)
 * @param flags		set flags (unused)
 * @return		new set wrapper
 */
/*@newref@*/ /*@null@*/
rpmset rpmsetNew(const char * fn, int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Add new symbol to set.
 */
void rpmsetAdd(rpmset set, const char * sym)
	/*@*/;

/**
 * Make set-version.
 */
const char * rpmsetFinish(rpmset set, int bpp)
	/*@*/;

/* Compare two set-versions.
 * Return value:
 *  1: set1  >  set2
 *  0: set1 ==  set2
 * -1: set1  <  set2 (aka set1 \subset set2)
 * -2: set1 !=  set2
 * -3: set1 decoder error
 * -4: set2 decoder error
 * For performance reasons, set1 should come on behalf of Provides.
 */
int rpmsetCmp(const char * set1, const char * set2)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif
