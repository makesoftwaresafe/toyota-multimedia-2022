#ifndef H_RPMWF
#define H_RPMWF

/**
 * \file rpmdb/rpmwf.h
 * Structure(s)and methods for a archive wrapper format (e.g. XAR).
 */

#include <rpmxar.h>

/*@unchecked@*/
extern int _rpmwf_debug;

/**
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmwf_s * rpmwf;

#ifdef	_RPMWF_INTERNAL
struct rpmwf_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@relnull@*/
    const char * fn;
/*@relnull@*/
    FD_t fd;
/*@relnull@*/ /*@owned@*/
    void * b;
    size_t nb;
/*@relnull@*/ /*@dependent@*/
    char * l;
    size_t nl;
/*@relnull@*/ /*@dependent@*/
    char * s;
    size_t ns;
/*@relnull@*/ /*@dependent@*/
    char * h;
    size_t nh;
/*@relnull@*/ /*@dependent@*/
    char * p;
    size_t np;
/*@relnull@*/ /*@refcounted@*/
    rpmxar xar;
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

rpmRC rpmwfPushXAR(rpmwf wf, const char * fn)
	/*@globals fileSystem @*/
	/*@modifies wf, fileSystem @*/;

rpmRC rpmwfPullXAR(rpmwf wf, const char * fn)
	/*@globals fileSystem @*/
	/*@modifies wf, fileSystem @*/;

rpmRC rpmwfFini(rpmwf wf)
	/*@globals fileSystem, internalState @*/
	/*@modifies wf, fileSystem, internalState @*/;

rpmRC rpmwfInit(rpmwf wf, const char * fn, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies wf, fileSystem, internalState @*/;

rpmRC rpmwfPushRPM(rpmwf wf, const char * fn)
	/*@globals fileSystem @*/
	/*@modifies wf, fileSystem @*/;

/**
 * Unreference a wrapper format instance.
 * @param wf		wrapper format
 * @param msg
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmwf rpmwfUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmwf wf,
		/*@null@*/ const char * msg)
	/*@modifies wf @*/;
#define	rpmwfUnlink(_wf, _msg)	\
    ((rpmwf)rpmioUnlinkPoolItem((rpmioItem)(_wf), _msg, __FILE__, __LINE__))

/**
 * Reference a wrapper format instance.
 * @param wf		wrapper format
 * @param msg
 * @return		new wrapper format reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmwf rpmwfLink (/*@null@*/ rpmwf wf, /*@null@*/ const char * msg)
	/*@modifies wf @*/;
#define	rpmwfLink(_wf, _msg)	\
    ((rpmwf)rpmioLinkPoolItem((rpmioItem)(_wf), _msg, __FILE__, __LINE__))

/*@null@*/
rpmwf rpmwfFree(/*@only@*/ rpmwf wf)
	/*@globals fileSystem, internalState @*/
	/*@modifies wf, fileSystem, internalState @*/;
#define	rpmwfFree(_wf)	\
    ((rpmwf)rpmioFreePoolItem((rpmioItem)(_wf), __FUNCTION__, __FILE__, __LINE__))

/*@relnull@*/
rpmwf rpmwfNew(const char * fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/*@relnull@*/
rpmwf rdRPM(const char * rpmfn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/*@relnull@*/
rpmwf rdXAR(const char * xarfn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

rpmRC wrXAR(const char * xarfn, rpmwf wf)
	/*@globals fileSystem @*/
	/*@modifies wf, fileSystem @*/;

rpmRC wrRPM(const char * rpmfn, rpmwf wf)
	/*@globals fileSystem, internalState @*/
	/*@modifies wf, fileSystem, internalState @*/;


#ifdef __cplusplus
}
#endif

#endif  /* H_RPMWF */
