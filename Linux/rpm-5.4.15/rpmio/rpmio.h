#ifndef	H_RPMIO
#define	H_RPMIO

/** \ingroup rpmio
 * \file rpmio/rpmio.h
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rpmiotypes.h>
#include <rpmzlog.h>

/** \ingroup rpmio
 * Hide libio API lossage.
 * The libio interface changed after glibc-2.1.3 to pass the seek offset
 * argument as a pointer rather than as an off_t. The snarl below defines
 * typedefs to isolate the lossage.
 */
/*@{*/
#if !defined(__LCLINT__) && !defined(__UCLIBC__) && defined(__GLIBC__) && \
	(__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2))
#define USE_COOKIE_SEEK_POINTER 1
typedef _IO_off64_t 	_libio_off_t;
typedef _libio_off_t *	_libio_pos_t;
#else
typedef off_t 		_libio_off_t;
typedef off_t 		_libio_pos_t;
#endif
/*@}*/

/** \ingroup rpmio
 */
/*@unchecked@*/
extern int _rpmio_debug;

/** \ingroup rpmio
 */
typedef	/*@abstract@*/ /*@refcounted@*/ struct _FD_s * FD_t;

/** \ingroup rpmio
 */
typedef /*@observer@*/ struct FDIO_s * FDIO_t;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmio
 * \name RPMIO Vectors.
 */
/*@{*/

/**
 */
typedef ssize_t (*fdio_read_function_t) (void *cookie, char *buf, size_t nbytes)
	/*@globals errno, fileSystem @*/
	/*@modifies *cookie, errno, fileSystem @*/
	/*@requires maxSet(buf) >= (nbytes - 1) @*/
	/*@ensures maxRead(buf) == result @*/ ;

/**
 */
typedef ssize_t (*fdio_write_function_t) (void *cookie, const char *buf, size_t nbytes)
	/*@globals errno, fileSystem @*/
	/*@modifies *cookie, errno, fileSystem @*/;

/**
 */
typedef int (*fdio_seek_function_t) (void *cookie, _libio_pos_t pos, int whence)
	/*@globals errno, fileSystem @*/
	/*@modifies *cookie, errno, fileSystem @*/;

/**
 */
typedef int (*fdio_close_function_t) (void *cookie)
	/*@globals errno, fileSystem, systemState @*/
	/*@modifies *cookie, errno, fileSystem, systemState @*/;

/**
 */
typedef FD_t (*fdio_fopen_function_t) (const char * path, const char * fmode)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/**
 */
typedef FD_t (*fdio_fdopen_function_t) (void * cookie, const char * fmode)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/**
 */
typedef int (*fdio_flush_function_t) (void * cookie)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/*@}*/


/** \ingroup rpmio
 */
struct FDIO_s {
  fdio_read_function_t		read;
  fdio_write_function_t		write;
  fdio_seek_function_t		seek;
  fdio_close_function_t		close;
/*@null@*/
  fdio_fopen_function_t		_fopen;
/*@null@*/
  fdio_fdopen_function_t	_fdopen;
/*@null@*/
  fdio_flush_function_t		_flush;
};


/** \ingroup rpmio
 * \name RPMIO Interface.
 */
/*@{*/

/**
 * strerror(3) clone.
 */
/*@observer@*/ const char * Fstrerror(/*@null@*/ FD_t fd)
	/*@*/;

/**
 * fread(3) clone.
 */
