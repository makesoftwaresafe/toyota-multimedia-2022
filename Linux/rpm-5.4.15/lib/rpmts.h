#ifndef H_RPMTS
#define H_RPMTS

/** \ingroup rpmts
 * \file lib/rpmts.h
 * Structures and prototypes used for an "rpmts" transaction set.
 */

#include "rpmps.h"
#include "rpmsw.h"
#include <rpmpgp.h>		/* XXX pgpVSFlags */
#if defined(_RPMTS_INTERNAL)
#include <rpmbag.h>
#endif

/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmts_debug;
/*@unchecked@*/
extern int _rpmts_macros;
/*@unchecked@*/
extern int _rpmts_stats;
/*@unchecked@*/
extern int _fps_debug;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Bit(s) to control digest and signature verification.
 */
typedef pgpVSFlags rpmVSFlags;

/** \ingroup rpmts
 * Bit(s) to control rpmtsCheck() and rpmtsOrder() operation.
 * @todo Move to rpmts.h.
 */
typedef enum rpmdepFlags_e {
    RPMDEPS_FLAG_NONE		= 0,
    RPMDEPS_FLAG_NOUPGRADE	= (1 <<  0),	/*!< from --noupgrade */
    RPMDEPS_FLAG_NOREQUIRES	= (1 <<  1),	/*!< from --norequires */
    RPMDEPS_FLAG_NOCONFLICTS	= (1 <<  2),	/*!< from --noconflicts */
    RPMDEPS_FLAG_NOOBSOLETES	= (1 <<  3),	/*!< from --noobsoletes */
    RPMDEPS_FLAG_NOPARENTDIRS	= (1 <<  4),	/*!< from --noparentdirs */
    RPMDEPS_FLAG_NOLINKTOS	= (1 <<  5),	/*!< from --nolinktos */
    RPMDEPS_FLAG_ANACONDA	= (1 <<  6),	/*!< from --anaconda */
    RPMDEPS_FLAG_NOSUGGEST	= (1 <<  7),	/*!< from --nosuggest */
    RPMDEPS_FLAG_ADDINDEPS	= (1 <<  8),	/*!< from --aid */
    RPMDEPS_FLAG_DEPLOOPS	= (1 <<  9)	/*!< from --deploops */
} rpmdepFlags;

/** \ingroup rpmts
 * Bit(s) to control rpmtsRun() operation.
 * @todo Move to rpmts.h.
 */
typedef enum rpmtransFlags_e {
    RPMTRANS_FLAG_NONE		= 0,
    RPMTRANS_FLAG_TEST		= (1 <<  0),	/*!< from --test */
    RPMTRANS_FLAG_BUILD_PROBS	= (1 <<  1),	/*!< don't process payload */
    RPMTRANS_FLAG_NOSCRIPTS	= (1 <<  2),	/*!< from --noscripts */
    RPMTRANS_FLAG_JUSTDB	= (1 <<  3),	/*!< from --justdb */
    RPMTRANS_FLAG_NOTRIGGERS	= (1 <<  4),	/*!< from --notriggers */
    RPMTRANS_FLAG_NODOCS	= (1 <<  5),	/*!< from --excludedocs */
    RPMTRANS_FLAG_ALLFILES	= (1 <<  6),	/*!< from --allfiles */
	/* 7 unused */
    RPMTRANS_FLAG_NOCONTEXTS	= (1 <<  8),	/*!< from --nocontexts */
    RPMTRANS_FLAG_DIRSTASH	= (1 <<  9),	/*!< from --dirstash */
    RPMTRANS_FLAG_REPACKAGE	= (1 << 10),	/*!< from --repackage */

    RPMTRANS_FLAG_PKGCOMMIT	= (1 << 11),
/*@-enummemuse@*/
    RPMTRANS_FLAG_PKGUNDO	= (1 << 12),
/*@=enummemuse@*/
    RPMTRANS_FLAG_COMMIT	= (1 << 13),
/*@-enummemuse@*/
    RPMTRANS_FLAG_UNDO		= (1 << 14),
/*@=enummemuse@*/
    RPMTRANS_FLAG_APPLYONLY	= (1 << 15),

    RPMTRANS_FLAG_NOTRIGGERPREIN= (1 << 16),	/*!< from --notriggerprein */
    RPMTRANS_FLAG_NOPRE		= (1 << 17),	/*!< from --nopre */
    RPMTRANS_FLAG_NOPOST	= (1 << 18),	/*!< from --nopost */
    RPMTRANS_FLAG_NOTRIGGERIN	= (1 << 19),	/*!< from --notriggerin */
    RPMTRANS_FLAG_NOTRIGGERUN	= (1 << 20),	/*!< from --notriggerun */
    RPMTRANS_FLAG_NOPREUN	= (1 << 21),	/*!< from --nopreun */
    RPMTRANS_FLAG_NOPOSTUN	= (1 << 22),	/*!< from --nopostun */
    RPMTRANS_FLAG_NOTRIGGERPOSTUN = (1 << 23),	/*!< from --notriggerpostun */
/*@-enummemuse@*/
    RPMTRANS_FLAG_NOPAYLOAD	= (1 << 24),
/*@=enummemuse@*/
    RPMTRANS_FLAG_NORPMDB	= (1 << 25),	/*!< from --norpmdb */
    RPMTRANS_FLAG_NOPOLICY	= (1 << 26),	/*!< from --nopolicy */
    RPMTRANS_FLAG_NOFDIGESTS	= (1 << 27),	/*!< from --nofdigests */
    RPMTRANS_FLAG_NOPRETRANS	= (1 << 28),	/*!< from --nopretrans */
    RPMTRANS_FLAG_NOPOSTTRANS	= (1 << 29),	/*!< from --noposttrans */
    RPMTRANS_FLAG_NOCONFIGS	= (1 << 30)	/*!< from --noconfigs */
	/* 31 unused */
} rpmtransFlags;

#define	_noTransScripts		\
  ( RPMTRANS_FLAG_NOPRETRANS |	\
    RPMTRANS_FLAG_NOPRE |	\
    RPMTRANS_FLAG_NOPOST |	\
    RPMTRANS_FLAG_NOPREUN |	\
    RPMTRANS_FLAG_NOPOSTUN |	\
    RPMTRANS_FLAG_NOPOSTTRANS	\
  )

#define	_noTransTriggers	\
  ( RPMTRANS_FLAG_NOTRIGGERPREIN | \
    RPMTRANS_FLAG_NOTRIGGERIN |	\
    RPMTRANS_FLAG_NOTRIGGERUN |	\
    RPMTRANS_FLAG_NOTRIGGERPOSTUN \
  )

/** \ingroup rpmts
 * Indices for timestamps.
 */
typedef	enum rpmtsOpX_e {
    RPMTS_OP_TOTAL		=  0,
    RPMTS_OP_CHECK		=  1,
    RPMTS_OP_ORDER		=  2,
    RPMTS_OP_FINGERPRINT	=  3,
    RPMTS_OP_REPACKAGE		=  4,
    RPMTS_OP_INSTALL		=  5,
    RPMTS_OP_ERASE		=  6,
    RPMTS_OP_SCRIPTLETS		=  7,
    RPMTS_OP_COMPRESS		=  8,
    RPMTS_OP_UNCOMPRESS		=  9,
    RPMTS_OP_DIGEST		= 10,
    RPMTS_OP_SIGNATURE		= 11,
    RPMTS_OP_DBADD		= 12,
    RPMTS_OP_DBREMOVE		= 13,
    RPMTS_OP_DBGET		= 14,
    RPMTS_OP_DBPUT		= 15,
    RPMTS_OP_DBDEL		= 16,
    RPMTS_OP_READHDR		= 17,
    RPMTS_OP_HDRLOAD		= 18,
    RPMTS_OP_HDRGET		= 19,
    RPMTS_OP_DEBUG		= 20,
    RPMTS_OP_MAX		= 20
} rpmtsOpX;

/** \ingroup rpmts
 * Transaction Types
 */
typedef enum rpmTSType_e {
	RPMTRANS_TYPE_NORMAL       = 0,
	RPMTRANS_TYPE_ROLLBACK     = (1 << 0),
	RPMTRANS_TYPE_AUTOROLLBACK = (1 << 1)
} rpmTSType;

/** \ingroup rpmts
 */
typedef enum tsStage_e {
    TSM_UNKNOWN		=  0,
    TSM_INSTALL		=  7,
    TSM_ERASE		=  8
} tsmStage;

#if defined(_RPMTS_INTERNAL)

#include <rpmbf.h>
#include "rpmhash.h"	/* XXX hashTable */
#include "rpmkeyring.h"
#include <rpmtxn.h>
#include "rpmal.h"	/* XXX availablePackage/relocateFileList ,*/

/*@unchecked@*/
/*@-exportlocal@*/
extern int _cacheDependsRC;
/*@=exportlocal@*/

/** \ingroup rpmts
 */
typedef	/*@abstract@*/ struct diskspaceInfo_s * rpmDiskSpaceInfo;

/** \ingroup rpmts
 * An internal copy of (linux) struct statvfs for portability, with extensions.
 */
struct diskspaceInfo_s {
    unsigned long f_bsize;	/*!< File system block size. */
    unsigned long f_frsize;	/*!< File system fragment size. */
    unsigned long long f_blocks;/*!< File system size (in frsize units). */
    unsigned long long f_bfree;	/*!< No. of free blocks. */
    signed long long f_bavail;	/*!< No. of blocks available to non-root. */
    unsigned long long f_files;	/*!< No. of inodes. */
    unsigned long long f_ffree;	/*!< No. of free inodes. */
    signed long long f_favail;	/*!< No. of inodes available to non-root. */
    unsigned long f_fsid;	/*!< File system id. */
    unsigned long f_flag;	/*!< Mount flags. */
    unsigned long f_namemax;	/*!< Maximum filename length. */

    signed long long bneeded;	/*!< No. of blocks needed. */
    signed long long ineeded;	/*!< No. of inodes needed. */
    signed long long obneeded;	/*!< Bookkeeping to avoid duplicate reports. */
    signed long long oineeded;	/*!< Bookkeeping to avoid duplicate reports. */
    dev_t dev;			/*!< File system device number. */
};

/** \ingroup rpmts
 * Adjust for root only reserved space. On linux e2fs, this is 5%.
 */
#define	adj_fs_blocks(_nb)	(((_nb) * 21) / 20)

#define BLOCK_ROUND(size, block) (((size) + (block) - 1) / (block))

/** \ingroup rpmts
 * The set of packages to be installed/removed atomically.
 */
struct rpmts_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmdepFlags depFlags;	/*!< Bit(s) to control rpmtsCheck(). */
    rpmtransFlags transFlags;	/*!< Bit(s) to control rpmtsRun(). */
    tsmStage goal;		/*!< Transaction goal (i.e. mode) */
    rpmTSType type;		/*!< default, rollback, autorollback */

/*@refcounted@*/ /*@null@*/
    rpmbag bag;			/*!< Solve store collection. */
/*@null@*/
    int (*solve) (rpmts ts, rpmds key, const void * data)
	/*@modifies ts @*/;	/*!< Search for NEVRA key. */
/*@relnull@*/
    const void * solveData;	/*!< Solve callback data */
    int nsuggests;		/*!< No. of depCheck suggestions. */
/*@only@*/ /*@null@*/
    const void ** suggests;	/*!< Possible depCheck suggestions. */

/*@observer@*/ /*@null@*/
    rpmCallbackFunction notify;	/*!< Callback function. */
/*@observer@*/ /*@null@*/
    rpmCallbackData notifyData;	/*!< Callback private data. */

/*@null@*/
    rpmPRCO PRCO;		/*!< Current transaction dependencies. */

/*@refcounted@*/ /*@null@*/
    rpmps probs;		/*!< Current problems in transaction. */
    rpmprobFilterFlags ignoreSet;
				/*!< Bits to filter current problems. */

    rpmuint32_t filesystemCount;/*!< No. of mounted filesystems. */
/*@dependent@*/ /*@null@*/
    const char ** filesystems;	/*!< Mounted filesystem names. */
/*@only@*/ /*@relnull@*/
    rpmDiskSpaceInfo dsi;	/*!< Per filesystem disk/inode usage. */

/*@refcounted@*/ /*@null@*/
    rpmdb rdb;			/*!< Install database handle. */
    int dbmode;			/*!< Install database open mode. */
/*@only@*/
    hashTable ht;		/*!< Fingerprint hash table. */
/*@null@*/
    rpmtxn txn;			/*!< Transaction set transaction pointer. */

/*@refcounted@*/ /*@null@*/
    rpmbf rbf;			/*!< Removed packages Bloom filter. */
/*@only@*/ /*@null@*/
    uint32_t * removedPackages;	/*!< Set of packages being removed. */
    int numRemovedPackages;	/*!< No. removed package instances. */
    int allocedRemovedPackages;	/*!< Size of removed packages array. */

/*@only@*/
    rpmal addedPackages;	/*!< Set of packages being installed. */
    int numAddedPackages;	/*!< No. added package instances. */
    int numAddedFiles;		/*!< No. of files in added packages. */

/*@only@*/
    rpmal erasedPackages;	/*!< Set of packages being erased. */
    int numErasedPackages;	/*!< No. erased package instances. */
    int numErasedFiles;		/*!< No. of files in erased packages. */

#ifndef	DYING
/*@only@*/
    rpmal availablePackages;	/*!< Universe of available packages. */
    int numAvailablePackages;	/*!< No. available package instances. */
#endif

/*@null@*/
    rpmte relocateElement;	/*!< Element to use when relocating packages. */

/*@owned@*/ /*@relnull@*/
    rpmte * order;		/*!< Packages sorted by dependencies. */
    int orderCount;		/*!< No. of transaction elements. */
    int orderAlloced;		/*!< No. of allocated transaction elements. */
    int unorderedSuccessors;	/*!< Index of 1st element of successors. */
    int ntrees;			/*!< No. of dependency trees. */
    int maxDepth;		/*!< Maximum depth of dependency tree(s). */

/*@dependent@*/ /*@relnull@*/
    rpmte teInstall;		/*!< current rpmtsAddInstallElement element. */
/*@dependent@*/ /*@relnull@*/
    rpmte teErase;		/*!< current rpmtsAddEraseElement element. */

    int selinuxEnabled;		/*!< Is SE linux enabled? */
    int chrootDone;		/*!< Has chroot(2) been been done? */
/*@only@*/ /*@null@*/
    const char * rootDir;	/*!< Path to top of install tree. */
/*@only@*/ /*@null@*/
    const char * currDir;	/*!< Current working directory. */
/*@null@*/
    FD_t scriptFd;		/*!< Scriptlet stdout/stderr. */
    int delta;			/*!< Delta for reallocation. */
    rpmuint32_t tid[2];		/*!< Transaction id. */

    rpmuint32_t color;		/*!< Transaction color bits. */
    rpmuint32_t prefcolor;	/*!< Preferred file color. */

/*@observer@*/ /*@dependent@*/ /*@null@*/
    const char * fn;		/*!< Current package fn. */

/*@refcounted@*/ /*@relnull@*/
    rpmKeyring keyring;		/*!< Keyring in use. */
/*@relnull@*/
    void * hkp;			/*!< Pubkey validation container. */

    struct rpmop_s ops[RPMTS_OP_MAX];

/*@refcounted@*/ /*@relnull@*/
    pgpDig dig;			/*!< Current signature/pubkey parameters. */

/*@null@*/
    Spec spec;			/*!< Spec file control structure. */

    rpmuint32_t arbgoal;	/*!< Autorollback goal */

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMTS_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmts
 * Perform dependency resolution on the transaction set.
 *
 * Any problems found by rpmtsCheck() can be examined by retrieving the 
 * problem set with rpmtsProblems(), success here only means that
 * the resolution was successfully attempted for all packages in the set.
 *
 * @param ts		transaction set
 * @return		0 = deps ok, 1 = dep problems, 2 = error
 */
extern int (*rpmtsCheck) (rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
int _rpmtsCheck(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Determine package order in a transaction set according to dependencies.
 *
 * Order packages, returning error if circular dependencies cannot be
 * eliminated by removing Requires's from the loop(s). Only dependencies from
 * added or removed packages are used to determine ordering using a
 * topological sort (Knuth vol. 1, p. 262). Use rpmtsCheck() to verify
 * that all dependencies can be resolved.
 *
 * The final order ends up as installed packages followed by removed packages,
 * with packages removed for upgrades immediately following the new package
 * to be installed.
 *
 * @param ts		transaction set
 * @return		no. of (added) packages that could not be ordered
 */
extern int (*rpmtsOrder) (rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
int _rpmtsOrder(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
int _orgrpmtsOrder(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Process all package elements in a transaction set.  Before calling
 * rpmtsRun be sure to have:
 *
 *    - setup the rpm root dir via rpmtsSetRootDir().
 *    - setup the rpm notify callback via rpmtsSetNotifyCallback().
 *    - setup the rpm transaction flags via rpmtsSetFlags().
 * 
 * Additionally, though not required you may want to:
 *
 *    - setup the rpm verify signature flags via rpmtsSetVSFlags().
 *       
 * @param ts		transaction set
 * @param okProbs	previously known problems (or NULL)
 * @param ignoreSet	bits to filter problem types
 * @return		0 on success, -1 on error, >0 with newProbs set
 */
extern int (*rpmtsRun) (rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
int _rpmtsRun(rpmts ts, rpmps okProbs, rpmprobFilterFlags ignoreSet)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Rollback a failed transaction.
 * @param rbts		failed transaction set
 * @param ignoreSet     problems to ignore
 * @param running       partial transaction?
 * @param rbte		failed transaction element
 * @return		RPMRC_OK, or RPMRC_FAIL
 */
rpmRC rpmtsRollback(rpmts rbts, rpmprobFilterFlags ignoreSet,
		int running, rpmte rbte)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies rbts, rbte, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Unreference a transaction instance.
 * @param ts		transaction set
 * @param msg
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmts rpmtsUnlink (/*@killref@*/ /*@only@*/ rpmts ts,
		const char * msg)
	/*@modifies ts @*/;
#define	rpmtsUnlink(_ts, _msg)	\
	((rpmts) rpmioUnlinkPoolItem((rpmioItem)(_ts), _msg, __FILE__, __LINE__))

/** \ingroup rpmts
 * Reference a transaction set instance.
 * @param ts		transaction set
 * @param msg
 * @return		new transaction set reference
 */
/*@unused@*/ /*@newref@*/
rpmts rpmtsLink (rpmts ts, const char * msg)
	/*@modifies ts @*/;
#define	rpmtsLink(_ts, _msg)	\
	((rpmts) rpmioLinkPoolItem((rpmioItem)(_ts), _msg, __FILE__, __LINE__))

/** \ingroup rpmts
 * Close the database used by the transaction.
 * @param ts		transaction set
 * @return		0 on success
 */
int rpmtsCloseDB(rpmts ts)
	/*@globals fileSystem @*/
	/*@modifies ts, fileSystem @*/;

/** \ingroup rpmts
 * Open the database used by the transaction.
 * @param ts		transaction set
 * @param dbmode	O_RDONLY or O_RDWR
 * @return		0 on success
 */
int rpmtsOpenDB(rpmts ts, int dbmode)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Initialize the database used by the transaction.
 * @deprecated An explicit rpmdbInit() is never needed.
 * @param ts		transaction set
 * @param dbmode	O_RDONLY or O_RDWR
 * @return		0 on success
 */
static inline /*@unused@*/
int rpmtsInitDB(/*@unused@*/ rpmts ts, /*@unused@*/ int dbmode)
	/*@*/
{
    return -1;
}

/** \ingroup rpmts
 * Rebuild the database used by the transaction.
 * @param ts		transaction set
 * @return		0 on success
 */
int rpmtsRebuildDB(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Verify the database used by the transaction.
 * @deprecated Use included standalone db_verify(1) utility instead.
 * @param ts		transaction set
 * @return		0 on success
 */
static inline /*@unused@*/
int rpmtsVerifyDB(/*@unused@*/ rpmts ts)
	/*@*/
{
    return -1;
}

/** \ingroup rpmts
 * Return transaction database iterator.
 * @param ts		transaction set
 * @param rpmtag	rpm tag
 * @param keyp		key data (NULL for sequential access)
 * @param keylen	key data length (0 will use strlen(keyp))
 * @return		NULL on failure
 */
/*@only@*/ /*@null@*/
rpmmi rpmtsInitIterator(const rpmts ts, rpmTag rpmtag,
			/*@null@*/ const void * keyp, size_t keylen)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Retrieve pubkey from rpm database.
 * @param ts		rpm transaction
 * @param _dig		container (NULL uses rpmtsDig(ts) instead).
 * @return		RPMRC_OK on success, RPMRC_NOKEY if not found
 */
/*@-exportlocal@*/
rpmRC rpmtsFindPubkey(rpmts ts, /*@null@*/ void * _dig)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, _dig, rpmGlobalMacroContext, fileSystem, internalState */;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Close the database used by the transaction to solve dependencies.
 * @param ts		transaction set
 * @return		0 on success
 */
/*@-exportlocal@*/
int rpmtsCloseSDB(rpmts ts)
	/*@globals fileSystem @*/
	/*@modifies ts, fileSystem @*/;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Open the database used by the transaction to solve dependencies.
 * @param ts		transaction set
 * @param dbmode	O_RDONLY or O_RDWR
 * @return		0 on success
 */
/*@-exportlocal@*/
int rpmtsOpenSDB(rpmts ts, int dbmode)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Attempt to solve a needed dependency using the solve database.
 * @param ts		transaction set
 * @param ds		dependency set
 * @param data		opaque data associated with callback
 * @return		-1 retry, 0 ignore, 1 not found
 */
/*@-exportlocal@*/
int rpmtsSolve(rpmts ts, rpmds ds, const void * data)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Attempt to solve a needed dependency using memory resident tables.
 * @deprecated This function will move from rpmlib to the python bindings.
 * @param ts		transaction set
 * @param ds		dependency set
 * @return		0 if resolved (and added to ts), 1 not found
 */
/*@unused@*/
int rpmtsAvailable(rpmts ts, const rpmds ds)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Set dependency solver callback.
 * @param ts		transaction set
 * @param (*solve)	dependency solver callback
 * @param solveData	dependency solver callback data (opaque)
 * @return		0 on success
 */
int rpmtsSetSolveCallback(rpmts ts,
		int (*solve) (rpmts ts, rpmds ds, const void * data),
		const void * solveData)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Return the type of a transaction.
 * @param ts		transaction set
 * @return		transaction type, 0 on unknown
 */
rpmTSType rpmtsType(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction type.
 *   Allowed types are:
 * 	RPMTRANS_TYPE_NORMAL
 *	RPMTRANS_TYPE_ROLLBACK
 * 	RPMTRANS_TYPE_AUTOROLLBACK
 *
 * @param ts		transaction set
 * @param type		transaction type
 */
void rpmtsSetType(rpmts ts, rpmTSType type)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Return the autorollback goal. 
 * @param ts		transaction set
 * @return		autorollback goal
 */
rpmuint32_t rpmtsARBGoal(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set autorollback goal.   
 * @param ts		transaction set
 * @param goal		autorollback goal
 */
void rpmtsSetARBGoal(rpmts ts, rpmuint32_t goal)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Return current transaction set problems.
 * @param ts		transaction set
 * @return		current problem set (or NULL)
 */
/*@null@*/
rpmps rpmtsProblems(rpmts ts)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Free signature verification data.
 * @param ts		transaction set
 */
void rpmtsCleanDig(rpmts ts)
	/*@globals fileSystem @*/
	/*@modifies ts, fileSystem @*/;

/** \ingroup rpmts
 * Free memory needed only for dependency checks and ordering.
 * @param ts		transaction set
 */
void rpmtsClean(rpmts ts)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem , internalState@*/;

/** \ingroup rpmts
 * Re-create an empty transaction set.
 * @param ts		transaction set
 */
void rpmtsEmpty(rpmts ts)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Destroy transaction set, closing the database as well.
 * @param ts		transaction set
 * @return		NULL on last dereference
 */
/*@null@*/
rpmts rpmtsFree(/*@killref@*/ /*@null@*/ rpmts ts)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;
#define	rpmtsFree(_ts)	\
	((rpmts) rpmioFreePoolItem((rpmioItem)(_ts), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmts
 * Get transaction keyring.
 * @param ts		transaction set
 * @param autoload	Should keyring be loaded? (unimplmented)
 * @return		transaction keyring
 */
void * rpmtsGetKeyring(rpmts ts, int autoload)
	/*@*/;

/** \ingroup rpmts
 * Set transaction keyring.
 * @param ts		transaction set
 * @param _keyring	new transaction keyring
 * @return		0 on success
 */
int rpmtsSetKeyring(rpmts ts, void * _keyring)
	/*modifies ts, _keyring @*/;

/** \ingroup rpmts
 * Get verify signatures flag(s).
 * @param ts		transaction set
 * @return		verify signatures flags
 */
rpmVSFlags rpmtsVSFlags(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set verify signatures flag(s).
 * @param ts		transaction set
 * @param vsflags	new verify signatures flags
 * @return		previous value
 */
rpmVSFlags rpmtsSetVSFlags(rpmts ts, rpmVSFlags vsflags)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Set index of 1st element of successors.
 * @param ts		transaction set
 * @param first		new index of 1st element of successors
 * @return		previous value
 */
int rpmtsUnorderedSuccessors(rpmts ts, int first)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get transaction rootDir, i.e. path to chroot(2).
 * @param ts		transaction set
 * @return		transaction rootDir
 */
/*@observer@*/ /*@null@*/
extern const char * rpmtsRootDir(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction rootDir, i.e. path to chroot(2).
 * @param ts		transaction set
 * @param rootDir	new transaction rootDir (or NULL)
 */
void rpmtsSetRootDir(rpmts ts, /*@null@*/ const char * rootDir)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get transaction currDir, i.e. current directory before chroot(2).
 * @param ts		transaction set
 * @return		transaction currDir
 */
/*@observer@*/ /*@null@*/
extern const char * rpmtsCurrDir(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction currDir, i.e. current directory before chroot(2).
 * @param ts		transaction set
 * @param currDir	new transaction currDir (or NULL)
 */
void rpmtsSetCurrDir(rpmts ts, /*@null@*/ const char * currDir)
	/*@modifies ts @*/;

#if defined(_RPMTS_INTERNAL)	/* XXX avoid FD_t in API. */
/** \ingroup rpmts
 * Get transaction script file handle, i.e. stdout/stderr on scriptlet execution
 * @param ts		transaction set
 * @return		transaction script file handle
 */
/*@null@*/
FD_t rpmtsScriptFd(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction script file handle, i.e. stdout/stderr on scriptlet execution
 * @param ts		transaction set
 * @param scriptFd	new script file handle (or NULL)
 */
void rpmtsSetScriptFd(rpmts ts, /*@null@*/ FD_t scriptFd)
	/*@globals fileSystem @*/
	/*@modifies ts, scriptFd, fileSystem @*/;
#endif

/** \ingroup rpmts
 * Get selinuxEnabled flag, i.e. is SE linux enabled?
 * @param ts		transaction set
 * @return		selinuxEnabled flag
 */
int rpmtsSELinuxEnabled(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Get chrootDone flag, i.e. has chroot(2) been performed?
 * @param ts		transaction set
 * @return		chrootDone flag
 */
int rpmtsChrootDone(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set chrootDone flag, i.e. has chroot(2) been performed?
 * @param ts		transaction set
 * @param chrootDone	new chrootDone flag
 * @return		previous chrootDone flag
 */
int rpmtsSetChrootDone(rpmts ts, int chrootDone)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get transaction id, i.e. transaction time stamp.
 * @param ts		transaction set
 * @return		transaction id
 */
rpmuint32_t rpmtsGetTid(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction id, i.e. transaction time stamp.
 * @param ts		transaction set
 * @param tid		new transaction id
 * @return		previous transaction id
 */
rpmuint32_t rpmtsSetTid(rpmts ts, rpmuint32_t tid)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get OpenPGP packet parameters, i.e. signature/pubkey constants.
 * @param ts		transaction set
 * @return		signature/pubkey constants.
 */
pgpDig rpmtsDig(rpmts ts)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/** \ingroup rpmts
 * Return OpenPGP pubkey constants.
 * @param ts		transaction set
 * @return		pubkey constants.
 */
/*@-exportlocal@*/
/*@exposed@*/ /*@null@*/
pgpDigParams rpmtsPubkey(const rpmts ts)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=exportlocal@*/

/** \ingroup rpmts
 * Get transaction set database handle.
 * @param ts		transaction set
 * @return		transaction database handle
 */
/*@null@*/
rpmdb rpmtsGetRdb(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Get transaction set dependencies.
 * @param ts		transaction set
 * @return		transaction set dependencies.
 */
/*@null@*/
rpmPRCO rpmtsPRCO(rpmts ts)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Initialize disk space info for each and every mounted file systems.
 * @param ts		transaction set
 * @return		0 on success
 */
int rpmtsInitDSI(const rpmts ts)
	/*@globals fileSystem, internalState @*/
	/*@modifies ts, fileSystem, internalState @*/;

/** \ingroup rpmts
 * Update disk space info for a file.
 * @param ts		transaction set
 * @param dev		mount point device
 * @param fileSize	file size
 * @param prevSize	previous file size (if upgrading)
 * @param fixupSize	size difference (if
 * @param _action	file disposition
 */
void rpmtsUpdateDSI(const rpmts ts, dev_t dev,
		rpmuint32_t fileSize, rpmuint32_t prevSize, rpmuint32_t fixupSize,
		int _action)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Check a transaction element for disk space problems.
 * @param ts		transaction set
 * @param te		current transaction element
 */
void rpmtsCheckDSIProblems(const rpmts ts, const rpmte te)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Perform transaction progress notify callback.
 * @warning This function's args have changed, so the function cannot be
 * used portably
 * @param ts		transaction set
 * @param te		current transaction element
 * @param what		type of call back
 * @param amount	current value
 * @param total		final value
 * @return		callback dependent pointer
 */
/*@null@*/
void * rpmtsNotify(rpmts ts, rpmte te,
                rpmCallbackType what, rpmuint64_t amount, rpmuint64_t total)
	/*@modifies te @*/;

/** \ingroup rpmts
 * Return number of (ordered) transaction set elements.
 * @param ts		transaction set
 * @return		no. of transaction set elements
 */
int rpmtsNElements(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Return (ordered) transaction set element.
 * @param ts		transaction set
 * @param ix		transaction element index
 * @return		transaction element (or NULL)
 */
/*@null@*/ /*@dependent@*/
rpmte rpmtsElement(rpmts ts, int ix)
	/*@*/;

/** \ingroup rpmts
 * Get problem ignore bit mask, i.e. bits to filter encountered problems.
 * @param ts		transaction set
 * @return		ignore bit mask
 */
rpmprobFilterFlags rpmtsFilterFlags(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Get transaction flags, i.e. bits that control rpmtsRun().
 * @param ts		transaction set
 * @return		transaction flags
 */
rpmtransFlags rpmtsFlags(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set transaction flags, i.e. bits that control rpmtsRun().
 * @param ts		transaction set
 * @param transFlags	new transaction flags
 * @return		previous transaction flags
 */
rpmtransFlags rpmtsSetFlags(rpmts ts, rpmtransFlags transFlags)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get dependency flags, i.e. bits that control rpmtsCheck() and rpmtsOrder().
 * @param ts		transaction set
 * @return		dependency flags
 */
rpmdepFlags rpmtsDFlags(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set dependency flags, i.e. bits that control rpmtsCheck() and rpmtsOrder().
 * @param ts		transaction set
 * @param depFlags	new dependency flags
 * @return		previous dependency flags
 */
rpmdepFlags rpmtsSetDFlags(rpmts ts, rpmdepFlags depFlags)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get spec control structure from transaction set.
 * @param ts		transaction set
 * @return		spec control structure
 */
/*@null@*/
Spec rpmtsSpec(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set a spec control structure in transaction set.
 * @param ts		transaction set
 * @param spec		new spec control structure
 * @return		previous spec control structure
 */
/*@null@*/
Spec rpmtsSetSpec(rpmts ts, /*@null@*/ Spec spec)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Get current relocate transaction element.
 * @param ts		transaction set
 * @return		current relocate transaction element
 */
/*@null@*/
rpmte rpmtsRelocateElement(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set current relocate transaction element.
 * @param ts		transaction set
 * @param relocateElement new relocate transaction element
 * @return		previous relocate transaction element
 */
/*@null@*/
rpmte rpmtsSetRelocateElement(rpmts ts, /*@null@*/ rpmte relocateElement)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Retrieve goal of transaction set.
 * @param ts		transaction set
 * @return		goal
 */
tsmStage rpmtsGoal(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set goal of transaction set.
 * @param ts		transaction set
 * @param goal		new goal
 * @return		previous goal
 */
tsmStage rpmtsSetGoal(rpmts ts, tsmStage goal)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Retrieve dbmode of transaction set.
 * @param ts		transaction set
 * @return		dbmode
 */
int rpmtsDBMode(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set dbmode of transaction set.
 * @param ts		transaction set
 * @param dbmode	new dbmode
 * @return		previous dbmode
 */
int rpmtsSetDBMode(rpmts ts, int dbmode)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Retrieve color bits of transaction set.
 * @param ts		transaction set
 * @return		color bits
 */
rpmuint32_t rpmtsColor(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Retrieve preferred file color
 * @param ts		transaction set
 * @return		color bits
 */
rpmuint32_t rpmtsPrefColor(rpmts ts)
	/*@*/;

/** \ingroup rpmts
 * Set color bits of transaction set.
 * @param ts		transaction set
 * @param color		new color bits
 * @return		previous color bits
 */
rpmuint32_t rpmtsSetColor(rpmts ts, rpmuint32_t color)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Retrieve operation timestamp from a transaction set.
 * @param ts		transaction set
 * @param opx		operation timestamp index
 * @return		pointer to operation timestamp.
 */
/*@relnull@*/
rpmop rpmtsOp(rpmts ts, rpmtsOpX opx)
	/*@*/;

/** \ingroup rpmts
 * Set transaction notify callback function and argument.
 *
 * @warning This call must be made before rpmtsRun() for
 *	install/upgrade/freshen to function correctly.
 *
 * @param ts		transaction set
 * @param notify	progress callback
 * @param notifyData	progress callback private data
 * @return		0 on success
 */
int rpmtsSetNotifyCallback(rpmts ts,
		/*@observer@*/ rpmCallbackFunction notify,
		/*@observer@*/ rpmCallbackData notifyData)
	/*@modifies ts @*/;

/** \ingroup rpmts
 * Create an empty transaction set.
 * @return		new transaction set
 */
/*@newref@*/
rpmts rpmtsCreate(void)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/*@-redecl@*/
/*@unchecked@*/
extern int rpmcliPackagesTotal;
/*@=redecl@*/

/** \ingroup rpmts
 * Add package to be installed to transaction set.
 *
 * The transaction set is checked for duplicate package names.
 * If found, the package with the "newest" EVR will be replaced.
 *
 * @param ts		transaction set
 * @param h		header
 * @param key		package retrieval key (e.g. file name)
 * @param upgrade	is package being upgraded?
 * @param relocs	package file relocations
 * @return		0 on success, 1 on I/O error, 2 needs capabilities
 */
int rpmtsAddInstallElement(rpmts ts, Header h,
		/*@exposed@*/ /*@null@*/ const fnpyKey key, int upgrade,
		/*@null@*/ rpmRelocation relocs)
	/*@globals rpmcliPackagesTotal, rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies ts, h, rpmcliPackagesTotal, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmts
 * Add package to be erased to transaction set.
 * @param ts		transaction set
 * @param h		header
 * @param hdrNum	rpm database instance
 * @return		0 on success
 */
int rpmtsAddEraseElement(rpmts ts, Header h, uint32_t hdrNum)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/;

#if !defined(SWIG)
#if defined(_RPMTS_PRINT)
/** \ingroup rpmts
 * Print current transaction set contents.
 * @param ts		transaction set
 * @param fp		file handle (NULL uses stderr)
 * @return		0 always
 */
/*@unused@*/ static inline
int rpmtsPrint(/*@null@*/ rpmts ts, /*@null@*/ FILE * fp)
	/*@globals fileSystem @*/
	/*@modifies ts, *fp, fileSystem @*/
{
    rpmuint32_t tid = rpmtsGetTid(ts);
    time_t ttid = tid;
    rpmtsi tsi;
    rpmte te;

    if (fp == NULL)
	fp = stderr;

    fprintf(fp, _("=== Transaction at %-24.24s (0x%08x):\n"), ctime(&ttid),tid);
    tsi = rpmtsiInit(ts);
    while ((te = rpmtsiNext(tsi, (rpmElementType)0)) != NULL)
        fprintf(fp, "t%s> %s\n", (rpmteType(te) == TR_ADDED ? "I" : "E"),
		rpmteNEVRA(te));
    tsi = rpmtsiFree(tsi);
    return 0;
}
#endif	/* defined(_RPMTS_PRINT) */
#endif	/* !defined(SWIG) */

#ifdef __cplusplus
}
#endif


#endif	/* H_RPMTS */
