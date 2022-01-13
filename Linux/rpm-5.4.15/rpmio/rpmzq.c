/** \ingroup rpmio
 * \file rpmio/rpmzq.c
 * Job queue and buffer management (originally swiped from PIGZ).
 */

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

#include "system.h"

#if defined(WITH_BZIP2)

#include <rpmiotypes.h>
#include <rpmlog.h>

#define	_RPMBZ_INTERNAL
#include "rpmbz.h"

/*@access rpmbz @*/

#include "yarn.h"

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

#define	_RPMZQ_INTERNAL
#include "rpmzq.h"

/*@access rpmzSpace @*/
/*@access rpmzPool @*/
/*@access rpmzQueue @*/
/*@access rpmzJob @*/

#include "debug.h"

#ifdef __cplusplus
GENfree(rpmzJob)
GENfree(rpmzPool)
GENfree(rpmzSpace)
GENfree(rpmzFIFO)
GENfree(rpmzSEQ)
#endif	/* __cplusplus */

/*@unchecked@*/
int _rpmzq_debug = 0;

/*@unchecked@*/
static struct rpmzQueue_s __zq;

/*@unchecked@*/
rpmzQueue _rpmzq = &__zq;

/*==============================================================*/
/**
 */
static void _rpmzqArgCallback(poptContext con,
		/*@unused@*/ enum poptCallbackReason reason,
		const struct poptOption * opt, /*@unused@*/ const char * arg,
		/*@unused@*/ void * data)
	/*@globals _rpmzq, fileSystem, internalState @*/
	/*@modifies _rpmzq, fileSystem, internalState @*/
{
    rpmzQueue zq = _rpmzq;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
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

#ifdef	REFERENCE
Parallel BZIP2 v1.0.5 - by: Jeff Gilchrist http://compression.ca
[Jan. 08, 2009]             (uses libbzip2 by Julian Seward)

Usage: ./pbzip2 [-1 .. -9] [-b#cdfhklp#qrtVz] <filename> <filename2> <filenameN>
 -b#      : where # is the file block size in 100k (default 9 = 900k)
 -c       : output to standard out (stdout)
 -d       : decompress file
 -f       : force, overwrite existing output file
 -h       : print this help message
 -k       : keep input file, dont delete
 -l       : load average determines max number processors to use
 -p#      : where # is the number of processors (default 2)
 -q       : quiet mode (default)
 -r       : read entire input file into RAM and split between processors
 -t       : test compressed file integrity
 -v       : verbose mode
 -V       : display version info for pbzip2 then exit
 -z       : compress file (default)
 -1 .. -9 : set BWT block size to 100k .. 900k (default 900k)

Example: pbzip2 -b15vk myfile.tar
Example: pbzip2 -p4 -r -5 myfile.tar second*.txt
Example: tar cf myfile.tar.bz2 --use-compress-prog=pbzip2 dir_to_compress/
Example: pbzip2 -d myfile.tar.bz2

#endif

/*@unchecked@*/ /*@observer@*/
struct poptOption rpmzqOptionsPoptTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	(void *)_rpmzqArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "fast", '\0', POPT_ARG_VAL,				&__zq.level,  1,
	N_("fast compression"), NULL },
  { "best", '\0', POPT_ARG_VAL,				&__zq.level,  9,
	N_("best compression"), NULL },
  { "extreme", 'e', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE|POPT_ARGFLAG_DOC_HIDDEN,
	&__zq.flags,  RPMZ_FLAGS_EXTREME,
	N_("extreme compression"), NULL },
  { NULL, '0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  0,
	NULL, NULL },
  { NULL, '1', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  1,
	NULL, NULL },
  { NULL, '2', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  2,
	NULL, NULL },
  { NULL, '3', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  3,
	NULL, NULL },
  { NULL, '4', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  4,
	NULL, NULL },
  { NULL, '5', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  5,
	NULL, NULL },
  { NULL, '6', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  6,
	NULL, NULL },
  { NULL, '7', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  7,
	NULL, NULL },
  { NULL, '8', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  8,
	NULL, NULL },
  { NULL, '9', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.level,  9,
	NULL, NULL },

#ifdef	NOTYET	/* XXX --blocksize/--processes callback to validate arg */
  { "blocksize", 'b', POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT,	&__zq.blocksize, 0,
	N_("Set compression block size to mmmK"), N_("mmm") },
  /* XXX same as --threads */
  { "processes", 'p', POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT,	&__zq.threads, 0,
	N_("Allow up to n compression threads"), N_("n") },
#else
  /* XXX show default is bogus with callback, can't find value. */
  { "blocksize", 'b', POPT_ARG_VAL|POPT_ARGFLAG_SHOW_DEFAULT,	NULL, 'b',
	N_("Set compression block size to mmmK"), N_("mmm") },
  /* XXX same as --threads */
  { "processes", 'p', POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT,	NULL, 'p',
	N_("Allow up to n compression threads"), N_("n") },
#endif
  /* XXX display toggle "-i,--[no]indepndent" bustage. */
  { "independent", 'i', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,	&__zq.flags, RPMZ_FLAGS_INDEPENDENT,
	N_("Compress blocks independently for damage recovery"), NULL },
  /* XXX display toggle "-r,--[no]rsyncable" bustage. */
  { "rsyncable", 'R', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,		&__zq.flags, RPMZ_FLAGS_RSYNCABLE,
	N_("Input-determined block locations for rsync"), NULL },
#if defined(NOTYET)
  /* XXX -T collides with pigz -T,--no-time */
  { "threads", '\0', POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT,	&__zq.threads, 0,
	N_("Allow up to n compression threads"), N_("n") },
#endif	/* _RPMZ_INTERNAL_XZ */

  /* ===== Operation modes */
  { "compress", 'z', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.mode, RPMZ_MODE_COMPRESS,
	N_("force compression"), NULL },
  { "uncompress", '\0', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.mode, RPMZ_MODE_DECOMPRESS,
	N_("force decompression"), NULL },
  { "decompress", 'd', POPT_ARG_VAL,		&__zq.mode, RPMZ_MODE_DECOMPRESS,
	N_("Decompress the compressed input"), NULL },
  { "test", 't', POPT_ARG_VAL,			&__zq.mode,  RPMZ_MODE_TEST,
	N_("Test the integrity of the compressed input"), NULL },
  { "list", 'l', POPT_BIT_SET,			&__zq.flags,  RPMZ_FLAGS_LIST,
	N_("List the contents of the compressed input"), NULL },
  { "info", '\0', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN,	&__zq.flags,  RPMZ_FLAGS_LIST,
	N_("list block sizes, total sizes, and possible metadata"), NULL },
  { "force", 'f', POPT_BIT_SET,		&__zq.flags,  RPMZ_FLAGS_FORCE,
	N_("Force: --overwrite --recompress --symlinks --tty"), NULL },
  { "overwrite", '\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,
	&__zq.flags,  RPMZ_FLAGS_OVERWRITE,
	N_("  Permit overwrite of output files"), NULL },
  { "recompress",'\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,
	&__zq.flags,  RPMZ_FLAGS_ALREADY,
	N_("  Permit compress of already compressed files"), NULL },
  { "symlinks",'\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,
	&__zq.flags,  RPMZ_FLAGS_SYMLINKS,
	N_("  Permit symlink input file to be compressed"), NULL },
  { "tty",'\0', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,
	&__zq.flags,  RPMZ_FLAGS_TTY,
	N_("  Permit compressed output to terminal"), NULL },

  /* ===== Operation modifiers */
  /* XXX display toggle "-r,--[no]recursive" bustage. */
  { "recursive", 'r', POPT_BIT_SET|POPT_ARGFLAG_TOGGLE,	&__zq.flags, RPMZ_FLAGS_RECURSE,
	N_("Process the contents of all subdirectories"), NULL },
  { "suffix", 'S', POPT_ARG_STRING,		&__zq.suffix, 0,
	N_("Use suffix .sss instead of .gz (for compression)"), N_(".sss") },
  { "ascii", 'a', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	NULL, 'a',
	N_("Compress to LZW (.Z) instead of gzip format"), NULL },
  { "bits", 'Z', POPT_ARG_VAL|POPT_ARGFLAG_DOC_HIDDEN,	NULL, 'Z',
	N_("Compress to LZW (.Z) instead of gzip format"), NULL },
  { "zlib", 'z', POPT_ARG_VAL,		&__zq.format, RPMZ_FORMAT_ZLIB,
	N_("Compress to zlib (.zz) instead of gzip format"), NULL },
  { "zip", 'K', POPT_ARG_VAL,		&__zq.format, RPMZ_FORMAT_ZIP2,
	N_("Compress to PKWare zip (.zip) single entry format"), NULL },
  { "keep", 'k', POPT_BIT_SET,			&__zq.flags, RPMZ_FLAGS_KEEP,
	N_("Do not delete original file after processing"), NULL },
  { "stdout", 'c', POPT_BIT_SET,		&__zq.flags,  RPMZ_FLAGS_STDOUT,
	N_("Write all processed output to stdout (won't delete)"), NULL },
  { "to-stdout", 'c', POPT_BIT_SET|POPT_ARGFLAG_DOC_HIDDEN, &__zq.flags,  RPMZ_FLAGS_STDOUT,
	N_("write to standard output and don't delete input files"), NULL },

  /* ===== Metadata options */
  /* XXX logic is reversed, disablers should clear with toggle. */
  { "name", 'N', POPT_BIT_SET,		&__zq.flags, (RPMZ_FLAGS_HNAME|RPMZ_FLAGS_HTIME),
	N_("Store/restore file name and mod time in/from header"), NULL },
  { "no-name", 'n', POPT_BIT_CLR,	&__zq.flags, RPMZ_FLAGS_HNAME,
	N_("Do not store or restore file name in/from header"), NULL },
  /* XXX -T collides with xz -T,--threads */
  { "no-time", 'T', POPT_BIT_CLR,	&__zq.flags, RPMZ_FLAGS_HTIME,
	N_("Do not store or restore mod time in/from header"), NULL },

  /* ===== Other options */

  POPT_TABLEEND
};

#define	zqFprintf	if (_rpmzq_debug) fprintf

/*==============================================================*/

/*@-mustmod@*/
int rpmbzCompressBlock(void * _bz, rpmzJob job)
{
    rpmbz bz = (rpmbz) _bz;
    unsigned int len = job->out->len;
    int rc;
    rc = BZ2_bzBuffToBuffCompress((char *)job->out->buf, &len,
		(char *)job->in->buf, job->in->len, bz->B, bz->V, bz->W);
    job->out->len = len;
    if (rc != BZ_OK)
	zqFprintf(stderr, "==> %s(%p,%p) rc %d\n", __FUNCTION__, bz, job, rc);
    return rc;
}
/*@=mustmod@*/

/*@-mustmod@*/
static int rpmbzDecompressBlock(rpmbz bz, rpmzJob job)
	/*@globals fileSystem @*/
	/*@modifies job, fileSystem @*/
{
    unsigned int len = job->out->len;
    int rc;
    rc = BZ2_bzBuffToBuffDecompress((char *)job->out->buf, &len,
		(char *)job->in->buf, job->in->len, bz->S, bz->V);
    job->out->len = len;
    if (rc != BZ_OK)
	zqFprintf(stderr, "==> %s(%p,%p) rc %d\n", __FUNCTION__, bz, job, rc);
    return rc;
}
/*@=mustmod@*/

/*==============================================================*/

/* -- pool of spaces for buffer management -- */

/* These routines manage a pool of spaces.  Each pool specifies a fixed size
   buffer to be contained in each space.  Each space has a use count, which
   when decremented to zero returns the space to the pool.  If a space is
   requested from the pool and the pool is empty, a space is immediately
   created unless a specified limit on the number of spaces has been reached.
   Only if the limit is reached will it wait for a space to be returned to the
   pool.  Each space knows what pool it belongs to, so that it can be returned.
 */

/* initialize a pool (pool structure itself provided, not allocated) -- the
   limit is the maximum number of spaces in the pool, or -1 to indicate no
   limit, i.e., to never wait for a buffer to return to the pool */
rpmzPool rpmzqNewPool(size_t size, int limit)
{
    rpmzPool pool = (rpmzPool) xcalloc(1, sizeof(*pool));
/*@=mustfreeonly@*/
    pool->have = yarnNewLock(0);
    pool->head = NULL;
/*@=mustfreeonly@*/
    pool->size = size;
    pool->limit = limit;
    pool->made = 0;
zqFprintf(stderr, "    ++ pool %p[%u,%d]\n", pool, (unsigned)size, limit);
    return pool;
}

/* get a space from a pool -- the use count is initially set to one, so there
   is no need to call rpmzqUseSpace() for the first use */
rpmzSpace rpmzqNewSpace(rpmzPool pool, size_t len)
{
    rpmzSpace space;

    if (pool != NULL) {
	/* if can't create any more, wait for a space to show up */
	yarnPossess(pool->have);
	if (pool->limit == 0)
	    yarnWaitFor(pool->have, NOT_TO_BE, 0);

	/* if a space is available, pull it from the list and return it */
	if (pool->head != NULL) {
	    space = pool->head;
	    yarnPossess(space->use);
	    pool->head = space->next;
	    yarnTwist(pool->have, BY, -1);      /* one less in pool */
	    yarnTwist(space->use, TO, 1);       /* initially one user */
	    return space;
	}

	/* nothing available, don't want to wait, make a new space */
assert(pool->limit != 0);
	if (pool->limit > 0)
	    pool->limit--;
	pool->made++;
	yarnRelease(pool->have);
    }

    space = (rpmzSpace) xcalloc(1, sizeof(*space));
/*@-mustfreeonly@*/
    space->use = yarnNewLock(1);	/* initially one user */
    space->len = (pool ? pool->size : len);
    if (space->len > 0)
	space->buf = (unsigned char *) xmalloc(space->len);
    space->ptr = space->buf;		/* XXX save allocated buffer */
    space->ix = 0;			/* XXX initialize to 0 */
/*@-assignexpose -temptrans @*/
    space->pool = pool;                 /* remember the pool this belongs to */
/*@=assignexpose =temptrans @*/
/*@=mustfreeonly@*/
zqFprintf(stderr, "    ++ space %p[%d] buf %p[%u]\n", space, 1, space->buf, (unsigned)space->len);
/*@-nullret@*/
    return space;
/*@=nullret@*/
}

/* increment the use count to require one more drop before returning this space
   to the pool */
void rpmzqUseSpace(rpmzSpace space)
{
    int use;
    yarnPossess(space->use);
    use = yarnPeekLock(space->use);
zqFprintf(stderr, "    ++ space %p[%d] buf %p[%u]\n", space, use+1, space->buf, (unsigned)space->len);
    yarnTwist(space->use, BY, 1);
}

/* drop a space, returning it to the pool if the use count is zero */
rpmzSpace rpmzqDropSpace(rpmzSpace space)
{
    int use;

    if (space == NULL)
	return NULL;

    yarnPossess(space->use);
    use = yarnPeekLock(space->use);
zqFprintf(stderr, "    -- space %p[%d] buf %p[%u]\n", space, use, space->buf, (unsigned)space->len);
#ifdef	NOTYET
assert(use > 0);
#else
if (use <= 0)
fprintf(stderr, "==> FIXME: %s: space %p[%d]\n", __FUNCTION__, space, use);
#endif
    if (use == 1) {
	rpmzPool pool = space->pool;
	if (pool != NULL) {
	    yarnPossess(pool->have);
	    space->buf = (unsigned char *) space->ptr;	/* XXX reset to original allocation. */
	    space->len = pool->size;	/* XXX reset to pool->size */
	    space->ix = 0;			/* XXX reset to 0 */
/*@-mustfreeonly@*/
	    space->next = pool->head;
/*@=mustfreeonly@*/
	    pool->head = space;
	    yarnTwist(pool->have, BY, 1);
	} else {
	    yarnTwist(space->use, BY, -1);
	    space->ptr = _free(space->ptr);
	    space->use = yarnFreeLock(space->use);
/*@-compdestroy@*/
	    space = _free(space);
/*@=compdestroy@*/
	    return NULL;
	}
    }
    yarnTwist(space->use, BY, -1);
    return NULL;
}

/* free the memory and lock resources of a pool -- return number of spaces for
   debugging and resource usage measurement */
rpmzPool rpmzqFreePool(rpmzPool pool, int *countp)
{
    rpmzSpace space;
    int count;

    yarnPossess(pool->have);
    count = 0;
    while ((space = pool->head) != NULL) {
	pool->head = space->next;
	space->ptr = _free(space->ptr);
	space->use = yarnFreeLock(space->use);
/*@-compdestroy@*/
	space = _free(space);
/*@=compdestroy@*/
	count++;
    }
    yarnRelease(pool->have);
    pool->have = yarnFreeLock(pool->have);
#ifdef	NOTYET
assert(count == pool->made);
#else
if (count != pool->made)
fprintf(stderr, "==> FIXME: %s: count %d pool->made %d\n", __FUNCTION__, count, pool->made);
#endif
zqFprintf(stderr, "    -- pool %p count %d\n", pool, count);
/*@-compdestroy@*/
    pool = _free(pool);
/*@=compdestroy@*/
    if (countp != NULL)
	*countp = count;
    return NULL;
}

rpmzJob rpmzqNewJob(long seq)
{
    rpmzJob job = (rpmzJob) xcalloc(1, sizeof(*job));
    job->use = yarnNewLock(1);           /* initially one user */
    job->seq = seq;
    job->calc = yarnNewLock(0);
zqFprintf(stderr, "    ++ job %p[%ld] use %d\n", job, seq, 1);
    return job;
}

rpmzJob rpmzqUseJob(rpmzJob job)
{
    int use;
    if (job == NULL) return NULL;
    yarnPossess(job->use);
    use = yarnPeekLock(job->use);
zqFprintf(stderr, "    ++ job %p[%ld] use %d\n", job, job->seq, use+1);
    yarnTwist(job->use, BY, 1);
    return job;
}

/* drop a job, returning it to the pool if the use count is zero */
rpmzJob rpmzqDropJob(rpmzJob job)
{
    int use;

    if (job == NULL)
	return NULL;

    yarnPossess(job->use);
    use = yarnPeekLock(job->use);
zqFprintf(stderr, "    -- job %p[%ld] use %d %p => %p\n", job, job->seq, use, job->in, job->out);
#ifdef	NOTYET
assert(use > 0);
#else
if (use <= 0)
fprintf(stderr, "==> FIXME: %s: job %p[%ld] use %d\n", __FUNCTION__, job, job->seq, use);
#endif
    if (use == 1) {
#ifdef	NOTYET
	rpmzJPool jpool = job->pool;
	if (jpool != NULL) {
	    yarnPossess(jpool->have);
	    jpool->head = job;
	    yarnTwist(jpool->have, BY, 1);
	} else
#endif
	{
	    yarnTwist(job->use, BY, -1);
	    if (job->use != NULL)
		job->use = yarnFreeLock(job->use);
	    if (job->calc != NULL)
		job->calc = yarnFreeLock(job->calc);
	    job = _free(job);
	    return NULL;
	}
    }
    yarnTwist(job->use, BY, -1);
    return NULL;
}

/* compress or write job (passed from compress list to write list) -- if seq is
   equal to -1, rpmzqCompressThread() is instructed to return; if more is false then
   this is the last chunk, which after writing tells write_thread to return */

/* command the compress threads to all return, then join them all (call from
   main thread), free all the thread-related resources */
void rpmzqFini(rpmzQueue zq)
{
    rpmzLog zlog = zq->zlog;

    struct rpmzJob_s job;
    int caught;

zqFprintf(stderr, "--> %s(%p)\n", __FUNCTION__, zq);
    /* only do this once */
    if (zq->_zc.q == NULL)
	return;

    /* command all of the extant compress threads to return */
    yarnPossess(zq->_zc.q->have);
    job.seq = -1;
    job.next = NULL;
/*@-immediatetrans -mustfreeonly@*/
    zq->_zc.q->head = &job;
/*@=immediatetrans =mustfreeonly@*/
    zq->_zc.q->tail = &job.next;
    yarnTwist(zq->_zc.q->have, BY, 1);		/* will wake them all up */

    /* join all of the compress threads, verify they all came back */
    caught = yarnJoinAll();
    Trace((zlog, "-- joined %d compress threads", caught));
#ifdef	NOTYET
assert(caught == zq->_zc.cthreads);
#else
if (caught != zq->_zc.cthreads)
fprintf(stderr, "==> FIXME: %s: caught %d z->_zc.cthreads %d\n", __FUNCTION__, caught, zq->_zc.cthreads);
#endif
    zq->_zc.cthreads = 0;

    /* free the resources */
    zq->_zw.pool = rpmzqFreePool(zq->_zw.pool, &caught);
    Trace((zlog, "-- freed %d output buffers", caught));
    zq->_zc.pool = rpmzqFreePool(zq->_zc.pool, &caught);
    Trace((zlog, "-- freed %d input buffers", caught));
    zq->_zc.q = rpmzqFiniFIFO(zq->_zc.q);
    zq->_zw.q = rpmzqFiniSEQ(zq->_zw.q);
}

/* setup job lists (call from main thread) */
void rpmzqInit(rpmzQueue zq)
{
zqFprintf(stderr, "--> %s(%p)\n", __FUNCTION__, zq);
    /* set up only if not already set up*/
    if (zq->_zc.q != NULL)
	return;

    /* allocate locks and initialize lists */
/*@-mustfreeonly@*/
    zq->_zc.q = rpmzqInitFIFO(0L);
    zq->_zw.q = rpmzqInitSEQ(-1L);

    zq->_zc.pool = rpmzqNewPool(zq->iblocksize, zq->ilimit);
zqFprintf(stderr, "-->  in_pool: %p[%u] blocksize %u\n", zq->_zc.pool, (unsigned)zq->ilimit, (unsigned)zq->iblocksize);
    zq->_zw.pool = rpmzqNewPool(zq->oblocksize, zq->olimit);
zqFprintf(stderr, "--> out_pool: %p[%u] blocksize %u\n", zq->_zw.pool, (unsigned)zq->olimit, (unsigned)zq->oblocksize);

}

rpmzQueue rpmzqFree(/*@unused@*/ rpmzQueue zq)
{
    return NULL;
}

rpmzQueue rpmzqNew(rpmzQueue zq, rpmzLog zlog, int limit)
{
    zq->_zinp.fn = NULL;
    zq->_zinp.fdno = -1;
    zq->_zout.fn = NULL;
    zq->_zout.fdno = -1;
    zq->iblocksize = zq->blocksize;
    zq->ilimit = limit;
    zq->oblocksize = zq->blocksize;
    zq->olimit = limit;
/*@-assignexpose@*/
    zq->zlog = zlog;
/*@=assignexpose@*/
    return zq;
}

rpmzFIFO rpmzqInitFIFO(long val)
{
    rpmzFIFO zs = (rpmzFIFO) xcalloc(1, sizeof(*zs));
    zs->have = yarnNewLock(val);
    zs->head = NULL;
    zs->tail = &zs->head;
    return zs;
}

rpmzFIFO rpmzqFiniFIFO(rpmzFIFO zs)
{
    if (zs->have != NULL)
	zs->have = yarnFreeLock(zs->have);
    zs->head = NULL;
    zs->tail = &zs->head;
    zs = _free(zs);
    return NULL;
}

void rpmzqVerifyFIFO(rpmzFIFO zs)
{
assert(zs != NULL);
    yarnPossess(zs->have);
assert(zs->head == NULL && yarnPeekLock(zs->have) == 0);
    yarnRelease(zs->have);
}

rpmzJob rpmzqDelFIFO(rpmzFIFO zs)
{
    rpmzJob job;

    /* get job from compress list, let all the compressors know */
    yarnPossess(zs->have);
    yarnWaitFor(zs->have, NOT_TO_BE, 0);
    job = zs->head;
assert(job != NULL);
    if (job->seq == -1) {
	yarnRelease(zs->have);
	return NULL;
    }

/*@-assignexpose -dependenttrans@*/
    zs->head = job->next;
/*@=assignexpose =dependenttrans@*/
    if (job->next == NULL)
	zs->tail = &zs->head;
    yarnTwist(zs->have, BY, -1);

    return job;
}

void rpmzqAddFIFO(rpmzFIFO zs, rpmzJob job)
{
    /* put job at end of compress list, let all the compressors know */
    yarnPossess(zs->have);
    job->next = NULL;
/*@-assignexpose@*/
    *zs->tail = job;
    zs->tail = &job->next;
/*@=assignexpose@*/
    yarnTwist(zs->have, BY, 1);
}

rpmzSEQ rpmzqInitSEQ(long val)
{
    rpmzSEQ zs = (rpmzSEQ) xcalloc(1, sizeof(*zs));
    zs->first = yarnNewLock(val);
    zs->head = NULL;
    return zs;
}

rpmzSEQ rpmzqFiniSEQ(rpmzSEQ zs)
{
    if (zs->first != NULL)
	zs->first = yarnFreeLock(zs->first);
    zs->head = NULL;
    zs = _free(zs);
    return NULL;
}

void rpmzqVerifySEQ(rpmzSEQ zs)
{
assert(zs != NULL);
    yarnPossess(zs->first);
assert(zs->head == NULL && yarnPeekLock(zs->first) == -1);
    yarnRelease(zs->first);
}

rpmzJob rpmzqDelSEQ(rpmzSEQ zs, long seq)
{
    rpmzJob job;

    /* get next read job in order */
    yarnPossess(zs->first);
    yarnWaitFor(zs->first, TO_BE, seq);
    job = zs->head;
assert(job != NULL);
/*@-assignexpose -dependenttrans@*/
    zs->head = job->next;
/*@=assignexpose =dependenttrans@*/
    yarnTwist(zs->first, TO, zs->head == NULL ? -1 : zs->head->seq);
    return job;
}

void rpmzqAddSEQ(rpmzSEQ zs, rpmzJob job)
{
    rpmzJob here;		/* pointers for inserting in SEQ list */
    rpmzJob * prior;		/* pointers for inserting in SEQ list */

    yarnPossess(zs->first);

    /* insert read job in list in sorted order, alert read thread */
    prior = &zs->head;
    while ((here = *prior) != NULL) {
	if (here->seq > job->seq)
	    break;
	prior = &here->next;
    }
/*@-assignexpose@*/
    job->next = here;
/*@=assignexpose@*/
    *prior = job;

    yarnTwist(zs->first, TO, zs->head->seq);
}

