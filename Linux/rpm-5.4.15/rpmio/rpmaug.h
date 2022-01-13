#ifndef	H_RPMAUG
#define	H_RPMAUG

/** \ingroup rpmio
 * \file rpmio/rpmaug.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>
#include <popt.h>

/** \ingroup rpmio
 */
typedef /*@refcounted@*/ struct rpmaug_s * rpmaug;

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmaug_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmaug _rpmaugI;

/*@unchecked@*/ /*@relnull@*/
extern const char *_rpmaugRoot;
/*@unchecked@*/ /*@relnull@*/
extern const char *_rpmaugLoadpath;
/*@unchecked@*/
extern unsigned int _rpmaugFlags;

#if defined(_RPMAUG_INTERNAL)
/** \ingroup rpmio
 */
struct rpmaug_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    const char * root;
    const char * loadpath;
    unsigned int flags;
/*@relnull@*/
    void * I;			/* XXX struct augeas * */
    rpmiob iob;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/*@unchecked@*/ /*@observer@*/
extern const struct poptOption	_rpmaugCommandTable[];

#endif	/* _RPMAUG_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmio
 * Popt option table for options to configure Augeas augtool.
 */
/*@unchecked@*/ /*@observer@*/
extern struct poptOption	rpmaugPoptTable[];

/**
 * Unreference a augeas wrapper instance.
 * @param aug		augeas wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmaug rpmaugUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmaug aug)
	/*@modifies aug @*/;
#define	rpmaugUnlink(_aug)	\
    ((rpmaug)rpmioUnlinkPoolItem((rpmioItem)(_aug), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a augeas wrapper instance.
 * @param aug		augeas wrapper
 * @return		new augeas wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmaug rpmaugLink (/*@null@*/ rpmaug aug)
	/*@modifies aug @*/;
#define	rpmaugLink(_aug)	\
    ((rpmaug)rpmioLinkPoolItem((rpmioItem)(_aug), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a augeas wrapper.
 * @param aug		augeas wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmaug rpmaugFree(/*@killref@*/ /*@null@*/rpmaug aug)
	/*@globals fileSystem @*/
	/*@modifies aug, fileSystem @*/;
#define	rpmaugFree(_aug)	\
    ((rpmaug)rpmioFreePoolItem((rpmioItem)(_aug), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a augeas wrapper.
 * @todo Change to common RPM embedded interpreter API.
 * @param root		augeas filesystem root
 * @param loadpath	augeas load path (colon separated)
 * @param flags		augeas flags
 * @return		new augeas wrapper
 */
/*@newref@*/ /*@null@*/
rpmaug rpmaugNew(/*@null@*/ const char * root, /*@null@*/ const char * loadpath,
		unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Define an augeas variable.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param name		variable name
 * @param expr		expression to be evaluated
 * @return		-1 on error, or no. nodes in nodeset
 */
int rpmaugDefvar(/*@null@*/ rpmaug aug, const char * name, const char * expr)
	/*@modifies aug @*/;

/**
 * Define an augeas node.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param name		variable name
 * @param expr		expression to be evaluated (must eval to a nodeset)
 * @param value		initial node value (if creating)
 * @retval *created	1 if node was created
 * @return		-1 on error, or no. nodes in set
 */
int rpmaugDefnode(/*@null@*/ rpmaug aug, const char * name, const char * expr,
		const char * value, /*@out@*/ /*@null@*/ int * created)
	/*@modifies aug, *created @*/;

/**
 * Get the value associated with a path.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param path		path to lookup
 * @retval *value	returned value (malloc'd)
 * @return		-1 if multiple paths, 0 if none, 1 on success
 */
int rpmaugGet(/*@null@*/ rpmaug aug, const char * path,
		/*@out@*/ /*@null@*/ const char ** value)
	/*@modifies aug, *value @*/;

/**
 * Set the value associated with a path.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param path		path to lookup
 * @param value		value
 * @return		-1 if multiple paths, 0 if none, 1 on success
 */
int rpmaugSet(/*@null@*/ rpmaug aug, const char * path, const char * value)
	/*@modifies aug @*/;

/**
 * Insert new sibling node before/after a given node.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param path		path to node in tree
 * @param label		label to insert
 * @param before	insert label into tree before path? (else after)
 * @return		-1 on failure, 0 on success
 */
int rpmaugInsert(/*@null@*/ rpmaug aug, const char * path, const char * label,
		int before)
	/*@modifies aug @*/;

/**
 * Remove node and associated sub-tree.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param path		path to node in tree to remove
 * @return		-1 on failure, 0 on success
 */
int rpmaugRm(/*@null@*/ rpmaug aug, const char * path)
	/*@modifies aug @*/;

/**
 * Move src node to dst node.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param src		src path to node in tree
 * @param dst		dst path to node in tree
 * @return		-1 on failure, 0 on success
 */
int rpmaugMv(/*@null@*/ rpmaug aug, const char * src, const char * dst)
	/*@modifies aug @*/;

/**
 * Return path(s) in tree that match an expression.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param path		path expression to match
 * @retval *matches	paths that match
 * @return		no. of matches
 */
int rpmaugMatch(/*@null@*/ rpmaug aug, const char * path,
		/*@out@*/ /*@null@*/ char *** matches)
	/*@modifies aug, *matches @*/;

/**
 * Save changed files to disk, appending .augnew or .augsave as requested.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @return		-1 on error, 0 on success
 */
int rpmaugSave(/*@null@*/ rpmaug aug)
	/*@modifies aug @*/;

/**
 * Load files/lenses from disk.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @return		-1 on error, 0 on success
 */
int rpmaugLoad(/*@null@*/ rpmaug aug)
	/*@modifies aug @*/;

/**
 * Print node paths that match an expression.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param out		ouput file (NULL uses stdout)
 * @param path		node path expression
 * @return		0 on success, <0 on error
 */
int rpmaugPrint(/*@null@*/ rpmaug aug, /*@null@*/ FILE * out, const char * path)
	/*@modifies aug, *out @*/;

/**
 * Append augeas output to an iob.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param fmt		format to use
 */
void rpmaugFprintf(rpmaug aug, const char *fmt, ...)
	/*@modifies aug @*/;

/**
 * Run augeas commands from a buffer.
 * @param aug		augeas wrapper (NULL uses global interpreter)
 * @param str		augeas commands to run
 * @retval *resultp	output running augeas commands
 * @return		RPMRC_OK on success
 */
rpmRC rpmaugRun(rpmaug aug, const char * str, const char ** resultp)
	/*@modifies aug, *resultp @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMAUG */
