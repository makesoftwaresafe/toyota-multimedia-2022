/*******************************/
/* luffa.c                     */
/* Version 1.1 (Feb 12th 2009) */
/* Made by Hitachi Ltd.        */
/*******************************/

#include "luffa.h"

#define	OPTIMIZED

const hashFunction luffa256 = {
    .name = "LUFFA-256",
    .paramsize = sizeof(luffaParam),
    .blocksize = 64,
    .digestsize = 256/8,        /* XXX default to LUFFA-256 */
    .reset = (hashFunctionReset) luffaReset,
    .update = (hashFunctionUpdate) luffaUpdate,
    .digest = (hashFunctionDigest) luffaDigest
};

enum { SUCCESS = 0, FAIL = 1, BAD_HASHBITLEN = 2};

/*********************************/
/* The parameters of Luffa       */
#define MSG_BLOCK_BIT_LEN 256  /* The bit length of a message block */
#define MSG_BLOCK_BYTE_LEN (MSG_BLOCK_BIT_LEN >> 3) /* The byte length
                                                     * of a message block*/
/* The number of blocks in Luffa */
#define WIDTH_224 3
#define WIDTH_256 3
#define WIDTH_384 4
#define WIDTH_512 5
/* The limit of the length of message */
#define LIMIT_224 64
#define LIMIT_256 64
#define LIMIT_384 128
#define LIMIT_512 128
/*********************************/

#define __XBIG_ENDIAN	4321
#define __XLITTLE_ENDIAN	1234
#ifdef  WORDS_BIGENDIAN
#define __XBYTE_ORDER	4321
#else
#define __XBYTE_ORDER	1234
#endif

#if	__XBYTE_ORDER == __XBIG_ENDIAN
# define BYTES_SWAP32(x) x
# define BYTES_SWAP64(x) x
#else
# define BYTES_SWAP32(x)                                                \
    ((x << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | (x >> 24))

# define BYTES_SWAP64(x)                                                \
    (((uint64_t)(BYTES_SWAP32((uint32_t)(x))) << 32) | (uint64_t)(BYTES_SWAP32((uint32_t)((x) >> 32))))
#endif

/* BYTES_SWAP256(x) stores each 32-bit word of 256 bits data in little-endian convention */
#define BYTES_SWAP256(x) {                                      \
	int _i = 8; while(_i--) { x[_i] = BYTES_SWAP32(x[_i]); }\
    }

/* BYTES_SWAP512(x) stores each 64-bit word of 512 bits data in little-endian convention */
#define BYTES_SWAP512(x) {                                      \
	int _i = 8; while(_i--) { x[_i] = BYTES_SWAP64(x[_i]); }\
    }

/* initial values of chaining variables */
static
const uint32_t IV[40] = {
    0x6d251e69, 0x44b051e0, 0x4eaa6fb4, 0xdbf78465,
    0x6e292011, 0x90152df4, 0xee058139, 0xdef610bb,
    0xc3b44b95, 0xd9d2f256, 0x70eee9a0, 0xde099fa3,
    0x5d9b0557, 0x8fc944b3, 0xcf1ccf0e, 0x746cd581,
    0xf7efc89d, 0x5dba5781, 0x04016ce5, 0xad659c05,
    0x0306194f, 0x666d1836, 0x24aa230a, 0x8b264ae7,
    0x858075d5, 0x36d79cce, 0xe571f7d7, 0x204b1f67,
    0x35870c6a, 0x57e9e923, 0x14bcb808, 0x7cde72ce,
    0x6c68e9be, 0x5ec41e22, 0xc825b7c7, 0xaffb4363,
    0xf5df3999, 0x0fc688f1, 0xb07224cc, 0x03e86cea
};

/* initial values of constant generators */
#ifdef	OPTIMIZED
static
const uint32_t CNS[80] = {
    0x303994a6, 0xe0337818, 0xc0e65299, 0x441ba90d,
    0x6cc33a12, 0x7f34d442, 0xdc56983e, 0x9389217f,
    0x1e00108f, 0xe5a8bce6, 0x7800423d, 0x5274baf4,
    0x8f5b7882, 0x26889ba7, 0x96e1db12, 0x9a226e9d,
    0xb6de10ed, 0x01685f3d, 0x70f47aae, 0x05a17cf4,
    0x0707a3d4, 0xbd09caca, 0x1c1e8f51, 0xf4272b28,
    0x707a3d45, 0x144ae5cc, 0xaeb28562, 0xfaa7ae2b,
    0xbaca1589, 0x2e48f1c1, 0x40a46f3e, 0xb923c704,
    0xfc20d9d2, 0xe25e72c1, 0x34552e25, 0xe623bb72,
    0x7ad8818f, 0x5c58a4a4, 0x8438764a, 0x1e38e2e7,
    0xbb6de032, 0x78e38b9d, 0xedb780c8, 0x27586719,
    0xd9847356, 0x36eda57f, 0xa2c78434, 0x703aace7,
    0xb213afa5, 0xe028c9bf, 0xc84ebe95, 0x44756f91,
    0x4e608a22, 0x7e8fce32, 0x56d858fe, 0x956548be,
    0x343b138f, 0xfe191be2, 0xd0ec4e3d, 0x3cb226e5,
    0x2ceb4882, 0x5944a28e, 0xb3ad2208, 0xa1c4c355,
    0xf0d2e9e3, 0x5090d577, 0xac11d7fa, 0x2d1925ab,
    0x1bcb66f2, 0xb46496ac, 0x6f2d9bc9, 0xd1925ab0,
    0x78602649, 0x29131ab6, 0x8edae952, 0x0fc053c3,
    0x3b6ba548, 0x3f014f0c, 0xedae9520, 0xfc053c31
};
#else
static
const uint32_t CNS[10] = {
    0x181cca53, 0x380cde06, 0x5b6f0876, 0xf16f8594,
    0x7e106ce9, 0x38979cb0, 0xbb62f364, 0x92e93c29,
    0x9a025047, 0xcff2a940
};

#endif

/***************************************************/
/* Multiplication by 2 on GF(2^8)^32 */
/* a[8]: chaining value */
static inline
void mult2(uint32_t a[8])
{
    uint32_t tmp;

    tmp = a[7];
    a[7] = a[6];
    a[6] = a[5];
    a[5] = a[4];
    a[4] = a[3] ^ tmp;
    a[3] = a[2] ^ tmp;
    a[2] = a[1];
    a[1] = a[0] ^ tmp;
    a[0] = tmp;

    return;
}

/***************************************************/
/* Message Insertion function */
/* sp: hash context     */
static inline
void mi(luffaParam *sp)
{
    uint32_t t[40];
    int i, j;

    /* mixing layer w=3,4,5 */
    for (i = 0; i < 8; i++) {
	t[i]=0;
	for (j = 0; j < sp->width; j++)
	    t[i] ^= sp->chainv[i+8*j];
    }
    mult2(t);

    for (j = 0; j < sp->width; j++) {
	for (i = 0; i < 8; i++)
	    sp->chainv[i+8*j] ^= t[i];
    }

    /* mixing layer w=5 */
    if (sp->width>=5) {
	for (j = 0; j< sp->width; j++) {
	    for (i = 0; i < 8; i++)
		t[i+8*j] = sp->chainv[i+8*j];
	    mult2(&sp->chainv[8*j]);
	}
	for (j = 0; j < sp->width; j++) {
	    for (i = 0; i < 8; i++)
		sp->chainv[8*j+i] ^= t[8*((j+1)%sp->width)+i];
	}
    }

    /* mixing layer w=4,5 */
    if (sp->width >= 4) {
	for (j = 0; j < sp->width; j++) {
	    for (i = 0; i < 8; i++)
		t[i+8*j] = sp->chainv[i+8*j];
	    mult2(&sp->chainv[8*j]);
	}
	for (j = 0; j < sp->width; j++) {
	    for (i = 0; i < 8; i++)
		sp->chainv[8*j+i] ^= t[8*((j-1+sp->width)%sp->width)+i];
	}
    }

    /* message addition */
    for (j = 0; j < sp->width; j++) {
	for (i = 0; i < 8; i++)
	    sp->chainv[i+8*j] ^= sp->buffer[i];
	mult2(sp->buffer);
    }
  
    return;
}

/***************************************************/
#ifdef	OPTIMIZED
#define TWEAK(a,j)                           \
  { *(a+4) = (*(a+4)<<(j))|(*(a+4)>>(32-j)); \
    *(a+5) = (*(a+5)<<(j))|(*(a+5)>>(32-j)); \
    *(a+6) = (*(a+6)<<(j))|(*(a+6)>>(32-j)); \
    *(a+7) = (*(a+7)<<(j))|(*(a+7)>>(32-j)); }
#else
/* Tweaks */
static inline
void tweak(luffaParam *sp) {
    int i,j;

    for (j = 0; j < sp->width; j++) {
	for (i = 4; i < 8; i++)
	    sp->chainv[8*j+i] =
		(sp->chainv[8*j+i]<<j) | (sp->chainv[8*j+i] >> (32-j));
    }

    return;
}
#endif

/***************************************************/
/* SubCrumb function                              */
/* a[4]: input and output of SubCrumb             */
/* (a[0],a[1],a[2],a[3]) -> (a[0],a[1],a[2],a[3]) */
#ifdef	OPTIMIZED
#define SUBCRUMB(a0,a1,a2,a3,a4) \
  { a4  = a0; \
    a2 ^= a1; \
    a0 &= a1; \
    a0 ^= a2; \
    a1  = ~a1;\
    a2 |= a4; \
    a2 ^= a3; \
    a4 ^= a0; \
    a3 &= a0; \
    a3 ^= a1; \
    a4  = ~a4;\
    a1 |= a2; \
    a4 ^= a1; \
    a0 ^= a3; \
    a1 &= a2; \
    a1 ^= a3; \
    a2  = a3; \
    a3  = a4; }
#else
static inline
void subcrumb(uint32_t *a)
{
    uint32_t tmp;
  
    tmp   = a[0];
    a[2] ^= a[1];
    a[0] &= a[1];
    a[0] ^= a[2];
    a[1]  = ~a[1];
    a[2] |= tmp;
    a[2] ^= a[3];
    tmp  ^= a[0];
    a[3] &= a[0];
    a[3] ^= a[1];
    tmp   = ~tmp;
    a[1] |= a[2];
    tmp  ^= a[1];
    a[0] ^= a[3];
    a[1] &= a[2];
    a[1] ^= a[3];
    a[2]  = a[3];
    a[3]  = tmp;
  
    return;
}
#endif

/***************************************************/
/* MixWord function                                 */
/* a[0],a[4]: input and outputs of MixWord function */
/* (a[0],a[4]) -> (a[0],a[4])                       */
#ifdef	OPTIMIZED
#define MIXWORD(a0,a4)\
  { a4 ^= a0; \
    a0  = (a0<<2) | (a0>>(30)); \
    a0 ^= a4; \
    a4  = (a4<<14) | (a4>>(18));\
    a4 ^= a0; \
    a0  = (a0<<10) | (a0>>(22));\
    a0 ^= a4; \
    a4  = (a4<<1) | (a4>>(31)); }
#else
static inline
void mixword(uint32_t *a)
{
    a[4] ^= a[0];
    a[0]  = (a[0]<< 2) | (a[0] >> (32-2));
    a[0] ^= a[4];
    a[4]  = (a[4]<<14) | (a[4] >> (32-14));
    a[4] ^= a[0];
    a[0]  = (a[0]<<10) | (a[0] >> (32-10));
    a[0] ^= a[4];
    a[4]  = (a[4]<< 1) | (a[4] >> (32-1));

    return;
}
#endif

/***************************************************/
/* AddConstant function */
/* a[0],a[4]: chaining values */
/* cns[2]: round constants */
#ifdef	OPTIMIZED
#define ADD_CONSTANT(a0, b0, c) \
  { a0 ^= *c;\
    b0 ^= *(c+1); }
#else
static inline
void addconstant(uint32_t *a, uint32_t *cns)
{
    a[0] ^= cns[0];
    a[4] ^= cns[1];
  
    return;
}
#endif

/***************************************************/
/* Constant Generator */
/* This function generates
   round constants of a sub-permutation at a round. */
/* c[2]: the variables which LFSR possess at the round. */
/* cns[2]: the round constants at the round which are generated
   through two LFSR-round calculation. */
#if !defined(OPTIMIZED)
static inline
void genconstant(uint32_t *c, uint32_t *cns)
{
    if(c[0] >> 31) {
	c[0] = (c[0]<<1) ^ (c[1] >> 31) ^ 0xc4d6496c;
	c[1] = (c[1]<<1) ^ 0x55c61c8d;
    } else {
	c[0] = (c[0]<<1) ^ (c[1] >> 31);
	c[1] <<= 1;
    }
    cns[0] = c[0];
    c[0] = c[1];
    c[1] = cns[0];
    if(c[0] >> 31) {
	c[0] = (c[0]<<1) ^ (c[1] >> 31) ^ 0xc4d6496c;
	c[1] = (c[1]<<1) ^ 0x55c61c8d;
    } else {
	c[0] = (c[0]<<1) ^ (c[1] >> 31);
	c[1] <<= 1;
    }
    cns[1] = c[0];
    c[0] = c[1];
    c[1] = cns[1];
  
    return;
}
#endif

/***************************************************/
/* Step function of a Sub-Permutation              */
/* a[8]: chaining values of a Sub-Permutation line */
/* c[2] : constant of a Sub-Permutation line       */
#ifdef	OPTIMIZED
#define STEP_PART(a, c, tmp)                        \
  { SUBCRUMB(*a,*(a+1),*(a+2),*(a+3),tmp);          \
    SUBCRUMB(*(a+4),*(a+5),*(a+6),*(a+7),tmp);      \
    MIXWORD(*a,*(a+4));                             \
    MIXWORD(*(a+1),*(a+5));                         \
    MIXWORD(*(a+2),*(a+6));                         \
    MIXWORD(*(a+3),*(a+7));                         \
    ADD_CONSTANT(*a,*(a+4),c); }
#else
static inline
void step_part(uint32_t *a, uint32_t *c)
{
    uint32_t cns[2];
  
    subcrumb(a  );
    subcrumb(a+4);
    mixword(a  );
    mixword(a+1);
    mixword(a+2);
    mixword(a+3);
    genconstant(c, cns);
    addconstant(a, cns);
  
    return;
}
#endif

/***************************************************/
#if !defined(OPTIMIZED)
/* Step function          */
/* sp: hash context    */
/* c[6] : constant        */
static inline
void step(luffaParam *sp, uint32_t *c)
{
    int j;

    for (j = 0; j < sp->width; j++)
	step_part(sp->chainv + 8*j, c+2*j);

    return;
}
#endif

/***************************************************/
/* Round function(s)      */
/* sp: hash context    */
/* c[6] : constant        */
#ifdef	OPTIMIZED
static inline
void rnd256(luffaParam *sp)
{
    int i,j;
    uint32_t t[8];
    uint32_t chainv[24];
    uint32_t buffer[8];

    for (i = 0; i < 24;i++)
        chainv[i] = sp->chainv[i];

    for (i = 0; i < 8; i++) {
        t[i] = 0;
        for (j = 0; j < 3; j++)
            t[i] ^= chainv[i+8*j];
    }

    t[3] ^= t[7];
    t[2] ^= t[7];
    t[0] ^= t[7];

    chainv[ 0] ^= t[7];
    chainv[ 8] ^= t[7];
    chainv[16] ^= t[7];
    chainv[ 1] ^= t[0];
    chainv[ 9] ^= t[0];
    chainv[17] ^= t[0];
    chainv[ 2] ^= t[1];
    chainv[10] ^= t[1];
    chainv[18] ^= t[1];
    chainv[ 3] ^= t[2];
    chainv[11] ^= t[2];
    chainv[19] ^= t[2];
    chainv[ 4] ^= t[3];
    chainv[12] ^= t[3];
    chainv[20] ^= t[3];
    chainv[ 5] ^= t[4];
    chainv[13] ^= t[4];
    chainv[21] ^= t[4];
    chainv[ 6] ^= t[5];
    chainv[14] ^= t[5];
    chainv[22] ^= t[5];
    chainv[ 7] ^= t[6];
    chainv[15] ^= t[6];
    chainv[23] ^= t[6];

    for (i = 0; i < 8; i++)
        buffer[i] = sp->buffer[i];

    chainv[ 0] ^= buffer[0];
    chainv[ 1] ^= buffer[1];
    chainv[ 2] ^= buffer[2];
    chainv[ 3] ^= buffer[3];
    chainv[ 4] ^= buffer[4];
    chainv[ 5] ^= buffer[5];
    chainv[ 6] ^= buffer[6];
    chainv[ 7] ^= buffer[7];
    chainv[ 8] ^= buffer[7];
    chainv[ 9] ^= buffer[7];
    chainv[ 9] ^= buffer[0];
    chainv[10] ^= buffer[1];
    chainv[11] ^= buffer[2];
    chainv[11] ^= buffer[7];
    chainv[12] ^= buffer[7];
    chainv[12] ^= buffer[3];
    chainv[13] ^= buffer[4];
    chainv[14] ^= buffer[5];
    chainv[15] ^= buffer[6];
    chainv[16] ^= buffer[6];
    chainv[17] ^= buffer[6];
    chainv[17] ^= buffer[7];
    chainv[18] ^= buffer[7];
    chainv[18] ^= buffer[0];
    chainv[19] ^= buffer[1];
    chainv[19] ^= buffer[6];
    chainv[20] ^= buffer[6];
    chainv[20] ^= buffer[2];
    chainv[20] ^= buffer[7];
    chainv[21] ^= buffer[7];
    chainv[21] ^= buffer[3];
    chainv[22] ^= buffer[4];
    chainv[23] ^= buffer[5];
      
    for (j = 0; j < 3; j++) {
        TWEAK(&chainv[8*j],j);
        for (i = 0; i < 8; i++)
            STEP_PART(&chainv[8*j], &CNS[2*i + 16*j], t[0]);
    }

    for (i = 0; i < 24; i++)
        sp->chainv[i] = chainv[i];

    return;
}

static inline
void rnd384(luffaParam *sp)
{
    int i,j;
    uint32_t t[9];
    uint32_t chainv[32];
    uint32_t buffer[8];

    for (i = 0; i < 32; i++)
        chainv[i] = sp->chainv[i];

    for (i = 0; i < 8; i++) {
        t[i]=0;
        for (j = 0; j < 4; j++)
            t[i] ^= chainv[i+8*j];
    }

    t[3] ^= t[7];
    t[2] ^= t[7];
    t[0] ^= t[7];

    chainv[ 0] ^= t[7];
    chainv[ 8] ^= t[7];
    chainv[16] ^= t[7];
    chainv[24] ^= t[7];
    chainv[ 1] ^= t[0];
    chainv[ 9] ^= t[0];
    chainv[17] ^= t[0];
    chainv[25] ^= t[0];
    chainv[ 2] ^= t[1];
    chainv[10] ^= t[1];
    chainv[18] ^= t[1];
    chainv[26] ^= t[1];
    chainv[ 3] ^= t[2];
    chainv[11] ^= t[2];
    chainv[19] ^= t[2];
    chainv[27] ^= t[2];
    chainv[ 4] ^= t[3];
    chainv[12] ^= t[3];
    chainv[20] ^= t[3];
    chainv[28] ^= t[3];
    chainv[ 5] ^= t[4];
    chainv[13] ^= t[4];
    chainv[21] ^= t[4];
    chainv[29] ^= t[4];
    chainv[ 6] ^= t[5];
    chainv[14] ^= t[5];
    chainv[22] ^= t[5];
    chainv[30] ^= t[5];
    chainv[ 7] ^= t[6];
    chainv[15] ^= t[6];
    chainv[23] ^= t[6];
    chainv[31] ^= t[6];

    t[7] = chainv[7];
    chainv[7] = chainv[6] ^ chainv[31];
    t[6] = chainv[6];
    chainv[6] = chainv[5] ^ chainv[30];
    t[5] = chainv[5];
    chainv[5] = chainv[4] ^ chainv[29];
    t[4] = chainv[4];
    chainv[4] = chainv[3] ^ t[7] ^ chainv[28];
    t[3] = chainv[3];
    chainv[3] = chainv[2] ^ t[7] ^ chainv[27];
    t[2] = chainv[2];
    chainv[2] = chainv[1] ^ chainv[26];
    t[1] = chainv[1];
    chainv[1] = chainv[0] ^ t[7] ^ chainv[25];
    t[0] = chainv[0];
    chainv[0] = t[7] ^ chainv[24];

    t[8] = chainv[31];
    chainv[31] = chainv[30] ^ chainv[23];
    chainv[30] = chainv[29] ^ chainv[22];
    chainv[29] = chainv[28] ^ chainv[21];
    chainv[28] = chainv[27] ^ t[8] ^ chainv[20];
    chainv[27] = chainv[26] ^ t[8] ^ chainv[19];
    chainv[26] = chainv[25] ^ chainv[18];
    chainv[25] = chainv[24] ^ t[8] ^ chainv[17];
    chainv[24] = t[8] ^ chainv[16];

    t[8] = chainv[23];
    chainv[23] = chainv[22] ^ chainv[15];
    chainv[22] = chainv[21] ^ chainv[14];
    chainv[21] = chainv[20] ^ chainv[13];
    chainv[20] = chainv[19] ^ t[8] ^ chainv[12];
    chainv[19] = chainv[18] ^ t[8] ^ chainv[11];
    chainv[18] = chainv[17] ^ chainv[10];
    chainv[17] = chainv[16] ^ t[8] ^ chainv[9];
    chainv[16] = t[8] ^ chainv[8];

    t[8] = chainv[15];
    chainv[15] = chainv[14] ^ t[7];
    chainv[14] = chainv[13] ^ t[6];
    chainv[13] = chainv[12] ^ t[5];
    chainv[12] = chainv[11] ^ t[8] ^ t[4];
    chainv[11] = chainv[10] ^ t[8] ^ t[3];
    chainv[10] = chainv[9] ^ t[2];
    chainv[9] = chainv[8] ^ t[8] ^ t[1];
    chainv[8] = t[8] ^ t[0];

    for (i = 0; i < 8; i++)
        buffer[i] = sp->buffer[i];

    chainv[ 0] ^= buffer[0];
    chainv[ 2] ^= buffer[2];
    chainv[ 3] ^= buffer[3];

    buffer[ 0] ^= buffer[7];
    buffer[ 2] ^= buffer[7];
    buffer[ 3] ^= buffer[7];

    chainv[ 7] ^= buffer[7];
    chainv[ 8] ^= buffer[7];
    chainv[ 1] ^= buffer[1];
    chainv[10] ^= buffer[1];
    chainv[11] ^= buffer[2];

    buffer[ 7] ^= buffer[6];
    buffer[ 1] ^= buffer[6];
    buffer[ 2] ^= buffer[6];

    chainv[ 6] ^= buffer[6];
    chainv[15] ^= buffer[6];
    chainv[16] ^= buffer[6];
    chainv[ 9] ^= buffer[0];
    chainv[18] ^= buffer[0];
    chainv[19] ^= buffer[1];

    buffer[ 6] ^= buffer[5];
    buffer[ 0] ^= buffer[5];
    buffer[ 1] ^= buffer[5];

    chainv[ 5] ^= buffer[5];
    chainv[14] ^= buffer[5];
    chainv[23] ^= buffer[5];
    chainv[24] ^= buffer[5];
    chainv[25] ^= buffer[6];
    chainv[17] ^= buffer[7];
    chainv[26] ^= buffer[7];
    chainv[27] ^= buffer[0];
    chainv[28] ^= buffer[1];
    chainv[20] ^= buffer[2];
    chainv[29] ^= buffer[2];
    chainv[12] ^= buffer[3];
    chainv[21] ^= buffer[3];
    chainv[30] ^= buffer[3];
    chainv[ 4] ^= buffer[4];
    chainv[13] ^= buffer[4];
    chainv[22] ^= buffer[4];
    chainv[31] ^= buffer[4];

    for (j = 0; j < 4; j++) {
        TWEAK(&chainv[8*j],j);
        for (i = 0; i < 8; i++)
            STEP_PART(&chainv[8*j], &CNS[2*i + 16*j], t[0]);
    }

    for (i = 0; i < 32; i++)
        sp->chainv[i] = chainv[i];

    return;
}

static inline
void rnd512(luffaParam *sp)
{
    int i,j;
    uint32_t t[9];
    uint32_t chainv[40];
    uint32_t buffer[8];

    for (i = 0; i < 40; i++)
        chainv[i] = sp->chainv[i];

    for (i = 0; i < 8; i++) {
        t[i] = 0;
        for (j = 0; j < 5; j++)
            t[i] ^= chainv[i+8*j];
    }

    t[3] ^= t[7];
    t[2] ^= t[7];
    t[0] ^= t[7];

    chainv[ 0] ^= t[7];
    chainv[ 8] ^= t[7];
    chainv[16] ^= t[7];
    chainv[24] ^= t[7];
    chainv[32] ^= t[7];
    chainv[ 1] ^= t[0];
    chainv[ 9] ^= t[0];
    chainv[17] ^= t[0];
    chainv[25] ^= t[0];
    chainv[33] ^= t[0];
    chainv[ 2] ^= t[1];
    chainv[10] ^= t[1];
    chainv[18] ^= t[1];
    chainv[26] ^= t[1];
    chainv[34] ^= t[1];
    chainv[ 3] ^= t[2];
    chainv[11] ^= t[2];
    chainv[19] ^= t[2];
    chainv[27] ^= t[2];
    chainv[35] ^= t[2];
    chainv[36] ^= t[3];
    chainv[ 4] ^= t[3];
    chainv[12] ^= t[3];
    chainv[20] ^= t[3];
    chainv[28] ^= t[3];
    chainv[ 5] ^= t[4];
    chainv[13] ^= t[4];
    chainv[21] ^= t[4];
    chainv[29] ^= t[4];
    chainv[37] ^= t[4];
    chainv[ 6] ^= t[5];
    chainv[14] ^= t[5];
    chainv[22] ^= t[5];
    chainv[30] ^= t[5];
    chainv[38] ^= t[5];
    chainv[ 7] ^= t[6];
    chainv[15] ^= t[6];
    chainv[23] ^= t[6];
    chainv[31] ^= t[6];
    chainv[39] ^= t[6];

    t[7] = chainv[7];
    chainv[7] = chainv[6] ^ chainv[15];
    t[6] = chainv[6];
    chainv[6] = chainv[5] ^ chainv[14];
    t[5] = chainv[5];
    chainv[5] = chainv[4] ^ chainv[13];
    t[4] = chainv[4];
    chainv[4] = chainv[3] ^ t[7] ^ chainv[12];
    t[3] = chainv[3];
    chainv[3] = chainv[2] ^ t[7] ^ chainv[11];
    t[2] = chainv[2];
    chainv[2] = chainv[1] ^ chainv[10];
    t[1] = chainv[1];
    chainv[1] = chainv[0] ^ t[7] ^ chainv[9];
    t[0] = chainv[0];
    chainv[0] = t[7] ^ chainv[8];

    t[8] = chainv[15];
    chainv[15] = chainv[14] ^ chainv[23];
    chainv[14] = chainv[13] ^ chainv[22];
    chainv[13] = chainv[12] ^ chainv[21];
    chainv[12] = chainv[11] ^ t[8] ^ chainv[20];
    chainv[11] = chainv[10] ^ t[8] ^ chainv[19];
    chainv[10] = chainv[9] ^ chainv[18];
    chainv[9] = chainv[8] ^ t[8] ^ chainv[17];
    chainv[8] = t[8] ^ chainv[16];

    t[8] = chainv[23];
    chainv[23] = chainv[22] ^ chainv[31];
    chainv[22] = chainv[21] ^ chainv[30];
    chainv[21] = chainv[20] ^ chainv[29];
    chainv[20] = chainv[19] ^ t[8] ^ chainv[28];
    chainv[19] = chainv[18] ^ t[8] ^ chainv[27];
    chainv[18] = chainv[17] ^ chainv[26];
    chainv[17] = chainv[16] ^ t[8] ^ chainv[25];
    chainv[16] = t[8] ^ chainv[24];

    t[8] = chainv[31];
    chainv[31] = chainv[30] ^ chainv[39];
    chainv[30] = chainv[29] ^ chainv[38];
    chainv[29] = chainv[28] ^ chainv[37];
    chainv[28] = chainv[27] ^ t[8] ^ chainv[36];
    chainv[27] = chainv[26] ^ t[8] ^ chainv[35];
    chainv[26] = chainv[25] ^ chainv[34];
    chainv[25] = chainv[24] ^ t[8] ^ chainv[33];
    chainv[24] = t[8] ^ chainv[32];

    t[8] = chainv[39];
    chainv[39] = chainv[38] ^ t[7];
    chainv[38] = chainv[37] ^ t[6];
    chainv[37] = chainv[36] ^ t[5];
    chainv[36] = chainv[35] ^ t[8] ^ t[4];
    chainv[35] = chainv[34] ^ t[8] ^ t[3];
    chainv[34] = chainv[33] ^ t[2];
    chainv[33] = chainv[32] ^ t[8] ^ t[1];
    chainv[32] = t[8] ^ t[0];

    t[7] = chainv[7];
    chainv[7] = chainv[6] ^ chainv[39];
    t[6] = chainv[6];
    chainv[6] = chainv[5] ^ chainv[38];
    t[5] = chainv[5];
    chainv[5] = chainv[4] ^ chainv[37];
    t[4] = chainv[4];
    chainv[4] = chainv[3] ^ t[7] ^ chainv[36];
    t[3] = chainv[3];
    chainv[3] = chainv[2] ^ t[7] ^ chainv[35];
    t[2] = chainv[2];
    chainv[2] = chainv[1] ^ chainv[34];
    t[1] = chainv[1];
    chainv[1] = chainv[0] ^ t[7] ^ chainv[33];
    t[0] = chainv[0];
    chainv[0] = t[7] ^ chainv[32];

    t[8] = chainv[39];
    chainv[39] = chainv[38] ^ chainv[31];
    chainv[38] = chainv[37] ^ chainv[30];
    chainv[37] = chainv[36] ^ chainv[29];
    chainv[36] = chainv[35] ^ t[8] ^ chainv[28];
    chainv[35] = chainv[34] ^ t[8] ^ chainv[27];
    chainv[34] = chainv[33] ^ chainv[26];
    chainv[33] = chainv[32] ^ t[8] ^ chainv[25];
    chainv[32] = t[8] ^ chainv[24];

    t[8] = chainv[31];
    chainv[31] = chainv[30] ^ chainv[23];
    chainv[30] = chainv[29] ^ chainv[22];
    chainv[29] = chainv[28] ^ chainv[21];
    chainv[28] = chainv[27] ^ t[8] ^ chainv[20];
    chainv[27] = chainv[26] ^ t[8] ^ chainv[19];
    chainv[26] = chainv[25] ^ chainv[18];
    chainv[25] = chainv[24] ^ t[8] ^ chainv[17];
    chainv[24] = t[8] ^ chainv[16];

    t[8] = chainv[23];
    chainv[23] = chainv[22] ^ chainv[15];
    chainv[22] = chainv[21] ^ chainv[14];
    chainv[21] = chainv[20] ^ chainv[13];
    chainv[20] = chainv[19] ^ t[8] ^ chainv[12];
    chainv[19] = chainv[18] ^ t[8] ^ chainv[11];
    chainv[18] = chainv[17] ^ chainv[10];
    chainv[17] = chainv[16] ^ t[8] ^ chainv[9];
    chainv[16] = t[8] ^ chainv[8];

    t[8] = chainv[15];
    chainv[15] = chainv[14] ^ t[7];
    chainv[14] = chainv[13] ^ t[6];
    chainv[13] = chainv[12] ^ t[5];
    chainv[12] = chainv[11] ^ t[8] ^ t[4];
    chainv[11] = chainv[10] ^ t[8] ^ t[3];
    chainv[10] = chainv[9] ^ t[2];
    chainv[9] = chainv[8] ^ t[8] ^ t[1];
    chainv[8] = t[8] ^ t[0];

    for (i = 0; i < 8; i++)
        buffer[i] = sp->buffer[i];

    chainv[ 0] ^= buffer[0];
    chainv[ 2] ^= buffer[2];
    chainv[ 3] ^= buffer[3];

    buffer[ 0] ^= buffer[7];
    buffer[ 2] ^= buffer[7];
    buffer[ 3] ^= buffer[7];

    chainv[ 7] ^= buffer[7];
    chainv[ 8] ^= buffer[7];
    chainv[ 1] ^= buffer[1];
    chainv[10] ^= buffer[1];
    chainv[11] ^= buffer[2];

    buffer[ 7] ^= buffer[6];
    buffer[ 1] ^= buffer[6];
    buffer[ 2] ^= buffer[6];

    chainv[ 6] ^= buffer[6];
    chainv[15] ^= buffer[6];
    chainv[16] ^= buffer[6];
    chainv[ 9] ^= buffer[0];
    chainv[18] ^= buffer[0];
    chainv[19] ^= buffer[1];

    buffer[ 6] ^= buffer[5];
    buffer[ 0] ^= buffer[5];
    buffer[ 1] ^= buffer[5];

    chainv[ 5] ^= buffer[5];
    chainv[14] ^= buffer[5];
    chainv[23] ^= buffer[5];
    chainv[24] ^= buffer[5];
    chainv[17] ^= buffer[7];
    chainv[26] ^= buffer[7];
    chainv[27] ^= buffer[0];

    buffer[ 5] ^= buffer[4];
    buffer[ 7] ^= buffer[4];
    buffer[ 0] ^= buffer[4];

    chainv[ 4] ^= buffer[4];
    chainv[13] ^= buffer[4];
    chainv[22] ^= buffer[4];
    chainv[31] ^= buffer[4];
    chainv[32] ^= buffer[4];
    chainv[33] ^= buffer[5];
    chainv[25] ^= buffer[6];
    chainv[34] ^= buffer[6];
    chainv[35] ^= buffer[7];
    chainv[36] ^= buffer[0];
    chainv[28] ^= buffer[1];
    chainv[37] ^= buffer[1];
    chainv[20] ^= buffer[2];
    chainv[29] ^= buffer[2];
    chainv[38] ^= buffer[2];
    chainv[12] ^= buffer[3];
    chainv[21] ^= buffer[3];
    chainv[30] ^= buffer[3];
    chainv[39] ^= buffer[3];

    for (j = 0; j < 5; j++) {
        TWEAK(&chainv[8*j],j);
        for (i = 0; i < 8; i++)
            STEP_PART(&chainv[8*j], &CNS[2*i + 16*j], t[0]);
    }

    for (i = 0; i < 40; i++)
        sp->chainv[i] = chainv[i];

    return;
}
#else
static inline
void rnd(luffaParam *sp, uint32_t *c)
{
    int i;
  
    mi(sp);

    tweak(sp);

    for (i = 0; i < 8; i++)
	step(sp, c);
  
    return;
}
#endif

/***************************************************/
/* Finalization function(s) */
/* sp: hash context      */
/* b[8]: hash values        */
#ifdef	OPTIMIZED
static inline
void finalization224(luffaParam *sp, uint32_t *b)
{
    int i,j;

    if (sp->bitlen[0] >= 256) {
        /*---- blank round with m=0 ----*/
        for (i = 0; i < 8; i++)
	    sp->buffer[i] =0;
        rnd256(sp);
    }

    for (i = 0; i < 7; i++) {
        b[i] = 0;
        for (j = 0; j < 3; j++)
            b[i] ^= sp->chainv[i+8*j];
        b[i] = BYTES_SWAP32((b[i]));
    }

    return;
}

static inline
void finalization256(luffaParam *sp, uint32_t *b)
{
    int i,j;

    if (sp->bitlen[0] >= 256) {
        /*---- blank round with m=0 ----*/
        for (i = 0; i < 8; i++)
	    sp->buffer[i] = 0;
        rnd256(sp);
    }

    for (i = 0; i < 8; i++) {
	b[i] = 0;
	for (j = 0; j < 3; j++)
	    b[i] ^= sp->chainv[i+8*j];
	b[i] = BYTES_SWAP32((b[i]));
    }

    return;
}

static inline
void finalization384(luffaParam *sp, uint32_t *b)
{
    int i,j;

    if (sp->bitlen[0] || sp->bitlen[1] >= 256) {
        /*---- blank round with m=0 ----*/
        for (i = 0; i < 8; i++)
	    sp->buffer[i] = 0;
        rnd384(sp);
    }

    for (i = 0; i < 8; i++) {
            b[i] = 0;
            for (j = 0; j < 4; j++)
                b[i] ^= sp->chainv[i+8*j];
            b[i] = BYTES_SWAP32((b[i]));
    }

    for (i = 0; i < 8; i++)
	sp->buffer[i] = 0;
    rnd384(sp);

    for (i = 0; i < 4; i++) {
            b[8+i] = 0;
            for (j = 0; j < 4; j++)
                b[8+i] ^= sp->chainv[i+8*j];
            b[8+i] = BYTES_SWAP32((b[8+i]));
    }

    return;
}

static inline
void finalization512(luffaParam *sp, uint32_t *b)
{
    int i,j;

    if (sp->bitlen[0] || sp->bitlen[1] >= 256) {
        /*---- blank round with m=0 ----*/
        for (i = 0; i < 8; i++)
	    sp->buffer[i] = 0;
        rnd512(sp);
    }

    for (i = 0; i < 8; i++) {
            b[i] = 0;
            for (j = 0; j < 5; j++)
                b[i] ^= sp->chainv[i+8*j];
            b[i] = BYTES_SWAP32((b[i]));
    }

    for (i = 0; i < 8; i++)
	sp->buffer[i] = 0;
    rnd512(sp);

    for (i = 0; i < 8; i++) {
            b[8+i] = 0;
            for (j = 0; j < 5; j++)
                b[8+i] ^= sp->chainv[i+8*j];
            b[8+i] = BYTES_SWAP32((b[8+i]));
    }

    return;
}
#else
static inline
void finalization(luffaParam *sp, uint32_t *b)
{
    int i, j, branch = 0;
    uint32_t c[10];

    switch(sp->hashbitlen) {
    case 224:
    case 256:
	if (sp->bitlen[0]>=256) branch = 1;
	break;

    case 384:
    case 512:
	if (sp->bitlen[0]||(sp->bitlen[1]>=256)) branch = 1;
	break;

    default:
	break;
	}

    if (branch) {
	/*---- blank round with m=0 ----*/
	memset(sp->buffer, 0, MSG_BLOCK_BYTE_LEN);
	memcpy(c, CNS, sp->width*8);
	rnd(sp, c);
    }

    switch(sp->hashbitlen) {
    case 224:
	for (i = 0; i < 7; i++) {
	    b[i] = 0;
	    for (j = 0; j < sp->width; j++)
		b[i] ^= sp->chainv[i+8*j];
	    b[i] = BYTES_SWAP32((b[i]));
	}
	break;

    case 256:
    case 384:
    case 512:
	for (i = 0; i < 8; i++) {
	    b[i] = 0;
	    for (j = 0; j < sp->width; j++)
		b[i] ^= sp->chainv[i+8*j];
	    b[i] = BYTES_SWAP32((b[i]));
	}
	if (sp->hashbitlen == 256) break;

	memset(sp->buffer, 0, MSG_BLOCK_BYTE_LEN);
	memcpy(c, CNS, sp->width*8);
	rnd(sp, c);

	for (i = 0; i < 4; i++) {
	    b[8+i] = 0;
	    for (j = 0; j < sp->width; j++)
		b[8+i] ^= sp->chainv[i+8*j];
	    b[8+i] = BYTES_SWAP32((b[8+i]));
	}
	if (sp->hashbitlen == 384) break;

	for (i = 4; i < 8; i++) {
	    b[8+i] = 0;
	    for (j = 0; j < sp->width; j++)
		b[8+i] ^= sp->chainv[i+8*j];
	    b[8+i] = BYTES_SWAP32((b[8+i]));
	}
	break;
    default:
	break;
    }

    return;
}
#endif

/***************************************************/
/* Process of the last blocks */
/* a[24]: chaining values */
/* msg[] : message in byte sequence */
/* msg_len: the length of the message in bit */
#ifdef	OPTIMIZED
static inline
void process_last_msgs256(luffaParam *sp)
{
    uint32_t tail_len = ((uint32_t)sp->bitlen[0]) % MSG_BLOCK_BIT_LEN;
    int i = tail_len/8;

    if (!(tail_len % 8))
        ((uint8_t*)sp->buffer)[i] = 0x80;
    else {
        ((uint8_t*)sp->buffer)[i] &= (0xff << (8-(tail_len % 8)));
        ((uint8_t*)sp->buffer)[i] |= (0x80 >> (tail_len % 8));
    }

    i++;

    for (; i < 32; i++)
        ((uint8_t*)sp->buffer)[i] = 0;
  
    for (i = 0; i < 8; i++)
        sp->buffer[i] = BYTES_SWAP32((sp->buffer[i]));

    rnd256(sp);

    return;
}

static inline
void process_last_msgs384(luffaParam *sp)
{
    uint32_t tail_len = ((uint32_t) sp->bitlen[1]) % MSG_BLOCK_BIT_LEN;
    int i = tail_len/8;

    if (!(tail_len % 8))
        ((uint8_t*)sp->buffer)[i] = 0x80;
    else {
        ((uint8_t*)sp->buffer)[i] &= (0xff << (8-(tail_len % 8)));
        ((uint8_t*)sp->buffer)[i] |= (0x80 >> (tail_len % 8));
    }

    i++;

    for (; i < 32; i++)
        ((uint8_t*)sp->buffer)[i] = 0;
  
    for (i = 0; i < 8; i++)
        sp->buffer[i] = BYTES_SWAP32((sp->buffer[i]));

    rnd384(sp);

    return;
}

static inline
void process_last_msgs512(luffaParam *sp)
{
    uint32_t tail_len = ((uint32_t)sp->bitlen[1]) % MSG_BLOCK_BIT_LEN;
    int i = tail_len/8;

    if (!(tail_len % 8))
        ((uint8_t*)sp->buffer)[i] = 0x80;
    else {
        ((uint8_t*)sp->buffer)[i] &= (0xff << (8-(tail_len % 8)));
        ((uint8_t*)sp->buffer)[i] |= (0x80 >> (tail_len % 8));
    }

    i++;

    for (; i < 32; i++)
        ((uint8_t*)sp->buffer)[i] = 0;
  
    for (i = 0; i < 8; i++)
        sp->buffer[i] = BYTES_SWAP32((sp->buffer[i]));

    rnd512(sp);

    return;
  
}
#else
static inline
void process_last_msgs(luffaParam *sp)
{
    int i;
    uint32_t tail_len;
    uint32_t c[10];
  
    if (sp->hashbitlen == 224 || sp->hashbitlen == 256)
	tail_len = ((uint32_t)sp->bitlen[0]) % MSG_BLOCK_BIT_LEN;
    else
	tail_len = ((uint32_t)sp->bitlen[1]) % MSG_BLOCK_BIT_LEN;

    i = tail_len/8;

    if (!(tail_len%8))
	((uint8_t *)sp->buffer)[i] = 0x80;
    else {
	((uint8_t *)sp->buffer)[i] &= (0xff << (8-(tail_len % 8)));
	((uint8_t *)sp->buffer)[i] |= (0x80 >> (tail_len % 8));
    }

    i++;

    for (; i < 32; i++)
	((uint8_t*)sp->buffer)[i] = 0;
    for (i = 0; i < 8;i++)
	sp->buffer[i] = BYTES_SWAP32(sp->buffer[i]);
    for (i = 0; i < sp->width*2; i++)
	c[i] = CNS[i];
    rnd(sp, c);

#if 0
    switch(sp->limit) {
    case 1:
	if (tail_len < MSG_BLOCK_BIT_LEN - 64) {
	    sp->buffer[6] = (uint32_t) (sp->bitlen[0] >> 32);
	    sp->buffer[7] = (uint32_t) sp->bitlen[0];
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
	} else {
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
    
	    for (i = 0; i < 6; i++)
		sp->buffer[i] = 0;
	    sp->buffer[6] = (uint32_t) (sp->bitlen[0] >> 32);
	    sp->buffer[7] = (uint32_t) sp->bitlen[0];
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
	}
	break;
    case 2:
	if (tail_len < MSG_BLOCK_BIT_LEN - 2*64) {
	    sp->buffer[4] = (uint32_t) (sp->bitlen[0] >> 32);
	    sp->buffer[5] = (uint32_t) sp->bitlen[0];
	    sp->buffer[6] = (uint32_t) (sp->bitlen[1] >> 32);
	    sp->buffer[7] = (uint32_t) sp->bitlen[1];
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
	} else {
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
    
	    for (i = 0; i < 4; i++)
		sp->buffer[i] = 0;
	    sp->buffer[4] = (uint32_t) (sp->bitlen[0] >> 32);
	    sp->buffer[5] = (uint32_t) sp->bitlen[0];
	    sp->buffer[6] = (uint32_t) (sp->bitlen[1] >> 32);
	    sp->buffer[7] = (uint32_t) sp->bitlen[1];
	    for (i = 0; i < sp->width*2; i++)
		c[i] = CNS[i];
	    rnd(sp, c);
	}
	break;
    default:
	break;
    }
#endif

    return;
}
#endif

/***************************************************/

int luffaInit(luffaParam *sp, int hashbitlen)
{
    sp->hashbitlen = hashbitlen;
#ifdef	OPTIMIZED
    int i;
#endif

    switch(hashbitlen) {
#ifdef	OPTIMIZED
    case 224:
        sp->bitlen[0] = 0;
        for (i = 0; i < 24; i++)
	    sp->chainv[i] = IV[i];
        break;

    case 256:
        sp->bitlen[0] = 0;
        for (i = 0; i < 24; i++)
	    sp->chainv[i] = IV[i];
        break;

    case 384:
        sp->bitlen[0] = 0;
        sp->bitlen[1] = 0;
        for (i = 0; i < 32; i++)
	    sp->chainv[i] = IV[i];
        break;

    case 512:
        sp->bitlen[0] = 0;
        sp->bitlen[1] = 0;
        for (i = 0; i < 40; i++)
	    sp->chainv[i] = IV[i];
        break;
#else
    case 224:
	sp->limit = LIMIT_224/64;
	memset(sp->bitlen, 0, LIMIT_224/8);
	sp->width = WIDTH_224;
	memcpy(sp->chainv, IV, WIDTH_224*32);
	break;
    case 256:
	sp->limit = LIMIT_256/64;
	memset(sp->bitlen, 0, LIMIT_256/8);
	sp->width = WIDTH_256;
	memcpy(sp->chainv, IV, WIDTH_256*32);
	break;
    case 384:
	sp->limit = LIMIT_384/64;
	memset(sp->bitlen, 0, LIMIT_384/8);
	sp->width = WIDTH_384;
	memcpy(sp->chainv, IV, WIDTH_384*32);
	break;
    case 512:
	sp->limit = LIMIT_512/64;
	memset(sp->bitlen, 0, LIMIT_512/8);
	sp->width = WIDTH_512;
	memcpy(sp->chainv, IV, WIDTH_512*32);
	break;
#endif
    default:
	return BAD_HASHBITLEN;
    }

    sp->rembitlen = 0;

#ifdef	OPTIMIZED
    for (i = 0; i < 8; i++)
	sp->buffer[i] = 0;
#else
    memset(sp->buffer, 0, MSG_BLOCK_BYTE_LEN);
#endif

    return SUCCESS;
}

int
luffaReset(luffaParam *sp)
{
    return luffaInit(sp, sp->hashbitlen);
}

#ifdef	OPTIMIZED
static
void Update256(luffaParam *sp, const byte *data, uint64_t databitlen)
{
    uint8_t *p = (uint8_t*)sp->buffer;
    int i;

    sp->bitlen[0] += databitlen;

    if (sp->rembitlen + databitlen >= MSG_BLOCK_BIT_LEN) {
	uint32_t cpylen = MSG_BLOCK_BYTE_LEN - (sp->rembitlen >> 3);

        if (!sp->rembitlen)
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];
        else
	    for (i = 0; i < (int)cpylen; i++)
		((uint8_t*)sp->buffer)[(sp->rembitlen>>3)+i] = data[i];

        BYTES_SWAP256(sp->buffer);

        rnd256(sp);

        databitlen -= (cpylen << 3);
        data += cpylen;
        sp->rembitlen = 0;

        while (databitlen >= MSG_BLOCK_BIT_LEN) {
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];

	    BYTES_SWAP256(sp->buffer);

	    rnd256(sp);

	    databitlen -= MSG_BLOCK_BIT_LEN;
	    data += MSG_BLOCK_BYTE_LEN;
        }
    }

    /* All remaining data copy to buffer */
    if (databitlen) {
	uint32_t len = databitlen >> 3;

	if (databitlen % 8 != 0)
	    len += 1;

	for (i = 0; i < (int)len; i++)
	    p[sp->rembitlen/8+i] = data[i];
	sp->rembitlen += databitlen;
    }

    return;
}

static
void Update384(luffaParam *sp, const byte *data, uint64_t databitlen)
{
    uint8_t *p = (uint8_t*)sp->buffer;
    int i;

    if ((sp->bitlen[1] += databitlen) < databitlen)
        sp->bitlen[0] += 1;

    if (sp->rembitlen + databitlen >= MSG_BLOCK_BIT_LEN) {
	uint32_t cpylen = MSG_BLOCK_BYTE_LEN - (sp->rembitlen >> 3);

        if (!sp->rembitlen)
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];
        else
	    for (i = 0; i < (int)cpylen; i++)
		((uint8_t*)sp->buffer)[(sp->rembitlen>>3)+i] = data[i];

        BYTES_SWAP256(sp->buffer);

        rnd384(sp);

	databitlen -= (cpylen << 3);
	data += cpylen;
	sp->rembitlen = 0;

	while (databitlen >= MSG_BLOCK_BIT_LEN) {
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];

	    BYTES_SWAP256(sp->buffer);

	    rnd384(sp);

	    databitlen -= MSG_BLOCK_BIT_LEN;
	    data += MSG_BLOCK_BYTE_LEN;
	}
    }

    /* All remaining data copy to buffer */
    if (databitlen) {
	uint32_t len = databitlen >> 3;

	if (databitlen % 8 != 0)
	    len += 1;

	for (i = 0; i < (int)len; i++)
	    p[sp->rembitlen/8+i] = data[i];
	sp->rembitlen += databitlen;
    }

    return;
}

static
void Update512(luffaParam *sp, const byte *data, uint64_t databitlen)
{
    uint8_t *p = (uint8_t*)sp->buffer;
    int i;

    if ((sp->bitlen[1] += databitlen) < databitlen)
        sp->bitlen[0] += 1;

    if (sp->rembitlen + databitlen >= MSG_BLOCK_BIT_LEN) {
	uint32_t cpylen = MSG_BLOCK_BYTE_LEN - (sp->rembitlen >> 3);

        if (!sp->rembitlen)
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];
        else
	    for (i = 0; i < (int)cpylen; i++)
		((uint8_t*)sp->buffer)[(sp->rembitlen>>3)+i] = data[i];

        BYTES_SWAP256(sp->buffer);

        rnd512(sp);

	databitlen -= (cpylen << 3);
	data += cpylen;
	sp->rembitlen = 0;

	while (databitlen >= MSG_BLOCK_BIT_LEN) {
	    for (i = 0; i < 8; i++)
		sp->buffer[i] = ((uint32_t*)data)[i];

	    BYTES_SWAP256(sp->buffer);

	    rnd512(sp);

	    databitlen -= MSG_BLOCK_BIT_LEN;
	    data += MSG_BLOCK_BYTE_LEN;
	}
    }

    /* All remaining data copy to buffer */
    if (databitlen) {
	uint32_t len = databitlen >> 3;

	if (databitlen % 8 != 0)
	    len += 1;

	for (i = 0; i < (int)len; i++)
	    p[sp->rembitlen/8+i] = data[i];
	sp->rembitlen += databitlen;
    }

    return;
}
#endif

int luffaUpdate(luffaParam *sp, const byte *data, size_t size)
{
    uint64_t databitlen = 8 * size;
    int ret = SUCCESS;

    switch(sp->hashbitlen) {
#ifdef	OPTIMIZED
    case 224:
    case 256:	Update256(sp, data, databitlen);	break;
    case 384:	Update384(sp, data, databitlen);	break;
    case 512:	Update512(sp, data, databitlen);	break;
#else
    case 224:
    case 256:
    case 384:
    case 512:
    {	uint8_t *p = (uint8_t*)sp->buffer;

	if (sp->hashbitlen == 224 || sp->hashbitlen == 256)
	    sp->bitlen[0] += databitlen;
	else {
	    if ((sp->bitlen[1] += databitlen) < databitlen)
		sp->bitlen[0] += 1;
	}

	if (sp->rembitlen + databitlen >= MSG_BLOCK_BIT_LEN) {
	    uint32_t cpylen = MSG_BLOCK_BYTE_LEN - (sp->rembitlen >> 3);
	    uint32_t c[10];
	    int i;

	    memcpy(p + (sp->rembitlen >> 3), data, cpylen);

	    BYTES_SWAP256(sp->buffer);

	    for (i = 0; i < (sp->width*2); i++)
		c[i] = CNS[i];
	    rnd(sp, c);

	    databitlen -= (cpylen << 3);
	    data += cpylen;
	    sp->rembitlen = 0;

	    while (databitlen >= MSG_BLOCK_BIT_LEN) {
		memcpy(p, data, MSG_BLOCK_BYTE_LEN);

		BYTES_SWAP256(sp->buffer);

		for (i = 0; i < (sp->width*2); i++)
		    c[i] = CNS[i];
		rnd(sp, c);

		databitlen -= MSG_BLOCK_BIT_LEN;
		data += MSG_BLOCK_BYTE_LEN;
	    }
	}

	/* All remaining data copy to buffer */
	if (databitlen) {
	    uint32_t len = databitlen >> 3;
	    if (databitlen % 8 != 0)
		len += 1;

	    memcpy(p + (sp->rembitlen >> 3), data, len);
	    sp->rembitlen += databitlen;
	}
    }	break;
#endif
    default:
	ret = BAD_HASHBITLEN;
	break;
    }
    return ret;
}

int luffaDigest(luffaParam *sp, byte *digest) 
{
    int ret = SUCCESS;

    switch(sp->hashbitlen) {
#ifdef	OPTIMIZED
    case 224:
        process_last_msgs256(sp);
        finalization224(sp, (uint32_t *)digest);
        break;
    case 256:
        process_last_msgs256(sp);
        finalization256(sp, (uint32_t *)digest);
        break;
    case 384:
        process_last_msgs384(sp);
        finalization384(sp, (uint32_t *)digest);
        break;
    case 512:
        process_last_msgs512(sp);
        finalization512(sp, (uint32_t *)digest);
        break;
#else
    case 224:
    case 256:
    case 384:
    case 512:
	process_last_msgs(sp);
	finalization(sp, (uint32_t *)digest);
	break;
#endif
    default:
	ret = BAD_HASHBITLEN;
	break;
    }

    return ret;
}
