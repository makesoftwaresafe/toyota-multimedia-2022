#ifndef H_RPMFI
#define H_RPMFI

/** \ingroup rpmfi
 * \file lib/rpmfi.h
 * Structure(s) used for file info tag sets.
 */

/*@-exportlocal@*/
/*@unchecked@*/
extern int _rpmfi_debug;
/*@=exportlocal@*/

/** \ingroup rpmfi
 * File types.
 * These are the file types used internally by rpm. The file
 * type is determined by applying stat(2) macros like S_ISDIR to
 * the file mode tag from a header. The values are arbitrary,
 * but are identical to the linux stat(2) file types.
 */
typedef enum rpmFileTypes_e {
    PIPE	=  1,	/*!< pipe/fifo */
    CDEV	=  2,	/*!< character device */
    XDIR	=  4,	/*!< directory */
    BDEV	=  6,	/*!< block device */
    REG		=  8,	/*!< regular file */
    LINK	= 10,	/*!< hard link */
    SOCK	= 12	/*!< socket */
} rpmFileTypes;

/**
 * File States (when installed).
 */
typedef enum rpmfileState_e {
    RPMFILE_STATE_NORMAL 	= 0,
    RPMFILE_STATE_REPLACED 	= 1,
    RPMFILE_STATE_NOTINSTALLED	= 2,
    RPMFILE_STATE_NETSHARED	= 3,
    RPMFILE_STATE_WRONGCOLOR	= 4
} rpmfileState;
#define	RPMFILE_STATE_MISSING	-1	/* XXX used for unavailable data */

/**
 * File Attributes.
 */
typedef	enum rpmfileAttrs_e {
/*@-enummemuse@*/
    RPMFILE_NONE	= 0,
/*@=enummemuse@*/
    RPMFILE_CONFIG	= (1 <<  0),	/*!< from %%config */
    RPMFILE_DOC		= (1 <<  1),	/*!< from %%doc */
    RPMFILE_ICON	= (1 <<  2),	/*!< from Icon: */
    RPMFILE_MISSINGOK	= (1 <<  3),	/*!< from %%config(missingok) */
    RPMFILE_NOREPLACE	= (1 <<  4),	/*!< from %%config(noreplace) */
    RPMFILE_SPECFILE	= (1 <<  5),	/*!< the specfile (srpm only). */
    RPMFILE_GHOST	= (1 <<  6),	/*!< from %%ghost */
    RPMFILE_LICENSE	= (1 <<  7),	/*!< from %%license */
    RPMFILE_README	= (1 <<  8),	/*!< from %%readme */
    RPMFILE_EXCLUDE	= (1 <<  9),	/*!< from %%exclude, internal */
    RPMFILE_UNPATCHED	= (1 << 10),	/*!< (deprecated) placeholder (SuSE) */
    RPMFILE_PUBKEY	= (1 << 11),	/*!< from %%pubkey */
    RPMFILE_POLICY	= (1 << 12),	/*!< from %%policy */
    RPMFILE_EXISTS	= (1 << 13),	/*!< did lstat(fn, st) succeed? */
    RPMFILE_SPARSE	= (1 << 14),	/*!< was ((512*st->st_blocks) < st->st_size) ? */
    RPMFILE_TYPED	= (1 << 15),	/*!< (unimplemented) from %%spook */
    RPMFILE_SOURCE	= (1 << 16),	/*!< from SourceN: (srpm only). */
    RPMFILE_PATCH	= (1 << 17),	/*!< from PatchN: (srpm only). */
    RPMFILE_OPTIONAL	= (1 << 18),	/*!< from %%optional. */
    RPMFILE_REMOVE	= (1 << 19)	/*!< remove file (after %post). */
} rpmfileAttrs;

#define	RPMFILE_SPOOK	(RPMFILE_GHOST|RPMFILE_TYPED)
#define	RPMFILE_ALL	~(RPMFILE_NONE)

/** \ingroup rpmfi
 * File info tag sets from a header, so that a header can be discarded early.
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct rpmfi_s * rpmfi;

#if defined(_RPMFI_INTERNAL)

#include "mire.h"

/** \ingroup rpmfi
 * A package filename set.
 */
struct rpmfi_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    int i;			/*!< Current file index. */
    int j;			/*!< Current directory index. */

/*@observer@*/
    const char * Type;		/*!< Tag name. */

    rpmTag tagN;		/*!< Header tag. */
/*@refcounted@*/ /*@null@*/
    Header h;			/*!< Header for file info set (or NULL) */

/*@only@*/ /*@relnull@*/
    const char ** bnl;		/*!< Base name(s) (from header) */
/*@only@*/ /*@relnull@*/
    const char ** dnl;		/*!< Directory name(s) (from header) */

/*@only@*/ /*@relnull@*/
    const char ** fdigests;	/*!< File digest(s) (from header) */
/*@only@*/ /*@null@*/
    rpmuint32_t * fdigestalgos;	/*!< File digest algorithm(s) (from header) */
/*@only@*/ /*@relnull@*/
    const char ** flinks;	/*!< File link(s) (from header) */
/*@only@*/ /*@null@*/
    const char ** flangs;	/*!< File lang(s) (from header) */

/*@only@*/ /*@relnull@*/
          rpmuint32_t * dil;	/*!< Directory indice(s) (from header) */
/*@only@*/ /*@relnull@*/
    const rpmuint32_t * fflags;	/*!< File flag(s) (from header) */
/*@only@*/ /*@relnull@*/
    const rpmuint32_t * fsizes;	/*!< File size(s) (from header) */
/*@only@*/ /*@relnull@*/
    const rpmuint32_t * fmtimes;/*!< File modification time(s) (from header) */
/*@only@*/ /*@relnull@*/
          rpmuint16_t * fmodes;	/*!< File mode(s) (from header) */
/*@only@*/ /*@relnull@*/
    const rpmuint16_t * frdevs;	/*!< File rdev(s) (from header) */
/*@only@*/ /*@relnull@*/
    const rpmuint32_t * finodes;/*!< File inodes(s) (from header) */

/*@only@*/ /*@null@*/
    const char ** fuser;	/*!< File owner(s) (from header) */
/*@only@*/ /*@null@*/
    const char ** fgroup;	/*!< File group(s) (from header) */

/*@only@*/ /*@null@*/
    rpmuint8_t * fstates;	/*!< File state(s) (from header) */

/*@only@*/ /*@null@*/
    const rpmuint32_t * fcolors;/*!< File color bits (header) */

/*@only@*/ /*@null@*/
    const char ** fcaps;	/*! File capabilities. */

/*@only@*/ /*@null@*/
    const char ** fcontexts;	/*! File security contexts. */

/*@only@*/ /*@null@*/
    const char ** cdict;	/*!< File class dictionary (header) */
    rpmuint32_t ncdict;		/*!< No. of class entries. */
/*@only@*/ /*@null@*/
    const rpmuint32_t * fcdictx;/*!< File class dictionary index (header) */

/*@only@*/ /*@null@*/
    const rpmuint32_t * ddict;	/*!< File depends dictionary (header) */
    rpmuint32_t nddict;		/*!< No. of depends entries. */
/*@only@*/ /*@null@*/
    const rpmuint32_t * fddictx;/*!< File depends dictionary start (header) */
/*@only@*/ /*@null@*/
    const rpmuint32_t * fddictn;/*!< File depends dictionary count (header) */

/*@only@*/ /*@relnull@*/
    const rpmuint32_t * vflags;	/*!< File verify flag(s) (from header) */

    rpmuint32_t dc;		/*!< No. of directories. */
    rpmuint32_t fc;		/*!< No. of files. */

/*=============================*/
/*@dependent@*/ /*@relnull@*/
    void * te;

/*@only@*/
    void * _fnbf;		/*!< File paths Bloom filter. */
/*@only@*/ /*@null@*/
    miRE exclude;		/*!< Iterator exclude patterns. */
    int nexclude;		/*!< No. of exclude patterns. */
/*@only@*/ /*@null@*/
    miRE include;		/*!< Iterator include patterns. */
    int ninclude;		/*!< No. of include patterns. */

/*-----------------------------*/
    uid_t uid;			/*!< File uid (default). */
    gid_t gid;			/*!< File gid (default). */
    rpmuint32_t flags;		/*!< File flags (default). */
    int action;			/*!< File disposition (default). */
/*@owned@*/ /*@relnull@*/
    int * actions;		/*!< File disposition(s). */
/*@owned@*/
    struct fingerPrint_s * fps;	/*!< File fingerprint(s). */
/*@owned@*/
    const char ** obnl;		/*!< Original basename(s) (from header) */
/*@owned@*/
    const char ** odnl;		/*!< Original dirname(s) (from header) */
/*@unused@*/
    rpmuint32_t * odil;		/*!< Original dirindex(s) (from header) */

/*@only@*/ /*@relnull@*/
    unsigned char * digests;	/*!< File digest(s) in binary. */
    rpmuint32_t digestalgo;	/*!< File digest algorithm. */
    rpmuint32_t digestlen;	/*!< No. bytes in binary digest. */

/*@only@*/ /*@relnull@*/
    const char * pretrans;
/*@only@*/ /*@relnull@*/
    const char * pretransprog;
/*@only@*/ /*@relnull@*/
    const char * posttrans;
/*@only@*/ /*@relnull@*/
    const char * posttransprog;
/*@only@*/ /*@relnull@*/
    const char * verifyscript;
/*@only@*/ /*@relnull@*/
    const char * verifyscriptprog;

/*@only@*/ /*@null@*/
    char * fn;			/*!< File name buffer, fnlen + 1 bytes. */
    size_t fnlen;		/*!< Maximum file name length (without NUL). */

    size_t astriplen;
    size_t striplen;
    unsigned long long archivePos;
    unsigned long long archiveSize;
    mode_t dperms;		/*!< Directory perms (0755) if not mapped. */
    mode_t fperms;		/*!< File perms (0644) if not mapped. */
/*@only@*/ /*@null@*/
    const char ** apath;
    int mapflags;
/*@owned@*/ /*@null@*/
    int * fmapflags;
/*@owned@*/
    void * fsm;			/*!< File state machine data. */
    rpmuint32_t color;		/*!< Color bit(s) from file color union. */

    int isSource;		/*!< Is this a SRPM? */

/*@owned@*/
    rpmuint32_t * replacedSizes;/*!< (TR_ADDED) */

    unsigned int record;	/*!< (TR_REMOVED) */
    int magic;
#define	RPMFIMAGIC	0x09697923
/*=============================*/
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#endif	/* _RPMFI_INTERNAL */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_RPMFI_NOMETHODS)
/** \name RPMFI */
/*@{*/

/**
 * Unreference a file info set instance.
 * @param fi		file info set
 * @param msg
 * @return		NULL on last dereference
 */
/*@unused@*/ /*@null@*/
rpmfi rpmfiUnlink (/*@killref@*/ /*@only@*/ /*@null@*/ rpmfi fi,
		/*@null@*/ const char * msg)
	/*@modifies fi @*/;
#define	rpmfiUnlink(_fi, _msg)	\
    ((rpmfi) rpmioUnlinkPoolItem((rpmioItem)(_fi), _msg, __FILE__, __LINE__))

/**
 * Reference a file info set instance.
 * @param fi		file info set
 * @param msg
 * @return		new file info set reference
 */
/*@unused@*/ /*@null@*/
rpmfi rpmfiLink (/*@null@*/ rpmfi fi, /*@null@*/ const char * msg)
	/*@modifies fi @*/;
#define	rpmfiLink(_fi, _msg)	\
    ((rpmfi) rpmioLinkPoolItem((rpmioItem)(_fi), _msg, __FILE__, __LINE__))

/**
 * Return file count from file info set.
 * @param fi		file info set
 * @return		current file count
 */
int rpmfiFC(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file index from file info set.
 * @param fi		file info set
 * @return		current file index
 */
/*@unused@*/
int rpmfiFX(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Set current file index in file info set.
 * @param fi		file info set
 * @param fx		new file index
 * @return		current file index
 */
/*@unused@*/
int rpmfiSetFX(/*@null@*/ rpmfi fi, int fx)
	/*@modifies fi @*/;

/**
 * Return directory count from file info set.
 * @param fi		file info set
 * @return		current directory count
 */
int rpmfiDC(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current directory index from file info set.
 * @param fi		file info set
 * @return		current directory index
 */
int rpmfiDX(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Set current directory index in file info set.
 * @param fi		file info set
 * @param dx		new directory index
 * @return		current directory index
 */
int rpmfiSetDX(/*@null@*/ rpmfi fi, int dx)
	/*@modifies fi @*/;

/**
 * Return source rpm marker from file info set.
 * @param fi		file info set
 * @return		source rpm?
 */
int rpmfiIsSource(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current base name from file info set.
 * @param fi		file info set
 * @return		current base name, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const char * rpmfiBN(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current directory name from file info set.
 * @param fi		file info set
 * @return		current directory, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const char * rpmfiDN(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file name from file info set.
 * @param fi		file info set
 * @return		current file name
 */
/*@observer@*/
extern const char * rpmfiFN(/*@null@*/ rpmfi fi)
	/*@modifies fi @*/;

/**
 * Return maximum file name length from file info set.
 * @param fi		file info set
 * @return		maximum file name length (not including NUL)
 */
extern size_t rpmfiFNMaxLen(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file flags from file info set.
 * @param fi		file info set
 * @return		current file flags, 0 on invalid
 */
rpmuint32_t rpmfiFFlags(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Set current file flags in file info set.
 * @param fi		file info set
 * @param FFlags	new file flags
 * @return		previous file flags, 0 on invalid
 */
rpmuint32_t rpmfiSetFFlags(/*@null@*/ rpmfi fi, rpmuint32_t FFlags)
	/*@modifies fi @*/;

/**
 * Return current file verify flags from file info set.
 * @param fi		file info set
 * @return		current file verify flags, 0 on invalid
 */
rpmuint32_t rpmfiVFlags(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Set current file verify flags in file info set.
 * @param fi		file info set
 * @param VFlags	new file verify flags
 * @return		previous file verify flags, 0 on invalid
 */
rpmuint32_t rpmfiSetVFlags(/*@null@*/ rpmfi fi, rpmuint32_t VFlags)
	/*@modifies fi @*/;

/**
 * Return current file mode from file info set.
 * @param fi		file info set
 * @return		current file mode, 0 on invalid
 */
rpmuint16_t rpmfiFMode(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file state from file info set.
 * @param fi		file info set
 * @return		current file state, 0 on invalid
 */
rpmfileState rpmfiFState(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Set current file state in file info set.
 * @param fi		file info set
 * @param fstate	new file state
 * @return		previous file state, 0 on invalid
 */
rpmfileState rpmfiSetFState(/*@null@*/ rpmfi fi, rpmfileState fstate)
	/*@modifies fi @*/;

/**
 * Return current file (binary) digest from file info set.
 * @param fi		file info set
 * @retval *algop	digest algorithm
 * @retval *lenp	digest length (in bytes)
 * @return		current file digest, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const unsigned char * rpmfiDigest(/*@null@*/ rpmfi fi,
		/*@out@*/ /*@null@*/ int * algop,
		/*@out@*/ /*@null@*/ size_t * lenp)
	/*@modifies *algop, *lenp @*/;

/**
 * Return current file linkto (i.e. symlink(2) target) from file info set.
 * @param fi		file info set
 * @return		current file linkto, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const char * rpmfiFLink(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file size from file info set.
 * @param fi		file info set
 * @return		current file size, 0 on invalid
 */
rpmuint32_t rpmfiFSize(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file rdev from file info set.
 * @param fi		file info set
 * @return		current file rdev, 0 on invalid
 */
rpmuint16_t rpmfiFRdev(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file inode from file info set.
 * @param fi		file info set
 * @return		current file inode, 0 on invalid
 */
rpmuint32_t rpmfiFInode(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return union of all file color bits from file info set.
 * @param fi		file info set
 * @return		current color
 */
rpmuint32_t rpmfiColor(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file color bits from file info set.
 * @param fi		file info set
 * @return		current file color
 */
rpmuint32_t rpmfiFColor(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file class from file info set.
 * @param fi		file info set
 * @return		current file class, 0 on invalid
 */
/*@-exportlocal@*/
/*@observer@*/ /*@null@*/
extern const char * rpmfiFClass(/*@null@*/ rpmfi fi)
	/*@*/;
/*@=exportlocal@*/

/**
 * Return current file security context from file info set.
 * @param fi		file info set
 * @return		current file context, 0 on invalid
 */
/*@-exportlocal@*/
/*@observer@*/ /*@null@*/
extern const char * rpmfiFContext(/*@null@*/ rpmfi fi)
	/*@*/;
/*@=exportlocal@*/

/**
 * Return current file depends dictionary from file info set.
 * @param fi		file info set
 * @retval *fddictp	file depends dictionary array (or NULL)
 * @return		no. of file depends entries, 0 on invalid
 */
rpmuint32_t rpmfiFDepends(/*@null@*/ rpmfi fi,
		/*@out@*/ /*@null@*/ const rpmuint32_t ** fddictp)
	/*@modifies *fddictp @*/;

/**
 * Return (calculated) current file nlink count from file info set.
 * @param fi		file info set
 * @return		current file nlink count, 0 on invalid
 */
rpmuint32_t rpmfiFNlink(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file modify time from file info set.
 * @param fi		file info set
 * @return		current file modify time, 0 on invalid
 */
rpmuint32_t rpmfiFMtime(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file owner from file info set.
 * @param fi		file info set
 * @return		current file owner, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const char * rpmfiFUser(/*@null@*/ rpmfi fi)
	/*@*/;

/**
 * Return current file group from file info set.
 * @param fi		file info set
 * @return		current file group, NULL on invalid
 */
/*@observer@*/ /*@null@*/
extern const char * rpmfiFGroup(/*@null@*/ rpmfi fi)
	/*@*/;

/** \ingroup rpmfi
 * Return file path Bloom filter.
 * @param fi		file info set
 * @return		file path Bloom filter (NULL if not set)
 */
void * rpmfiBloomFN(/*@null@*/ const rpmfi fi)
	/*@*/;

/** \ingroup rpmfi
 * Return file info exclude patterns.
 * @param fi		file info set
 * @return		file info exclude patterns (NULL if not set)
 */
/*@null@*/
void * rpmfiExclude(/*@null@*/ const rpmfi fi)
	/*@*/;

/** \ingroup rpmfi
 * Return no. of file info exclude patterns.
 * @param fi		file info set
 * @return		file info exclude patterns (0 if not set)
 */
int rpmfiNExclude(/*@null@*/ const rpmfi fi)
	/*@*/;

/** \ingroup rpmfi
 * Return file info include patterns.
 * @param fi		file info set
 * @return		file info include patterns (NULL if not set)
 */
/*@null@*/
void * rpmfiInclude(/*@null@*/ const rpmfi fi)
	/*@*/;

/** \ingroup rpmfi
 * Return no. of file info include patterns.
 * @param fi		file info set
 * @return		file info include patterns (0 if not set)
 */
int rpmfiNInclude(/*@null@*/ const rpmfi fi)
	/*@*/;

#if defined(_RPMFI_INTERNAL)
#include "fprint.h"
/**
 * Return fingerprint from file info set.
 * @param fi		file info set
 * @param ix
 * @return		fingerprint, NULL on invalid
 */
/*@null@*/
struct fingerPrint_s * rpmfiFpsIndex(rpmfi fi, int ix)
	/*@*/;

void rpmfiFpLookup(rpmfi fi, fingerPrintCache fpc)
	/*@modifies fi, fpc @*/;
#endif

/**
 * Return FN Bloom filter from file info set.
 * @param fi		file info set
 * @return		FN Bloom filter, NULL on invalid
 */
/*@null@*/
void * rpmfiFNBF(rpmfi fi)
	/*@*/;

/**
 * Return next file iterator index.
 * @param fi		file info set
 * @return		file iterator index, -1 on termination
 */
int rpmfiNext(/*@null@*/ rpmfi fi)
	/*@modifies fi @*/;

/**
 * Initialize file iterator index.
 * @param fi		file info set
 * @param fx		file iterator index
 * @return		file info set
 */
/*@null@*/
rpmfi rpmfiInit(/*@null@*/ rpmfi fi, int fx)
	/*@modifies fi @*/;

/**
 * Return next directory iterator index.
 * @param fi		file info set
 * @return		directory iterator index, -1 on termination
 */
/*@unused@*/
int rpmfiNextD(/*@null@*/ rpmfi fi)
	/*@modifies fi @*/;

/**
 * Initialize directory iterator index.
 * @param fi		file info set
 * @param dx		directory iterator index
 * @return		file info set, NULL if dx is out of range
 */
/*@unused@*/ /*@null@*/
rpmfi rpmfiInitD(/*@null@*/ rpmfi fi, int dx)
	/*@modifies fi @*/;

/**
 * Link a header to a file info set.
 * @param fi		file info set
 * @param h		header
 * @return		0 always
 */
int rpmfiSetHeader(rpmfi fi, /*@null@*/ Header h)
	/*@modifies fi, h @*/;

/**
 * Destroy a file info set.
 * @param fi		file info set
 * @return		NULL on last dereference
 */
/*@null@*/
rpmfi rpmfiFree(/*@killref@*/ /*@only@*/ /*@null@*/ rpmfi fi)
	/*@globals fileSystem @*/
	/*@modifies fi, fileSystem @*/;
#define	rpmfiFree(_fi)	\
    ((rpmfi) rpmioFreePoolItem((rpmioItem)(_fi), __FUNCTION__, __FILE__, __LINE__))

/**
 * Create and load a file info set.
 * @param _ts		transaction set (NULL skips path relocation)
 * @param h		header
 * @param tagN		RPMTAG_BASENAMES
 * @param flags		scareMem(0x1), nofilter(0x2)
 * @return		new file info set
 */
/*@null@*/
rpmfi rpmfiNew(/*@null@*/ const void * _ts, Header h, rpmTag tagN, int flags)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies _ts, h, rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Return current stat(2) data from file info set.
 * @param fi		file info set
 * @retval *st		stat(2) data
 * @return		-1 on error, 0 on success
 */
int rpmfiFStat(rpmfi fi, /*@out@*/ struct stat * st)
	/*@modifies *st @*/;

/**
 * Return lstat(2) data of path from file info set.
 * @param fi		file info set
 * @param path		file path
 * @retval *st		stat(2) data
 * @return		-1 on error, 0 on success
 */
int rpmfiStat(rpmfi fi, const char * path, /*@out@*/ struct stat * st)
	/*@modifies fi, *st @*/;

/**
 * Return directory stream onto file info set.
 * @param fi		file info set
 * @param name		directory path
 * @return		NULL on error
 */
/*@null@*/
void * rpmfiOpendir(rpmfi fi, const char * name)
	/*@globals fileSystem, internalState @*/
	/*@modifies fi, fileSystem, internalState @*/;

/**
 * Retrieve file classes from header.
 *
 * This function is used to retrieve file classes from the header.
 * 
 * @param h		header
 * @retval *fclassp	array of file classes
 * @retval *fcp		number of files
 */
void rpmfiBuildFClasses(Header h,
		/*@out@*/ const char *** fclassp, /*@out@*/ rpmuint32_t * fcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fclassp, *fcp, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

#ifdef	DYING
/**
 * Retrieve file security contexts from header.
 *
 * This function is used to retrieve file contexts from the header.
 * 
 * @param h		header
 * @retval *fcontextp	array of file contexts
 * @retval *fcp		number of files
 */
void rpmfiBuildFContexts(Header h,
		/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fcontextp, *fcp, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Retrieve file security contexts from file system.
 *
 * This function is used to retrieve file contexts from the file system.
 * 
 * @param h		header
 * @retval *fcontextp	array of file contexts
 * @retval *fcp		number of files
 */
void rpmfiBuildFSContexts(Header h,
		/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fcontextp, *fcp, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/**
 * Retrieve file security contexts from policy RE's.
 *
 * This function is used to retrieve file contexts from policy RE's.
 * 
 * @param h		header
 * @retval *fcontextp	array of file contexts
 * @retval *fcp		number of files
 */
void rpmfiBuildREContexts(Header h,
		/*@out@*/ const char *** fcontextp, /*@out@*/ rpmuint32_t * fcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fcontextp, *fcp, rpmGlobalMacroContext,
		fileSystem, internalState @*/;
#endif

/**
 * Retrieve per-file dependencies from header.
 *
 * This function is used to retrieve per-file dependencies from the header.
 * 
 * @param h		header
 * @param tagN		RPMTAG_PROVIDENAME | RPMTAG_REQUIRENAME
 * @retval *fdepsp	array of file dependencies
 * @retval *fcp		number of files
 */
void rpmfiBuildFDeps(Header h, rpmTag tagN,
		/*@out@*/ const char *** fdepsp, /*@out@*/ rpmuint32_t * fcp)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies h, *fdepsp, *fcp,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/**
 * Return file info comparison.
 * @param afi		1st file info
 * @param bfi		2nd file info
 * @return		0 if identical
 */
int rpmfiCompare(const rpmfi afi, const rpmfi bfi)
	/*@*/;

/**
 * Return file disposition.
 * @param ofi		old file info
 * @param nfi		new file info
 * @param skipMissing	OK to skip missing files?
 * @return		file disposition
 */
int rpmfiDecideFate(const rpmfi ofi, rpmfi nfi, int skipMissing)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies nfi, fileSystem, internalState @*/;

/**
 * Return formatted string representation of package disposition.
 * @param fi		file info set
 * @return		formatted string
 */
/*@-redef@*/
/*@observer@*/
const char * rpmfiTypeString(rpmfi fi)
	/*@*/;
/*@=redef@*/

/**
 * Free relocation array.
 * @param relocs	relocation array
 * @return		NULL always
 */
/*@null@*/
rpmRelocation rpmfiFreeRelocations(/*@only@*/ rpmRelocation relocs)
	/*@modifies relocs@*/;

/**
 * Duplicate a relocation array.
 * @param relocs	relocation array
 * @retval *nrelocsp	no. of elements
 * @return		duplicate relocation array
 */
/*@only@*/ /*@null@*/
rpmRelocation rpmfiDupeRelocations(rpmRelocation relocs, int * nrelocsp)
	/*@modifies *nrelocsp @*/;

/**
 * Add relocation element to array.
 * @retval *relp	relocation array
 * @retval *nrelp	no. of elements
 * @param oldPath	old path
 * @param newPath	new path
 * @return		0 on success
 */
int rpmfiAddRelocation(rpmRelocation * relp, int * nrelp,
		const char * oldPath, const char * newPath)
	/*@modifies *relp, *nrelp @*/;

/*@}*/
#endif	/* _RPMFI_NOMETHODS */

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMDS */
