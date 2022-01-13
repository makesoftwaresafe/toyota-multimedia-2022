#ifndef H_RPMROLLBACK
#define H_RPMROLLBACK

/** \ingroup rpmts
 * \file lib/rpmrollback.h
 *
 */

#include <rpmcli.h>	/* XXX QVA_t typedef */

/**
 */
typedef /*@abstract@*/ struct IDT_s * IDT;

/**
 */
typedef /*@abstract@*/ struct IDTindex_s * IDTX;

#if defined(_RPMROLLBACK_INTERNAL)
/**
 * A rollback transaction id element.
 */
/*@-fielduse@*/
#if !defined(SWIG)
struct IDT_s {
    int done;			/*!< package processed? */
    unsigned int instance;	/*!< installed package transaction id. */
/*@owned@*/ /*@null@*/
    const char * key;		/*! removed package file name. */
    Header h;			/*!< removed package header. */
    union {
	rpmuint32_t u32;	/*!< install/remove transaction id */
    } val;
};
#endif
/*@=fielduse@*/

/**
 * A rollback transaction id index.
 */
#if !defined(SWIG)
struct IDTindex_s {
    int delta;			/*!< no. elements to realloc as a chunk. */
    int size;			/*!< size of id index element. */
    int alloced;		/*!< current number of elements allocated. */
    int nidt;			/*!< current number of elements initialized. */
/*@only@*/ /*@null@*/
    IDT idt;			/*!< id index elements. */
};
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Destroy id index.
 * @param idtx		id index
 * @return		NULL always
 */
/*@null@*/
IDTX IDTXfree(/*@only@*/ /*@null@*/ IDTX idtx)
	/*@modifies idtx @*/;

/**
 * Create id index.
 * @return		new id index
 */
/*@only@*/
IDTX IDTXnew(void)
	/*@*/;

/**
 * Insure that index has room for "need" elements.
 * @param idtx		id index
 * @param need		additional no. of elements needed
 * @return 		id index (with room for "need" elements)
 */
/*@only@*/ /*@null@*/
IDTX IDTXgrow(/*@only@*/ /*@null@*/ IDTX idtx, int need)
	/*@modifies idtx @*/;

/**
 * Sort tag (instance,value) pairs.
 * @param idtx		id index
 * @return 		id index
 */
/*@only@*/ /*@null@*/
IDTX IDTXsort(/*@only@*/ /*@null@*/ IDTX idtx)
	/*@modifies idtx @*/;

/**
 * Load tag (instance,value) pairs from rpm databse, and return sorted id index.
 * @param ts		transaction set
 * @param tag		rpm tag
 * @param rbtid		rollback goal
 * @return 		id index
 */
/*@only@*/ /*@null@*/
IDTX IDTXload(rpmts ts, rpmTag tag, rpmuint32_t rbtid)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState  @*/;

/**
 * Load tag (instance,value) pairs from packages, and return sorted id index.
 * @param ts		transaction set
 * @param globstr	glob expression
 * @param tag		rpm tag
 * @param rbtid		rollback goal
 * @return 		id index
 */
/*@only@*/ /*@null@*/
IDTX IDTXglob(rpmts ts, const char * globstr, rpmTag tag, rpmuint32_t rbtid)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;

/** \ingroup rpmcli
 * Rollback transactions, erasing new, reinstalling old, package(s).
 * @param ts		transaction set
 * @param ia		mode flags and parameters
 * @param argv		array of arguments (NULL terminated)
 * @return		0 on success
 */
int rpmRollback(rpmts ts, QVA_t ia, /*@null@*/ const char ** argv)
	/*@globals rpmcliPackagesTotal, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, ia, rpmcliPackagesTotal, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif  /* H_RPMROLLBACK */
