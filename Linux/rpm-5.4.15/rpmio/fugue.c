/*********************************************************
 *
 * Filename	: fugue.c
 *
 * Originator	: W. Eric Hall
 *
 * Purpose      : Reference Implementation of Fugue for
 *                224, 256, 384 and 512 bit hash sizes
 *
 *********************************************************/

#include "fugue.h"

const hashFunction fugue256 = {
    .name = "FUGUE-256",
    .paramsize = sizeof(fugueParam),
    .blocksize = 64,
    .digestsize = 256/8,        /* XXX default to FUGUE-256 */
    .reset = (hashFunctionReset) fugueReset,
    .update = (hashFunctionUpdate) fugueUpdate,
    .digest = (hashFunctionDigest) fugueDigest
};

enum   { SUCCESS = 0, FAIL = 1, BAD_HASHBITLEN = 2 };

/* ===== "fugue.h" */

#if WORDS_BIGENDIAN
#define BE2HO_8(_x)  (_x)
#define HO2BE_8(_x)  (_x)
#define BE2HO_4(_x)  (_x)
#define HO2BE_4(_x)  (_x)
#define LE2HO_4(_x)  ((_x<<24)|((_x<<8)&0xff0000)|((_x>>8)&0xff00)|(_x>>24))
#define HO2LE_4(_x)  LE2HO_4(_x)
#else
#define BE2HO_8(_x)  ((_x<<56)|((_x<<40)&0xff000000000000ull)|((_x<<24)&0xff0000000000ull)|((_x<<8)&0xff00000000ull)|\
                     ((_x>>8)&0xff000000ull)|((_x>>24)&0xff0000ull)|((_x>>40)&0xff00ull)|(_x>>56))
#define HO2BE_8(_x)  BE2HO_8(_x)
#define BE2HO_4(_x)  ((_x<<24)|((_x<<8)&0xff0000)|((_x>>8)&0xff00)|(_x>>24))
#define HO2BE_4(_x)  BE2HO_4(_x)
#define LE2HO_4(_x)  (_x)
#define HO2LE_4(_x)  (_x)
#endif

/* ===== */

static inline
unsigned COLUMN(fugueParam* sp, unsigned col)
{
    unsigned x = sp->Base + col;
    return (x < (unsigned)sp->Cfg->s ? x : x - sp->Cfg->s);
}

#define STATE(_s,_c)     _s->State[COLUMN(_s,_c)].d
#define BYTES(_s,_r,_c)  _s->State[COLUMN(_s,_c)].b[_r]
#define ENTRY(_s,_r,_c)  _s[_c].b[_r]
#define ROTATE(_s,_r)    _s->Base = COLUMN(_s,_s->Cfg->s-_r)

#define cdiv(_n,_d) ((_n + _d - 1) / _d)

static const struct {
    int       hashbitlen;
    hashCfg   Cfg;
    uint32_t    IV[16];
} hashSizes[] = {
    {224,{7 ,30,2,5,13},
      {	0x0d12c9f4, 0x57f78662, 0x1ce039ee, 0xcbe374e0,
	0x627c12a1, 0x15d2439a, 0x9a678dbd, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000 }},
    {256,{8 ,30,2,5,13},
      {	0xdebd52e9, 0x5f137166, 0x68f6d4e0, 0x94b5b0d2,
	0x1d626cf9, 0xde29f9fb, 0x99e84991, 0x48c2f834,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000 }},
    {384,{12,36,3,6,13},
      {	0x0dec61aa, 0x1f2e2531, 0xc7b41da0, 0x85096000,
	0x4af45e21, 0x9c5e1b74, 0x9a3e69fa, 0x40b03e47,
	0x8aae02e5, 0xe0259ca9, 0x7c5195bc, 0xa195105c,
	0x00000000, 0x00000000, 0x00000000, 0x00000000 }},
    {512,{16,36,4,8,13},
      { 0x7ea50788, 0x75af16e6, 0xdbe4d3c5, 0x27b09aac,
	0x17f115d9, 0x54cceeb6, 0x0b02e806, 0xd1ef924a,
	0xc9e2c6aa, 0x9813b2dd, 0x3858e6ca, 0x3f207f43,
	0xe778ea25, 0xd6dd1f95, 0x1dd16eda, 0x67353ee1 }},
    {0}
};

