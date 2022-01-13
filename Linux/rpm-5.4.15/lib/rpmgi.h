#ifndef	H_RPMGI
#define	H_RPMGI

/** \ingroup rpmgi
 * \file lib/rpmgi.h
 */

#include <fts.h>
#include <argv.h>
#include <rpmtypes.h>
#include <rpmds.h>
#include <rpmte.h>
#include <rpmts.h>

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmgi_debug;
/*@=exportlocal@*/

/**
 */
typedef enum rpmgiFlags_e {
    RPMGI_NONE		= 0,
    RPMGI_TSADD		= (1 << 0),
    RPMGI_TSORDER	= (1 << 1),
    RPMGI_NOGLOB	= (1 << 2),
    RPMGI_NOMANIFEST	= (1 << 3),
    RPMGI_NOHEADER	= (1 << 4),
    RPMGI_ERASING	= (1 << 5)
} rpmgiFlags;

/**
 */
/*@unchecked@*/
extern rpmgiFlags giFlags;

#if defined(_RPMGI_INTERNAL)
/** \ingroup rpmgi
 */
struct rpmgi_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@refcounted@*/
    rpmts ts;			/*!< Iterator transaction set. */
    int (*tsOrder) (rpmts ts);	/*!< Iterator transaction ordering. */
    rpmTag tag;			/*!< Iterator type. */
/*@kept@*/ /*@relnull@*/
    const void * keyp;		/*!< Iterator key. */
    size_t keylen;		/*!< Iterator key length. */

    rpmgiFlags flags;		/*!< Iterator control bits. */
    int active;			/*!< Iterator is active? */
    int i;			/*!< Element index. */
/*@null@*/
    const char * hdrPath;	/*!< Path to current iterator header. */
/*@refcounted@*/ /*@null@*/
    Header h;			/*!< Current iterator header. */
    int rc;			/*!< Current exit code, specific to item. */

/*@null@*/
    rpmtsi tsi;

/*@null@*/
    rpmmi mi;

/*@refcounted@*/ /*@relnull@*/
    FD_t fd;

    ARGV_t argv;
    int argc;

    int ftsOpts;
/*@null@*/
    FTS * ftsp;
/*@relnull@*/
    FTSENT * fts;
/*@null@*/
    rpmRC (*walkPathFilter) (rpmgi gi);
/*@null@*/
    rpmRC (*stash) (rpmgi gi, Header h);

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** \name RPMGI */
/*@{*/

/**
 * Unreference a generalized iterator instance.
 * @param gi		generalized iterator
 * @param msg
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmgi rpmgiUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmgi gi,
		/*@null@*/ const char * msg)
	/*@modifies gi @*/;
#define	rpmgiUnlink(_gi, _msg)	\
    ((rpmgi)rpmioUnlinkPoolItem((rpmioItem)(_gi), _msg, __FILE__, __LINE__))

/**
 * Reference a generalized iterator instance.
 * @param gi		generalized iterator
 * @param msg
 * @return		new generalized iterator reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmgi rpmgiLink (/*@null@*/ rpmgi gi, /*@null@*/ const char * msg)
	/*@modifies gi @*/;
#define	rpmgiLink(_gi, _msg)	\
    ((rpmgi)rpmioLinkPoolItem((rpmioItem)(_gi), _msg, __FILE__, __LINE__))

/** Destroy a generalized iterator.
 * @param gi		generalized iterator
 * @return		NULL on last dereference
 */
/*@null@*/
rpmgi rpmgiFree(/*@killref@*/ /*@only@*/ /*@null@*/ rpmgi gi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
        /*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/;
#define	rpmgiFree(_gi)	\
    ((rpmgi)rpmioFreePoolItem((rpmioItem)(_gi), __FUNCTION__, __FILE__, __LINE__))

/**
 * Return a generalized iterator.
 * @param ts		transaction set
 * @param tag		rpm tag
 * @param keyp		key data (NULL for sequential access)
 * @param keylen	key data length (0 will use strlen(keyp))
 * @return		new iterator
 */
/*@null@*/
rpmgi rpmgiNew(rpmts ts, int tag, /*@kept@*/ /*@null@*/ const void * keyp,
		size_t keylen)
	/*@globals internalState @*/
	/*@modifies ts, internalState @*/;

/**
 * Perform next iteration step.
 * @param gi		generalized iterator
 * @return		RPMRC_OK on success, RPMRC_NOTFOUND on EOI
 */
rpmRC rpmgiNext(/*@null@*/ rpmgi gi)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
        /*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/;

/**
 * Return current iteration flags.
 * @param gi		generalized iterator
 * @return		flags
 */
rpmgiFlags rpmgiGetFlags(/*@null@*/ rpmgi gi)
        /*@*/;

/**
 * Return current header path.
 * @param gi		generalized iterator
 * @return		header path
 */
/*@observer@*/ /*@null@*/
const char * rpmgiHdrPath(/*@null@*/ rpmgi gi)
	/*@*/;

/**
 * Return current iteration header.
 * @param gi		generalized iterator
 * @return		header
 */
/*@null@*/
Header rpmgiHeader(/*@null@*/ rpmgi gi)
        /*@*/;

/**
 * Return current iteration transaction set.
 * @param gi		generalized iterator
 * @return		transaction set
 */
/*@null@*/
rpmts rpmgiTs(/*@null@*/ rpmgi gi)
        /*@*/;

/**
 * Return current iteration item(s) exit code.
 * @param gi		generalized iterator
 * @return		transaction set
 */
/*@null@*/
int rpmgiRc(/*@null@*/ rpmgi gi)
        /*@*/;

/**
 * Escape isspace(3) characters in string.
 * @param s		string
 * @return		escaped string
 */
const char * rpmgiEscapeSpaces(const char * s)
	/*@*/;

/**
 * Load iterator args.
 * @param gi		generalized iterator
 * @param argv		arg list
 * @param ftsOpts	fts(3) flags
 * @param flags		iterator flags
 * @return		RPMRC_OK on success
 */
rpmRC rpmgiSetArgs(/*@null@*/ rpmgi gi, /*@null@*/ ARGV_t argv,
		int ftsOpts, rpmgiFlags flags)
	/*@globals internalState @*/
	/*@modifies gi, internalState @*/;

/**
 * Return header from package.
 * @param gi		generalized iterator
 * @param path		file path
 * @return		header (NULL on failure)
 */
/*@null@*/
Header rpmgiReadHeader(rpmgi gi, const char * path)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies gi, rpmGlobalMacroContext, h_errno, internalState @*/;

/*@}*/

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMGI */
