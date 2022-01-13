/* This program gives the reference implementation of JH.
 * 1) The description given in this program is suitable for implementation in hardware
 * 2) All the operations are operated on bytes, so the description is suitable for implementation on 8-bit processor
 */

#ifndef _JH_H
#define _JH_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

#if  defined(__SSE2__)
#define OPTIMIZE_SSE2
#endif

#if defined(OPTIMIZE_SSE2)
#include <emmintrin.h>
#define DECLSPEC16      __declspec(align(16))
#else
#define DECLSPEC16
#endif

/*!\brief Holds all the parameters necessary for the JH algorithm.
 * \ingroup HASH_jh_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI jhParam
#else
struct _jhParam
#endif
{
    int hashbitlen;		/* message digest size*/
    uint64_t databitlen;	/* message size in bits*/
#if defined(OPTIMIZE_SSE2)
    /* Only the x0/x1/x2/x3 are used for JH-224 and JH-256 */
    __m128i  x0,x1,x2,x3,x4,x5,x6,x7;
#else
    uint8_t H[128];		/* hash value H; 128 bytes;*/
    uint8_t A[256];		/* temporary round value; 256 4-bit elements*/
    unsigned char roundconstant[64];	/* round constant for one round; 64 4-bit elements*/
#endif
    unsigned char buffer[64];	/* message block to be hashed; 64 bytes*/
};

#ifndef __cplusplus
typedef struct _jhParam jhParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var jh256
 * \brief Holds the full API description of the JH algorithm.
 */
extern BEECRYPTAPI const hashFunction jh256;

BEECRYPTAPI
int jhInit(jhParam* sp, int hashbitlen);

BEECRYPTAPI
int jhReset(jhParam* sp);

BEECRYPTAPI
int jhUpdate(jhParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int jhDigest(jhParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif /* _JH_H */
