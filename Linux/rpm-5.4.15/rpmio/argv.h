#ifndef _H_ARGV_
#define	_H_ARGV_

/** \ingroup rpmio
 * \file rpmio/argv.h
 */
#include <stdio.h>

typedef	const char * ARGstr_t;
typedef ARGstr_t * ARGV_t;

typedef	unsigned int * ARGint_t;

struct ARGI_s {
    unsigned nvals;
    ARGint_t vals;
};
typedef	struct ARGI_s * ARGI_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print argv array elements.
 * @param msg		output message prefix (or NULL)
 * @param argv		argv array
 * @param fp		output file handle (NULL uses stderr)
 */
void argvPrint(/*@null@*/ const char * msg, /*@null@*/ ARGV_t argv,
		/*@null@*/ FILE * fp)
	/*@globals fileSystem @*/
	/*@modifies *fp, fileSystem @*/;

/**
 * Destroy an argi array.
 * @param argi		argi array
 * @return		NULL always
 */
/*@null@*/
ARGI_t argiFree(/*@only@*/ /*@null@*/ ARGI_t argi)
	/*@modifies argi @*/;

/**
 * Destroy an argv array.
 * @param argv		argv array
 * @return		NULL always
 */
/*@null@*/
ARGV_t argvFree(/*@only@*/ /*@null@*/ ARGV_t argv)
	/*@modifies argv @*/;

/**
 * Return no. of elements in argi array.
 * @param argi		argi array
 * @return		no. of elements
 */
int argiCount(/*@null@*/ const ARGI_t argi)
	/*@*/;

/**
 * Return data from argi array.
 * @param argi		argi array
 * @return		argi array data address
 */
/*@null@*/
ARGint_t argiData(/*@null@*/ ARGI_t argi)
	/*@*/;

/**
 * Return no. of elements in argv array.
 * @param argv		argv array
 * @return		no. of elements
 */
int argvCount(/*@null@*/ ARGV_t argv)
	/*@*/;

/**
 * Return data from argv array.
 * @param argv		argv array
 * @return		argv array data address
 */
/*@null@*/
ARGV_t argvData(/*@null@*/ ARGV_t argv)
	/*@*/;

/**
 * Compare argi elements (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
/*@-exportlocal@*/
int argiCmp(ARGint_t * a, ARGint_t * b)
	/*@*/;
/*@=exportlocal@*/

/**
 * Compare argv elements using strcmp (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
/*@-exportlocal@*/
int argvCmp(ARGstr_t * a, ARGstr_t * b)
	/*@*/;
/*@=exportlocal@*/

/**
 * Compare argv elements using strcasecmp (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
int argvStrcasecmp(ARGstr_t * a, ARGstr_t * b)
	/*@*/;

#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
/**
 * Wildcard-match argv arrays using fnmatch.
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
int argvFnmatch(ARGstr_t * a, ARGstr_t * b)
	/*@*/;

/**
 * Wildcard-match argv arrays using fnmatch (case-insensitive).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
int argvFnmatchCasefold(ARGstr_t * a, ARGstr_t * b)
	/*@*/;
#endif

/**
 * Sort an argi array.
 * @param argi		argi array
 * @param compar	strcmp-like comparison function, or NULL for argiCmp()
 * @return		0 always
 */
int argiSort(ARGI_t argi, int (*compar)(ARGint_t *, ARGint_t *))
	/*@*/;

/**
 * Sort an argv array.
 * @param argv		argv array
 * @param compar	strcmp-like comparison function, or NULL for argvCmp()
 * @return		0 always
 */
int argvSort(ARGV_t argv, int (*compar)(ARGstr_t *, ARGstr_t *))
	/*@modifies *argv @*/;

/**
 * Find an element in an argv array.
 * @param argv		argv array
 * @param val		string to find
 * @param compar	strcmp-like comparison function, or NULL for argvCmp()
 * @return		found string (NULL on failure)
 */
/*@dependent@*/ /*@null@*/
ARGV_t argvSearch(ARGV_t argv, ARGstr_t val,
		int (*compar)(ARGstr_t *, ARGstr_t *))
	/*@*/;

#if defined(RPM_VENDOR_OPENPKG) /* wildcard-matching-arbitrary-tagnames */
/**
 * Find an element in an argv array (via linear searching and just match/no-match comparison).
 * @param argv		argv array
 * @param val		string to find
 * @param compar	strcmp-like comparison function, or NULL for argvCmp()
 * @return		found string (NULL on failure)
 */
/*@dependent@*/ /*@null@*/
ARGV_t argvSearchLinear(ARGV_t argv, ARGstr_t val,
		int (*compar)(ARGstr_t *, ARGstr_t *))
	/*@*/;
#endif

/**
 * Add an int to an argi array.
 * @retval *argip	argi array
 * @param ix		argi array index (or -1 to append)
 * @param val		int arg to add
 * @return		0 always
 */
int argiAdd(/*@out@*/ ARGI_t * argip, int ix, int val)
	/*@modifies *argip @*/;

/**
 * Add a string to an argv array.
 * @retval *argvp	argv array
 * @param val		string arg to append
 * @return		0 always
 */
int argvAdd(/*@out@*/ ARGV_t * argvp, ARGstr_t val)
	/*@modifies *argvp @*/;

/**
 * Append one argv array to another.
 * @retval *argvp	argv array
 * @param av		argv array to append (NULL does nothing)
 * @return		0 always
 */
int argvAppend(/*@out@*/ ARGV_t * argvp, /*@null@*/ ARGV_t av)
	/*@modifies *argvp @*/;

/**
 * Split a string into an argv array.
 * @retval *argvp	argv array
 * @param str		string arg to split
 * @param seps		separator characters (NULL is C isspace() chars)
 * @return		0 always
 */
int argvSplit(ARGV_t * argvp, const char * str, /*@null@*/ const char * seps)
	/*@modifies *argvp @*/;

/**
 * Concatenate an argv array into a string.
 * @param argv		argv array
 * @param sep		arg separator
 * @return		concatenated string
 */
/*@only@*/
char * argvJoin(ARGV_t argv, char sep)
	/*@*/;

/**
 * Read lines into an argv array.
 * @retval *argvp	argv array
 * @param fd		rpmio FD_t (NULL uses stdin)
 * @return		0 on success
 */
int argvFgets(ARGV_t * argvp, void * fd)
	/*@globals fileSystem@*/
	/*@modifies *argvp, fd, fileSystem @*/;

#ifdef __cplusplus
}
#endif

#endif /* _H_ARGV_ */