rpmzJob rpmzqDelCJob(rpmzQueue zq)
{
    rpmzJob job;

    /* get job from compress list, let all the compressors know */
    yarnPossess(zq->_zc.q->have);
    yarnWaitFor(zq->_zc.q->have, NOT_TO_BE, 0);
    job = zq->_zc.q->head;
assert(job != NULL);
    if (job->seq == -1) {
	yarnRelease(zq->_zc.q->have);
	return NULL;
    }

/*@-assignexpose -dependenttrans@*/
    zq->_zc.q->head = job->next;
/*@=assignexpose =dependenttrans@*/
    if (job->next == NULL)
	zq->_zc.q->tail = &zq->_zc.q->head;
    yarnTwist(zq->_zc.q->have, BY, -1);

    return job;
}

void rpmzqAddCJob(rpmzQueue zq, rpmzJob job)
{
    /* put job at end of compress list, let all the compressors know */
    yarnPossess(zq->_zc.q->have);
    job->next = NULL;
/*@-assignexpose@*/
    *zq->_zc.q->tail = job;
    zq->_zc.q->tail = &job->next;
/*@=assignexpose@*/
    yarnTwist(zq->_zc.q->have, BY, 1);
}

rpmzJob rpmzqDelWJob(rpmzQueue zq, long seq)
{
    rpmzJob job;

    /* get next write job in order */
    yarnPossess(zq->_zw.q->first);
    yarnWaitFor(zq->_zw.q->first, TO_BE, seq);
    job = zq->_zw.q->head;
assert(job != NULL);
/*@-assignexpose -dependenttrans@*/
    zq->_zw.q->head = job->next;
/*@=assignexpose =dependenttrans@*/
    yarnTwist(zq->_zw.q->first, TO, zq->_zw.q->head == NULL ? -1 : zq->_zw.q->head->seq);
    return job;
}

void rpmzqAddWJob(rpmzQueue zq, rpmzJob job)
{
    rpmzLog zlog = zq->zlog;

    rpmzJob here;		/* pointers for inserting in write list */
    rpmzJob * prior;		/* pointers for inserting in write list */
    double pct;

    yarnPossess(zq->_zw.q->first);

    switch (zq->omode) {
    default:	assert(0);	break;
    case O_WRONLY:
	pct = (100.0 * job->out->len) / job->in->len;
	zqFprintf(stderr, "       job %p[%ld]:\t%p[%u] => %p[%u]\t(%3.1f%%)\n",
			job, job->seq, job->in->buf, (unsigned)job->in->len,
			job->out->buf, (unsigned)job->out->len, pct);
	Trace((zlog, "-- compressed #%ld %3.1f%%%s", job->seq, pct,
		(job->more ? "" : " (last)")));
	break;
    case O_RDONLY:
	pct = (100.0 * job->in->len) / job->out->len;
	zqFprintf(stderr, "       job %p[%ld]:\t%p[%u] <= %p[%u]\t(%3.1f%%)\n",
			job, job->seq, job->in->buf, (unsigned)job->in->len,
			job->out->buf, (unsigned)job->out->len, pct);
	Trace((zlog, "-- decompressed #%ld %3.1f%%%s", job->seq, pct,
		(job->more ? "" : " (last)")));
	break;
    }

    /* insert write job in list in sorted order, alert write thread */
    prior = &zq->_zw.q->head;
    while ((here = *prior) != NULL) {
	if (here->seq > job->seq)
	    break;
	prior = &here->next;
    }
/*@-assignexpose@*/
    job->next = here;
/*@=assignexpose@*/
    *prior = job;

    yarnTwist(zq->_zw.q->first, TO, zq->_zw.q->head->seq);
}