/*@-incondefs@*/
size_t Fread(/*@out@*/ void * buf, size_t size, size_t nmemb, FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies fd, *buf, fileSystem @*/
	/*@requires maxSet(buf) >= (nmemb - 1) @*/;
/*@=incondefs@*/

/**
 * fwrite(3) clone.
 */
/*@-incondefs@*/
size_t Fwrite(const void * buf, size_t size, size_t nmemb, FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies fd, fileSystem @*/
	/*@requires maxRead(buf) >= nmemb @*/;
/*@=incondefs@*/

/**
 * fseek(3) clone.
 */
int Fseek(FD_t fd, _libio_off_t offset, int whence)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
long Ftell(FD_t fd)
	/*@*/;
void Rewind(FD_t fd)
	/*@*/;
int Fgetpos(FD_t fd, fpos_t *pos)
	/*@*/;
int Fsetpos(FD_t fd, fpos_t *pos)
	/*@*/;

/**
 * fclose(3) clone.
 */
int Fclose( /*@killref@*/ FD_t fd)
	/*@globals fileSystem, internalState @*/
	/*@modifies fd, fileSystem, internalState @*/;

/**
 */
/*@null@*/ FD_t	Fdopen(FD_t ofd, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies ofd, fileSystem, internalState @*/;

/**
 * fopen(3) clone.
 */
/*@null@*/ FD_t	Fopen(/*@null@*/ const char * path,
			/*@null@*/ const char * fmode)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;


/**
 * fflush(3) clone.
 */
int Fflush(/*@null@*/ FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies fd, fileSystem @*/;

/**
 * ferror(3) clone.
 */
int Ferror(/*@null@*/ FD_t fd)
	/*@*/;

/**
 * fileno(3) clone.
 */
int Fileno(FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies fileSystem@*/;

/**
 * fcntl(2) clone.
 */
/*@unused@*/
int Fcntl(FD_t fd, int op, void *lip)
	/*@globals errno, fileSystem @*/
	/*@modifies fd, *lip, errno, fileSystem @*/;

/*@}*/

/** \ingroup rpmrpc
 * \name RPMRPC Interface.
 */
/*@{*/

/**
 * mkdir(2) clone.
 */
int Mkdir(const char * path, mode_t mode)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * chdir(2) clone.
 */
int Chdir(const char * path)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * rmdir(2) clone.
 */
int Rmdir(const char * path)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/*@unchecked@*/ /*@observer@*/ /*@null@*/
extern const char * _chroot_prefix;

/**
 * chroot(2) clone.
 * @todo Implement remotely.
 */
int Chroot(const char * path)
	/*@globals _chroot_prefix, errno, fileSystem, internalState @*/
	/*@modifies _chroot_prefix, errno, fileSystem, internalState @*/;

/**
 * open(2) clone.
 * @todo Implement remotely.
 */
int Open(const char * path, int flags, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * rename(2) clone.
 */
int Rename(const char * oldpath, const char * newpath)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * link(2) clone.
 */
int Link(const char * oldpath, const char * newpath)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * unlink(2) clone.
 */
int Unlink(const char * path)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * stat(2) clone.
 */
int Stat(const char * path, /*@out@*/ struct stat * st)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies *st, errno, fileSystem, internalState @*/;

/**
 * lstat(2) clone.
 */
int Lstat(const char * path, /*@out@*/ struct stat * st)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies *st, errno, fileSystem, internalState @*/;

/**
 * fstat(2) clone.
 */
int Fstat(FD_t fd, /*@out@*/ struct stat * st)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies *st, errno, fileSystem, internalState @*/;

/**
 * posix_fadvise(2) clone.
 */
int Fadvise(FD_t fd, off_t offset, off_t length, int advice)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * posix_fallocate(3)/fallocate(2) clone.
 */
int Fallocate(FD_t fd, off_t offset, off_t length)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * chown(2) clone.
 * @todo Implement remotely.
 */
int Chown(const char * path, uid_t owner, gid_t group)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * fchown(2) clone.
 * @todo Implement remotely.
 */
int Fchown(FD_t fd, uid_t owner, gid_t group)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * lchown(2) clone.
 * @todo Implement remotely.
 */
int Lchown(const char * path, uid_t owner, gid_t group)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * chmod(2) clone.
 * @todo Implement remotely.
 */
int Chmod(const char * path, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * lchmod(2) clone.
 * @todo Implement remotely.
 */
int Lchmod(const char * path, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * fchmod(2) clone.
 * @todo Implement remotely.
 */
int Fchmod(FD_t fd, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * chflags(2) clone.
 * @todo Implement remotely.
 */
int Chflags(const char * path, unsigned int flags)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * lchflags(2) clone.
 * @todo Implement remotely.
 */
int Lchflags(const char * path, unsigned int flags)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * fchflags(2) clone.
 * @todo Implement remotely.
 */
int Fchflags(FD_t fd, unsigned int flags)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * mkfifo(3) clone.
 * @todo Implement remotely.
 */
int Mkfifo(const char * path, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * mknod(3) clone.
 * @todo Implement remotely.
 */
int Mknod(const char * path, mode_t mode, dev_t dev)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * utime(2) clone.
 * @todo Implement remotely.
 */
struct utimbuf;
int Utime(const char * path, const struct utimbuf * buf)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * utimes(2) clone.
 * @todo Implement remotely.
 */
int Utimes(const char * path, const struct timeval * times)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * lutimes(2) clone.
 * @todo Implement remotely.
 */
int Lutimes(const char * path, const struct timeval * times)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * symlink(3) clone.
 * @todo Implement remotely.
 */
int Symlink(const char * oldpath, const char * newpath)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * readlink(2) clone.
 * @todo Implement remotely.
 */
/*@-incondefs@*/
int Readlink(const char * path, /*@out@*/ char * buf, size_t bufsiz)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies *buf, errno, fileSystem, internalState @*/;
/*@=incondefs@*/

/**
 * access(2) clone.
 * @todo Implement remotely.
 */
int Access(const char * path, int amode)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

#if defined(__linux__)
/**
 * mount(2) clone.
 */
int Mount(const char *source, const char *target,
		const char *filesystemtype, unsigned long mountflags,
		const void *data)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/**
 * umount(2) clone.
 */
int Umount(const char *target)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/**
 * umount2(2) clone.
 */
int Umount2(const char *target, int flags)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;
#endif

/**
 * glob_pattern_p(3) clone.
 */
int Glob_pattern_p (const char *pattern, int quote)
	/*@*/;

/**
 * glob_error(3) clone.
 */
int Glob_error(const char * epath, int eerrno)
	/*@*/;

/**
 * glob(3) clone.
 */
int Glob(const char * pattern, int flags,
		int errfunc(const char * epath, int eerrno),
		/*@out@*/ void * _pglob)
	/*@globals fileSystem @*/
	/*@modifies *_pglob, fileSystem @*/;

/**
 * globfree(3) clone.
 */
void Globfree( /*@only@*/ void * _pglob)
	/*@globals fileSystem @*/
	/*@modifies *_pglob, fileSystem @*/;


/**
 * realpath(3) clone.
 */
/*@-globuse@*/
/*@null@*/
char * Realpath(const char * path, /*@out@*/ /*@null@*/ char * resolved_path)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies resolved_path, errno, fileSystem, internalState @*/;
/*@=globuse@*/

/**
 * lseek(2) clone.
 * @todo Implement SEEK_HOLE/SEEK_DATA.
 */
off_t Lseek(int fdno, off_t offset, int whence)
	/*@globals errno, fileSystem @*/
	/*@modifies errno, fileSystem @*/;

/*@}*/


/** \ingroup rpmio
 * \name RPMIO Utilities.
 */
/*@{*/

/**
 */
/*@null@*/ FD_t fdDup(int fdno)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/*@-exportlocal@*/
/**
 */
ssize_t fdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies *cookie, *buf, errno, fileSystem, internalState @*/;
#define	fdRead(_fd, _buf, _count)	fdio->read((_fd), (_buf), (_count))

/**
 */
ssize_t	fdWrite(void * cookie, const char * buf, size_t count)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies *cookie, errno, fileSystem, internalState @*/;
#define	fdWrite(_fd, _buf, _count)	fdio->write((_fd), (_buf), (_count))

/**
 */
int fdClose( /*@only@*/ void * cookie)
	/*@globals errno, fileSystem, systemState, internalState @*/
	/*@modifies *cookie, errno, fileSystem, systemState, internalState @*/;
#define	fdClose(_fd)		fdio->close(_fd)

/**
 */
/*@null@*/ FD_t fdOpen(const char *path, int flags, mode_t mode)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;
#define	fdOpen(_path, _flags, _mode)	fdio->_open((_path), (_flags), (_mode))

/**
 */
/*@unused@*/
/*@newref@*/ /*@null@*/
FD_t fdLink (void * cookie, const char * msg)
	/*@globals fileSystem @*/
	/*@modifies *cookie, fileSystem @*/;
#define	fdLink(_fd, _msg)	\
	((FD_t)rpmioLinkPoolItem((rpmioItem)(_fd), _msg, __FILE__, __LINE__))

/**
 */
/*@unused@*/ /*@null@*/
FD_t fdFree(/*@killref@*/ FD_t fd, const char * msg)
	/*@globals fileSystem @*/
	/*@modifies fd, fileSystem @*/;
#define	fdFree(_fd, _msg)	\
	((FD_t)rpmioFreePoolItem((rpmioItem)(_fd), _msg, __FILE__, __LINE__))

/**
 */
/*@unused@*/
/*@newref@*/ /*@null@*/
FD_t fdNew (const char * msg)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@newref@*/ /*@null@*/
FD_t XfdNew (const char * msg, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
#define	fdNew(_msg)		XfdNew(_msg, __FILE__, __LINE__)

/**
 */
int fdWritable(FD_t fd, int secs)
	/*@globals errno, fileSystem @*/
	/*@modifies fd, errno, fileSystem @*/;

/**
 */
int fdReadable(FD_t fd, int secs)
	/*@globals errno @*/
	/*@modifies fd, errno @*/;
/*@=exportlocal@*/

/**
 * Insure that directories in path exist, creating as needed.
 * @param path		directory path
 * @param mode		directory mode (if created)
 * @param uid		directory uid (if created), or -1 to skip
 * @param gid		directory uid (if created), or -1 to skip
 * @return		0 on success, errno (or -1) on error
 */
int rpmioMkpath(const char * path, mode_t mode, uid_t uid, gid_t gid)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Check FN access, expanding relative paths and twiddles.
 * @param FN		file path to check
 * @param path		colon separated search path (NULL uses $PATH)
 * @param mode		type of access(2) to check (0 uses X_OK)
 * @return		0 if accessible
 */
int rpmioAccess(const char *FN, /*@null@*/ const char * path, int mode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Return a password.
 * @param prompt	prompt string
 * @return		password
 */
extern char * (*Getpass) (const char * prompt)
	/*@*/;
char * _GetPass (const char * prompt)
	/*@*/;
char * _RequestPass (const char * prompt)
	/*@*/;

/**
 * FTP and HTTP error codes.
 */
/*@-typeuse@*/
typedef enum ftperrCode_e {
    FTPERR_NE_ERROR		= -1,	/*!< Generic error. */
    FTPERR_NE_LOOKUP		= -2,	/*!< Hostname lookup failed. */
    FTPERR_NE_AUTH		= -3,	/*!< Server authentication failed. */
    FTPERR_NE_PROXYAUTH		= -4,	/*!< Proxy authentication failed. */
    FTPERR_NE_CONNECT		= -5,	/*!< Could not connect to server. */
    FTPERR_NE_TIMEOUT		= -6,	/*!< Connection timed out. */
    FTPERR_NE_FAILED		= -7,	/*!< The precondition failed. */
    FTPERR_NE_RETRY		= -8,	/*!< Retry request. */
    FTPERR_NE_REDIRECT		= -9,	/*!< Redirect received. */

    FTPERR_BAD_SERVER_RESPONSE	= -81,	/*!< Bad server response */
    FTPERR_SERVER_IO_ERROR	= -82,	/*!< Server I/O error */
    FTPERR_SERVER_TIMEOUT	= -83,	/*!< Server timeout */
    FTPERR_BAD_HOST_ADDR	= -84,	/*!< Unable to lookup server host address */
    FTPERR_BAD_HOSTNAME		= -85,	/*!< Unable to lookup server host name */
    FTPERR_FAILED_CONNECT	= -86,	/*!< Failed to connect to server */
    FTPERR_FILE_IO_ERROR	= -87,	/*!< Failed to establish data connection to server */
    FTPERR_PASSIVE_ERROR	= -88,	/*!< I/O error to local file */
    FTPERR_FAILED_DATA_CONNECT	= -89,	/*!< Error setting remote server to passive mode */
    FTPERR_FILE_NOT_FOUND	= -90,	/*!< File not found on server */
    FTPERR_NIC_ABORT_IN_PROGRESS= -91,	/*!< Abort in progress */
    FTPERR_UNKNOWN		= -100	/*!< Unknown or unexpected error */
} ftperrCode;
/*@=typeuse@*/

/**
 */
/*@-redecl@*/
/*@observer@*/
const char * ftpStrerror(int errorNumber)
	/*@*/;
/*@=redecl@*/

/**
 */
/*@unused@*/
/*@dependent@*/ /*@null@*/
void * ufdGetUrlinfo(FD_t fd)
	/*@globals fileSystem @*/
	/*@modifies fd, fileSystem @*/;

/**
 */
/*@-redecl@*/
/*@unused@*/
/*@observer@*/
const char * urlStrerror(const char * url)
	/*@globals h_errno, internalState @*/
	/*@modifies internalState @*/;
/*@=redecl@*/

/**
 */
/*@-exportlocal@*/
int ufdCopy(FD_t sfd, FD_t tfd)
	/*@globals fileSystem @*/
	/*@modifies sfd, tfd, fileSystem @*/;
/*@=exportlocal@*/

/**
 */
int ufdGetFile( /*@killref@*/ FD_t sfd, FD_t tfd)
	/*@globals fileSystem, internalState @*/
	/*@modifies sfd, tfd, fileSystem, internalState @*/;

/*@-exportlocal@*/
/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t fdio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t fpio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t ufdio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t gzdio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t bzdio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t lzdio;

/**
 */
/*@observer@*/ /*@unchecked@*/ extern FDIO_t xzdio;

/*@=exportlocal@*/
/*@}*/

/*@unchecked@*/ /*@only@*/ /*@null@*/
extern rpmioPool _fdPool;

/**
 * Free all memory allocated by rpmio usage.
 */
void rpmioClean(void)
	/*@globals _fdPool, fileSystem, internalState @*/
	/*@modifies _fdPool, fileSystem, internalState @*/;

/**
 * Reclaim memory pool items.
 * @param pool		memory pool (NULL uses global rpmio pool)
 * @return		NULL always
 */
/*@null@*/
rpmioPool rpmioFreePool(/*@only@*//*@null@*/ rpmioPool pool)
	/*@globals fileSystem, internalState @*/
	/*@modifies pool, fileSystem, internalState @*/;

/**
 * Create a memory pool.
 * @param name		pool name
 * @param size		item size
 * @param limit		no. of items permitted (-1 for unlimited)
 * @param flags		debugging flags
 * @param (*dbg)()	generate string for Unlink/Link/Free debugging
 * @param (*init)()	create item contents
 * @param (*fini)()	destroy item contents
 * @return		memory pool
 */
rpmioPool rpmioNewPool(/*@observer@*/ const char * name,
		size_t size, int limit, int flags,
		/*@null@*/ char * (*dbg) (void *item), 
		/*@null@*/ void (*init) (void *item),
		/*@null@*/ void (*fini) (void *item))
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 * Decrement a pool item refcount.
 * @param item		pool item
 * @param msg		debugging msg (NULL disables debugging)
 * @param fn		usually __FILE__
 * @param ln		usually __LINE__
 * @return		pool item (NULL on last dereference)
 */
/*@null@*/
rpmioItem rpmioUnlinkPoolItem(/*@killref@*/ /*@null@*/ rpmioItem item,
		const char * msg, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies item, fileSystem @*/;

/**
 * Increment a pool item refcount.
 * @param item		pool item
 * @param msg		debugging msg (NULL disables debugging)
 * @param fn		usually __FILE__
 * @param ln		usually __LINE__
 * @return		pool item
 */
/*@newref@*/ /*@null@*/
rpmioItem rpmioLinkPoolItem(/*@returned@*/ /*@null@*/ rpmioItem item,
		const char * msg, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies item, fileSystem @*/;

/**
 * Free a pool item.
 * @param item		pool item
 * @param msg		debugging msg (NULL disables debugging)
 * @param fn		usually __FILE__
 * @param ln		usually __LINE__
 * @return		pool item (NULL on last dereference)
 */
/*@null@*/
void * rpmioFreePoolItem(/*@killref@*/ /*@null@*/ rpmioItem item,
		const char * msg, const char * fn, unsigned ln)
	/*@globals fileSystem @*/
	/*@modifies item, fileSystem @*/;

/**
 * Get unused item from pool, or alloc a new item.
 * @param pool		memory pool (NULL will always alloc a new item)
 * @param size		item size
 * @return		new item
 */
rpmioItem rpmioGetPool(/*@kept@*/ /*@null@*/ rpmioPool pool, size_t size)
	/*@globals fileSystem @*/
        /*@modifies pool, fileSystem @*/;

/**
 * Put unused item into pool (or free).
 * @param item		unused item
 * @return		NULL always
 */
/*@null@*/
rpmioItem rpmioPutPool(rpmioItem item)
	/*@globals fileSystem @*/
        /*@modifies item, fileSystem @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMIO */
