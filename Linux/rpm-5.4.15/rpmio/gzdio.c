/** \ingroup rpmio
 * \file rpmio/gzdio.c
 * Support for ZLIB compression library.
 */

#include "system.h"

#if	defined(HAVE_STDBOOL_H) || defined(__LCLINT__)
#include <stdbool.h>
#else
typedef	enum { true = 1, false = 0 } bool;
#endif

#include "rpmio_internal.h"
#include <rpmmacro.h>
#include <rpmcb.h>

#if defined(WITH_ZLIB)

/*@-noparams@*/
#include <zlib.h>
/*@=noparams@*/

#include "debug.h"

/*@access FD_t @*/

#define	GZDONLY(fd)	assert(fdGetIo(fd) == gzdio)

typedef struct cpio_state_s {
    rpmuint32_t n;			/* byte progress in cpio header */
    rpmuint32_t mode;			/* file attributes */
    rpmuint32_t nlnk;
    rpmuint32_t size;
} * cpio_state;

#define RSYNC_WIN 4096

typedef struct rsync_state_s {
    rpmuint32_t n;			/* number of elements in the window */
    rpmuint32_t sum;			/* current sum */
    unsigned char win[RSYNC_WIN];	/* window elements */
} * rsync_state;

typedef struct rpmGZFILE_s {
    gzFile gz;				/* gzFile is a pointer */
    struct rsync_state_s rs;
    struct cpio_state_s cs;
    rpmuint32_t nb;			/* bytes pending for sync */
} * rpmGZFILE;				/* like FILE, to use with star */


#ifdef __cplusplus
GENfree(rpmGZFILE)
#endif	/* __cplusplus */

/* Should gzflush be called only after RSYNC_WIN boundaries? */
/*@unchecked@*/
static int enable_rsync = 1;

/* =============================================================== */
/* from ../lib/cpio.h */
#define CPIO_NEWC_MAGIC "070701"
#define PHYS_HDR_SIZE 110

#define OFFSET_MODE (sizeof(CPIO_NEWC_MAGIC)-1 + 1*8)
#define OFFSET_NLNK (sizeof(CPIO_NEWC_MAGIC)-1 + 4*8)
#define OFFSET_SIZE (sizeof(CPIO_NEWC_MAGIC)-1 + 6*8)

static inline
int hex(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (int)(c - '0');
    else if (c >= 'a' && c <= 'f')
	return (int)(c - 'a') + 10;
    else if (c >= 'A' && c <= 'F')
	return (int)(c - 'A') + 10;
    return -1;
}

static inline
bool cpio_next(cpio_state s, unsigned char c)
	/*@modifies s @*/
{
    if (s->n >= sizeof(CPIO_NEWC_MAGIC)-1) {
	int d = hex(c);
	if (d < 0) {
	    s->n = 0;
	    return false;
	}
	if (0){} /* indent */
	else if (s->n >= OFFSET_MODE && s->n < OFFSET_MODE+8) {
	    if (s->n == OFFSET_MODE)
		s->mode = 0;
	    else
		s->mode <<= 4;
	    s->mode |= d;
	}
	else if (s->n >= OFFSET_NLNK && s->n < OFFSET_NLNK+8) {
	    if (s->n == OFFSET_NLNK)
		s->nlnk = 0;
	    else
		s->nlnk <<= 4;
	    s->nlnk |= d;
	}
	else if (s->n >= OFFSET_SIZE && s->n < OFFSET_SIZE+8) {
	    if (s->n == OFFSET_SIZE)
		s->size = 0;
	    else
		s->size <<= 4;
	    s->size |= d;
	}
	s->n++;
	if (s->n >= PHYS_HDR_SIZE) {
	    s->n = 0;
	    if (!S_ISREG(s->mode) || s->nlnk != 1)
		/* no file data */
		s->size = 0;
	    return true;
	}
    }
    else if (CPIO_NEWC_MAGIC[s->n] == c) {
	s->n++;
    }
    else {
	s->n = 0;
    }
    return false;
}

/* =============================================================== */
static inline
bool rsync_next(rsync_state s, unsigned char c)
	/*@modifies s @*/
{
    rpmuint32_t i;

    if (s->n < RSYNC_WIN) {		/* not enough elements */
	s->sum += (rpmuint32_t)c;	/* update the sum */
	s->win[s->n++] = c;		/* remember the element */
	return false;			/* no match */
    }
    i = s->n++ % RSYNC_WIN;		/* wrap up */
    s->sum -= (rpmuint32_t)s->win[i];	/* move the window on */
    s->sum += (rpmuint32_t)c;
    s->win[i] = c;
    if (s->sum % RSYNC_WIN == 0) {	/* match */
	s->n = 0;			/* reset */
	s->sum = 0;
	return true;
    }
    return false;
}

#define CHUNK 4096

static inline
bool sync_hint(rpmGZFILE rpmgz, unsigned char c)
	/*@modifies rpmgz @*/
{
    bool cpio_hint;
    bool rsync_hint;

    rpmgz->nb++;
    cpio_hint = cpio_next(&rpmgz->cs, c);
    if (cpio_hint) {
	/* cpio header/data boundary */
	rpmgz->rs.n = rpmgz->rs.sum = 0;
	if (rpmgz->nb >= 2*CHUNK)
	    /* better sync here */
	    goto cpio_sync;
	if (rpmgz->cs.size < CHUNK)
	    /* file is too small */
	    return false;
	if (rpmgz->nb < CHUNK/2)
	    /* not enough pending bytes */
	    return false;
    cpio_sync:
	rpmgz->nb = 0;
	return true;
    }
    rsync_hint = rsync_next(&rpmgz->rs, c);
    if (rsync_hint) {
	/* rolling checksum match */
	assert(rpmgz->nb >= RSYNC_WIN);
	rpmgz->nb = 0;
	return true;
    }
    return false;
}

static ssize_t
rsyncable_gzwrite(rpmGZFILE rpmgz, const unsigned char *const buf, const size_t len)
	/*@modifies rpmgz @*/
{
    ssize_t rc;
    size_t n;
    ssize_t n_written = 0;
    const unsigned char *begin = buf;
    size_t i;

    for (i = 0; i < len; i++) {
	if (!sync_hint(rpmgz, buf[i]))
	    continue;
	n = i + 1 - (begin - buf);
	rc = gzwrite(rpmgz->gz, begin, (unsigned)n);
	if (rc < 0)
	    return (n_written ? n_written : rc);
	n_written += rc;
	if (rc < (ssize_t)n)
	    return n_written;
	begin += n;
	rc = gzflush(rpmgz->gz, Z_SYNC_FLUSH);
	if (rc < 0)
	    return (n_written ? n_written : rc);
    }
    if (begin < buf + len) {
	n = len - (begin - buf);
	rc = gzwrite(rpmgz->gz, begin, (unsigned)n);
	if (rc < 0)
	    return (n_written ? n_written : rc);
	n_written += rc;
    }
    return n_written;
}

/* =============================================================== */
/*@-moduncon@*/

static inline /*@dependent@*/ /*@null@*/ void * gzdFileno(FD_t fd)
	/*@*/
{
    void * rc = NULL;
    int i;

    FDSANE(fd);
    for (i = fd->nfps; i >= 0; i--) {
	FDSTACK_t * fps = &fd->fps[i];
	if (fps->io != gzdio)
	    continue;
	rc = fps->fp;
	break;
    }

    return rc;
}

static /*@null@*/
FD_t gzdOpen(const char * path, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd;
    rpmGZFILE rpmgz;
    mode_t mode = (fmode && fmode[0] == 'w' ? O_WRONLY : O_RDONLY);

    rpmgz = (rpmGZFILE) xcalloc(1, sizeof(*rpmgz));
    rpmgz->gz = gzopen(path, fmode);
    if (rpmgz->gz == NULL) {
	rpmgz = _free(rpmgz);
	return NULL;
    }
    fd = fdNew("open (gzdOpen)");
    fdPop(fd); fdPush(fd, gzdio, rpmgz, -1);
    fdSetOpen(fd, path, -1, mode);

DBGIO(fd, (stderr, "==>\tgzdOpen(\"%s\", \"%s\") fd %p %s\n", path, fmode, (fd ? fd : NULL), fdbg(fd)));
    return fdLink(fd, "gzdOpen");
}

static /*@null@*/ FD_t gzdFdopen(void * cookie, const char *fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    int fdno;
    rpmGZFILE rpmgz;

    if (fmode == NULL) return NULL;
    fdno = fdFileno(fd);
    fdSetFdno(fd, -1);		/* XXX skip the fdio close */
    if (fdno < 0) return NULL;
    rpmgz = (rpmGZFILE) xcalloc(1, sizeof(*rpmgz));
    rpmgz->gz = gzdopen(fdno, fmode);
    if (rpmgz->gz == NULL) {
	rpmgz = _free(rpmgz);
	return NULL;
    }

    fdPush(fd, gzdio, rpmgz, fdno);		/* Push gzdio onto stack */

    return fdLink(fd, "gzdFdopen");
}

static int gzdFlush(void * cookie)
	/*@*/
{
    FD_t fd = c2f(cookie);
    rpmGZFILE rpmgz;
    rpmgz = (rpmGZFILE) gzdFileno(fd);
    if (rpmgz == NULL) return -2;
    return gzflush(rpmgz->gz, Z_SYNC_FLUSH);	/* XXX W2DO? */
}

/* =============================================================== */
/*@-mustmod@*/
static ssize_t gzdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies buf, fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmGZFILE rpmgz;
    ssize_t rc;

    if (fd == NULL || fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */

    rpmgz = (rpmGZFILE) gzdFileno(fd);
    if (rpmgz == NULL) return -2;	/* XXX can't happen */

    fdstat_enter(fd, FDSTAT_READ);
    rc = gzread(rpmgz->gz, buf, (unsigned)count);
DBGIO(fd, (stderr, "==>\tgzdRead(%p,%p,%u) rc %lx %s\n", cookie, buf, (unsigned)count, (unsigned long)rc, fdbg(fd)));
    if (rc < 0) {
	int zerror = 0;
	fd->errcookie = gzerror(rpmgz->gz, &zerror);
	if (zerror == Z_ERRNO) {
	    fd->syserrno = errno;
	    fd->errcookie = strerror(fd->syserrno);
	}
    } else {
	fdstat_exit(fd, FDSTAT_READ, (rc > 0 ? rc : 0));
	if (fd->ndigests > 0 && rc > 0) fdUpdateDigests(fd, (const unsigned char *)buf, rc);
    }
    return rc;
}
/*@=mustmod@*/

static ssize_t gzdWrite(void * cookie, const char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmGZFILE rpmgz;
    ssize_t rc;

    if (fd == NULL || fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */

    if (fd->ndigests > 0 && count > 0) fdUpdateDigests(fd, (const unsigned char *)buf, count);

    rpmgz = (rpmGZFILE) gzdFileno(fd);
    if (rpmgz == NULL) return -2;	/* XXX can't happen */

    fdstat_enter(fd, FDSTAT_WRITE);
    if (enable_rsync)
	rc = rsyncable_gzwrite(rpmgz, (const unsigned char *)buf, (unsigned)count);
    else
	rc = gzwrite(rpmgz->gz, (void *)buf, (unsigned)count);
DBGIO(fd, (stderr, "==>\tgzdWrite(%p,%p,%u) rc %lx %s\n", cookie, buf, (unsigned)count, (unsigned long)rc, fdbg(fd)));
    if (rc < (ssize_t)count) {
	int zerror = 0;
	fd->errcookie = gzerror(rpmgz->gz, &zerror);
	if (zerror == Z_ERRNO) {
	    fd->syserrno = errno;
	    fd->errcookie = strerror(fd->syserrno);
	}
    }
    if (rc > 0)
	fdstat_exit(fd, FDSTAT_WRITE, rc);
    return rc;
}

/* XXX zlib-1.0.4 has not */
#define	HAVE_GZSEEK	/* XXX autoFu doesn't set this anymore. */
static int gzdSeek(void * cookie, _libio_pos_t pos, int whence)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int rc;
#if defined(HAVE_GZSEEK)
#ifdef USE_COOKIE_SEEK_POINTER
    _IO_off64_t p = *pos;
#else
    off_t p = pos;
#endif
    FD_t fd = c2f(cookie);
    rpmGZFILE rpmgz;

    if (fd == NULL) return -2;
    assert(fd->bytesRemain == -1);	/* XXX FIXME */

    rpmgz = (rpmGZFILE) gzdFileno(fd);
    if (rpmgz == NULL) return -2;	/* XXX can't happen */

    fdstat_enter(fd, FDSTAT_SEEK);
    rc = gzseek(rpmgz->gz, (long)p, whence);
DBGIO(fd, (stderr, "==>\tgzdSeek(%p,%ld,%d) rc %lx %s\n", cookie, (long)p, whence, (unsigned long)rc, fdbg(fd)));
    if (rc < 0) {
	int zerror = 0;
	fd->errcookie = gzerror(rpmgz->gz, &zerror);
	if (zerror == Z_ERRNO) {
	    fd->syserrno = errno;
	    fd->errcookie = strerror(fd->syserrno);
	}
    }
    if (rc > 0)
	fdstat_exit(fd, FDSTAT_SEEK, rc);
#else
    rc = -2;
#endif
    return rc;
}

static int gzdClose( /*@only@*/ void * cookie)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmGZFILE rpmgz;
    int rc;

    rpmgz = (rpmGZFILE) gzdFileno(fd);
    if (rpmgz == NULL) return -2;	/* XXX can't happen */

    fdstat_enter(fd, FDSTAT_CLOSE);
    /*@-dependenttrans@*/
    rc = gzclose(rpmgz->gz);
    /*@=dependenttrans@*/
    rpmgz->gz = NULL;
/*@-dependenttrans@*/
    rpmgz = _free(rpmgz);
/*@=dependenttrans@*/

    /* XXX TODO: preserve fd if errors */

    if (fd) {
DBGIO(fd, (stderr, "==>\tgzdClose(%p) zerror %d %s\n", cookie, rc, fdbg(fd)));
	if (rc < 0) {
	    fd->errcookie = "gzclose error";
	    if (rc == Z_ERRNO) {
		fd->syserrno = errno;
		fd->errcookie = strerror(fd->syserrno);
	    }
	} else if (rc >= 0) {
	    fdstat_exit(fd, FDSTAT_CLOSE, rc);
	}
    }

DBGIO(fd, (stderr, "==>\tgzdClose(%p) rc %lx %s\n", cookie, (unsigned long)rc, fdbg(fd)));

    if (_rpmio_debug || rpmIsDebug()) fdstat_print(fd, "GZDIO", stderr);
    if (rc == 0)
	fd = fdFree(fd, "open (gzdClose)");
    return rc;
}

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s gzdio_s = {
  gzdRead, gzdWrite, gzdSeek, gzdClose,	gzdOpen, gzdFdopen, gzdFlush,
};
/*@=type@*/

FDIO_t gzdio = /*@-compmempass@*/ &gzdio_s /*@=compmempass@*/ ;

/*@=moduncon@*/
#endif	/* WITH_ZLIB */