static rpmzJob rpmzqFillOut(rpmzQueue zq, /*@returned@*/rpmzJob job, rpmbz bz)
	/*@globals fileSystem, internalState @*/
	/*@modifies zq, job, fileSystem, internalState @*/
{
    size_t outlen;
    int ret;

    switch (zq->omode) {
    default:	assert(0);	break;
    case O_WRONLY:
	/* set up input and output (the output size is assured to be big enough
	 * for the worst case expansion of the input buffer size, plus five
	 * bytes for the terminating stored block) */
	outlen = ((job->in->len*1.01)+600);
/*@-mustfreeonly@*/
	job->out = rpmzqNewSpace(zq->_zw.pool, zq->_zw.pool->size);
/*@=mustfreeonly@*/
	if (job->out->len < outlen) {
fprintf(stderr, "==> FIXME: %s: job->out %p %p[%u] malloc(%u)\n", __FUNCTION__, job->out, job->out->buf, (unsigned)job->out->len, (unsigned)outlen);
	    job->out = rpmzqDropSpace(job->out);
	    job->out = (rpmzSpace) xcalloc(1, sizeof(*job->out));
	    job->out->len = outlen;
	    job->out->buf = (unsigned char *) xmalloc(job->out->len);
	}

	/* compress job->in to job-out */
	ret = rpmbzCompressBlock(bz, job);
	break;
    case O_RDONLY:
	outlen = 6 * job->in->len;
	job->out = rpmzqNewSpace(zq->_zw.pool, zq->_zw.pool->size);
	if (job->out->len < outlen) {
fprintf(stderr, "==> FIXME: %s: job->out %p %p[%u] malloc(%u)\n", __FUNCTION__, job->out, job->out->buf, (unsigned)job->out->len, (unsigned)outlen);
	    job->out = rpmzqDropSpace(job->out);
	    job->out = (rpmzSpace) xcalloc(1, sizeof(*job->out));
	    job->out->len = outlen;
	    job->out->buf = (unsigned char *) xmalloc(job->out->len);
	}

	for (;;) {
	    static int _grow = 2;	/* XXX factor to scale malloc by. */

	    outlen = job->out->len * _grow;
	    ret = rpmbzDecompressBlock(bz, job);
	    if (ret != BZ_OUTBUFF_FULL)
		/*@loopbreak@*/ break;
fprintf(stderr, "==> FIXME: %s: job->out %p %p[%u] realloc(%u)\n", __FUNCTION__, job->out, job->out->buf, (unsigned)job->out->len, (unsigned)outlen);
	    if (job->out->use != NULL)
		job->out = rpmzqDropSpace(job->out);
	    else {
fprintf(stderr, "==> FIXME: %s: job->out %p %p[%u] free\n", __FUNCTION__, job->out, job->out->buf, (unsigned)job->out->len);
		job->out->buf = _free(job->out->buf);
		job->out = _free(job->out);
	    }
	    job->out = (rpmzSpace) xcalloc(1, sizeof(*job->out));
	    job->out->len = outlen;
	    job->out->buf = (unsigned char *) xmalloc(job->out->len);
	}
assert(ret == BZ_OK);
	break;
    }
    return job;
}

/* get the next compression job from the head of the list, compress and compute
   the check value on the input, and put a job in the write list with the
   results -- keep looking for more jobs, returning when a job is found with a
   sequence number of -1 (leave that job in the list for other incarnations to
   find) */
static void rpmzqCompressThread (void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies _zq, fileSystem, internalState @*/
{
    rpmzQueue zq = (rpmzQueue) _zq;
    rpmbz bz = rpmbzInit(zq->level, -1, -1, zq->omode);
    rpmzJob job;

zqFprintf(stderr, "--> %s(%p) bz %p\n", __FUNCTION__, zq, bz);

    /* get job, insert write job in list in sorted order, alert write thread */
/*@-evalorder@*/
    while ((job = rpmzqDelCJob(zq)) != NULL) {
	rpmzqAddWJob(zq, rpmzqFillOut(zq, job, bz));
    }
/*@=evalorder@*/

    bz = rpmbzFini(bz);
}

static void rpmzqDecompressThread(void *_zq)
	/*@globals fileSystem, internalState @*/
	/*@modifies _zq, fileSystem, internalState @*/
{
    rpmzQueue zq = (rpmzQueue) _zq;
    rpmbz bz = rpmbzInit(zq->level, -1, -1, zq->omode);
    rpmzJob job;

zqFprintf(stderr, "--> %s(%p) bz %p\n", __FUNCTION__, zq, bz);

    /* get job, insert write job in list in sorted order, alert write thread */
/*@-evalorder@*/
    while ((job = rpmzqDelCJob(zq)) != NULL) {
	rpmzqAddWJob(zq, rpmzqFillOut(zq, job, bz));
    }
/*@=evalorder@*/

    bz = rpmbzFini(bz);
}

/* start another compress/decompress thread if needed */
void rpmzqLaunch(rpmzQueue zq, long seq, unsigned int threads)
{
    if (zq->_zc.cthreads < seq && zq->_zc.cthreads < (int)threads) {
	switch (zq->omode) {
	default:	assert(0);	break;
	case O_WRONLY: (void)yarnLaunch(rpmzqCompressThread, zq);	break;
	case O_RDONLY: (void)yarnLaunch(rpmzqDecompressThread, zq);	break;
	}
	zq->_zc.cthreads++;
    }
}

/* verify no more jobs, prepare for next use */
void rpmzqVerify(rpmzQueue zq)
{
    rpmzqVerifyFIFO(zq->_zc.q);
    rpmzqVerifySEQ(zq->_zw.q);
}

#endif /* WITH_BZIP2 */
