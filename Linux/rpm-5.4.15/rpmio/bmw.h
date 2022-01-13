#ifndef _BMW_H
#define _BMW_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

// Specific algorithm definitions
#define BlueMidnightWish224_DIGEST_SIZE  28
#define BlueMidnightWish224_BLOCK_SIZE   64
#define BlueMidnightWish256_DIGEST_SIZE  32
#define BlueMidnightWish256_BLOCK_SIZE   64
#define BlueMidnightWish384_DIGEST_SIZE  48
#define BlueMidnightWish384_BLOCK_SIZE  128
#define BlueMidnightWish512_DIGEST_SIZE  64
#define BlueMidnightWish512_BLOCK_SIZE  128

typedef struct {
    uint32_t DoublePipe[32];
    byte LastPart[BlueMidnightWish256_BLOCK_SIZE * 2];
} bmwData256;
typedef struct {
    uint64_t DoublePipe[32];
    byte LastPart[BlueMidnightWish512_BLOCK_SIZE * 2];
} bmwData512;

/*!\brief Holds all the parameters necessary for the BlueMidnightWish algorithm.
 * \ingroup HASH_bmw_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI bmwParam
#else
struct _bmwParam
#endif
{
    int hashbitlen;

    // + algorithm specific parameters
    uint64_t bits_processed;
    union { 
	bmwData256  p256[1];
	bmwData512  p512[1];
    } pipe[1];
    int unprocessed_bits;
};

#ifndef __cplusplus
typedef struct _bmwParam bmwParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var bmw256
 * \brief Holds the full API description of the BlueMidnightWish algorithm.
 */
extern BEECRYPTAPI const hashFunction bmw256;

BEECRYPTAPI
int bmwInit(bmwParam* sp, int hashbitlen);

BEECRYPTAPI
int bmwReset(bmwParam* sp);

BEECRYPTAPI
int bmwUpdate(bmwParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int bmwDigest(bmwParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif /* _BMW_H */
