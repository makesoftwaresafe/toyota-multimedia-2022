/**
 * \file system.h
 */

#ifndef	H_SYSTEM
#define	H_SYSTEM

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#if defined(HAVE_SYS_BYTEORDER_H) && !defined(BYTE_ORDER)
/* construct Linux & BSD-compatible defines for Solaris and other
 * SVR4 systems that have sys/byteorder.h
 */
# define __LITTLE_ENDIAN 1234
# define __BIG_ENDIAN    4321
/* synonyms used in some places */
# define LITTLE_ENDIAN __LITTLE_ENDIAN
# define BIG_ENDIAN    __BIG_ENDIAN
# include <sys/byteorder.h>
# ifdef _LITTLE_ENDIAN
#  define BYTE_ORDER __LITTLE_ENDIAN
# elif _BIG_ENDIAN
#  define BYTE_ORDER __BIG_ENDIAN
# endif
#endif

#if defined(__LCLINT__)
/*@-redef@*/
typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;
/*@-incondefs@*/        /* LCLint 3.0.0.15 */
typedef int int32_t;
/*@=incondefs@*/
/* XXX from /usr/include/bits/sigset.h */
/*@-sizeoftype@*/
# define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))
typedef struct
  {
    unsigned long int __val[_SIGSET_NWORDS];
  } __sigset_t;
/*@=sizeoftype@*/
/*@=redef@*/
#endif

#include <sys/stat.h>
/* XXX retrofit the *BSD constant if not present. */
#if !defined(HAVE_S_ISTXT) && defined(HAVE_S_ISVTX)
#define	S_ISTXT	 S_ISVTX
#endif
#if !defined(HAVE_STRUCT_STAT_ST_BIRTHTIME)
#if (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)) && defined(st_birthtime)
#undef	st_birthtime
#endif
#define	st_birthtime	st_ctime	/* Use st_ctime if no st_birthtime. */
#endif
/* XXX retrofit the *BSD st_[acm]timespec names if not present. */
#if !defined(HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC) && defined(HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
#define	st_atimespec	st_atim
#define	st_ctimespec	st_ctim
#define	st_mtimespec	st_mtim
#endif

#include <stdio.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
/* XXX retrofit the *BSD constant if not present. */
#if !defined(MAXPHYS)
#define MAXPHYS         (128 * 1024)    /* max raw I/O transfer size */
#endif

/* <unistd.h> should be included before any preprocessor test
   of _POSIX_VERSION.  */
#ifdef HAVE_UNISTD_H
#define	uuid_t	unistd_uuid_t	/* XXX Mac OS X dares to be different. */
#include <unistd.h>
#undef	unistd_uuid_t		/* XXX Mac OS X dares to be different. */
#if defined(__LCLINT__)
/*@-superuser -declundef -incondefs @*/	/* LCL: modifies clause missing */
extern int chroot (const char *__path)
	/*@globals errno, systemState @*/
	/*@modifies errno, systemState @*/;
/*@=superuser =declundef =incondefs @*/
#endif
#if !defined(__GLIBC__) && !defined(__LCLINT__)
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char ** environ;
#endif /* __APPLE__ */
#endif
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* XXX retrofit the (POSIX? GNU? *BSD?) macros if not present. */
#if !defined(TIMEVAL_TO_TIMESPEC)
# define TIMEVAL_TO_TIMESPEC(tv, ts) { \
        (ts)->tv_sec = (tv)->tv_sec; \
        (ts)->tv_nsec = (tv)->tv_usec * 1000; \
}
#endif
#if !defined(TIMESPEC_TO_TIMEVAL)
# define TIMESPEC_TO_TIMEVAL(tv, ts) { \
        (tv)->tv_sec = (ts)->tv_sec; \
        (tv)->tv_usec = (ts)->tv_nsec / 1000; \
}
#endif

