#ifndef	H_RPMSM
#define	H_RPMSM

/** \ingroup rpmio
 * \file rpmio/rpmsm.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@refcounted@*/ struct rpmsm_s * rpmsm;

/*@unchecked@*/
extern int _rpmsm_debug;

/*@unchecked@*/ /*@relnull@*/
extern rpmsm _rpmsmI;

/**
 * Bit field enum for semamange/semodule flags.
 */
enum rpmsmFlags_e {
    RPMSM_FLAGS_NONE	= 0,
    RPMSM_FLAGS_BASE	= (1 <<  1),	/* -b,--base ... */
    RPMSM_FLAGS_INSTALL	= (1 <<  2),	/* -i,--install ... */
    RPMSM_FLAGS_LIST	= (1 <<  3),	/* -l,--list-modules ... */
    RPMSM_FLAGS_REMOVE	= (1 <<  4),	/* -r,--remove ... */
    RPMSM_FLAGS_UPGRADE	= (1 <<  5),	/* -u,--upgrade ... */
    RPMSM_FLAGS_RELOAD	= (1 <<  6),	/* -R,--reload ... */
    RPMSM_FLAGS_REBUILD	= (1 <<  7),	/* -B,--build ... */
    RPMSM_FLAGS_NOAUDIT	= (1 <<  8),	/* -D,--disable_dontaudit ... */
    RPMSM_FLAGS_COMMIT	= (1 <<  9),
    RPMSM_FLAGS_CREATE	= (1 << 10),
    RPMSM_FLAGS_CONNECT	= (1 << 11),
    RPMSM_FLAGS_SELECT	= (1 << 12),
    RPMSM_FLAGS_ACCESS	= (1 << 13),
    RPMSM_FLAGS_BEGIN	= (1 << 14),
};

#if defined(_RPMSM_INTERNAL)
/** \ingroup rpmio
 */
struct rpmsm_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    char * fn;			/*!< policy store (i.e. "targeted") */
    unsigned int flags;
    unsigned int state;
    unsigned int access;	/*!< access 1: readable 2: writable */
    const char ** av;
    void * I;			/*!< semanage_handle_t */
    rpmiob iob;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMSM_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a semanage wrapper instance.
 * @param sm		semanage wrapper
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmsm rpmsmUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmsm sm)
	/*@modifies sm @*/;
#define	rpmsmUnlink(_sm)	\
    ((rpmsm)rpmioUnlinkPoolItem((rpmioItem)(_sm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a semanage wrapper instance.
 * @param sm		semanage wrapper
 * @return		new semanage wrapper reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmsm rpmsmLink (/*@null@*/ rpmsm sm)
	/*@modifies sm @*/;
#define	rpmsmLink(_sm)	\
    ((rpmsm)rpmioLinkPoolItem((rpmioItem)(_sm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a semanage wrapper.
 * @param sm		semanage wrapper
 * @return		NULL on last dereference
 */
/*@null@*/
rpmsm rpmsmFree(/*@killref@*/ /*@null@*/rpmsm sm)
	/*@globals fileSystem @*/
	/*@modifies sm, fileSystem @*/;
#define	rpmsmFree(_sm)	\
    ((rpmsm)rpmioFreePoolItem((rpmioItem)(_sm), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a semanage wrapper.
 * @param fn		semanage policy store (i.e. "targeted")
 * @param flags		semanage flags
 * @return		new semanage wrapper
 */
/*@newref@*/ /*@null@*/
rpmsm rpmsmNew(/*@null@*/ const char * fn, unsigned int flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;


/**
 * Run semanage commands.
 *
 * Commands are keyword strings with appended optional argument.
 *
 * @param sm		semanage wrapper
 * @param av		semanage commands
 * @retval *resultp	string result (malloc'd)
 * @return		RPMRC_OK on success, RPMRC_FAIL on error
 */
rpmRC rpmsmRun(rpmsm sm, char ** av, /*@out@*/ const char ** resultp)
	/*@globals fileSystem, internalState @*/
	/*@modifies sm, *resultp, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMSM */
