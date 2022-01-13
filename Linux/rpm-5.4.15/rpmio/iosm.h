#ifndef H_IOSM
#define H_IOSM

/** \ingroup payload
 * \file rpmio/iosm.h
 * File state machine to handle archive I/O and system call's.
 */

#define	_IOSM_INTERNAL	/* XXX expose internals for unit tests. */
#include <rpmiotypes.h>
#include <rpmio.h>	/* XXX FD_t */
#include <rpmsw.h>

/** \ingroup payload
 * File state machine data.
 */
typedef /*@abstract@*/ struct iosm_s * IOSM_t;

/*@-exportlocal@*/
/*@unchecked@*/
extern int _iosm_debug;
/*@=exportlocal@*/

/**
 * File disposition(s) during package install/erase processing.
 */
typedef enum iosmFileAction_e {
    FA_UNKNOWN = 0,	/*!< initial action for file ... */
    FA_CREATE,		/*!< ... copy in from payload. */
    FA_COPYIN,		/*!< ... copy in from payload. */
    FA_COPYOUT,		/*!< ... copy out to payload. */
    FA_BACKUP,		/*!< ... renamed with ".rpmorig" extension. */
    FA_SAVE,		/*!< ... renamed with ".rpmsave" extension. */
    FA_SKIP, 		/*!< ... already replaced, don't remove. */
    FA_ALTNAME,		/*!< ... create with ".rpmnew" extension. */
    FA_ERASE,		/*!< ... to be removed. */
    FA_SKIPNSTATE,	/*!< ... untouched, state "not installed". */
    FA_SKIPNETSHARED,	/*!< ... untouched, state "netshared". */
    FA_SKIPCOLOR	/*!< ... untouched, state "wrong color". */
} iosmFileAction;

/** \ingroup payload
 */
typedef enum iosmMapFlags_e {
    IOSM_MAP_PATH	= (1 <<  0),
    IOSM_MAP_MODE	= (1 <<  1),
    IOSM_MAP_UID	= (1 <<  2),
    IOSM_MAP_GID	= (1 <<  3),
    IOSM_FOLLOW_SYMLINKS= (1 <<  4), /*!< only for building. */
    IOSM_MAP_ABSOLUTE	= (1 <<  5),
    IOSM_MAP_ADDDOT	= (1 <<  6),
    IOSM_ALL_HARDLINKS	= (1 <<  7), /*!< fail if hardlinks are missing. */
    IOSM_MAP_TYPE	= (1 <<  8), /*!< only for building. */
    IOSM_SBIT_CHECK	= (1 <<  9),
    IOSM_PAYLOAD_LIST	= (1 << 10),
    IOSM_PAYLOAD_EXTRACT= (1 << 11),
    IOSM_PAYLOAD_CREATE	= (1 << 12)
} iosmMapFlags;

#if defined(_IOSM_INTERNAL)
/** \ingroup payload
 * @note IOSM_CHECK_ERRNO bit is set only if errno is valid.
 */
#define IOSMERR_CHECK_ERRNO	0x00008000

/** \ingroup payload
 */
enum iosmErrorReturns_e {
	IOSMERR_BAD_MAGIC	= (2			),
	IOSMERR_BAD_HEADER	= (3			),
	IOSMERR_OPEN_FAILED	= (4    | IOSMERR_CHECK_ERRNO),
	IOSMERR_CHMOD_FAILED	= (5    | IOSMERR_CHECK_ERRNO),
	IOSMERR_CHOWN_FAILED	= (6    | IOSMERR_CHECK_ERRNO),
	IOSMERR_WRITE_FAILED	= (7    | IOSMERR_CHECK_ERRNO),
	IOSMERR_UTIME_FAILED	= (8    | IOSMERR_CHECK_ERRNO),
	IOSMERR_UNLINK_FAILED	= (9    | IOSMERR_CHECK_ERRNO),

	IOSMERR_RENAME_FAILED	= (10   | IOSMERR_CHECK_ERRNO),
	IOSMERR_SYMLINK_FAILED	= (11   | IOSMERR_CHECK_ERRNO),
	IOSMERR_STAT_FAILED	= (12   | IOSMERR_CHECK_ERRNO),
	IOSMERR_LSTAT_FAILED	= (13   | IOSMERR_CHECK_ERRNO),
	IOSMERR_MKDIR_FAILED	= (14   | IOSMERR_CHECK_ERRNO),
	IOSMERR_RMDIR_FAILED	= (15   | IOSMERR_CHECK_ERRNO),
	IOSMERR_MKNOD_FAILED	= (16   | IOSMERR_CHECK_ERRNO),
	IOSMERR_MKFIFO_FAILED	= (17   | IOSMERR_CHECK_ERRNO),
	IOSMERR_LINK_FAILED	= (18   | IOSMERR_CHECK_ERRNO),
	IOSMERR_READLINK_FAILED	= (19   | IOSMERR_CHECK_ERRNO),
	IOSMERR_READ_FAILED	= (20   | IOSMERR_CHECK_ERRNO),
	IOSMERR_COPY_FAILED	= (21   | IOSMERR_CHECK_ERRNO),
	IOSMERR_LSETFCON_FAILED	= (22   | IOSMERR_CHECK_ERRNO),
	IOSMERR_HDR_SIZE	= (23			),
	IOSMERR_HDR_TRAILER	= (24			),
	IOSMERR_UNKNOWN_FILETYPE= (25			),
	IOSMERR_MISSING_HARDLINK= (26			),
	IOSMERR_DIGEST_MISMATCH	= (27			),
	IOSMERR_INTERNAL	= (28			),
	IOSMERR_UNMAPPED_FILE	= (29			),
	IOSMERR_ENOENT		= (30			),
	IOSMERR_ENOTEMPTY	= (31			)
};
#endif

/**
 */
#define	IOSM_VERBOSE	0x8000
#define	IOSM_INTERNAL	0x4000
#define	IOSM_SYSCALL	0x2000
#define	IOSM_DEAD	0x1000

#define	_fv(_a)		((_a) | IOSM_VERBOSE)
#define	_fi(_a)		((_a) | IOSM_INTERNAL)
#define	_fs(_a)		((_a) | (IOSM_INTERNAL | IOSM_SYSCALL))
#define	_fd(_a)		((_a) | (IOSM_INTERNAL | IOSM_DEAD))

typedef enum iosmFileStage_e {
    IOSM_UNKNOWN =   0,
    IOSM_INIT	=  _fd(1),
    IOSM_PRE	=  _fd(2),
    IOSM_PROCESS=  _fv(3),
    IOSM_POST	=  _fd(4),
    IOSM_UNDO	=  5,
    IOSM_FINI	=  6,

    IOSM_PKGINSTALL	= _fd(7),
    IOSM_PKGERASE	= _fd(8),
    IOSM_PKGBUILD	= _fd(9),
    IOSM_PKGCOMMIT	= _fd(10),
    IOSM_PKGUNDO	= _fd(11),

    IOSM_CREATE	=  _fd(17),
    IOSM_MAP	=  _fd(18),
    IOSM_MKDIRS	=  _fi(19),
    IOSM_RMDIRS	=  _fi(20),
    IOSM_MKLINKS=  _fi(21),
    IOSM_NOTIFY	=  _fd(22),
    IOSM_DESTROY=  _fd(23),
    IOSM_VERIFY	=  _fd(24),
    IOSM_COMMIT	=  _fd(25),

    IOSM_UNLINK	=  _fs(33),
    IOSM_RENAME	=  _fs(34),
    IOSM_MKDIR	=  _fs(35),
    IOSM_RMDIR	=  _fs(36),
    IOSM_LSETFCON= _fs(39),
    IOSM_CHOWN	=  _fs(40),
    IOSM_LCHOWN	=  _fs(41),
    IOSM_CHMOD	=  _fs(42),
    IOSM_UTIME	=  _fs(43),
    IOSM_SYMLINK=  _fs(44),
    IOSM_LINK	=  _fs(45),
    IOSM_MKFIFO	=  _fs(46),
    IOSM_MKNOD	=  _fs(47),
    IOSM_LSTAT	=  _fs(48),
    IOSM_STAT	=  _fs(49),
    IOSM_READLINK= _fs(50),
    IOSM_CHROOT	=  _fs(51),

    IOSM_NEXT	=  _fd(65),
    IOSM_EAT	=  _fd(66),
    IOSM_POS	=  _fd(67),
    IOSM_PAD	=  _fd(68),
    IOSM_TRAILER=  _fd(69),
    IOSM_HREAD	=  _fd(70),
    IOSM_HWRITE	=  _fd(71),
    IOSM_DREAD	=  _fs(72),
    IOSM_DWRITE	=  _fs(73),

    IOSM_ROPEN	=  _fs(129),
    IOSM_READ	=  _fs(130),
    IOSM_RCLOSE	=  _fs(131),
    IOSM_WOPEN	=  _fs(132),
    IOSM_WRITE	=  _fs(133),
    IOSM_WCLOSE	=  _fs(134)
} iosmFileStage;
#undef	_fv
#undef	_fi
#undef	_fs
#undef	_fd

#if defined(_IOSM_INTERNAL)
/** \ingroup payload
 * Iterator across package file info, forward on install, backward on erase.
 */
typedef /*@abstract@*/ struct iosmIterator_s * IOSMI_t;

/** \ingroup payload
 * Keeps track of the set of all hard links to a file in an archive.
 */
struct hardLink_s {
/*@owned@*/ /*@relnull@*/
    struct hardLink_s * next;
/*@owned@*/
    const char ** nsuffix;
/*@owned@*/
    int * filex;
    struct stat sb;
    int nlink;
    int linksLeft;
    int linkIndex;
    int createdPath;
};

/** \ingroup payload
 * Iterator across package file info, forward on install, backward on erase.
 * @todo rpmts and rpmsx need to be moved elsewhere.
 */
struct iosmIterator_s {
    void * ts;			/*!< transaction set. */
    void * fi;			/*!< transaction element file info. */
    int reverse;		/*!< reversed traversal? */
    int isave;			/*!< last returned iterator index. */
    int i;			/*!< iterator index. */
};

/** \ingroup payload
 * File name and stat information.
 */
struct iosm_s {
/*@owned@*/ /*@relnull@*/
    const char * path;		/*!< Current file name. */
/*@owned@*/ /*@relnull@*/
    const char * lpath;		/*!< Current link name. */
/*@owned@*/ /*@relnull@*/
    const char * opath;		/*!< Original file name. */
/*@relnull@*/
    FD_t cfd;			/*!< Payload file handle. */
/*@relnull@*/
    FD_t rfd;			/*!<  read: File handle. */
/*@dependent@*/ /*@relnull@*/
    char * rdbuf;		/*!<  read: Buffer. */
/*@owned@*/ /*@relnull@*/
    char * rdb;			/*!<  read: Buffer allocated. */
    size_t rdsize;		/*!<  read: Buffer allocated size. */
    size_t rdlen;		/*!<  read: Number of bytes requested.*/
    size_t rdnb;		/*!<  read: Number of bytes returned. */
    FD_t wfd;			/*!< write: File handle. */
/*@dependent@*/ /*@relnull@*/
    char * wrbuf;		/*!< write: Buffer. */
/*@owned@*/ /*@relnull@*/
    char * wrb;			/*!< write: Buffer allocated. */
    size_t wrsize;		/*!< write: Buffer allocated size. */
    size_t wrlen;		/*!< write: Number of bytes requested.*/
    size_t wrnb;		/*!< write: Number of bytes returned. */
/*@only@*/ /*@relnull@*/
    IOSMI_t iter;		/*!< File iterator. */
    int ix;			/*!< Current file iterator index. */
/*@only@*/ /*@relnull@*/
    struct hardLink_s * links;	/*!< Pending hard linked file(s). */
/*@only@*/ /*@relnull@*/
    struct hardLink_s * li;	/*!< Current hard linked file(s). */
/*@kept@*/ /*@null@*/
    unsigned int * archiveSize;	/*!< Pointer to archive size. */
/*@kept@*/ /*@null@*/
    const char ** failedFile;	/*!< First file name that failed. */
/*@shared@*/ /*@relnull@*/
    const char * subdir;	/*!< Current file sub-directory. */
/*@unused@*/
    char subbuf[64];	/* XXX eliminate */
/*@observer@*/ /*@relnull@*/
    const char * osuffix;	/*!< Old, preserved, file suffix. */
/*@observer@*/ /*@relnull@*/
    const char * nsuffix;	/*!< New, created, file suffix. */
/*@shared@*/ /*@relnull@*/
    const char * suffix;	/*!< Current file suffix. */
    char sufbuf[64];	/* XXX eliminate */
/*@only@*/ /*@null@*/
    unsigned short * dnlx;	/*!< Last dirpath verified indexes. */
/*@only@*/ /*@null@*/
    char * ldn;			/*!< Last dirpath verified. */
    size_t ldnlen;		/*!< Last dirpath current length. */
    size_t ldnalloc;		/*!< Last dirpath allocated length. */
    int postpone;		/*!< Skip remaining stages? */
    int diskchecked;		/*!< Has stat(2) been performed? */
    int exists;			/*!< Does current file exist on disk? */
    int mkdirsdone;		/*!< Have "orphan" dirs been created? */
    size_t astriplen;		/*!< Length of buildroot prefix. */
    int rc;			/*!< External file stage return code. */
    int commit;			/*!< Commit synchronously? */
    int repackaged;		/*!< Is payload repackaged? */
    int strict_erasures;	/*!< Are Rmdir/Unlink failures errors? */
    int multithreaded;		/*!< Run stages on their own thread? */
    int adding;			/*!< Is the rpmte element type TR_ADDED? */
    int debug;			/*!< Print detailed operations? */
    int nofdigests;		/*!< Disable file digests? */
    int nofcontexts;		/*!< Disable file conexts? */
    iosmMapFlags mapFlags;	/*!< Bit(s) to control mapping. */
    rpmuint32_t fdigestalgo;	/*!< Digest algorithm (~= PGPHASHALGO_MD5) */
    rpmuint32_t digestlen;	/*!< No. of bytes in binary digest (~= 16) */
/*@shared@*/ /*@relnull@*/
    const char * dirName;	/*!< File directory name. */
/*@shared@*/ /*@relnull@*/
    const char * baseName;	/*!< File base name. */
/*@shared@*/ /*@relnull@*/
    const char * fdigest;	/*!< Hex digest (usually MD5, NULL disables). */
/*@shared@*/ /*@relnull@*/
    const unsigned char * digest;/*!< Bin digest (usually MD5, NULL disables). */
/*@dependent@*/ /*@observer@*/ /*@null@*/
    const char * fcontext;	/*!< File security context (NULL disables). */

