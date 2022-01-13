/**
 * \file syck/system.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#ifdef HAVE_MCHECK_H
#include <mcheck.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_UNISTD_H) && !defined(__LCLINT__)
#include <unistd.h>
#endif

#include <ctype.h>

/*@-incondefs@*/
/*@mayexit@*/ /*@only@*/ /*@out@*/ /*@unused@*/
void * xmalloc (size_t size)
	/*@globals errno @*/
	/*@ensures maxSet(result) == (size - 1) @*/
	/*@modifies errno @*/;

/*@mayexit@*/ /*@only@*/ /*@unused@*/
void * xcalloc (size_t nmemb, size_t size)
	/*@ensures maxSet(result) == (nmemb - 1) @*/
	/*@*/;

/*@mayexit@*/ /*@only@*/ /*@unused@*/
void * xrealloc (/*@null@*/ /*@only@*/ void * ptr, size_t size)
	/*@ensures maxSet(result) == (size - 1) @*/
	/*@modifies *ptr @*/;

/*@mayexit@*/ /*@only@*/ /*@unused@*/
char * xstrdup (const char *str)
	/*@*/;
/*@=incondefs@*/

/* Memory allocation via macro defs to get meaningful locations from mtrace() */
#if defined(HAVE_MCHECK_H) && defined(__GNUC__)
#define	vmefail()	(fprintf(stderr, "virtual memory exhausted.\n"), exit(EXIT_FAILURE), NULL)
#define	xmalloc(_size) 		(malloc(_size) ? : vmefail())
#define	xcalloc(_nmemb, _size)	(calloc((_nmemb), (_size)) ? : vmefail())
#define	xrealloc(_ptr, _size)	(realloc((_ptr), (_size)) ? : vmefail())
#define xstrdup(_str)   (strcpy((malloc(strlen(_str)+1) ? : vmefail()), (_str)))
#else
#define	xmalloc(_size) 		malloc(_size)
#define	xcalloc(_nmemb, _size)	calloc((_nmemb), (_size))
#define	xrealloc(_ptr, _size)	realloc((_ptr), (_size))
#define	xstrdup(_str)	strdup(_str)
#endif  /* defined(HAVE_MCHECK_H) && defined(__GNUC__) */

/**
 * Wrapper to free(3), hides const compilation noise, permit NULL, return NULL.
 * @param p		memory to free
 * @return		NULL always
 */
#if defined(WITH_DMALLOC)
#define _free(p) ((p) != NULL ? free((void *)(p)) : (void)0, NULL)
#else
/*@unused@*/ static inline /*@null@*/
void * _free(/*@only@*/ /*@null@*/ /*@out@*/ const void * p)
	/*@modifies p @*/
{
    if (p != NULL)	free((void *)p);
    return NULL;
}
#endif
