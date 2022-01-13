/**
 * \file rpmio/rpmz.c
 */

/*
 *
 *  Copyright (C) 2007 Lasse Collin
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "system.h"

#include <lzma.h>

#define	_RPMIOB_INTERNAL
#define	_RPMZ_INTERNAL
#define	_RPMZ_INTERNAL_XZ
#include "rpmz.h"

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

#ifdef	NOTYET
/*@unchecked@*/
static int opt_preserve_name;
#endif

#ifdef	NOTYET
/*@unchecked@*/
static lzma_stream strm = LZMA_STREAM_INIT;
#endif

#ifdef	NOTYET
/*  True if we should auto-adjust the compression settings to use less memory
    if memory usage limit is too low for the original settings. */
/*@unchecked@*/
static bool auto_adjust = true;
/*  Indicate if no preset has been explicitly given. In that case, if we need
    to auto-adjust for lower memory usage, we won't print a warning. */
/*@unchecked@*/
static bool preset_default = true;
#endif

enum {
    OPT_SUBBLOCK = INT_MIN,

    OPT_DELTA,
    OPT_LZMA1,
    OPT_LZMA2,

};

#define F_ISSET(_f, _FLAG) (((_f) & ((RPMZ_FLAGS_##_FLAG) & ~0x40000000)) != RPMZ_FLAGS_NONE)
#define RZ_ISSET(_FLAG) F_ISSET(z->flags, _FLAG)

/*@unchecked@*/
struct rpmz_s __rpmz = {
    .stdin_fn	= "(stdin)",
    .stdout_fn	= "(stdout)",

  /* XXX logic is reversed, disablers should clear with toggle. */
    .flags	= (RPMZ_FLAGS_HNAME|RPMZ_FLAGS_HTIME|RPMZ_FLAGS_INDEPENDENT),
    .format	= RPMZ_FORMAT_AUTO,
    .mode	= RPMZ_MODE_COMPRESS,
    .level	= 6,		/* XXX compression level is type specific. */

    .nb		= 16 * BUFSIZ,
    .ifmode	= "rb",
    .ofmode	= "wb",

    ._auto_adjust = 1,
#ifdef	NOTYET
    ._format_compress_auto = RPMZ_FORMAT_XZ,
#else
    ._format_compress_auto = RPMZ_FORMAT_GZIP,
#endif
    ._checksum	= LZMA_CHECK_CRC64,

};

/*@unchecked@*/
rpmz _rpmz = &__rpmz;

/*==============================================================*/
/**
 */
static int checkfd(const char * devnull, int fdno, int flags)
	/*@*/
{
    struct stat sb;
    int ret = 0;

    if (fstat(fdno, &sb) == -1 && errno == EBADF)
	ret = (open(devnull, flags) == fdno) ? 1 : 2;
    return ret;
}

/**
 */
static void io_init(void)
	/*@*/
{
    static int _oneshot = 0;

    /* Insure that stdin/stdout/stderr are open, lest stderr end up in rpmdb. */
   if (!_oneshot) {
	static const char _devnull[] = "/dev/null";
#if defined(STDIN_FILENO)
	(void) checkfd(_devnull, STDIN_FILENO, O_RDONLY);
#endif
#if defined(STDOUT_FILENO)
	(void) checkfd(_devnull, STDOUT_FILENO, O_WRONLY);
#endif
#if defined(STDERR_FILENO)
	(void) checkfd(_devnull, STDERR_FILENO, O_WRONLY);
#endif
	_oneshot++;
    }
}

/*  \brief      Unlinks a file

    This tries to verify that the file being unlinked really is the file that
    we want to unlink by verifying device and inode numbers. There's still
    a small unavoidable race, but this is much better than nothing (the file
    could have been moved/replaced even hours earlier). */
static void
io_unlink(const char *fn, const struct stat *ost)
	/*@*/
{
    /* On Windows, st_ino is meaningless, so don't bother testing it. */
#ifndef _WIN32
    struct stat nsb;

    if (Lstat(fn, &nsb) != 0
     || nsb.st_dev != ost->st_dev || nsb.st_ino != ost->st_ino)
	rpmlog(RPMLOG_ERR, _("%s: File seems to be moved, not removing\n"), fn);
    else
    /* There's a race condition between Lstat() and Unlink()
       but at least we have tried to avoid removing wrong file. */
    if (S_ISREG(nsb.st_mode) && Unlink(fn))
	rpmlog(RPMLOG_ERR, _("%s: Cannot remove: %s\n"), fn, strerror(errno));
#else
    if (Unlink(fn))
	rpmlog(RPMLOG_ERR, _("%s: Cannot remove: %s\n"), fn, strerror(errno));
#endif
    return;
}

/*  \brief      Copies owner/group and permissions

    \todo       ACL and EA support
*/
static void
io_copy_attrs(rpmz z)
	/*@*/
{
	/* Skip chown and chmod on Windows. */
#ifndef _WIN32
	/* This function is more tricky than you may think at first.
	   Blindly copying permissions may permit users to access the
	   destination file who didn't have permission to access the
	   source file. */

#ifdef	NOTYET		/* XXX Fileno used by Fchmod/Fchown needs work. */
	/* Try changing the owner of the file. If we aren't root or the owner
	   isn't already us, fchown() probably doesn't succeed. We warn
	   about failing fchown() only if we are root. */
	if (Fchown(z->ofd, z->isb.st_uid, -1) && (geteuid() == 0))
		rpmlog(RPMLOG_WARNING, _("%s: Cannot set the file owner: %s\n"),
				zq->ofn, strerror(errno));

    {	mode_t mode;

	if (Fchown(z->ofd, -1, z->isb.st_gid)) {
		rpmlog(RPMLOG_WARNING, _("%s: Cannot set the file group: %s\n"),
				zq->ofn, strerror(errno));
		/* We can still safely copy some additional permissions:
		   `group' must be at least as strict as `other' and
		   also vice versa.

		   NOTE: After this, the owner of the source file may
		   get additional permissions. This shouldn't be too bad,
		   because the owner would have had permission to chmod
		   the original file anyway. */
		mode = ((z->isb.st_mode & 0070) >> 3)
				& (z->isb.st_mode & 0007);
		mode = (z->isb.st_mode & 0700) | (mode << 3) | mode;
	} else {
		/* Drop the setuid, setgid, and sticky bits. */
		mode = z->isb.st_mode & 0777;
	}

	if (Fchmod(z->ofd, mode))
		rpmlog(RPMLOG_WARNING, _("%s: Cannot set the file permissions: %s\n"),
				zq->ofn, strerror(errno));
    }
#endif
#endif

	/* Copy the timestamps. We have several possible ways to do this, of
	   which some are better in both security and precision.

	   First, get the nanosecond part of the timestamps. As of writing,
	   it's not standardized by POSIX, and there are several names for
	   the same thing in struct stat. */
    {	long atime_nsec;
	long mtime_nsec;

#	if defined(HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
	/* GNU and Solaris */
	atime_nsec = z->isb.st_atim.tv_nsec;
	mtime_nsec = z->isb.st_mtim.tv_nsec;

#	elif defined(HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
	/* BSD */
	atime_nsec = z->isb.st_atimespec.tv_nsec;
	mtime_nsec = z->isb.st_mtimespec.tv_nsec;

#	elif defined(HAVE_STRUCT_STAT_ST_ATIMENSEC)
	/* GNU and BSD without extensions */
	atime_nsec = z->isb.st_atimensec;
	mtime_nsec = z->isb.st_mtimensec;

#	elif defined(HAVE_STRUCT_STAT_ST_UATIME)
	/* Tru64 */
	atime_nsec = z->isb.st_uatime * 1000;
	mtime_nsec = z->isb.st_umtime * 1000;

#	elif defined(HAVE_STRUCT_STAT_ST_ATIM_ST__TIM_TV_NSEC)
	/* UnixWare */
	atime_nsec = z->isb.st_atim.st__tim.tv_nsec;
	mtime_nsec = z->isb.st_mtim.st__tim.tv_nsec;

#	else
	/* Safe fallback */
	atime_nsec = 0;
	mtime_nsec = 0;
#	endif

	/* Construct a structure to hold the timestamps and call appropriate
	   function to set the timestamps. */
#if defined(HAVE_FUTIMENS)
	/* Use nanosecond precision. */
      {	struct timespec tv[2];
	tv[0].tv_sec = z->isb.st_atime;
	tv[0].tv_nsec = atime_nsec;
	tv[1].tv_sec = z->isb.st_mtime;
	tv[1].tv_nsec = mtime_nsec;

	/* XXX Fileno onto libio FILE * is -1 so EBADF is returned. */
	(void)futimens(Fileno(z->ofd), tv);
      }

#elif defined(HAVE_FUTIMES) || defined(HAVE_FUTIMESAT) || defined(HAVE_UTIMES)
	/* Use microsecond precision. */
      {	struct timeval tv[2];
	tv[0].tv_sec = z->isb.st_atime;
	tv[0].tv_usec = atime_nsec / 1000;
	tv[1].tv_sec = z->isb.st_mtime;
	tv[1].tv_usec = mtime_nsec / 1000;

#	if defined(HAVE_FUTIMES)
	/* XXX Fileno onto libio FILE * is -1 so EBADF is returned. */
	(void)futimes(Fileno(z->ofd), tv);
#	elif defined(HAVE_FUTIMESAT)
	/* XXX Fileno onto libio FILE * is -1 so EBADF is returned. */
	(void)futimesat(Fileno(z->ofd), NULL, tv);
#	else
	/* Argh, no function to use a file descriptor to set the timestamp. */
	(void)Utimes(zq->ofn, tv);
#	endif
      }

#elif defined(HAVE_UTIME)
	/* Use one-second precision. utime() doesn't support using file
	   descriptor either. Some systems have broken utime() prototype
	   so don't make this const. */
      {	struct utimbuf buf = {
		.actime = z->isb.st_atime,
		.modtime = z->isb.st_mtime,
	};

	/* Avoid warnings. */
	(void)atime_nsec;
	(void)mtime_nsec;

	(void)Utime(zq->ofn, &buf);
      }
#endif
    }

	return;
}

/*==============================================================*/

#if defined(HAVE_PHYSMEM_SYSCTL) /* HAVE_SYS_SYSCTL_H */
#include <sys/sysctl.h>
#endif

static rpmuint64_t physmem(void)
	/*@*/
{
    rpmuint64_t ret = 0;

#if defined(HAVE_PHYSMEM_SYSCONF)
    {	const long pagesize = sysconf(_SC_PAGESIZE);
	const long pages = sysconf(_SC_PHYS_PAGES);
	if (pagesize != -1 || pages != -1)
	    ret = (rpmuint64_t)(pagesize) * (rpmuint64_t)(pages);
    }
#elif defined(HAVE_PHYSMEM_SYSCTL)
    {	int name[2] = { CTL_HW, HW_PHYSMEM };
	unsigned long mem;
	size_t mem_ptr_size = sizeof(mem);
	if (!sysctl(name, 2, &mem, &mem_ptr_size, NULL, 0)) {
	    if (mem_ptr_size != sizeof(mem)) {
		if (mem_ptr_size == sizeof(unsigned int))
		    ret = *(unsigned int *)(&mem);
	    } else {
		ret = mem;
	    }
	}
    }
#endif
    return ret;
}

#if defined(HAVE_NCPU_SYSCTL) /* HAVE_SYS_SYSCTL_H */
#include <sys/sysctl.h>
#endif

static void
hw_cores(rpmz z)
	/*@modifies z @*/
{
    rpmzQueue zq = z->zq;

#if defined(HAVE_NCPU_SYSCONF)
    {	const long cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (cpus > 0)
	    zq->threads = (size_t)(cpus);
    }

#elif defined(HAVE_NCPU_SYSCTL)
    {	int name[2] = { CTL_HW, HW_NCPU };
	int cpus;
	size_t cpus_size = sizeof(cpus);
	if (!sysctl(name, 2, &cpus, &cpus_size, NULL, 0)
	 && cpus_size == sizeof(cpus) && cpus > 0)
	    z->threads = (size_t)(cpus);
    }
#endif

#if defined(_SC_THREAD_THREADS_MAX)
    {	const long threads_max = sysconf(_SC_THREAD_THREADS_MAX);
	if (threads_max > 0 && (unsigned)(threads_max) < zq->threads)
	    zq->threads = (unsigned)(threads_max);
    }
#elif defined(PTHREAD_THREADS_MAX)
    if (zq->threads > PTHREAD_THREADS_MAX)
	zq->threads = PTHREAD_THREADS_MAX;
#endif

    return;
}

static void
hw_memlimit_init(rpmz z)
	/*@globals memlimit_decoder, memlimit_encoder @*/
	/*@modifies memlimit_decoder, memlimit_encoder @*/
{
    z->mem = physmem();

    /* Assume 16MB as minimum memory. */
    if (z->mem == 0)
	    z->mem = 16 * 1024 * 1024;

    /* Use 90% of RAM when encoding, 33% when decoding. */
    z->memlimit_encoder = z->mem - z->mem / 10;
    z->memlimit_decoder = z->mem / 3;
    return;
}

static void
hw_init(rpmz z)
{
    hw_memlimit_init(z);
    hw_cores(z);
    return;
}

/*==============================================================*/

/* XXX todo: add zlib/zip and other common suffixes. */
/*@unchecked@*/ /*@observer@*/
static struct suffixPairs_s {
    enum rpmzFormat_e format;
/*@null@*/
    const char * csuffix;
/*@relnull@*/
    const char * usuffix;
} suffixPairs[] = {
    { RPMZ_FORMAT_XZ,		".xz",	"" },
    { RPMZ_FORMAT_XZ,		".txz",	".tar" },
    { RPMZ_FORMAT_LZMA,		".lzma","" },
    { RPMZ_FORMAT_LZMA,		".tlz",	".tar" },
    { RPMZ_FORMAT_GZIP,		".gz",	"" },
    { RPMZ_FORMAT_GZIP,		".tgz",	".tar" },
    { RPMZ_FORMAT_ZLIB,		".zz",	"" },
    { RPMZ_FORMAT_ZLIB,		".tzz",	".tar" },
    { RPMZ_FORMAT_ZIP2,		".zip",	"" },	/* XXX zip3 from pigz? */
#ifdef	NOTYET
    { RPMZ_FORMAT_ZIP2,		".tgz",	".tar" },
#endif
    { RPMZ_FORMAT_BZIP2,	".bz2",	"" },
    { RPMZ_FORMAT_RAW,		NULL,	NULL }
};

/**
 * Check string for a suffix.
 * @param fn		string
 * @param suffix	suffix
 * @return		1 if string ends with suffix
 */
static int chkSuffix(const char * fn, const char * suffix)
	/*@*/
{
    size_t flen = strlen(fn);
    size_t slen = strlen(suffix);
    int rc = (flen > slen && !strcmp(fn + flen - slen, suffix));
if (_debug)
fprintf(stderr, "\tchk(%s,%s) ret %d\n", fn, suffix, rc);
    return rc;
}

/**
 * Return (malloc'd) string with new suffix substituted for old.
 * @param fn		string
 * @param old		old suffix
 * @param new		new suffix
 * @return		new string
 */
static char * changeSuffix(const char * fn,
		/*@null@*/ const char * old, 
		/*@null@*/ const char * new)
	/*@*/
{
    size_t nfn = strlen(fn);
    size_t nos = (old ? strlen(old) : 0);
    size_t nns = (new ? strlen(new) : 0);
    char * t = xmalloc(nfn - nos + nns + 1);

    {	char * te = t;

	strncpy(te, fn, (nfn - nos));
	te += (nfn - nos);
	if (new)
	    te = stpcpy(te, new);
	*te = '\0';
    }

    return t;
}

/**
 * Return (malloc'd) uncompressed file name.
 * @param z		rpmz container
 * @return		uncompressed file name
 */
static const char * uncompressedFN(rpmz z)
	/*@*/
{
    struct suffixPairs_s * sp;
    const char * fn = z->ifn;
    const char * t = NULL;

    for (sp = suffixPairs; sp->csuffix != NULL; sp++) {
	if (z->format != sp->format)
	    continue;
	if (!chkSuffix(fn, sp->csuffix))
	    continue;
	t = changeSuffix(fn, sp->csuffix, sp->usuffix);
	break;
    }

    if (t == NULL)
	t = xstrdup(fn);

if (_debug)
fprintf(stderr, "==> uncompressedFN: %s\n", t);
    return t;
}

/**
 * Return (malloc'd) compressed file name.
 * @param z		rpmz container
 * @return		compressed file name
 */
static const char * compressedFN(rpmz z)
	/*@*/
{
    struct suffixPairs_s * sp;
    const char * fn = z->ifn;
    const char * t = NULL;

    for (sp = suffixPairs; sp->csuffix != NULL; sp++) {
	if (z->format != sp->format || *sp->usuffix == '\0')
	    continue;
	if (!chkSuffix(fn, sp->usuffix))
	    continue;
	t = changeSuffix(fn, sp->usuffix, sp->csuffix);
	break;
    }

    if (t == NULL)
	t = rpmGetPath(fn, (z->suffix ? z->suffix : z->osuffix), NULL);

if (_debug)
fprintf(stderr, "==>   compressedFN: %s (suffix %s osuffix %s)\n", t, z->suffix, z->osuffix);
    return t;
}

/* forward ref */
static rpmRC rpmzProcess(rpmz z, /*@null@*/ const char * ifn)
	/*@modifies z @*/;

static rpmRC rpmzFini(rpmz z, rpmRC rc)
{
    rpmzQueue zq = z->zq;
    int xx;

    /* XXX mask signals */
    if (z->ofd != NULL) {
	xx = Fflush(z->ofd);

	/* Timestamp output file same as input. */
	if (zq->ofn != NULL && strcmp(zq->ofn, z->stdout_fn))
	    io_copy_attrs(z);	/* XXX add error return */
	if ((xx = Fclose(z->ofd)) != 0) {
	    rpmlog(RPMLOG_ERR, _("%s: Closing output file failed: %s\n"),
                                zq->ofn, strerror(errno));
	    rc = RPMRC_FAIL;
	}
	z->ofd = NULL;
    }
    if (z->ifd != NULL) {
	if ((xx = Fclose(z->ifd)) != 0) {
	    rpmlog(RPMLOG_ERR, _("%s: Closing input file failed: %s\n"),
                                z->ifn, strerror(errno));
	    rc = RPMRC_FAIL;
	}
	z->ifd = NULL;
    }

    /* Clean up input/output files as needed. */
    switch (rc) {
    default:
	break;
    case RPMRC_FAIL:
	if (zq->ofn != NULL && strcmp(zq->ofn, z->stdout_fn)) {
	    xx = Unlink(zq->ofn);
if (_debug)
fprintf(stderr, "==> Unlink(%s) FAIL\n", zq->ofn);
	}
	break;
    case RPMRC_OK:
	if (!F_ISSET(z->flags, KEEP) && z->ifn != z->stdin_fn) {
	    io_unlink(z->ifn, &z->isb);
if (_debug)
fprintf(stderr, "==> Unlink(%s)\n", z->ifn);
	}
	break;
    }

    zq->ofn = _free(zq->ofn);

    return rc;
}

static rpmRC rpmzInit(rpmz z, /*@null@*/ const char * ifn)
	/*@*/
{
    rpmzQueue zq = z->zq;
    rpmRC rc = RPMRC_FAIL;

if (_debug)
fprintf(stderr, "==> rpmzInit(%p) ifn %s ofmode %s\n", z, z->ifn, z->ofmode);

    z->ifn = ifn;	/* XXX ifn[] for rpmzProcess() append w --recurse */

    if (ifn == NULL) {
	z->ifn = z->stdin_fn;
	switch (z->mode) {
	default:
	    break;
	case RPMZ_MODE_COMPRESS:
	    z->ifd = fdDup(STDIN_FILENO);
	    break;
	case RPMZ_MODE_DECOMPRESS:
	    z->ifd = z->idio->_fdopen(fdDup(STDIN_FILENO), z->ifmode);
	    break;
	}
    } else {
	struct stat * st = &z->isb;

	/* set input file name (already set if recursed here) */
	if (ifn != z->_ifn) {
/*@-mayaliasunique@*/
	    strncpy(z->_ifn, ifn, sizeof(z->_ifn));
/*@=mayaliasunique@*/
assert(z->_ifn[sizeof(z->_ifn) - 1] == '\0');
	    z->ifn = z->_ifn;
	}
	if (Lstat(z->ifn, &z->isb) < 0) {
#ifdef	EOVERFLOW
assert(errno != EOVERFLOW);
assert(errno != EFBIG);
#endif
	    fprintf(stderr, "%s: input %s does not exist, skipping\n",
			__progname, z->ifn);
	    goto exit;
	}
	if (!S_ISREG(st->st_mode) && !S_ISLNK(st->st_mode) && !S_ISDIR(st->st_mode))
	{
	    fprintf(stderr, "%s: input %s is a special file or device -- skipping\n",
			__progname, z->ifn);
	    goto exit;
	}
	if (S_ISLNK(st->st_mode) && !F_ISSET(z->flags, SYMLINKS)) {
	    fprintf(stderr, "%s: input %s is a symbolic link -- skipping\n",
			__progname, z->ifn);
	    goto exit;
	}

	/* Recurse into directory */
	if (S_ISDIR(st->st_mode)) {
	    struct stat sb = *st;	/* structure assignment. */
	    struct dirent * dp;
	    DIR * dir;
	    ARGV_t av = NULL;
	    size_t len;
	    char * te;
	    int xx;
	    int i;

	    if (!F_ISSET(z->flags, RECURSE)) {
		fprintf(stderr, "%s: input %s is a directory -- skipping\n",
			__progname, z->_ifn);
		goto exit;
	    }
	    len = strlen(z->_ifn);
	    te = z->_ifn + len;
	    if ((dir = Opendir(z->_ifn)) == NULL)
		goto exit;
	    while ((dp = Readdir(dir)) != NULL) {
		if (dp->d_name[0] == '\0' ||
		   (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
		   (dp->d_name[1] == '.' && dp->d_name[2] == '\0'))))
		    continue;
		xx = argvAdd(&av, dp->d_name);
	    }
	    xx = Closedir(dir);
	    if (av != NULL)
	    for (i = 0; av[i] != NULL; i++) {
		*te = '/';
		strncpy(te+1, av[i], sizeof(z->_ifn) - len - 1);
assert(z->_ifn[sizeof(z->_ifn) - 1] == '\0');
		(void) rpmzProcess(z, z->_ifn);
		*te = '\0';
	    }
	    av = argvFree(av);
	    *st = sb;			/* structure assignment. */
	    rc = RPMRC_OK;
	    goto exit;
	}

	/* Open input file. */
	switch (z->mode) {
	default:
	    break;
	case RPMZ_MODE_COMPRESS:
	    z->ifd = Fopen(z->ifn, z->ifmode);
	    break;
	case RPMZ_MODE_DECOMPRESS:
	    z->ifd = z->idio->_fopen(z->ifn, z->ifmode);
	    fdFree(z->ifd, NULL);		/* XXX adjust refcounts. */
	    break;
	}
    }
    if (z->ifd == NULL || Ferror(z->ifd)) {
	fprintf(stderr, "%s: can't open %s\n", __progname, z->ifn);
	goto exit;
    }

    if (F_ISSET(z->flags, STDOUT))  {
	zq->ofn = xstrdup(z->stdout_fn);
	switch (z->mode) {
	default:
	    break;
	case RPMZ_MODE_COMPRESS:
	    z->ofd = z->odio->_fdopen(fdDup(STDOUT_FILENO), z->ofmode);
	    break;
	case RPMZ_MODE_DECOMPRESS:
	    z->ofd = fdDup(STDOUT_FILENO);
	    break;
	}
    } else {
	if (z->ifn == z->stdin_fn)	/* XXX error needed here. */
	    goto exit;

	/* Open output file. */
	switch (z->mode) {
	default:
	    break;
	case RPMZ_MODE_COMPRESS:
	    zq->ofn = compressedFN(z);
	    if (!F_ISSET(z->flags, OVERWRITE) && Stat(zq->ofn, &z->osb) == 0) {
		fprintf(stderr, "%s: output file %s already exists\n",
			__progname, zq->ofn);
		/* XXX TODO: ok to overwrite(y/N)? */
		goto exit;
	    }
	    z->ofd = z->odio->_fopen(zq->ofn, z->ofmode);
	    fdFree(z->ofd, NULL);		/* XXX adjust refcounts. */
	    break;
	case RPMZ_MODE_DECOMPRESS:
	    zq->ofn = uncompressedFN(z);
	    if (!F_ISSET(z->flags, OVERWRITE) && Stat(zq->ofn, &z->osb) == 0) {
		fprintf(stderr, "%s: output file %s already exists\n",
			__progname, zq->ofn);
		/* XXX TODO: ok to overwrite(y/N)? */
		goto exit;
	    }
	    z->ofd = Fopen(zq->ofn, z->ofmode);
	    break;
	}
    }
    if (z->ofd == NULL || Ferror(z->ofd)) {
	fprintf(stderr, "%s: can't open %s\n", __progname, zq->ofn);
	goto exit;
    }

    rc = RPMRC_OK;

