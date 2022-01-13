/** \ingroup rpmio
 * \file rpmio/rpmio-stub.c
 */

#include "system.h"
#include <rpmio.h>
#include <rpmdir.h>
#include "rpmio-stub.h"
#include "debug.h"

/*@-redecl@*/
const char * (*_Fstrerror) (void * fd)
	= (const char *(*)(void *))Fstrerror;
size_t (*_Fread) (void * buf, size_t size, size_t nmemb, void * fd)
	= (size_t (*)(void *, size_t, size_t, void *))Fread;
size_t (*_Fwrite) (const void * buf, size_t size, size_t nmemb, void * fd)
	= (size_t (*)(const void * buf, size_t size, size_t nmemb, void * fd))Fwrite;
int (*_Fseek) (void * fd, _libio_off_t offset, int whence)
	= (int (*) (void *, _libio_off_t, int))Fseek;
int (*_Fclose) (void * fd)
	= (int (*) (void *))Fclose;
void * (*_Fdopen) (void * ofd, const char * fmode)
	= (void * (*) (void *, const char *))Fdopen;
void * (*_Fopen) (const char * path, const char * fmode)
	= (void * (*) (const char *, const char *))Fopen;
int (*_Fflush) (void * fd)
	= (int (*) (void *))Fflush;
int (*_Ferror) (void * fd)
	= (int (*) (void *))Ferror;
int (*_Fileno) (void * fd)
	= (int (*) (void *))Fileno;
int (*_Fcntl) (void * fd, int op, void *lip)
	= (int (*) (void *, int, void *))Fcntl;

int (*_Mkdir) (const char * path, mode_t mode) = Mkdir;
int (*_Chdir) (const char * path) = Chdir;
int (*_Rmdir) (const char * path) = Rmdir;
int (*_Chroot) (const char * path) = Chroot;
int (*_Open) (const char * path, int flags, mode_t mode) = Open;
int (*_Rename) (const char * oldpath, const char * newpath) = Rename;
int (*_Link) (const char * oldpath, const char * newpath) = Link;
int (*_Unlink) (const char * path) = Unlink;
/*@-type@*/
int (*_Stat) (const char * path, struct stat * st) = Stat;
int (*_Lstat) (const char * path, struct stat * st) = Lstat;
int (*_Fstat) (void * fd, struct stat * st)
	= (int (*) (void *, struct stat * st))Fstat;
/*@=type@*/
int (*_Chown) (const char * path, uid_t owner, gid_t group) = Chown;
int (*_Fchown) (void * fd, uid_t owner, gid_t group)
	= (int (*) (void *, uid_t, gid_t)) Fchown;
int (*_Lchown) (const char * path, uid_t owner, gid_t group) = Lchown;
int (*_Chflags) (const char * path, unsigned int flags) = Chflags;
int (*_Lchflags) (const char * path, unsigned int flags) = Lchflags;
int (*_Fchflags) (void * fd, unsigned int flags)
	= (int (*) (void *, unsigned int))Fchflags;
int (*_Chmod) (const char * path, mode_t mode) = Chmod;
int (*_Lchmod) (const char * path, mode_t mode) = Lchmod;
int (*_Fchmod) (void * fd, mode_t mode)
	= (int (*) (void *, mode_t))Fchmod;
int (*_Mkfifo) (const char * path, mode_t mode) = Mkfifo;
int (*_Mknod) (const char * path, mode_t mode, dev_t dev) = Mknod;
int (*_Utime) (const char * path, const struct utimbuf * buf) = Utime;
int (*_Utimes) (const char * path, const struct timeval * times) = Utimes;
int (*_Lutimes) (const char * path, const struct timeval * times) = Lutimes;
int (*_Symlink) (const char * oldpath, const char * newpath) = Symlink;
/*@-type@*/
int (*_Readlink) (const char * path, char * buf, size_t bufsiz) = Readlink;
/*@=type@*/
int (*_Access) (const char * path, int amode) = Access;

int (*_Glob_pattern_p) (const char *pattern, int quote) = Glob_pattern_p;
int (*_Glob_error) (const char * epath, int eerrno) = Glob_error;
/*@-type@*/
int (*_Glob) (const char * pattern, int flags,
		int errfunc(const char * epath, int eerrno),
		glob_t * pglob) = Glob;
void (*_Globfree) (glob_t * pglob) = Globfree;
/*@=type@*/

int (*_Closedir) (DIR * dir) = Closedir;
DIR * (*_Opendir) (const char * path) = Opendir;
struct dirent * (*_Readdir) (DIR * dir) = Readdir;
void (*_Rewinddir) (DIR *dir) = Rewinddir;
void (*_Scandir) (const char * path, struct dirent *** nl,
                int (*filter) (const struct dirent *),
                int (*compar) (const void *, const void *)) = Scandir;
void (*_Seekdir) (DIR *dir, off_t offset) = Seekdir;
off_t (*_Telldir) (DIR *dir) = Telldir;

/*@-type@*/
char * (*_Realpath) (const char * path, char * resolved_path) = Realpath;
/*@=type@*/
off_t (*_Lseek) (int fdno, off_t offset, int whence) = Lseek;
/*@=redecl@*/
