#ifndef _FUGUE_H
#define _FUGUE_H

#include <string.h>
#include <stdint.h>
#include <beecrypt/beecrypt.h>

typedef union {
    uint32_t    d;
    uint8_t     b[4];
} hash32_s;
typedef hash32_s* hash32_p;

typedef struct {
    int        n;	/* columns in output */
    int        s;	/* columns in state */
    int        k;	/* number of smix's per TIX or round */
    int        r;	/* number of G1 rounds in final part */
    int        t;	/* number of G2 rounds in final part */
} hashCfg;

/*!\brief Holds all the parameters necessary for the FUGUE algorithm.
 * \ingroup HASH_fugue_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI fugueParam
#else
struct _fugueParam
#endif
{
    int            hashbitlen;
    const hashCfg* Cfg;
    int            Base;
    hash32_s       State[36];
    uint32_t       Partial[1];
    uint64_t       TotalBits;
};

#ifndef __cplusplus
typedef struct _fugueParam fugueParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var fugue256
 * \brief Holds the full API description of the FUGUE algorithm.
 */
extern BEECRYPTAPI const hashFunction fugue256;

BEECRYPTAPI
int fugueInit(fugueParam *sp, int hashbitlen);

BEECRYPTAPI
int fugueReset(fugueParam *sp);

BEECRYPTAPI
int fugueUpdate(fugueParam *sp, const byte *data, size_t size);

BEECRYPTAPI
int fugueDigest(fugueParam *sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif
