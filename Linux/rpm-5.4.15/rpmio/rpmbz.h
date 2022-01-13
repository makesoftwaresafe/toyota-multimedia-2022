#ifndef	_H_RPMBZ_
#define	_H_RPMBZ_

#include <bzlib.h>

#if defined(__LCLINT__)
/*@-incondefs =protoparammatch@*/
/*@-exportheader@*/

BZ_EXTERN BZFILE* BZ_API(BZ2_bzReadOpen) (
   /*@out@*/
      int*  bzerror,
      FILE* f,
      int   verbosity,
      int   small,
   /*@out@*/
      void* unused,
      int   nUnused
   )
	/*@modifies *bzerror, f @*/;

BZ_EXTERN void BZ_API(BZ2_bzReadClose) (
   /*@out@*/
      int*    bzerror,
   /*@only@*/
      BZFILE* b
   )
	/*@modifies *bzerror, b @*/;

BZ_EXTERN void BZ_API(BZ2_bzReadGetUnused) (
   /*@out@*/
      int*    bzerror,
      BZFILE* b,
   /*@out@*/
      void**  unused,
      int*    nUnused
   )
	/*@modifies *bzerror, b, *unused, *nUnused @*/;

BZ_EXTERN int BZ_API(BZ2_bzRead) (
   /*@out@*/
      int*    bzerror,
      BZFILE* b,
   /*@out@*/
      void*   buf,
      int     len
   )
	/*@modifies *bzerror, b, *buf @*/;

BZ_EXTERN BZFILE* BZ_API(BZ2_bzWriteOpen) (
      int*  bzerror,
      FILE* f,
      int   blockSize100k,
      int   verbosity,
      int   workFactor
   )
	/*@modifies *bzerror @*/;

BZ_EXTERN void BZ_API(BZ2_bzWrite) (
   /*@out@*/
      int*    bzerror,
      BZFILE* b,
      void*   buf,
      int     len
   )
	/*@modifies *bzerror, b @*/;

BZ_EXTERN void BZ_API(BZ2_bzWriteClose) (
   /*@out@*/
      int*          bzerror,
   /*@only@*/
      BZFILE*       b,
      int           abandon,
   /*@out@*/
      unsigned int* nbytes_in,
   /*@out@*/
      unsigned int* nbytes_out
   )
	/*@modifies *bzerror, b, *nbytes_in, *nbytes_out @*/;

BZ_EXTERN int BZ_API(BZ2_bzflush) (
      BZFILE* b
   )
	/*@modifies b @*/;

BZ_EXTERN const char * BZ_API(BZ2_bzerror) (
      BZFILE *b,
   /*@out@*/
      int    *errnum
   )
	/*@modifies *errnum @*/;

BZ_EXTERN int BZ_API(BZ2_bzBuffToBuffCompress) (
      char*         dest,
      unsigned int* destLen,
      char*         source,
      unsigned int  sourceLen,
      int           blockSize100k,
      int           verbosity,
      int           workFactor
   )
	/*@modifies *dest, *destLen @*/;

BZ_EXTERN int BZ_API(BZ2_bzBuffToBuffDecompress) (
      char*         dest,
      unsigned int* destLen,
      char*         source,
      unsigned int  sourceLen,
      int           small,
      int           verbosity
   )
	/*@modifies *dest, *destLen @*/;

BZ_EXTERN int BZ_API(BZ2_bzCompressInit) ( 
      bz_stream* strm, 
      int        blockSize100k, 
      int        verbosity, 
      int        workFactor 
   )
	/*@modifies strm @*/;

BZ_EXTERN int BZ_API(BZ2_bzCompress) ( 
      bz_stream* strm, 
      int action 
   )
	/*@modifies strm @*/;

BZ_EXTERN int BZ_API(BZ2_bzCompressEnd) ( 
      bz_stream* strm 
   )
	/*@modifies strm @*/;

BZ_EXTERN int BZ_API(BZ2_bzDecompressInit) ( 
      bz_stream *strm, 
      int       verbosity, 
      int       small
   )
	/*@modifies strm @*/;

BZ_EXTERN int BZ_API(BZ2_bzDecompress) ( 
      bz_stream* strm 
   )
	/*@modifies strm @*/;

BZ_EXTERN int BZ_API(BZ2_bzDecompressEnd) ( 
      bz_stream *strm 
   )
	/*@modifies strm @*/;


/*@=exportheader@*/
/*@=incondefs =protoparammatch@*/
#endif	/* __LCLINT__ */

/**
 */
typedef	/*@abstract@*/ struct rpmbz_s * rpmbz;

/**
 */
#if defined(_RPMBZ_INTERNAL)
struct rpmbz_s {
/*@null@*/
    BZFILE *bzfile;	
    bz_stream strm;
    int bzerr;
    int omode;			/*!< open mode: O_RDONLY | O_WRONLY */
/*@dependent@*/ /*@null@*/
    FILE * fp;			/*!< file pointer */
    int B;			/*!< blockSize100K (default: 9) */
    int S;			/*!< small (default: 0) */
    int V;			/*!< verboisty (default: 0) */
    int W;			/*!< workFactor (default: 30) */
    unsigned int nbytes_in;
    unsigned int nbytes_out;

    unsigned int blocksize;	/*!< compression block size */

/*@null@*/
    void * zq;			/*!< buffer pools. */
/*@null@*/
    void * zlog;		/*!< trace logging. */
};

#ifdef __cplusplus
GENfree(rpmbz)
#endif	/* __cplusplus */

/*@only@*/ /*@null@*/
static rpmbz rpmbzFini(/*@only@*/ rpmbz bz)
	/*@modifies bz @*/
{
    bz = _free(bz);
    return NULL;
}

/*@only@*/
static rpmbz rpmbzInit(int level, int small, int verbosity, mode_t omode)
	/*@*/
{
    rpmbz bz = (rpmbz) xcalloc(1, sizeof(*bz));
    static int _bzdB = 9;
    static int _bzdS = 0;
    static int _bzdV = 1;
    static int _bzdW = 30;

    bz->B = (level >= 1 && level <= 9) ? level : _bzdB;
    bz->S = (small >= 0) ? small : _bzdS;
    bz->V = (verbosity >= 0 && verbosity <= 4) ? verbosity : _bzdV;
    bz->W = _bzdW;
    bz->omode = omode;
    return bz;
}
#endif	/* _RPMBZ_INTERNAL */

/**
 */
const char * rpmbzStrerror(rpmbz bz)
	/*@*/;

/**
 */
void rpmbzClose(rpmbz bz, int abort, /*@null@*/ const char ** errmsg)
	/*@modifies bz, *errmsg @*/;

/**
 */
/*@only@*/ /*@null@*/
rpmbz rpmbzFree(/*@only@*/ rpmbz bz, int abort)
	/*@globals fileSystem @*/
	/*@modifies bz, fileSystem @*/;

/**
 */
/*@only@*/
rpmbz rpmbzNew(const char * path, const char * fmode, int fdno)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 */
ssize_t rpmbzRead(rpmbz bz, /*@out@*/ char * buf, size_t count,
		/*@null@*/ const char ** errmsg)
	/*@globals internalState @*/
	/*@modifies bz, *buf, *errmsg, internalState @*/;

/**
 */
ssize_t rpmbzWrite(rpmbz bz, const char * buf, size_t count,
		/*@null@*/ const char ** errmsg)
	/*@modifies bz, *errmsg @*/;

#ifdef	NOTYET
/**
 */
int rpmbzSeek(/*@unused@*/ void * _bz, /*@unused@*/ _libio_pos_t pos,
			/*@unused@*/ int whence)
	/*@*/;

/**
 */
/*@null@*/
rpmbz rpmbzOpen(const char * path, const char * fmode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 */
/*@null@*/
rpmbz rpmbzFdopen(void * _fdno, const char * fmode)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 */
int rpmbzFlush(void * _bz)
	/*@*/;

/**
 */
int rpmbzCompressBlock(rpmbz bz, rpmzJob job)
	/*@modifies job @*/;

/**
 */
int rpmbzDecompressBlock(rpmbz bz, rpmzJob job)
	/*@modifies job @*/;
#endif	/* NOTYET */

#endif	/* _H_RPMBZ_ */
