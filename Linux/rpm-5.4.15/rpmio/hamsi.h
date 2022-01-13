#ifndef HAMSI_H
#define HAMSI_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the HAMSI algorithm.
 * \ingroup HASH_hamsi_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI hamsiParam
#else
struct _hamsiParam
#endif
{
    int hashbitlen;
    int leftbits;
    uint8_t leftdata[8];
    uint32_t state[16];     // chain value
    uint32_t counter;

    int cvsize;
    // int blocksize;
    int ROUNDS;
    int PFROUNDS;
};

#ifndef __cplusplus
typedef struct _hamsiParam hamsiParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var hamsi256
 * \brief Holds the full API description of the HAMSI algorithm.
 */
extern BEECRYPTAPI const hashFunction hamsi256;

BEECRYPTAPI
int hamsiInit(hamsiParam* sp, int hashbitlen);

BEECRYPTAPI
int hamsiReset(hamsiParam* sp);

BEECRYPTAPI
int hamsiUpdate(hamsiParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int hamsiDigest(hamsiParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif
