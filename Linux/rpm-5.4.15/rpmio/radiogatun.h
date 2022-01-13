/* RadioGatún reference code
 * Public domain
 * For more information on RadioGatún, please refer to 
 * http://radiogatun.noekeon.org/
 *
 * Streaming API and padding conventions borrowed from sphlib-3.0.
*/
#ifndef _RADIOGATUN_H_
#define _RADIOGATUN_H_

#include <stdint.h>
#include <beecrypt/beecrypt.h>

#define	RG32_SIZE	256	/* bits */
#ifdef __cplusplus
struct BEECRYPTAPI rg32Param
#else
struct _rg32Param
#endif
{
    unsigned char data[156];
    unsigned ix;
    uint32_t a[19];
    uint32_t b[39];
};
#ifndef __cplusplus
typedef struct _rg32Param rg32Param;
#endif

#define	RG64_SIZE	256	/* bits */
#ifdef __cplusplus
struct BEECRYPTAPI rg64Param
#else
struct _rg64Param
#endif
{
    unsigned char data[2 * 156];
    unsigned ix;
    uint64_t a[19];
    uint64_t b[39];
};
#ifndef __cplusplus
typedef struct _rg64Param rg64Param;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__RADIOGATUN_INTERNAL)
/**
 * Output size (in bits) for RadioGatun[32].
 */
#define SPH_SIZE_radiogatun32   256

/**
 * This structure is a context for RadioGatun[32] computations: it
 * contains intermediate values and some data from the last entered
 * block. Once a RadioGatun[32] computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running RadioGatun[32]
 * computation can be cloned by copying the context (e.g. with a
 * simple <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char data[156];   /* first field, for alignment */
	unsigned data_ptr;
	sph_u32 a[19], b[39];
#endif
} sph_radiogatun32_context;

/**
 * Initialize a RadioGatun[32] context. This process performs no
 * memory allocation.
 *
 * @param cc   the RadioGatun[32] context (pointer to a
 *             <code>sph_radiogatun32_context</code>)
 */
void sph_radiogatun32_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the RadioGatun[32] context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_radiogatun32(void *cc, const void *data, size_t len);

/**
 * Terminate the current RadioGatun[32] computation and output the
 * result into the provided buffer. The destination buffer must be wide
 * enough to accomodate the result (32 bytes). The context is
 * automatically reinitialized.
 *
 * @param cc    the RadioGatun[32] context
 * @param dst   the destination buffer
 */
void sph_radiogatun32_close(void *cc, void *dst);

/**
 * Output size (in bits) for RadioGatun[64].
 */
#define SPH_SIZE_radiogatun64   256

/**
 * This structure is a context for RadioGatun[64] computations: it
 * contains intermediate values and some data from the last entered
 * block. Once a RadioGatun[64] computation has been performed, the
 * context can be reused for another computation.
 *
 * The contents of this structure are private. A running RadioGatun[64]
 * computation can be cloned by copying the context (e.g. with a
 * simple <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char data[312];   /* first field, for alignment */
	unsigned data_ptr;
	sph_u64 a[19], b[39];
#endif
} sph_radiogatun64_context;

/**
 * Initialize a RadioGatun[64] context. This process performs no
 * memory allocation.
 *
 * @param cc   the RadioGatun[64] context (pointer to a
 *             <code>sph_radiogatun64_context</code>)
 */
void sph_radiogatun64_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the RadioGatun[64] context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void sph_radiogatun64(void *cc, const void *data, size_t len);

/**
 * Terminate the current RadioGatun[64] computation and output the
 * result into the provided buffer. The destination buffer must be wide
 * enough to accomodate the result (32 bytes). The context is
 * automatically reinitialized.
 *
 * @param cc    the RadioGatun[64] context
 * @param dst   the destination buffer
 */
void sph_radiogatun64_close(void *cc, void *dst);

#endif

BEECRYPTAPI
int rg32Init(rg32Param* sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int rg32Reset(rg32Param* sp)
	/*@*/;
BEECRYPTAPI
int rg32Update(rg32Param* sp, const byte *data, size_t size)
	/*@*/;
BEECRYPTAPI
int rg32Digest(rg32Param* sp, byte *digest)
	/*@*/;

BEECRYPTAPI
int rg64Init(rg64Param* sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int rg64Reset(rg64Param* sp)
	/*@*/;
BEECRYPTAPI
int rg64Update(rg64Param* sp, const byte *data, size_t size)
	/*@*/;
BEECRYPTAPI
int rg64Digest(rg64Param* sp, byte *digest)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* _RADIOGATUN_H */
