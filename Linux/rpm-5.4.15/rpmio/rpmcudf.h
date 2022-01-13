#ifndef RPMCUDF_H
#define RPMCUDF_H

/** \ingroup rpmio
 * \file rpmio/rpmcudf.h
 */

#include <rpmiotypes.h>
#include <rpmio.h>

typedef /*@abstract@*/ /*@refcounted@*/ struct rpmcudf_s * rpmcudf;
typedef	/*@abstract@*/ struct rpmcudp_s * rpmcudp;
typedef /*@abstract@*/ struct rpmcudv_s * rpmcudv;

/*@unchecked@*/
extern int _rpmcudf_debug;

#ifdef	NOTYET
/*@unchecked@*/ /*@relnull@*/
extern rpmcudf _rpmcudfI;
#endif

#if defined(_RPMCUDF_INTERNAL)
#if defined(WITH_CUDF)
#include <cudf.h>
#else
typedef void *cudf_preamble_t;  /* preamble of a CUDF document */
typedef void *cudf_request_t;   /* request of a CUDF document */
typedef void *cudf_universe_t;  /* package universe (i.e. all known packages) */
typedef void *cudf_package_t;   /* single package from the universe */

typedef	struct _GList GList;
typedef	struct _GHashTable GHashTable;

typedef GList *cudf_packages_t;
typedef struct __cudf_doc cudf_doc_t;
typedef struct __cudf cudf_t;
typedef struct __cudf_vpkg cudf_vpkg_t;
typedef GList *cudf_vpkglist_t;
typedef GHashTable *cudf_extra_t;
typedef GList *cudf_vpkgformula_t;
typedef struct __cudf_value cudf_value_t;

#endif

/* XXX Extend cudf_value_t to include additional types. */

#define	RPMCUDV_EBASE	256
enum rpmcudv_e {
    RPMCUDV_NOTYPE	= 0,			/* TYPE_NOTYPE */
    RPMCUDV_INT		= 1,			/* TYPE_INT */
    RPMCUDV_POSINT	= 2,			/* TYPE_POSINT */
    RPMCUDV_NAT		= 3,			/* TYPE_NAT */
    RPMCUDV_BOOL	= 4,			/* TYPE_BOOL */
    RPMCUDV_STRING	= 5,			/* TYPE_STRING */
    RPMCUDV_ENUM	= 6,			/* TYPE_ENUM */
    RPMCUDV_PKGNAME	= 7,			/* TYPE_PKGNAME */
    RPMCUDV_IDENT	= 8,			/* TYPE_IDENT */
    RPMCUDV_VPKG	= 9,			/* TYPE_VPKG */
    RPMCUDV_VPKGFORMULA	= 10,			/* TYPE_VPKGFORMULA */
    RPMCUDV_VPKGLIST	= 11,			/* TYPE_VPKGLIST */
    RPMCUDV_VEQPKG	= 12,			/* TYPE_VEQPKG */
    RPMCUDV_VEQPKGLIST	= 13,			/* TYPE_VEQPKGLIST */
    RPMCUDV_TYPEDECL	= 14,			/* TYPE_TYPEDECL */

	/* XXX extensions */
    RPMCUDV_PACKAGE	= RPMCUDV_EBASE+0,	/* cudf_package_t */

    RPMCUDV_CUDFDOC	= RPMCUDV_EBASE+1,	/* cudf_doc_t */
    RPMCUDV_CUDF	= RPMCUDV_EBASE+2,	/* cudf_t */

    RPMCUDV_EXTRA	= RPMCUDV_EBASE+3,	/* cudf_extra_t */

};

union rpmcudv_u {
    int i;
    char *s;
    cudf_vpkg_t *vpkg;
    cudf_vpkgformula_t f;
    cudf_vpkglist_t vpkgs;

    void * ptr;
    cudf_preamble_t preamble;
    cudf_request_t request;

    cudf_package_t *pkg;

    cudf_doc_t *doc;
    cudf_t *cudf;

    cudf_extra_t extra;
};

struct rpmcudv_s {
    enum rpmcudv_e typ;
    union rpmcudv_u val;
};

struct rpmcudp_s {
    GList * l;
    struct rpmcudv_s V;
    struct rpmcudv_s W;
};

struct rpmcudf_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct rpmcudv_s V;		/*!< union of cudf_doc_t and cudf_t */
    rpmiob iob;			/*!< output collector */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif /* _RPMCUDF_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a cudf interpreter instance.
 * @param cudf	cudf interpreter
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmcudf rpmcudfUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmcudf cudf)
	/*@modifies cudf @*/;
#define	rpmcudfUnlink(_cudf)	\
    ((rpmcudf)rpmioUnlinkPoolItem((rpmioItem)(_cudf), __FUNCTION__, __FILE__, __LINE__))

/**
 * Reference a cudf interpreter instance.
 * @param cudf	cudf interpreter
 * @return		new cudf interpreter reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmcudf rpmcudfLink (/*@null@*/ rpmcudf cudf)
	/*@modifies cudf @*/;
#define	rpmcudfLink(_cudf)	\
    ((rpmcudf)rpmioLinkPoolItem((rpmioItem)(_cudf), __FUNCTION__, __FILE__, __LINE__))

/**
 * Destroy a cudf interpreter.
 * @param cudf		cudf interpreter
 * @return		NULL on last dereference
 */
/*@null@*/
rpmcudf rpmcudfFree(/*@killref@*/ /*@null@*/rpmcudf cudf)
	/*@globals fileSystem @*/
	/*@modifies cudf, fileSystem @*/;
#define	rpmcudfFree(_cudf)	\
    ((rpmcudf)rpmioFreePoolItem((rpmioItem)(_cudf), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a cudf interpreter.
 * @param *av		cudf interpreter args (or NULL)
 * @param flags		cudf interpreter flags ((1<<31) == use global interpreter)
 * @return		new cudf interpreter
 */
/*@newref@*/ /*@null@*/
rpmcudf rpmcudfNew(/*@null@*/ char ** av, uint32_t flags)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

int rpmcudfHasPreamble(rpmcudf cudf)
	/*@*/;
int rpmcudfHasRequest(rpmcudf cudf)
	/*@*/;
int rpmcudfIsConsistent(rpmcudf cudf)
	/*@*/;
int rpmcudfInstalledSize(rpmcudf cudf)
	/*@*/;
int rpmcudfUniverseSize(rpmcudf cudf)
	/*@*/;
void rpmcudfPrintPreamble(rpmcudf cudf)
	/*@*/;
void rpmcudfPrintRequest(rpmcudf cudf)
	/*@*/;
void rpmcudfPrintUniverse(rpmcudf cudf)
	/*@*/;
int rpmcudfIsSolution(rpmcudf X, rpmcudf Y)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMCUDF_H */