exit:
    return rc;
}

/*
 * Copy input to output.
 */
static rpmRC rpmzCopy(rpmz z, rpmiob iob)
	/*@*/
{
    for (;;) {
	iob->blen = Fread(iob->b, 1, iob->allocated, z->ifd);

	if (Ferror(z->ifd))
	    return RPMRC_FAIL;

	if (iob->blen == 0) break;

	if (Fwrite(iob->b, 1, iob->blen, z->ofd) != iob->blen)
	    return RPMRC_FAIL;
    }
    return RPMRC_OK;
}

/*
 * Copy input to output.
 */
static rpmRC rpmzProcess(rpmz z, /*@null@*/ const char * ifn)
{
    rpmRC rc;

    rc = rpmzInit(z, ifn);
    if (rc == RPMRC_OK && z->ifd != NULL && z->ofd != NULL)
	rc = rpmzCopy(z, z->iob);
    rc = rpmzFini(z, rc);
    return rc;
}

/*==============================================================*/
/*@unchecked@*/
static int signals_init_count;

static void signals_init(void)
	/*@*/
{
    int xx;

    if (signals_init_count++ == 0) {
	xx = rpmsqEnable(SIGHUP,      NULL);
	xx = rpmsqEnable(SIGINT,      NULL);
	xx = rpmsqEnable(SIGTERM,     NULL);
	xx = rpmsqEnable(SIGQUIT,     NULL);
	xx = rpmsqEnable(SIGPIPE,     NULL);
	xx = rpmsqEnable(SIGXCPU,     NULL);
	xx = rpmsqEnable(SIGXFSZ,     NULL);
    }
}

static void signals_fini(void)
	/*@*/
{
    int xx;

    if (--signals_init_count == 0) {
	xx = rpmsqEnable(-SIGHUP,     NULL);
	xx = rpmsqEnable(-SIGINT,     NULL);
	xx = rpmsqEnable(-SIGTERM,    NULL);
	xx = rpmsqEnable(-SIGQUIT,    NULL);
	xx = rpmsqEnable(-SIGPIPE,    NULL);
	xx = rpmsqEnable(-SIGXCPU,    NULL);
	xx = rpmsqEnable(-SIGXFSZ,    NULL);
    }
}

static int signals_terminating(int terminate)
	/*@*/
{
    static int terminating = 0;

    if (terminating) return 1;

    if (sigismember(&rpmsqCaught, SIGHUP)
     || sigismember(&rpmsqCaught, SIGINT)
     || sigismember(&rpmsqCaught, SIGTERM)
     || sigismember(&rpmsqCaught, SIGQUIT)
     || sigismember(&rpmsqCaught, SIGPIPE)
     || sigismember(&rpmsqCaught, SIGXCPU)
     || sigismember(&rpmsqCaught, SIGXFSZ)
     || terminate)
	terminating = 1;
    return terminating;
}

#ifdef	NOTYET
/*@unchecked@*/
static int signals_block_count;

static void
signals_block(void)
        /*@globals errno, signals_block_count @*/
        /*@modifies errno, signals_block_count @*/
{
    if (signals_block_count++ == 0) {
	const int saved_errno = errno;
	mythread_sigmask(SIG_BLOCK, &hooked_signals, NULL);
	errno = saved_errno;
    }
    return;
}

static void
signals_unblock(void)
        /*@globals errno, signals_block_count @*/
        /*@modifies errno, signals_block_count @*/
{
    assert(signals_block_count > 0);

    if (--signals_block_count == 0) {
	const int saved_errno = errno;
	mythread_sigmask(SIG_UNBLOCK, &hooked_signals, NULL);
	errno = saved_errno;
    }
    return;
}
#endif

static void signals_exit(void)
	/*@*/
{
#ifdef	NOTYET
    const int sig = exit_signal;

    if (sig != 0) {
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(sig, &sa, NULL);
	raise(exit_signal);
    }

    return;
#else
    signals_fini();
#endif
}

/*==============================================================*/

typedef struct {
    const char *name;
    rpmuint64_t id;
} name_id_map;

typedef struct {
    const char *name;
    const name_id_map *map;
    rpmuint64_t min;
    rpmuint64_t max;
} option_map;

/*  Parses option=value pairs that are separated with colons, semicolons,
    or commas: opt=val:opt=val;opt=val,opt=val

    Each option is a string, that is converted to an integer using the
    index where the option string is in the array.

    Value can be either a number with minimum and maximum value limit, or
    a string-id map mapping a list of possible string values to integers.

    When parsing both option and value succeed, a filter-specific function
    is called, which should update the given value to filter-specific
    options structure.

    \param      str     String containing the options from the command line
    \param      opts    Filter-specific option map
    \param      set     Filter-specific function to update filter_options
    \param      filter_options  Pointer to filter-specific options structure

    \return     Returns only if no errors occur.
*/
static void
parse_options(const char *str, const option_map *opts,
		void (*set)(void *filter_options,
			rpmuint32_t key, rpmuint64_t value),
		void *filter_options)
	/*@*/
{
    char * s;
    char *name;

    if (str == NULL || str[0] == '\0')
	return;

    s = xstrdup(str);
    name = s;

    while (1) {
	char *split = strchr(name, ',');
	char *value;
	int found;
	size_t i;

	if (split != NULL)
	    *split = '\0';

	value = strchr(name, '=');
	if (value != NULL)
	    *value++ = '\0';

	if (value == NULL || value[0] == '\0') {
	    fprintf(stderr, _("%s: %s: Options must be `name=value' pairs separated with commas\n"),
					__progname, str);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	}

	/* Look for the option name from the option map. */
	found = 0;
	for (i = 0; opts[i].name != NULL; ++i) {
	    if (strcmp(name, opts[i].name) != 0)
		continue;

	    if (opts[i].map == NULL) {
		/* value is an integer. */
		rpmuint64_t v;
#ifdef NOTYET
		v = str_to_uint64(name, value, opts[i].min, opts[i].max);
#else
		v = strtoull(value, NULL, 0);
#endif
		set(filter_options, i, v);
	    } else {
		/* value is a string which we should map
		   to an integer. */
		size_t j;
		for (j = 0; opts[i].map[j].name != NULL; ++j) {
		    if (strcmp(opts[i].map[j].name, value) == 0)
			break;
		}

		if (opts[i].map[j].name == NULL) {
		    fprintf(stderr, _("%s: %s: Invalid option value\n"),
			__progname, value);
		    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
		}

		set(filter_options, i, opts[i].map[j].id);
	    }

	    found = 1;
	    break;
	}

	if (!found) {
	    fprintf(stderr, _("%s: %s: Invalid option name\n"), __progname, name);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	}

	if (split == NULL)
	    break;

	name = split + 1;
    }

    s = _free(s);
    return;
}

/* ===== Subblock ===== */
enum {
    OPT_SIZE,
    OPT_RLE,
    OPT_ALIGN,
};

static void
set_subblock(void *options, rpmuint32_t key, rpmuint64_t value)
	/*@*/
{
    lzma_options_subblock *opt = options;

    switch (key) {
    case OPT_SIZE:	opt->subblock_data_size = value;	break;
    case OPT_RLE:	opt->rle = value;			break;
    case OPT_ALIGN:	opt->alignment = value;			break;
    }
}

static lzma_options_subblock *
options_subblock(const char *str)
	/*@*/
{
    static const option_map opts[] = {
	{ "size", NULL,   LZMA_SUBBLOCK_DATA_SIZE_MIN,
	                  LZMA_SUBBLOCK_DATA_SIZE_MAX },
	{ "rle",  NULL,   LZMA_SUBBLOCK_RLE_OFF,
	                  LZMA_SUBBLOCK_RLE_MAX },
	{ "align",NULL,   LZMA_SUBBLOCK_ALIGNMENT_MIN,
	                  LZMA_SUBBLOCK_ALIGNMENT_MAX },
	{ NULL,   NULL,   0, 0 }
    };

    lzma_options_subblock *options = xmalloc(sizeof(lzma_options_subblock));
    options->allow_subfilters = 0;
    options->alignment = LZMA_SUBBLOCK_ALIGNMENT_DEFAULT;
    options->subblock_data_size = LZMA_SUBBLOCK_DATA_SIZE_DEFAULT;
    options->rle = LZMA_SUBBLOCK_RLE_OFF;

    parse_options(str, opts, &set_subblock, options);

    return options;
}

/* ===== Delta ===== */

enum {
    OPT_DIST,
};

static void
set_delta(void *options, rpmuint32_t key, rpmuint64_t value)
	/*@*/
{
    lzma_options_delta *opt = options;
    switch (key) {
    case OPT_DIST:	opt->dist = value;		break;
    }
}

static lzma_options_delta *
options_delta(const char *str)
{
    static const option_map opts[] = {
	{ "dist",     NULL,  LZMA_DELTA_DIST_MIN,
	                     LZMA_DELTA_DIST_MAX },
	{ NULL,       NULL,  0, 0 }
    };

    lzma_options_delta *options = xmalloc(sizeof(lzma_options_delta));
    /* It's hard to give a useful default for this. */
    options->type = LZMA_DELTA_TYPE_BYTE;
    options->dist = LZMA_DELTA_DIST_MIN;

    parse_options(str, opts, &set_delta, options);

    return options;
}

/* ===== LZMA ===== */
enum {
    OPT_PRESET,
    OPT_DICT,
    OPT_LC,
    OPT_LP,
    OPT_PB,
    OPT_MODE,
    OPT_NICE,
    OPT_MF,
    OPT_DEPTH,
};

static void
set_lzma(void *options, rpmuint32_t key, rpmuint64_t value)
	/*@*/
{
    lzma_options_lzma *opt = options;

    switch (key) {
    case OPT_PRESET:
	if (lzma_lzma_preset(options, (uint32_t)(value))) {
	    fprintf(stderr, _("LZMA1/LZMA2 preset %u is not supported\n"),
					(unsigned int)(value));
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	}
	break;
    case OPT_DICT:	opt->dict_size = value;		break;
    case OPT_LC:	opt->lc = value;		break;
    case OPT_LP:	opt->lp = value;		break;
    case OPT_PB:	opt->pb = value;		break;
    case OPT_MODE:	opt->mode = value;		break;
    case OPT_NICE:	opt->nice_len = value;		break;
    case OPT_MF:	opt->mf = value;		break;
    case OPT_DEPTH:	opt->depth = value;		break;
    }
}

static lzma_options_lzma *
options_lzma(const char *str)
{
    /*@unchecked@*/ /*@observer@*/
    static const name_id_map modes[] = {
	{ "fast",   LZMA_MODE_FAST },
	{ "normal", LZMA_MODE_NORMAL },
	{ NULL,     0 }
    };

    /*@unchecked@*/ /*@observer@*/
    static const name_id_map mfs[] = {
	{ "hc3", LZMA_MF_HC3 },
	{ "hc4", LZMA_MF_HC4 },
	{ "bt2", LZMA_MF_BT2 },
	{ "bt3", LZMA_MF_BT3 },
	{ "bt4", LZMA_MF_BT4 },
	{ NULL,  0 }
    };

    /*@unchecked@*/ /*@observer@*/
    static const option_map opts[] = {
	{ "preset", NULL,   1, 9 },
	{ "dict",   NULL,   LZMA_DICT_SIZE_MIN,
			(UINT32_C(1) << 30) + (UINT32_C(1) << 29) },
	{ "lc",     NULL,   LZMA_LCLP_MIN, LZMA_LCLP_MAX },
	{ "lp",     NULL,   LZMA_LCLP_MIN, LZMA_LCLP_MAX },
	{ "pb",     NULL,   LZMA_PB_MIN, LZMA_PB_MAX },
	{ "mode",   modes,  0, 0 },
	{ "nice",   NULL,   2, 273 },
	{ "mf",     mfs,    0, 0 },
	{ "depth",  NULL,   0, UINT32_MAX },
	{ NULL,     NULL,   0, 0 }
    };

    lzma_options_lzma *options = xmalloc(sizeof(lzma_options_lzma));

    options->dict_size = LZMA_DICT_SIZE_DEFAULT;
    options->preset_dict = NULL;
    options->preset_dict_size = 0,
    options->lc = LZMA_LC_DEFAULT;
    options->lp = LZMA_LP_DEFAULT;
    options->pb = LZMA_PB_DEFAULT;
    options->persistent = 0;
    options->mode = LZMA_MODE_NORMAL;
    options->nice_len = 64;
    options->mf = LZMA_MF_BT4;
    options->depth = 0;

    parse_options(str, opts, &set_lzma, options);

    return options;
}

static void
coder_add_filter(rpmz z, lzma_vli id, void *options)
	/*@modifies z @*/
{
    if (z->_filters_count == LZMA_FILTERS_MAX) {
	fprintf(stderr, _("%s: Maximum number of filters is %d\n"),
		__progname, LZMA_FILTERS_MAX);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
    }

    z->_filters[z->_filters_count].id = id;
    z->_filters[z->_filters_count].options = options;
    z->_filters_count++;

    return;
}

static rpmuint64_t
hw_memlimit_encoder(rpmz z)
	/*@*/
{
    return z->memlimit_custom != 0 ? z->memlimit_custom : z->memlimit_encoder;
}

static rpmuint64_t
hw_memlimit_decoder(rpmz z)
	/*@*/
{
    return z->memlimit_custom != 0 ? z->memlimit_custom : z->memlimit_decoder;
}

static void
message_filters(int code, const lzma_filter *filters)
{
    char t[BUFSIZ];
    char * te = t;
    unsigned pri = RPMLOG_PRI(code);
    unsigned mask = RPMLOG_MASK(pri);
    size_t i;

    if ((mask & rpmlogSetMask(0)) == 0)
	return;

    *te = '\0';
    te = stpcpy( stpcpy(te, __progname), ": Filter chain:");

    for (i = 0; filters[i].id != LZMA_VLI_UNKNOWN; ++i) {
	te = stpcpy(te, " --");

	switch (filters[i].id) {
	case LZMA_FILTER_LZMA1:
	case LZMA_FILTER_LZMA2: {
	    const lzma_options_lzma *opt = filters[i].options;
	    const char *lzmaver;
	    const char *mode;
	    const char *mf;

	    switch (opt->mode) {
	    case LZMA_MODE_FAST:	mode = "fast";		break;
	    case LZMA_MODE_NORMAL:	mode = "normal";	break;
	    default:			mode = "UNKNOWN";	break;
	    }

	    switch (opt->mf) {
	    case LZMA_MF_HC3:		mf = "hc3";		break;
	    case LZMA_MF_HC4:		mf = "hc4";		break;
	    case LZMA_MF_BT2:		mf = "bt2";		break;
	    case LZMA_MF_BT3:		mf = "bt3";		break;
	    case LZMA_MF_BT4:		mf = "bt4";		break;
	    default:			mf = "UNKNOWN";		break;
	    }

	    lzmaver = (filters[i].id == LZMA_FILTER_LZMA2 ? "2" : "1");
	    te = stpcpy( stpcpy(te, "lzma"), lzmaver);
	    te = stpcpy(te, "=dict=");
	    sprintf(te, "%u", (unsigned)opt->dict_size);
	    te += strlen(te);
	    te = stpcpy(te, ",lc=");
	    sprintf(te, "%u", (unsigned)opt->lc);
	    te += strlen(te);
	    te = stpcpy(te, ",lp=");
	    sprintf(te, "%u", (unsigned)opt->lp);
	    te += strlen(te);
	    te = stpcpy(te, ",pb=");
	    sprintf(te, "%u", (unsigned)opt->pb);
	    te += strlen(te);
	    te = stpcpy( stpcpy(te, ",mode="), mode);
	    te = stpcpy(te, ",nice=");
	    sprintf(te, "%u", (unsigned)opt->nice_len);
	    te += strlen(te);
	    te = stpcpy( stpcpy(te, ",mf="), mf);
	    te = stpcpy(te, ",depth=");
	    sprintf(te, "%u", (unsigned)opt->depth);
	    te += strlen(te);
	    break;
	}

	case LZMA_FILTER_X86:	te = stpcpy(te, "x86");		break;
	case LZMA_FILTER_POWERPC: te = stpcpy(te, "powerpc");	break;
	case LZMA_FILTER_IA64:	te = stpcpy(te, "ia64");	break;
	case LZMA_FILTER_ARM:	te = stpcpy(te, "arm");	break;
	case LZMA_FILTER_ARMTHUMB: te = stpcpy(te, "armthumb"); break;
	case LZMA_FILTER_SPARC:	te = stpcpy(te, "sparc");	break;
	case LZMA_FILTER_DELTA: {
	    const lzma_options_delta *opt = filters[i].options;
	    te = stpcpy(te, "delta=dist=");
	    sprintf(te, "%u", (unsigned)opt->dist);
	    te += strlen(te);
	    break;
	}

	default:		te = stpcpy(te, "UNKNOWN");	break;
	}
    }
    *te = '\0';
    rpmlog(code, "%s\n", t);
    return;
}

static void
memlimit_too_small(rpmuint64_t memory_usage, rpmuint64_t memory_limit)
	/*@*/
{
    rpmlog(RPMLOG_CRIT, _("Memory usage limit (%llu MiB) is too small for the given filter setup (%llu MiB)\n"),
			(unsigned long long)(memory_limit >> 20),
			(unsigned long long)(memory_usage >> 20));
/*@notreached@*/
    exit(EXIT_FAILURE);
}

static void
coder_set_compression_settings(rpmz z)
	/*@globals preset_default, preset_number @*/
	/*@modifies z, preset_default, preset_number @*/
{
    rpmzQueue zq = z->zq;

    /* Options for LZMA1 or LZMA2 in case we are using a preset. */
    rpmuint64_t memory_usage;
    rpmuint64_t memory_limit;
    size_t thread_limit;

    /* Add the per-architecture filters. */
    if (F_ISSET(z->flags, X86))
	coder_add_filter(z, LZMA_FILTER_X86, NULL);
    if (F_ISSET(z->flags, POWERPC))
	coder_add_filter(z, LZMA_FILTER_POWERPC, NULL);
    if (F_ISSET(z->flags, IA64))
	coder_add_filter(z, LZMA_FILTER_IA64, NULL);
    if (F_ISSET(z->flags, ARM))
	coder_add_filter(z, LZMA_FILTER_ARM, NULL);
    if (F_ISSET(z->flags, ARMTHUMB))
	coder_add_filter(z, LZMA_FILTER_ARMTHUMB, NULL);
    if (F_ISSET(z->flags, SPARC))
	coder_add_filter(z, LZMA_FILTER_SPARC, NULL);

    if (z->_filters_count == 0) {
	/* We are using a preset. This is not a good idea in raw mode
	   except when playing around with things. Different versions
	   of this software may use different options in presets, and
	   thus make uncompressing the raw data difficult. */
	if (z->format == RPMZ_FORMAT_RAW) {
	    /* The message is shown only if warnings are allowed
	       but the exit status isn't changed. */
	    rpmlog(RPMLOG_WARNING, _("Using a preset in raw mode is discouraged.\n"));
	    rpmlog(RPMLOG_WARNING, _("The exact options of the presets may vary between software versions.\n"));
	}

	{   size_t preset_number = z->level;

	    /* Get the preset for LZMA1 or LZMA2. */
	    if (F_ISSET(z->flags, EXTREME))
		preset_number |= LZMA_PRESET_EXTREME;

	    if (lzma_lzma_preset(&z->_options, preset_number))
assert(0);
	}

	/* Use LZMA2 except with --format=lzma we use LZMA1. */
	z->_filters[0].id = z->format == RPMZ_FORMAT_LZMA
			? LZMA_FILTER_LZMA1 : LZMA_FILTER_LZMA2;
	z->_filters[0].options = &z->_options;
	z->_filters_count = 1;
    }
#ifdef	NOTYET
    else
	preset_default = false;
#endif	/* NOTYET */

    /* Terminate the filter options array. */
    z->_filters[z->_filters_count].id = LZMA_VLI_UNKNOWN;

    /* If we are using the LZMA_Alone format, allow exactly one filter
       which has to be LZMA. */
    if (z->format == RPMZ_FORMAT_LZMA && (z->_filters_count != 1
			|| z->_filters[0].id != LZMA_FILTER_LZMA1))
	rpmlog(RPMLOG_CRIT, _("With --format=lzma only the LZMA1 filter is supported\n"));

    /* Print the selected filter chain. */
    message_filters(RPMLOG_DEBUG, z->_filters);

    /* If using --format=raw, we can be decoding. The memusage function
       also validates the filter chain and the options used for the
       filters. */
    if (z->mode == RPMZ_MODE_COMPRESS) {
	memory_usage = lzma_raw_encoder_memusage(z->_filters);
	memory_limit = hw_memlimit_encoder(z);
    } else {
	memory_usage = lzma_raw_decoder_memusage(z->_filters);
	memory_limit = hw_memlimit_decoder(z);
    }

    if (memory_usage == UINT64_MAX)
	rpmlog(RPMLOG_CRIT, "Unsupported filter chain or filter options\n");

    /* Print memory usage info. */
    rpmlog(RPMLOG_DEBUG, _("%'llu MiB (%'llu B) of memory is required per thread, limit is %'llu MiB (%'llu B)\n"),
			(unsigned long long)(memory_usage >> 20),
			(unsigned long long)memory_usage,
			(unsigned long long)(memory_limit >> 20),
			(unsigned long long)memory_limit);

    if (memory_usage > memory_limit) {
	size_t i = 0;
	lzma_options_lzma *opt;
	rpmuint32_t orig_dict_size;

	/* If --no-auto-adjust was used or we didn't find LZMA1 or
	   LZMA2 as the last filter, give an error immediatelly.
	   --format=raw implies --no-auto-adjust. */
	if (!z->_auto_adjust || z->format == RPMZ_FORMAT_RAW)
	    memlimit_too_small(memory_usage, memory_limit);

#ifdef	NOTYET
	assert(z->mode == RPMZ_MODE_COMPRESS);
#endif	/* NOTYET */

	/* Look for the last filter if it is LZMA2 or LZMA1, so
	   we can make it use less RAM. With other filters we don't
	   know what to do. */
	while (z->_filters[i].id != LZMA_FILTER_LZMA2
		&& z->_filters[i].id != LZMA_FILTER_LZMA1) {
	    if (z->_filters[i].id == LZMA_VLI_UNKNOWN)
		memlimit_too_small(memory_usage, memory_limit);

	    ++i;
	}

	/* Decrease the dictionary size until we meet the memory
	   usage limit. First round down to full mebibytes. */
	opt = z->_filters[i].options;
	orig_dict_size = opt->dict_size;
	opt->dict_size &= ~((UINT32_C(1) << 20) - 1);
	while (1) {
	    /* If it is below 1 MiB, auto-adjusting failed. We
	       could be more sophisticated and scale it down even
	       more, but let's see if many complain about this
	       version.

	       FIXME: Displays the scaled memory usage instead
	       of the original. */
	    if (opt->dict_size < (UINT32_C(1) << 20))
		memlimit_too_small(memory_usage, memory_limit);

	    memory_usage = lzma_raw_encoder_memusage(z->_filters);
assert(memory_usage != UINT64_MAX);

	    /* Accept it if it is low enough. */
	    if (memory_usage <= memory_limit)
		break;

	    /* Otherwise 1 MiB down and try again. I hope this
	       isn't too slow method for cases where the original
	       dict_size is very big. */
	    opt->dict_size -= UINT32_C(1) << 20;
	}

	/* Tell the user that we decreased the dictionary size.
	   However, omit the message if no preset or custom chain
	   was given. FIXME: Always warn? */
#ifdef	NOTYET
	if (!preset_default)
#endif	/* NOTYET */
	    rpmlog(RPMLOG_WARNING, "Adjusted LZMA%c dictionary size from %'u MiB to %'u MiB to not exceed the memory usage limit of %'llu MiB\n",
				z->_filters[i].id == LZMA_FILTER_LZMA2
					? '2' : '1',
				(unsigned)(orig_dict_size >> 20),
				(unsigned)(opt->dict_size >> 20),
				(unsigned long long)(memory_limit >> 20));
    }

    /* Limit the number of worker threads so that memory usage
       limit isn't exceeded. */
    assert(memory_usage > 0);
    thread_limit = memory_limit / memory_usage;
    if (thread_limit == 0)
	thread_limit = 1;

    if (zq->threads > thread_limit)
	zq->threads = thread_limit;

    return;
}

#ifdef	DYING
/*==============================================================*/

/**
 */
static int rpmzLoadManifests(rpmz z)
	/*@modifies z @*/
{
    ARGV_t manifests;
    const char * fn;
    int rc = 0;	/* assume success */

    if ((manifests = z->manifests) != NULL)	/* note rc=0 return with no files to load. */
    while ((fn = *manifests++) != NULL) {
	unsigned lineno;
	char * be = NULL;
	rpmiob iob = NULL;
	int xx = rpmiobSlurp(fn, &iob);
	char * f;
	char * fe;

	if (!(xx == 0 && iob != NULL)) {
	    fprintf(stderr, _("%s: Failed to open %s\n"), __progname, fn);
	    rc = -1;
	    goto bottom;
	}

	be = (char *)(iob->b + iob->blen);
	while (be > (char *)iob->b && (be[-1] == '\n' || be[-1] == '\r')) {
	  be--;
	  *be = '\0';
	}

	/* Parse and save manifest items. */
	lineno = 0;
	for (f = (char *)iob->b; *f; f = fe) {
	    const char * path;
	    char * g, * ge;
	    lineno++;

	    fe = f;
	    while (*fe && !(*fe == '\n' || *fe == '\r'))
		fe++;
	    g = f;
	    ge = fe;
	    while (*fe && (*fe == '\n' || *fe == '\r'))
		*fe++ = '\0';

	    while (*g && xisspace((int)*g))
		*g++ = '\0';
	    /* Skip comment lines. */
	    if (*g == '#')
		continue;

	    while (ge > g && xisspace(ge[-1]))
		*--ge = '\0';
	    /* Skip empty lines. */
	    if (ge == g)
		continue;

	    /* Prepend z->base_prefix if specified. */
	    if (z->base_prefix)
		path = rpmExpand(z->base_prefix, g, NULL);
	    else
		path = rpmExpand(g, NULL);
	    (void) argvAdd(&z->argv, path);
	    path = _free(path);
	}

bottom:
	iob = rpmiobFree(iob);
	if (rc != 0)
	    goto exit;
    }

exit:
    return rc;
}
#endif

/*==============================================================*/

/**
 */
void rpmzArgCallback(poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt, const char * arg,
		/*@unused@*/ void * data)
	/*@*/
{
    rpmz z = _rpmz;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'M':
    {	char * end = NULL;
	z->memlimit_custom = (rpmuint64_t) strtoll(arg, &end, 0);
	if (end == NULL || *end == '\0')
	    break;
	if (!strcmp(end, "k") || !strcmp(end, "kB"))
	    z->memlimit_custom *= 1000;
	else if (!strcmp(end, "M") || !strcmp(end, "MB"))
	    z->memlimit_custom *= 1000 * 1000;
	else if (!strcmp(end, "G") || !strcmp(end, "GB"))
	    z->memlimit_custom *= 1000 * 1000 * 1000;
	else if (!strcmp(end, "Ki") || !strcmp(end, "KiB"))
	    z->memlimit_custom *= 1024;
	else if (!strcmp(end, "Mi") || !strcmp(end, "MiB"))
	    z->memlimit_custom *= 1024 * 1024;
	else if (!strcmp(end, "Gi") || !strcmp(end, "GiB"))
	    z->memlimit_custom *= 1024 * 1024 * 1024;
	else {
	    fprintf(stderr, _("%s: Invalid memory scaling suffix \"%s\"\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
    }	break;
    case 'C':
	if (!strcmp(arg, "none"))
	    z->_checksum = LZMA_CHECK_NONE;
	else if (!strcmp(arg, "crc32"))
	    z->_checksum = LZMA_CHECK_CRC32;
	else if (!strcmp(arg, "crc64"))
	    z->_checksum = LZMA_CHECK_CRC64;
	else if (!strcmp(arg, "sha256"))
	    z->_checksum = LZMA_CHECK_SHA256;
	else {
	    fprintf(stderr, _("%s: Unknown integrity check method \"%s\"\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
	break;
    case 'F':
	if (!strcmp(arg, "auto"))
	    z->format = RPMZ_FORMAT_AUTO;
	else if (!strcmp(arg, "xz"))
	    z->format = RPMZ_FORMAT_XZ;
	else if (!strcmp(arg, "lzma") || !strcmp(arg, "alone"))
	    z->format = RPMZ_FORMAT_LZMA;
	else if (!strcmp(arg, "raw"))
	    z->format = RPMZ_FORMAT_RAW;
	else if (!strcmp(arg, "gzip") || !strcmp(arg, "gz"))
	    z->format = RPMZ_FORMAT_GZIP;
	else if (!strcmp(arg, "zlib"))
	    z->format = RPMZ_FORMAT_ZLIB;
	else if (!strcmp(arg, "zip"))
	    z->format = RPMZ_FORMAT_ZIP2;	/* XXX zip3 from pigz? */
	else if (!strcmp(arg, "bzip2") || !strcmp(arg, "bz2"))
	    z->format = RPMZ_FORMAT_BZIP2;
	else {
	    fprintf(stderr, _("%s: Unknown file format type \"%s\"\n"),
		__progname, arg);
	    /*@-exitarg@*/ exit(2); /*@=exitarg@*/
	    /*@notreached@*/
	}
	break;

    case OPT_SUBBLOCK:
	coder_add_filter(z, LZMA_FILTER_SUBBLOCK, options_subblock(optarg));
	break;
    case OPT_DELTA:
	coder_add_filter(z, LZMA_FILTER_DELTA, options_delta(optarg));
	break;
    case OPT_LZMA1:
	coder_add_filter(z, LZMA_FILTER_LZMA1, options_lzma(optarg));
	break;
    case OPT_LZMA2:
	coder_add_filter(z, LZMA_FILTER_LZMA2, options_lzma(optarg));
	break;

    case 'L':
	fprintf(stderr, "%s\n", VERSION);
	/* XXX add license display. */
	exit(EXIT_SUCCESS);
	/*@notreached@*/ break;
    case 'V':	fprintf(stderr, "%s\n", VERSION); exit(EXIT_SUCCESS);

    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/
/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmzFiltersPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmzArgCallback, 0, NULL, NULL },
/*@=type@*/

  /* ===== Custom filters */
#ifdef	REFERENCE
"  --lzma=[OPTS]       LZMA filter; OPTS is a comma-separated list of zero or\n"
"                      more of the following options (valid values; default):\n"
"                        dict=NUM   dictionary size in bytes (1 - 1Gi; 8Mi)\n"
"                        lc=NUM     number of literal context bits (0-8; 3)\n"
"                        lp=NUM     number of literal position bits (0-4; 0)\n"
"                        pb=NUM     number of position bits (0-4; 2)\n"
"                        mode=MODE  compression mode (`fast' or `best'; `best')\n"
"                        nice_len=NUM     number of fast bytes (5-273; 128)\n"
"                        mf=NAME    match finder (hc3, hc4, bt2, bt3, bt4; bt4)\n"
"                        depth=NUM    match finder cycles; 0=automatic (default)\n"
#endif
  { "lzma1", '\0', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, OPT_LZMA1,
	N_("LZMA1 filter"), N_("OPTS") },
  { "lzma2", '\0', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, OPT_LZMA2,
	N_("LZMA2 filter"), N_("OPTS") },

  { "x86", '\0', POPT_BIT_SET,			&__rpmz.flags, RPMZ_FLAGS_X86,
	N_("ix86 filter (sometimes called BCJ filter)"), NULL },
  { "bcj", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmz.flags, RPMZ_FLAGS_X86,
	N_("x86 filter (also called BCJ filter)"), NULL },
  { "powerpc", '\0', POPT_BIT_SET,		&__rpmz.flags, RPMZ_FLAGS_POWERPC,
	N_("PowerPC (big endian) filter"), NULL },
  { "ppc", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmz.flags, RPMZ_FLAGS_POWERPC,
	N_("PowerPC (big endian) filter"), NULL },
  { "ia64", '\0', POPT_BIT_SET,			&__rpmz.flags, RPMZ_FLAGS_IA64,
	N_("IA64 (Itanium) filter"), NULL },
  { "itanium", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&__rpmz.flags, RPMZ_FLAGS_IA64,
	N_("IA64 (Itanium) filter"), NULL },
  { "arm", '\0', POPT_BIT_SET,			&__rpmz.flags, RPMZ_FLAGS_ARM,
	N_("ARM filter"), NULL },
  { "armthumb", '\0', POPT_BIT_SET,		&__rpmz.flags, RPMZ_FLAGS_ARMTHUMB,
	N_("ARM-Thumb filter"), NULL },
  { "sparc", '\0', POPT_BIT_SET,		&__rpmz.flags, RPMZ_FLAGS_SPARC,
	N_("SPARC filter"), NULL },

#ifdef	REFERENCE
"  --delta=[OPTS]      Delta filter; valid OPTS (valid values; default):\n"
"                        dist=NUM  Distance between bytes being\n"
"                                      subtracted from each other (1-256; 1)\n"
#endif
  { "delta", '\0', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, OPT_DELTA,
	N_("Delta filter"), N_("OPTS") },

#ifdef	REFERENCE
"  --subblock=[OPTS]   Subblock filter; valid OPTS (valid values; default):\n"
"                        size=NUM    number of bytes of data per subblock\n"
"                                    (1 - 256Mi; 4Ki)\n"
"                        rle=NUM     run-length encoder chunk size (0-256; 0)\n"
#endif
  { "subblock", '\0', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, OPT_SUBBLOCK,
	N_("Subblock filter"), N_("OPTS") },

  POPT_TABLEEND
};

/* XXX grrr, popt needs better way to insert text strings in --help. */
/* XXX fixme: popt does post order recursion into sub-tables. */
/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmzPrivatePoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmzArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "debug", 'D', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_debug, -1,
	N_("debug spewage"), NULL },

  { "format", 'F', POPT_ARG_STRING,		NULL, 'F',
	N_("file FORMAT {auto|xz|lzma|alone|raw|gz|bz2}"), N_("FORMAT") },
  { "check", 'C', POPT_ARG_STRING,		NULL, 'C',
	N_("integrity check METHOD {none|crc32|crc64|sha256}"), N_("METHOD") },
  { "memory", 'M', POPT_ARG_STRING,		NULL,  'M',
	N_("use roughly NUM Mbytes of memory at maximum"), N_("NUM") },

	/* XXX POPT_ARG_ARGV portability. */
  { "files", '\0', POPT_ARG_ARGV,		&__rpmz.manifests, 0,
	N_("Read file names from MANIFEST"), N_("MANIFEST") },
#ifdef	NOTYET
  { "quiet", 'q',		POPT_ARG_VAL,       NULL,  'q',
	N_("suppress warnings; specify twice to suppress errors too"), NULL },
  { "verbose", 'v',		POPT_ARG_VAL,       NULL,  'v',
	N_("be verbose; specify twice for even more verbose"), NULL },
  { "help", 'h',		POPT_ARG_VAL,       NULL,  'h',
	N_("display this help and exit"), NULL },
  { "long-help", 'h',		POPT_ARG_VAL,       NULL,  'H',
	N_("display this help and exit"), NULL },
#endif
  { "version", 'V',	POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	NULL,  'V',
	N_("Display software version"), NULL },
  { "license", 'L',	POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	NULL,  'L',
	N_("Display softwre license"), NULL },

  POPT_TABLEEND
};
/*@unchecked@*/ /*@observer@*/
static struct poptOption optionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmzArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzPrivatePoptTable, 0,
        N_("\
  Compress or decompress FILEs.\n\
\n\
Options:\
"), NULL },

  /* ===== Basic compression settings */
  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzOptionsPoptTable, 0,
        N_("Compression options: "), NULL },

  /* ===== Compression options */
#ifdef	DYING
  /* XXX compressor specific flags need to be set some other way. */
  { "extreme", 'e', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,	&__rpmz.flags,  RPMZ_FLAGS_EXTREME,
	N_("extreme compression"), NULL },
#endif

  /* ===== Custom filters */
  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzFiltersPoptTable, 0,
        N_("Filters:"), NULL },

  /* ===== Metadata options */
#ifdef	NOTYET
  { "sign", 'S', POPT_ARG_STRING,		NULL, 0,
	N_("sign the data with GnuPG when compressing, or verify the signature when decompressing"), N_("PUBKEY") },
#endif

  /* ===== Other options */

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmz [OPTION]... [FILE]...\n\
  Compress or decompress FILEs.\
"), NULL },

  POPT_TABLEEND

};

int
main(int argc, char *argv[])
	/*@globals __assert_program_name,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies __assert_program_name, _rpmrepo,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = &z->_zq;

    poptContext optCon;
    int ac;
    int rc = 1;		/* assume failure. */
    int xx;
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmz";
/*@=observertrans =readonlytrans @*/

    z->zq = zq;		/* XXX initialize rpmzq */
    zq->blocksize = 128;
    zq->threads	= 8;

    /* Set modes and format based on argv[0]. */
    xx = rpmzParseArgv0(z, argv[0]);
    if (xx)
	goto exit;

    z->iob = rpmiobNew(z->nb);

    /* Make sure that stdin/stdout/stderr are open. */
    /* XXX done by rpmioInit as well. */
    io_init();

    /* Set hardware specific parameters. */
    hw_init(z);

    /* Parse environment options. */
    /* XXX NULL uses "RPMZ" envvar. */
    xx = rpmzParseEnv(z, NULL, optionsTable);
    if (xx)
	goto exit;

    /* Parse CLI options. */
    /* XXX todo: needs to be earlier. */
    optCon = rpmioInit(argc, argv, optionsTable);

    /* Never remove the source file when the destination is not on disk.
       In test mode the data is written nowhere, but setting opt_stdout
       will make the rest of the code behave well. */
    if (F_ISSET(z->flags, STDOUT) || z->mode == RPMZ_MODE_TEST) {
	z->flags |= RPMZ_FLAGS_KEEP;
	z->flags |= RPMZ_FLAGS_STDOUT;
    }

    /* If no --format flag was used, or it was --format=auto, we need to
       decide what is the target file format we are going to use. This
       depends on how we were called (checked earlier in this function). */
    if (z->mode == RPMZ_MODE_COMPRESS && z->format == RPMZ_FORMAT_AUTO)
	z->format = z->_format_compress_auto;

    /* XXX Set additional parameters from z->format. */
    switch (z->format) {
    default:
    case RPMZ_FORMAT_XZ:
	z->idio = z->odio = xzdio;
	z->osuffix = ".xz";
	if (z->mode == RPMZ_MODE_COMPRESS)
	    coder_set_compression_settings(z);
	break;
    case RPMZ_FORMAT_LZMA:
	z->idio = z->odio = lzdio;
	z->osuffix = ".lzma";
	if (z->mode == RPMZ_MODE_COMPRESS)
	    coder_set_compression_settings(z);
	break;
    case RPMZ_FORMAT_RAW:
	z->idio = z->odio = xzdio;
	z->osuffix = ".xz";	/* XXX W2DO? */
	coder_set_compression_settings(z);
	break;
    case RPMZ_FORMAT_AUTO:
fprintf(stderr, "==> warning: assuming auto format is gzip\n");
	z->format = RPMZ_FORMAT_GZIP;
	/*@fallthrough@*/
    case RPMZ_FORMAT_GZIP:
	z->idio = z->odio = gzdio;
	z->osuffix = ".gz";
	break;
    case RPMZ_FORMAT_ZLIB:
	z->idio = z->odio = gzdio;
	z->osuffix = ".zz";
	break;
    case RPMZ_FORMAT_ZIP2:
	z->idio = z->odio = gzdio;
	z->osuffix = ".zip";
	break;
    case RPMZ_FORMAT_BZIP2:
	z->idio = z->odio = bzdio;
	z->osuffix = ".bz2";
	break;
    }

    if (z->mode == RPMZ_MODE_COMPRESS) {
	int xx;
	xx = snprintf(z->ofmode, sizeof(z->ofmode)-1, "wb%u",
		(unsigned)(__rpmz.level % 10));
    }

    /* Add files from CLI. */
    {	ARGV_t av = poptGetArgs(optCon);
	if (av != NULL)
	    xx = argvAppend(&z->argv, av);
    }

    /* Add files from --files manifest(s). */
    if (z->manifests != NULL)
        xx = rpmzLoadManifests(z);

if (_debug)
argvPrint("input args", z->argv, NULL);

    ac = argvCount(z->argv);
    /* With no arguments, act as a stdin/stdout filter. */
    if (z->argv == NULL || z->argv[0] == NULL)
	ac++;

    signals_init();

    /* Process arguments. */
    for (i = 0; i < ac; i++) {
	const char * ifn;
	rpmRC frc = RPMRC_OK;

	/* Use NULL for stdin. */
	ifn = (z->argv && strcmp(z->argv[i], "-") ? z->argv[i] : NULL);

	switch (z->mode) {
	case RPMZ_MODE_COMPRESS:
	case RPMZ_MODE_DECOMPRESS:
	    frc = rpmzProcess(z, ifn);
	    break;
	case RPMZ_MODE_TEST:
	    break;
	}

	if (frc != RPMRC_OK || signals_terminating(0))
	    goto exit;
    }

    rc = 0;

exit:
    z->manifests = argvFree(z->manifests);
    z->argv = argvFree(z->argv);
    z->iob = rpmiobFree(z->iob);

    optCon = rpmioFini(optCon);

    signals_exit();

    return rc;
}
