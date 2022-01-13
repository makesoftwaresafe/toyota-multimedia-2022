#ifndef H_RPMTE
#define H_RPMTE

/** \ingroup rpmts rpmte
 * \file lib/rpmte.h
 * Structures used for an "rpmte" transaction element.
 */
#include <rpmfi.h>

/**
 */
/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmte_debug;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Transaction element ordering chain linkage.
 */
typedef /*@abstract@*/ struct tsortInfo_s *		tsortInfo;

/** \ingroup rpmte
 * Transaction element iterator.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmtsi_s *rpmtsi;

/** \ingroup rpmte
 * Transaction element type.
 */
typedef enum rpmElementType_e {
    TR_ADDED		= (1 << 0),	/*!< Package will be installed. */
    TR_REMOVED		= (1 << 1)	/*!< Package will be removed. */
} rpmElementType;

#if	defined(_RPMTE_INTERNAL)
#include <argv.h>
#include <rpmtxn.h>
#include <rpmal.h>

typedef struct relation_s * relation;

#if defined(_RPMTS_ORDER_INTERNAL)
struct relation_s {
    rpmte   rel_suc;  // pkg requiring this package
    rpmsenseFlags rel_flags; // accumulated flags of the requirements
    struct relation_s * rel_next;
};
#endif

/** \ingroup rpmte
 * Dependncy ordering information.
 */
/*@-fielduse@*/	/* LCL: confused by union? */
struct tsortInfo_s {
/*@owned@*/ /*@null@*/
    tsortInfo	tsi_next;
/*@exposed@*/ /*@dependent@*/ /*@null@*/
    rpmte	tsi_chain;
    int		tsi_tagn;
    int		tsi_queued;

    int		tsi_count;
    int		tsi_qcnt;
    int		tsi_reqx;
    relation	tsi_relations;
    relation	tsi_forward_relations;
    rpmte	tsi_suc;
    int		tsi_SccIdx;
    int		tsi_SccLowlink;
};
/*@=fielduse@*/

/** \ingroup rpmte
 * Info used to link transaction element upgrade/rollback side effects.
 */
struct rpmChainLink_s {
/*@only@*/ /*@null@*/
    ARGV_t Pkgid;		/*!< link element pkgid's. */
/*@only@*/ /*@null@*/
    ARGV_t Hdrid;		/*!< link element hdrid's. */
/*@only@*/ /*@null@*/
    ARGV_t NEVRA;		/*!< link element NEVRA's. */
};

/** \ingroup rpmte
 */
typedef struct sharedFileInfo_s *		sharedFileInfo;

/** \ingroup rpmte
 * Replaced file cross reference.
 */
struct sharedFileInfo_s {
    rpmuint32_t pkgFileNum;
    rpmuint32_t otherFileNum;
    rpmuint32_t otherPkg;
    rpmuint32_t isRemoved;
};

/** \ingroup rpmte
 * A single package instance to be installed/removed atomically.
 */
struct rpmte_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmElementType type;	/*!< Package disposition (installed/removed). */

/*@refcounted@*/ /*@relnull@*/
    Header h;			/*!< Package header. */
/*@only@*/
    const char * NEVR;		/*!< Package name-version-release. */
/*@only@*/
    const char * NEVRA;		/*!< Package name-version-release.arch. */
/*@only@*/ /*@relnull@*/
    const char * pkgid;		/*!< Package identifier (header+payload md5). */
/*@only@*/ /*@relnull@*/
    const char * hdrid;		/*!< Package header identifier (header sha1). */
/*@only@*/ /*@null@*/
    const char * sourcerpm;	/*!< Source package. */
/*@only@*/
    const char * name;		/*!< Name: */
/*@only@*/ /*@null@*/
    char * epoch;
/*@only@*/ /*@null@*/
    char * version;		/*!< Version: */
/*@only@*/ /*@null@*/
    char * release;		/*!< Release: */
#ifdef	RPM_VENDOR_MANDRIVA
/*@only@*/ /*@null@*/
    char * distepoch;
#endif
/*@only@*/ /*@null@*/
    const char * arch;		/*!< Architecture hint. */
/*@only@*/ /*@null@*/
    const char * os;		/*!< Operating system hint. */
    int isSource;		/*!< (TR_ADDED) source rpm? */

    rpmte parent;		/*!< Parent transaction element. */
    int degree;			/*!< No. of immediate children. */
    int npreds;			/*!< No. of predecessors. */
    int tree;			/*!< Tree index. */
    int depth;			/*!< Depth in dependency tree. */
    int breadth;		/*!< Breadth in dependency tree. */
    uint32_t db_instance;	/*!< Database Instance after add */
/*@owned@*/
    tsortInfo tsi;		/*!< Dependency ordering chains. */

/*@null@*/
    rpmPRCO PRCO;		/*!< Current dependencies. */

/*@null@*/
    rpmtxn txn;			/*!< Package transaction pointer. */
/*@refcounted@*/ /*@null@*/
    rpmfi fi;			/*!< File information. */
#ifdef	REFERENCE
    rpmpol pol;			/*!< Policies */
#endif

    rpmuint32_t depFlags;	/*!< Package depFlags mask. */
    rpmuint32_t transFlags;	/*!< Package transFlags mask. */
    rpmuint32_t color;		/*!< Color bit(s) from package dependencies. */
    rpmuint32_t pkgFileSize;	/*!< No. of bytes in package file (approx). */

/*@exposed@*/ /*@dependent@*/ /*@null@*/
    fnpyKey key;		/*!< (TR_ADDED) Retrieval key. */
/*@owned@*/ /*@null@*/
    rpmRelocation relocs;	/*!< (TR_ADDED) Payload file relocations. */
    int nrelocs;		/*!< (TR_ADDED) No. of relocations. */
    int autorelocatex;		/*!< (TR_ADDED) Auto relocation entry index. */
/*@refcounted@*/ /*@null@*/	
    FD_t fd;			/*!< (TR_ADDED) Payload file descriptor. */

/*@owned@*/ /*@null@*/
    sharedFileInfo replaced;	/*!< (TR_ADDED) Replaced file reference. */
    int nreplaced;		/*!< (TR_ADDED) No. of replaced files. */

    struct rpmChainLink_s blink;/*!< Backward link info to erased element. */
    struct rpmChainLink_s flink;/*!< Forward link info to installed element. */
    int linkFailed;		/*!< Did the linked element upgrade succeed? */
    int done;			/*!< Has the element been installed/erased? */
    rpmuint32_t originTid[2];	/*!< Transaction id of first install. */
    rpmuint32_t originTime[2];	/*!< Time that package was first installed. */

    int installed;		/*!< Was the header installed? */
    int downgrade;		/*!< Adjust package count on downgrades. */

    struct {
/*@exposed@*/ /*@dependent@*/ /*@null@*/
	alKey addedKey;
	struct {
/*@exposed@*/ /*@dependent@*/ /*@null@*/
	    alKey dependsOnKey;
	    uint32_t dboffset;
	} removed;
    } u;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/** \ingroup rpmte
 * Iterator across transaction elements, forward on install, backward on erase.
 */
struct rpmtsi_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@refcounted@*/
    rpmts ts;		/*!< transaction set. */
    int reverse;	/*!< reversed traversal? */
    int ocsave;		/*!< last returned iterator index. */
    int oc;		/*!< iterator index. */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

int rpmteClose(rpmte te, rpmts ts, int reset_fi)
	/*@modifies te, ts @*/;
Header rpmteDBHeader(rpmts ts, uint32_t rec)
	/*@modifies ts @*/;
Header rpmteFDHeader(rpmts ts, rpmte te)
	/*@modifies ts, te @*/;
int rpmteOpen(rpmte te, rpmts ts, int reload_fi)
	/*@modifies te, ts @*/;

/** \ingroup rpmte
 * Return failed status of transaction element.
 * @param te		transaction element
 * @return		1 if transaction element (or its parents) failed
 */
int rpmteFailed(rpmte te)
	/*@*/;

