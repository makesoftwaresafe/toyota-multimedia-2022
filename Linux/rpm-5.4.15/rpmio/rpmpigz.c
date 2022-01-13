/* pigz.c -- parallel implementation of gzip
 * Copyright (C) 2007, 2008 Mark Adler
 * Version 2.1.4  9 Nov 2008  Mark Adler
 */

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu

  Mark accepts donations for providing this software.  Donations are not
  required or expected.  Any amount that you feel is appropriate would be
  appreciated.  You can use this link:

  https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=536055

 */

/* Version history:
   1.0    17 Jan 2007  First version, pipe only
   1.1    28 Jan 2007  Avoid void * arithmetic (some compilers don't get that)
                       Add note about requiring zlib 1.2.3
                       Allow compression level 0 (no compression)
                       Completely rewrite parallelism -- add a write thread
                       Use deflateSetDictionary() to make use of history
                       Tune argument defaults to best performance on four cores
   1.2.1   1 Feb 2007  Add long command line options, add all gzip options
                       Add debugging options
   1.2.2  19 Feb 2007  Add list (--list) function
                       Process file names on command line, write .gz output
                       Write name and time in gzip header, set output file time
                       Implement all command line options except --recursive
                       Add --keep option to prevent deleting input files
                       Add thread tracing information with -vv used
                       Copy crc32_combine() from zlib (possible thread issue)
   1.3    25 Feb 2007  Implement --recursive
                       Expand help to show all options
                       Show help if no arguments or output piping are provided
                       Process options in GZIP environment variable
                       Add progress indicator to write thread if --verbose
   1.4     4 Mar 2007  Add --independent to facilitate damaged file recovery
                       Reallocate jobs for new --blocksize or --processes
                       Do not delete original if writing to stdout
                       Allow --processes 1, which does no threading
                       Add NOTHREAD define to compile without threads
                       Incorporate license text from zlib in source code
   1.5    25 Mar 2007  Reinitialize jobs for new compression level
                       Copy attributes and owner from input file to output file
                       Add decompression and testing
                       Add -lt (or -ltv) to show all entries and proper lengths
                       Add decompression, testing, listing of LZW (.Z) files
                       Only generate and show trace log if DEBUG defined
                       Take "-" argument to mean read file from stdin
   1.6    30 Mar 2007  Add zlib stream compression (--zlib), and decompression
   1.7    29 Apr 2007  Decompress first entry of a zip file (if deflated)
                       Avoid empty deflate blocks at end of deflate stream
                       Show zlib check value (Adler-32) when listing
                       Don't complain when decompressing empty file
                       Warn about trailing junk for gzip and zlib streams
                       Make listings consistent, ignore gzip extra flags
                       Add zip stream compression (--zip)
   1.8    13 May 2007  Document --zip option in help output
   2.0    19 Oct 2008  Complete rewrite of thread usage and synchronization
                       Use polling threads and a pool of memory buffers
                       Remove direct pthread library use, hide in yarn.c
   2.0.1  20 Oct 2008  Check version of zlib at compile time, need >= 1.2.3
   2.1    24 Oct 2008  Decompress with read, write, inflate, and check threads
                       Remove spurious use of ctime_r(), ctime() more portable
                       Change application of job->calc lock to be a semaphore
                       Detect size of off_t at run time to select %lu vs. %llu
                       #define large file support macro even if not __linux__
                       Remove _LARGEFILE64_SOURCE, _FILE_OFFSET_BITS is enough
                       Detect file-too-large error and report, blame build
                       Replace check combination routines with those from zlib
   2.1.1  28 Oct 2008  Fix a leak for files with an integer number of blocks
                       Update for yarn 1.1 (yarn_prefix and yarn_abort)
   2.1.2  30 Oct 2008  Work around use of beta zlib in production systems
   2.1.3   8 Nov 2008  Don't use zlib combination routines, put back in pigz
   2.1.4   9 Nov 2008  Fix bug when decompressing very short files
 */

#define PIGZVERSION "pigz 2.1.4\n"

/* To-do:
    - add --rsyncable (or -R) [use my own algorithm, set min/max block size]
    - make source portable for Windows, VMS, etc. (see gzip source code)
    - make build portable (currently good for Unixish)
 */

/*
   pigz compresses using threads to make use of multiple processors and cores.
   The input is broken up into 128 KB chunks with each compressed in parallel.
   The individual check value for each chunk is also calculated in parallel.
   The compressed data is written in order to the output, and a combined check
   value is calculated from the individual check values.

   The compressed data format generated is in the gzip, zlib, or single-entry
   zip format using the deflate compression method.  The compression produces
   partial raw deflate streams which are concatenated by a single write thread
   and wrapped with the appropriate header and trailer, where the trailer
   contains the combined check value.

   Each partial raw deflate stream is terminated by an empty stored block
   (using the Z_SYNC_FLUSH option of zlib), in order to end that partial bit
   stream at a byte boundary.  That allows the partial streams to be
   concatenated simply as sequences of bytes.  This adds a very small four to
   five byte overhead to the output for each input chunk.

   The default input block size is 128K, but can be changed with the -b option.
   The number of compress threads is set by default to 8, which can be changed
   using the -p option.  Specifying -p 1 avoids the use of threads entirely.

   The input blocks, while compressed independently, have the last 32K of the
   previous block loaded as a preset dictionary to preserve the compression
   effectiveness of deflating in a single thread.  This can be turned off using
   the --independent or -i option, so that the blocks can be decompressed
   independently for partial error recovery or for random access.

   Decompression can't be parallelized, at least not without specially prepared
   deflate streams for that purpose.  As a result, pigz uses a single thread
   (the main thread) for decompression, but will create three other threads for
   reading, writing, and check calculation, which can speed up decompression
   under some circumstances.  Parallel decompression can be turned off by
   specifying one process (-dp 1 or -tp 1).

   pigz requires zlib 1.2.1 or later to allow setting the dictionary when doing
   raw deflate.  Since zlib 1.2.3 corrects security vulnerabilities in zlib
   version 1.2.1 and 1.2.2, conditionals check for zlib 1.2.3 or later during
   the compilation of pigz.c.

   pigz uses the POSIX pthread library for thread control and communication,
   through the yarn.h interface to yarn.c.  yarn.c can be replaced with
   equivalent implementations using other thread libraries.  pigz can be
   compiled with NOTHREAD #defined to not use threads at all (in which case
   pigz will not be able to live up to the "parallel" in its name).
 */

/*
   Details of parallel compression implementation:

   When doing parallel compression, pigz uses the main thread to read the input
   in 'size' sized chunks (see -b), and puts those in a compression job list,
   each with a sequence number to keep track of the ordering.  If it is not the
   first chunk, then that job also points to the previous input buffer, from
   which the last 32K will be used as a dictionary (unless -i is specified).
   This sets a lower limit of 32K on 'size'.

   pigz launches up to 'zq->threads' compression threads (see --threads).  Each compression
   thread continues to look for jobs in the compression list and perform those
   jobs until instructed to return.  When a job is pulled, the dictionary, if
   provided, will be loaded into the deflate engine and then that input buffer
   is dropped for reuse.  Then the input data is compressed into an output
   buffer sized to assure that it can contain maximally expanded deflate data.
   The job is then put into the write job list, sorted by the sequence number.
   The compress thread however continues to calculate the check value on the
   input data, either a CRC-32 or Adler-32, possibly in parallel with the write
   thread writing the output data.  Once that's done, the compress thread drops
   the input buffer and also releases the lock on the check value so that the
   write thread can combine it with the previous check values.  The compress
   thread has then completed that job, and goes to look for another.

   All of the compress threads are left running and waiting even after the last
   chunk is processed, so that they can support the next input to be compressed
   (more than one input file on the command line).  Once pigz is done, it will
   call all the compress threads home (that'll do pig, that'll do).

   Before starting to read the input, the main thread launches the write thread
   so that it is ready pick up jobs immediately.  The compress thread puts the
   write jobs in the list in sequence sorted order, so that the first job in
   the list is always has the lowest sequence number.  The write thread waits
   for the next write job in sequence, and then gets that job.  The job still
   holds its input buffer, from which the write thread gets the input buffer
   length for use in check value combination.  Then the write thread drops that
   input buffer to allow its reuse.  Holding on to the input buffer until the
   write thread starts also has the benefit that the read and compress threads
   can't get way ahead of the write thread and build up a large backlog of
   unwritten compressed data.  The write thread will write the compressed data,
   drop the output buffer, and then wait for the check value to be unlocked
   by the compress thread.  Then the write thread combines the check value for
   this chunk with the total check value for eventual use in the trailer.  If
   this is not the last chunk, the write thread then goes back to look for the
   next output chunk in sequence.  After the last chunk, the write thread
   returns and joins the main thread.  Unlike the compress threads, a new write
   thread is launched for each input stream.  The write thread writes the
   appropriate header and trailer around the compressed data.

   The input and output buffers are reused through their collection in pools.
   Each buffer has a use count, which when decremented to zero returns the
   buffer to the respective pool.  Each input buffer has up to three parallel
   uses: as the input for compression, as the data for the check value
   calculation, and as a dictionary for compression.  Each output buffer has
   only one use, which is as the output of compression followed serially as
   data to be written.  The input pool is limited in the number of buffers, so
   that reading does not get way ahead of compression and eat up memory with
   more input than can be used.  The limit is approximately two times the
   number of compression threads.  In the case that reading is fast as compared
   to compression, that number allows a second set of buffers to be read while
   the first set of compressions are being performed.  The number of output
   buffers is not directly limited, but is indirectly limited by the release of
   input buffers to the same number.
 */

#define	DEBUG	/* XXX this shouldn't be here. */

#include "system.h"

#include "zlib.h"       /* deflateInit2(), deflateReset(), deflate(), */
                        /* deflateEnd(), deflateSetDictionary(), crc32(),
                           Z_DEFAULT_COMPRESSION, Z_DEFAULT_STRATEGY,
                           Z_DEFLATED, Z_NO_FLUSH, Z_NULL, Z_OK,
                           Z_SYNC_FLUSH, z_stream */

#if !defined(ZLIB_VERNUM) || ZLIB_VERNUM < 0x1230
#  error Need zlib version 1.2.3 or later
#endif

#if defined(__LCLINT__)
/*@-incondefs@*/
ZEXTERN int ZEXPORT deflateInit2_ OF((z_streamp strm, int  level, int  method,
                                      int windowBits, int memLevel,
                                      int strategy, const char *version,
                                      int stream_size))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT deflateReset OF((z_streamp strm))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT deflateParams OF((z_streamp strm,
				      int level, int strategy))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT deflateSetDictionary OF((z_streamp strm,
                                             const Bytef *dictionary,
                                             uInt  dictLength))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT deflate OF((z_streamp strm, int flush))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT deflateEnd OF((z_streamp strm))
	/*@modifies strm @*/;

ZEXTERN int ZEXPORT inflateBackInit_ OF((z_streamp strm, int windowBits,
                                         unsigned char FAR *window,
                                         const char *version,
                                         int stream_size))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT inflateBack OF((z_streamp strm,
                                    in_func in, void FAR *in_desc,
                                    out_func out, void FAR *out_desc))
	/*@modifies strm @*/;
ZEXTERN int ZEXPORT inflateBackEnd OF((z_streamp strm))
	/*@modifies strm @*/;

ZEXTERN uLong ZEXPORT adler32 OF((uLong adler, const Bytef *buf, uInt len))
	/*@*/;
ZEXTERN uLong ZEXPORT crc32   OF((uLong crc, const Bytef *buf, uInt len))
	/*@*/;
/*@=incondefs@*/

#endif

#define	_RPMIOB_INTERNAL
#define	_RPMZQ_INTERNAL
#define	_RPMZ_INTERNAL
#define	_RPMZ_INTERNAL_PIGZ
#include <rpmz.h>

#include <rpmdir.h>

#include "debug.h"

#if defined(DEBUG) || defined(__LCLINT__)
#define Trace(x) \
    do { \
	if (zq->verbosity > 2) { \
	    rpmzLogAdd x; \
	} \
    } while (0)
#else	/* !DEBUG */
#define rpmzLogDump(_zlog, _fp)
#define Trace(x)
#endif	/* DEBUG */

/*@unchecked@*/
static int _debug = 0;

#define F_ISSET(_f, _FLAG) (((_f) & ((RPMZ_FLAGS_##_FLAG) & ~0x40000000)) != RPMZ_FLAGS_NONE)

/*@-fullinitblock@*/
/*@unchecked@*/
struct rpmz_s __rpmz = {
    .stdin_fn	= "<stdin>",
    .stdout_fn	= "<stdout>",
};
/*@=fullinitblock@*/

/* set option defaults */
/*@-mustmod@*/
static void rpmzDefaults(rpmzQueue zq)
	/*@modifies zq @*/
{
/*@-observertrans -readonlytrans@*/
    zq->suffix = ".gz";               /* compressed file suffix */
/*@=observertrans =readonlytrans@*/
    zq->_zinp.fn = _rpmz->_ifn;
    zq->_zinp.fdno = -1;
    zq->_zout.fn = NULL;
    zq->_zout.fdno = -1;

    zq->level = Z_DEFAULT_COMPRESSION;	/* XXX level is format specific. */
#ifdef _PIGZNOTHREAD
    zq->threads = 1;
#else
    zq->threads = 8;
#endif
    zq->verbosity = 1;              /* normal message level */
    zq->blocksize = 131072UL;
    zq->flags &= ~RPMZ_FLAGS_RSYNCABLE; /* don't do rsync blocking */
  /* XXX logic is reversed, disablers should clear with toggle. */
    zq->flags |= RPMZ_FLAGS_INDEPENDENT; /* initialize dictionary each thread */
    zq->flags |= (RPMZ_FLAGS_HNAME|RPMZ_FLAGS_HTIME); /* store/restore name and timestamp */
    zq->flags &= ~RPMZ_FLAGS_STDOUT; /* don't force output to stdout */
    zq->mode = RPMZ_MODE_COMPRESS;   /* compress */
    zq->flags &= ~RPMZ_FLAGS_LIST;   /* compress */
    zq->flags &= ~RPMZ_FLAGS_KEEP;   /* delete input file once compressed */
    zq->flags &= ~RPMZ_FLAGS_FORCE;  /* don't overwrite, don't compress links */
    zq->flags &= ~RPMZ_FLAGS_RECURSE;/* don't go into directories */
    zq->format = RPMZ_FORMAT_GZIP;   /* use gzip format */
}
/*@=mustmod@*/

/*@unchecked@*/
rpmz _rpmz = &__rpmz;

#define	WBITS	15
/* deflate window bits (should be 15). deflate is negative for raw stream. */
/*@unchecked@*/
static int defWBits = -WBITS;
/* inflate window bits (should be 15). */
/*@unchecked@*/
static int infWBits = WBITS;

/*==============================================================*/

/* prevent end-of-line conversions on MSDOSish operating systems */
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <io.h>       /* setmode(), O_BINARY */
#  define SET_BINARY_MODE(_fdno) setmode(_fdno, O_BINARY)
#else
#  define SET_BINARY_MODE(_fdno)
#endif

/* exit with error, delete output file if in the middle of writing it */
/*@exits@*/
static int bail(const char *why, const char *what)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _rpmz->zq;

/*@-globs@*/
    if (zq->_zout.fdno > STDOUT_FILENO && zq->_zout.fn != NULL)
	Unlink(zq->_zout.fn);
/*@=globs@*/
    if (zq->verbosity > 0)
	fprintf(stderr, "%s abort: %s%s\n", "pigz", why, what);
    exit(EXIT_FAILURE);
/*@notreached@*/
    return 0;
}

/*==============================================================*/
/* sliding dictionary size for deflate */
#define _PIGZDICT 32768U

/* largest power of 2 that fits in an unsigned int -- used to limit requests
   to zlib functions that use unsigned int lengths */
#define _PIGZMAX ((((unsigned)0 - 1) >> 1) + 1)

typedef /*@abstract@*/ struct rpmgz_s * rpmgz;
/*@access rpmgz @*/

struct rpmgz_s {
    z_stream strm;
    int strategy;
    int level;
    int omode;			/*!< open mode: O_RDONLY | O_WRONLY */
};

/*@only@*/ /*@null@*/
static rpmgz rpmgzFini(/*@only@*/ rpmgz gz)
	/*@modifies gz @*/
{
/*@-compdestroy@*/
    gz = _free(gz);
/*@=compdestroy@*/
    return NULL;
}

/*@only@*/
static rpmgz rpmgzInit(int level, mode_t omode)
	/*@*/
{
    rpmgz gz = xcalloc(1, sizeof(*gz));

    gz->strategy = Z_DEFAULT_STRATEGY;
    gz->level = (level >= 1 && level <= 9) ? level : Z_DEFAULT_COMPRESSION;
    gz->omode = omode;
    return gz;
}

/*@only@*/
static rpmgz rpmgzCompressInit(int level, mode_t omode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmgz gz = rpmgzInit(level, omode);
    z_stream * sp = &gz->strm;

/*@-nullstate -nullret @*/
    sp->zfree = Z_NULL;
    sp->zalloc = Z_NULL;
    sp->opaque = Z_NULL;
    if (deflateInit2(sp, gz->level, Z_DEFLATED, defWBits, 8, gz->strategy) != Z_OK)
	bail("not enough memory", "deflateInit2");
    return gz;
/*@=nullstate =nullret @*/
}

/*@-mustmod@*/
static void rpmgzCompressReset(rpmgz gz, rpmzJob job)
	/*@globals fileSystem, internalState @*/
	/*@modifies gz, job, fileSystem, internalState @*/
{
    z_stream * sp = &gz->strm;

    /* got a job -- initialize and set the compression level (note that if
     * deflateParams() is called immediately after deflateReset(), there is
     * no need to initialize the input/output for the stream) */
/*@-noeffect@*/
    (void)deflateReset(sp);
    (void)deflateParams(sp, gz->level, gz->strategy);
/*@=noeffect@*/

    /* set dictionary if provided, release that input buffer (only provided
     * if F_ISSET(zq->flags, INDEPENDENT) is true and if this is not the
     * first work unit) */
    if (job->out != NULL) {
	unsigned char * buf = job->out->buf;
assert(job->out->len >= _PIGZDICT);
	buf += job->out->len;
	buf -= _PIGZDICT;
/*@-noeffect@*/
	deflateSetDictionary(sp, buf, _PIGZDICT);
/*@=noeffect@*/
	(void) rpmzqDropSpace(job->out);	/* XXX don't reset job->out */
    }
}
/*@=mustmod@*/

/*@-mustmod@*/
static void rpmgzCompress(rpmgz gz, rpmzJob job)
	/*@modifies gz, job->out @*/
{
    z_stream * sp = &gz->strm;
    unsigned char * out_buf = job->out->buf;
    size_t len;

    sp->next_in = job->in->buf;
    sp->next_out = job->out->buf;

    /* run _PIGZMAX-sized amounts of input through deflate -- this loop is
     * needed for those cases where the integer type is smaller than the
     * size_t type, or when len is close to the limit of the size_t type */
    for (len = job->in->len; len > _PIGZMAX; len -= _PIGZMAX) {
	sp->avail_in = _PIGZMAX;
	sp->avail_out = (unsigned)-1;
/*@-noeffect@*/
	(void)deflate(sp, Z_NO_FLUSH);
/*@=noeffect@*/
assert(sp->avail_in == 0 && sp->avail_out != 0);
    }

    /* run the last piece through deflate -- terminate with a sync marker,
     * or finish deflate stream if this is the last block */
    sp->avail_in = (unsigned)len;
    sp->avail_out = (unsigned)-1;
/*@-noeffect@*/
    (void)deflate(sp, job->more ? Z_SYNC_FLUSH :  Z_FINISH);
/*@=noeffect@*/
assert(sp->avail_in == 0 && sp->avail_out != 0);
    job->out->len = sp->next_out - out_buf;
}
/*@=mustmod@*/

/*@-mustmod@*/
/*@only@*/ /*@null@*/
static rpmgz rpmgzCompressFini(/*@only@*/ rpmgz gz)
	/*@modifies gz @*/
{
    z_stream * sp = &gz->strm;

/*@-noeffect@*/
    deflateEnd(sp);
/*@=noeffect@*/
    return rpmgzFini(gz);
}
/*@=mustmod@*/

/*==============================================================*/

static void jobDebug(/*@null@*/ const char * msg, /*@null@*/ rpmzJob job)
	/*@*/
{

    if (_debug == 0)
	return;
/*@-modfilesys@*/
    if (msg != NULL)
	fprintf(stderr, "--- %s\tjob %p", msg, job);
    else
	fprintf(stderr, "\tjob %p", job);

    if (job != NULL) {
	fprintf(stderr, "[%u]:", (unsigned)job->seq);
	if (job->in != NULL && job->in->buf != NULL)
	    fprintf(stderr, " %p[%lu]", job->in->buf, (unsigned long)job->in->len);
	else
	    fprintf(stderr, " %p", job->in);
	fprintf(stderr, " ->\t");
	if (job->out != NULL && job->out->buf != NULL)
	    fprintf(stderr, " %p[%lu]", job->out->buf, (unsigned long)job->out->len);
	else
	    fprintf(stderr, " %p", job->out);
    }
    fprintf(stderr, "\n");
/*@=modfilesys@*/
}

/* read up to len bytes into buf, repeating read() calls as needed */
static size_t rpmzRead(rpmzf zf, unsigned char *buf, size_t len)
	/*@globals fileSystem, internalState @*/
	/*@modifies *buf, fileSystem, internalState @*/
{
    size_t got = 0;

    got = 0;
    while (len) {
	ssize_t ret = read(zf->fdno, buf, len);
	if (ret < 0)
	    bail("read error on ", zf->fn);
	if (ret == 0)
	    break;
	buf += ret;
	len -= ret;
	got += ret;
    }
    return got;		/* XXX never returns < 0 */
}

/* write len bytes, repeating write() calls as needed */
static void rpmzWrite(rpmzf zf, unsigned char *buf, size_t len)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    size_t put = 0;

    while (len) {
	ssize_t ret = write(zf->fdno, buf, len);
	if (ret < 1)
	    fprintf(stderr, "write error code %d\n", errno);
	if (ret < 1)
	    bail("write error on ", zf->fn);
	buf += ret;
	len -= ret;
	put += ret;
    }
}

static off_t rpmzSeek(rpmzf zf, off_t offset, int whence)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    return lseek(zf->fdno, offset, whence);
}

static int rpmzClose(rpmzf zf)
{
    return close(zf->fdno);
}

/* convert Unix time to MS-DOS date and time, assuming current timezone
   (you got a better idea?) */
static unsigned long time2dos(time_t t)
	/*@*/
{
    struct tm *tm;
    unsigned long dos;

    if (t == 0)
	t = time(NULL);
    tm = localtime(&t);
    if (tm->tm_year < 80 || tm->tm_year > 207)
	return 0;
    dos = (tm->tm_year - 80) << 25;
    dos += (tm->tm_mon + 1) << 21;
    dos += tm->tm_mday << 16;
    dos += tm->tm_hour << 11;
    dos += tm->tm_min << 5;
    dos += (tm->tm_sec + 1) >> 1;   /* round to double-seconds */
    return dos;
}

/* put a 4-byte integer into a byte array in LSB order or MSB order */
#define PUT2L(a,b) (*(a)=(b)&0xff,(a)[1]=(b)>>8)
#define PUT4L(a,b) (PUT2L(a,(b)&0xffff),PUT2L((a)+2,(b)>>16))
#define PUT4M(a,b) (*(a)=(b)>>24,(a)[1]=(b)>>16,(a)[2]=(b)>>8,(a)[3]=(b))

/* write a gzip, zlib, or zip header using the information in the globals */
static unsigned long rpmzPutHeader(rpmzQueue zq, rpmzh zh)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzf zout = &zq->_zout;
    unsigned long len;
    unsigned char head[30];

    switch (zq->format) {
    case RPMZ_FORMAT_ZIP2:	/* zip */
    case RPMZ_FORMAT_ZIP3:
	/* write local header */
	PUT4L(head, 0x04034b50UL);  /* local header signature */
	PUT2L(head + 4, 20);        /* version needed to extract (2.0) */
	PUT2L(head + 6, 8);         /* flags: data descriptor follows data */
	PUT2L(head + 8, 8);         /* deflate */
	PUT4L(head + 10, time2dos(zh->mtime));
	PUT4L(head + 14, 0);        /* crc (not here) */
	PUT4L(head + 18, 0);        /* compressed length (not here) */
	PUT4L(head + 22, 0);        /* uncompressed length (not here) */
	PUT2L(head + 26, zh->name == NULL ? 1 : strlen(zh->name));  /* name length */
	PUT2L(head + 28, 9);        /* length of extra field (see below) */
	rpmzWrite(zout, head, 30);  /* write local header */
	len = 30;

	/* write file name (use "-" for stdin) */
	if (zh->name == NULL)
	    rpmzWrite(zout, (unsigned char *)"-", 1);
	else
	    rpmzWrite(zout, (unsigned char *)zh->name, strlen(zh->name));
	len += zh->name == NULL ? 1 : strlen(zh->name);

	/* write extended timestamp extra field block (9 bytes) */
	PUT2L(head, 0x5455);        /* extended timestamp signature */
	PUT2L(head + 2, 5);         /* number of data bytes in this block */
	head[4] = 1;                /* flag presence of mod time */
	PUT4L(head + 5, zh->mtime); /* mod time */
	rpmzWrite(zout, head, 9);   /* write extra field block */
	len += 9;
	break;
    case RPMZ_FORMAT_ZLIB:	/* zlib */
	head[0] = 0x78;             /* deflate, 32K window */
	head[1] = (zq->level == 9U ? 3 : (zq->level == 1U ? 0 :
	    (zq->level >= 6U || zq->level == (unsigned)Z_DEFAULT_COMPRESSION ? 1 :  2))) << 6;
	head[1] += 31 - (((head[0] << 8) + head[1]) % 31);
	rpmzWrite(zout, head, 2);
	len = 2;
	break;
    case RPMZ_FORMAT_GZIP:	/* gzip */
	head[0] = 31;
	head[1] = 139;
	head[2] = 8;                /* deflate */
	head[3] = zh->name != NULL ? 8 : 0;
	PUT4L(head + 4, zh->mtime);
	head[8] = zq->level == 9 ? 2 : (zq->level == 1 ? 4 : 0);
	head[9] = 3;                /* unix */
	rpmzWrite(zout, head, 10);
	len = 10;
	if (zh->name != NULL)
	    rpmzWrite(zout, (unsigned char *)zh->name, strlen(zh->name) + 1);
	if (zh->name != NULL)
	    len += strlen(zh->name) + 1;
	break;
    default:
assert(0);
	break;
    }
    return len;
}

/* write a gzip, zlib, or zip trailer */
static void rpmzPutTrailer(rpmzQueue zq, rpmzh zh)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzf zout = &zq->_zout;
    unsigned char tail[46];

    switch (zq->format) {
    case RPMZ_FORMAT_ZIP2:	/* zip */
    case RPMZ_FORMAT_ZIP3:
    {	unsigned long cent;

	/* write data descriptor (as promised in local header) */
	PUT4L(tail, zh->check);
	PUT4L(tail + 4, zh->clen);
	PUT4L(tail + 8, zh->ulen);
	rpmzWrite(zout, tail, 12);

	/* write central file header */
	PUT4L(tail, 0x02014b50UL);  /* central header signature */
	tail[4] = 63;               /* obeyed version 6.3 of the zip spec */
	tail[5] = 255;              /* ignore external attributes */
	PUT2L(tail + 6, 20);        /* version needed to extract (2.0) */
	PUT2L(tail + 8, 8);         /* data descriptor is present */
	PUT2L(tail + 10, 8);        /* deflate */
	PUT4L(tail + 12, time2dos(zh->mtime));
	PUT4L(tail + 16, zh->check);/* crc */
	PUT4L(tail + 20, zh->clen); /* compressed length */
	PUT4L(tail + 24, zh->ulen); /* uncompressed length */
	PUT2L(tail + 28, zh->name == NULL ? 1 : strlen(zh->name));  /* name length */
	PUT2L(tail + 30, 9);        /* length of extra field (see below) */
	PUT2L(tail + 32, 0);        /* no file comment */
	PUT2L(tail + 34, 0);        /* disk number 0 */
	PUT2L(tail + 36, 0);        /* internal file attributes */
	PUT4L(tail + 38, 0);        /* external file attributes (ignored) */
	PUT4L(tail + 42, 0);        /* offset of local header */
	rpmzWrite(zout, tail, 46);  /* write central file header */
	cent = 46;

	/* write file name (use "-" for stdin) */
	if (zh->name == NULL)
	    rpmzWrite(zout, (unsigned char *)"-", 1);
	else
	    rpmzWrite(zout, (unsigned char *)zh->name, strlen(zh->name));
	cent += zh->name == NULL ? 1 : strlen(zh->name);

	/* write extended timestamp extra field block (9 bytes) */
	PUT2L(tail, 0x5455);        /* extended timestamp signature */
	PUT2L(tail + 2, 5);         /* number of data bytes in this block */
	tail[4] = 1;                /* flag presence of mod time */
	PUT4L(tail + 5, zh->mtime); /* mod time */
	rpmzWrite(zout, tail, 9);   /* write extra field block */
	cent += 9;

	/* write end of central directory record */
	PUT4L(tail, 0x06054b50UL);  /* end of central directory signature */
	PUT2L(tail + 4, 0);         /* number of this disk */
	PUT2L(tail + 6, 0);         /* disk with start of central directory */
	PUT2L(tail + 8, 1);         /* number of entries on this disk */
	PUT2L(tail + 10, 1);        /* total number of entries */
	PUT4L(tail + 12, cent);     /* size of central directory */
	PUT4L(tail + 16, zh->head + zh->clen + 12); /* offset of central directory */
	PUT2L(tail + 20, 0);        /* no zip file comment */
	rpmzWrite(zout, tail, 22);  /* write end of central directory record */
    }	break;
    case RPMZ_FORMAT_ZLIB:	/* zlib */
	PUT4M(tail, zh->check);
	rpmzWrite(zout, tail, 4);
	break;
    case RPMZ_FORMAT_GZIP:	/* gzip */
	PUT4L(tail, zh->check);
	PUT4L(tail + 4, zh->ulen);
	rpmzWrite(zout, tail, 8);
	break;
    default:
assert(0);
	break;
    }
}

/* compute check value depending on format */
#define CHECK(a,b,c) \
    (zq->format == RPMZ_FORMAT_ZLIB ? adler32(a,b,c) : crc32(a,b,c))

#ifndef _PIGZNOTHREAD
/* -- threaded portions of pigz -- */

/* -- check value combination routines for parallel calculation -- */

#define COMB(a,b,c) \
    (zq->format == RPMZ_FORMAT_ZLIB ? adler32_comb(a,b,c) : crc32_comb(a,b,c))
/* combine two crc-32's or two adler-32's (copied from zlib 1.2.3 so that pigz
   can be compatible with older versions of zlib) */

/* we copy the combination routines from zlib here, in order to avoid
   linkage issues with the zlib builds on Sun, Ubuntu, and others */

static unsigned long gf2_matrix_times(unsigned long *mat, unsigned long vec)
	/*@*/
{
    unsigned long sum;

    sum = 0;
    while (vec) {
	if (vec & 1)
	    sum ^= *mat;
	vec >>= 1;
	mat++;
    }
    return sum;
}

static void gf2_matrix_square(unsigned long *square, unsigned long *mat)
	/*@modifies square @*/
{
    int n;

    for (n = 0; n < 32; n++)
	square[n] = gf2_matrix_times(mat, mat[n]);
}

static unsigned long crc32_comb(unsigned long crc1, unsigned long crc2,
		size_t len2)
	/*@*/
{
    int n;
    unsigned long row;
    unsigned long even[32];     /* even-power-of-two zeros operator */
    unsigned long odd[32];      /* odd-power-of-two zeros operator */

    /* degenerate case */
    if (len2 == 0)
	return crc1;

    /* put operator for one zero bit in odd */
    odd[0] = 0xedb88320UL;          /* CRC-32 polynomial */
    row = 1;
    for (n = 1; n < 32; n++) {
	odd[n] = row;
	row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square(odd, even);

    /* apply len2 zeros to crc1 (first square will put the operator for one
       zero byte, eight zero bits, in even) */
    do {
	/* apply zeros operator for this bit of len2 */
	gf2_matrix_square(even, odd);
	if (len2 & 1)
	    crc1 = gf2_matrix_times(even, crc1);
	len2 >>= 1;

	/* if no more bits set, then done */
	if (len2 == 0)
	    break;

	/* another iteration of the loop with odd and even swapped */
	gf2_matrix_square(odd, even);
	if (len2 & 1)
	    crc1 = gf2_matrix_times(odd, crc1);
	len2 >>= 1;

	/* if no more bits set, then done */
    } while (len2 != 0);

    /* return combined crc */
    crc1 ^= crc2;
    return crc1;
}

#define BASE 65521U     /* largest prime smaller than 65536 */
#define LOW16 0xffff    /* mask lower 16 bits */

static unsigned long adler32_comb(unsigned long adler1, unsigned long adler2,
		size_t len2)
	/*@*/
{
    unsigned long sum1;
    unsigned long sum2;
    unsigned rem;

    /* the derivation of this formula is left as an exercise for the reader */
    rem = (unsigned)(len2 % BASE);
    sum1 = adler1 & LOW16;
    sum2 = (rem * sum1) % BASE;
    sum1 += (adler2 & LOW16) + BASE - 1;
    sum2 += ((adler1 >> 16) & LOW16) + ((adler2 >> 16) & LOW16) + BASE - rem;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum2 >= (BASE << 1)) sum2 -= (BASE << 1);
    if (sum2 >= BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}

/*==============================================================*/

/* -- parallel compression -- */

/* compress or write job (passed from compress list to write list) -- if seq is
   equal to -1, compress_thread() is instructed to return; if more is false then
   this is the last chunk, which after writing tells write_thread to return */

/* get the next compression job from the head of the list, compress and compute
   the check value on the input, and put a job in the write list with the
   results -- keep looking for more jobs, returning when a job is found with a
   sequence number of -1 (leave that job in the list for other incarnations to
   find) */
/*@-nullstate@*/
static void compress_thread(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzLog zlog = zq->zlog;
    rpmzc zc = &zq->_zc;
    rpmzw zw = &zq->_zw;

    rpmzJob job;                    /* job pulled and working on */ 
    unsigned long check;            /* check value of input */
    unsigned char *next;            /* pointer for check value data */
    size_t len;                     /* remaining bytes to compress/check */
    rpmgz gz;                       /* deflate stream */

    /* initialize the deflate stream for this thread */
    gz = rpmgzCompressInit(zc->level, zc->omode);

    /* keep looking for work */
    for (;;) {
	/* get a job (like I tell my son) */
	job = rpmzqDelFIFO(zc->q);
	if (job == NULL)
	    break;

	/* got a job -- initialize and set the compression level. */
	Trace((zlog, "-- compressing #%ld", job->seq));
	rpmgzCompressReset(gz, job);

	/* set up input and output (the output size is assured to be big enough
	 * for the worst case expansion of the input buffer size, plus five
	 * bytes for the terminating stored block) */
/*@-mustfreeonly@*/
	job->out = rpmzqNewSpace(zw->pool, zw->pool->size);
/*@=mustfreeonly@*/

	/* Compress the job. */
	rpmgzCompress(gz, job);

	/* reserve input buffer until check value has been calculated */
	rpmzqUseSpace(job->in);

	/* insert write job in list in sorted order, alert write thread */
	rpmzqAddSEQ(zw->q, job);

	/* calculate the check value in parallel with writing, alert the write
	 * thread that the calculation is complete, and drop this usage of the
	 * input buffer */
	len = job->in->len;
	next = job->in->buf;
	check = CHECK(0L, Z_NULL, 0);
	while (len > _PIGZMAX) {
	    check = CHECK(check, next, _PIGZMAX);
	    len -= _PIGZMAX;
	    next += _PIGZMAX;
	}
	check = CHECK(check, next, (unsigned)len);
	(void) rpmzqDropSpace(job->in);		/* XXX don't reset job->in */
	job->check = check;
	yarnPossess(job->calc);
	Trace((zlog, "-- checked #%ld%s", job->seq, job->more ? "" : " (last)"));
	yarnTwist(job->calc, TO, 1);
	job = NULL;	/* XXX job is already free'd here. */

	/* done with that one -- go find another job */
    }

    gz = rpmgzCompressFini(gz);

/*@-mustfreeonly@*/	/* XXX zc->q->head not released */
    return;
/*@=mustfreeonly@*/
}
/*@=nullstate@*/

/* collect the write jobs off of the list in sequence order and write out the
   compressed data until the last chunk is written -- also write the header and
   trailer and combine the individual check values of the input buffers */
static void write_thread(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies _zq, fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzLog zlog = zq->zlog;
    rpmzc zc = &zq->_zc;
    rpmzw zw = &zq->_zw;
    rpmzf zout = &zq->_zout;
    rpmzh zh = zq->_zh;

    long seq;                       /* next sequence number looking for */
    rpmzJob job;                    /* job pulled and working on */
    size_t len;                     /* input length */
    int more;                       /* true if more chunks to write */

    /* build and write header */
    Trace((zlog, "-- write thread running"));
    zh->head = rpmzPutHeader(zq, zh);

    /* process output of compress threads until end of input */    
    zh->ulen = zh->clen = 0;
    zh->check = CHECK(0L, Z_NULL, 0);
    seq = 0;
    do {
	/* get next write job in order */
	job = rpmzqDelSEQ(zw->q, seq);
assert(job != NULL);

	/* update lengths, save uncompressed length for COMB */
	more = job->more;
	len = job->in->len;
	(void) rpmzqDropSpace(job->in);		/* XXX don't reset job->in */
	zh->ulen += (unsigned long)len;
	zh->clen += (unsigned long)(job->out->len);

	/* write the compressed data and drop the output buffer */
	Trace((zlog, "-- writing #%ld", seq));
	rpmzWrite(zout, job->out->buf, job->out->len);
	(void) rpmzqDropSpace(job->out);	/* XXX don't reset job->out */
	Trace((zlog, "-- wrote #%ld%s", seq, more ? "" : " (last)"));

	/* wait for check calculation to complete, then combine, once
	 * the compress thread is done with the input, release it */
	yarnPossess(job->calc);
	yarnWaitFor(job->calc, TO_BE, 1);
	yarnRelease(job->calc);
	zh->check = COMB(zh->check, job->check, len);

	/* free the job */
	job = rpmzqDropJob(job);

	/* get the next buffer in sequence */
	seq++;
    } while (more);

    /* write trailer */
    rpmzPutTrailer(zq, zh);

    /* verify no more jobs, prepare for next use */
    rpmzqVerifyFIFO(zc->q);
    rpmzqVerifySEQ(zw->q);

/*@-globstate@*/	/* XXX zq->q->head is reachable */
    return;
/*@=globstate@*/
}

/*@null@*/
static rpmzc rpmzcFini(rpmzc zc, /*@null@*/ int *caughtp)
	/*@modifies zc, *caughtp @*/
{
    if (caughtp != NULL) {	/* XXX garbage collecting? */
	struct rpmzJob_s job;

	if (zc->q == NULL) {	/* XXX only do this once */
	    if (caughtp)
		*caughtp = 0;
	    return NULL;
	}

	/* command all of the extant compress threads to return */
	yarnPossess(zc->q->have);
	job.seq = -1;
	job.next = NULL;
/*@-immediatetrans -mustfreeonly@*/
	zc->q->head = &job;
/*@=immediatetrans =mustfreeonly@*/
	zc->q->tail = &(job.next);
	yarnTwist(zc->q->have, BY, 1);       /* will wake them all up */

    /* join all of the compress threads, verify they all came back */
	*caughtp = yarnJoinAll();
#ifdef	NOTYET
assert(zc->cthreads == *caughtp);
#else
if (zc->cthreads != *caughtp)
fprintf(stderr, "*** FIXME: cthreads %d joined %d\n", zc->cthreads, *caughtp);
#endif
	zc->cthreads = 0;

	/* free the resources */
	if (zc->pool != NULL)
	    zc->pool = rpmzqFreePool(zc->pool, caughtp);
	else
	    *caughtp = 0;
	if (zc->q != NULL)
	    zc->q = rpmzqFiniFIFO(zc->q);
    }
    return NULL;
}

static rpmzc rpmzcInit(rpmzQueue zq, long val)
	/*@modifies zc @*/
{
    rpmzc zc = &zq->_zc;

    zc->flags = zq->flags;	/* XXX unused */
    zc->format = zq->format;	/* XXX unused */
    zc->mode = zq->mode;	/* XXX unused */

    zc->level = zq->level;
    zc->threads = zq->threads;
    zc->blocksize = (size_t)zq->blocksize;

    zc->omode = zq->omode;

    /* allocate locks and initialize lists */
    if (zc->q == NULL)
	zc->q = rpmzqInitFIFO(val);
    /* initialize buffer pools */
    if (zc->pool == NULL)
	zc->pool = rpmzqNewPool(zc->blocksize, (zc->threads << 1) + 2);
    return zc;
}

static void rpmzcLaunch(rpmzQueue zq, rpmzc zc, long seq)
	/*@modifies zc @*/
{
    if (zc->cthreads < (int)seq && zc->cthreads < (int)zc->threads) {
	(void)yarnLaunch(compress_thread, zq);
/*@-noeffect@*/
	zc->cthreads++;
/*@=noeffect@*/
    }
}

/*@null@*/
static rpmzw rpmzwFini(rpmzw zw, /*@null@*/ int *caughtp)
	/*@modifies zw, *caughtp @*/
{
    if (zw->thread != NULL)
	zw->thread = yarnJoin(zw->thread);

    /* free the resources */
    if (caughtp != NULL) {	/* XXX garbage collecting? */
	if (zw->pool != NULL)
	    zw->pool = rpmzqFreePool(zw->pool, caughtp);
	else
	    *caughtp = 0;
    }
    if (zw->q != NULL)
	zw->q = rpmzqFiniSEQ(zw->q);
    return NULL;
}

static rpmzw rpmzwInit(rpmzQueue zq, long val)
	/*@modifies zc @*/
{
    rpmzw zw = &zq->_zw;

    /* initialize buffer pools */
    if (zw->pool == NULL)
	zw->pool = rpmzqNewPool(zq->blocksize + (zq->blocksize >> 11) + 10, -1);
    /* allocate locks and initialize lists */
    if (zw->q == NULL)
	zw->q = rpmzqInitSEQ(val);
    /* start write thread */
    if (zw->thread == NULL)
	zw->thread = yarnLaunch(write_thread, zq);
    return zw;
}

/* compress zq->ifdno to zq->ofdno, using multiple threads for the compression and check
   value calculations and one other thread for writing the output -- compress
   threads will be launched and left running (waiting actually) to support
   subsequent calls of rpmzParallelCompress() */
static void rpmzParallelCompress(rpmzQueue zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzLog zlog = zq->zlog;
    rpmzf zinp = &zq->_zinp;
    rpmzc zc;
    rpmzw zw;

    long seq;                       /* sequence number */
    rpmzSpace prev;                 /* previous input space */
    rpmzSpace next;                 /* next input space */
    rpmzJob job;                    /* job for compress, then write */
    int more;                       /* true if more input to read */

    /* allocate locks, initialize lists, initialize buffer pools */
    zq->omode = O_WRONLY;	/* XXX eliminate */
    zc = rpmzcInit(zq, 0L);
    zw = rpmzwInit(zq, -1L);

    /* read from input and start compress threads (write thread will pick up
       the output of the compress threads) */
    seq = 0;
    prev = NULL;
    next = rpmzqNewSpace(zc->pool, zc->pool->size);
    next->len = rpmzRead(zinp, next->buf, next->pool->size);
    do {
	/* create a new job, use next input chunk, previous as dictionary */
	job = rpmzqNewJob(seq);
/*@-mustfreeonly@*/
	job->in = next;
	job->out = F_ISSET(zq->flags, INDEPENDENT) ? prev : NULL;  /* dictionary for compression */
/*@=mustfreeonly@*/

	/* check for end of file, reading next chunk if not sure */
	if (next->len < next->pool->size)
	    more = 0;
	else {
	    next = rpmzqNewSpace(zc->pool, zc->pool->size);
	    next->len = rpmzRead(zinp, next->buf, next->pool->size);
	    more = next->len != 0;
	    if (!more)
		next = rpmzqDropSpace(next);  /* won't be using it */
	    if (F_ISSET(zq->flags, INDEPENDENT) && more) {
		rpmzqUseSpace(job->in);    /* hold as dictionary for next loop */
		prev = job->in;
	    }
	}
	job->more = more;
	Trace((zlog, "-- read #%ld%s", seq, more ? "" : " (last)"));
	if (++seq < 1)
	    bail("input too long: ", zq->_zinp.fn);

	/* start another compress thread if needed */
#ifdef	DYING
	if (zc->cthreads < (int)seq && zc->cthreads < (int)zc->threads) {
	    (void)yarnLaunch(compress_thread, zq);
/*@-noeffect@*/
	    zc->cthreads++;
/*@=noeffect@*/
	}
#else
	rpmzcLaunch(zq, zc, seq);
#endif

	/* put job at end of compress list, let all the compressors know */
	rpmzqAddFIFO(zc->q, job);

	/* do until end of input */
    } while (more);

    /* wait for the write thread to complete (we leave the compress threads out
       there and waiting in case there is another stream to compress) */
    rpmzcFini(zc, NULL);
    zw = rpmzwFini(zw, NULL);
    Trace((zlog, "-- write thread joined"));
}

#endif	/* _PIGZNOTHREAD */

/* do a simple compression in a single thread from zq->ifdno to zq->ofdno -- if reset is
   true, instead free the memory that was allocated and retained for input,
   output, and deflate */
/*@-nullstate@*/
static void rpmzSingleCompress(rpmzQueue zq, int reset)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzh zh = zq->_zh;
    rpmzf zinp = &zq->_zinp;
    rpmzf zout = &zq->_zout;

    size_t got;                     /* amount read */
    size_t more;                    /* amount of next read (0 if eof) */
    static unsigned out_size;       /* size of output buffer */
/*@only@*/
    static unsigned char *in;       /* reused i/o buffers */
/*@only@*/
    static unsigned char *next;     /* reused i/o buffers */
/*@only@*/
    static unsigned char *out;      /* reused i/o buffers */
/*@only@*/ /*@null@*/
    static z_stream *strm = NULL;   /* reused deflate structure */

    /* if requested, just release the allocations and return */
    if (reset) {
	if (strm != NULL) {
/*@-noeffect@*/
	    deflateEnd(strm);
/*@=noeffect@*/
	    out = _free(out);
	    next = _free(next);
	    in = _free(in);
	    strm = _free(strm);
	}
	return;
    }

    /* initialize the deflate structure if this is the first time */
    if (strm == NULL) {
	out_size = zq->blocksize > _PIGZMAX ? _PIGZMAX : (unsigned)zq->blocksize;

/*@-mustfreeonly@*/
	in = xmalloc(zq->blocksize);
	next = xmalloc(zq->blocksize);
	out = xmalloc(out_size);
	strm = xmalloc(sizeof(*strm));
/*@=mustfreeonly@*/

/*@-mustfreeonly@*/
	strm->zfree = Z_NULL;
	strm->zalloc = Z_NULL;
	strm->opaque = Z_NULL;
/*@=mustfreeonly@*/
	if (deflateInit2(strm, zq->level, Z_DEFLATED, defWBits, 8,
		Z_DEFAULT_STRATEGY) != Z_OK)
	    bail("not enough memory", "");
    }

    /* write header */
    zh->head = rpmzPutHeader(zq, zh);

    /* set compression level in case it changed */
/*@-noeffect@*/
    (void)deflateReset(strm);
    (void)deflateParams(strm, zq->level, Z_DEFAULT_STRATEGY);
/*@=noeffect@*/

    /* do raw deflate and calculate check value */
    zh->ulen = zh->clen = 0;
    zh->check = CHECK(0L, Z_NULL, 0);
    more = rpmzRead(zinp, next, zq->blocksize);
    do {
	/* get data to compress, see if there is any more input */
	got = more;
	{ unsigned char *temp; temp = in; in = next; next = temp; }
	more = got < zq->blocksize ? 0 : rpmzRead(zinp, next, zq->blocksize);
	zh->ulen += (unsigned long)got;
/*@-mustfreeonly@*/
	strm->next_in = in;
/*@=mustfreeonly@*/

	/* compress _PIGZMAX-size chunks in case unsigned type is small */
	while (got > _PIGZMAX) {
	    strm->avail_in = _PIGZMAX;
	    zh->check = CHECK(zh->check, strm->next_in, strm->avail_in);
	    do {
		strm->avail_out = out_size;
/*@-mustfreeonly@*/
		strm->next_out = out;
/*@=mustfreeonly@*/
/*@-noeffect@*/
		(void)deflate(strm, Z_NO_FLUSH);
/*@=noeffect@*/
		rpmzWrite(zout, out, out_size - strm->avail_out);
		zh->clen += out_size - strm->avail_out;
	    } while (strm->avail_out == 0);
assert(strm->avail_in == 0);
	    got -= _PIGZMAX;
	}

	/* compress the remainder, finishing if end of input -- when not -i,
	 * use a Z_SYNC_FLUSH so that this and parallel compression produce the
	 * same output */
	strm->avail_in = (unsigned)got;
	zh->check = CHECK(zh->check, strm->next_in, strm->avail_in);
	do {
	    strm->avail_out = out_size;
/*@-kepttrans -mustfreeonly@*/
	    strm->next_out = out;
/*@=kepttrans =mustfreeonly@*/
/*@-noeffect@*/
	    (void)deflate(strm,
		more ? (F_ISSET(zq->flags, INDEPENDENT) ? Z_SYNC_FLUSH : Z_FULL_FLUSH) : Z_FINISH);
/*@=noeffect@*/
	    rpmzWrite(zout, out, out_size - strm->avail_out);
	    zh->clen += out_size - strm->avail_out;
	} while (strm->avail_out == 0);
assert(strm->avail_in == 0);

	/* do until no more input */
    } while (more);

    /* write trailer */
    rpmzPutTrailer(zq, zh);
}
/*@=nullstate@*/

/*==============================================================*/
/* --- decompression --- */

#ifndef _PIGZNOTHREAD

/* parallel read thread */
static void load_read_thread(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzi zi = &zq->_zi;
    rpmzf zinp = &zq->_zinp;
    rpmzLog zlog = zq->zlog;
    size_t nread;

    Trace((zlog, "-- launched decompress read thread"));
    do {
	rpmzJob job;
	static long seq = 0;
	yarnPossess(zi->state);
	yarnWaitFor(zi->state, TO_BE, 1);

	job = rpmzqNewJob(seq++);
	job->in = rpmzqNewSpace(zi->pool, zi->pool->size);
	nread = rpmzRead(zinp, job->in->buf, job->in->len);
	job->more = (job->in->len == nread);
	job->in->len = nread;

	/* queue read job at end of list. */
	rpmzqAddFIFO(zi->q, job);

	Trace((zlog, "-- decompress read thread read %lu bytes", nread));
	yarnTwist(zi->state, TO, 0);
    } while (nread == zq->_in_buf_allocated);
    Trace((zlog, "-- exited decompress read thread"));
}

static void rpmziFini(rpmzi zi, /*@null@*/ int *caughtp)
	/*@modifies zi, *caughtp @*/
{
    if (zi->reader != NULL)
	zi->reader = yarnJoin(zi->reader);
    if (caughtp != NULL) {	/* XXX garbage collecting? */
	if (zi->state != NULL)
	    zi->state = yarnFreeLock(zi->state);
	if (zi->q != NULL) {
#ifndef	DYING
	    if (zi->q->head != NULL) {
		zi->q->head->in = rpmzqDropSpace(zi->q->head->in);
		zi->q->head = rpmzqDropJob(zi->q->head);
	    }
#else
	    rpmzqVerifyFIFO(zi->q);
#endif
	    zi->q = rpmzqFiniFIFO(zi->q);
	}
	if (zi->pool != NULL)
	    zi->pool = rpmzqFreePool(zi->pool, caughtp);
	else
	    *caughtp = 0;
    }
}

static void rpmziInit(rpmzQueue zq, rpmzi zi, long val, off_t in_tot)
	/*@modifies zi @*/
{
    if (zi->pool == NULL)
	zi->pool = rpmzqNewPool(zq->_in_buf_allocated, 2);
    if (zi->q == NULL)
	zi->q = rpmzqInitFIFO(0L);
/*@-mustfreeonly@*/
    if (zi->state == NULL)
	zi->state = yarnNewLock(val);
    if (zi->reader == NULL)
	zi->reader = yarnLaunch(load_read_thread, zq);
    zi->cnt = 0;
    zi->in_tot = in_tot;
/*@=mustfreeonly@*/
}

static void rpmziNext(rpmzi zi)
	/*@modifies zi @*/
{
    rpmzFIFO qi = zi->q;

    /* wait for the previously requested read to complete */
    yarnPossess(zi->state);
    yarnWaitFor(zi->state, TO_BE, 0);
    yarnRelease(zi->state);

    /* Drop previous buffer if not initializing. */
    if (zi->cnt++ > 0) {
	rpmzJob ojob = rpmzqDelFIFO(qi);
	ojob->in = rpmzqDropSpace(ojob->in);
	ojob = rpmzqDropJob(ojob);
    }
    zi->in_tot += qi->head->in->len;

    /* if not at end of file, alert read thread to load next buffer */
    if (qi->head->in->len == qi->head->in->pool->size) {
	yarnPossess(zi->state);
	yarnTwist(zi->state, TO, 1);
    } else
	rpmziFini(zi, NULL);
}

#endif	/* _PIGZNOTHREAD */

/* load() is called when job->in->len has gone to zero in order to provide more
   input data: load the input buffer with zq->_in_buf_allocated (or fewer if at end of file) bytes
   from the file zq->ifdno, set job->in->buf to point to the job->in->len bytes read,
   update zq->in_tot, and return job->in->len -- job->more is set to 0 when job->in->len has
   gone to zero and there is no more data left to read from zq->ifdno */
/*@-mustmod@*/
static size_t load(rpmzQueue zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzi zi = &zq->_zi;
    rpmzf zinp = &zq->_zinp;

    /* if already detected end of file, do nothing */
    if (zi->q && zi->q->head && zi->q->head->more < 0) {
	zi->q->head->more = 0;
	return 0;
    }

#if 0		/* XXX hmmm, the comment above is incorrect. todo++ */
assert(zq->qi->in->len == 0);
#endif

#ifndef _PIGZNOTHREAD
    /* if first time in or zq->threads == 1, read a buffer to have something to
       return, otherwise wait for the previous read job to complete */
    if (zq->threads > 1) {
	/* if first time, fire up the read thread, ask for a read */
	if (zq->_in_which == -1)
	    rpmziInit(zq, zi, 1L, zq->in_tot);

	rpmziNext(zi);
	zq->_in_which = (zi->state != NULL ? 1 : -1);

    } else
#endif
    {	size_t nread;
	/* don't use threads -- free old buffer, malloc and read a new buffer */
	
	/* Drop previous buffer if not initializing. */
	if (zq->_in_which != -1)
	    zi->q->head->in = rpmzqDropSpace(zi->q->head->in);
	else {
	    zi->q->head = rpmzqNewJob(0);
	    zq->_in_which = 1;
	}

	zi->q->head->in = rpmzqNewSpace(NULL, zq->_in_buf_allocated);
	nread = rpmzRead(zinp, zi->q->head->in->buf, zi->q->head->in->len);
	zi->q->head->in->len = nread;
    }

    /* note end of file */
    if (zi->q->head->in->len < zq->_in_buf_allocated) {
	zi->q->head->more = -1;

	/* if we got bupkis, now is the time to mark eof */
	if (zi->q->head->in->len == 0)
	    zi->q->head->more = 0;
    } else
	zi->q->head->more = 1;

if (_debug)
jobDebug("loaded", zi->q->head);

    /* update the total and return the available bytes */
    zq->in_tot += zi->q->head->in->len;
#ifdef	NOTYET
assert(zq->in_tot == zi->in_tot);
#else
if (zq->in_tot != zi->in_tot)
fprintf(stderr, "*** FIXME: in_tot zq % ld zi %ld\n", (long)zq->in_tot, (long)zi->in_tot);
#endif
    return zi->q->head->in->len;
}
/*@=mustmod@*/

/* initialize for reading new input */
/*@-mustmod@*/
static void in_init(rpmzQueue zq)
	/*@modifies zq @*/
{
    zq->in_tot = 0;
    zq->_in_which = -1;
}
/*@=mustmod@*/

/* buffered reading macros for decompression and listing */
static size_t rpmzqJobPull(rpmzQueue zq,
		/*@null@*/ unsigned char *buf, size_t len)
	/*@modifies *buf @*/
{
    rpmzi zi = &zq->_zi;
    size_t togo = len;
    size_t got = 0;
    int rc;

    /* initialize input buffer */
    if (zi->q == NULL) {
	in_init(zq);
	rc = load(zq);
    }

    yarnPossess(zi->q->have);

    if (!zi->q->head->more)
	goto exit;
    if (!(zi->q->head->in && zi->q->head->in->len)) {
	yarnRelease(zi->q->have);
	rc = load(zq);
	yarnPossess(zi->q->have);
	if (rc == 0)
	    goto exit;
    }

    while (togo > zi->q->head->in->len) {
	if (buf != NULL) {
	    memcpy(buf, zi->q->head->in->buf, zi->q->head->in->len);
	    buf += zi->q->head->in->len;
	}
	got += zi->q->head->in->len;
	togo -= zi->q->head->in->len;
	{   unsigned char * _buf = zi->q->head->in->buf;
	    _buf += zi->q->head->in->len;
	    zi->q->head->in->buf = _buf;	/* XXX don't change job->in->buf?!? */
	}
	zi->q->head->in->len -= zi->q->head->in->len;

	yarnRelease(zi->q->have);
	rc = load(zq);
	yarnPossess(zi->q->have);
	if (rc == 0)
	    goto exit;
    }
    if (togo > 0) {
	if (buf != NULL) {
	    memcpy(buf, zi->q->head->in->buf, togo);
	    buf += togo;
	}
	{   unsigned char * _buf = zi->q->head->in->buf;
	    _buf += togo;
	    zi->q->head->in->buf = _buf;	/* XXX don't change job->in->buf?!? */
	}
	zi->q->head->in->len -= togo;
	got += togo;
	togo -= togo;
    }
exit:
    yarnRelease(zi->q->have);
    return got;
}

#define PULL(_buf, _len, _rc) \
    do { size_t jpull = (_len); \
	unsigned char * jbuf = (unsigned char *)(_buf); \
assert(jbuf == NULL || jpull < sizeof(_buf)); \
	if ((nb = rpmzqJobPull(zq, jbuf, jpull)) != jpull) \
	    return (_rc); \
	b = jbuf; \
    } while (0);

#define BPULL(_buf, _len, _why) \
    do { size_t jpull = (_len); \
	unsigned char * jbuf = (unsigned char *)(_buf); \
assert(jbuf == NULL || jpull < sizeof(_buf)); \
	if ((nb = rpmzqJobPull(zq, jbuf, jpull)) != jpull) \
	    bail((_why), zq->_zinp.fn); \
	b = jbuf; \
    } while (0);

#define	GET()	(nb--, *b++)
#define GET2()	(tmp2 = GET(), tmp2 + (GET() << 8))
#define GET4()	(tmp4 = GET2(), tmp4 + ((unsigned long)(GET2()) << 16))
#define	SKIP(_nskip) \
    do { size_t nskip = (_nskip); \
assert(nskip >= nb); \
	b += nskip, nb -= nskip; \
    } while (0);

/* convert MS-DOS date and time to a Unix time, assuming current timezone
   (you got a better idea?) */
static time_t dos2time(unsigned long dos)
	/*@*/
{
    struct tm tm;

    if (dos == 0)
	return time(NULL);
    tm.tm_year = ((int)(dos >> 25) & 0x7f) + 80;
    tm.tm_mon  = ((int)(dos >> 21) & 0xf) - 1;
    tm.tm_mday = (int)(dos >> 16) & 0x1f;
    tm.tm_hour = (int)(dos >> 11) & 0x1f;
    tm.tm_min  = (int)(dos >> 5) & 0x3f;
    tm.tm_sec  = (int)(dos << 1) & 0x3e;
    tm.tm_isdst = -1;           /* figure out if DST or not */
    return mktime(&tm);
}

/* convert an unsigned 32-bit integer to signed, even if long > 32 bits */
static long tolong(unsigned long val)
	/*@*/
{
    return (long)(val & 0x7fffffffUL) - (long)(val & 0x80000000UL);
}

#define LOW32 0xffffffffUL

/* process zip extra field to extract zip64 lengths and Unix mod time */
static int rpmzReadExtra(rpmzQueue zq, unsigned len, int save)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzh zh = zq->_zh;
    unsigned id;
    unsigned size;
    unsigned tmp2;
    unsigned long tmp4;
    unsigned char _b[64], *b = _b;
    size_t nb = 0;

    /* process extra blocks */
    while (len >= 4) {
	PULL(_b, 4, -1);
	id = GET2();
	size = GET2();
	len -= 4;
	if (size > len)
	    break;
	len -= size;
	if (id == 0x0001) {
	    /* Zip64 Extended Information Extra Field */
	    if (zh->zip_ulen == LOW32 && size >= 8) {
		PULL(_b, 8, -1);
		zh->zip_ulen = GET4();
		SKIP(4);
		size -= 8;
	    }
	    if (zh->zip_clen == LOW32 && size >= 8) {
		PULL(_b, 8, -1);
		zh->zip_clen = GET4();
		SKIP(4);
		size -= 8;
	    }
	}
	if (save) {
	    if ((id == 0x000d || id == 0x5855) && size >= 8) {
		/* PKWare Unix or Info-ZIP Type 1 Unix block */
		PULL(_b, 8, -1);
		SKIP(4);
		zh->stamp = tolong(GET4());
		size -= 8;
	    }
	    if (id == 0x5455 && size >= 5) {
		/* Extended Timestamp block */
		PULL(_b, 1, -1);
		size--;
		if (GET() & 1) {
		    PULL(_b, 4, -1);
		    zh->stamp = tolong(GET4());
		    size -= 4;
		}
	    }
	}
	PULL(NULL, size, -1);
    }
    PULL(NULL, len, -1);
    return 0;
}

/* read a gzip, zip, zlib, or lzw header from zq->ifdno and extract useful
   information, return the method -- or on error return negative: -1 is
   immediate EOF, -2 is not a recognized compressed format, -3 is premature EOF
   within the header, -4 is unexpected header flag values; a method of 256 is
   lzw -- set zq->format to indicate gzip, zlib, or zip */
static int rpmzGetHeader(rpmzQueue zq, int save)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzh zh = zq->_zh;
    unsigned magic;             /* magic header */
    int method;                 /* compression method */
    int flags;                  /* header flags */
    unsigned fname, extra;      /* name and extra field lengths */
    unsigned tmp2;              /* for macro */
    unsigned long tmp4;         /* for macro */
    char * next = NULL;
    unsigned char _b[64], *b = _b;
    size_t nb = 0;
    int rc;

    /* clear return information */
    if (save) {
	zh->stamp = 0;
	zh->hname = _free(zh->hname);
    }

    /* see if it's a gzip, zlib, or lzw file */
    zq->format = RPMZ_FORMAT_GZIP;

    PULL(_b, 1, -1);
    magic = GET() << 8;

if (_debug)
jobDebug(" start", zq->_zi.q->head);

    PULL(_b, 1, -2);
    magic += GET();
    if (magic % 31 == 0) {          /* it's zlib */
	zq->format = RPMZ_FORMAT_ZLIB;
	return (int)((magic >> 8) & 0xf);
    }
    if (magic == 0x1f9d)            /* it's lzw */
	return 256;
    if (magic == 0x504b) {          /* it's zip */
	PULL(_b, 4, -3);
	if (GET() != 3 || GET() != 4)
	    return -3;
	SKIP(2);
	PULL(_b, 24, -3);
	flags = GET2();
	if (flags & 0xfff0)
	    return -4;
	method = GET2();
	if (flags & 1)              /* encrypted */
	    method = 255;           /* mark as unknown method */
	if (save)
	    zh->stamp = dos2time(GET4());
	else
	    SKIP(4);
	zh->zip_crc = GET4();
	zh->zip_clen = GET4();
	zh->zip_ulen = GET4();
	fname = GET2();
	extra = GET2();

	next = xmalloc(fname + 1);
	if (rpmzqJobPull(zq, (unsigned char *)next, fname) != fname) {
	    next = _free(next);
	    return -3;
	}
	if (save) {
	    next[fname] = '\0';
/*@-mustfreeonly@*/
	    zh->hname = next;
/*@=mustfreeonly@*/
	} else
	    next = _free(next);
	rc = rpmzReadExtra(zq, extra, save);
	zq->format = (flags & 8) ? RPMZ_FORMAT_ZIP3 : RPMZ_FORMAT_ZIP2;
	return (rc == 0) ? method : -3;
    }
    if (magic != 0x1f8b)            /* not gzip */
	return -2;

    /* it's gzip -- get method and flags */
    PULL(_b, 8, -1);
    method = GET();
    flags = GET();
    if (flags & 0xe0)
	return -4;

    /* get time stamp */
    if (save)
	zh->stamp = tolong(GET4());
    else
	SKIP(4);

    /* skip extra field and OS */
    SKIP(2);

    /* skip extra field, if present */
    if (flags & 4) {
	PULL(_b, 2, -3);
	extra = GET2();
	PULL(NULL, extra, -3);
    }

    /* read file name, if present, into allocated memory */
    if (flags & 8) {
	size_t size = 128;
	size_t have = 0;
/*@-mustfreeonly@*/
	next = xmalloc(size+1);
/*@=mustfreeonly@*/
	do {
	    if (rpmzqJobPull(zq, (unsigned char *)next+have, 1) != 1) {
		next = _free(next);
		return -3;
	    }
	    if (next[have] == '\0')
		break;
	    have++;
	    if (have == size) {
		size += size;
		next = xrealloc(next, size);
	    }
	} while (1);
	next[have] = '\0';
	next = xrealloc(next, have+1);
    }
    if (save)
	zh->hname = next;
    else
	next = _free(next);

    /* skip comment */
    if (flags & 16)
	do {
	    PULL(_b, 1, -3);
	} while (*b != '\0');

    /* skip header crc */
    if (flags & 2)
	PULL(NULL, 2, -3);

    /* return compression method */
    return method;
}

/* --- list contents of compressed input (gzip, zlib, or lzw) */

/* find standard compressed file suffix, return length of suffix */
static size_t compressed_suffix(const char *fn)
	/*@*/
{
    size_t len;

    len = strlen(fn);
    if (len > 4) {
	fn += len - 4;
	len = 4;
	if (strcmp(fn, ".zip") == 0 || strcmp(fn, ".ZIP") == 0)
	    return 4;
    }
    if (len > 3) {
	fn += len - 3;
	len = 3;
	if (strcmp(fn, ".gz") == 0 || strcmp(fn, "-gz") == 0 ||
	    strcmp(fn, ".zz") == 0 || strcmp(fn, "-zz") == 0)
	    return 3;
    }
    if (len > 2) {
	fn += len - 2;
	if (strcmp(fn, ".z") == 0 || strcmp(fn, "-z") == 0 ||
	    strcmp(fn, "_z") == 0 || strcmp(fn, ".Z") == 0)
	    return 2;
    }
    return 0;
}

/* listing file name lengths for -l and -lv */
#define NAMEMAX1 48     /* name display limit at verbosity 1 */
#define NAMEMAX2 16     /* name display limit at verbosity 2 */

/* print gzip or lzw file information */
static void rpmzShowInfo(rpmzQueue zq, int method, unsigned long check, off_t len, int cont)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzh zh = zq->_zh;
    static int oneshot = 1; /* true if we need to print listing header */
    size_t max;             /* maximum name length for current verbosity */
    size_t n;               /* name length without suffix */
    time_t now;             /* for getting current year */
    char mod[64+1];         /* modification time in text */
    char name[NAMEMAX1+1];  /* header or file name, possibly truncated */

if (_debug)
jobDebug("  show", zq->_zi.q->head);

    /* create abbreviated name from header file name or actual file name */
    max = zq->verbosity > 1 ? NAMEMAX2 : NAMEMAX1;
    memset(name, 0, max + 1);
    if (cont)
	strncpy(name, "<...>", max + 1);
    else if (zh->hname == NULL) {
	n = strlen(zq->_zinp.fn) - compressed_suffix(zq->_zinp.fn);
	strncpy(name, zq->_zinp.fn, n > max + 1 ? max + 1 : n);
    }
    else
	strncpy(name, zh->hname, max + 1);
    if (name[max])
	strcpy(name + max - 3, "...");

    /* convert time stamp to text */
    if (zh->stamp) {
	strncpy(mod, ctime(&zh->stamp), sizeof(mod));
	mod[sizeof(mod)-1] = '\0';
	now = time(NULL);
/*@-aliasunique@*/
	if (strcmp(mod + 20, ctime(&now) + 20) != 0)
	    strcpy(mod + 11, mod + 19);
/*@=aliasunique@*/
    }
    else
	strcpy(mod + 4, "------ -----");
    mod[16] = 0;

    /* if first time, print header */
    if (oneshot) {
	if (zq->verbosity > 1)
	    fputs("method    check    timestamp    ", stdout);
	if (zq->verbosity > 0)
	    puts("compressed   original reduced  name");
	oneshot = 0;
    }

    /* print information */
    if (zq->verbosity > 1) {
	if (zq->format == RPMZ_FORMAT_ZIP3 && zq->mode == RPMZ_MODE_COMPRESS)
	    printf("zip%3d  --------  %s  ", method, mod + 4);
	else if (zq->format == RPMZ_FORMAT_ZIP2 || zq->format == RPMZ_FORMAT_ZIP3)
	    printf("zip%3d  %08lx  %s  ", method, check, mod + 4);
	else if (zq->format == RPMZ_FORMAT_ZLIB)
	    printf("zlib%2d  %08lx  %s  ", method, check, mod + 4);
	else if (method == 256)
	    printf("lzw     --------  %s  ", mod + 4);
	else
	    printf("gzip%2d  %08lx  %s  ", method, check, mod + 4);
    }
    if (zq->verbosity > 0) {
	if ((zq->format == RPMZ_FORMAT_ZIP3 && zq->mode == RPMZ_MODE_COMPRESS) ||
	    (method == 8 && zq->in_tot > (len + (len >> 10) + 12)) ||
	    (method == 256 && zq->in_tot > len + (len >> 1) + 3))
	    printf(sizeof(off_t) == 4 ? "%10lu %10lu?  unk    %s\n" :
                                        "%10llu %10llu?  unk    %s\n",
                   (unsigned long long)zq->in_tot, (unsigned long long)len, name);
	else
	    printf(sizeof(off_t) == 4 ? "%10lu %10lu %6.1f%%  %s\n" :
                                        "%10llu %10llu %6.1f%%  %s\n",
		    (unsigned long long)zq->in_tot, (unsigned long long)len,
		    len == 0 ? 0 : 100 * (len - zq->in_tot)/(double)len,
		    name);
    }
}

/* list content information about the gzip file at zq->ifdno (only works if the gzip
   file contains a single gzip stream with no junk at the end, and only works
   well if the uncompressed length is less than 4 GB) */
static void rpmzListInfo(rpmzQueue zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzJob job = zq->_zi.q->head;
    rpmzh zh = zq->_zh;
    rpmzf zinp = &zq->_zinp;
    int method;             /* rpmzGetHeader() return value */
    size_t n;               /* available trailer bytes */
    off_t at;               /* used to calculate compressed length */
    unsigned char tail[8];  /* trailer containing check and length */
    unsigned long check;    /* check value from trailer */
    unsigned long len;      /* check length from trailer */
    unsigned char * bufend;

if (_debug)
jobDebug("  list", zq->_zi.q->head);

    /* read header information and position input after header */
    method = rpmzGetHeader(zq, 1);
    if (method < 0) {
	zh->hname = _free(zh->hname);
	if (method != -1 && zq->verbosity > 1)
	    fprintf(stderr, "%s not a compressed file -- skipping\n", zq->_zinp.fn);
	return;
    }

    /* list zip file */
    if (zq->format == RPMZ_FORMAT_ZIP2 || zq->format == RPMZ_FORMAT_ZIP3) {
	zq->in_tot = zh->zip_clen;
	rpmzShowInfo(zq, method, zh->zip_crc, zh->zip_ulen, 0);
	return;
    }

    /* list zlib file */
    if (zq->format == RPMZ_FORMAT_ZLIB) {
	at = rpmzSeek(&zq->_zinp, 0, SEEK_END);
	if (at == -1) {
	    check = 0;
	    do {
		len = job->in->len < 4 ? job->in->len : 4;
		bufend = job->in->buf;
		bufend += job->in->len;
		bufend -= len;
		while (len--)
		    check = (check << 8) + *bufend++;
		job->in->buf = bufend;	/* XXX don't change job->in->buf?!? */
	    } while (load(zq) != 0);
	    check &= LOW32;
	}
	else {
	    zq->in_tot = at;
	    (void) rpmzSeek(&zq->_zinp, -4, SEEK_END);
	    rpmzRead(zinp, tail, 4);
	    check = (*tail << 24) + (tail[1] << 16) + (tail[2] << 8) + tail[3];
	}
	zq->in_tot -= 6;
	rpmzShowInfo(zq, method, check, 0, 0);
	return;
    }

    /* list lzw file */
    if (method == 256) {
	at = rpmzSeek(&zq->_zinp, 0, SEEK_END);
	if (at == -1)
	    while (load(zq) != 0)
		;
	else
	    zq->in_tot = at;
	zq->in_tot -= 3;
	rpmzShowInfo(zq, method, 0, 0, 0);
	return;
    }

    /* skip to end to get trailer (8 bytes), compute compressed length */
    if (job->more < 0) {
	/* whole thing already read */
	if (job->in->len < 8) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s not a valid gzip file -- skipping\n",
			zq->_zinp.fn);
	    return;
	}
	zq->in_tot = job->in->len - 8;     /* compressed size */
	bufend = job->in->buf;
	bufend += job->in->len;
	memcpy(tail, bufend - 8, 8);
    }
    else if ((at = rpmzSeek(&zq->_zinp, -8, SEEK_END)) != -1) {
	zq->in_tot = at - zq->in_tot + job->in->len; /* compressed size */
	rpmzRead(zinp, tail, 8);           /* get trailer */
    } else {                             /* can't seek */
	at = zq->in_tot - job->in->len;  /* save header size */
	do {
	    n = job->in->len < 8 ? job->in->len : 8;
	    bufend = job->in->buf;
	    bufend += job->in->len;
	    memcpy(tail, bufend - n, n);
	    load(zq);
	} while (job->in->len == zq->_in_buf_allocated);       /* read until end */
	if (job->in->len < 8) {
	    if (n + job->in->len < 8) {
		if (zq->verbosity > 0)
		    fprintf(stderr, "%s not a valid gzip file -- skipping\n",
				zq->_zinp.fn);
		return;
	    }
	    if (job->in->len) {
/*@-aliasunique@*/
		if (n + job->in->len > 8)
		    memcpy(tail, tail + n - (8 - job->in->len), 8 - job->in->len);
/*@=aliasunique@*/
		memcpy(tail + 8 - job->in->len, job->in->buf, job->in->len);
	    }
	}
	else {
	    bufend = job->in->buf;
	    bufend += job->in->len;
	    memcpy(tail, bufend - 8, 8);
	}
	zq->in_tot -= at + 8;
    }
    if (zq->in_tot < 2) {
	if (zq->verbosity > 0)
	    fprintf(stderr, "%s not a valid gzip file -- skipping\n", zq->_zinp.fn);
	return;
    }

    /* convert trailer to check and uncompressed length (modulo 2^32) */
    check = tail[0] + (tail[1] << 8) + (tail[2] << 16) + (tail[3] << 24);
    len = tail[4] + (tail[5] << 8) + (tail[6] << 16) + (tail[7] << 24);

    /* list information about contents */
    rpmzShowInfo(zq, method, check, len, 0);
    zh->hname = _free(zh->hname);
}

/* --- decompress deflate input --- */

/* call-back input function for inflateBack() */
static unsigned inb(void *_zq, unsigned char **buf)
	/*@globals fileSystem, internalState @*/
	/*@modifies _zq, *buf, fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;

    load(zq);

if (_debug)
jobDebug("  post", zq->_zi.q->head);

    *buf = zq->_zi.q->head->in->buf;
    return zq->_zi.q->head->in->len;
}

#ifndef	_PIGZNOTHREAD

/* output write thread */
static void outb_write(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzo zo = &zq->_zo;
    rpmzf zout = &zq->_zout;
    rpmzLog zlog = zq->zlog;
    size_t nwrote;

    Trace((zlog, "-- launched decompress write thread"));
    do {
	yarnPossess(zo->wstate);
	yarnWaitFor(zo->wstate, TO_BE, 1);

if (_debug)
jobDebug(" write", zo->q->head);
	nwrote = zo->q->head->out->len;
	if (nwrote && zq->mode == RPMZ_MODE_DECOMPRESS)
	    rpmzWrite(zout, zo->q->head->out->buf, zo->q->head->out->len);
	rpmzqDropSpace(zo->q->head->out);

	Trace((zlog, "-- decompress wrote %lu bytes", nwrote));
	yarnTwist(zo->wstate, TO, 0);
    } while (nwrote);
    Trace((zlog, "-- exited decompress write thread"));
}

/* output check thread */
static void outb_check(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzo zo = &zq->_zo;
    rpmzLog zlog = zq->zlog;
    size_t nchecked;

    Trace((zlog, "-- launched decompress check thread"));
    do {
	yarnPossess(zo->kstate);
	yarnWaitFor(zo->kstate, TO_BE, 1);

if (_debug)
jobDebug(" check", zo->q->head);
	nchecked = zo->q->head->out->len;
	if (nchecked > 0)
	    zo->q->head->check = CHECK(zo->q->head->check, zo->q->head->out->buf, nchecked);
	rpmzqDropSpace(zo->q->head->out);

	Trace((zlog, "-- decompress checked %lu bytes", nchecked));
	yarnTwist(zo->kstate, TO, 0);
    } while (nchecked);
    Trace((zlog, "-- exited decompress check thread"));
}

static void rpmzoWait(rpmzo zo)
	/*@modifies zo @*/
{
    /* wait for write and check threads to complete */
    yarnPossess(zo->kstate);
    yarnWaitFor(zo->kstate, TO_BE, 0);
    yarnPossess(zo->wstate);
    yarnWaitFor(zo->wstate, TO_BE, 0);
}

static void rpmzoFini(rpmzo zo, /*@null@*/ int *caughtp)
	/*@modifies zo, *caughtp @*/
{
    if (zo->_checker != NULL)
	zo->_checker = yarnJoin(zo->_checker);
    if (zo->_writer != NULL)
	zo->_writer = yarnJoin(zo->_writer);

    if (caughtp != NULL) {	/* XXX garbage collecting? */
	if (zo->kstate != NULL)
	    zo->kstate = yarnFreeLock(zo->kstate);
	if (zo->wstate != NULL)
	    zo->wstate = yarnFreeLock(zo->wstate);
	if (zo->q != NULL) {
#ifndef	DYING
	    if (zo->q->head != NULL) {
		zo->q->head->out = rpmzqDropSpace(zo->q->head->out);
		zo->q->head = rpmzqDropJob(zo->q->head);
	    }
#else
	    rpmzqVerifyFIFO(zo->q);
#endif
	    zo->q = rpmzqFiniFIFO(zo->q);
	}
	if (zo->pool != NULL)
	    zo->pool = rpmzqFreePool(zo->pool, caughtp);
	else
	    *caughtp = 0;
    }
}

static void rpmzoInit(rpmzQueue zq, rpmzo zo, off_t out_tot)
	/*@modifies zo @*/
{
    if (zo->pool == NULL) {
	/* inflateBack() window is a function of windowBits */
	size_t _out_len = (1 << infWBits);
	zo->pool = rpmzqNewPool(_out_len,  2);
    }
    if (zo->q == NULL)
	zo->q = rpmzqInitFIFO(0L);

    /* XXX spaces, not jobs, are currently queued by rpmzo */
    if (zo->q->head == NULL)
	zo->q->head = rpmzqNewJob(0);
    zo->q->head->check = CHECK(0L, Z_NULL, 0);

/*@-mustfreeonly@*/
    if (zo->wstate == NULL)
	zo->wstate = yarnNewLock(0);
    if (zo->_writer == NULL)
	zo->_writer = yarnLaunch(outb_write, zq);
    if (zo->kstate == NULL)
	zo->kstate = yarnNewLock(0);
    if (zo->_checker == NULL)
	zo->_checker = yarnLaunch(outb_check, zq);
    zo->out_tot = out_tot;
/*@=mustfreeonly@*/
}

static void rpmzoNext(rpmzo zo, unsigned char *buf, unsigned len)
	/*@modifies zo @*/
{
    rpmzFIFO qo = zo->q;

    /* wait for previous operation to complete */
    rpmzoWait(zo);

if (_debug)
jobDebug("  outb", qo->head);

    /* queue the output and alert the worker bees */
    qo->head->out = rpmzqNewSpace(zo->pool, zo->pool->size);
assert(qo->head->out->len >= len);
    qo->head->out->len = len;
    if (len > 0) {
	zo->out_tot += len;
assert(buf != NULL);
	/* XXX this memcpy cannot be avoided with inflateBack. */
/*@-mayaliasunique@*/
	memcpy(qo->head->out->buf, buf, len);
/*@=mayaliasunique@*/
    }
    rpmzqUseSpace(qo->head->out);	/* XXX nrefs++ for check. */

    yarnTwist(zo->wstate, TO, 1);
    yarnTwist(zo->kstate, TO, 1);

    /* if requested with len == 0, clean up. */
    if (len == 0) {
	rpmzoWait(zo);	/* wait for last buffer to be finished. */
	yarnRelease(zo->wstate);
	yarnRelease(zo->kstate);
	rpmzoFini(zo, NULL);
    }
}

#endif	/* _PIGZNOTHREAD */

/* call-back output function for inflateBack() -- wait for the last write and
   check calculation to complete, copy the write buffer, and then alert the
   write and check threads and return for more decompression while that's
   going on (or just write and check if no threads or if proc == 1) */
static int outb(void *_zq, /*@null@*/ unsigned char *buf, unsigned len)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _zq;
    rpmzo zo = &zq->_zo;
    rpmzf zout = &zq->_zout;

#ifndef _PIGZNOTHREAD

    if (zq->threads > 1) {
	/* if first time, initialize state and launch threads */
	if (zo->wstate == NULL) {
assert(zq->out_tot == 0);
	    rpmzoInit(zq, zo, zq->out_tot);
	}

	rpmzoNext(zo, buf, len);

	/* return for more decompression while last buffer is being written
	 * and having its check value calculated -- we wait for those to finish
	 * the next time this function is called */
    } else
#endif
    if (len) {		/* if no threads, then do it without threads */
	if (zq->mode == RPMZ_MODE_DECOMPRESS)
	    rpmzWrite(zout, buf, len);
	zo->q->head->check = CHECK(zo->q->head->check, buf, len);
    }

    if (len > 0)
	zq->out_tot += len;
assert(zq->out_tot == zo->out_tot);

    return 0;
}

/* inflate for decompression or testing -- decompress from zq->ifdno to zq->ofdno unless
   zq->mode != RPMZ_MODE_DECOMPRESS, in which case just test zq->ifdno, and then also list if zq->mode == RPMZ_MODE_LIST;
   look for and decode multiple, concatenated gzip and/or zlib streams;
   read and check the gzip, zlib, or zip trailer */
/*@-nullstate@*/
static void rpmzInflateCheck(rpmzQueue zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzh zh = zq->_zh;

    int ret;
    int cont;
    unsigned long check;
    unsigned long len;
    unsigned tmp2;
    unsigned long tmp4;
    off_t clen;
    unsigned char _b[64], *b = _b;
    size_t nb = 0;

if (_debug) {
fprintf(stderr, "\n");
jobDebug("  init", zq->_zi.q->head);
}

    cont = 0;
    do {
	/* header already read -- set up for decompression */

	zq->in_tot = zq->_zi.q->head->in->len;	/* track compressed data length */
	zq->_zi.in_tot = zq->_zi.q->head->in->len;	/* track compressed data length */

	zq->out_tot = 0;

    {	
	z_stream strm;
	/* inflateBack() window is a function of windowBits */
	size_t _out_len = (1 << infWBits);
	unsigned char * _out_buf = xmalloc(_out_len);

if (_debug)
jobDebug("before", zq->_zi.q->head);

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = inflateBackInit(&strm, infWBits, _out_buf);
	if (ret != Z_OK)
	    bail("not enough memory", "");

	/* decompress, compute lengths and check value */
	strm.avail_in = zq->_zi.q->head->in->len;
/*@-sharedtrans@*/
	strm.next_in = zq->_zi.q->head->in->buf;
/*@=sharedtrans@*/

	ret = inflateBack(&strm, inb, zq, outb, zq);
	if (ret != Z_STREAM_END)
	    bail("corrupted input -- invalid deflate data: ", zq->_zinp.fn);

	zq->_zi.q->head->in->len = strm.avail_in;
/*@-onlytrans@*/
	zq->_zi.q->head->in->buf = strm.next_in;
/*@=onlytrans@*/

/*@-noeffect@*/
	inflateBackEnd(&strm);
/*@=noeffect@*/
	outb(zq, NULL, 0);        /* finish off final write and check */

	_out_buf = _free(_out_buf);

if (_debug)
jobDebug(" after", zq->_zi.q->head);

    }

	/* compute compressed data length */
	clen = zq->in_tot - zq->_zi.q->head->in->len;

	/* read and check trailer */
	switch (zq->format) {
	case RPMZ_FORMAT_ZIP3:	/* data descriptor follows */
	    /* read original version of data descriptor*/
	    BPULL(_b, 12, "corrupted zip entry -- missing trailer: ");
	    zh->zip_crc = GET4();
	    zh->zip_clen = GET4();
	    zh->zip_ulen = GET4();

	    /* if crc doesn't match, try info-zip variant with sig */
	    if (zh->zip_crc != zq->_zo.q->head->check) {
		if (zh->zip_crc != 0x08074b50UL || zh->zip_clen != zq->_zo.q->head->check)
		    bail("corrupted zip entry -- crc32 mismatch: ", zq->_zinp.fn);
		zh->zip_crc = zh->zip_clen;
		zh->zip_clen = zh->zip_ulen;
		BPULL(_b, 4, "corrupted zip entry -- missing trailer: ");
		zh->zip_ulen = GET4();
	    }

	    /* if second length doesn't match, try 64-bit lengths */
	    if (zh->zip_ulen != (zq->out_tot & LOW32)) {
		BPULL(_b, 8, "corrupted zip entry -- missing trailer: ");
		zh->zip_ulen = GET4();
		(void) GET4();
	    }
	    /*@fallthrough@*/
	case RPMZ_FORMAT_ZIP2:
	    if (zh->zip_clen != (clen & LOW32) || zh->zip_ulen != (zq->out_tot & LOW32))
		bail("corrupted zip entry -- length mismatch: ", zq->_zinp.fn);
	    check = zh->zip_crc;
	    break;
	case RPMZ_FORMAT_ZLIB:	/* zlib (big-endian) trailer */
	    BPULL(_b, 4, "corrupted zlib stream -- missing trailer: ");
	    check = GET() << 24;
	    check += GET() << 16;
	    check += GET() << 8;
	    check += GET();
	    if (check != zq->_zo.q->head->check)
		bail("corrupted zlib stream -- adler32 mismatch: ", zq->_zinp.fn);
	    break;
	case RPMZ_FORMAT_GZIP:	/* gzip trailer */
	    BPULL(_b, 8, "corrupted gzip stream -- missing trailer: ");
	    check = GET4();
	    len = GET4();
	    if (check != zq->_zo.q->head->check)
		bail("corrupted gzip stream -- crc32 mismatch: ", zq->_zinp.fn);
	    if (len != (zq->out_tot & LOW32))
		bail("corrupted gzip stream -- length mismatch: ", zq->_zinp.fn);
	    break;
	default:
assert(0);
	    break;
	}

	/* show file information if requested */
	if (F_ISSET(zq->flags, LIST)) {
	    zq->in_tot = clen;
	    rpmzShowInfo(zq, 8, check, zq->out_tot, cont);
	    cont = 1;
	}

	/* if a gzip or zlib entry follows a gzip or zlib entry, decompress it
	 * (don't replace saved header information from first entry) */
    } while ((zq->format == RPMZ_FORMAT_GZIP || zq->format == RPMZ_FORMAT_ZLIB) && (ret = rpmzGetHeader(zq, 0)) == 8);
    if (ret != -1 && (zq->format == RPMZ_FORMAT_GZIP || zq->format == RPMZ_FORMAT_ZLIB))
	fprintf(stderr, "%s OK, has trailing junk which was ignored\n", zq->_zinp.fn);

    zq->_zi.q->head->in = rpmzqDropSpace(zq->_zi.q->head->in);
    zq->_zi.q->head = rpmzqDropJob(zq->_zi.q->head);
#ifdef	DYING
    zq->_zo.q->head->out = rpmzqDropSpace(zq->_zo.q->head->out);
#endif
    zq->_zo.q->head = rpmzqDropJob(zq->_zo.q->head);
if (_debug)
jobDebug("finish", zq->_zi.q->head);

}
/*@=nullstate@*/

/* --- decompress Unix compress (LZW) input --- */

/* throw out what's left in the current bits byte buffer (this is a vestigial
   aspect of the compressed data format derived from an implementation that
   made use of a special VAX machine instruction!) */
#define FLUSHCODE() \
    do { \
	left = 0; \
	rem = 0; \
	if (chunk > job->in->len) { \
	    chunk -= job->in->len; \
	    if (load(zq) == 0) \
		break; \
	    if (chunk > job->in->len) { \
		chunk = job->in->len = 0; \
		break; \
	    } \
	} \
	job->in->len -= chunk; \
	{   unsigned char * _buf = job->in->buf; \
	    _buf += job->in->len; \
	    job->in->buf = _buf; /* XXX don't change job->in->buf?!? */ \
	} \
	chunk = 0; \
    } while (0)

/* Decompress a compress (LZW) file from zq->ifdno to zq->ofdno.  The compress magic
   header (two bytes) has already been read and verified. */
static void rpmzDecompressLZW(rpmzQueue zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, fileSystem, internalState @*/
{
    rpmzf zout = &zq->_zout;

    rpmzJob job = zq->_zi.q->head;
    /* XXX LZW _out_buf scales with windowBits like inflateBack()? */
    size_t _out_len = (1 << infWBits);
    unsigned char * _out_buf = xmalloc(_out_len);

    /* --- memory for rpmzDecompressLZW()
     * the first 256 entries of prefix[] and suffix[] are never used, could
     * have offset the index, but it's faster to waste the memory
     */
    unsigned short _prefix[65536];	/* index to LZW prefix string */
    unsigned char _suffix[65536];	/* one-character LZW suffix */
    unsigned char _match[65280 + 2];	/* buffer for reversed match */

    int got;                    /* byte just read by GET() */
    unsigned int chunk;         /* bytes left in current chunk */
    int left;                   /* bits left in rem */
    unsigned rem;               /* unused bits from input */
    int bits;                   /* current bits per code */
    unsigned code;              /* code, table traversal index */
    unsigned mask;              /* mask for current bits codes */
    int max;                    /* maximum bits per code for this stream */
    int flags;                  /* compress flags, then block compress flag */
    unsigned end;               /* last valid entry in prefix/suffix tables */
    unsigned temp;              /* current code */
    unsigned prev;              /* previous code */
    unsigned final;             /* last character written for previous code */
    unsigned stack;             /* next position for reversed string */
    unsigned outcnt;            /* bytes in output buffer */
    unsigned char *p;
    unsigned char _b[64], *b = _b;
    size_t nb = 0;

assert(job != NULL);
assert(job->out == NULL);

    /* process remainder of compress header -- a flags byte */
    zq->out_tot = 0;

    BPULL(_b, 1, "missing lzw data: ");

    flags = GET();
    if (flags & 0x60)
	bail("unknown lzw flags set: ", zq->_zinp.fn);
    max = flags & 0x1f;
    if (max < 9 || max > 16)
	bail("lzw bits out of range: ", zq->_zinp.fn);
    if (max == 9)                           /* 9 doesn't really mean 9 */
	max = 10;
    flags &= 0x80;                          /* true if block compress */

    /* clear table */
    bits = 9;
    mask = 0x1ff;
    end = flags ? 256 : 255;

    /* set up: get first 9-bit code, which is the first decompressed byte, but
       don't create a table entry until the next code */
    nb = rpmzqJobPull(zq, _b, 2);
    if (nb == 0)                            /* no compressed data is ok */
	goto exit;
    b = _b;
    got = GET();
    final = prev = (unsigned)got;           /* low 8 bits of code */
    if (nb == 0 && (got & 1) != 0)          /* missing a bit or code >= 256 */
	bail("invalid lzw code: ", zq->_zinp.fn);
    got = GET();
    rem = (unsigned)got >> 1;               /* remaining 7 bits */
    left = 7;
    chunk = bits - 2;                       /* 7 bytes left in this chunk */
    _out_buf[0] = (unsigned char)final; /* write first decompressed byte */
    outcnt = 1;

    /* decode codes */
    stack = 0;
    for (;;) {
	/* if the table will be full after this, increment the code size */
	if (end >= mask && bits < max) {
	    FLUSHCODE();
	    bits++;
	    mask <<= 1;
	    mask++;
	}

	/* get a code of length bits */
	if (chunk == 0)                     /* decrement chunk modulo bits */
	    chunk = bits;
	code = rem;                         /* low bits of code */

	nb = rpmzqJobPull(zq, _b, 1);
	if (nb != 1) {                      /* EOF is end of compressed data */
	    /* write remaining buffered output */
	    zq->out_tot += outcnt;
	    if (outcnt && zq->mode == RPMZ_MODE_DECOMPRESS)
		rpmzWrite(zout, _out_buf, outcnt);
	    goto exit;
	}
	b = _b;

	got = GET();
	code += (unsigned)got << left;      /* middle (or high) bits of code */
	left += 8;
	chunk--;
	if (bits > left) {                  /* need more bits */
	    BPULL(_b, 1, "invalid lzw code: ");
	    got = GET();
	    code += (unsigned)got << left;  /* high bits of code */
	    left += 8;
	    chunk--;
	}
	code &= mask;                       /* mask to current code length */
	left -= bits;                       /* number of unused bits */
	rem = (unsigned)got >> (8 - left);  /* unused bits from last byte */

	/* process clear code (256) */
	if (code == 256 && flags) {
	    FLUSHCODE();
	    bits = 9;                       /* initialize bits and mask */
	    mask = 0x1ff;
	    end = 255;                      /* empty table */
	    continue;                       /* get next code */
	}

	/* special code to reuse last match */
	temp = code;                        /* save the current code */
	if (code > end) {
	    /*
	     * Be picky on the allowed code here, and make sure that the code
	     * we drop through (prev) will be a valid index so that random
	     * input does not cause an exception.  The code != end + 1 check is
	     * empirically derived, and not checked in the original uncompress
	     * code.  If this ever causes a problem, that check could be safely
	     * removed.  Leaving this check in greatly improves pigz's ability
	     * to detect random or corrupted input after a compress header.
	     * In any case, the prev > end check must be retained.
	     */
	    if (code != end + 1 || prev > end)
		bail("invalid lzw code: ", zq->_zinp.fn);
	    _match[stack++] = (unsigned char)final;
	    code = prev;
	}

	/* walk through linked list to generate output in reverse order */
	p = _match + stack;
	while (code >= 256) {
	    *p++ = _suffix[code];
	    code = _prefix[code];
	}
	stack = p - _match;
	_match[stack++] = (unsigned char)code;
	final = code;

	/* link new table entry */
	if (end < mask) {
	    end++;
	    _prefix[end] = (unsigned short)prev;
	    _suffix[end] = (unsigned char)final;
	}

	/* set previous code for next iteration */
	prev = temp;

	/* write output in forward order */
	while (stack > _out_len - outcnt) {
	    while (outcnt < _out_len)
		_out_buf[outcnt++] = _match[--stack];
	    zq->out_tot += outcnt;
	    if (zq->mode == RPMZ_MODE_DECOMPRESS)
		rpmzWrite(zout, _out_buf, outcnt);
	    outcnt = 0;
	}
	p = _match + stack;
	do {
	    _out_buf[outcnt++] = *--p;
	} while (p > _match);
	stack = 0;

	/* loop for next code with final and prev as the last match, rem and
	 * left provide the first 0..7 bits of the next code, end is the last
	 * valid table entry */
    }

exit:
    _out_buf = _free(_out_buf);

}

/* --- file processing --- */

/* extract file name from path */
/*@observer@*/
static const char *justname(const char *path)
	/*@*/
{
    const char * p = path + strlen(path);

    while (--p >= path)
	if (*p == '/')
	    break;
    return p + 1;
}

/* Copy file attributes, from -> to, as best we can.  This is best effort, so
   no errors are reported.  The mode bits, including suid, sgid, and the sticky
   bit are copied (if allowed), the owner's user id and group id are copied
   (again if allowed), and the access and modify times are copied. */
static void copymeta(const char *from, const char *to)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    struct stat sb, *st = &sb;
    struct timeval times[2];
    int xx;

    /* get all of from's Unix meta data, return if not a regular file */
    if (Stat(from, st) != 0 || !S_ISREG(st->st_mode))
	return;

    /* set to's mode bits, ignore errors */
    xx = Chmod(to, st->st_mode & 07777);

    /* copy owner's user and group, ignore errors */
    xx = Chown(to, st->st_uid, st->st_gid);

    /* copy access and modify times, ignore errors */
    times[0].tv_sec = st->st_atime;
    times[0].tv_usec = 0;
    times[1].tv_sec = st->st_mtime;
    times[1].tv_usec = 0;
    xx = Utimes(to, times);
}

/* set the access and modify times of fd to t */
static void touch(const char *path, time_t t)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    struct timeval times[2];
    int xx;

    times[0].tv_sec = t;
    times[0].tv_usec = 0;
    times[1].tv_sec = t;
    times[1].tv_usec = 0;
    xx = Utimes(path, times);
}

/* process provided input file, or stdin if path is NULL -- rpmzProcess() can
   call itself for recursive directory processing */
static void rpmzProcess(rpmz z, /*@null@*/ const char *path)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies z, fileSystem, internalState @*/
{
    rpmzQueue zq = z->zq;
    rpmzh zh = zq->_zh;
    int method = -1;                /* rpmzGetHeader() return value */
    size_t len;                     /* length of base name (minus suffix) */
    struct stat sb, *st = &sb;      /* to get file type and mod time */

    /* open input file with name in, descriptor zq->ifdno -- set name and mtime */
    if (path == NULL) {
	strncpy(z->_ifn, z->stdin_fn, sizeof(z->_ifn));
	z->_ifn[sizeof(z->_ifn)-1] = '\0';
	zq->_zinp.fdno = STDIN_FILENO;
/*@-mustfreeonly@*/
	zh->name = NULL;
/*@=mustfreeonly@*/
	zh->mtime = F_ISSET(zq->flags, HTIME) ?
		(fstat(zq->_zinp.fdno, st) ? time(NULL) : st->st_mtime) : 0;
	len = 0;
    } else {
	/* set input file name (already set if recursed here) */
	if (path != zq->_zinp.fn) {
/*@-mayaliasunique@*/
	    strncpy(z->_ifn, path, sizeof(z->_ifn));
	    z->_ifn[sizeof(z->_ifn)-1] = '\0';
/*@=mayaliasunique@*/
	    if (z->_ifn[sizeof(z->_ifn) - 1])
		bail("name too long: ", path);
	}
	len = strlen(zq->_zinp.fn);

	/* only process regular files, but allow symbolic links if -f,
	 * recurse into directory if -r */
	if (Lstat(zq->_zinp.fn, st)) {
#ifdef EOVERFLOW
	    if (errno == EOVERFLOW || errno == EFBIG)
		bail(zq->_zinp.fn, " too large -- pigz not compiled with large file support");
#endif
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s does not exist -- skipping\n", zq->_zinp.fn);
	    return;
	}
	if (!S_ISREG(st->st_mode) && !S_ISLNK(st->st_mode) && !S_ISDIR(st->st_mode)) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s is a special file or device -- skipping\n", zq->_zinp.fn);
	    return;
	}
	if (S_ISLNK(st->st_mode) && !F_ISSET(zq->flags, SYMLINKS)) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s is a symbolic link -- skipping\n", zq->_zinp.fn);
	    return;
	}

	/* Recurse into directory */
	if (S_ISDIR(st->st_mode)) {
	    struct dirent * dp;
	    DIR * dir;
	    ARGV_t av = NULL;
	    char * te;
	    int xx;
	    int i;

	    if (!F_ISSET(zq->flags, RECURSE)) {
		if (zq->verbosity > 0)
		    fprintf(stderr, "%s is a directory -- skipping\n", zq->_zinp.fn);
		return;
	    }
	    te = z->_ifn + len;
	    if ((dir = Opendir(zq->_zinp.fn)) == NULL)
		return;
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
		(void) rpmzProcess(z, zq->_zinp.fn);
		*te = '\0';
	    }
	    av = argvFree(av);
	    return;
	}

	/* don't compress .gz (or provided suffix) files, unless -f */
	if (!(F_ISSET(zq->flags, ALREADY) || F_ISSET(zq->flags, LIST) || zq->mode != RPMZ_MODE_COMPRESS) && len >= strlen(zq->suffix) &&
		strcmp(zq->_zinp.fn + len - strlen(zq->suffix), zq->suffix) == 0) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s ends with %s -- skipping\n", zq->_zinp.fn, zq->suffix);
	    return;
	}

	/* only decompress or list files with compressed suffix */
	if (F_ISSET(zq->flags, LIST) || zq->mode != RPMZ_MODE_COMPRESS) {
	    int suf = compressed_suffix(zq->_zinp.fn);
	    if (suf == 0) {
		if (zq->verbosity > 0)
		    fprintf(stderr, "%s does not have compressed suffix -- skipping\n",
                            zq->_zinp.fn);
		return;
	    }
	    len -= suf;
	}

	/* open input file */
	zq->_zinp.fdno = open(zq->_zinp.fn, O_RDONLY, 0);
	if (zq->_zinp.fdno < 0)
	    bail("read error on ", zq->_zinp.fn);

	/* prepare gzip header information for compression */
/*@-mustfreeonly@*/
	zh->name = F_ISSET(zq->flags, HNAME) ? justname(zq->_zinp.fn) : NULL;
/*@=mustfreeonly@*/
	zh->mtime = F_ISSET(zq->flags, HTIME) ? st->st_mtime : 0;
    }
    SET_BINARY_MODE(zq->_zinp.fdno);

    /* if decoding or testing, try to read gzip header */
assert(zh->hname == NULL);
    zh->hname = _free(zh->hname);
    if (zq->mode != RPMZ_MODE_COMPRESS) {
	method = rpmzGetHeader(zq, 1);
	if (method != 8 && method != 256) {
	    if (method != -1 && zq->verbosity > 0)
		fprintf(stderr,
		    method < 0 ? "%s is not compressed -- skipping\n" :
			"%s has unknown compression method -- skipping\n",
			zq->_zinp.fn);
	    goto exit;
	}

	/* if requested, test input file (possibly a special list) */
	if (zq->mode == RPMZ_MODE_TEST) {
	    if (method == 8)
		rpmzInflateCheck(zq);
	    else {
		rpmzDecompressLZW(zq);
		if (F_ISSET(zq->flags, LIST)) {
		    zq->in_tot -= 3;
		    rpmzShowInfo(zq, method, 0, zq->out_tot, 0);
		}
	    }
	    goto exit;
	}
    }

    /* if requested, just list information about input file */
    if (F_ISSET(zq->flags, LIST)) {
	rpmzListInfo(zq);
	goto exit;
    }

    /* create output file out, descriptor zq->ofdno */
    if (path == NULL || F_ISSET(zq->flags, STDOUT)) {
	/* write to stdout */
/*@-mustfreeonly@*/
	zq->_zout.fn = xstrdup(z->stdout_fn);
/*@=mustfreeonly@*/
	zq->_zout.fdno = STDOUT_FILENO;
	if (zq->mode == RPMZ_MODE_COMPRESS && !F_ISSET(zq->flags, TTY) && isatty(zq->_zout.fdno))
	    bail("trying to write compressed data to a terminal",
		" (use -f to force)");
    } else {
	const char *to;

	/* use header name for output when decompressing with -N */
	to = zq->_zinp.fn;
	if (zq->mode != RPMZ_MODE_COMPRESS && F_ISSET(zq->flags, HNAME) && zh->hname != NULL) {
	    to = zh->hname;
	    len = strlen(zh->hname);
	}

	/* create output file and open to write */
	{   size_t nb = len +
		(zq->mode != RPMZ_MODE_COMPRESS ? 0 : strlen(zq->suffix)) + 1;
	    char * t = xmalloc(nb);
	    memcpy(t, to, len);
	    strcpy(t + len, zq->mode != RPMZ_MODE_COMPRESS ? "" : zq->suffix);
	    zq->_zout.fn = t;
	}
	zq->_zout.fdno = open(zq->_zout.fn, O_CREAT | O_TRUNC | O_WRONLY |
                         (F_ISSET(zq->flags, OVERWRITE) ? 0 : O_EXCL), 0666);

	/* if exists and not -f, give user a chance to overwrite */
	if (zq->_zout.fdno < 0 && errno == EEXIST && isatty(0) && zq->verbosity) {
	    int ch, reply;

	    fprintf(stderr, "%s exists -- overwrite (y/n)? ", zq->_zout.fn);
	    fflush(stderr);
	    reply = -1;
	    do {
		ch = getchar();
		if (reply < 0 && ch != ' ' && ch != '\t')
		    reply = ch == 'y' || ch == 'Y' ? 1 : 0;
	    } while (ch != EOF && ch != '\n' && ch != '\r');
	    if (reply == 1)
		zq->_zout.fdno = open(zq->_zout.fn, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	}

	/* if exists and no overwrite, report and go on to next */
	if (zq->_zout.fdno < 0 && errno == EEXIST) {
	    if (zq->verbosity > 0)
		fprintf(stderr, "%s exists -- skipping\n", zq->_zout.fn);
	    goto exit;
	}

	/* if some other error, give up */
	if (zq->_zout.fdno < 0)
	    bail("write error on ", zq->_zout.fn);
    }
    SET_BINARY_MODE(zq->ofdno);
    zh->hname = _free(zh->hname);

    /* process zq->ifdno to zq->ofdno */
    if (zq->verbosity > 1)
	fprintf(stderr, "%s to %s ", zq->_zinp.fn, zq->_zout.fn);
    if (zq->mode != RPMZ_MODE_COMPRESS) {
	if (method == 8)
	    rpmzInflateCheck(zq);
	else
	    rpmzDecompressLZW(zq);
    }
#ifndef _PIGZNOTHREAD
    else if (zq->threads > 1)
	rpmzParallelCompress(zq);
#endif
    else
	rpmzSingleCompress(zq, 0);
    if (zq->verbosity > 1) {
	putc('\n', stderr);
	fflush(stderr);
    }

    /* finish up, copy attributes, set times, delete original */
    if (zq->_zout.fdno != STDOUT_FILENO) {
	if (rpmzClose(&zq->_zout))
	    bail("write error on ", zq->_zout.fn);
	zq->_zout.fdno = -1;	/* now prevent deletion on interrupt */
	if (zq->_zinp.fdno != STDIN_FILENO) {
	    copymeta(zq->_zinp.fn, zq->_zout.fn);
	    if (!F_ISSET(zq->flags, KEEP))
		Unlink(zq->_zinp.fn);
	}
	if (zq->mode != RPMZ_MODE_COMPRESS && F_ISSET(zq->flags, HTIME) && zh->stamp)
	    touch(zq->_zout.fn, zh->stamp);
    }

exit:
    zh->hname = _free(zh->hname);
    if (zq->_zinp.fdno > STDIN_FILENO) {
	(void) rpmzClose(&zq->_zinp);
	zq->_zinp.fdno = -1;
    }
    zq->_zout.fdno = -1;
    zq->_zout.fn = _free(zq->_zout.fn);
    return;
}

/*==============================================================*/

/* catch termination signal */
/*@exits@*/
static void rpmzAbort(/*@unused@*/ int sig)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/
{
    rpmzQueue zq = _rpmz->zq;

    Trace((zq->zlog, "termination by user"));
    if (zq->_zout.fdno != -1 && zq->_zout.fn != NULL)
	Unlink(zq->_zout.fn);
    zq->zlog = rpmzLogDump(zq->zlog, NULL);
    _exit(EXIT_FAILURE);
}

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

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'L':
	fputs(PIGZVERSION, stderr);
	fputs("Copyright (C) 2007 Mark Adler\n", stderr);
	fputs("Subject to the terms of the zlib license.\n", stderr);
	fputs("No warranty is provided or implied.\n", stderr);
	exit(EXIT_SUCCESS);
	/*@notreached@*/ break;
    case 'V':	fputs(PIGZVERSION, stderr); exit(EXIT_SUCCESS);
    case 'Z':	bail("invalid option: LZW output not supported", "");
    case 'a':	bail("invalid option: ascii conversion not supported", "");
    case 'b':
	zq->blocksize = (size_t)(atol(arg)) << 10;   /* chunk size */
	if (zq->blocksize < _PIGZDICT)
	    bail("block size too small (must be >= 32K)", "");
	if (zq->blocksize + (zq->blocksize >> 11) + 10 < (zq->blocksize >> 11) + 10 ||
	    (ssize_t)(zq->blocksize + (zq->blocksize >> 11) + 10) < 0)
	    bail("block size too large", "");
	break;
    case 'p':
	zq->threads = atoi(arg);                  /* # processes */
	if (zq->threads < 1)
	    bail("need at least one process", "");
	if ((2 + (zq->threads << 1)) < 1)
	    bail("too many processes", "");
#ifdef _PIGZNOTHREAD
	if (zq->threads > 1)
	    bail("this pigz compiled without threads", "");
#endif
	break;
    case 'q':	zq->verbosity = 0; break;
/*@-noeffect@*/
    case 'v':	zq->verbosity++; break;
/*@=noeffect@*/
    default:
	/* XXX really need to display longName/shortName instead. */
	fprintf(stderr, _("Unknown option -%c\n"), (char)opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

/*==============================================================*/
#ifdef	REFERENCE
Usage: pigz [options] [files ...]
  will compress files in place, adding the suffix '.gz'.  If no files are
  specified, stdin will be compressed to stdout.  pigz does what gzip does,
  but spreads the work over multiple processors and cores when compressing.

Options:
  -0 to -9, --fast, --best   Compression levels, --fast is -1, --best is -9
  -b, --blocksize mmm  Set compression block size to mmmK (default 128K)
  -p, --processes n    Allow up to n compression threads (default 8)
  -i, --independent    Compress blocks independently for damage recovery
  -R, --rsyncable      Input-determined block locations for rsync
  -d, --decompress     Decompress the compressed input
  -t, --test           Test the integrity of the compressed input
  -l, --list           List the contents of the compressed input
  -f, --force          Force overwrite, compress .gz, links, and to terminal
  -r, --recursive      Process the contents of all subdirectories
  -s, --suffix .sss    Use suffix .sss instead of .gz (for compression)
  -z, --zlib           Compress to zlib (.zz) instead of gzip format
  -K, --zip            Compress to PKWare zip (.zip) single entry format
  -k, --keep           Do not delete original file after processing
  -c, --stdout         Write all processed output to stdout (wont delete)
  -N, --name           Store/restore file name and mod time in/from header
  -n, --no-name        Do not store or restore file name in/from header
  -T, --no-time        Do not store or restore mod time in/from header
  -q, --quiet          Print no messages, even on error
  -v, --verbose        Provide more verbose output
#endif

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

	/* XXX POPT_ARG_ARGV portability. */
  { "files", '\0', POPT_ARG_ARGV,		&__rpmz.manifests, 0,
	N_("Read file names from MANIFEST"), N_("MANIFEST") },
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
  rpmpigz will compress files in place, adding the suffix '.gz'. If no files are\n\
  specified, stdin will be compressed to stdout.  rpmpigz does what gzip does,\n\
  but spreads the work over multiple processors and cores when compressing.\n\
\n\
Options:\
"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmzqOptionsPoptTable, 0,
	N_("Compression options: "), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

#ifdef	DYING
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage: rpmpigz [options] [files ...]\n\
  rpmpigz will compress files in place, adding the suffix '.gz'. If no files are\n\
  specified, stdin will be compressed to stdout.  rpmpigz does what gzip does,\n\
  but spreads the work over multiple processors and cores when compressing.\n\
"), NULL },
#endif

  POPT_TABLEEND
};

/* Process arguments, compress in the gzip format.  Note that z->threads must be at
   least two in order to provide a dictionary in one work unit for the other
   work unit, and that size must be at least 32K to store a full dictionary. */
int main(int argc, char **argv)
	/*@globals _rpmz, rpmzqOptionsPoptTable, __assert_program_name, yarnPrefix, h_errno,
		rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies _rpmz, rpmzqOptionsPoptTable, __assert_program_name, yarnPrefix,
		rpmGlobalMacroContext, fileSystem, internalState @*/
{
    rpmz z = _rpmz;
    rpmzQueue zq = _rpmzq;
    poptContext optCon = NULL;
    int ac;
    int rc = 1;		/* assume failure */
    int xx;
    int i;

/*@-observertrans -readonlytrans @*/
    __progname = "rpmpigz";
/*@=observertrans =readonlytrans @*/
    z->zq = zq;		/* XXX initialize rpmzq */

    /* XXX sick hack to initialize the popt callback. */
/*@-castfcnptr@*/
    rpmzqOptionsPoptTable[0].arg = (void *)&rpmzqArgCallback;
/*@=castfcnptr@*/

    /* prepare for interrupts and logging */
    signal(SIGINT, rpmzAbort);

#ifndef _PIGZNOTHREAD
    yarnPrefix = __progname;	/* prefix for yarn error messages */
    yarnAbort = rpmzAbort;	/* call on thread error */
#endif
    /* XXX add POPT_ARG_TIMEOFDAY oneshot? */
    gettimeofday(&zq->start, NULL);  /* starting time for log entries */
#if defined(DEBUG) || defined(__LCLINT__)
    zq->zlog = rpmzLogNew(&zq->start);/* initialize logging */
#endif

    zq->_zh = xcalloc(1, sizeof(*zq->_zh));	/* XXX do lazily */
    zq->_in_buf_allocated = IN_BUF_ALLOCATED;

    /* set all options to defaults */
    rpmzDefaults(zq);

    /* process user environment variable defaults */
    if (rpmzParseEnv(z, "GZIP", optionsTable))
	goto exit;

    optCon = rpmioInit(argc, argv, optionsTable);

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
    /* if no command line arguments and stdout is a terminal, show help */
    if ((z->argv == NULL || z->argv[0] == NULL) && isatty(STDOUT_FILENO)) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    /* process command-line arguments */
    if (z->argv == NULL || z->argv[0] == NULL) {
	/* list stdin or compress stdin to stdout if no file names provided */
	rpmzProcess(z, NULL);
    } else {
	for (i = 0; i < ac; i++) {
	    if (i == 1 && F_ISSET(zq->flags, STDOUT) && zq->mode == RPMZ_MODE_COMPRESS && !F_ISSET(zq->flags, LIST) && (zq->format == RPMZ_FORMAT_ZIP2 || zq->format == RPMZ_FORMAT_ZIP3)) {
		fprintf(stderr, "warning: output is concatenated zip files ");
		fprintf(stderr, "-- %s will not be able to extract\n", __progname);
	    }
	    rpmzProcess(z, strcmp(z->argv[i], "-") ? (char *)z->argv[i] : NULL);
	}
    }

    rc = 0;

exit:
    /* done -- release resources, show log */
    {	rpmzLog zlog = zq->zlog;
	int caught;
	caught = 0;
	rpmziFini(&zq->_zi, &caught);
	if (caught > 0)
	    Trace((zlog, "-- freed %d input buffers", caught));
	caught = 0;
	rpmzoFini(&zq->_zo, &caught);
	if (caught > 0)
	    Trace((zlog, "-- freed %d output buffers", caught));
    }
    zq->_zh = _free(zq->_zh);

    rpmzSingleCompress(zq, 1);
#ifndef _PIGZNOTHREAD
    {	rpmzLog zlog = zq->zlog;
	int caught;

	caught = 0;
	(void) rpmzcFini(&zq->_zc, &caught);
	if (caught)
	    Trace((zlog, "-- freed %d input buffers", caught));
	caught = 0;
	(void) rpmzwFini(&zq->_zw, &caught);
	if (caught)
	    Trace((zlog, "-- freed %d output buffers", caught));
    }
#endif	/* _PIGZNOTHREAD */

    z->zq->zlog = rpmzLogDump(z->zq->zlog, NULL);

    z->manifests = argvFree(z->manifests);
/*@-nullstate@*/
    z->argv = argvFree(z->argv);
/*@=nullstate@*/

    optCon = rpmioFini(optCon);
    
/*@-observertrans@*/	/* XXX __progname silliness */
    return rc;
/*@=observertrans@*/
}
