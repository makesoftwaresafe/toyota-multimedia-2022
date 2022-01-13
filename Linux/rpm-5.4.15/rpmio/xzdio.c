/** \ingroup rpmio
 * \file rpmio/xzdio.c
 * Support for LZMA compression library.
 */

#include "system.h"
#include "rpmio_internal.h"
#include <rpmmacro.h>
#include <rpmcb.h>

#if defined(WITH_XZ)

/* provide necessary defines for inclusion of <lzma.h>
   similar to LZMAUtils's internal <common.h> and as
   explicitly stated in the top-level comment of <lzma.h> */
#ifndef UINT32_C
#	define UINT32_C(n) n ## U
#endif
#ifndef UINT32_MAX
#	define UINT32_MAX UINT32_C(4294967295)
#endif
#if SIZEOF_UNSIGNED_LONG == 4
#	ifndef UINT64_C
#		define UINT64_C(n) n ## ULL
#	endif
#else
#	ifndef UINT64_C
#		define UINT64_C(n) n ## UL
#	endif
#endif
#ifndef UINT64_MAX
#	define UINT64_MAX UINT64_C(18446744073709551615)
#endif

#include "lzma.h"

#ifndef LZMA_PRESET_DEFAULT
#define LZMA_PRESET_DEFAULT     UINT32_C(6)
#endif

#include "debug.h"

/*@access FD_t @*/

#define	XZDONLY(fd)	assert(fdGetIo(fd) == xzdio)

#define kBufferSize (1 << 15)

typedef struct xzfile {
/*@only@*/
    rpmuint8_t buf[kBufferSize];	/*!< IO buffer */
    lzma_stream strm;		/*!< LZMA stream */
/*@dependent@*/
    FILE * fp;
    int encoding;
    int eof;
} XZFILE;

/*@-globstate@*/
/*@null@*/
static XZFILE *xzopen_internal(const char *path, const char *mode, int fdno, int xz)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int level = LZMA_PRESET_DEFAULT;
    int encoding = 0;
    FILE *fp;
    XZFILE *xzfile;
    lzma_stream tmp;
    lzma_ret ret;

    for (; *mode != '\0'; mode++) {
	if (*mode == 'w')
	    encoding = 1;
	else if (*mode == 'r')
	    encoding = 0;
	else if (*mode >= '0' && *mode <= '9')
	    level = (int)(*mode - '0');
    }
    if (fdno != -1)
	fp = fdopen(fdno, encoding ? "w" : "r");
    else
	fp = fopen(path, encoding ? "w" : "r");
    if (!fp)
	return NULL;
    xzfile = (XZFILE *) calloc(1, sizeof(*xzfile));
    if (!xzfile) {
	(void) fclose(fp);
	return NULL;
    }
    xzfile->fp = fp;
    xzfile->encoding = encoding;
    xzfile->eof = 0;
    tmp = (lzma_stream)LZMA_STREAM_INIT;
    xzfile->strm = tmp;
    if (encoding) {
	if (xz) {
	    ret = lzma_easy_encoder(&xzfile->strm, level, LZMA_CHECK_CRC32);
	} else {
	    lzma_options_lzma options;
	    (void) lzma_lzma_preset(&options, level);
	    ret = lzma_alone_encoder(&xzfile->strm, &options);
	}
    } else {
	/* We set the memlimit for decompression to 100MiB which should be
	 * more than enough to be sufficient for level 9 which requires 65 MiB.
	 */
	ret = lzma_auto_decoder(&xzfile->strm, 100<<20, 0);
    }
    if (ret != LZMA_OK) {
	(void) fclose(fp);
	memset(xzfile, 0, sizeof(*xzfile));
	free(xzfile);
	return NULL;
    }
    return xzfile;
}
/*@=globstate@*/

/*@null@*/
static XZFILE *lzopen(const char *path, const char *mode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    return xzopen_internal(path, mode, -1, 0);
}

/*@null@*/
static XZFILE *lzdopen(int fdno, const char *mode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (fdno < 0)
	return NULL;
    return xzopen_internal(0, mode, fdno, 0);
}

/*@null@*/
static XZFILE *xzopen(const char *path, const char *mode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    return xzopen_internal(path, mode, -1, 1);
}

/*@null@*/
static XZFILE *xzdopen(int fdno, const char *mode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    if (fdno < 0)
	return NULL;
    return xzopen_internal(0, mode, fdno, 1);
}

static int xzflush(XZFILE *xzfile)
	/*@globals fileSystem @*/
	/*@modifies xzfile, fileSystem @*/
{
    return fflush(xzfile->fp);
}

