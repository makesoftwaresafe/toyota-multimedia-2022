/* laneref.h   v1.1   October 2008
 * Reference ANSI C implementation of LANE
 * Based on the AES reference implementation of Paulo Barreto and Vincent Rijmen.
 * Author: Nicky Mouha
 * This code is placed in the public domain.
 */

#ifndef H_LANE
#define H_LANE

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the LANE algorithm.
 * \ingroup HASH_lane_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI laneParam
#else
struct _laneParam
#endif
{
    int hashbitlen;		/* length in bits of the hash value */
    uint64_t ctr;		/* number of data bits processed so far */
    uint32_t h[64/4];		/* intermediate hash value */
    uint8_t buffer[128];	/* space for one block of data to be hashed */
};

#ifndef __cplusplus
typedef struct _laneParam laneParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var chi256
 * \brief Holds the full API description of the LANE algorithm.
 */
extern BEECRYPTAPI const hashFunction lane256;

BEECRYPTAPI
int laneInit(laneParam* sp, int hashbitlen);

BEECRYPTAPI
int laneReset(laneParam* sp);

BEECRYPTAPI
int laneUpdate(laneParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int laneDigest(laneParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif /* H_LANE */
