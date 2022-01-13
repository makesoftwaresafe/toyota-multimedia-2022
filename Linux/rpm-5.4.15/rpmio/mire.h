#ifndef H_MIRE
#define H_MIRE

/** \ingroup rpmtrans
 * \file rpmio/mire.h
 * RPM pattern matching.
 */

#include <rpmiotypes.h>

/*@-noparams@*/
#include <fnmatch.h>
/*@=noparams@*/
#if defined(__LCLINT__)
/*@-declundef -exportheader -redecl @*/ /* LCL: missing annotation */
extern int fnmatch (const char *__pattern, const char *__name, int __flags)
	/*@*/;
/*@=declundef =exportheader =redecl @*/
#endif

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _mire_debug;
/*@=exportlocal@*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
extern rpmioPool _mirePool;

/**
 */
/*@unchecked@*/ /*@null@*/ /*@shared@*/
extern const unsigned char * _mirePCREtables;

/** Line ending types */
typedef enum mireEL_e { EL_LF, EL_CR, EL_CRLF, EL_ANY, EL_ANYCRLF } mireEL_t;

/*@unchecked@*/
extern mireEL_t _mireEL;

/** STRING default: 0 */
/*@unchecked@*/
extern int _mireSTRINGoptions;

/** GLOB default: FNM_PATHNAME | FNM_PERIOD */
/*@unchecked@*/
extern int _mireGLOBoptions;

/** REGEX default: REG_EXTENDED */
/*@unchecked@*/
extern int _mireREGEXoptions;

/** PCRE default: 0 */
/*@unchecked@*/
extern int _mirePCREoptions;

/**
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct miRE_s * miRE;

/**
 * Tag value pattern match mode.
 */
typedef enum rpmMireMode_e {
    RPMMIRE_DEFAULT	= 0,	/*!< posix regex with \., .* and ^...$ added */
    RPMMIRE_STRCMP	= 1,	/*!< strings using strcmp(3) */
    RPMMIRE_REGEX	= 2,	/*!< posix regex(7) patterns using regcomp(3) */
    RPMMIRE_GLOB	= 3,	/*!< glob(7) patterns using fnmatch(3) */
    RPMMIRE_PCRE	= 4	/*!< pcre patterns using pcre_compile2(3) */
} rpmMireMode;

#if defined(_MIRE_INTERNAL)

#if defined(__LCLINT__)
/*@-declundef -exportheader @*/ /* LCL: missing modifies (only is bogus) */
extern void regfree (/*@only@*/ regex_t *preg)
	/*@modifies *preg @*/;
/*@=declundef =exportheader @*/
#endif

#if defined(WITH_PCRE)
#include <pcre.h>
#endif
#if defined(WITH_PCRE) && defined(WITH_PCRE_POSIX)
#include <pcreposix.h>
#else
#include <regex.h>
#endif

/**
 */
struct miRE_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmMireMode	mode;		/*!< pattern match mode */
/*@only@*/ /*@relnull@*/
    const char *pattern;	/*!< pattern string */
/*@only@*/ /*@relnull@*/
    regex_t *preg;		/*!< regex compiled pattern buffer */
/*@only@*/ /*@relnull@*/
    void *pcre;			/*!< pcre compiled pattern buffer. */
/*@only@*/ /*@relnull@*/
    void *hints;		/*!< pcre compiled pattern hints buffer. */
/*@shared@*/ /*@relnull@*/
    const char * errmsg;	/*!< pcre error message. */
/*@shared@*/ /*@relnull@*/
    const unsigned char * table;/*!< pcre locale table. */
/*@kept@*/
    int * offsets;		/*!< pcre substring offset table. */
    int noffsets;		/*!< pcre substring offset table count. */
    int erroff;			/*!< pcre error offset. */
    int errcode;		/*!< pcre error code. */
    int	fnflags;	/*!< fnmatch(3) flags (0 uses FNM_PATHNAME|FNM_PERIOD)*/
    int	cflags;		/*!< regcomp(3) flags (0 uses REG_EXTENDED|REG_NOSUB) */
    int	eflags;		/*!< regexec(3) flags */
    int coptions;	/*!< pcre_compile2(3) options. */
    int startoff;	/*!< pcre_exec(3) starting offset. */
    int eoptions;	/*!< pcre_exec(3) options. */
    int notmatch;		/*!< non-zero: negative match, like "grep -v" */
    int tag;			/*!< sort identifier (e.g. an rpmTag) */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;				/*!< (unused) keep splint happy */
#endif
};
#endif	/* defined(_MIRE_INTERNAL) */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Deallocate pattern match memory.
 * @param mire		pattern container
 * @return		0 on success
 */
int mireClean(/*@null@*/ miRE mire)
	/*@modifies mire @*/;

/**
 * Allocate a miRE container from the pool.
 * @param pool		mire pool
 * @return		miRE container
 */
miRE mireGetPool(/*@null@*/ rpmioPool pool)
        /*@globals _mirePool, fileSystem @*/
        /*@modifies pool, _mirePool, fileSystem @*/;

/**
 * Unreference a pattern container instance.
 * @param mire		pattern container
 * @return		new pattern container reference
 */
/*@unused@*/ /*@null@*/
miRE mireUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ miRE mire)
        /*@globals fileSystem @*/
        /*@modifies mire, fileSystem @*/;
#define	mireUnlink(_mire)	\
    (miRE)rpmioUnlinkPoolItem((rpmioItem)_mire, __FUNCTION__, __FILE__, __LINE__)

/**
 * Reference a pattern container instance.
 * @param mire		pattern container
 * @return		new pattern container reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
miRE mireLink (/*@null@*/ miRE mire)
	/*@modifies mire @*/;
#define	mireLink(_mire)	\
    (miRE)rpmioLinkPoolItem((rpmioItem)_mire, __FUNCTION__, __FILE__, __LINE__)

/**
 * Free pattern container.
 * @param mire		pattern container
 * @return		NULL on last derefernce
 */
/*@null@*/
miRE mireFree(/*@killref@*/ /*@only@*/ /*@null@*/ miRE mire)
        /*@globals fileSystem @*/
        /*@modifies mire, fileSystem @*/;
#define	mireFree(_mire)	\
    (miRE)rpmioFreePoolItem((rpmioItem)_mire, __FUNCTION__, __FILE__, __LINE__)

/**
 * Destroy compiled patterns.
 * @param mire		pattern array
 * @param nmire		no of patterns in array
 * @return		NULL always
 */
/*@null@*/
void * mireFreeAll(/*@only@*/ /*@null@*/ miRE mire, int nmire)
        /*@globals fileSystem @*/
        /*@modifies mire, fileSystem @*/;

/**
 * Create pattern container.
 * @param mode		type of pattern match
 * @param tag		identifier (e.g. an rpmTag)
 * @return		new pattern container (NULL on error)
 */
/*@null@*/
miRE mireNew(rpmMireMode mode, int tag)
        /*@globals fileSystem @*/
        /*@modifies fileSystem @*/;

/**
 * Initialize pattern compile options.
 * @param mire		pattern container
 * @param mode		type of pattern match
 * @param tag		identifier (e.g. an rpmTag)
 * @param options	pattern options (0 uses default options)
 * @param table		(PCRE only) locale tables
 * @return		0 on success
 */
int mireSetCOptions(miRE mire, rpmMireMode mode, int tag, int options,
		/*@null@*/ const unsigned char * table)
	/*@modifies mire @*/;

/**
 * Initialize pattern execute options (PCRE only).
 * @param mire		pattern container
 * @param *offsets	(PCRE only) string offset(s)
 * @param noffsets	(PCRE only) no. of string offsets
 * @return		0 on success
 */
int mireSetEOptions(miRE mire, /*@out@*/ /*@kept@*/ int * offsets, int noffsets)
	/*@modifies mire @*/;

/**
 * Initialize pattern global options (PCRE only).
 * @param newline	newline ending identifier
 * @param caseless	should case be ignored?
 * @param multiline	are multiline matches permitted?
 * @param utf8		assume utf8 matching?
 * @return		0 on success
 */
int mireSetGOptions(/*@null@*/ const char * newline,
		int caseless, int multiline, int utf8)
	/*globals _mireGLOBoptions, _mireREGEXoptions, _mirePCREoptions */
	/*modifies _mireGLOBoptions, _mireREGEXoptions, _mirePCREoptions */;

/**
 * Compile locale-specific PCRE tables.
 * @param mire		pattern container
 * @param locale	locale string (NULL uses usual envvar's)
 * @return		0 on success
 */
int mireSetLocale(/*@null@*/ miRE mire, /*@null@*/ const char * locale)
	/*@globals _mirePCREtables, internalState @*/
	/*@modifies mire, _mirePCREtables, internalState @*/;

/**
 * Compile pattern match.
 *
 * @param mire		pattern container
 * @param pattern	pattern to compile
 * @return		0 on success
 */
int mireRegcomp(miRE mire, const char * pattern)
	/*@modifies mire @*/;

/**
 * Execute pattern match.
 * @param mire		pattern container
 * @param val		value to match
 * @param vallen	length of value string (0 will use strlen)
 * @return		>=0 if pattern matches, -1 on nomatch, else error
 */
int mireRegexec(miRE mire, const char * val, size_t vallen)
	/*@modifies mire @*/;

/**
 * Append pattern to array.
 * @param mode		type of pattern match
 * @param tag		identifier (like an rpmTag)
 * @param pattern	pattern to compile
 * @param table		(PCRE) locale table to use (NULL uses default table)
 * @retval *mirep	pattern array
 * @retval *nmirep	no. of patterns in array
 */
/*@null@*/
int mireAppend(rpmMireMode mode, int tag, const char * pattern,
		/*@null@*/ const unsigned char * table,
		miRE * mirep, int * nmirep)
        /*@globals fileSystem @*/
        /*@modifies *mirep, *nmirep, fileSystem @*/;

/**
 * Load patterns from string array.
 * @param mode		type of pattern match
 * @param tag		identifier (like an rpmTag)
 * @param patterns	patterns to compile
 * @param table		(PCRE) locale table to use (NULL uses default table)
 * @retval *mirep	pattern array
 * @retval *nmirep	no. of patterns in array
 * @return		0 on success
 */
int mireLoadPatterns(rpmMireMode mode, int tag,
		/*@null@*/ const char ** patterns,
		/*@null@*/ const unsigned char * table,
		miRE * mirep, int * nmirep)
        /*@globals fileSystem @*/
        /*@modifies *mirep, *nmirep, fileSystem @*/;

/**
 * Apply array of patterns to a string.
 * @param mire		compiled pattern array
 * @param nmire		no. of patterns in array
 * @param s		string to apply against
 * @param slen		length of string (0 will use strlen(s))
 * @param rc		-1 == excluding, +1 == including, 0 == single pattern
 * @return		termination condition
 */
int mireApply(/*@null@*/ miRE mire, int nmire,
		const char *s, size_t slen, int rc)
	/*@modifies mire@*/;

/**
 * Study PCRE patterns (if any).
 * @param mire		pattern container
 * @param nmires	no. of patterns in container
 * @return		0 on success
 */
int mireStudy(miRE mire, int nmires)
	/*@modifies mire @*/;

#ifdef __cplusplus
}
#endif

#endif
