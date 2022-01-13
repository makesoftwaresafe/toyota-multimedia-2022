#ifndef	_BLAKE_H
#define	_BLAKE_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the BLAKE algorithm.
 * \ingroup HASH_blake_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI blakeParam
#else
struct _blakeParam
#endif
{ 
    int hashbitlen;  /* length of the hash value (bits) */
    unsigned int datalen;     /* amount of remaining data to hash (bits) */
    int init;        /* set to 1 when initialized */
    int nullt;       /* Boolean value for special case \ell_i=0 */
    /* variables for the 32-bit version  */
    uint32_t h32[8];         /* current chain value (initialized to the IV) */
    uint32_t t32[2];         /* number of bits hashed so far */
    uint8_t data32[64];      /* remaining data to hash (less than a block) */
    uint32_t salt32[4];      /* salt (null by default) */
    /* variables for the 64-bit version */
    uint64_t h64[8];      /* current chain value (initialized to the IV) */
    uint64_t t64[2];      /* number of bits hashed so far */
    uint8_t data64[128];  /* remaining data to hash (less than a block) */
    uint64_t salt64[4];   /* salt (null by default) */
};

#ifndef __cplusplus
typedef struct _blakeParam blakeParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var blake256
 * \brief Holds the full API description of the BLAKE algorithm.
 */
extern BEECRYPTAPI const hashFunction blake256;

BEECRYPTAPI
int blakeInit(blakeParam * sp, int hashbitlen);

BEECRYPTAPI
int blakeReset(blakeParam * sp);

BEECRYPTAPI
int blakeUpdate(blakeParam * sp, const byte * data, size_t size);

BEECRYPTAPI
int blakeDigest(blakeParam * sp, byte * digest);

#ifdef __cplusplus
}
#endif

#endif /* _BLAKE_H */
