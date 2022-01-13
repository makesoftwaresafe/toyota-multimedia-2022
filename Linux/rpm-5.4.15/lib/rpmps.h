#ifndef H_RPMPS
#define H_RPMPS

/** \ingroup rpmps
 * \file lib/rpmps.h
 * Structures and prototypes used for an "rpmps" problem set.
 */

/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmps_debug;
/*@=exportlocal@*/

/**
 * Raw data for an element of a problem set.
 */
typedef /*@abstract@*/ struct rpmProblem_s * rpmProblem;

/**
 * Transaction problems found while processing a transaction set/
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmps_s * rpmps;

/**
*/
typedef /*@abstract@*/ struct rpmpsi_s * rpmpsi;

/**
 * @todo Generalize filter mechanism.
 */
typedef enum rpmprobFilterFlags_e {
    RPMPROB_FILTER_NONE		= 0,
    RPMPROB_FILTER_IGNOREOS	= (1 << 0),	/*!< from --ignoreos */
    RPMPROB_FILTER_IGNOREARCH	= (1 << 1),	/*!< from --ignorearch */
    RPMPROB_FILTER_REPLACEPKG	= (1 << 2),	/*!< from --replacepkgs */
    RPMPROB_FILTER_FORCERELOCATE= (1 << 3),	/*!< from --badreloc */
    RPMPROB_FILTER_REPLACENEWFILES= (1 << 4),	/*!< from --replacefiles */
    RPMPROB_FILTER_REPLACEOLDFILES= (1 << 5),	/*!< from --replacefiles */
    RPMPROB_FILTER_OLDPACKAGE	= (1 << 6),	/*!< from --oldpackage */
    RPMPROB_FILTER_DISKSPACE	= (1 << 7),	/*!< from --ignoresize */
    RPMPROB_FILTER_DISKNODES	= (1 << 8)	/*!< from --ignoresize */
} rpmprobFilterFlags;

/**
 * Enumerate transaction set problem types.
 */
typedef enum rpmProblemType_e {
    RPMPROB_BADARCH,	/*!< (unused) package ... is for a different architecture */
    RPMPROB_BADOS,	/*!< (unused) package ... is for a different operating system */
    RPMPROB_PKG_INSTALLED, /*!< package ... is already installed */
    RPMPROB_BADRELOCATE,/*!< path ... is not relocatable for package ... */
    RPMPROB_REQUIRES,	/*!< package ... has unsatisfied Requires: ... */
    RPMPROB_CONFLICT,	/*!< package ... has unsatisfied Conflicts: ... */
    RPMPROB_NEW_FILE_CONFLICT, /*!< file ... conflicts between attempted installs of ... */
    RPMPROB_FILE_CONFLICT,/*!< file ... from install of ... conflicts with file from package ... */
    RPMPROB_OLDPACKAGE,	/*!< package ... (which is newer than ...) is already installed */
    RPMPROB_DISKSPACE,	/*!< installing package ... needs ... on the ... filesystem */
    RPMPROB_DISKNODES,	/*!< installing package ... needs ... on the ... filesystem */
    RPMPROB_RDONLY,	/*!< installing package ... on ... rdonly filesystem */
    RPMPROB_BADPRETRANS,/*!< (unimplemented) */
    RPMPROB_BADPLATFORM,/*!< package ... is for a different platform */
    RPMPROB_NOREPACKAGE /*!< re-packaged package ... is missing */
 } rpmProblemType;

/**
 */
#if defined(_RPMPS_INTERNAL)
struct rpmProblem_s {
/*@only@*/ /*@null@*/
    char * pkgNEVR;
/*@only@*/ /*@null@*/
    char * altNEVR;
/*@exposed@*/ /*@null@*/
    fnpyKey key;
    rpmProblemType type;
    int ignoreProblem;
/*@only@*/ /*@null@*/
    char * str1;
    unsigned long long ulong1;
};

/**
 */
struct rpmps_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    int numProblems;		/*!< Current probs array size. */
    int numProblemsAlloced;	/*!< Allocated probs array size. */
    rpmProblem probs;		/*!< Array of specific problems. */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/**
 */
struct rpmpsi_s {
    int ix;
    rpmps ps;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return formatted string representation of a problem.
 * @param prob		rpm problem
 * @return		formatted string (malloc'd)
 */
/*@-exportlocal@*/
/*@-redecl@*/	/* LCL: is confused. */
/*@only@*/ extern const char * rpmProblemString(const rpmProblem prob)
	/*@*/;
/*@=redecl@*/
/*@=exportlocal@*/

/**
 * Unreference a problem set instance.
 * @param ps		problem set
 * @param msg
 * @return		problem set
 */
/*@unused@*/ /*@null@*/
rpmps rpmpsUnlink (/*@killref@*/ /*@returned@*/ rpmps ps,
		const char * msg)
	/*@modifies ps @*/;
#define	rpmpsUnlink(_ps, _msg)	\
	((rpmps)rpmioUnlinkPoolItem((rpmioItem)(_ps), _msg, __FILE__, __LINE__))

/**
 * Reference a problem set instance.
 * @param ps		transaction set
 * @param msg
 * @return		new transaction set reference
 */
/*@unused@*/ /*@newref@*/
rpmps rpmpsLink (rpmps ps, const char * msg)
	/*@modifies ps @*/;
#define	rpmpsLink(_ps, _msg)	\
	((rpmps)rpmioLinkPoolItem((rpmioItem)(_ps), _msg, __FILE__, __LINE__))

/**
 * Destroy a problem set.
 * @param ps		problem set
 * @return		NULL on last dereference
 */
/*@null@*/
rpmps rpmpsFree(/*@killref@*/ /*@only@*/ /*@null@*/ rpmps ps)
	/*@modifies ps @*/;
#define	rpmpsFree(_ps)	\
	((rpmps)rpmioFreePoolItem((rpmioItem)(_ps), __FUNCTION__, __FILE__, __LINE__))

/**
 * Return number of problems in set.
 * @param ps		problem set
 * @return		number of problems
 */
int rpmpsNumProblems(/*@null@*/ rpmps ps)
	/*@*/;

/**
 * Initialize problem set iterator.
 * @param ps		problem set
 * @return		problem set iterator
 */
rpmpsi rpmpsInitIterator(rpmps ps)
	/*@modifies ps @*/;

/**
 * Destroy problem set iterator.
 * @param psi		problem set iterator
 * @return		problem set iterator (NULL)
 */
rpmpsi rpmpsFreeIterator(/*@only@*/ rpmpsi psi)
	/*@modifies psi @*/;

/**
 * Return next problem set iterator index
 * @param psi		problem set iterator
 * @return		iterator index, -1 on termination
 */
int rpmpsNextIterator(rpmpsi psi)
	/*@modifies psi @*/;

/**
 * Return current problem from problem set
 * @param psi		problem set iterator
 * @return		current rpmProblem 
 */
/*@observer@*/
rpmProblem rpmpsProblem(rpmpsi psi)
	/*@*/;

/**
 * Create a problem set.
 * @return		new problem set
 */
rpmps rpmpsCreate(void)
	/*@*/;

/**
 * Print problems to file handle.
 * @param fp		file handle (NULL uses stderr)
 * @param ps		problem set
 */
void rpmpsPrint(/*@null@*/ FILE *fp, /*@null@*/ rpmps ps)
	/*@globals fileSystem @*/
	/*@modifies *fp, ps, fileSystem @*/;

/**
 * Append a problem to current set of problems.
 * @warning This function's args have changed, so the function cannot be
 * used portably
 * @param ps		problem set
 * @param type		type of problem
 * @param pkgNEVR	package name
 * @param key		filename or python object address
 * @param dn		directory name
 * @param bn		file base name
 * @param altNEVR	related (e.g. through a dependency) package name
 * @param ulong1	generic pointer/long attribute
 */
void rpmpsAppend(/*@null@*/ rpmps ps, rpmProblemType type,
		/*@null@*/ const char * pkgNEVR,
		/*@exposed@*/ /*@null@*/ fnpyKey key,
		/*@null@*/ const char * dn, /*@null@*/ const char * bn,
		/*@null@*/ const char * altNEVR,
		rpmuint64_t ulong1)
	/*@modifies ps @*/;

/**
 * Filter a problem set.
 *
 * As the problem sets are generated in an order solely dependent
 * on the ordering of the packages in the transaction, and that
 * ordering can't be changed, the problem sets must be parallel to
 * one another. Additionally, the filter set must be a subset of the
 * target set, given the operations available on transaction set.
 * This is good, as it lets us perform this trim in linear time, rather
 * then logarithmic or quadratic.
 *
 * @param ps		problem set
 * @param filter	problem filter (or NULL)
 * @return		0 no problems, 1 if problems remain
 */
int rpmpsTrim(/*@null@*/ rpmps ps, /*@null@*/ rpmps filter)
	/*@modifies ps @*/;

/**
 * Return a problem from problem set
 *
 * @param ps        problem set
 * @param num       problem number (<0 is last problem)
 * @return          rpmProblem, or NULL if error
 */
/*@exposed@*/
rpmProblem rpmpsGetProblem(/*@null@*/ rpmps ps, int num)
	/*@*/;

/**
 * Return the package NEVR causing the problem
 *
 * @param prob  rpm problem
 * @return      NEVR string ptr
 */
/*@null@*/ /*@exposed@*/
char * rpmProblemGetPkgNEVR(rpmProblem prob)
	/*@*/;

/**
 * Return the second package NEVR causing the problem
 *
 * @param prob  rpm problem
 * @return      NEVR string ptr, or NULL if unset
 */
/*@null@*/ /*@exposed@*/
char * rpmProblemGetAltNEVR(rpmProblem prob)
	/*@*/;

/**
 * Return a generic data string from a problem
 * @param prob		rpm problem
 * @return		a generic data string
 * @todo		needs a better name
 */
/*@null@*/ /*@exposed@*/
char * rpmProblemGetStr(rpmProblem prob)
	/*@*/;

/**
 * Return generic pointer/long attribute from a problem
 * @param prob		rpm problem
 * @return		a generic pointer/long attribute
 */
rpmuint64_t rpmProblemGetDiskNeed(rpmProblem prob)
	/*@*/;

/**
 * Return the problem type
 *
 * @param prob  rpm problem
 * @return      rpmProblemType
 */
rpmProblemType rpmProblemGetType(rpmProblem prob)
	/*@*/;

/**
 * Return the transaction key causing the problem
 *
 * @param prob  rpm problem
 * @return      fnpkey ptr if any or NULL
 */
/*@null@*/ /*@exposed@*/
fnpyKey rpmProblemKey(rpmProblem prob)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMPS */
