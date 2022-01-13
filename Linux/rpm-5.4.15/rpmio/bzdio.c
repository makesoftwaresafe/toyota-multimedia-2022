/** \ingroup rpmio
 * \file rpmio/bzdio.c
 * Support for BZIP2 compression library.
 */

#include "system.h"
#include "rpmio_internal.h"
#include <rpmmacro.h>
#include <rpmcb.h>

#if defined(WITH_BZIP2)

#define	_RPMBZ_INTERNAL
#include "rpmbz.h"

#include "debug.h"

/*@access FD_t @*/

#define	BZDONLY(fd)	assert(fdGetIo(fd) == bzdio)

const char * rpmbzStrerror(rpmbz bz)
	/*@*/
{
    return BZ2_bzerror(bz->bzfile, &bz->bzerr);
}

/*@-mustmod@*/
void rpmbzClose(rpmbz bz, int abort, /*@null@*/ const char ** errmsg)
	/*@modifies bz, *errmsg @*/
{
    if (bz->bzfile != NULL) {
	if (bz->omode == O_RDONLY)
	    BZ2_bzReadClose(&bz->bzerr, bz->bzfile);
	else
	    BZ2_bzWriteClose(&bz->bzerr, bz->bzfile, abort,
		&bz->nbytes_in, &bz->nbytes_out);
/*@-usereleased@*/	/* XXX does bz->bzfile persist after *Close? */
	if (bz->bzerr != BZ_OK && errmsg)
	    *errmsg = rpmbzStrerror(bz);
/*@=usereleased@*/
    }
    bz->bzfile = NULL;
}
/*@=mustmod@*/

/*@only@*/ /*@null@*/
rpmbz rpmbzFree(/*@only@*/ rpmbz bz, int abort)
	/*@globals fileSystem @*/
	/*@modifies bz, fileSystem @*/
{
    rpmbzClose(bz, abort, NULL);
    if (bz->fp != NULL) {
	(void) fclose(bz->fp);
	bz->fp = NULL;
    }
    return rpmbzFini(bz);
}

/*@-mustmod@*/
/*@only@*/
rpmbz rpmbzNew(const char * path, const char * fmode, int fdno)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmbz bz;
    int level = -1;	/* XXX use _bzdB default */
    mode_t omode = O_RDONLY;
    int small = -1;	/* XXX use _bzdS default */
    int verbosity = -1;	/* XXX use _bzdV default */
    const char * s = fmode;
    char stdio[20];
    char *t = stdio;
    char *te = t + sizeof(stdio) - 2;
    int c;

assert(fmode != NULL);		/* XXX return NULL instead? */

    switch ((c = *s++)) {
    case 'a':
    case 'w':
	omode = O_WRONLY;
	*t++ = (char)c;
	break;
    case 'r':
	omode = O_RDONLY;
	*t++ = (char)c;
	break;
    }
	
    while ((c = *s++) != 0) {
    switch (c) {
    case '.':
	/*@switchbreak@*/ break;
    case '+':
    case 'x':
    case 'm':
    case 'c':
    case 'b':
	if (t < te) *t++ = c;
	/*@switchbreak@*/ break;
    case 's':
	if (small < 0) small = 0;
	small++;
	/*@switchbreak@*/ break;
    case 'q':
	verbosity = 0;
	/*@switchbreak@*/ break;
    case 'v':
	if (verbosity < 0) verbosity = 0;
	if (verbosity < 4) verbosity++;
	/*@switchbreak@*/ break;
    default:
	if (c >= (int)'0' && c <= (int)'9')
	    level = c - (int)'0';
	/*@switchbreak@*/ break;
    }}
    *t = '\0';

    bz = rpmbzInit(level, small, verbosity, omode);

    if (fdno >= 0) {
	if ((bz->fp = fdopen(fdno, stdio)) != NULL)
	    bz->bzfile = (bz->omode == O_RDONLY)
		? BZ2_bzReadOpen(&bz->bzerr, bz->fp, bz->V, bz->S, NULL, 0)
		: BZ2_bzWriteOpen(&bz->bzerr, bz->fp, bz->B, bz->V, bz->W);
    } else if (path != NULL) {
	if ((bz->fp = fopen(path, stdio)) != NULL)
	    bz->bzfile = (bz->omode == O_RDONLY)
		? BZ2_bzReadOpen(&bz->bzerr, bz->fp, bz->V, bz->S, NULL, 0)
		: BZ2_bzWriteOpen(&bz->bzerr, bz->fp, bz->B, bz->V, bz->W);
    }

/*@-usereleased@*/
    return (bz->bzfile != NULL ? bz : rpmbzFree(bz, 0));
/*@=usereleased@*/
}
/*@=mustmod@*/

#ifdef	NOTYET
/*@-mustmod -nullstate@*/
static void rpmbzCompress(rpmbz bz, rpmzJob job)
	/*@globals fileSystem @*/
	/*@modifies bz, job, fileSystem @*/
{
    bz_stream *bzstrm = &bz->strm;
    size_t len;			/* remaining bytes to compress/check */
    int ret;

    /* initialize the deflate stream for this block */
    bzstrm->bzfree = NULL;
    bzstrm->bzalloc = NULL;
    bzstrm->opaque = NULL;
    if ((ret = BZ2_bzCompressInit(bzstrm, bz->B, bz->V, bz->W)) != BZ_OK)
	bail("not enough memory", "BZ2_bzCompressInit");

    bzstrm->next_in = job->in->buf;
    bzstrm->next_out = job->out->buf;
    bzstrm->avail_out = job->out->len;

    /* run _PIGZMAX-sized amounts of input through deflate -- this loop is
    * needed for those cases where the integer type is smaller than the
    * size_t type, or when len is close to the limit of the size_t type */
    len = job->in->len;
    while (len > _PIGZMAX) {
	bzstrm->avail_in = _PIGZMAX;
	if ((ret = BZ2_bzCompress(bzstrm, BZ_RUN)) != BZ_RUN_OK)
	    fprintf(stderr, "*** BZ2_bzCompress(%d): %d\n", BZ_RUN, ret);
assert(bzstrm->avail_in == 0 && bzstrm->avail_out != 0);
	len -= _PIGZMAX;
    }

    /* run the last piece through deflate -- terminate with a sync marker,
    * or finish deflate stream if this is the last block */
    bzstrm->avail_in = (unsigned)len;
    ret = BZ2_bzCompress(bzstrm, BZ_FINISH);
    if (!(ret == BZ_FINISH_OK || ret == BZ_STREAM_END))
	fprintf(stderr, "*** BZ2_bzCompress(%d): %d\n", BZ_FINISH, ret);
    if ((ret = BZ2_bzCompressEnd(bzstrm)) != BZ_OK)
	fprintf(stderr, "*** BZ2_bzCompressEnd: %d\n", ret);
#ifdef	NOTYET
assert(bzstrm->avail_in == 0 && bzstrm->avail_out != 0);
#endif

}
/*@=mustmod =nullstate@*/

/*@-mustmod -nullstate@*/
static void rpmbzDecompress(rpmbz bz, rpmzJob job)
	/*@globals fileSystem @*/
	/*@modifies bz, job, fileSystem @*/
{
    bz_stream *bzstrm = &bz->strm;
    int ret;

    /* initialize the inflate stream for this block */
    bzstrm->bzfree = NULL;
    bzstrm->bzalloc = NULL;
    bzstrm->opaque = NULL;
    if ((ret = BZ2_bzDecompressInit(bzstrm, bz->V, bz->S)) != BZ_OK)
	bail("not enough memory", "BZ2_bzDecompressInit");

    bzstrm->next_in = job->in->buf;
    bzstrm->avail_in = job->in->len;
    bzstrm->next_out = job->out->buf;
    bzstrm->avail_out = job->out->len;

    if ((ret = BZ2_bzDecompress(bzstrm)) != BZ_RUN_OK)
	fprintf(stderr, "*** BZ2_bzDecompress: %d\n", ret);

    job->out->len -= bzstrm->avail_out;

    if ((ret = BZ2_bzDecompressEnd(bzstrm)) != BZ_OK)
	fprintf(stderr, "*** BZ2_bzDecompressEnd: %d\n", ret);

}
/*@=mustmod =nullstate@*/
#endif	/* NOTYET */

/*@-mustmod@*/
ssize_t rpmbzRead(rpmbz bz, /*@out@*/ char * buf, size_t count,
		/*@null@*/ const char ** errmsg)
	/*@globals internalState @*/
	/*@modifies bz, *buf, *errmsg, internalState @*/
{
    ssize_t rc = 0;

#ifdef	NOTYET	/* XXX hmmm, read after close needs to return EOF. */
assert(bz->bzfile != NULL);
#else
    if (bz->bzfile == NULL) return 0;
#endif
    rc = BZ2_bzRead(&bz->bzerr, bz->bzfile, buf, (int)count);
    switch (bz->bzerr) {
    case BZ_STREAM_END: {
	void * unused = NULL;
	int nUnused = 0;
	    
	BZ2_bzReadGetUnused(&bz->bzerr, bz->bzfile, &unused, &nUnused);
	if (unused != NULL && nUnused > 0)
	    unused = memcpy(xmalloc(nUnused), unused, nUnused);
	else {
	    unused = NULL;
	    nUnused = 0;
	}
	rpmbzClose(bz, 0, NULL);
	bz->bzfile = BZ2_bzReadOpen(&bz->bzerr, bz->fp, bz->V, bz->S,
			unused, nUnused);
	unused = _free(unused);
    }   /*@fallthrough@*/
    case BZ_OK:
assert(rc >= 0);
	break;
    default:
	rc = -1;
	if (errmsg != NULL)
	    *errmsg = rpmbzStrerror(bz);
	rpmbzClose(bz, 1, NULL);
	break;
    }
    return rc;
}
/*@=mustmod@*/

ssize_t rpmbzWrite(rpmbz bz, const char * buf, size_t count,
		/*@null@*/ const char ** errmsg)
	/*@globals internalState @*/
	/*@modifies bz, *errmsg, internalState @*/
{
    ssize_t rc;

assert(bz->bzfile != NULL);	/* XXX TODO: lazy open? */
    BZ2_bzWrite(&bz->bzerr, bz->bzfile, (void *)buf, (int)count);
    switch (bz->bzerr) {
    case BZ_OK:
	rc = count;
	break;
    default:
	if (errmsg != NULL)
	    *errmsg = rpmbzStrerror(bz);
	rpmbzClose(bz, 1, NULL);
	rc = -1;
	break;
    }
    return rc;
}

static int rpmbzSeek(/*@unused@*/ void * _bz, /*@unused@*/ _libio_pos_t pos,
			/*@unused@*/ int whence)
	/*@*/
{
    return -2;
}

static /*@null@*/ rpmbz rpmbzOpen(const char * path, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return  rpmbzNew(path, fmode, -1);
}

static /*@null@*/ rpmbz rpmbzFdopen(void * _fdno, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int fdno = (int)((long)_fdno);	/* XXX hack */
    return  rpmbzNew(NULL, fmode, fdno);
}

static int rpmbzFlush(void * _bz)
	/*@*/
{
    rpmbz bz = (rpmbz) _bz;
    return BZ2_bzflush(bz->bzfile);
}

/* =============================================================== */
static inline /*@dependent@*/ /*@null@*/ void * bzdFileno(FD_t fd)
	/*@*/
{
    void * rc = NULL;
    int i;

    FDSANE(fd);
    for (i = fd->nfps; i >= 0; i--) {
	FDSTACK_t * fps = &fd->fps[i];
	if (fps->io != bzdio)
	    continue;
	rc = fps->fp;
	break;
    }

    return rc;
}

/*@-globuse@*/
static /*@null@*/ FD_t bzdOpen(const char * path, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd;
    rpmbz bz = rpmbzOpen(path, fmode);

    if (bz == NULL)
	return NULL;
    fd = fdNew("open (bzdOpen)");
#ifdef	NOTYET		/* XXX persistent URI cache prevents fileno(bz->fp) */
    fdPop(fd); fdPush(fd, bzdio, bz, fileno(bz->fp));
#else
    fdPop(fd); fdPush(fd, bzdio, bz, -1);
#endif
    fdSetOpen(fd, path, -1, bz->omode);
    return fdLink(fd, "bzdOpen");
}
/*@=globuse@*/

/*@-globuse@*/
static /*@null@*/ FD_t bzdFdopen(void * cookie, const char * fmode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    int fdno = fdFileno(fd);
    rpmbz bz = rpmbzFdopen((void *)((long)fdno), fmode);

    if (bz == NULL)
	return NULL;
#ifdef	NOTYET		/* XXX persistent URI cache prevents pop */
    fdPop(fd); fdPush(fd, bzdio, bz, fileno(bz->fp));
#else
    fdSetFdno(fd, -1);		/* XXX skip the fdio close */
    fdPush(fd, bzdio, bz, fdno);		/* Push bzdio onto stack */
#endif
    return fdLink(fd, "bzdFdopen");
}
/*@=globuse@*/

/*@-globuse@*/
static int bzdFlush(void * cookie)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    FD_t fd = c2f(cookie);
    rpmbz bz = (rpmbz) bzdFileno(fd);
    return rpmbzFlush(bz);
}
/*@=globuse@*/

/*@-globuse@*/
/*@-mustmod@*/		/* LCL: *buf is modified */
static ssize_t bzdRead(void * cookie, /*@out@*/ char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies *buf, fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmbz bz = (rpmbz) bzdFileno(fd);
    ssize_t rc = 0;

assert(bz != NULL);
    if (fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */
    fdstat_enter(fd, FDSTAT_READ);
/*@-modobserver@*/ /* FIX: is errcookie an observer? */
    rc = rpmbzRead(bz, buf, count, (const char **)&fd->errcookie);
/*@=modobserver@*/
    if (rc >= 0) {
	fdstat_exit(fd, FDSTAT_READ, rc);
	if (fd->ndigests && rc > 0) fdUpdateDigests(fd, (const unsigned char *)buf, rc);
    }
    return rc;
}
/*@=mustmod@*/
/*@=globuse@*/

/*@-globuse@*/
static ssize_t bzdWrite(void * cookie, const char * buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmbz bz = (rpmbz) bzdFileno(fd);
    ssize_t rc;

assert(bz != NULL);
    if (fd->bytesRemain == 0) return 0;	/* XXX simulate EOF */

    if (fd->ndigests && count > 0) fdUpdateDigests(fd, (const unsigned char *)buf, count);

    fdstat_enter(fd, FDSTAT_WRITE);
/*@-modobserver@*/ /* FIX: is errcookie an observer? */
    rc = rpmbzWrite(bz, buf, count, (const char **)&fd->errcookie);
/*@=modobserver@*/
    if (rc >= 0)
	fdstat_exit(fd, FDSTAT_WRITE, rc);
    return rc;
}
/*@=globuse@*/

static int bzdSeek(void * cookie, _libio_pos_t pos, int whence)
	/*@*/
{
    FD_t fd = c2f(cookie);
    rpmbz bz = (rpmbz) bzdFileno(fd);

assert(bz != NULL);
    BZDONLY(fd);
    return rpmbzSeek(bz, pos, whence);
}

static int bzdClose( /*@only@*/ void * cookie)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t fd = c2f(cookie);
    rpmbz bz = (rpmbz) bzdFileno(fd);
    int rc;

assert(bz != NULL);
#ifdef	DYING
    if (bz->bzfile == NULL)	/* XXX memory leak w errors? */
	return -2;
#endif

    fdstat_enter(fd, FDSTAT_CLOSE);
/*@-modobserver@*/ /* FIX: is errcookie an observer? */
    rpmbzClose(bz, 0, (const char **)&fd->errcookie);
/*@=modobserver@*/
    rc = 0;	/* XXX FIXME */

    /* XXX TODO: preserve fd if errors */

    if (fd)
    if (rc >= 0)
	fdstat_exit(fd, FDSTAT_CLOSE, rc);

DBGIO(fd, (stderr, "==>\tbzdClose(%p) rc %lx %s\n", cookie, (unsigned long)rc, fdbg(fd)));

    if (_rpmio_debug || rpmIsDebug()) fdstat_print(fd, "BZDIO", stderr);

    if (rc == 0) {
/*@-dependenttrans@*/
	bz = rpmbzFree(bz, 0);
/*@=dependenttrans@*/
	fd = fdFree(fd, "open (bzdClose)");
    }
    return rc;
}

/*@-type@*/ /* LCL: function typedefs */
static struct FDIO_s bzdio_s = {
  bzdRead, bzdWrite, bzdSeek, bzdClose,	bzdOpen, bzdFdopen, bzdFlush,
};
/*@=type@*/

FDIO_t bzdio = /*@-compmempass@*/ &bzdio_s /*@=compmempass@*/ ;

#endif