#ifdef	UNUSED
static uint8_t    init_fugue = 1;
#endif

static const hash32_s M[4] = {
    {0x04070101}, {0x07010104}, {0x01010407}, {0x01040701},
};

static const union {
    uint64_t   d;
    uint8_t    b[8];
} gf2mul[256] = {
{0x0000000000000000ull},{0x0706050403020100ull},{0x0e0c0a0806040200ull},{0x090a0f0c05060300ull},
{0x1c1814100c080400ull},{0x1b1e11140f0a0500ull},{0x12141e180a0c0600ull},{0x15121b1c090e0700ull},
{0x3830282018100800ull},{0x3f362d241b120900ull},{0x363c22281e140a00ull},{0x313a272c1d160b00ull},
{0x24283c3014180c00ull},{0x232e3934171a0d00ull},{0x2a243638121c0e00ull},{0x2d22333c111e0f00ull},
{0x7060504030201000ull},{0x7766554433221100ull},{0x7e6c5a4836241200ull},{0x796a5f4c35261300ull},
{0x6c7844503c281400ull},{0x6b7e41543f2a1500ull},{0x62744e583a2c1600ull},{0x65724b5c392e1700ull},
{0x4850786028301800ull},{0x4f567d642b321900ull},{0x465c72682e341a00ull},{0x415a776c2d361b00ull},
{0x54486c7024381c00ull},{0x534e6974273a1d00ull},{0x5a446678223c1e00ull},{0x5d42637c213e1f00ull},
{0xe0c0a08060402000ull},{0xe7c6a58463422100ull},{0xeeccaa8866442200ull},{0xe9caaf8c65462300ull},
{0xfcd8b4906c482400ull},{0xfbdeb1946f4a2500ull},{0xf2d4be986a4c2600ull},{0xf5d2bb9c694e2700ull},
{0xd8f088a078502800ull},{0xdff68da47b522900ull},{0xd6fc82a87e542a00ull},{0xd1fa87ac7d562b00ull},
{0xc4e89cb074582c00ull},{0xc3ee99b4775a2d00ull},{0xcae496b8725c2e00ull},{0xcde293bc715e2f00ull},
{0x90a0f0c050603000ull},{0x97a6f5c453623100ull},{0x9eacfac856643200ull},{0x99aaffcc55663300ull},
{0x8cb8e4d05c683400ull},{0x8bbee1d45f6a3500ull},{0x82b4eed85a6c3600ull},{0x85b2ebdc596e3700ull},
{0xa890d8e048703800ull},{0xaf96dde44b723900ull},{0xa69cd2e84e743a00ull},{0xa19ad7ec4d763b00ull},
{0xb488ccf044783c00ull},{0xb38ec9f4477a3d00ull},{0xba84c6f8427c3e00ull},{0xbd82c3fc417e3f00ull},
{0xdb9b5b1bc0804000ull},{0xdc9d5e1fc3824100ull},{0xd5975113c6844200ull},{0xd2915417c5864300ull},
{0xc7834f0bcc884400ull},{0xc0854a0fcf8a4500ull},{0xc98f4503ca8c4600ull},{0xce894007c98e4700ull},
{0xe3ab733bd8904800ull},{0xe4ad763fdb924900ull},{0xeda77933de944a00ull},{0xeaa17c37dd964b00ull},
{0xffb3672bd4984c00ull},{0xf8b5622fd79a4d00ull},{0xf1bf6d23d29c4e00ull},{0xf6b96827d19e4f00ull},
{0xabfb0b5bf0a05000ull},{0xacfd0e5ff3a25100ull},{0xa5f70153f6a45200ull},{0xa2f10457f5a65300ull},
{0xb7e31f4bfca85400ull},{0xb0e51a4fffaa5500ull},{0xb9ef1543faac5600ull},{0xbee91047f9ae5700ull},
{0x93cb237be8b05800ull},{0x94cd267febb25900ull},{0x9dc72973eeb45a00ull},{0x9ac12c77edb65b00ull},
{0x8fd3376be4b85c00ull},{0x88d5326fe7ba5d00ull},{0x81df3d63e2bc5e00ull},{0x86d93867e1be5f00ull},
{0x3b5bfb9ba0c06000ull},{0x3c5dfe9fa3c26100ull},{0x3557f193a6c46200ull},{0x3251f497a5c66300ull},
{0x2743ef8bacc86400ull},{0x2045ea8fafca6500ull},{0x294fe583aacc6600ull},{0x2e49e087a9ce6700ull},
{0x036bd3bbb8d06800ull},{0x046dd6bfbbd26900ull},{0x0d67d9b3bed46a00ull},{0x0a61dcb7bdd66b00ull},
{0x1f73c7abb4d86c00ull},{0x1875c2afb7da6d00ull},{0x117fcda3b2dc6e00ull},{0x1679c8a7b1de6f00ull},
{0x4b3babdb90e07000ull},{0x4c3daedf93e27100ull},{0x4537a1d396e47200ull},{0x4231a4d795e67300ull},
{0x5723bfcb9ce87400ull},{0x5025bacf9fea7500ull},{0x592fb5c39aec7600ull},{0x5e29b0c799ee7700ull},
{0x730b83fb88f07800ull},{0x740d86ff8bf27900ull},{0x7d0789f38ef47a00ull},{0x7a018cf78df67b00ull},
{0x6f1397eb84f87c00ull},{0x681592ef87fa7d00ull},{0x611f9de382fc7e00ull},{0x661998e781fe7f00ull},
{0xad2db6369b1b8000ull},{0xaa2bb33298198100ull},{0xa321bc3e9d1f8200ull},{0xa427b93a9e1d8300ull},
{0xb135a22697138400ull},{0xb633a72294118500ull},{0xbf39a82e91178600ull},{0xb83fad2a92158700ull},
{0x951d9e16830b8800ull},{0x921b9b1280098900ull},{0x9b11941e850f8a00ull},{0x9c17911a860d8b00ull},
{0x89058a068f038c00ull},{0x8e038f028c018d00ull},{0x8709800e89078e00ull},{0x800f850a8a058f00ull},
{0xdd4de676ab3b9000ull},{0xda4be372a8399100ull},{0xd341ec7ead3f9200ull},{0xd447e97aae3d9300ull},
{0xc155f266a7339400ull},{0xc653f762a4319500ull},{0xcf59f86ea1379600ull},{0xc85ffd6aa2359700ull},
{0xe57dce56b32b9800ull},{0xe27bcb52b0299900ull},{0xeb71c45eb52f9a00ull},{0xec77c15ab62d9b00ull},
{0xf965da46bf239c00ull},{0xfe63df42bc219d00ull},{0xf769d04eb9279e00ull},{0xf06fd54aba259f00ull},
{0x4ded16b6fb5ba000ull},{0x4aeb13b2f859a100ull},{0x43e11cbefd5fa200ull},{0x44e719bafe5da300ull},
{0x51f502a6f753a400ull},{0x56f307a2f451a500ull},{0x5ff908aef157a600ull},{0x58ff0daaf255a700ull},
{0x75dd3e96e34ba800ull},{0x72db3b92e049a900ull},{0x7bd1349ee54faa00ull},{0x7cd7319ae64dab00ull},
{0x69c52a86ef43ac00ull},{0x6ec32f82ec41ad00ull},{0x67c9208ee947ae00ull},{0x60cf258aea45af00ull},
{0x3d8d46f6cb7bb000ull},{0x3a8b43f2c879b100ull},{0x33814cfecd7fb200ull},{0x348749face7db300ull},
{0x219552e6c773b400ull},{0x269357e2c471b500ull},{0x2f9958eec177b600ull},{0x289f5deac275b700ull},
{0x05bd6ed6d36bb800ull},{0x02bb6bd2d069b900ull},{0x0bb164ded56fba00ull},{0x0cb761dad66dbb00ull},
{0x19a57ac6df63bc00ull},{0x1ea37fc2dc61bd00ull},{0x17a970ced967be00ull},{0x10af75cada65bf00ull},
{0x76b6ed2d5b9bc000ull},{0x71b0e8295899c100ull},{0x78bae7255d9fc200ull},{0x7fbce2215e9dc300ull},
{0x6aaef93d5793c400ull},{0x6da8fc395491c500ull},{0x64a2f3355197c600ull},{0x63a4f6315295c700ull},
{0x4e86c50d438bc800ull},{0x4980c0094089c900ull},{0x408acf05458fca00ull},{0x478cca01468dcb00ull},
{0x529ed11d4f83cc00ull},{0x5598d4194c81cd00ull},{0x5c92db154987ce00ull},{0x5b94de114a85cf00ull},
{0x06d6bd6d6bbbd000ull},{0x01d0b86968b9d100ull},{0x08dab7656dbfd200ull},{0x0fdcb2616ebdd300ull},
{0x1acea97d67b3d400ull},{0x1dc8ac7964b1d500ull},{0x14c2a37561b7d600ull},{0x13c4a67162b5d700ull},
{0x3ee6954d73abd800ull},{0x39e0904970a9d900ull},{0x30ea9f4575afda00ull},{0x37ec9a4176addb00ull},
{0x22fe815d7fa3dc00ull},{0x25f884597ca1dd00ull},{0x2cf28b5579a7de00ull},{0x2bf48e517aa5df00ull},
{0x96764dad3bdbe000ull},{0x917048a938d9e100ull},{0x987a47a53ddfe200ull},{0x9f7c42a13edde300ull},
{0x8a6e59bd37d3e400ull},{0x8d685cb934d1e500ull},{0x846253b531d7e600ull},{0x836456b132d5e700ull},
{0xae46658d23cbe800ull},{0xa940608920c9e900ull},{0xa04a6f8525cfea00ull},{0xa74c6a8126cdeb00ull},
{0xb25e719d2fc3ec00ull},{0xb55874992cc1ed00ull},{0xbc527b9529c7ee00ull},{0xbb547e912ac5ef00ull},
{0xe6161ded0bfbf000ull},{0xe11018e908f9f100ull},{0xe81a17e50dfff200ull},{0xef1c12e10efdf300ull},
{0xfa0e09fd07f3f400ull},{0xfd080cf904f1f500ull},{0xf40203f501f7f600ull},{0xf30406f102f5f700ull},
{0xde2635cd13ebf800ull},{0xd92030c910e9f900ull},{0xd02a3fc515effa00ull},{0xd72c3ac116edfb00ull},
{0xc23e21dd1fe3fc00ull},{0xc53824d91ce1fd00ull},{0xcc322bd519e7fe00ull},{0xcb342ed11ae5ff00ull},
};

static const uint8_t aessub[256] = {
0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};

static inline
void Input_Mix(fugueParam* sp, uint32_t sc)
{
    int s = sp->Cfg->s;
    int k = sp->Cfg->k;
    int i;

    STATE(sp, 6*k-2) ^= STATE(sp, 0);
    STATE(sp, 0)      = sc;
    STATE(sp, 8)     ^= sc;
    for (i = 0; i <= k-2; i++)
	STATE(sp, 3*i+1) ^= STATE(sp, s-3*k+3*i);
}

static inline
void ROR3_Col_Mix(fugueParam* sp)
{
    int s = sp->Cfg->s;
    int i;

    ROTATE(sp, 3);
    for (i = 0; i < 3; i++) {
	STATE(sp, i)     ^= STATE(sp, 4+i);
	STATE(sp, s/2+i) ^= STATE(sp, 4+i);
    }
}

static inline
void Col_Xor(fugueParam* sp, int a, int b, int c)
{
    STATE(sp, 4)        ^= STATE(sp, 0);
    if (a) STATE(sp, a) ^= STATE(sp, 0);
    if (b) STATE(sp, b) ^= STATE(sp, 0);
    if (c) STATE(sp, c) ^= STATE(sp, 0);
}

static inline
void Col_Xor_RORn(fugueParam* sp, int ror, int a, int b, int c)
{
    Col_Xor(sp, a, b, c);
    ROTATE(sp, ror);
}

#if 0
static inline
void Super_Mix(fugueParam* sp)
{
    hash32_s U[4], T1[4], T2[4], T3[4];
    unsigned     r, c, k;

    /** AES substitution **/
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    ENTRY(U, r, c) = aessub[BYTES(sp, r, c)];

    /**   M * U using multiplication table for entries in M  **/
    memset(T1, 0, sizeof (T1));
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    for (k = 0; k < 4; k++)
		ENTRY(T1, r, c) ^= gf2mul[ENTRY(U,k,c)].b[ENTRY(M,r,k)];

    /**   Create diagonal matrix Diag(U)   **/
    memset(T2, 0, sizeof (T2));
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    if (r != c) ENTRY(T2, r, r) ^= ENTRY(U, r, c);

    /**   Multiply T2 (Diag) by M transpose using multiplication table for entries in M   **/
    memset(T3, 0, sizeof (T3));
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    for (k = 0; k < 4; k++)
		ENTRY(T3, r, c) ^= gf2mul[ENTRY(T2,r,k)].b[ENTRY(M,c,k)];

    /** Add and Rotate Left **/
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    BYTES(sp, r, (c-r)&3) = ENTRY(T1, r, c) ^ ENTRY(T3, r, c);
}
#else
static inline
void Super_Mix(fugueParam* sp)
{
    hash32_s U[4],D,W[4];
    unsigned     r, c;

    /** AES substitution **/
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    ENTRY(U, r, c) = aessub[BYTES(sp, r, c)];

    /**   Create diagonal matrix Diag(U)   **/
    memset(&D, 0, sizeof (D));
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    if (r != c) D.b[r] ^= ENTRY(U, r, c);

    /**   M * U + D * M transpose using multiplication table for entries in M  **/
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    ENTRY(W, r, c) = gf2mul[ENTRY(U,0,c)].b[ENTRY(M,r,0)] 
			   ^ gf2mul[ENTRY(U,1,c)].b[ENTRY(M,r,1)] 
			   ^ gf2mul[ENTRY(U,2,c)].b[ENTRY(M,r,2)] 
			   ^ gf2mul[ENTRY(U,3,c)].b[ENTRY(M,r,3)] 
			   ^ gf2mul[D.b[r]].b[ENTRY(M,c,r)];      // only non-zero terms in diagonal matrix

    /** Rotate Left **/
    for (r = 0; r < 4; r++)
	for (c = 0; c < 4; c++)
	    BYTES(sp, r, (c-r)&3) = ENTRY(W, r, c);
}
#endif

static unsigned
Next_Fugue(fugueParam* sp, const uint32_t* msg, uint64_t len)
{
    unsigned k;

    if (!sp || !sp->Cfg) return 0;
    while (len--) {
	Input_Mix(sp, *msg++);
	for (k = 0; k < (unsigned)sp->Cfg->k; k++) {
	    ROR3_Col_Mix(sp);
	    Super_Mix(sp);
	}
    }
    return 1;
}

static unsigned
Done_Fugue(fugueParam* sp, uint32_t* md, int* hashwordlen)
{
    if (sp && sp->Cfg) {
	int n = sp->Cfg->n;
	int s = sp->Cfg->s;
	int k = sp->Cfg->k;
	int r = sp->Cfg->r;
	int t = sp->Cfg->t;
	int N = cdiv (n, 4);
	int p = s / N, j;
	/* final G1 rounds */
	for (j = 0; j < r*k; j++) {
	    ROR3_Col_Mix(sp);
	    Super_Mix(sp);
	}
	/* final G2 rounds */
	switch (N) {
	case 1:
	    for (j = 0; j < t; j++) {
		Col_Xor_RORn(sp, s-1, 0, 0, 0);
		Super_Mix(sp);
	    }
	    Col_Xor(sp, 0, 0, 0);
	    break;
	case 2:
	    for (j = 0; j < t; j++) {
		Col_Xor_RORn(sp, p,   p,   0, 0);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p-1, p+1, 0, 0);
		Super_Mix(sp);
	    }
	    Col_Xor(sp, p, 0, 0);
	    break;
	case 3:
	    for (j = 0; j < t; j++) {
		Col_Xor_RORn(sp, p,   p,   2*p,   0);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p,   p+1, 2*p,   0);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p-1, p+1, 2*p+1, 0);
		Super_Mix(sp);
	    }
	    Col_Xor(sp, p, 2*p, 0);
	    break;
	case 4:
	    for (j = 0; j < t; j++) {
		Col_Xor_RORn(sp, p,   p,   2*p,   3*p);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p,   p+1, 2*p,   3*p);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p,   p+1, 2*p+1, 3*p);
		Super_Mix(sp);
		Col_Xor_RORn(sp, p-1, p+1, 2*p+1, 3*p+1);
		Super_Mix(sp);
	    }
	    Col_Xor(sp, p, 2*p, 3*p);
	    break;
	}
	/* digest output */
	for (j = 0; j < 4 && j < n; j++)
	    *md++ = STATE(sp, j+1);
	for (r = 1; r <= N-2; r++)
	    for (j = 0; j < 4; j++)
		*md++ = STATE(sp, r*p+j);
	if (n > 4)
	    for (j = 0; j+r*4 < n; j++)
		*md++ = STATE(sp, s-p+j);
	if (hashwordlen) *hashwordlen = n;
	return 1;
    }
    return 0;
}

#ifdef	UNUSED
static
void Fugue_IV(void)
{
    fugueParam HS;
    int i;

    for (i = 0; hashSizes[i].hashbitlen; i++) {
	uint32_t Data = HO2BE_4 ((uint32_t)hashSizes[i].Cfg.n * 32);
	memset(&HS, 0, sizeof (fugueParam));
	HS.hashbitlen = hashSizes[i].Cfg.n * 32;
	HS.Cfg = &hashSizes[i].Cfg;
	Next_Fugue(&HS, &Data, 1);
	Done_Fugue(&HS, hashSizes[i].IV, NULL);
    }
}

#define POLY 0x11b
#define gf_0(x)   (0)
#define gf_1(x)   (x)
#define gf_2(x)   ((x<<1) ^ (((x>>7) & 1) * POLY))
#define gf_3(x)   (gf_2(x) ^ gf_1(x))
#define gf_4(x)   (gf_2(gf_2(x)))
#define gf_5(x)   (gf_4(x) ^ gf_1(x))
#define gf_6(x)   (gf_4(x) ^ gf_2(x))
#define gf_7(x)   (gf_4(x) ^ gf_2(x) ^ gf_1(x))

static
void Init_Fugue(void)
{
    uint32_t i, j, k, sb, X[8], SB[256];

    if (init_fugue) return;
    init_fugue = 1;

/** multiplicative inverse with result repeated in 2 least significant bytes **/
    SB[0] = 0;
    SB[1] = (1 << 8) | 1;
    for (i = 2; i < 256; i++) {
	X[0] = i;
	for (j = 1; j < 8; j++)
	    if ((X[j] = X[j-1] << 1) & 256) X[j] ^= POLY;
	for (j = 2; j < 256; j++) {
	    sb = 0;
	    for (k = 0; k < 8; k++)
		if (j & (1 << k)) sb ^= X[k];
	    if (sb == 1) break;
	}
	SB[i] = (j << 8) | j;
    }
    for (i = 0; i < 256; i++) {
/** affine assuming repeated inverse from above **/
	aessub[i] = SB[i] ^ 0x63;
	for (j = 4; j < 8; j++) aessub[i] ^= (SB[i] >> j);
/** GF2 multiplication table with n = 0 through 7 **/
	gf2mul[i].b[0] = gf_0(i);
	gf2mul[i].b[1] = gf_1(i);
	gf2mul[i].b[2] = gf_2(i);
	gf2mul[i].b[3] = gf_3(i);
	gf2mul[i].b[4] = gf_4(i);
	gf2mul[i].b[5] = gf_5(i);
	gf2mul[i].b[6] = gf_6(i);
	gf2mul[i].b[7] = gf_7(i);
    }

/** generate M matrix **/
    ENTRY(M, 0, 0) = 1; ENTRY(M, 0, 1) = 4; ENTRY(M, 0, 2) = 7; ENTRY(M, 0, 3) = 1;
    ENTRY(M, 1, 0) = 1; ENTRY(M, 1, 1) = 1; ENTRY(M, 1, 2) = 4; ENTRY(M, 1, 3) = 7;
    ENTRY(M, 2, 0) = 7; ENTRY(M, 2, 1) = 1; ENTRY(M, 2, 2) = 1; ENTRY(M, 2, 3) = 4;
    ENTRY(M, 3, 0) = 4; ENTRY(M, 3, 1) = 7; ENTRY(M, 3, 2) = 1; ENTRY(M, 3, 3) = 1;

/** generate initial values **/
    Fugue_IV();
}

static
unsigned Load_Fugue(fugueParam* sp, int hashbitlen, const uint32_t* iv, int hashwordlen)
{
    int i;

    Init_Fugue();
    if (!sp) return 0;
    memset(sp, 0, sizeof (fugueParam));
    for (i = 0; hashSizes[i].hashbitlen; i++)
	if (hashSizes[i].hashbitlen == hashbitlen && hashSizes[i].Cfg.n == hashwordlen) {
	    int n = hashSizes[i].Cfg.n;
	    int s = hashSizes[i].Cfg.s;
	    sp->hashbitlen = n * 32;
	    sp->Cfg = &hashSizes[i].Cfg;
	    memcpy(&sp->State[s-n].d, iv, n * 4);
	    return 1;
	}
    return 0;
}
#endif	/* UNUSED */

int
fugueInit(fugueParam *sp, int hashbitlen)
{
    int i;

#ifdef	UNUSED
    Init_Fugue();
#endif
    for (i = 0; hashSizes[i].hashbitlen; i++)
	if (hashSizes[i].hashbitlen == hashbitlen) {
	    int n = hashSizes[i].Cfg.n;
	    int s = hashSizes[i].Cfg.s;
	    memset(sp, 0, sizeof (fugueParam));
	    sp->hashbitlen = n * 32;
	    sp->Cfg = &hashSizes[i].Cfg;
	    memcpy(&sp->State[s-n].d, hashSizes[i].IV, n * 4);
	    return SUCCESS;
	}
    return FAIL;
}

int
fugueReset(fugueParam *sp)
{
    return fugueInit(sp, sp->hashbitlen);
}

int
fugueUpdate(fugueParam *sp, const byte *data, size_t size)
{
    uint64_t databitlen = 8 * size;
    if (!sp || !sp->Cfg)
	return FAIL;
    if (!databitlen)
	return SUCCESS;
    if (sp->TotalBits & 7)
	return FAIL;
    if (sp->TotalBits & 31) {
	unsigned int need = 32 - (sp->TotalBits & 31);
	if (need > databitlen) {
	    memcpy((uint8_t*)sp->Partial + ((sp->TotalBits & 31)/8), data, (databitlen + 7)/8);
	    sp->TotalBits += databitlen;
	    return SUCCESS;
	} else {
	    memcpy((uint8_t*)sp->Partial + ((sp->TotalBits & 31)/8), data, need/8);
	    Next_Fugue(sp, sp->Partial, 1);
	    sp->TotalBits += need;
	    databitlen -= need;
	    data += need/8;
	}
    }
    if (databitlen>31) {
	Next_Fugue(sp, (uint32_t*)data, databitlen/32);
	sp->TotalBits += (databitlen/32)*32;
	data += (databitlen/32)*4;
	databitlen &= 31;
    }
    if (databitlen) {
	memcpy((uint8_t*)sp->Partial, data, (databitlen + 7)/8);
	sp->TotalBits += databitlen;
    }
    return SUCCESS;
}

int
fugueDigest(fugueParam *sp, byte *digest)
{
    if (!sp || !sp->Cfg)
	return FAIL;
    if (sp->TotalBits & 31) {
	int need = 32 - (sp->TotalBits & 31);
	memset((uint8_t*)sp->Partial + ((sp->TotalBits & 31)/8), 0, need/8);
	Next_Fugue(sp, sp->Partial, 1);
    }
    sp->TotalBits = BE2HO_8 (sp->TotalBits);
    Next_Fugue(sp, (uint32_t*)&sp->TotalBits, 2);
    Done_Fugue(sp, (uint32_t*)digest, NULL);
    return SUCCESS;
}
