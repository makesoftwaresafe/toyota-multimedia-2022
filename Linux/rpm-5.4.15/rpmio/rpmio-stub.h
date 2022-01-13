#ifndef	H_RPMIO_STUB
#define	H_RPMIO_STUB

/** \ingroup rpmio
 * \file rpmio/rpmio-stub.h
 */

#ifdef __cplusplus
extern "C" {
#endif

extern const char * (*_Fstrerror) (void * fd);
extern size_t (*_Fread) (void * buf, size_t size, size_t nmemb, void * fd);
extern size_t (*_Fwrite) (const void * buf, size_t size, size_t nmemb, void * fd);
extern int (*_Fseek) (void * fd, _libio_off_t offset, int whence);
extern int (*_Fclose) (void * fd);
extern void * (*_Fdopen) (void * ofd, const char * fmode);
extern void * (*_Fopen) (const char * path, const char * fmode);
extern int (*_Fflush) (void * fd);
extern int (*_Ferror) (void * fd);
extern int (*_Fileno) (void * fd);
extern int (*_Fcntl) (void * fd, int op, void *lip);

extern int (*_Mkdir) (const char * path, mode_t mode);
extern int (*_Chdir) (const char * path);
extern int (*_Rmdir) (const char * path);
extern int (*_Chroot) (const char * path);
extern int (*_Open) (const char * path, int flags, mode_t mode);
extern int (*_Rename) (const char * oldpath, const char * newpath);
extern int (*_Link) (const char * oldpath, const char * newpath);
extern int (*_Unlink) (const char * path);
extern int (*_Stat) (const char * path, struct stat * st);
extern int (*_Lstat) (const char * path, struct stat * st);
extern int (*_Fstat) (void * fd, struct stat * st);
extern int (*_Chown) (const char * path, uid_t owner, gid_t group);
extern int (*_Fchown) (void * fd, uid_t owner, gid_t group);
extern int (*_Lchown) (const char * path, uid_t owner, gid_t group);
extern int (*_Chflags) (const char * path, unsigned int flags);
extern int (*_Lflags) (const char * path, unsigned int flags);
extern int (*_Fflags) (void * fd, unsigned int flags);
extern int (*_Chmod) (const char * path, mode_t mode);
extern int (*_Lchmod) (const char * path, mode_t mode);
extern int (*_Fchmod) (void * fd, mode_t mode);
extern int (*_Mkfifo) (const char * path, mode_t mode);
extern int (*_Mknod) (const char * path, mode_t mode, dev_t dev);
extern int (*_Utime) (const char * path, const struct utimbuf * buf);
extern int (*_Utimes) (const char * path, const struct timeval * times);
extern int (*_Lutimes) (const char * path, const struct timeval * times);
extern int (*_Symlink) (const char * oldpath, const char * newpath);
extern int (*_Readlink) (const char * path, char * buf, size_t bufsiz);
extern int (*_Access) (const char * path, int amode);
extern int (*_Glob_pattern_p) (const char *pattern, int quote);
extern int (*_Glob_error) (const char * epath, int eerrno);
extern int (*_Glob) (const char * pattern, int flags,
		int errfunc(const char * epath, int eerrno),
		glob_t * pglob);
extern void (*_Globfree) (glob_t * pglob);

extern int (*_Closedir) (DIR * dir);
extern DIR * (*_Opendir) (const char * path);
extern struct dirent * (*_Readdir) (DIR * dir);
extern void (*_Rewinddir) (DIR *dir);
extern void (*_Scandir) (const char * path, struct dirent *** nl,
                int (*filter) (const struct dirent *),
                int (*compar) (const void *, const void *));
extern void (*_Seekdir) (DIR *dir, off_t offset);
extern off_t (*_Telldir) (DIR *dir);

extern char * (*_Realpath) (const char * path, char * resolved_path);
extern off_t (*_Lseek) (int fdno, off_t offset, int whence);

#ifdef __cplusplus
}
#endif

#endif	/* H_RPMIO_STUB */
