/* Reference code of ARIRANG 224/256/284/512 family (64 bit). */

#ifndef	_ARIRANG_H
#define	_ARIRANG_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the ARIRANG algorithm.
 * \ingroup HASH_arirang_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI arirangParam
#else
struct _arirangParam
#endif
{
    int hashbitlen;		/* Hash length */
    uint64_t counter[2];	/* Counter */
    uint64_t count[2];		/* Count */
    uint8_t block[128];		/* Message block */
    uint64_t workingvar[8];	/* Working variables */
    uint32_t blocklen;		/* hash block length */
    uint32_t remainderbit;	/* bit_length % 7 */
};

#ifndef __cplusplus
typedef struct _arirangParam arirangParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var shabal256
 * \brief Holds the full API description of the ARIRANG algorithm.
 */
extern BEECRYPTAPI const hashFunction arirang256;

BEECRYPTAPI
int arirangInit(arirangParam* sp, int hashbitlen);

BEECRYPTAPI
int arirangReset(arirangParam* sp);

BEECRYPTAPI
int arirangUpdate(arirangParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int arirangDigest(arirangParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif
