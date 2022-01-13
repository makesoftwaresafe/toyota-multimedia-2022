/*******************************/
/* luffa.h                     */
/* Version 1.1 (Feb 12th 2009) */
/* Made by Hitachi Ltd.        */
/*******************************/

#ifndef _LUFFA_H
#define _LUFFA_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the LUFFA algorithm.
 * \ingroup HASH_luffa_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI luffaParam
#else
struct _luffaParam
#endif
{
    int hashbitlen;
    int width;			/* Number of blocks in chaining values */
    int limit;			/* Limit of message length in unit of 64bit */
    uint64_t bitlen[2];		/* Message length in bits */
    uint32_t rembitlen;		/* Length of buffer data to be hashed */
    uint32_t buffer[8];		/* Buffer to be hashed */
    uint32_t chainv[40];	/* Chaining values */
};

#ifndef __cplusplus
typedef struct _luffaParam luffaParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var luffa256
 * \brief Holds the full API description of the LUFFA algorithm.
 */
extern BEECRYPTAPI const hashFunction luffa256;

BEECRYPTAPI
int luffaInit(luffaParam* sp, int hashbitlen);

BEECRYPTAPI
int luffaReset(luffaParam* sp);

BEECRYPTAPI
int luffaUpdate(luffaParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int luffaDigest(luffaParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif /* _LUFFA_H */
