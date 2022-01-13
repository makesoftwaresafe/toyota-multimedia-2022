#ifndef H_PSM
#define H_PSM

/** \ingroup rpmtrans payload
 * \file lib/psm.h
 * Package state machine to handle a package from a transaction set.
 */


/** \ingroup rpmts
 * Package state machine data.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmpsm_s * rpmpsm;

#include <rpmsq.h>
#include <rpmfi.h>	/* XXX rpmfi typedef */

/*@-exportlocal@*/
/*@unchecked@*/
extern int _psm_debug;
/*@=exportlocal@*/

/**
 */
#define	PSM_VERBOSE	0x8000
#define	PSM_INTERNAL	0x4000
#define	PSM_SYSCALL	0x2000
#define	PSM_DEAD	0x1000
#define	_fv(_a)		((_a) | PSM_VERBOSE)
#define	_fi(_a)		((_a) | PSM_INTERNAL)
#define	_fs(_a)		((_a) | (PSM_INTERNAL | PSM_SYSCALL))
#define	_fd(_a)		((_a) | (PSM_INTERNAL | PSM_DEAD))
typedef enum pkgStage_e {
    PSM_UNKNOWN		=  0,
    PSM_INIT		=  1,
    PSM_PRE		=  2,
    PSM_PROCESS		=  3,
    PSM_POST		=  4,
    PSM_UNDO		=  5,
    PSM_FINI		=  6,

    PSM_PKGINSTALL	=  7,
    PSM_PKGERASE	=  8,
    PSM_PKGCOMMIT	= 10,
    PSM_PKGSAVE		= 12,

    PSM_CREATE		= 17,
    PSM_NOTIFY		= 22,
    PSM_DESTROY		= 23,
    PSM_COMMIT		= 25,

    PSM_CHROOT_IN	= 51,
    PSM_CHROOT_OUT	= 52,
    PSM_SCRIPT		= 53,
    PSM_TRIGGERS	= 54,
    PSM_IMMED_TRIGGERS	= 55,
    PSM_RPMIO_FLAGS	= 56,

    PSM_RPMDB_LOAD	= 97,
    PSM_RPMDB_ADD	= 98,
    PSM_RPMDB_REMOVE	= 99

} pkgStage;
#undef	_fv
#undef	_fi
#undef	_fs
#undef	_fd

#if defined(_RPMPSM_INTERNAL)
/**
 * PSM control bits.
 */
typedef enum rpmpsmFlags_e {
    RPMPSM_FLAGS_DEBUG		= (1 << 0), /*!< (unimplemented) */
    RPMPSM_FLAGS_CHROOTDONE	= (1 << 1), /*!< Was chroot(2) done? */
    RPMPSM_FLAGS_UNORDERED	= (1 << 2), /*!< Are all pre-requsites done? */
    RPMPSM_FLAGS_GOTTRIGGERS	= (1 << 3), /*!< Triggers were retrieved? */
} rpmpsmFlags;

/**
 */
struct rpmpsm_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    struct rpmsqElem sq;	/*!< Scriptlet/signal queue element. */

/*@only@*/ /*@null@*/
    const char * NVRA;		/*!< NVRA identifier (for debugging) */
    rpmpsmFlags flags;		/*!< PSM control bit(s). */
/*@refcounted@*/
    rpmts ts;			/*!< transaction set */
/*@dependent@*/ /*@null@*/
    rpmte te;			/*!< current transaction element */
/*@refcounted@*/ /*@relnull@*/
    rpmfi fi;			/*!< file info */
/*@refcounted@*/ /*@relnull@*/
    rpmds triggers;		/*!< trigger dependency set */
/*@null@*/
    const char ** Tpats;	/*!< rpmdb trigger pattern strings */
/*@null@*/
    void * Tmires;		/*!< rpmdb trigger patterns */
    int nTmires;		/*!< no. of rpmdb trigger patterns */
/*@only@*/
    HE_t IPhe;			/*!< Install prefixes */
/*@relnull@*/
    FD_t cfd;			/*!< Payload file handle. */
/*@relnull@*/
    FD_t fd;			/*!< Repackage file handle. */
    Header oh;			/*!< Repackage header. */
/*@null@*/
    rpmmi mi;			/*!< An rpmdb iterator for this psm's use. */
/*@observer@*/
    const char * stepName;	/*!< The current PSM step (for display). */
/*@only@*/ /*@null@*/
    const char * rpmio_flags;	/*!< Payload compression type/flags. */
/*@only@*/ /*@null@*/
    const char * payload_format;/*!< Payload archive format. */
/*@only@*/ /*@null@*/
    const char * failedFile;
/*@only@*/ /*@null@*/
    const char * pkgURL;	/*!< Repackage URL. */
/*@dependent@*/
    const char * pkgfn;		/*!< Repackage file name. */
/*@only@*/ /*@null@*/
    rpmuint32_t sstates[RPMSCRIPT_MAX];	/*!< Scriptlet return codes. */
    rpmuint32_t smetrics[RPMSCRIPT_MAX];/*!< Scriptlet time metrics. */
    rpmTag scriptTag;		/*!< Scriptlet data tag. */
    rpmTag progTag;		/*!< Scriptlet interpreter tag. */
    int npkgs_installed;	/*!< No. of installed instances. */
    int scriptArg;		/*!< Scriptlet package arg. */
    int sense;			/*!< One of RPMSENSE_TRIGGER{PREIN,IN,UN,POSTUN}. */
    int countCorrection;	/*!< 0 if installing, -1 if removing. */
    rpmCallbackType what;	/*!< Callback type. */
    unsigned long long amount;	/*!< Callback amount. */
    unsigned long long total;	/*!< Callback total. */
    rpmRC rc;
    pkgStage goal;
/*@unused@*/
    pkgStage stage;		/*!< Current psm stage. */
    pkgStage nstage;		/*!< Next psm stage. */

#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};
#endif	/* _RPMPSM_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unreference a package state machine instance.
 * @param psm		package state machine
 * @param msg
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmpsm rpmpsmUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmpsm psm,
		/*@null@*/ const char * msg)
	/*@modifies psm @*/;
#define	rpmpsmUnlink(_psm, _msg)	\
    ((rpmpsm)rpmioUnlinkPoolItem((rpmioItem)(_psm), _msg, __FILE__, __LINE__))

/**
 * Reference a package state machine instance.
 * @param psm		package state machine
 * @param msg
 * @return		new package state machine reference
 */
/*@unused@*/ /*@newref@*/ /*@null@*/
rpmpsm rpmpsmLink (/*@null@*/ rpmpsm psm, /*@null@*/ const char * msg)
	/*@modifies psm @*/;
#define	rpmpsmLink(_psm, _msg)	\
    ((rpmpsm)rpmioLinkPoolItem((rpmioItem)(_psm), _msg, __FILE__, __LINE__))

/**
 * Destroy a package state machine.
 * @param psm		package state machine
 * @param msg
 * @return		NULL on last dereference
 */
/*@null@*/
rpmpsm rpmpsmFree(/*@killref@*/ /*@only@*/ /*@null@*/ rpmpsm psm,
		/*@null@*/ const char * msg)
	/*@globals fileSystem @*/
	/*@modifies psm, fileSystem @*/;
#define	rpmpsmFree(_psm, _msg)	\
    ((rpmpsm)rpmioFreePoolItem((rpmioItem)(_psm), _msg, __FILE__, __LINE__))

/**
 * Create and load a package state machine.
 * @param ts		transaction set
 * @param te		transaction set element
 * @param fi		file info set
 * @return		new package state machine
 */
/*@null@*/
rpmpsm rpmpsmNew(rpmts ts, /*@null@*/ rpmte te, rpmfi fi)
	/*@modifies ts, fi @*/;

/**
 * Package state machine driver.
 * @param psm		package state machine data
 * @param stage		next stage
 * @return		0 on success
 */
rpmRC rpmpsmStage(rpmpsm psm, pkgStage stage)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext, fileSystem, internalState @*/;
#define	rpmpsmUNSAFE	rpmpsmSTAGE

/**
 * Run rpmpsmStage(PSM_SCRIPT) for scriptTag and progTag
 * @param psm		package state machine data
 * @param scriptTag	scriptlet tag to execute
 * @param progTag	scriptlet prog tag to execute
 * @return 		0 on success
 */
rpmRC rpmpsmScriptStage(rpmpsm psm, rpmTag scriptTag, rpmTag progTag)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies psm, rpmGlobalMacroContext, fileSystem, internalState @*/;

void rpmpsmSetAsync(rpmpsm psm, int async)
	/*@modifies psm @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_PSM */
