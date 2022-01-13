/*
 * This program, "pbzip2" is copyright (C) 2003-2009 Jeff Gilchrist.
 * All rights reserved.
 *
 * The library "libbzip2" which pbzip2 uses, is copyright
 * (C) 1996-2008 Julian R Seward.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 *
 * 3. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 *
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Jeff Gilchrist, Ottawa, Canada.
 * pbzip2@compression.ca
 * pbzip2 version 1.0.5 of January 8, 2009
 *
 */

// uncomment for debug output
#define PBZIP_DEBUG

#include "system.h"

#include <rpmiotypes.h>
#include "rpmio_internal.h"
#include <rpmlog.h>
#include "argv.h"
#include "rpmsq.h"

#include "poptIO.h"

#include "yarn.h"

#if defined(PBZIP_DEBUG) || defined(__LCLINT__)
#define	_RPMZLOG_INTERNAL
#include "rpmzlog.h"

/*@access rpmzMsg @*/
/*@access rpmzLog @*/

#define Trace(x) \
    do { \
	if (zq->verbosity > 2) { \
	    rpmzLogAdd x; \
	} \
    } while (0)

#else	/* !PBZIP_DEBUG */

#define rpmzLogDump(_zlog, _fp)
#define Trace(x)

#endif	/* PBZIP_DEBUG */

#define	_RPMBZ_INTERNAL
#include "rpmbz.h"

/*@access rpmbz @*/

#define	_RPMZQ_INTERNAL
#include "rpmzq.h"

/*@access rpmzSpace @*/
/*@access rpmzPool @*/
/*@access rpmzQueue @*/
/*@access rpmzJob @*/

#include "debug.h"

#define PATH_SEP	'/'

#define OFF_T		off_t

/* largest power of 2 that fits in an unsigned int -- used to limit requests
   to zlib functions that use unsigned int lengths */
#define _PIGZMAX ((((unsigned)0 - 1) >> 1) + 1)

/*@access FILE @*/

/*@-exportheadervar@*/
/*@unchecked@*/
const char * __progname;
/*@=exportheadervar@*/

/*@unchecked@*/
static int _debug;

/**
 */
typedef /*@abstract@*/ struct rpmz_s * rpmz;
/*@access rpmz @*/

typedef struct {
    OFF_T dataStart;
    OFF_T dataSize;
} bz2BlockListing;

/**
 */
struct rpmz_s {
/*@observer@*/
    const char *stdin_fn;	/*!< Display name for stdin. */
/*@observer@*/
    const char *stdout_fn;	/*!< Display name for stdout. */

/*@null@*/
    const char ** argv;		/*!< Files to process. */
/*@null@*/
    const char **  manifests;	/*!< Manifests to process. */
/*@null@*/
    const char * base_prefix;	/*!< Prefix for file manifests paths. */

#if !defined(PATH_MAX)		/* XXX grrr ... */
#define	PATH_MAX	4096
#endif
    char _ifn[PATH_MAX+1];	/*!< input file name (accommodate recursion) */

    /* PBZIP2 specific configuration. */
    struct timeval start;	/*!< starting time of day for tracing */

    struct stat isb;

    char _ofn[PATH_MAX+1];

/*@owned@*/ /*@relnull@*/
    rpmzQueue zq;		/*!< buffer pools. */

    int NumBlocks;

    OFF_T fileSize;

/*@observer@*/ /*@null@*/
    const char * name;		/*!< name for gzip header */
    time_t mtime;		/*!< time stamp for gzip header */

/* saved gzip/zip header data for decompression, testing, and listing */
    time_t stamp;		/*!< time stamp from gzip header */
/*@only@*/ /*@null@*/
    char * hname;		/*!< name from header (allocated) */

};

#define F_ISSET(_f, _FLAG) (((_f) & ((RPMZ_FLAGS_##_FLAG) & ~0x40000000)) != RPMZ_FLAGS_NONE)

/*@-fullinitblock@*/
/*@unchecked@*/
static struct rpmz_s __rpmz = {
    .stdin_fn	= "<stdin>",
    .stdout_fn	= "<stdout>",

};
/*@=fullinitblock@*/

/*@unchecked@*/
static rpmz _rpmz = &__rpmz;

#if !defined(O_BINARY)
#define O_BINARY 0
#endif
#  define SET_BINARY_MODE(_fdno)

/* exit with error, delete output file if in the middle of writing it */
/*@exits@*/
static void bail(const char *why, const char *what)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = z->zq;
    int xx;

/*@-globs@*/
    if (zq->_zout.fdno != -1 && zq->_zout.fn != NULL)
	xx = unlink(zq->_zout.fn);
/*@=globs@*/
    if (zq->verbosity > 0)
	fprintf(stderr, "%s abort: %s%s\n", __progname, why, what);
    exit(EXIT_FAILURE);
/*@notreached@*/
}

/*==============================================================*/

static void rpmzClose(rpmzQueue zq, int iclose, int oclose)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    int xx;

    if (iclose > 0) {
	if (zq->_zinp.fdno > STDERR_FILENO)
	    xx = close(zq->_zinp.fdno);
	zq->_zinp.fdno = -1;
    }
    if (oclose > 0) {
	if (zq->_zout.fdno > STDERR_FILENO)
	    xx = close(zq->_zout.fdno);
	zq->_zout.fdno = -1;
    }
}

static int rpmzOpen(rpmzQueue zq, const char * ofn)
	/*@globals internalState @*/
	/*@modifies zq, internalState @*/
{
    int rc = 0;
assert(ofn == zq->_zout.fn);
    if (!F_ISSET(zq->flags, STDOUT)) {
	static mode_t _mode = 0666;	/* XXX pbzip2 attempts 0644 */
	zq->_zout.fdno = open(zq->_zout.fn, O_CREAT | O_TRUNC | O_WRONLY |
			(F_ISSET(zq->flags, OVERWRITE) ? 0 : O_EXCL), _mode);
	if (zq->_zout.fdno < 0)
	    rc = -1;
    } else
	zq->_zout.fdno = STDOUT_FILENO;
    return rc;
}

/* read up to count bytes into buf, repeating read() calls as needed */
static ssize_t rpmzRead(rpmzQueue zq, unsigned char *buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies *buf, fileSystem, internalState @*/
{
    int fdno = zq->_zinp.fdno;
    size_t got = 0;

    while (count > 0) {
	ssize_t ret = read(fdno, buf, count);
	if (ret < 0)
	    bail("read error on ", zq->_zinp.fn);
	if (ret == 0)
	    break;
	buf += ret;
	count -= ret;
	got += ret;
    }
fprintf(stderr, "--> %s(%p,%p[%u]) rc %u\n", __FUNCTION__, zq, buf, (unsigned)count, (unsigned)got);
    return (ssize_t) got;	/* XXX never returns -1 */
}

/* write count bytes, repeating write() calls as needed */
static ssize_t rpmzWrite(rpmzQueue zq, const unsigned char *buf, size_t count)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    int fdno = zq->_zout.fdno;
    size_t put = 0;

    while (count) {
	ssize_t ret = write(fdno, buf, count);
	if (ret < 1) {
	    fprintf(stderr, "write error code %d\n", errno);
	    bail("write error on ", zq->_zout.fn);
	}
	buf += ret;
	count -= ret;
	put += ret;
    }
fprintf(stderr, "--> %s(%p,%p[%u]) rc %u\n", __FUNCTION__, zq, buf, (unsigned)count, (unsigned)put);
    return (ssize_t) put;	/* XXX never returns -1 */
}

/* ********************************************************* */
static void rpmzProgress(rpmzQueue zq, long cur, long tot)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    if (zq->verbosity > 0) {
	double pct = 100.0 * cur / tot;
	int xx;
	fprintf(stderr, "Completed: %3.1f%%             \r", pct);
	xx = fflush(stderr);
    }
}

/* collect the write jobs off of the list in sequence order and write out the
   compressed data until the last chunk is written -- also write the header and
   trailer and combine the individual check values of the input buffers */
static void write_thread(void * _z)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmz z = _z;
    rpmzQueue zq = z->zq;
    rpmzLog zlog = zq->zlog;

    long seq;			/* next sequence number looking for */
    int more;			/* true if more chunks to write */

fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, z);
    Trace((zlog, "-- write thread running"));

assert(zq->_zout.fdno == -1);
assert(zq->lastseq == z->NumBlocks);

    if (rpmzOpen(zq, zq->_zout.fn) < 0)	/* XXX open ofdno */
	bail("open error on ", zq->_zout.fn);

    seq = 0;
    do {
	rpmzJob job;
	ssize_t nwrote;

        /* get next write job in order */
	job = rpmzqDelWJob(zq, seq);
assert(job != NULL);

	/* XXX pigz write_thread */
	more = ((seq+1) < zq->lastseq);
#ifdef	NOTYET
assert(more == job->more);
#else
if (more != job->more)
fprintf(stderr, "==> FIXME: %s: job->more %d more %d\n", __FUNCTION__, job->more, more);
#endif

	if (job->in->use != NULL)
	    job->in = rpmzqDropSpace(job->in);
	else {
fprintf(stderr, "==> FIXME: %s: job->in %p %p[%u] free\n", __FUNCTION__, job->in, job->in->buf, (unsigned)job->in->len);
	    job->in = rpmzqDropSpace(job->in);
	}

        /* write the compressed data and drop the output buffer */
        Trace((zlog, "-- writing #%ld", seq));
	nwrote = rpmzWrite(zq, job->out->buf, job->out->len);
assert((size_t)nwrote == job->out->len);

	if (job->out->use != NULL) {
	    job->out = rpmzqDropSpace(job->out);
	} else {
fprintf(stderr, "==> FIXME: %s: job->out %p %p[%u] free\n", __FUNCTION__, job->out, job->out->buf, (unsigned)job->out->len);
	    job->out = rpmzqDropSpace(job->out);
	}

        Trace((zlog, "-- wrote #%ld%s", seq, more ? "" : " (last)"));

	rpmzProgress(zq, seq, zq->lastseq);

	job = rpmzqDropJob(job);

	seq++;

    } while (more);

    /* verify no more jobs, prepare for next use */
    rpmzqVerify(zq);

#ifdef	UNUSED
exit:
#endif
    rpmzClose(zq, -1, 1);	/* XXX close ofdno */
    return;
}

/*==============================================================*/

/* *********************************************************
    This function will search the array pointed to by
    b[] for the string s[] and return
    a pointer to the start of the s[] if found
    otherwise return NULL if not found.
*/
/*@null@*/ /*@observer@*/
static const unsigned char *memstr(const unsigned char *b, size_t blen,
		const unsigned char *s, size_t slen)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/
{
    int i;

fprintf(stderr, "--> %s(%p[%u], %p[%u])\n", __FUNCTION__, b, (unsigned)blen, s, (unsigned)slen);
    for (i = 0; i < (int)blen; i++) {
	if ((blen - i) < slen)
	    break;

	if (b[i] == s[0] && memcmp(b+i, s, slen) == 0 )
	    return &b[i];
    }

    return NULL;
}

static int rpmzParallelDecompressBlocks(rpmz z,
		bz2BlockListing ** _blocksp, size_t * _nblocksp)
	/*@globals fileSystem, internalState @*/
	/*@modifies *_blocksp, *_nblocksp, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;

    size_t _nblocks = 0;
    bz2BlockListing * _blocks = NULL;
    bz2BlockListing * blp;
    bz2BlockListing * nextblp;

    unsigned char bz2Header[] = "BZh91AY&SY";  // for 900k BWT block size
    OFF_T bytesLeft = 0;
    OFF_T inSize = 100000;
    OFF_T ret = 0;
    int i;
    unsigned char *b;
    const unsigned char * be;
    size_t blag;
    size_t blen;
    OFF_T boff = 0;
    OFF_T startByte = 0;
    int rc = -1;
    int xx;

fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, z);

    // set search header to value in file
    {	char _buf[16];
	xx = snprintf(_buf, sizeof(_buf), "%d", (zq->level % 10));
	bz2Header[3] = _buf[0];
    }

    // go to start of file
    ret = lseek(zq->_zinp.fdno, 0, SEEK_SET);
    if (ret != 0) {
	fprintf(stderr, "*ERROR: Could not seek to beginning of file [%llu]!  Skipping...\n", (unsigned long long)ret);
	goto exit;
    }

    // scan input file for BZIP2 block markers (BZh91AY&SY)
    // allocate memory to read in file

    // keep going until all the file is scanned for BZIP2 blocks
    boff = 0;
    blag = 0;
    be = NULL;
    blen = inSize;
    b = xmalloc(blen * sizeof(*b));
    bytesLeft = z->fileSize;
    while (bytesLeft > 0) {
	ssize_t nread;

	/* Overlap with blag bytes of previous buffer. */
	if (blag != 0 && blag < blen)
	    memmove(b, b+blen-blag, blag);

	// read file data minus overflow from previous buffer
	nread = rpmzRead(zq, (unsigned char *)b+blag, blen-blag);
	/* XXX 0b EOF read is prolly scwewy */

	if (nread < 0) {
	    fprintf(stderr, "*ERROR: Could not read from file!  Skipping...\n");
	    b = _free(b);
	    goto exit;
	}

	// scan buffer for bzip2 start header
	be = memstr(b, nread+blag, bz2Header, sizeof(bz2Header)-1);

	while (be != NULL) {
/*@-nullptrarith@*/
	    startByte = boff + be - b - blag;
/*@=nullptrarith@*/
#ifdef PBZIP_DEBUG
	    fprintf(stderr, " Found substring at: %p\n", be);
	    fprintf(stderr, " startByte = %llu\n", (unsigned long long)startByte);
	    fprintf(stderr, " bz2NumBlocks = %u\n", (unsigned)_nblocks);
#endif

	    // add data to end of block list
	    _blocks = xrealloc(_blocks, (_nblocks+1) * sizeof(*_blocks));
	    _blocks[_nblocks].dataStart = startByte;
	    _blocks[_nblocks].dataSize = 0;
	    _nblocks++;

	    be++;
	    be = memstr(be, nread+blag-(be-b), bz2Header, sizeof(bz2Header)-1);
	}

	boff += nread;
	bytesLeft -= nread;
	blag = sizeof(bz2Header) - 1 - 1;
    }

    // use direcdecompress() instead to process 1 bzip2 stream
    if (_nblocks <= 1) {
	if (zq->verbosity > 0)
	    fprintf(stderr, "Switching to no threads mode: only 1 BZIP2 block found.\n");
	b = _free(b);
	goto exit;
    }

    b = _free(b);

    // calculate data sizes for each block
    if (_blocks != NULL)	/* XXX can't happen */
    for (i = 0, blp = _blocks; i < (int)_nblocks; i++, blp++) {
	nextblp = blp + 1;
	if (i == (int)_nblocks-1)
	    // special case for last block
	    blp->dataSize = z->fileSize - blp->dataStart;
	else if (i == 0)
	    // special case for first block
	    blp->dataSize = nextblp->dataStart;
	else
	    // normal case
	    blp->dataSize = nextblp->dataStart - blp->dataStart;
#ifdef PBZIP_DEBUG
	fprintf(stderr, " bz2BlockList[%d].dataStart = %llu\n", i, (unsigned long long)blp->dataStart);
	fprintf(stderr, " bz2BlockList[%d].dataSize = %llu\n", i, (unsigned long long)blp->dataSize);
#endif
    }

    rc = 0;

exit:
    if (_nblocksp != NULL)
	*_nblocksp = (rc == 0 ? _nblocks : 0);
    if (rc == 0 && _blocksp != NULL)
	*_blocksp = _blocks;
    else {
	_blocks = _free(_blocks);
	if (_blocksp != NULL)
	    *_blocksp = NULL;
    }
/*@-nullstate@*/
    return rc;
/*@=nullstate@*/
}

/* *********************************************************
    This function works in two passes of the input file.
    The first pass will look for BZIP2 headers in the file
    and note their location and size of the sections.
    The second pass will read in those BZIP2 sections and
    pass them off the the selected CPU(s) for decompression.
 */
static int rpmzParallelDecompress(rpmz z)
	/*@globals fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;
    rpmzLog zlog = zq->zlog;

    long seq;			/* sequence number */
    rpmzSpace next;		/* next input space */
    rpmzJob job;		/* job for compress, then write */

    size_t _nblocks = 0;
    bz2BlockListing * _blocks = NULL;
    bz2BlockListing * blp;
    OFF_T ret = 0;
    int rc = -1;
    int i;

fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, z);

    /* Find BZIP2 block offsets. */
/*@-nullstate@*/
    rc = rpmzParallelDecompressBlocks(z, &_blocks, &_nblocks);
    if (rc != 0)
	goto exit;
/*@=nullstate@*/

    z->NumBlocks = _nblocks;

    rpmzqFini(zq);		/* XXX lose any existing values. */
    /* XXX needs to be done one-time in rpmzProcess, not here. */
/*@-assignexpose@*/
    zq->_zout.fn = xstrdup(z->_ofn);
/*@=assignexpose@*/
    zq->lastseq = z->NumBlocks;
    zq->omode = O_RDONLY;
#ifdef	REFERENCE	/* XXX this is what compression uses. */
    zq->iblocksize = zq->blocksize;
    zq->ilimit = z->NumBlocks;
    zq->oblocksize = (size_t) ((zq->iblocksize*1.01)+600);
    zq->olimit = z->NumBlocks/2 + 1;
#else
    zq->ilimit = z->NumBlocks/2;
    zq->oblocksize = 6 * zq->iblocksize;
    zq->olimit = z->NumBlocks;
#endif
    rpmzqInit(zq);

    /* start write thread */
/*@-mustfreeonly@*/
    zq->_zw.thread = yarnLaunch(write_thread, z);
/*@=mustfreeonly@*/

    // keep going until all the blocks are processed
    seq = 0;
    if (_blocks != NULL)	/* XXX can't happen */
    for (i = 0, blp = _blocks; i < (int)_nblocks; i++, blp++) {
	size_t nread;

	// go to start of block position in file
/*@-compdef@*/
	ret = lseek(zq->_zinp.fdno, blp->dataStart, SEEK_SET);
/*@=compdef@*/
	if (ret != blp->dataStart) {
	    fprintf(stderr, "*ERROR: Could not seek to beginning of file [%llu]!  Skipping...\n", (unsigned long long)ret);
	    goto exit;
	}

	next = rpmzqNewSpace(zq->_zc.pool, zq->_zc.pool->size);
	if (next->len >= (size_t)blp->dataSize) {
	    nread = rpmzRead(zq, next->buf, next->len);
assert(nread != 0);
	    next->len = nread;
	} else {
fprintf(stderr, "==> FIXME: %s: next->len(%u) < dataSize(%u)\n", __FUNCTION__, (unsigned)next->len, (unsigned)blp->dataSize);
	    next = rpmzqDropSpace(next);
	    next = rpmzqNewSpace(NULL, blp->dataSize);
	    nread = rpmzRead(zq, next->buf, next->len);
assert(nread == next->len);
	    next->len = nread;
	}

	job = rpmzqNewJob(seq);
/*@-mustfreeonly@*/
	job->in = next;
	job->out = NULL;
/*@=mustfreeonly@*/
	job->more = ((i+1) < (int)_nblocks);

	Trace((zlog, "-- read #%ld%s", seq, job->more ? "" : " (last)"));
	if (++seq < 1)
	    bail("input too long: ", "");	/* XXX z->_ifn */

assert(zq->omode == O_RDONLY);
	/* start another decompress thread if needed */
	rpmzqLaunch(zq, seq, zq->threads);

	/* put job at end of compress list, let all the compressors know */
	rpmzqAddCJob(zq, job);

    }

    /* wait for the write thread to complete (we leave the decompress threads out
       there and waiting in case there is another stream to decompress) */
    zq->_zw.thread = yarnJoin(zq->_zw.thread);
    Trace((zlog, "-- write thread joined"));

    rc = 0;

exit:
    _blocks = _free(_blocks);
    if (rc != -99)	/* XXX special hacks */
	rpmzClose(zq, 1, -1);	/* XXX close ifdno */

    return rc;
}

static int rpmzParallelCompress(rpmz z)
	/*@globals fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;
    rpmzLog zlog = zq->zlog;

    long seq;			/* sequence number */
#ifdef	NOTYET
    rpmzSpace prev;		/* previous input space */
#endif
    rpmzSpace next;		/* next input space */
    rpmzJob job;		/* job for compress, then write */
    int more;			/* true if more input to read */
    int rc = -1;

fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, z);

assert(zq->omode == O_WRONLY);

    /* start write thread */
/*@-mustfreeonly@*/
    zq->_zw.thread = yarnLaunch(write_thread, z);
/*@=mustfreeonly@*/

    /* read from input and start compress threads (write thread will pick up
       the output of the compress threads) */
    seq = 0;
    next = rpmzqNewSpace(zq->_zc.pool, zq->_zc.pool->size);
    next->len = rpmzRead(zq, next->buf, next->pool->size);
    do {
	/* create a new job, use next input chunk, previous as dictionary */
	job = rpmzqNewJob(seq);
/*@-mustfreeonly@*/
	job->in = next;
#ifdef	NOTYET
	job->out = F_ISSET(zq->flags, INDEPENDENT) ? prev : NULL;  /* dictionary for compression */
#else
	job->out = NULL;
#endif
/*@=mustfreeonly@*/

	/* check for end of file, reading next chunk if not sure */
	if (next->len < next->pool->size)
	    more = 0;
	else {
	    next = rpmzqNewSpace(zq->_zc.pool, zq->_zc.pool->size);
	    next->len = rpmzRead(zq, next->buf, next->pool->size);
	    more = next->len != 0;
	    if (!more)
		next = rpmzqDropSpace(next);  /* won't be using it */
#ifdef	NOTYET
	    if (F_ISSET(zq->flags, INDEPENDENT) && more) {
		rpmzqUseSpace(job->in);    /* hold as dictionary for next loop */
		prev = job->in;
	    }
#endif
	}
	job->more = more;
	Trace((zlog, "-- read #%ld%s", seq, more ? "" : " (last)"));
	if (++seq < 1)
	    bail("input too long: ", "");	/* XXX z->_ifn */

	/* start another compress thread if needed */
	rpmzqLaunch(zq, seq, zq->threads);

	/* put job at end of compress list, let all the compressors know */
	rpmzqAddCJob(zq, job);

    } while (more);

    /* wait for the write thread to complete (we leave the compress threads out
       there and waiting in case there is another stream to compress) */
    zq->_zw.thread = yarnJoin(zq->_zw.thread);
    Trace((zlog, "-- write thread joined"));

    rc = 0;

    rpmzClose(zq, 1, -1);	/* XXX close ifdno */

    return rc;
}

/* ********************************************************* */
/* do a simple compression in a single thread from zq->ifdno to zq->ofdno -- if reset is
   true, instead free the memory that was allocated and retained for input,
   output, and deflate */
static int rpmzSingleCompress(rpmz z, /*@unused@*/ int reset)
	/*@globals fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;

    long seq = 0;
    rpmzJob job;
    rpmbz bz;

    OFF_T bytesLeft = 0;
    size_t nread = 0;
    size_t nwrote = 0;
    int ret = 0;
    int rc = -1;

fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, z);

    job = rpmzqNewJob(seq);
    job->in = rpmzqNewSpace(NULL, 0);
    job->out = rpmzqNewSpace(NULL, 0);

assert(zq->_zout.fdno == -1);
assert(zq->iblocksize == zq->blocksize);
    zq->lastseq = z->NumBlocks;
    zq->omode = O_WRONLY;

    bz = rpmbzInit(zq->level, -1, -1, O_WRONLY);

    if (rpmzOpen(zq, zq->_zout.fn) < 0)	/* XXX open ofdno */
	bail("open error on ", zq->_zout.fn);

    bytesLeft = z->fileSize;

    // keep going until all the file is processed
    while (bytesLeft > 0) {

	job->in->len = (bytesLeft > (OFF_T) zq->iblocksize)
		? zq->iblocksize : bytesLeft;
	job->in->ptr = job->in->buf = xmalloc(job->in->len);

	nread = rpmzRead(zq, job->in->buf, job->in->len);

	if (nread == 0) {
	    job->in->ptr = _free(job->in->ptr);
	    job->in->len = 0;
	    break;
	}

	bytesLeft -= nread;

	job->out->len = (size_t) ((job->in->len*1.01)+600);
	job->in->ptr = job->out->buf = xmalloc(job->out->len);

	ret = rpmbzCompressBlock(bz, job);
	if (ret != BZ_OK) {
	    fprintf(stderr, "*ERROR during compression: %d\n", ret);
	    goto exit;
	}

	nwrote = rpmzWrite(zq, job->out->buf, job->out->len);

	if (nwrote == 0 || nwrote != job->out->len) {
	    fprintf(stderr, "*ERROR: Could not write to file!  Skipping...\n");
	    goto exit;
	}

	seq++;

	// print current completion status
	rpmzProgress(zq, seq, zq->lastseq);

	// clean up memory
	job->in->buf = _free(job->in->buf);
	job->in->len = 0;
	job->out->buf = _free(job->out->buf);
	job->out->len = 0;

    }

    rc = 0;

exit:
    job->out = rpmzqDropSpace(job->out);
    job->in = rpmzqDropSpace(job->in);
    job = rpmzqDropJob(job);

    bz = rpmbzFini(bz);

    rpmzClose(zq, 1, 1);	/* XXX close ifdno/ofdno */

    return rc;
}

/* ********************************************************* */
static int rpmzSingleDecompress(rpmz z, const char *ifn, const char *ofn)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;

    FD_t ofd = NULL;
    FD_t ifd = NULL;
    char b[BZ_MAX_UNUSED];
    size_t blen = sizeof(b);
    size_t nread;
    size_t ntotal;
    int rc = -1;
    int xx;

fprintf(stderr, "--> %s(%s,%s)\n", __FUNCTION__, ifn, ofn);

assert(zq->omode == O_RDONLY);

    zq->lastseq = z->NumBlocks;
    zq->omode = O_RDONLY;

    ifd = Fopen(ifn, "rb.bzdio");

    if (ifd == NULL || Ferror(ifd)) {
	fprintf(stderr, "*ERROR: Problem with input stream of file [%s]!  Skipping...\n", ifn);
	goto exit;
    }

    ofd = Fopen((!F_ISSET(zq->flags, STDOUT) ? ofn : "-"), "wb");

    if (ofd == NULL || Ferror(ofd)) {
	fprintf(stderr, "*ERROR: Problem with output stream of file [%s]!  Skipping...\n", ifn);
	goto exit;
    }

    ntotal = 0;
    do {
	for (;;) {
	    if ((nread = Fread(b, 1, blen, ifd)) == 0)
		/*@innerbreak@*/ break;
	    if (Fwrite(b, 1, nread, ofd) != nread)
		/*@innerbreak@*/ break;
	    ntotal += nread;
	}
    } while (0);

exit:
    if (ifd != NULL) {
	xx = Fclose(ifd);
	ifd = NULL;
    }
    if (ofd != NULL) {
	xx = Fclose(ofd);
	ofd = NULL;
    }
    return rc;
}

static int testCompressedData(rpmz z, const char *ifn)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    FD_t ifd = NULL;
    unsigned char unused[20 * BZ_MAX_UNUSED];
    size_t nread;
    size_t ntotal;
    int rc = -1;
    int xx;

fprintf(stderr, "--> %s(%p,%s)\n", __FUNCTION__, z, ifn);
    ifd = Fopen(ifn, "rb.bzdio");

    if (ifd == NULL || Ferror(ifd)) {
	fprintf(stderr, "*ERROR: Problem with stream of file [%s]!  Skipping...\n", ifn);
	goto exit;
    }

    ntotal = 0;
    do {
	nread = Fread(unused, 1, sizeof(unused), ifd);
	ntotal += nread;
    } while (nread == sizeof(unused) && !Ferror(ifd));

fprintf(stderr, "*** %s: I/O total: %u:%u\n", __FUNCTION__, (unsigned)ntotal, (unsigned)z->fileSize);

    if (Ferror(ifd)) {
	fprintf(stderr, "*ERROR: Problem with stream of file [%s]!  Skipping...\n", ifn);
	goto exit;
    }

    rc = 0;

exit:
    if (ifd != NULL) {
	xx = Fclose(ifd);
	ifd = NULL;
    }
    return rc;
}

/* ********************************************************* */
static int getFileMetaData(rpmz z, const char *fn)
	/*@globals fileSystem @*/
	/*@modifies z, fileSystem @*/
{
fprintf(stderr, "--> %s(%p,%s)\n", __FUNCTION__, z, fn);
    // get the file meta data and store it in the global structure
    return stat(fn, &z->isb);
}

/* ********************************************************* */
static int writeFileMetaData(rpmz z, const char *fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    struct utimbuf uTimBuf;
    int ret = 0;
    int xx;

fprintf(stderr, "--> %s(%p,%s)\n", __FUNCTION__, z, fn);
    // store file times in structure
/*@-type@*/
    uTimBuf.actime = z->isb.st_atime;
    uTimBuf.modtime = z->isb.st_mtime;
/*@=type@*/

    // update file with stored file permissions
    ret = chmod(fn, z->isb.st_mode);
    if (ret != 0)
	return ret;

    // update file with stored file access and modification times
    ret = utime(fn, &uTimBuf);
    if (ret != 0)
	return ret;

    // update file with stored file ownership (if access allows)
#ifndef WIN32
    xx = chown(fn, z->isb.st_uid, z->isb.st_gid);
#endif

    return 0;
}

/* ********************************************************* */
static int rpmzProcess(rpmz z, const char * ifn, int fileLoop)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq;

    const unsigned char bz2Header[] = "BZh91AY&SY";  // using 900k block size
/*@+charint@*/
    unsigned char bz2HeaderZero[] = { 0x42, 0x5A, 0x68, 0x39, 0x17, 0x72, 0x45, 0x38, 0x50, 0x90, 0x00, 0x00, 0x00, 0x00 };
/*@=charint@*/
    unsigned char tmpBuff[50];
    size_t size;
    int zeroByteFile = 0;
    int numBlocks = 0;
    int ret;
    int rc = 0;
    int xx;

fprintf(stderr, "--> %s(%p,%s)\n", __FUNCTION__, z, ifn);

    zq = z->zq;
assert(zq != NULL);

    z->fileSize = 0;

    // test file for errors if requested
    if (zq->mode == RPMZ_MODE_TEST) {
	if (zq->verbosity > 0) {
	    fprintf(stderr, "      File #: %d\n", fileLoop+1);
	    if (strcmp(ifn, "-") != 0)
		fprintf(stderr, "     Testing: %s\n", ifn);
	    else
		fprintf(stderr, "     Testing: %s\n", z->stdin_fn);
	}
	ret = testCompressedData(z, ifn);
	if (ret > 0) {
	    rc = ret;
	    goto exit;
	} else if (ret == 0) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "        Test: OK\n");
	    rc = 0;
	} else
	    rc = 2;

	if (zq->verbosity > 0)
	    fprintf(stderr, "-------------------------------------------\n");
	goto exit;
    }

    // set ouput filename
/*@-mayaliasunique@*/
    strncpy(z->_ofn, ifn, 2040);
/*@=mayaliasunique@*/
    if (zq->mode == RPMZ_MODE_DECOMPRESS && (strcmp(ifn, "-") != 0)) {
	size_t nb;

	// check if input file is a valid .bz2 compressed file
	zq->_zinp.fdno = open(ifn, O_RDONLY | O_BINARY);
	// check to see if file exists before processing
	if (zq->_zinp.fdno == -1) {
	    fprintf(stderr, "*ERROR: File [%s] NOT found!  Skipping...\n", ifn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}
	memset(tmpBuff, 0, sizeof(tmpBuff));
	size = rpmzRead(zq, tmpBuff, sizeof(bz2Header));
	rpmzClose(zq, 1, -1);	/* XXX close ifdno */

	if ((size == (size_t)(-1)) || (size < sizeof(bz2Header))) {
	    fprintf(stderr, "*ERROR: File [%s] is NOT a valid bzip2!  Skipping...\n", ifn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	} else {
	    // make sure start of file has valid bzip2 header
	    if (memstr(tmpBuff, 4, bz2Header, 3) == NULL) {
		fprintf(stderr, "*ERROR: File [%s] is NOT a valid bzip2!  Skipping...\n", ifn);
		fprintf(stderr, "-------------------------------------------\n");
		rc = 1;
		goto exit;
	    }
	    // skip 4th char which differs depending on BWT block size used
	    if (memstr(tmpBuff+4, size-4, bz2Header+4, sizeof(bz2Header)-1-4) == NULL) {
		// check to see if this is a special 0 byte file
		if (memstr(tmpBuff+4, size-4, bz2HeaderZero+4, sizeof(bz2Header)-1-4) == NULL) {
		    fprintf(stderr, "*ERROR: File [%s] is NOT a valid bzip2!  Skipping...\n", ifn);
		    fprintf(stderr, "-------------------------------------------\n");
		    rc = 1;
		    goto exit;
		}
#ifdef PBZIP_DEBUG
		fprintf(stderr, "** ZERO byte compressed file detected\n");
#endif
	    }
	    // set block size for decompression
	    if ((tmpBuff[3] >= '1') && (tmpBuff[3] <= '9'))
		zq->level = (tmpBuff[3] - '0');
	    else {
		fprintf(stderr, "*ERROR: File [%s] is NOT a valid bzip2!  Skipping...\n", ifn);
		fprintf(stderr, "-------------------------------------------\n");
		rc = 1;
		goto exit;
	    }
	}

	// check if filename ends with .bz2
	nb = strlen(z->_ofn);
	if (nb >= 4 && strncasecmp(&z->_ofn[nb-4], ".bz2", 4) == 0) {
	    // remove .bz2 extension
	    z->_ofn[nb-4] = '\0';
	} else {
	    // add .out extension so we don't overwrite original file
	    strcat(z->_ofn, ".out");
	}
    } else {	// decompress == 1
	size_t nb = strlen(ifn);
	// check input file to make sure its not already a .bz2 file
	if (nb >= 4 && strncasecmp(&ifn[nb-4], ".bz2", 4) == 0) {
	    fprintf(stderr, "*ERROR: Input file [%s] already has a .bz2 extension!  Skipping...\n", ifn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}
	strcat(z->_ofn, ".bz2");
    }

    // setup signal handling filenames
/*@-assignexpose@*/
    zq->_zinp.fn = ifn;
    zq->_zout.fn = xstrdup(z->_ofn);
/*@=assignexpose@*/

    if (strcmp(ifn, "-") != 0) {
	struct stat sb;
	// read file for compression
	zq->_zinp.fdno = open(zq->_zinp.fn, O_RDONLY | O_BINARY);
	// check to see if file exists before processing
	if (zq->_zinp.fdno == -1) {
	    fprintf(stderr, "*ERROR: File [%s] NOT found!  Skipping...\n", zq->_zinp.fn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}

	// get some information about the file
	xx = fstat(zq->_zinp.fdno, &sb);
	// check to make input is not a directory
	if (S_ISDIR(sb.st_mode)) {
	    fprintf(stderr, "*ERROR: File [%s] is a directory!  Skipping...\n", zq->_zinp.fn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}
	// check to make sure input is a regular file
	if (!S_ISREG(sb.st_mode)) {
	    fprintf(stderr, "*ERROR: File [%s] is not a regular file!  Skipping...\n", zq->_zinp.fn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}
	// get size of file
	z->fileSize = (OFF_T) sb.st_size;

	// don't process a 0 byte file
	if (z->fileSize == 0) {
	    if (zq->mode == RPMZ_MODE_DECOMPRESS) {
		fprintf(stderr, "*ERROR: File is of size 0 [%s]!  Skipping...\n", ifn);
		fprintf(stderr, "-------------------------------------------\n");
		rc = 1;
		goto exit;
	    }

	    // make sure we handle zero byte files specially
	    zeroByteFile = 1;
	} else
	    zeroByteFile = 0;

	// get file meta data to write to output file
	if (getFileMetaData(z, zq->_zinp.fn) != 0) {
	    fprintf(stderr, "*ERROR: Could not get file meta data from [%s]!  Skipping...\n", ifn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rc = 1;
	    goto exit;
	}
    } else {
	zq->_zinp.fdno = STDIN_FILENO;
	z->fileSize = -1;	// fake it
    }

    // check to see if output file exists
    if (!F_ISSET(zq->flags, OVERWRITE) && !F_ISSET(zq->flags, STDOUT)) {
	zq->_zout.fdno = open(zq->_zout.fn, O_RDONLY | O_BINARY);
	// check to see if file exists before processing
	if (zq->_zout.fdno != -1) {
	    fprintf(stderr, "*ERROR: Output file [%s] already exists!  Use -f to overwrite...\n", zq->_zout.fn);
	    fprintf(stderr, "-------------------------------------------\n");
	    rpmzClose(zq, -1, 1);	/* XXX close ofdno */
	    rc = 1;
	    goto exit;
	}
    }

    // display per file settings
    if (zq->verbosity > 0) {
	fprintf(stderr, "         File #: %d\n", fileLoop+1);
	fprintf(stderr, "     Input Name: %s\n", zq->_zinp.fdno != STDIN_FILENO ? zq->_zinp.fn : "<stdin>");

	if (!F_ISSET(zq->flags, STDOUT))
	    fprintf(stderr, "    Output Name: %s\n\n", zq->_zout.fn);
	else
	    fprintf(stderr, "    Output Name: %s\n\n", z->stdout_fn);

	if (zq->mode == RPMZ_MODE_DECOMPRESS)
	    fprintf(stderr, " BWT Block Size: %d00k\n", (zq->level % 10));
	if (strcmp(zq->_zinp.fn, "-") != 0)
	    fprintf(stderr, "     Input Size: %llu bytes\n", (unsigned long long)z->fileSize);
    }

    switch (zq->mode) {
    case RPMZ_MODE_DECOMPRESS:
	numBlocks = 0;
	break;
    case RPMZ_MODE_COMPRESS:
    default:
	if (z->fileSize > 0) {
	    // calculate the # of blocks of data
	    numBlocks = (z->fileSize + zq->blocksize - 1) / zq->blocksize;
	}

	// write special compressed data for special 0 byte input file case
	if (zeroByteFile == 1) {
	    size_t nwrote;

	    if (rpmzOpen(zq, zq->_zout.fn) < 0) {	/* XXX open ofdno */
		fprintf(stderr, "*ERROR: Could not create output file [%s]!\n", zq->_zout.fn);
		rc = 1;
		goto exit;
	    }

	    // write data to the output file
	    nwrote = rpmzWrite(zq, bz2HeaderZero, sizeof(bz2HeaderZero));
	    rpmzClose(zq, -1, 1);	/* XXX close ofdno */

	    if (nwrote == 0 || nwrote != sizeof(bz2HeaderZero)) {
		fprintf(stderr, "*ERROR: Could not write to file!  Skipping...\n");
		fprintf(stderr, "-------------------------------------------\n");
		rc = 1;
		goto exit;
	    }
	    if (zq->verbosity > 0) {
		fprintf(stderr, "    Output Size: %llu bytes\n", (unsigned long long)sizeof(bz2HeaderZero));
		fprintf(stderr, "-------------------------------------------\n");
	    }
	    rc = 0;
	    goto exit;
	}
	break;
    }

    z->NumBlocks = numBlocks;

    switch (zq->mode) {
    case RPMZ_MODE_DECOMPRESS:
	// use multi-threaded code
	if (zq->threads > 0) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "Decompressing data...\n");

	    // start reading in data for decompression
	    ret = rpmzParallelDecompress(z);
	    if (ret == -99) {
		// only 1 block detected, use single threaded code to decompress
		zq->threads = 0;
	    } else if (ret != 0)
		rc = 1;
	    else
		rc = 0;
	}

	// use single threaded code
	if (zq->threads < 1) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "Decompressing data (no threads)...\n");

	    rpmzClose(zq, 1, -1);	/* XXX close ifdno */
	    ret = rpmzSingleDecompress(z, zq->_zinp.fn, zq->_zout.fn);
	    if (ret != 0)
		rc = 1;
	    else
		rc = 0;
	}
	break;
    case RPMZ_MODE_COMPRESS:
    default:

	rpmzqFini(zq);		/* XXX lose any existing values. */
	/* XXX needs to be done one-time in rpmzProcess, not here. */
	zq->lastseq = z->NumBlocks;
	zq->omode = O_WRONLY;
	zq->ilimit = z->NumBlocks;
	zq->oblocksize = (size_t) ((zq->iblocksize*1.01)+600);
	zq->olimit = z->NumBlocks/2 + 1;
	rpmzqInit(zq);

	// use multi-threaded code
	if (zq->threads > 0) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "Compressing data...\n");

#if defined(PIGZ_FLIP)
	    z->threads = 20;
	    _rpmzParallelCompress(z);
	    rc = 0;
#else
	    ret = rpmzParallelCompress(z);
	    if (ret != 0)
		rc = 1;
	    else
		rc = 0;
#endif
	} else {
	    // do not use threads for compression
	    if (zq->verbosity > 0)
		fprintf(stderr, "Compressing data (no threads)...\n");

	    ret = rpmzSingleCompress(z, 0);	/* XXX reset? */
	    if (ret != 0)
		rc = 1;
	    else
		rc = 0;
	}
	break;
    }

    rpmzqFini(zq);		/* XXX no need to do this every time. */

    if (!F_ISSET(zq->flags, STDOUT)) {
	// write store file meta data to output file
	if (writeFileMetaData(z, zq->_zout.fn) != 0)
	    fprintf(stderr, "*ERROR: Could not write file meta data to [%s]!\n", ifn);
    }

    // finished processing file
    zq->_zinp.fn = NULL;

    // remove input file unless requested not to by user
    if (!F_ISSET(zq->flags, KEEP)) {
	struct stat sb;
	if (!F_ISSET(zq->flags, STDOUT)) {
	    // only remove input file if output file exists
	    if (stat(zq->_zout.fn, &sb) == 0)
		xx = unlink(zq->_zinp.fn);
	} else
	    xx = unlink(zq->_zinp.fn);
    }

    if (zq->verbosity > 0)
	fprintf(stderr, "-------------------------------------------\n");

exit:
    zq->_zout.fn = _free(zq->_zout.fn);
    return rc;
}

/*==============================================================*/

/*@unchecked@*/
static int signals_init_count;

static void signals_init(void)
	/*@globals signals_init_count, fileSystem, internalState @*/
	/*@modifies signals_init_count, fileSystem, internalState @*/
{
    if (signals_init_count++ == 0) {
	int xx;
	xx = rpmsqEnable(SIGHUP,      NULL);
	xx = rpmsqEnable(SIGINT,      NULL);
	xx = rpmsqEnable(SIGTERM,     NULL);
	xx = rpmsqEnable(SIGQUIT,     NULL);
	xx = rpmsqEnable(SIGPIPE,     NULL);
	xx = rpmsqEnable(SIGXCPU,     NULL);
	xx = rpmsqEnable(SIGXFSZ,     NULL);
    }
}

/*@-mustmod@*/
static void signals_fini(void)
	/*@globals signals_init_count, fileSystem, internalState @*/
	/*@modifies signals_init_count, fileSystem, internalState @*/
{
    if (--signals_init_count == 0) {
	int xx;
	xx = rpmsqEnable(-SIGHUP,     NULL);
	xx = rpmsqEnable(-SIGINT,     NULL);
	xx = rpmsqEnable(-SIGTERM,    NULL);
	xx = rpmsqEnable(-SIGQUIT,    NULL);
	xx = rpmsqEnable(-SIGPIPE,    NULL);
	xx = rpmsqEnable(-SIGXCPU,    NULL);
	xx = rpmsqEnable(-SIGXFSZ,    NULL);
    }
}
/*@=mustmod@*/

static int signals_terminating(int terminate)
	/*@globals internalState @*/
	/*@modifies internalState @*/
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

/*@-mustmod@*/
static void signals_exit(void)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
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
/*@=mustmod@*/

/* ********************************************************* */
static void hw_init(/*@unused@*/ rpmz z)
	/*@*/
{
    int ncpu = 1;

    // Autodetect the number of CPUs on a box, if available
#if defined(__APPLE__)
    size_t len = sizeof(ncpu);
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &ncpu, &len, 0, 0) < 0 || len != sizeof(ncpu))
	ncpu = 1;
#elif defined(_SC_NPROCESSORS_ONLN)
    ncpu = sysconf((int)_SC_NPROCESSORS_ONLN);
#endif

    // Ensure we have at least one processor to use
    if (ncpu < 1)
	ncpu = 1;

#ifdef	DYING
    z->numCPU = ncpu;
#endif

    return;
}

/*==============================================================*/

#ifdef	NOTYET
/* either new buffer size, new compression level, or new number of processes --
   get rid of old buffers and threads to force the creation of new ones with
   the new settings */