    rpmuint32_t fflags;		/*!< File flags. */
    iosmFileAction action;	/*!< File disposition. */
    iosmFileStage goal;		/*!< Package state machine goal. */
    iosmFileStage stage;	/*!< External file stage. */
    iosmFileStage nstage;	/*!< Next file stage. */
    struct stat sb;		/*!< Current file stat(2) info. */
    struct stat osb;		/*!< Original file stat(2) info. */

    unsigned blksize;		/*!< Archive block size. */
    int (*headerRead) (void * _iosm, struct stat *st)
	/*@modifies _iosm, st @*/;
    int (*headerWrite) (void * _iosm, struct stat *st)
	/*@modifies _iosm, st @*/;
    int (*trailerWrite) (void * _iosm)
	/*@modifies _iosm @*/;

/*@null@*/
    char * lmtab;		/*!< ar(1) long member name table. */
    size_t lmtablen;		/*!< ar(1) no. bytes in lmtab. */
    size_t lmtaboff;		/*!< ar(1) current offset in lmtab. */

    struct rpmop_s op_digest;	/*!< RPMSW_OP_DIGEST accumulator. */
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*@-exportlocal@*/
/**
 * Return formatted string representation of file stages.
 * @param a		file stage
 * @return		formatted string
 */
/*@observer@*/ const char * iosmFileStageString(iosmFileStage a)	/*@*/;

/**
 * Return formatted string representation of file disposition.
 * @param a		file disposition
 * @return		formatted string
 */
/*@observer@*/ const char * iosmFileActionString(iosmFileAction a)	/*@*/;
/*@=exportlocal@*/

/** \ingroup payload
 * Return formatted error message on payload handling failure.
 * @param rc		error code
 * @return		(malloc'd) formatted error string
 */
/*@only@*/
char * iosmStrerror(int rc)
	/*@*/;

#if defined(_IOSM_INTERNAL)
/**
 * Create I/O state machine instance.
 * @return		I/O state machine
 */
/*@only@*/ IOSM_t newIOSM(void)
	/*@*/;

/**
 * Destroy I/O state machine instance.
 * @param iosm		I/O state machine
 * @return		always NULL
 */
/*@null@*/ IOSM_t freeIOSM(/*@only@*/ /*@null@*/ IOSM_t iosm)
	/*@modifies iosm @*/;
#endif

/**
 * Load external data into I/O state machine.
 * @param iosm		I/O state machine
 * @param goal
 * @param afmt		archive format (NULL uses cpio)
 * @param _ts		transaction set
 * @param _fi		transaction element file info
 * @param cfd		payload descriptor
 * @retval archiveSize	pointer to archive size
 * @retval failedFile	pointer to first file name that failed.
 * @return		0 on success
 */
int iosmSetup(IOSM_t iosm, iosmFileStage goal, /*@null@*/ const char * afmt,
		const void * _ts,
		const void * _fi,
		FD_t cfd,
		/*@out@*/ /*@null@*/ unsigned int * archiveSize,
		/*@out@*/ /*@null@*/ const char ** failedFile)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, _ts, _fi, cfd, *archiveSize, *failedFile,
		fileSystem, internalState @*/;

/**
 * Clean I/O state machine.
 * @param iosm		I/O state machine
 * @return		0 on success
 */
int iosmTeardown(IOSM_t iosm)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies iosm, fileSystem, internalState @*/;

#if defined(_IOSM_INTERNAL)
/*@-exportlocal@*/
/**
 * Retrieve transaction set from I/O state machine iterator.
 * @param iosm		I/O state machine
 * @return		transaction set
 */
void * iosmGetTs(const IOSM_t iosm)
	/*@*/;

/**
 * Retrieve transaction element file info from I/O state machine iterator.
 * @param iosm		I/O state machine
 * @return		transaction element file info
 */
void * iosmGetFi(/*@partial@*/ const IOSM_t iosm)
	/*@*/;

/**
 * Map next file path and action.
 * @param iosm		I/O state machine
 */
int iosmMapPath(IOSM_t iosm)
	/*@modifies iosm @*/;

/**
 * Map file stat(2) info.
 * @param iosm		I/O state machine
 */
int iosmMapAttrs(IOSM_t iosm)
	/*@modifies iosm @*/;
/*@=exportlocal@*/

/**
 * Vector to iosmNext.
 */
extern int (*_iosmNext) (IOSM_t iosm, iosmFileStage nstage)
	/*@modifies iosm @*/;
#endif

/**
 * Is the file going to be skipped?
 * @param action	file action
 * @return		Is file to be skipped?
 */
int iosmFileActionSkipped(iosmFileAction action)
	/*@*/;

/**
 * File state machine driver.
 * @param iosm		I/O state machine
 * @param nstage	next stage
 * @return		0 on success
 */
int iosmNext(IOSM_t iosm, iosmFileStage nstage)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies iosm, errno, fileSystem, internalState @*/;

/**
 * File state machine driver.
 * @param iosm		I/O state machine
 * @param stage		next stage
 * @return		0 on success
 */
/*@-exportlocal@*/
int iosmStage(/*@partial@*/ IOSM_t iosm, iosmFileStage stage)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies iosm, errno, fileSystem, internalState @*/;
/*@=exportlocal@*/

#ifdef __cplusplus
}
#endif

#endif	/* H_IOSM */