/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#define HAVE_MAJOR
#endif
#if defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#define HAVE_MAJOR
#endif
#ifdef major			/* Might be defined in sys/types.h.  */
#define HAVE_MAJOR
#endif

#ifndef HAVE_MAJOR
#define major(dev)  (((dev) >> 8) & 0xff)
#define minor(dev)  ((dev) & 0xff)
#define makedev(maj, min)  (((maj) << 8) | (min))
#endif
#undef HAVE_MAJOR

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#else
# include <strings.h>
char *memchr ();
#endif

#include <errno.h>
#ifndef errno
/*@-declundef @*/
extern int errno;
/*@=declundef @*/
#endif

#if defined(__LCLINT__)
/*@-declundef @*/
/*@exits@*/
extern void error(int status, int errnum, const char *format, ...)
	__attribute__ ((__format__ (__printf__, 3, 4)))
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;
/*@=declundef @*/
#else
#if defined(HAVE_ERROR) && defined(HAVE_ERROR_H)
#include <error.h>
#endif
#endif

#if !defined(__LCLINT__)
#if defined(HAVE_SECURE_GETENV) 
#define	getenv(_s)	secure_getenv(_s)
#else
#if defined(HAVE___SECURE_GETENV) 
#define	getenv(_s)	__secure_getenv(_s)
#endif /* defined(HAVE_SECURE_GETENV)   */
#endif /* defined(HAVE___SECURE_GETENV) */
#endif /* !defined(__LCLINT__)          */

#ifdef STDC_HEADERS
/*@-macrounrecog -incondefs -globuse -mustmod @*/ /* FIX: shrug */
#define getopt system_getopt
/*@=macrounrecog =incondefs =globuse =mustmod @*/
/*@-skipansiheaders@*/
#include <stdlib.h>
/*@=skipansiheaders@*/
#undef getopt
#if defined(__LCLINT__)
/*@-declundef -incondefs @*/	/* LCL: modifies clause missing */
extern char * realpath (const char * file_name, /*@out@*/ char * resolved_name)
	/*@globals errno, fileSystem @*/
	/*@requires maxSet(resolved_name) >=  (PATH_MAX - 1); @*/
	/*@modifies *resolved_name, errno, fileSystem @*/;
/*@=declundef =incondefs @*/
#endif
#else /* not STDC_HEADERS */
char *getenv (const char *name);
#if !defined(HAVE_REALPATH)
char *realpath(const char *path, char resolved_path []);
#endif
#endif /* STDC_HEADERS */

/* XXX solaris2.5.1 has not */
#if !defined(EXIT_FAILURE)
#define	EXIT_FAILURE	1
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#if !defined(SEEK_SET) && !defined(__LCLINT__)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#if !defined(F_OK) && !defined(__LCLINT__)
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
# define NLENGTH(direct) (strlen((direct)->d_name))
#else /* not HAVE_DIRENT_H */
# define dirent direct
# define NLENGTH(direct) ((direct)->d_namlen)
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */

#if defined(__LCLINT__)
/*@-declundef -incondefs @*/ /* LCL: missing annotation */
/*@only@*/ /*@out@*/ void * alloca (size_t __size)
	/*@ensures maxSet(result) == (__size - 1) @*/
	/*@*/;
/*@=declundef =incondefs @*/
#endif

#ifdef __GNUC__
# undef alloca
# define alloca __builtin_alloca
#else
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifndef _AIX
/* AIX alloca decl has to be the first thing in the file, bletch! */
char *alloca ();
#  endif
# endif
#endif

#if defined (__GLIBC__) && defined(__LCLINT__)
/*@-declundef@*/
/*@unchecked@*/
extern __const __int32_t *__ctype_tolower;
/*@unchecked@*/
extern __const __int32_t *__ctype_toupper;
/*@=declundef@*/
#endif

#include <ctype.h>

#if defined (__GLIBC__) && defined(__LCLINT__)
/*@-exportlocal@*/
extern int isalnum(int) __THROW	/*@*/;
extern int iscntrl(int) __THROW	/*@*/;
extern int isgraph(int) __THROW	/*@*/;
extern int islower(int) __THROW	/*@*/;
extern int ispunct(int) __THROW	/*@*/;
extern int isxdigit(int) __THROW	/*@*/;
extern int isascii(int) __THROW	/*@*/;
extern int toascii(int) __THROW	/*@*/;
extern int _toupper(int) __THROW	/*@*/;
extern int _tolower(int) __THROW	/*@*/;
/*@=exportlocal@*/

#endif

#if defined(HAVE_SYS_MMAN_H) && !defined(__LCLINT__)
#include <sys/mman.h>
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

/* XXX FIXME: popt on sunos4.1.3: <sys/resource.h> requires <sys/time.h> */
#if defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SYS_TIME_H)
#include <sys/resource.h>
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#if defined(HAVE_GETOPT_H)
/*@-noparams@*/
#include <getopt.h>
/*@=noparams@*/
#endif

#if defined(HAVE_GRP_H)
#define	uuid_t	unistd_uuid_t	/* XXX Mac OS X dares to be different. */
#define	uuid_create	unistd_uuid_create
#define	uuid_compare	unistd_uuid_compare
#include <grp.h>
#undef	unistd_uuid_t		/* XXX Mac OS X dares to be different. */
#undef	unistd_uuid_create
#undef	unistd_uuid_compare
#endif

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif

#if defined(HAVE_ERR_H)
#include <err.h>
#endif

#if defined(HAVE_LIBGEN_H)
#include <libgen.h>
#endif

/* -- Retrofit glibc __progname */
#if defined __GLIBC__ && __GLIBC__ >= 2
#if __GLIBC_MINOR__ >= 1
#define	__progname	__assert_program_name
#endif
#define	setprogname(pn)
#else
#define	__progname	program_name
#define	setprogname(pn)	\
  { if ((__progname = strrchr(pn, '/')) != NULL) __progname++; \
    else __progname = pn;		\
  }
#endif

/*@unchecked@*/
extern const char *__progname;

/* -- Retrofit missing prototypes (if needed). */
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HAVE_STPCPY)
char * stpcpy(/*@out@*/ char * dest, const char * src);
#endif

#if !defined(HAVE_STPNCPY)
char * stpncpy(/*@out@*/ char * dest, const char * src, size_t n);
#endif

#if defined(NEED_MYREALLOC)
#define realloc(ptr,size) myrealloc(ptr,size)
extern void *myrealloc(void *, size_t);
#endif

#if !defined(HAVE_SETENV)
extern int setenv(const char *name, const char *value, int replace);
extern void unsetenv(const char *name);
#endif

/*@-declundef -incondefs @*/ /* FIX: these are macros */
/**
 */
/*@mayexit@*/ /*@only@*/ /*@out@*/ void * xmalloc (size_t size)
	/*@globals errno @*/
	/*@ensures maxSet(result) == (size - 1) @*/
	/*@modifies errno @*/;

/**
 */
/*@mayexit@*/ /*@only@*/ void * xcalloc (size_t nmemb, size_t size)
	/*@ensures maxSet(result) == (nmemb - 1) @*/
	/*@*/;

/**
 * @todo Annotate ptr with returned/out.
 */
/*@mayexit@*/ /*@only@*/ void * xrealloc (/*@null@*/ /*@only@*/ void * ptr,
					size_t size)
	/*@ensures maxSet(result) == (size - 1) @*/
	/*@modifies *ptr @*/;

/**
 */
/*@mayexit@*/ /*@only@*/ char * xstrdup (const char *str)
	/*@*/;
/*@=declundef =incondefs @*/

/**
 */
/*@unused@*/ /*@exits@*/ /*@only@*/ void * vmefail(size_t size)
	/*@*/;

/* XXX limit the fiddle up to linux for now. */
#if !defined(HAVE_SETPROCTITLE) && defined(__linux__)
extern int finiproctitle(void)
	/*@globals environ @*/
	/*@modifies environ @*/;
extern int initproctitle(int argc, char *argv[], char *envp[])
	/*@globals environ @*/
	/*@modifies environ @*/;

extern int setproctitle (const char *fmt, ...)
        __attribute__ ((__format__ (__printf__, 1, 2)))
	/*@*/;
#endif	/* !defined(HAVE_SETPROCTITLE) && defined(__linux__) */

#ifdef __cplusplus
}
#endif

#if defined(HAVE_MCHECK_H)
#include <mcheck.h>
#if defined(__LCLINT__)
/*@-declundef -incondefs @*/ /* LCL: missing annotations */
#if 0
enum mcheck_status
  {
    MCHECK_DISABLED = -1,       /* Consistency checking is not turned on.  */
    MCHECK_OK,                  /* Block is fine.  */
    MCHECK_FREE,                /* Block freed twice.  */
    MCHECK_HEAD,                /* Memory before the block was clobbered.  */
    MCHECK_TAIL                 /* Memory after the block was clobbered.  */
  };
#endif

extern int mcheck (void (*__abortfunc) (enum mcheck_status))
	/*@globals internalState@*/
	/*@modifies internalState @*/;
extern int mcheck_pedantic (void (*__abortfunc) (enum mcheck_status))
	/*@globals internalState@*/
	/*@modifies internalState @*/;
extern void mcheck_check_all (void)
	/*@globals internalState@*/
	/*@modifies internalState @*/;
extern enum mcheck_status mprobe (void *__ptr)
	/*@globals internalState@*/
	/*@modifies internalState @*/;
extern void mtrace (void)
	/*@globals internalState@*/
	/*@modifies internalState @*/;
extern void muntrace (void)
	/*@globals internalState@*/
	/*@modifies internalState @*/;
/*@=declundef =incondefs @*/
#endif /* defined(__LCLINT__) */

/* Memory allocation via macro defs to get meaningful locations from mtrace() */
#if defined(__GNUC__)
#define	xmalloc(_size) 		(malloc(_size) ? : vmefail(_size))
#define	xcalloc(_nmemb, _size)	(calloc((_nmemb), (_size)) ? : vmefail(_size))
#define	xrealloc(_ptr, _size)	(realloc((_ptr), (_size)) ? : vmefail(_size))
#define	xstrdup(_str)	(strcpy((char *)(malloc(strlen(_str)+1) ? : vmefail(strlen(_str)+1)), (_str)))
#endif	/* defined(__GNUC__) */
#endif	/* HAVE_MCHECK_H */

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

/* Take care of NLS matters.  */

#if defined(HAVE_LOCALE_H)
# include <locale.h>
#endif
#if !defined(HAVE_SETLOCALE)
# define setlocale(Category, Locale) /* empty */
#endif

#if defined(ENABLE_NLS) && !defined(__LCLINT__)
# include <libintl.h>
# define _(Text) dgettext (PACKAGE, Text)
# define D_(Text) Text
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) Text
# define D_(Text) Text
# undef dgettext
# define dgettext(DomainName, Text) Text
#endif

#define N_(Text) Text

/* ============== from misc/miscfn.h */

/*@-noparams@*/
#include "rpmio/glob.h"
#include "rpmio/fnmatch.h"
/*@=noparams@*/

#if defined(__LCLINT__)
/*@-declundef -incondefs @*/ /* LCL: missing annotation */
#if 0
typedef /*@concrete@*/ struct
  {
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
    int gl_flags;

    void (*gl_closedir) (void *);
#ifdef _GNU_SOURCE
    struct dirent *(*gl_readdir) (void *);
#else
    void *(*gl_readdir) (void *);
#endif
    ptr_t (*gl_opendir) (const char *);
#ifdef _GNU_SOURCE
    int (*gl_lstat) (const char *restrict, struct stat *restrict);
    int (*gl_stat) (const char *restrict, struct stat *restrict);
#else
    int (*gl_lstat) (const char *restrict, void *restrict);
    int (*gl_stat) (const char *restrict, void *restrict);
#endif
  } glob_t;
#endif

#if 0
/*@-constuse@*/
/*@constant int GLOB_ERR@*/
/*@constant int GLOB_MARK@*/
/*@constant int GLOB_NOSORT@*/
/*@constant int GLOB_DOOFFS@*/
/*@constant int GLOB_NOCHECK@*/
/*@constant int GLOB_APPEND@*/
/*@constant int GLOB_NOESCAPE@*/
/*@constant int GLOB_PERIOD@*/

#ifdef _GNU_SOURCE
/*@constant int GLOB_MAGCHAR@*/
/*@constant int GLOB_ALTDIRFUNC@*/
/*@constant int GLOB_BRACE@*/
/*@constant int GLOB_NOMAGIC@*/
/*@constant int GLOB_TILDE@*/
/*@constant int GLOB_ONLYDIR@*/
/*@constant int GLOB_TILDE_CHECK@*/
#endif

/*@constant int GLOB_FLAGS@*/

/*@constant int GLOB_NOSPACE@*/
/*@constant int GLOB_ABORTED@*/
/*@constant int GLOB_NOMATCH@*/
/*@constant int GLOB_NOSYS@*/
#ifdef _GNU_SOURCE
/*@constant int GLOB_ABEND@*/
#endif
/*@=constuse@*/
#endif

/*@-protoparammatch -redecl@*/
/*@-type@*/	/* XXX glob64_t */
extern int glob (const char *__pattern, int __flags,
                      int (*__errfunc) (const char *, int),
                      /*@out@*/ glob_t *__pglob)
	/*@globals errno, fileSystem @*/
	/*@modifies *__pglob, errno, fileSystem @*/;
	/* XXX only annotation is a white lie */
extern void globfree (/*@only@*/ glob_t *__pglob)
	/*@modifies *__pglob @*/;
/*@=type@*/
#ifdef _GNU_SOURCE
extern int glob_pattern_p (const char *__pattern, int __quote)
	/*@*/;
#endif
/*@=protoparammatch =redecl@*/

#if 0
/*@-constuse@*/
/*@constant int FNM_PATHNAME@*/
/*@constant int FNM_NOESCAPE@*/
/*@constant int FNM_PERIOD@*/

#ifdef _GNU_SOURCE
/*@constant int FNM_FILE_NAME@*/	/* GNU extension */
/*@constant int FNM_LEADING_DIR@*/	/* GNU extension */
/*@constant int FNM_CASEFOLD@*/		/* GNU extension */
/*@constant int FNM_EXTMATCH@*/		/* GNU extension */
#endif

/*@constant int FNM_NOMATCH@*/

#ifdef _XOPEN_SOURCE
/*@constant int FNM_NOSYS@*/		/* X/Open */
#endif
/*@=constuse@*/
#endif

/*@-redecl@*/
extern int fnmatch (const char *__pattern, const char *__name, int __flags)
	/*@*/;
/*@=redecl@*/
/*@=declundef =incondefs @*/
#endif

#if !defined(__cplusplus)
#if !defined(HAVE_S_IFSOCK)
#define S_IFSOCK (0xc000)
#endif

#if !defined(HAVE_S_ISLNK)
#define S_ISLNK(mode) ((mode & 0xf000) == S_IFLNK)
#endif

#if !defined(HAVE_S_ISSOCK)
#define S_ISSOCK(mode) ((mode & 0xf000) == S_IFSOCK)
#endif
#endif /* !defined(__cplusplus) */

#if defined(NEED_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAVE_SYS_SOCKET_H)
#include <sys/types.h>
#include <sys/socket.h>
#endif

#if defined(HAVE_POLL_H)
#include <poll.h>
#else
#if defined(HAVE_SYS_SELECT_H) && !defined(__LCLINT__)
#include <sys/select.h>
#endif
#endif

/* Solaris <= 2.6 limits getpass return to only 8 chars */
#if defined(HAVE_GETPASSPHRASE)
#define	getpass	getpassphrase
#endif

#if !defined(HAVE_LCHOWN)
#define lchown chown
#endif

#if defined(HAVE_GETMNTINFO) || defined(HAVE_GETMNTINFO_R) || defined(HAVE_MNTCTL)
# define GETMNTENT_ONE 0
# define GETMNTENT_TWO 0
# if defined(HAVE_SYS_MNTCTL_H)
#  include <sys/mntctl.h>
# endif
# if defined(HAVE_SYS_VMOUNT_H)
#  include <sys/vmount.h>
# endif
# if defined(HAVE_SYS_MOUNT_H)
#  include <sys/mount.h>
# endif
#elif defined(HAVE_MNTENT_H) || !defined(HAVE_GETMNTENT) || defined(HAVE_STRUCT_MNTTAB)
# if defined(HAVE_MNTENT_H)
#  include <stdio.h>
#  include <mntent.h>
#  define our_mntent struct mntent
#  define our_mntdir mnt_dir
# elif defined(HAVE_STRUCT_MNTTAB)
#  include <stdio.h>
#  include <mnttab.h>
   struct our_mntent {
       char * our_mntdir;
   };
   struct our_mntent *getmntent(FILE *filep);
#  define our_mntent struct our_mntent
# else
#  include <stdio.h>
   struct our_mntent {
       char * our_mntdir;
   };
   struct our_mntent *getmntent(FILE *filep);
#  define our_mntent struct our_mntent
# endif
# define GETMNTENT_ONE 1
# define GETMNTENT_TWO 0
#elif defined(HAVE_SYS_MNTTAB_H)
# include <stdio.h>
# include <sys/mnttab.h>
# define GETMNTENT_ONE 0
# define GETMNTENT_TWO 1
# define our_mntent struct mnttab
# define our_mntdir mnt_mountp
#else /* if !HAVE_MNTCTL */
# error Neither mntent.h, mnttab.h, or mntctl() exists. I cannot build on this system.
#endif

#ifndef MOUNTED
#define MOUNTED "/etc/mnttab"
#endif

#if defined(__LCLINT__)
#define FILE_RCSID(id)
#else
#define FILE_RCSID(id) \
static inline const char *rcsid(const char *p) { \
        return rcsid(p = id); \
}
#endif

#if defined(HAVE_SEARCH_H)
#include <search.h>
#endif

/**
 * makedev() on QNX takes three parameters
 * the additional one (first place) specifies the node for QNET
 * as this applic is not QNET aware, we can set it to 'local node'
 */
#if defined(__QNXNTO__)
#include <sys/netmgr.h>
#define Makedev(x,y)   makedev(ND_LOCAL_NODE,(x),(y))
#else
#define Makedev(x,y)   makedev((x),(y))
#endif

#if defined(WITH_PTHREADS)
#if defined(HAVE_PTHREAD_H) && !defined(__LCLINT__)
#include <pthread.h>
#endif
#endif

/**
 * Use the tag data type compiled into rpm, not the type from the header.
 */
#define SUPPORT_IMPLICIT_TAG_DATA_TYPES	1

/**
 * Eliminate RPM_I18NSTRING_TYPE.
 */
#define SUPPORT_I18NSTRING_TYPE	1

/**
 * Eliminate signature/digest disablers.
 */
#undef	SUPPORT_NOSIGNATURES

/**
 * Permit ar(1) payloads. Disabled while rpmio/iosm.c is under development.
 */
#undef	SUPPORT_AR_PAYLOADS

#endif	/* H_SYSTEM */