static void rpmzNewOpts(rpmz z)
	/*@globals fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzSingleCompress(z, 1);
#ifndef NOTHREAD
    rpmzqFini(z);
#endif
}

/* catch termination signal */
/*@exits@*/
static void rpmzAbort(/*@unused@*/ int sig)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = z->zq;

    Trace((z->zlog, "termination by user"));
    if (zq->_zout.fdno != -1 && zq->_zout.fn != NULL)
	Unlink(z->_zout.fn);
    zq->zlog = rpmzLogDump(zq->zlog, NULL);
    _exit(EXIT_FAILURE);
}
#endif

/**
 */
static void rpmzqArgCallback(poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt, /*@unused@*/ const char * arg,
		/*@unused@*/ void * data)
	/*@globals _rpmz, fileSystem, internalState @*/
	/*@modifies _rpmz, fileSystem, internalState @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = z->zq;

#define	PBZIP2VERSION	"1.0.5"
    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'L':
	fprintf(stderr, "Parallel BZIP2 v%s - by: Jeff Gilchrist [http://compression.ca]\n", PBZIP2VERSION);
	fprintf(stderr, "[Jan. 08, 2009]             (uses libbzip2 by Julian Seward)\n");
	exit(EXIT_SUCCESS);
	/*@notreached@*/ break;
    case 'V':	(void)fputs(PBZIP2VERSION, stderr); exit(EXIT_SUCCESS);
    case 'b':
	zq->blocksize = (size_t)(atol(arg)) * 100000;   /* chunk size */
	if (zq->blocksize < 100000)
	    bail("block size too small (must be > 0)", "");
	if (zq->blocksize > 1000000000)
	    bail("block size too large", "");
	break;
    case 'p':
	zq->threads = atoi(arg);                  /* # processes */
	if (zq->threads < 1)
	    bail("need at least one process", "");
	if ((2 + (zq->threads << 1)) < 1)
	    bail("too many processes", "");
	break;
    case 'q':	zq->verbosity = 0; break;
    case 'v':	zq->verbosity++; break;
    default:
	/* XXX really need to display longName/shortName instead. */
	fprintf(stderr, _("Unknown option -%c\n"), (char)opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/


/* XXX grrr, popt needs better way to insert text strings in --help. */
/* XXX fixme: popt does post order recursion into sub-tables. */
/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmzPrivatePoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	rpmzqArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "debug", 'D', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN, &_debug, -1,
	N_("debug spewage"), NULL },

#ifdef	NOTYET
	/* XXX POPT_ARG_ARGV portability. */
  { "files", '\0', POPT_ARG_ARGV,		&__rpmz.manifests, 0,
	N_("Read file names from MANIFEST"), N_("MANIFEST") },
#endif
  { "quiet", 'q',	POPT_ARG_VAL,				NULL,  'q',
	N_("Print no messages, even on error"), NULL },
  { "verbose", 'v',	POPT_ARG_VAL,				NULL,  'v',
	N_("Provide more verbose output"), NULL },
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
	rpmzqArgCallback, 0, NULL, NULL },
/*@=type@*/

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzPrivatePoptTable, 0,
        N_("\
  rpmpbzip2 will compress files in place, adding the suffix '.bz2'. If no\n\
  files are specified, stdin will be compressed to stdout. rpmpbzip2 does\n\
  what bzip2 does, but spreads the work over multiple processors and cores\n\
  when compressing.\n\
\n\
Options:\
"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzqOptionsPoptTable, 0,
        N_("Compression options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

#ifdef	DYING
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Parallel BZIP2 v1.0.5 - by: Jeff Gilchrist [http://compression.ca]\n\
[Jan. 08, 2009]             (uses libbzip2 by Julian Seward)\n\
\n\
Usage: ./pbzip2 [-1 .. -9] [-b#cdfhklp#qrtVz] <filename> <filename2> <filenameN>\n\
 -b#      : where # is the file block size in 100k (default 9 = 900k)\n\
 -c       : output to standard out (stdout)\n\
 -d       : decompress file\n\
 -f       : force, overwrite existing output file\n\
 -h       : print this help message\n\
 -k       : keep input file, dont delete\n\
 -l       : load average determines max number processors to use\n\
 -p#      : where # is the number of processors (default 2)\n\
 -q       : quiet mode (default)\n\
 -r       : read entire input file into RAM and split between processors\n\
 -t       : test compressed file integrity\n\
 -v       : verbose mode\n\
 -V       : display version info for pbzip2 then exit\n\
 -z       : compress file (default)\n\
 -1 .. -9 : set BWT block size to 100k .. 900k (default 900k)\n\
\n\
Example: pbzip2 -b15vk myfile.tar\n\
Example: pbzip2 -p4 -r -5 myfile.tar second*.txt\n\
Example: tar cf myfile.tar.bz2 --use-compress-prog=pbzip2 dir_to_compress/\n\
Example: pbzip2 -d myfile.tar.bz2\n\
"), NULL },
#endif

  POPT_TABLEEND

};

#ifdef	NOTYET
/**
 */
static rpmRC rpmzParseEnv(/*@unused@*/ rpmz z, /*@null@*/ const char * envvar)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    static char whitespace[] = " \f\n\r\t\v,";
    static char _envvar[] = "RPMZ";
    char * s = getenv((envvar ? envvar : _envvar));
    ARGV_t av = NULL;
    poptContext optCon = NULL;
    rpmRC rc = RPMRC_OK;
    int ac;
    int xx;

    if (s == NULL)
	goto exit;

    /* XXX todo: argvSplit() assumes single separator between args. */
/*@-nullstate@*/
    xx = argvSplit(&av, s, whitespace);
/*@=nullstate@*/
    ac = argvCount(av);
    if (ac < 1)
	goto exit;

    optCon = poptGetContext(__progname, ac, (const char **)av,
		optionsTable, POPT_CONTEXT_KEEP_FIRST);

    /* Process all options, whine if unknown. */
    while ((xx = poptGetNextOpt(optCon)) > 0) {
	const char * optArg = poptGetOptArg(optCon);
/*@-dependenttrans -modobserver -observertrans @*/
	optArg = _free(optArg);
/*@=dependenttrans =modobserver =observertrans @*/
	switch (xx) {
	default:
/*@-nullpass@*/
	    fprintf(stderr, _("%s: option table misconfigured (%d)\n"),
		__progname, xx);
/*@=nullpass@*/
	    rc = RPMRC_FAIL;
	    goto exit;
	    /*@notreached@*/ /*@switchbreak@*/ break;
        }
    }

    if (xx < -1) {
/*@-nullpass@*/
	fprintf(stderr, "%s: %s: %s\n", __progname,
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		poptStrerror(xx));
/*@=nullpass@*/
	rc = RPMRC_FAIL;
    }

    /* Check that only options were in the envvar. */
    if (argvCount(poptGetArgs(optCon)))
	bail("cannot provide files in GZIP environment variable", "");

exit:
    if (optCon)
	optCon = poptFreeContext(optCon);
    av = argvFree(av);
    return rc;
}
#endif

/**
 */
static int rpmzParseArgv0(rpmz z, const char * argv0)
	/*@globals internalState @*/
	/*@modifies z, internalState @*/
{
    rpmzQueue zq = z->zq;
    const char * s = strrchr(argv0, PATH_SEP);
    const char * name = (s != NULL ? (s + 1) : argv0);
#ifdef	NOTYET
    rpmRC rc = RPMRC_OK;

#if defined(WITH_XZ)
    if (strstr(name, "xz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_XZ;
	z->format = RPMZ_FORMAT_XZ;	/* XXX eliminate */
    } else
    if (strstr(name, "lz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_LZMA;
	zq->format = RPMZ_FORMAT_LZMA;	/* XXX eliminate */
    } else
#endif	/* WITH_XZ */
#if defined(WITH_BZIP2)
    if (strstr(name, "bz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_BZIP2;
	zq->format = RPMZ_FORMAT_BZIP2;	/* XXX eliminate */
    } else
#endif	/* WITH_BZIP2 */
#if defined(WITH_ZLIB)
    if (strstr(name, "gz") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_GZIP;
	zq->format = RPMZ_FORMAT_GZIP;	/* XXX eliminate */
    } else
    if (strstr(name, "zlib") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_ZLIB;
	zq->format = RPMZ_FORMAT_ZLIB;	/* XXX eliminate */
    } else
	/* XXX watchout for "bzip2" matching */
    if (strstr(name, "zip") != NULL) {
	z->_format_compress_auto = RPMZ_FORMAT_ZIP2;
	zq->format = RPMZ_FORMAT_ZIP2;	/* XXX eliminate */
    } else
#endif	/* WITH_ZLIB */
    {
	z->_format_compress_auto = RPMZ_FORMAT_AUTO;
	zq->format = RPMZ_FORMAT_AUTO;	/* XXX eliminate */
    }

    if (strstr(name, "cat") != NULL) {
	zq->mode = RPMZ_MODE_DECOMPRESS;
	zq->flags |= RPMZ_FLAGS_STDOUT;
    } else if (strstr(name, "un") != NULL) {
	zq->mode = RPMZ_MODE_DECOMPRESS;
    }

    return rc;
#else

    if ((strstr(name, "unzip") != 0) || (strstr(name, "UNZIP") != 0))
	zq->mode = RPMZ_MODE_DECOMPRESS;
    if ((strstr(name, "zcat") != 0) || (strstr(name, "ZCAT") != 0)) {
	zq->flags |= RPMZ_FLAGS_STDOUT;
	zq->flags |= RPMZ_FLAGS_KEEP;
	zq->mode = RPMZ_MODE_DECOMPRESS;
    }

    return 0;

#endif
}

int main(int argc, char* argv[])
	/*@globals _rpmz, __assert_program_name, h_errno, fileSystem, internalState @*/
	/*@modifies _rpmz, __assert_program_name, fileSystem, internalState @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = _rpmzq;
    rpmzLog zlog = NULL;
    poptContext optCon;

    const char stdinFile[] = "-";
    int ac = 0;
    int rc = 0;
    int xx;
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmpbzip2";
/*@=observertrans =readonlytrans @*/
    z->zq = zq;                /* XXX initialize rpmzq */
    zq->threads	= 2;
    zq->blocksize = 1*100000;
    zq->verbosity = 1;		/* normal message level */
    zq->flags = RPMZ_FLAGS_NONE;
    zq->format = RPMZ_FORMAT_BZIP2;
    zq->mode = RPMZ_MODE_COMPRESS;
    zq->level = 1;
    zq->suffix = "bz2",	/* compressed file suffix */

    /* XXX sick hack to initialize the popt callback. */
    rpmzqOptionsPoptTable[0].arg = (void *)&rpmzqArgCallback;

    /* XXX add POPT_ARG_TIMEOFDAY oneshot? */
    xx = gettimeofday(&z->start, NULL);	/* starting time for log entries */
#if defined(PBZIP_DEBUG) || defined(__LCLINT__)
    zlog = rpmzLogNew(&z->start);		/* initialize logging */
#endif

    hw_init(z);

    // check to see if we are likely being called from TAR
    if (argc < 2) {
	zq->flags |= RPMZ_FLAGS_STDOUT;
	zq->flags |= RPMZ_FLAGS_KEEP;
    }

#ifdef	NOTYET
    /* set all options to defaults */
    rpmzDefaults(zq);

    /* process user environment variable defaults */
    if (rpmzParseEnv(z, "BZIP2", optionsTable))
        goto exit;
#endif

    // get program name to determine if decompress mode should be used
    xx = rpmzParseArgv0(z, argv[0]);

    optCon = rpmioInit(argc, argv, optionsTable);

    /* Add files from CLI. */
    {	ARGV_t av = poptGetArgs(optCon);
	if (av != NULL)
	xx = argvAppend(&z->argv, av);
    }

#ifdef	NOTYET
    /* Add files from --files manifest(s). */
    if (z->manifests != NULL)
        xx = rpmzLoadManifests(z);
#endif

if (_debug)
argvPrint("input args", z->argv, NULL);

    ac = argvCount(z->argv);
    /* if no command line arguments and stdout is a terminal, show help */
    if ((z->argv == NULL || z->argv[0] == NULL) && isatty(STDOUT_FILENO)) {
        poptPrintUsage(optCon, stderr, 0);
rc = 1;
        goto exit;
    }

    if (ac == 0) {
	if (zq->mode == RPMZ_MODE_TEST) {
	    if (isatty(fileno(stdin))) {
		    fprintf(stderr,"*ERROR: Won't read compressed data from terminal.  Aborting!\n");
		    fprintf(stderr,"For help type: %s -h\n", argv[0]);
		    rc = 1;
		    goto exit;
	    }
	    // expecting data from stdin
	    z->argv[ac++] = stdinFile;
	} else if (F_ISSET(zq->flags, STDOUT)) {
	    if (isatty(fileno(stdout))) {
		    fprintf(stderr,"*ERROR: Won't write compressed data to terminal.  Aborting!\n");
		    fprintf(stderr,"For help type: %s -h\n", argv[0]);
		    rc = 1;
		    goto exit;
	    }
	    // expecting data from stdin
	    z->argv[ac++] = stdinFile;
	} else if (zq->mode == RPMZ_MODE_DECOMPRESS && (argc == 2)) {
	    if (isatty(fileno(stdin))) {
		    fprintf(stderr,"*ERROR: Won't read compressed data from terminal.  Aborting!\n");
		    fprintf(stderr,"For help type: %s -h\n", argv[0]);
		    rc = 1;
		    goto exit;
	    }
	    // expecting data from stdin via TAR
	    zq->flags |= RPMZ_FLAGS_STDOUT;
	    zq->flags |= RPMZ_FLAGS_KEEP;
	    z->argv[ac++] = stdinFile;
	} else {
	    poptPrintUsage(optCon, stderr, 0);
	    bail("Not enough files given", "");
	}
    }

    if (F_ISSET(zq->flags, STDOUT))
	zq->flags |= RPMZ_FLAGS_KEEP;

    z->zq = rpmzqNew(zq, zlog, 0);	/* XXX eliminate */

    signals_init();

    if (zq->verbosity > 0) {
	if (zq->mode != RPMZ_MODE_TEST) {
	    if (zq->mode != RPMZ_MODE_DECOMPRESS) {
		fprintf(stderr, " BWT Block Size: %d00k\n", zq->level);
		if (zq->blocksize < 100000)
		    fprintf(stderr, "File Block Size: %d bytes\n", zq->blocksize);
		else
		    fprintf(stderr, "File Block Size: %dk\n", zq->blocksize/1000);
	    }
	}
	fprintf(stderr, "-------------------------------------------\n");
    }

    // process all files
    for (i = 0; i < ac; i++) {
	const char * ifn = z->argv[i];
	rc = rpmzProcess(z, ifn, i);
    }

exit:
    z->zq->zlog = rpmzLogDump(z->zq->zlog, NULL);
    z->zq = rpmzqFree(z->zq);

    z->argv = argvFree(z->argv);

    signals_exit();

    optCon = rpmioFini(optCon);

/*@-globstate@*/	/* XXX __progname silliness */
    return rc;
/*@=globstate@*/
}