int rpmteHaveTransScript(rpmte te, rpmTag tag)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* _RPMTE_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmte
 * Destroy a transaction element.
 * @param te		transaction element
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmte rpmteFree(/*@only@*/ /*@null@*/ rpmte te)
	/*@globals fileSystem @*/
	/*@modifies te, fileSystem @*/;
#define rpmteFree(_te)  \
    ((rpmte) rpmioFreePoolItem((rpmioItem)(_te), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmte
 * Create a transaction element.
 * @param ts		transaction set
 * @param h		header
 * @param type		TR_ADDED/TR_REMOVED
 * @param key		(TR_ADDED) package retrieval key (e.g. file name)
 * @param relocs	(TR_ADDED) package file relocations
 * @param dboffset	(TR_REMOVED) rpmdb instance
 * @param pkgKey	associated added package (if any)
 * @return		new transaction element
 */
/*@only@*/ /*@null@*/
rpmte rpmteNew(const rpmts ts, Header h, rpmElementType type,
		/*@exposed@*/ /*@dependent@*/ /*@null@*/ fnpyKey key,
		/*@null@*/ rpmRelocation relocs,
		uint32_t dboffset,
		/*@exposed@*/ /*@dependent@*/ /*@null@*/ alKey pkgKey)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmte
 * Retrieve header from transaction element.
 * @param te		transaction element
 * @return		header
 */
extern Header rpmteHeader(rpmte te)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Save header into transaction element.
 * @param te		transaction element
 * @param h		header
 * @return		NULL always
 */
extern Header rpmteSetHeader(rpmte te, Header h)
	/*@modifies te, h @*/;

/** \ingroup rpmte
 * Retrieve type of transaction element.
 * @param te		transaction element
 * @return		type
 */
rpmElementType rpmteType(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve name string of transaction element.
 * @param te		transaction element
 * @return		name string
 */
/*@observer@*/
extern const char * rpmteN(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve epoch string of transaction element.
 * @param te		transaction element
 * @return		epoch string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteE(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve version string of transaction element.
 * @param te		transaction element
 * @return		version string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteV(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve release string of transaction element.
 * @param te		transaction element
 * @return		release string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteR(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve distepoch string of transaction element.
 * @param te		transaction element
 * @return		distepoch string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteD(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve arch string of transaction element.
 * @param te		transaction element
 * @return		arch string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteA(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve os string of transaction element.
 * @param te		transaction element
 * @return		os string
 */
/*@observer@*/ /*@null@*/
extern const char * rpmteO(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve isSource attribute of transaction element.
 * @param te		transaction element
 * @return		isSource attribute
 */
extern int rpmteIsSource(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve color bits of transaction element.
 * @param te		transaction element
 * @return		color bits
 */
rpmuint32_t rpmteColor(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set color bits of transaction element.
 * @param te		transaction element
 * @param color		new color bits
 * @return		previous color bits
 */
rpmuint32_t rpmteSetColor(rpmte te, rpmuint32_t color)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve last instance installed to the database.
 * @param te		transaction element
 * @return		last install instance.
 */
uint32_t rpmteDBInstance(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set last instance installed to the database.
 * @param te		transaction element
 * @param instance	Database instance of last install element.
 * @return		last install instance.
 */
void rpmteSetDBInstance(rpmte te, unsigned int instance)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve size in bytes of package file.
 * @todo Signature header is estimated at 256b.
 * @param te		transaction element
 * @return		size in bytes of package file.
 */
rpmuint32_t rpmtePkgFileSize(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve transaction start time that package was first installed.
 * @param te		transaction element
 * @return		origin time
 */
/*@observer@*/
rpmuint32_t * rpmteOriginTid(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve time that package was first installed.
 * @param te		transaction element
 * @return		origin time
 */
/*@observer@*/
rpmuint32_t * rpmteOriginTime(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve dependency tree depth of transaction element.
 * @param te		transaction element
 * @return		depth
 */
int rpmteDepth(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set dependency tree depth of transaction element.
 * @param te		transaction element
 * @param ndepth	new depth
 * @return		previous depth
 */
int rpmteSetDepth(rpmte te, int ndepth)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve dependency tree breadth of transaction element.
 * @param te		transaction element
 * @return		breadth
 */
int rpmteBreadth(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set dependency tree breadth of transaction element.
 * @param te		transaction element
 * @param nbreadth	new breadth
 * @return		previous breadth
 */
int rpmteSetBreadth(rpmte te, int nbreadth)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve tsort no. of predecessors of transaction element.
 * @param te		transaction element
 * @return		no. of predecessors
 */
int rpmteNpreds(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set tsort no. of predecessors of transaction element.
 * @param te		transaction element
 * @param npreds	new no. of predecessors
 * @return		previous no. of predecessors
 */
int rpmteSetNpreds(rpmte te, int npreds)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve tree index of transaction element.
 * @param te		transaction element
 * @return		tree index
 */
int rpmteTree(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set tree index of transaction element.
 * @param te		transaction element
 * @param ntree		new tree index
 * @return		previous tree index
 */
int rpmteSetTree(rpmte te, int ntree)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve parent transaction element.
 * @param te		transaction element
 * @return		parent transaction element
 */
/*@observer@*/ /*@unused@*/
rpmte rpmteParent(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set parent transaction element.
 * @param te		transaction element
 * @param pte		new parent transaction element
 * @return		previous parent transaction element
 */
/*@null@*/
rpmte rpmteSetParent(rpmte te, rpmte pte)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve number of children of transaction element.
 * @param te		transaction element
 * @return		tree index
 */
int rpmteDegree(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set number of children of transaction element.
 * @param te		transaction element
 * @param ndegree	new number of children
 * @return		previous number of children
 */
int rpmteSetDegree(rpmte te, int ndegree)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Retrieve tsort info for transaction element.
 * @param te		transaction element
 * @return		tsort info
 */
tsortInfo rpmteTSI(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Destroy tsort info of transaction element.
 * @param te		transaction element
 */
void rpmteFreeTSI(rpmte te)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Initialize tsort info of transaction element.
 * @param te		transaction element
 */
void rpmteNewTSI(rpmte te)
	/*@modifies te @*/;

/** \ingroup rpmte
 * Destroy dependency set info of transaction element.
 * @param te		transaction element
 */
/*@unused@*/
void rpmteCleanDS(rpmte te)
	/*@modifies te @*/;

#if	defined(_RPMTE_INTERNAL)
/** \ingroup rpmte
 * Retrieve pkgKey of TR_ADDED transaction element.
 * @param te		transaction element
 * @return		pkgKey
 */
/*@exposed@*/ /*@dependent@*/ /*@null@*/
alKey rpmteAddedKey(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Set pkgKey of TR_ADDED transaction element.
 * @param te		transaction element
 * @param npkgKey	new pkgKey
 * @return		previous pkgKey
 */
/*@exposed@*/ /*@dependent@*/ /*@null@*/
alKey rpmteSetAddedKey(rpmte te,
		/*@exposed@*/ /*@dependent@*/ /*@null@*/ alKey npkgKey)
	/*@modifies te @*/;
#endif	/* _RPMTE_INTERNAL */

/** \ingroup rpmte
 * Retrieve rpmdb instance of TR_REMOVED transaction element.
 * @param te		transaction element
 * @return		rpmdb instance
 */
int rpmteDBOffset(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve name-version-release string from transaction element.
 * @param te		transaction element
 * @return		name-version-release string
 */
/*@observer@*/
extern const char * rpmteNEVR(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve name-version-release.arch string from transaction element.
 * @param te		transaction element
 * @return		name-version-release.arch string
 */
/*@-exportlocal@*/
/*@observer@*/
extern const char * rpmteNEVRA(rpmte te)
	/*@*/;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Retrieve pkgid string from transaction element.
 * @param te		transaction element
 * @return		pkgid string
 */
/*@-exportlocal@*/
/*@observer@*/ /*@null@*/
extern const char * rpmtePkgid(rpmte te)
	/*@*/;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Retrieve hdrid string from transaction element.
 * @param te		transaction element
 * @return		hdrid string
 */
/*@-exportlocal@*/
/*@observer@*/ /*@null@*/
extern const char * rpmteHdrid(rpmte te)
	/*@*/;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Retrieve sourcerpm string from transaction element.
 * @param te		transaction element
 * @return		sourcerpm string
 */
/*@-exportlocal@*/
/*@observer@*/ /*@null@*/
extern const char * rpmteSourcerpm(rpmte te)
	/*@*/;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Retrieve file handle from transaction element.
 * @param te		transaction element
 * @return		file handle
 */
FD_t rpmteFd(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve key from transaction element.
 * @param te		transaction element
 * @return		key
 */
/*@exposed@*/
fnpyKey rpmteKey(rpmte te)
	/*@*/;

/** \ingroup rpmte
 * Retrieve dependency tag set from transaction element.
 * @param te		transaction element
 * @param tag		dependency tag
 * @return		dependency tag set
 */
rpmds rpmteDS(rpmte te, rpmTag tag)
	/*@*/;

/** \ingroup rpmte
 * Retrieve file info tag set from transaction element.
 * @param te		transaction element
 * @param tag		file info tag (RPMTAG_BASENAMES)
 * @return		file info tag set
 */
rpmfi rpmteFI(rpmte te, rpmTag tag)
	/*@*/;
/*@null@*/
rpmfi rpmteSetFI(rpmte te, rpmfi fi)
	/*@modifies te, fi @*/;

/** \ingroup rpmte
 * Calculate transaction element dependency colors/refs from file info.
 * @param te		transaction element
 * @param tag		dependency tag (RPMTAG_PROVIDENAME, RPMTAG_REQUIRENAME)
 */
/*@-exportlocal@*/
void rpmteColorDS(rpmte te, rpmTag tag)
        /*@modifies te @*/;
/*@=exportlocal@*/

/** \ingroup rpmte
 * Chain p <-> q forward/backward transaction element links.
 * @param p		installed element (needs backward link)
 * @param q		erased element (needs forward link)
 * @param oh		erased element header
 * @param msg		operation identifier for debugging (NULL uses "")
 * @return		0 on success
 */
int rpmteChain(rpmte p, rpmte q, Header oh, /*@null@*/ const char * msg)
	/*@globals internalState @*/
	/*@modifies p, q, oh, internalState @*/;

#define	RPMTE_CHAIN_END	"CHAIN END"	/*!< End of chain marker. */

/** \ingroup rpmte
 * Return transaction element index.
 * @param tsi		transaction element iterator
 * @return		transaction element index
 */
int rpmtsiOc(rpmtsi tsi)
	/*@*/;

/** \ingroup rpmte
 * Destroy transaction element iterator.
 * @param tsi		transaction element iterator
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmtsi rpmtsiFree(/*@killref@*//*@null@*/ rpmtsi tsi)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
#define	rpmtsiFree(_tsi)	\
	((rpmtsi)rpmioFreePoolItem((rpmioItem)(_tsi), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmte
 * Create transaction element iterator.
 * @param ts		transaction set
 * @return		transaction element iterator
 */
/*@unused@*/ /*@only@*/
rpmtsi rpmtsiInit(rpmts ts)
	/*@modifies ts @*/;

/** \ingroup rpmte
 * Create transaction element iterator.
 * @param ts		transaction set
 * @param fn
 * @param ln
 * @return		transaction element iterator
 */
/*@unused@*/ /*@only@*/
rpmtsi XrpmtsiInit(rpmts ts,
		const char * fn, unsigned int ln)
	/*@modifies ts @*/;
#define	rpmtsiInit(_ts)		XrpmtsiInit(_ts, __FILE__, __LINE__)

/** \ingroup rpmte
 * Return next transaction element of type.
 * @param tsi		transaction element iterator
 * @param type		transaction element type selector (0 for any)
 * @return		next transaction element of type, NULL on termination
 */
/*@dependent@*/ /*@null@*/
rpmte rpmtsiNext(rpmtsi tsi, rpmElementType type)
        /*@modifies tsi @*/;

#if	defined(DYING)
#if !defined(SWIG)
/** \ingroup rpmte
 */
static inline void rpmtePrintID(rpmte p)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (p != NULL) {
	if (p->blink.Pkgid) argvPrint("blink.Pkgid", p->blink.Pkgid, NULL);
	if (p->blink.Hdrid) argvPrint("blink.Hdrid", p->blink.Hdrid, NULL);
	if (p->blink.NEVRA) argvPrint("blink.NEVRA", p->blink.NEVRA, NULL);
	if (p->flink.Pkgid) argvPrint("flink.Pkgid", p->flink.Pkgid, NULL);
	if (p->flink.Hdrid) argvPrint("flink.Hdrid", p->flink.Hdrid, NULL);
	if (p->flink.NEVRA) argvPrint("flink.NEVRA", p->flink.NEVRA, NULL);
    }
};

/** \ingroup rpmte
 */
static inline void hdrPrintInstalled(Header h)
	/*@globals fileSystem @*/
	/*@modifies h, fileSystem @*/
{
    const char * qfmt = "[%{erasednevra} O:%{packageorigin} P:%{erasedpkgid} H:%{erasedhdrid}\n]";
    const char * errstr = "(unknown error)";
/*@-modobserver@*/
    const char * str = headerSprintf(h, qfmt, rpmTagTable, rpmHeaderFormats, &errstr);
/*@=modobserver@*/

    if (str == NULL)
	fprintf(stderr, "error: %s\n", errstr);
    else {
	fprintf(stderr, "%s", str);
	str = _free(str);
    }
}

/** \ingroup rpmte
 */
static inline void hdrPrintErased(Header h)
	/*@globals fileSystem @*/
	/*@modifies h, fileSystem @*/
{
    const char * qfmt = "[%{installednevra} O:%{packageorigin} P:%{installedpkgid} H:%{installedhdrid}\n]";
    const char * errstr = "(unknown error)";
/*@-modobserver@*/
    const char * str = headerSprintf(h, qfmt, rpmTagTable, rpmHeaderFormats, &errstr);
/*@=modobserver@*/
    if (str == NULL)
	fprintf(stderr, "error: %s\n", errstr);
    else {
	fprintf(stderr, "%s", str);
	str = _free(str);
    }
}
#endif
#endif

#ifdef	REFERENCE
/** \ingroup rpmte
 * Transaction element file states.
 */
typedef struct rpmfs_s *		rpmfs;

/**
 */
struct sharedFileInfo_s {
    int pkgFileNum;
    int otherPkg;
    int otherFileNum;
};

typedef char rpm_fstate_t;

struct rpmfs_s {
    unsigned int fc;

    rpm_fstate_t * states;
    rpmFileAction * actions;	/*!< File disposition(s). */

    sharedFileInfo replaced;	/*!< (TR_ADDED) to be replaced files in the rpmdb */
    int numReplaced;
    int allocatedReplaced;
};

int rpmteMarkFailed(rpmte te, rpmts ts);

rpmps rpmteProblems(rpmte te);
rpmfs rpmteGetFileStates(rpmte te);
rpmfs rpmfsNew(unsigned int fc, rpmElementType type);
rpmfs rpmfsFree(rpmfs fs);
rpm_count_t rpmfsFC(rpmfs fs);
void rpmfsAddReplaced(rpmfs fs, int pkgFileNum, int otherPkg, int otherFileNum);
sharedFileInfo rpmfsGetReplaced(rpmfs fs);
sharedFileInfo rpmfsNextReplaced(rpmfs fs , sharedFileInfo replaced);
void rpmfsSetState(rpmfs fs, unsigned int ix, rpmfileState state);
rpmfileState rpmfsGetState(rpmfs fs, unsigned int ix);
/*@null@*/
rpm_fstate_t * rpmfsGetStates(rpmfs fs);
void rpmfsSetAction(rpmfs fs, unsigned int ix, rpmFileAction action);
#endif	/* REFERENCE */

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMTE */
