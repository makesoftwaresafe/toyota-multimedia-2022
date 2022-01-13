/*
 * Implementation of the Shabal hash function (header file). This header
 * is used for both the "reference" and the "optimized" implementations.
 *
 * (c) 2008 SAPHIR project. This software is provided 'as-is', without
 * any express or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to no restriction.
 *
 * Technical remarks and questions can be addressed to:
 * <thomas.pornin@cryptolog.com>
 */

#ifndef _SHABAL_H
#define _SHABAL_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the SHABAL algorithm.
 * \ingroup HASH_shabal_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI shabalParam
#else
struct _shabalParam
#endif
{
    byte buffer[16 * 4];	/* SHABAL_BLOCK_SIZE */
    size_t buffer_ptr;
    unsigned last_byte_significant_bits;
    int hashbitlen;
    uint32_t A[12];		/* SHABAL_PARAM_R */
    uint32_t B[16];		/* SHABAL_BLOCK_SIZE */
    uint32_t C[16];		/* SHABAL_BLOCK_SIZE */
    uint32_t Whigh, Wlow;
};

#ifndef __cplusplus
typedef struct _shabalParam shabalParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var shabal256
 * \brief Holds the full API description of the SHABAL algorithm.
 */
extern BEECRYPTAPI const hashFunction shabal256;

BEECRYPTAPI
int shabalInit(shabalParam* sp, int hashbitlen);

BEECRYPTAPI
int shabalReset(shabalParam* sp);

BEECRYPTAPI
int shabalUpdate(shabalParam* sp, const byte *data, size_t size);

BEECRYPTAPI
int shabalDigest(shabalParam* sp, byte *digest);

#ifdef __cplusplus
}
#endif

#endif
