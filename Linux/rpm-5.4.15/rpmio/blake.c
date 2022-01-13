#include "blake.h"

const hashFunction blake256 = {
    .name = "BLAKE-256",
    .paramsize = sizeof(blakeParam),
    .blocksize = 64,
    .digestsize = 256/8,        /* XXX default to BLAKE-256 */
    .reset = (hashFunctionReset) blakeReset,
    .update = (hashFunctionUpdate) blakeUpdate,
    .digest = (hashFunctionDigest) blakeDigest
};

enum { SUCCESS=0, FAIL=1, BAD_HASHBITLEN=2  };

#define NB_ROUNDS32 14
#define NB_ROUNDS64 16

/* byte-to-word conversion and vice-versa (little endian)  */
#define U8TO32_BE(p) \
  (((uint32_t)((p)[0]) << 24) | \
   ((uint32_t)((p)[1]) << 16) | \
   ((uint32_t)((p)[2]) <<  8) | \
   ((uint32_t)((p)[3])      ))

#define U8TO64_BE(p) \
  (((uint64_t)U8TO32_BE(p) << 32) | (uint64_t)U8TO32_BE((p) + 4))

#define U32TO8_BE(p, v) \
  do { \
    (p)[0] = (byte)((v) >> 24);  \
    (p)[1] = (byte)((v) >> 16); \
    (p)[2] = (byte)((v) >>  8); \
    (p)[3] = (byte)((v)      ); \
  } while (0)

#define U64TO8_BE(p, v) \
  do { \
    U32TO8_BE((p),     (uint32_t)((v) >> 32));  \
    U32TO8_BE((p) + 4, (uint32_t)((v)      ));  \
  } while (0)

/*
  adds a salt to the hash function (OPTIONAL)
  should be called AFTER Init, and BEFORE Update

  INPUT
  sp: hashSate structure
  salt: the salt, whose length is determined by hashbitlen

  OUTPUT
  SUCCESS on success
 */
int blakeAddSalt(blakeParam * sp, const byte * salt);

/* the 10 permutations of {0,...15} */
static const unsigned char sigma[][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 }
  };

/* constants for BLAKE-32 and BLAKE-28 */
static const uint32_t c32[16] = {
    0x243F6A88, 0x85A308D3,
    0x13198A2E, 0x03707344,
    0xA4093822, 0x299F31D0,
    0x082EFA98, 0xEC4E6C89,
    0x452821E6, 0x38D01377,
    0xBE5466CF, 0x34E90C6C,
    0xC0AC29B7, 0xC97C50DD,
    0x3F84D5B5, 0xB5470917
};

/* constants for BLAKE-64 and BLAKE-48 */
static const uint64_t c64[16] = {
  0x243F6A8885A308D3ULL,0x13198A2E03707344ULL,
  0xA4093822299F31D0ULL,0x082EFA98EC4E6C89ULL,
  0x452821E638D01377ULL,0xBE5466CF34E90C6CULL,
  0xC0AC29B7C97C50DDULL,0x3F84D5B5B5470917ULL,
  0x9216D5D98979FB1BULL,0xD1310BA698DFB5ACULL,
  0x2FFD72DBD01ADFB7ULL,0xB8E1AFED6A267E96ULL,
  0xBA7C9045F12C7F99ULL,0x24A19947B3916CF7ULL,
  0x0801F2E2858EFC16ULL,0x636920D871574E69ULL
};

/* padding data */
static const byte padding[129] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* initial values ( IVx for BLAKE-x) */
static const uint32_t IV256[8] = {
  0x6A09E667, 0xBB67AE85,
  0x3C6EF372, 0xA54FF53A,
  0x510E527F, 0x9B05688C,
  0x1F83D9AB, 0x5BE0CD19
};
static const uint32_t IV224[8] = {
  0xC1059ED8, 0x367CD507,
  0x3070DD17, 0xF70E5939,
  0xFFC00B31, 0x68581511,
  0x64F98FA7, 0xBEFA4FA4
};
static const uint64_t IV384[8] = {
  0xCBBB9D5DC1059ED8ULL, 0x629A292A367CD507ULL,
  0x9159015A3070DD17ULL, 0x152FECD8F70E5939ULL,
  0x67332667FFC00B31ULL, 0x8EB44A8768581511ULL,
  0xDB0C2E0D64F98FA7ULL, 0x47B5481DBEFA4FA4ULL
};
static const uint64_t IV512[8] = {
  0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL,
  0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
  0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL,
  0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL
};

static int compress32(blakeParam * sp, const byte * datablock )
{
    uint32_t v[16];
    uint32_t m[16];
    int round;

#define ROT32(x,n) (((x)<<(32-n))|( (x)>>(n)))
#define ADD32(x,y)   ((uint32_t)((x) + (y)))
#define XOR32(x,y)    ((uint32_t)((x) ^ (y)))
  
#define G32(a,b,c,d,i)\
  do { \
    v[a] = ADD32(v[a],v[b])+XOR32(m[sigma[round][2*i]], c32[sigma[round][2*i+1]]);\
    v[d] = ROT32(XOR32(v[d],v[a]),16);\
    v[c] = ADD32(v[c],v[d]);\
    v[b] = ROT32(XOR32(v[b],v[c]),12);\
    v[a] = ADD32(v[a],v[b])+XOR32(m[sigma[round][2*i+1]], c32[sigma[round][2*i]]);\
    v[d] = ROT32(XOR32(v[d],v[a]), 8);\
    v[c] = ADD32(v[c],v[d]);\
    v[b] = ROT32(XOR32(v[b],v[c]), 7);\
  } while (0)

    /* get message */
    m[ 0] = U8TO32_BE(datablock + 0);
    m[ 1] = U8TO32_BE(datablock + 4);
    m[ 2] = U8TO32_BE(datablock + 8);
    m[ 3] = U8TO32_BE(datablock +12);
    m[ 4] = U8TO32_BE(datablock +16);
    m[ 5] = U8TO32_BE(datablock +20);
    m[ 6] = U8TO32_BE(datablock +24);
    m[ 7] = U8TO32_BE(datablock +28);
    m[ 8] = U8TO32_BE(datablock +32);
    m[ 9] = U8TO32_BE(datablock +36);
    m[10] = U8TO32_BE(datablock +40);
    m[11] = U8TO32_BE(datablock +44);
    m[12] = U8TO32_BE(datablock +48);
    m[13] = U8TO32_BE(datablock +52);
    m[14] = U8TO32_BE(datablock +56);
    m[15] = U8TO32_BE(datablock +60);

    /* initialization */
    v[ 0] = sp->h32[0];
    v[ 1] = sp->h32[1];
    v[ 2] = sp->h32[2];
    v[ 3] = sp->h32[3];
    v[ 4] = sp->h32[4];
    v[ 5] = sp->h32[5];
    v[ 6] = sp->h32[6];
    v[ 7] = sp->h32[7];
    v[ 8] = sp->salt32[0] ^ c32[0];
    v[ 9] = sp->salt32[1] ^ c32[1];
    v[10] = sp->salt32[2] ^ c32[2];
    v[11] = sp->salt32[3] ^ c32[3];
    if (sp->nullt) { /* special case t=0 for the last block */
	v[12] =  c32[4];
	v[13] =  c32[5];
	v[14] =  c32[6];
	v[15] =  c32[7];
    } else {
	v[12] = sp->t32[0] ^ c32[4];
	v[13] = sp->t32[0] ^ c32[5];
	v[14] = sp->t32[1] ^ c32[6];
	v[15] = sp->t32[1] ^ c32[7];
    }

    /*  do 14 rounds */
    for (round = 0; round < NB_ROUNDS32; ++round) {

	/* column step */
	G32( 0, 4, 8,12, 0);
	G32( 1, 5, 9,13, 1);
	G32( 2, 6,10,14, 2);
	G32( 3, 7,11,15, 3);    

	/* diagonal step */
	G32( 0, 5,10,15, 4);
	G32( 1, 6,11,12, 5);
	G32( 2, 7, 8,13, 6);
	G32( 3, 4, 9,14, 7);

    }

    /* finalization */
    sp->h32[0] ^= v[ 0]^v[ 8]^sp->salt32[0];
    sp->h32[1] ^= v[ 1]^v[ 9]^sp->salt32[1];
    sp->h32[2] ^= v[ 2]^v[10]^sp->salt32[2];
    sp->h32[3] ^= v[ 3]^v[11]^sp->salt32[3];
    sp->h32[4] ^= v[ 4]^v[12]^sp->salt32[0];
    sp->h32[5] ^= v[ 5]^v[13]^sp->salt32[1];
    sp->h32[6] ^= v[ 6]^v[14]^sp->salt32[2];
    sp->h32[7] ^= v[ 7]^v[15]^sp->salt32[3];

    return SUCCESS;
}


static int compress64(blakeParam * sp, const byte * datablock)
{
    uint64_t v[16];
    uint64_t m[16];
    int round;

#define ROT64(x,n) (((x)<<(64-n))|( (x)>>(n)))
#define ADD64(x,y)   ((uint64_t)((x) + (y)))
#define XOR64(x,y)    ((uint64_t)((x) ^ (y)))
  
#define G64(a,b,c,d,i)\
  do { \
    v[a] = ADD64(v[a],v[b])+XOR64(m[sigma[round][2*i]], c64[sigma[round][2*i+1]]);\
    v[d] = ROT64(XOR64(v[d],v[a]),32);\
    v[c] = ADD64(v[c],v[d]);\
    v[b] = ROT64(XOR64(v[b],v[c]),25);\
    v[a] = ADD64(v[a],v[b])+XOR64(m[sigma[round][2*i+1]], c64[sigma[round][2*i]]);\
    v[d] = ROT64(XOR64(v[d],v[a]),16);\
    v[c] = ADD64(v[c],v[d]);\
    v[b] = ROT64(XOR64(v[b],v[c]),11);\
  } while (0)

    /* get message */
    m[ 0] = U8TO64_BE(datablock +  0);
    m[ 1] = U8TO64_BE(datablock +  8);
    m[ 2] = U8TO64_BE(datablock + 16);
    m[ 3] = U8TO64_BE(datablock + 24);
    m[ 4] = U8TO64_BE(datablock + 32);
    m[ 5] = U8TO64_BE(datablock + 40);
    m[ 6] = U8TO64_BE(datablock + 48);
    m[ 7] = U8TO64_BE(datablock + 56);
    m[ 8] = U8TO64_BE(datablock + 64);
    m[ 9] = U8TO64_BE(datablock + 72);
    m[10] = U8TO64_BE(datablock + 80);
    m[11] = U8TO64_BE(datablock + 88);
    m[12] = U8TO64_BE(datablock + 96);
    m[13] = U8TO64_BE(datablock +104);
    m[14] = U8TO64_BE(datablock +112);
    m[15] = U8TO64_BE(datablock +120);

    /* initialization */
    v[ 0] = sp->h64[0];
    v[ 1] = sp->h64[1];
    v[ 2] = sp->h64[2];
    v[ 3] = sp->h64[3];
    v[ 4] = sp->h64[4];
    v[ 5] = sp->h64[5];
    v[ 6] = sp->h64[6];
    v[ 7] = sp->h64[7];
    v[ 8] = sp->salt64[0] ^ c64[0];
    v[ 9] = sp->salt64[1] ^ c64[1];
    v[10] = sp->salt64[2] ^ c64[2];
    v[11] = sp->salt64[3] ^ c64[3];
    if (sp->nullt) { 
	v[12] =  c64[4];
	v[13] =  c64[5];
	v[14] =  c64[6];
	v[15] =  c64[7];
    } else {
	v[12] = sp->t64[0] ^ c64[4];
	v[13] = sp->t64[0] ^ c64[5];
	v[14] = sp->t64[1] ^ c64[6];
	v[15] = sp->t64[1] ^ c64[7];
    }  

    /*  do 16 rounds */
    for (round = 0; round < NB_ROUNDS64; ++round) {
	/* column step */
	G64( 0, 4, 8,12, 0);
	G64( 1, 5, 9,13, 1);
	G64( 2, 6,10,14, 2);
	G64( 3, 7,11,15, 3);    
	/* diagonal step */
	G64( 0, 5,10,15, 4);
	G64( 1, 6,11,12, 5);
	G64( 2, 7, 8,13, 6);
	G64( 3, 4, 9,14, 7);
    }

    /* finalization */
    sp->h64[0] ^= v[ 0]^v[ 8]^sp->salt64[0];
    sp->h64[1] ^= v[ 1]^v[ 9]^sp->salt64[1];
    sp->h64[2] ^= v[ 2]^v[10]^sp->salt64[2];
    sp->h64[3] ^= v[ 3]^v[11]^sp->salt64[3];
    sp->h64[4] ^= v[ 4]^v[12]^sp->salt64[0];
    sp->h64[5] ^= v[ 5]^v[13]^sp->salt64[1];
    sp->h64[6] ^= v[ 6]^v[14]^sp->salt64[2];
    sp->h64[7] ^= v[ 7]^v[15]^sp->salt64[3];

    return SUCCESS;
}

int blakeInit(blakeParam * sp, int hashbitlen)
{
    int i;

    if (hashbitlen == 224 || hashbitlen == 256)  {
	/* 224- and 256-bit versions (32-bit words) */

	if (hashbitlen == 224) 
	    memcpy(sp->h32, IV224, sizeof(IV224));      
	else 
	    memcpy(sp->h32, IV256, sizeof(IV256));

	sp->t32[0] = 0;
	sp->t32[1] = 0;

	for(i=0; i<64; ++i)
	    sp->data32[i] = 0;

	sp->salt32[0] = 0;
	sp->salt32[1] = 0;
	sp->salt32[2] = 0;
	sp->salt32[3] = 0;
    } else if (hashbitlen == 384 || hashbitlen == 512){
	/* 384- and 512-bit versions (64-bit words) */

	if (hashbitlen == 384) 
	    memcpy(sp->h64, IV384, sizeof(IV384));      
	else 
	    memcpy(sp->h64, IV512, sizeof(IV512));

	sp->t64[0] = 0;
	sp->t64[1] = 0;

	for (i = 0; i < 64; ++i)
	    sp->data64[i] = 0;
    
	sp->salt64[0] = 0;
	sp->salt64[1] = 0;
	sp->salt64[2] = 0;
	sp->salt64[3] = 0;    
    } else
	return BAD_HASHBITLEN;

    sp->hashbitlen = hashbitlen;
    sp->datalen = 0;
    sp->init = 1;
    sp->nullt = 0;

    return SUCCESS;
}

int
blakeReset(blakeParam * sp)
{
    return blakeInit(sp, sp->hashbitlen);
}

int blakeAddSalt(blakeParam * sp, const byte * salt)
{
    /* if hashbitlen=224 or 256, then the salt should be 128-bit (16 bytes) */
    /* if hashbitlen=384 or 512, then the salt should be 256-bit (32 bytes) */

    /* fail if Init() was not called before */
    if (sp->init != 1) 
	return FAIL;

    if (sp->hashbitlen < 384) {
	sp->salt32[0] = U8TO32_BE(salt + 0);
	sp->salt32[1] = U8TO32_BE(salt + 4);
	sp->salt32[2] = U8TO32_BE(salt + 8);
	sp->salt32[3] = U8TO32_BE(salt +12);
    } else {
	sp->salt64[0] = U8TO64_BE(salt + 0);
	sp->salt64[1] = U8TO64_BE(salt + 8);
	sp->salt64[2] = U8TO64_BE(salt +16);
	sp->salt64[3] = U8TO64_BE(salt +24);
    }

    return SUCCESS;
}

static int Update32(blakeParam * sp, const byte * data, uint64_t databitlen)
{
    int fill;
    unsigned int left; /* to handle data inputs of up to 2^64-1 bits */
  
    if (databitlen == 0 && sp->datalen != 512)
	return SUCCESS;

    left = (sp->datalen >> 3); 
    fill = 64 - left;

    /* compress remaining data filled with new bits */
    if (left && (int)((databitlen >> 3) & 0x3F) >= fill) {
	memcpy((void *) (sp->data32 + left), (void *) data, fill);
	/* update counter */
	sp->t32[0] += 512;
	if (sp->t32[0] == 0)
	    sp->t32[1]++;
      
	compress32(sp, sp->data32);
	data += fill;
	databitlen  -= (fill << 3); 
      
	left = 0;
    }

    /* compress data until enough for a block */
    while (databitlen >= 512) {
	/* update counter */
	sp->t32[0] += 512;

	if (sp->t32[0] == 0)
	    sp->t32[1]++;
	compress32(sp, data);
	data += 64;
	databitlen  -= 512;
    }
  
    if (databitlen > 0) {
	memcpy((void *) (sp->data32 + left), (void *) data, databitlen>>3);
	sp->datalen = (left<<3) + databitlen;
	/* when non-8-multiple, add remaining bits (1 to 7)*/
	if (databitlen & 0x7)
	    sp->data32[left + (databitlen>>3)] = data[databitlen>>3];
    } else
	sp->datalen = 0;

    return SUCCESS;
}

static int Update64(blakeParam * sp, const byte * data, uint64_t databitlen )
{
    int fill;
    unsigned int left;

    if (databitlen == 0 && sp->datalen != 1024)
	return SUCCESS;

    left = (sp->datalen >> 3);
    fill = 128 - left;

    /* compress remaining data filled with new bits */
    if (left && (int)((databitlen >> 3) & 0x7F) >= fill) {
	memcpy((void *) (sp->data64 + left), (void *) data, fill);
	/* update counter  */
	sp->t64[0] += 1024;

	compress64(sp, sp->data64);
	data += fill;
	databitlen  -= (fill << 3); 
      
	left = 0;
    }

    /* compress data until enough for a block */
    while (databitlen >= 1024) {
	/* update counter */
	sp->t64[0] += 1024;
	compress64( sp, data );
	data += 128;
	databitlen  -= 1024;
    }

    if (databitlen > 0) {
	memcpy((void *) (sp->data64 + left), (void *) data, (databitlen>>3) & 0x7F);
	sp->datalen = (left<<3) + databitlen;

	/* when non-8-multiple, add remaining bits (1 to 7)*/
	if (databitlen & 0x7)
	    sp->data64[left + (databitlen>>3)] = data[databitlen>>3];
    } else
	sp->datalen = 0;

    return SUCCESS;
}

int blakeUpdate(blakeParam * sp, const byte * data, size_t size)
{
    uint64_t databitlen = 8 * size;

    if (sp->hashbitlen < 384)
	return Update32(sp, data, databitlen);
    else
	return Update64(sp, data, databitlen);
}

static int Final32(blakeParam * sp, byte * digest)
{
    unsigned char msglen[8];
    byte zz=0x00, zo=0x01, oz=0x80, oo=0x81;
    /* copy nb. bits hash in total as a 64-bit BE word */
    uint32_t low  = sp->t32[0] + sp->datalen;
    uint32_t high = sp->t32[1];

    if (low < sp->datalen)
	high++;
    U32TO8_BE(msglen + 0, high);
    U32TO8_BE(msglen + 4, low );

    if (sp->datalen % 8 == 0) {
	/* message bitlength multiple of 8 */

	if (sp->datalen == 440) {
	    /* special case of one padding byte */
	    sp->t32[0] -= 8;
	    if (sp->hashbitlen == 224) 
		Update32(sp, &oz, 8);
	    else
		Update32(sp, &oo, 8);
	} else {
	    if (sp->datalen < 440) {
		/* use t=0 if no remaining data */
		if (sp->datalen == 0) 
		    sp->nullt = 1;
		/* enough space to fill the block  */
		sp->t32[0] -= 440 - sp->datalen;
		Update32(sp, padding, 440 - sp->datalen);
	    } else {
		/* NOT enough space, need 2 compressions */
		sp->t32[0] -= 512 - sp->datalen;
		Update32(sp, padding, 512 - sp->datalen);
		sp->t32[0] -= 440;
		Update32(sp, padding+1, 440);  /* pad with zeroes */
		sp->nullt = 1; /* raise flag to set t=0 at the next compress */
	    }
	    if (sp->hashbitlen == 224) 
		Update32(sp, &zz, 8);
	    else
		Update32(sp, &zo, 8);
	    sp->t32[0] -= 8;
	}
	sp->t32[0] -= 64;
	Update32(sp, msglen, 64);    
    } else {  
	/* message bitlength NOT multiple of 8 */

	/*  add '1' */
	sp->data32[sp->datalen/8] &= (0xFF << (8-sp->datalen%8)); 
	sp->data32[sp->datalen/8] ^= (0x80 >> (sp->datalen%8)); 

	if (sp->datalen > 440 && sp->datalen < 447) {
	    /*  special case of one padding byte */
	    if (sp->hashbitlen == 224) 
		sp->data32[sp->datalen/8] ^= 0x00;
	    else
		sp->data32[sp->datalen/8] ^= 0x01;
	    sp->t32[0] -= (8 - (sp->datalen%8));
	    /* set datalen to a 8 multiple */
	    sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
	} else { 
	    if (sp->datalen < 440) {
		/* enough space to fill the block */
		sp->t32[0] -= 440 - sp->datalen;
		sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
		blakeUpdate(sp, padding+1, 440 - sp->datalen);
	    } else { 
		if (sp->datalen > 504) {
		    /* special case */
		    sp->t32[0] -= 512 - sp->datalen;
		    sp->datalen=512;
		    Update32(sp, padding+1, 0);
		    sp->t32[0] -= 440;
		    Update32(sp, padding+1, 440);
		    sp->nullt = 1; /* raise flag for t=0 at the next compress */
		} else {
		    /* NOT enough space, need 2 compressions */
		    sp->t32[0] -= 512 - sp->datalen;
		    /* set datalen to a 8 multiple */
		    sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
		    Update32(sp, padding+1, 512 - sp->datalen);
		    sp->t32[0] -= 440;
		    Update32(sp, padding+1, 440);
		    sp->nullt = 1; /* raise flag for t=0 at the next compress */
		}
	    }
	    sp->t32[0] -= 8;
	    if (sp->hashbitlen == 224) 
		Update32(sp, &zz, 8);
	    else
		Update32(sp, &zo, 8);
	}
	sp->t32[0] -= 64;
	Update32(sp, msglen, 64); 
    }

    U32TO8_BE( digest + 0, sp->h32[0]);
    U32TO8_BE( digest + 4, sp->h32[1]);
    U32TO8_BE( digest + 8, sp->h32[2]);
    U32TO8_BE( digest +12, sp->h32[3]);
    U32TO8_BE( digest +16, sp->h32[4]);
    U32TO8_BE( digest +20, sp->h32[5]);
    U32TO8_BE( digest +24, sp->h32[6]);

    if (sp->hashbitlen == 256) {
	U32TO8_BE( digest +28, sp->h32[7]);
    }
  
    return SUCCESS;
}

static int Final64(blakeParam * sp, byte * digest)
{
    unsigned char msglen[16];
    byte zz=0x00,zo=0x01,oz=0x80,oo=0x81;

    /* copy nb. bits hash in total as a 128-bit BE word */
    uint64_t low  = sp->t64[0] + sp->datalen;
    uint64_t high = sp->t64[1];
    if (low < sp->datalen)
	high++;
    U64TO8_BE(  msglen + 0, high );
    U64TO8_BE(  msglen + 8, low  );

    if (sp->datalen % 8 == 0) {
	/* message bitlength multiple of 8 */

	if (sp->datalen == 888) {
	    /* special case of one padding byte */
	    sp->t64[0] -= 8; 
	    if (sp->hashbitlen == 384) 
		Update64( sp, &oz, 8 );
	    else
		Update64( sp, &oo, 8 );
	} else {
		if (sp->datalen < 888) {
		    /* use t=0 if no remaining data */
		    if (sp->datalen == 0) 
			sp->nullt=1;
		    /* enough space to fill the block */
		    sp->t64[0] -= 888 - sp->datalen;
		    Update64(sp, padding, 888 - sp->datalen);
		} else { 
		    /* NOT enough space, need 2 compressions */
		    sp->t64[0] -= 1024 - sp->datalen; 
		    Update64(sp, padding, 1024 - sp->datalen);
		    sp->t64[0] -= 888;
		    Update64(sp, padding+1, 888);  /* padd with zeros */
		    sp->nullt = 1; /* raise flag to set t=0 at next compress */
		}
		if (sp->hashbitlen == 384) 
		    Update64(sp, &zz, 8);
		else
		    blakeUpdate(sp, &zo, 8);
		sp->t64[0] -= 8;
	}
	sp->t64[0] -= 128;
	blakeUpdate(sp, msglen, 128);    
    } else {  
	/* message bitlength NOT multiple of 8 */

	/* add '1' */
	sp->data64[sp->datalen/8] &= (0xFF << (8-sp->datalen%8)); 
	sp->data64[sp->datalen/8] ^= (0x80 >> (sp->datalen%8)); 

	if (sp->datalen > 888 && sp->datalen < 895) {
	    /*  special case of one padding byte */
	    if (sp->hashbitlen == 384) 
		sp->data64[sp->datalen/8] ^= zz;
	    else
		sp->data64[sp->datalen/8] ^= zo;
	    sp->t64[0] -= (8 - (sp->datalen%8));
	    /* set datalen to a 8 multiple */
	    sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
	} else { 
	    if (sp->datalen < 888) {
		/* enough space to fill the block */
		sp->t64[0] -= 888 - sp->datalen;
		sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
		Update64(sp, padding+1, 888 - sp->datalen);
	    } else {
		if (sp->datalen > 1016) {
		    /* special case */
		    sp->t64[0] -= 1024 - sp->datalen;
		    sp->datalen=1024;
		    Update64(sp, padding+1, 0);
		    sp->t64[0] -= 888;
		    Update64(sp, padding+1, 888);
		    sp->nullt = 1; /* raise flag for t=0 at the next compress */
		} else {
		    /* NOT enough space, need 2 compressions */
		    sp->t64[0] -= 1024 - sp->datalen;
		    /* set datalen to a 8 multiple */
		    sp->datalen = (sp->datalen&(uint64_t)0xfffffffffffffff8ULL)+8;
		    Update64(sp, padding+1, 1024 - sp->datalen);
		    sp->t64[0] -= 888;
		    Update64(sp, padding+1, 888);
		    sp->nullt = 1; /* raise flag for t=0 at the next compress */
		}
	    }
	    sp->t64[0] -= 8;
	    if (sp->hashbitlen == 384) 
		Update64(sp, &zz, 8);
	    else
		Update64(sp, &zo, 8);
	}
	sp->t64[0] -= 128;
	blakeUpdate(sp, msglen, 128); 
    }

    U64TO8_BE(digest + 0, sp->h64[0]);
    U64TO8_BE(digest + 8, sp->h64[1]);
    U64TO8_BE(digest +16, sp->h64[2]);
    U64TO8_BE(digest +24, sp->h64[3]);
    U64TO8_BE(digest +32, sp->h64[4]);
    U64TO8_BE(digest +40, sp->h64[5]);

    if (sp->hashbitlen == 512) {
	U64TO8_BE(digest +48, sp->h64[6]);
	U64TO8_BE(digest +56, sp->h64[7]);
    }
  
    return SUCCESS;
}

int blakeDigest(blakeParam * sp, byte * digest)
{
    if (sp->hashbitlen < 384)
	return Final32(sp, digest);
    else
	return Final64(sp, digest);
}