static int xzclose(/*@only@*/ XZFILE *xzfile)
	/*@globals fileSystem @*/
	/*@modifies *xzfile, fileSystem @*/
{
    lzma_ret ret;
    size_t n;
    int rc;

    if (!xzfile)
	return -1;
    if (xzfile->encoding) {
	for (;;) {
	    xzfile->strm.avail_out = kBufferSize;
	    xzfile->strm.next_out = (uint8_t *)xzfile->buf;
	    ret = lzma_code(&xzfile->strm, LZMA_FINISH);
	    if (ret != LZMA_OK && ret != LZMA_STREAM_END)
		return -1;
	    n = kBufferSize - xzfile->strm.avail_out;
	    if (n && fwrite(xzfile->buf, 1, n, xzfile->fp) != n)
		return -1;
	    if (ret == LZMA_STREAM_END)
		break;
	}
    }
    lzma_end(&xzfile->strm);
    rc = fclose(xzfile->fp);
    memset(xzfile, 0, sizeof(*xzfile));
    free(xzfile);
    return rc;
}

/*@-mustmod@*/
static ssize_t xzread(XZFILE *xzfile, void *buf, size_t len)
	/*@globals fileSystem @*/
	/*@modifies xzfile, *buf, fileSystem @*/
{
    lzma_ret ret;
    int eof = 0;

    if (!xzfile || xzfile->encoding)
      return -1;
    if (xzfile->eof)
      return 0;
/*@-temptrans@*/
    xzfile->strm.next_out = (uint8_t *) buf;
/*@=temptrans@*/
    xzfile->strm.avail_out = len;
    for (;;) {
	if (!xzfile->strm.avail_in) {
	    xzfile->strm.next_in = (uint8_t *)xzfile->buf;
	    xzfile->strm.avail_in = fread(xzfile->buf, 1, kBufferSize, xzfile->fp);
	    if (!xzfile->strm.avail_in)
		eof = 1;
	}
	ret = lzma_code(&xzfile->strm, LZMA_RUN);
	if (ret == LZMA_STREAM_END) {
	    xzfile->eof = 1;
	    return len - xzfile->strm.avail_out;
	}
	if (ret != LZMA_OK)
	    return -1;
	if (!xzfile->strm.avail_out)
	    return len;
	if (eof)
	    return -1;
      }
    /*@notreached@*/
}
/*@=mustmod@*/

static ssize_t xzwrite(XZFILE *xzfile, void *buf, size_t len)
	/*@globals fileSystem @*/
	/*@modifies xzfile, fileSystem @*/
{
    lzma_ret ret;
    size_t n;

    if (!xzfile || !xzfile->encoding)
	return -1;
    if (!len)
	return 0;
/*@-temptrans@*/
    xzfile->strm.next_in = (const uint8_t *) buf;
/*@=temptrans@*/
    xzfile->strm.avail_in = len;
    for (;;) {
	xzfile->strm.next_out = (uint8_t *)xzfile->buf;
	xzfile->strm.avail_out = kBufferSize;
	ret = lzma_code(&xzfile->strm, LZMA_RUN);
	if (ret != LZMA_OK)
	    return -1;
	n = kBufferSize - xzfile->strm.avail_out;
	if (n && fwrite(xzfile->buf, 1, n, xzfile->fp) != n)
	    return -1;
	if (!xzfile->strm.avail_in)
	    return len;
    }
    /*@notreached@*/
}

/* =============================================================== */

static inline /*@dependent@*/ /*@null@*/ void * xzdFileno(FD_t fd)
	/*@*/
{
    void * rc = NULL;
    int i;

    FDSANE(fd);
    for (i = fd->nfps; i >= 0; i--) {
/*@-boundsread@*/
	    FDSTACK_t * fps = &fd->fps[i];
/*@=boundsread@*/
	    if (fps->io != xzdio && fps->io != lzdio)
		continue;
	    rc = fps->fp;
	break;
    }
    
    return rc;
}

/*@-globuse@*/
static /*@null@*/ FD_t lzdOpen(const char * path, const char * fmode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    FD_t fd;
    mode_t mode = (fmode && fmode[0] == 'w' ? O_WRONLY : O_RDONLY);
    XZFILE * xzfile = lzopen(path, fmode);

    if (xzfile == NULL)
	return NULL;
    fd = fdNew("open (lzdOpen)");
    fdPop(fd); fdPush(fd, lzdio, xzfile, -1);
    fdSetOpen(fd, path, fileno(xzfile->fp), mode);
    return fdLink(fd, "lzdOpen");
}
/*@=globuse@*/

/*@-globuse@*/
static /*@null@*/ FD_t lzdFdopen(void * cookie, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    int fdno = fdFileno(fd);
    XZFILE *xzfile;

assert(fmode != NULL);
    fdSetFdno(fd, -1);          /* XXX skip the fdio close */
    if (fdno < 0) return NULL;
    xzfile = lzdopen(fdno, fmode);
    if (xzfile == NULL) return NULL;
    fdPush(fd, lzdio, xzfile, fdno);
    return fdLink(fd, "lzdFdopen");
}
/*@=globuse@*/

/*@-globuse@*/
static /*@null@*/ FD_t xzdOpen(const char * path, const char * fmode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    FD_t fd;
    mode_t mode = (fmode && fmode[0] == 'w' ? O_WRONLY : O_RDONLY);
    XZFILE * xzfile = xzopen(path, fmode);

    if (xzfile == NULL)
	return NULL;
    fd = fdNew("open (xzdOpen)");
    fdPop(fd); fdPush(fd, xzdio, xzfile, -1);
    fdSetOpen(fd, path, fileno(xzfile->fp), mode);
    return fdLink(fd, "xzdOpen");
}
/*@=globuse@*/

/*@-globuse@*/
static /*@null@*/ FD_t xzdFdopen(void * cookie, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    int fdno = fdFileno(fd);
    XZFILE *xzfile;

assert(fmode != NULL);
    fdSetFdno(fd, -1);          /* XXX skip the fdio close */
    if (fdno < 0) return NULL;
    xzfile = xzdopen(fdno, fmode);
    if (xzfile == NULL) return NULL;
    fdPush(fd, xzdio, xzfile, fdno);
    return fdLink(fd, "xzdFdopen");
}
/*@=globuse@*/

/*@-globuse@*/
static int xzdFlush(void * cookie)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    FD_t fd = c2f(cookie);
    XZFILE * xzfile = (XZFILE *) xzdFileno(fd);
    return xzflush(xzfile);
}
/*@=globuse@*/

/* =============================================================== */
/*@-globuse@*/
/*@-mustmod@*/          /* LCL: *buf is modified */
static ssize_t xzdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies *buf, fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    XZFILE *xzfile;
    ssize_t rc = -1;

assert(fd != NULL);
    if (fd->bytesRemain == 0) return 0; /* XXX simulate EOF */
    xzfile = (XZFILE *) xzdFileno(fd);
assert(xzfile != NULL);
    fdstat_enter(fd, FDSTAT_READ);
/*@-compdef@*/
    rc = xzread(xzfile, buf, count);
/*@=compdef@*/
DBGIO(fd, (stderr, "==>\txzdRead(%p,%p,%u) rc %lx %s\n", cookie, buf, (unsigned)count, (unsigned long)rc, fdbg(fd)));
    if (rc == -1) {
	fd->errcookie = "Lzma: decoding error";
    } else if (rc >= 0) {
	fdstat_exit(fd, FDSTAT_READ, rc);
	/*@-compdef@*/
	if (fd->ndigests > 0 && rc > 0) fdUpdateDigests(fd, (const unsigned char  *)buf, rc);
	/*@=compdef@*/
    }
    return rc;
}
/*@=mustmod@*/
/*@=globuse@*/

/*@-globuse@*/
static ssize_t xzdWrite(void * cookie, const char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    XZFILE *xzfile;
    ssize_t rc = 0;

    if (fd == NULL || fd->bytesRemain == 0) return 0;   /* XXX simulate EOF */

    if (fd->ndigests > 0 && count > 0) fdUpdateDigests(fd, (const unsigned char  *)buf, count);

    xzfile = (XZFILE *) xzdFileno(fd);

    fdstat_enter(fd, FDSTAT_WRITE);
    rc = xzwrite(xzfile, (void *)buf, count);
DBGIO(fd, (stderr, "==>\txzdWrite(%p,%p,%u) rc %lx %s\n", cookie, buf, (unsigned)count, (unsigned long)rc, fdbg(fd)));
    if (rc < 0) {
	fd->errcookie = "Lzma: encoding error";
    } else if (rc > 0) {
	fdstat_exit(fd, FDSTAT_WRITE, rc);
    }
    return rc;
}

static int xzdSeek(void * cookie, /*@unused@*/ _libio_pos_t pos,
			/*@unused@*/ int whence)
	/*@*/
{
    FD_t fd = c2f(cookie);

    XZDONLY(fd);
    return -2;
}

static int xzdClose( /*@only@*/ void * cookie)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    XZFILE *xzfile;
    const char * errcookie;
    int rc;

    xzfile = (XZFILE *) xzdFileno(fd);

    if (xzfile == NULL) return -2;
    errcookie = strerror(ferror(xzfile->fp));

    fdstat_enter(fd, FDSTAT_CLOSE);
    /*@-dependenttrans@*/
    rc = xzclose(xzfile);
    /*@=dependenttrans@*/
    fdstat_exit(fd, FDSTAT_CLOSE, rc);

    if (fd && rc == -1)
	fd->errcookie = errcookie;

DBGIO(fd, (stderr, "==>\txzdClose(%p) rc %lx %s\n", cookie, (unsigned long)rc, fdbg(fd)));

    if (_rpmio_debug || rpmIsDebug()) fdstat_print(fd, "XZDIO", stderr);
    /*@-branchstate@*/
    if (rc == 0)
	fd = fdFree(fd, "open (xzdClose)");
    /*@=branchstate@*/
    return rc;
}

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s lzdio_s = {
  xzdRead, xzdWrite, xzdSeek, xzdClose, lzdOpen, lzdFdopen, xzdFlush,
};
/*@=type@*/

FDIO_t lzdio = /*@-compmempass@*/ &lzdio_s /*@=compmempass@*/ ;

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s xzdio_s = {
  xzdRead, xzdWrite, xzdSeek, xzdClose, xzdOpen, xzdFdopen, xzdFlush,
};
/*@=type@*/

FDIO_t xzdio = /*@-compmempass@*/ &xzdio_s /*@=compmempass@*/ ;

#endif

