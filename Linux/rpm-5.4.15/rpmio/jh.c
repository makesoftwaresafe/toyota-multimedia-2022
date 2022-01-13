/* This program gives the reference implementation of JH.
 * 1) The description given in this program is suitable for implementation in hardware
 * 2) All the operations are operated on bytes, so the description is suitable for implementation on 8-bit processor
 */

#include <jh.h>

enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 };

const hashFunction jh256 = {
    .name = "JH-256",
    .paramsize = sizeof(jhParam),
    .blocksize = 64,
    .digestsize = 256/8,	/* XXX default to JH-256 */
    .reset = (hashFunctionReset) jhReset,
    .update = (hashFunctionUpdate) jhUpdate,
    .digest = (hashFunctionDigest) jhDigest
};

/*The initial hash value H(0)*/
static
const unsigned char JH224_H0[128] __attribute__((aligned(16))) = {
0x82,0xc2,0x70,0xe0,0x0b,0xed,0x02,0x30,0x8d,0x0c,0x3a,0x9e,0x31,0xce,0x34,0xb1,
0x8f,0x0c,0x94,0x2f,0xba,0x46,0xcd,0x87,0x1e,0xc4,0xd8,0x0a,0xfc,0x79,0x71,0xc4,
0x61,0xe0,0x1a,0xbb,0x69,0x96,0x2d,0x7b,0xaf,0x71,0x89,0x3d,0xe1,0x3d,0x86,0x97,
0xd2,0x52,0x04,0x60,0xf7,0xc9,0xc0,0x94,0xc7,0x63,0x49,0xca,0x3d,0xa5,0x79,0x9c,
0xfd,0x8b,0x55,0x1f,0xbd,0xbc,0xeb,0x9f,0x08,0x34,0xbd,0x5b,0xb4,0x42,0xf8,0xbf,
0xba,0x51,0x5c,0x35,0xb9,0xc7,0x99,0x9e,0x55,0xa4,0x4e,0x62,0x71,0xcc,0x13,0xb3,
0x85,0x72,0x57,0x93,0xc1,0x85,0xf7,0x25,0x45,0x36,0x6b,0x69,0x00,0x50,0x25,0xd2,
0x33,0x90,0xeb,0xdb,0x27,0xdd,0x1e,0xdf,0xcc,0xba,0xad,0xe1,0x7e,0x60,0x3d,0xe9
};

static
const unsigned char JH256_H0[128] __attribute__((aligned(16))) = {
0xc9,0x68,0xb8,0xe2,0xc5,0x3a,0x59,0x6e,0x42,0x7e,0x45,0xef,0x1d,0x7a,0xe6,0xe5,
0x61,0x45,0xb7,0xd9,0x06,0x71,0x1f,0x7a,0x2f,0xc7,0x61,0x78,0x06,0xa9,0x22,0x01,
0x7b,0x29,0x91,0xc1,0xb9,0x19,0x29,0xe2,0xc4,0x2b,0x4c,0xe1,0x8c,0xc5,0xa2,0xd6,
0x62,0x20,0xbe,0xca,0x90,0x1b,0x5d,0xdf,0xd3,0xb2,0x05,0x63,0x8e,0xa7,0xac,0x5f,
0x14,0x3e,0x8c,0xba,0x6d,0x31,0x31,0x04,0xb0,0xe7,0x00,0x54,0x90,0x52,0x72,0x71,
0x4c,0xce,0x32,0x1e,0x07,0x5d,0xe5,0x10,0x1b,0xa8,0x00,0xec,0xe2,0x02,0x51,0x78,
0x9f,0x57,0x72,0x79,0x5f,0xd1,0x04,0xa5,0xf0,0xb8,0xb6,0x34,0x25,0xf5,0xb2,0x38,
0x16,0x70,0xfa,0x3e,0x5f,0x90,0x7f,0x17,0xe2,0x8f,0xc0,0x64,0xe7,0x69,0xac,0x90
};

static
const unsigned char JH384_H0[128] __attribute__((aligned(16))) = {
0x07,0x9c,0x23,0xab,0x64,0xab,0x2d,0x40,0x8c,0xb5,0x1c,0xe4,0x47,0xde,0xe9,0x8d,
0x8d,0x9b,0xb1,0x62,0x7e,0xc2,0x52,0x69,0xba,0xb6,0x2d,0x2b,0x00,0x2f,0xfc,0x80,
0xcb,0xaf,0xbc,0xef,0x30,0x8c,0x17,0x3a,0xad,0x6f,0xa3,0xaa,0x31,0x19,0x40,0x31,
0x89,0x89,0x77,0x42,0x3a,0x6f,0x4c,0xe3,0xbf,0x2e,0x73,0x2b,0x44,0x0d,0xdb,0x7d,
0xf2,0xc4,0x3e,0xca,0xa6,0x3a,0x54,0xe5,0x8a,0x37,0xb8,0x0a,0xfc,0x44,0x22,0xc5,
0xa3,0x97,0xc3,0xbc,0x04,0xe9,0xe0,0x91,0x37,0xa8,0x04,0x53,0xe1,0x48,0x60,0xfa,
0x71,0x31,0xd3,0x3a,0x5f,0xd4,0xbe,0xa6,0xdc,0xda,0x4a,0xf8,0xf4,0x33,0x85,0x12,
0x6e,0xc7,0xf8,0xf4,0xc8,0x49,0x58,0xd0,0x8b,0x9e,0x94,0xa3,0x46,0x95,0xb6,0xa9
};

static
const unsigned char JH512_H0[128] __attribute__((aligned(16))) = {
0x50,0xab,0x60,0x58,0xc6,0x09,0x42,0xcc,0x4c,0xe7,0xa5,0x4c,0xbd,0xb9,0xdc,0x1b,
0xaf,0x2e,0x7a,0xfb,0xd1,0xa1,0x5e,0x24,0xe5,0xf4,0x4e,0xab,0xc4,0xd5,0xc0,0xa1,
0x4c,0xf2,0x43,0x66,0x0c,0x56,0x20,0x73,0x99,0x93,0x81,0xea,0x9a,0x8b,0x3d,0x18,
0xcf,0x65,0xd9,0xfc,0xa9,0x40,0xb6,0xc7,0x9e,0x83,0x12,0x73,0xbe,0xfe,0x3b,0x66,
0x0f,0x9a,0x2f,0x7e,0x0a,0x32,0xd8,0xe0,0x17,0xd4,0x91,0x55,0x8e,0x0b,0x13,0x40,
0x05,0xb5,0xe4,0xde,0xc4,0x4e,0x5f,0x3f,0x8c,0xbc,0x5a,0xee,0x98,0xfd,0x1d,0x32,
0x14,0x08,0x1c,0x25,0xe4,0x6c,0xe6,0xc4,0x1b,0x4b,0x95,0xbc,0xe1,0xbd,0x43,0xdb,
0x7f,0x22,0x9e,0xc2,0x43,0xb6,0x80,0x14,0x0a,0x33,0xb9,0x09,0x33,0x3c,0x03,0x03
};

#if	defined(OPTIMIZE_SSE2)
/* 36 round constants, each round constant is 32-byte (256-bit) */
static
const unsigned char E8_bitslice_roundconstant00[32] __attribute__((aligned(16))) = {
0x72,0xd5,0xde,0xa2,0xdf,0x15,0xf8,0x67,0x7b,0x84,0x15,0x0a,0xb7,0x23,0x15,0x57,
0x81,0xab,0xd6,0x90,0x4d,0x5a,0x87,0xf6,0x4e,0x9f,0x4f,0xc5,0xc3,0xd1,0x2b,0x40
};
static
const unsigned char E8_bitslice_roundconstant01[32] __attribute__((aligned(16))) = {
0xea,0x98,0x3a,0xe0,0x5c,0x45,0xfa,0x9c,0x03,0xc5,0xd2,0x99,0x66,0xb2,0x99,0x9a,
0x66,0x02,0x96,0xb4,0xf2,0xbb,0x53,0x8a,0xb5,0x56,0x14,0x1a,0x88,0xdb,0xa2,0x31
};
static
const unsigned char E8_bitslice_roundconstant02[32] __attribute__((aligned(16))) = {
0x03,0xa3,0x5a,0x5c,0x9a,0x19,0x0e,0xdb,0x40,0x3f,0xb2,0x0a,0x87,0xc1,0x44,0x10,
0x1c,0x05,0x19,0x80,0x84,0x9e,0x95,0x1d,0x6f,0x33,0xeb,0xad,0x5e,0xe7,0xcd,0xdc
};
static
const unsigned char  E8_bitslice_roundconstant03[32] __attribute__((aligned(16))) = {
0x10,0xba,0x13,0x92,0x02,0xbf,0x6b,0x41,0xdc,0x78,0x65,0x15,0xf7,0xbb,0x27,0xd0,
0x0a,0x2c,0x81,0x39,0x37,0xaa,0x78,0x50,0x3f,0x1a,0xbf,0xd2,0x41,0x00,0x91,0xd3
};
static
const unsigned char  E8_bitslice_roundconstant04[32] __attribute__((aligned(16))) = {
0x42,0x2d,0x5a,0x0d,0xf6,0xcc,0x7e,0x90,0xdd,0x62,0x9f,0x9c,0x92,0xc0,0x97,0xce,
0x18,0x5c,0xa7,0x0b,0xc7,0x2b,0x44,0xac,0xd1,0xdf,0x65,0xd6,0x63,0xc6,0xfc,0x23
};
static
const unsigned char  E8_bitslice_roundconstant05[32] __attribute__((aligned(16))) = {
0x97,0x6e,0x6c,0x03,0x9e,0xe0,0xb8,0x1a,0x21,0x05,0x45,0x7e,0x44,0x6c,0xec,0xa8,
0xee,0xf1,0x03,0xbb,0x5d,0x8e,0x61,0xfa,0xfd,0x96,0x97,0xb2,0x94,0x83,0x81,0x97
};
static
const unsigned char  E8_bitslice_roundconstant06[32] __attribute__((aligned(16))) = {
0x4a,0x8e,0x85,0x37,0xdb,0x03,0x30,0x2f,0x2a,0x67,0x8d,0x2d,0xfb,0x9f,0x6a,0x95,
0x8a,0xfe,0x73,0x81,0xf8,0xb8,0x69,0x6c,0x8a,0xc7,0x72,0x46,0xc0,0x7f,0x42,0x14
};
static
const unsigned char  E8_bitslice_roundconstant07[32] __attribute__((aligned(16))) = {
0xc5,0xf4,0x15,0x8f,0xbd,0xc7,0x5e,0xc4,0x75,0x44,0x6f,0xa7,0x8f,0x11,0xbb,0x80,
0x52,0xde,0x75,0xb7,0xae,0xe4,0x88,0xbc,0x82,0xb8,0x00,0x1e,0x98,0xa6,0xa3,0xf4
};
static
const unsigned char  E8_bitslice_roundconstant08[32] __attribute__((aligned(16))) = {
0x8e,0xf4,0x8f,0x33,0xa9,0xa3,0x63,0x15,0xaa,0x5f,0x56,0x24,0xd5,0xb7,0xf9,0x89,
0xb6,0xf1,0xed,0x20,0x7c,0x5a,0xe0,0xfd,0x36,0xca,0xe9,0x5a,0x06,0x42,0x2c,0x36
};
static
const unsigned char  E8_bitslice_roundconstant09[32] __attribute__((aligned(16))) = {
0xce,0x29,0x35,0x43,0x4e,0xfe,0x98,0x3d,0x53,0x3a,0xf9,0x74,0x73,0x9a,0x4b,0xa7,
0xd0,0xf5,0x1f,0x59,0x6f,0x4e,0x81,0x86,0x0e,0x9d,0xad,0x81,0xaf,0xd8,0x5a,0x9f
};
static
const unsigned char  E8_bitslice_roundconstant10[32] __attribute__((aligned(16))) = {
0xa7,0x05,0x06,0x67,0xee,0x34,0x62,0x6a,0x8b,0x0b,0x28,0xbe,0x6e,0xb9,0x17,0x27,
0x47,0x74,0x07,0x26,0xc6,0x80,0x10,0x3f,0xe0,0xa0,0x7e,0x6f,0xc6,0x7e,0x48,0x7b
};
static
const unsigned char  E8_bitslice_roundconstant11[32] __attribute__((aligned(16))) = {
0x0d,0x55,0x0a,0xa5,0x4a,0xf8,0xa4,0xc0,0x91,0xe3,0xe7,0x9f,0x97,0x8e,0xf1,0x9e,
0x86,0x76,0x72,0x81,0x50,0x60,0x8d,0xd4,0x7e,0x9e,0x5a,0x41,0xf3,0xe5,0xb0,0x62
};
static
const unsigned char  E8_bitslice_roundconstant12[32] __attribute__((aligned(16))) = {
0xfc,0x9f,0x1f,0xec,0x40,0x54,0x20,0x7a,0xe3,0xe4,0x1a,0x00,0xce,0xf4,0xc9,0x84,
0x4f,0xd7,0x94,0xf5,0x9d,0xfa,0x95,0xd8,0x55,0x2e,0x7e,0x11,0x24,0xc3,0x54,0xa5
};
static
const unsigned char  E8_bitslice_roundconstant13[32] __attribute__((aligned(16))) = {
0x5b,0xdf,0x72,0x28,0xbd,0xfe,0x6e,0x28,0x78,0xf5,0x7f,0xe2,0x0f,0xa5,0xc4,0xb2,
0x05,0x89,0x7c,0xef,0xee,0x49,0xd3,0x2e,0x44,0x7e,0x93,0x85,0xeb,0x28,0x59,0x7f
};
static
const unsigned char  E8_bitslice_roundconstant14[32] __attribute__((aligned(16))) = {
0x70,0x5f,0x69,0x37,0xb3,0x24,0x31,0x4a,0x5e,0x86,0x28,0xf1,0x1d,0xd6,0xe4,0x65,
0xc7,0x1b,0x77,0x04,0x51,0xb9,0x20,0xe7,0x74,0xfe,0x43,0xe8,0x23,0xd4,0x87,0x8a
};
static
const unsigned char  E8_bitslice_roundconstant15[32] __attribute__((aligned(16))) = {
0x7d,0x29,0xe8,0xa3,0x92,0x76,0x94,0xf2,0xdd,0xcb,0x7a,0x09,0x9b,0x30,0xd9,0xc1,
0x1d,0x1b,0x30,0xfb,0x5b,0xdc,0x1b,0xe0,0xda,0x24,0x49,0x4f,0xf2,0x9c,0x82,0xbf
};
static
const unsigned char  E8_bitslice_roundconstant16[32] __attribute__((aligned(16))) = {
0xa4,0xe7,0xba,0x31,0xb4,0x70,0xbf,0xff,0x0d,0x32,0x44,0x05,0xde,0xf8,0xbc,0x48,
0x3b,0xae,0xfc,0x32,0x53,0xbb,0xd3,0x39,0x45,0x9f,0xc3,0xc1,0xe0,0x29,0x8b,0xa0
};
static
const unsigned char  E8_bitslice_roundconstant17[32] __attribute__((aligned(16))) = {
0xe5,0xc9,0x05,0xfd,0xf7,0xae,0x09,0x0f,0x94,0x70,0x34,0x12,0x42,0x90,0xf1,0x34,
0xa2,0x71,0xb7,0x01,0xe3,0x44,0xed,0x95,0xe9,0x3b,0x8e,0x36,0x4f,0x2f,0x98,0x4a
};
static
const unsigned char  E8_bitslice_roundconstant18[32] __attribute__((aligned(16))) = {
0x88,0x40,0x1d,0x63,0xa0,0x6c,0xf6,0x15,0x47,0xc1,0x44,0x4b,0x87,0x52,0xaf,0xff,
0x7e,0xbb,0x4a,0xf1,0xe2,0x0a,0xc6,0x30,0x46,0x70,0xb6,0xc5,0xcc,0x6e,0x8c,0xe6
};
static
const unsigned char  E8_bitslice_roundconstant19[32] __attribute__((aligned(16))) = {
0xa4,0xd5,0xa4,0x56,0xbd,0x4f,0xca,0x00,0xda,0x9d,0x84,0x4b,0xc8,0x3e,0x18,0xae,
0x73,0x57,0xce,0x45,0x30,0x64,0xd1,0xad,0xe8,0xa6,0xce,0x68,0x14,0x5c,0x25,0x67
};
static
const unsigned char  E8_bitslice_roundconstant20[32] __attribute__((aligned(16))) = {
0xa3,0xda,0x8c,0xf2,0xcb,0x0e,0xe1,0x16,0x33,0xe9,0x06,0x58,0x9a,0x94,0x99,0x9a,
0x1f,0x60,0xb2,0x20,0xc2,0x6f,0x84,0x7b,0xd1,0xce,0xac,0x7f,0xa0,0xd1,0x85,0x18
};
static
const unsigned char  E8_bitslice_roundconstant21[32] __attribute__((aligned(16))) = {
0x32,0x59,0x5b,0xa1,0x8d,0xdd,0x19,0xd3,0x50,0x9a,0x1c,0xc0,0xaa,0xa5,0xb4,0x46,
0x9f,0x3d,0x63,0x67,0xe4,0x04,0x6b,0xba,0xf6,0xca,0x19,0xab,0x0b,0x56,0xee,0x7e
};
static
const unsigned char  E8_bitslice_roundconstant22[32] __attribute__((aligned(16))) = {
0x1f,0xb1,0x79,0xea,0xa9,0x28,0x21,0x74,0xe9,0xbd,0xf7,0x35,0x3b,0x36,0x51,0xee,
0x1d,0x57,0xac,0x5a,0x75,0x50,0xd3,0x76,0x3a,0x46,0xc2,0xfe,0xa3,0x7d,0x70,0x01
};
static
const unsigned char  E8_bitslice_roundconstant23[32] __attribute__((aligned(16))) = {
0xf7,0x35,0xc1,0xaf,0x98,0xa4,0xd8,0x42,0x78,0xed,0xec,0x20,0x9e,0x6b,0x67,0x79,
0x41,0x83,0x63,0x15,0xea,0x3a,0xdb,0xa8,0xfa,0xc3,0x3b,0x4d,0x32,0x83,0x2c,0x83
};
static
const unsigned char  E8_bitslice_roundconstant24[32] __attribute__((aligned(16))) = {
0xa7,0x40,0x3b,0x1f,0x1c,0x27,0x47,0xf3,0x59,0x40,0xf0,0x34,0xb7,0x2d,0x76,0x9a,
0xe7,0x3e,0x4e,0x6c,0xd2,0x21,0x4f,0xfd,0xb8,0xfd,0x8d,0x39,0xdc,0x57,0x59,0xef
};
static
const unsigned char  E8_bitslice_roundconstant25[32] __attribute__((aligned(16))) = {
0x8d,0x9b,0x0c,0x49,0x2b,0x49,0xeb,0xda,0x5b,0xa2,0xd7,0x49,0x68,0xf3,0x70,0x0d,
0x7d,0x3b,0xae,0xd0,0x7a,0x8d,0x55,0x84,0xf5,0xa5,0xe9,0xf0,0xe4,0xf8,0x8e,0x65
};
static
const unsigned char  E8_bitslice_roundconstant26[32] __attribute__((aligned(16))) = {
0xa0,0xb8,0xa2,0xf4,0x36,0x10,0x3b,0x53,0x0c,0xa8,0x07,0x9e,0x75,0x3e,0xec,0x5a,
0x91,0x68,0x94,0x92,0x56,0xe8,0x88,0x4f,0x5b,0xb0,0x5c,0x55,0xf8,0xba,0xbc,0x4c
};
static
const unsigned char  E8_bitslice_roundconstant27[32] __attribute__((aligned(16))) = {
0xe3,0xbb,0x3b,0x99,0xf3,0x87,0x94,0x7b,0x75,0xda,0xf4,0xd6,0x72,0x6b,0x1c,0x5d,
0x64,0xae,0xac,0x28,0xdc,0x34,0xb3,0x6d,0x6c,0x34,0xa5,0x50,0xb8,0x28,0xdb,0x71
};
static
const unsigned char  E8_bitslice_roundconstant28[32] __attribute__((aligned(16))) = {
0xf8,0x61,0xe2,0xf2,0x10,0x8d,0x51,0x2a,0xe3,0xdb,0x64,0x33,0x59,0xdd,0x75,0xfc,
0x1c,0xac,0xbc,0xf1,0x43,0xce,0x3f,0xa2,0x67,0xbb,0xd1,0x3c,0x02,0xe8,0x43,0xb0
};
static
const unsigned char  E8_bitslice_roundconstant29[32] __attribute__((aligned(16))) = {
0x33,0x0a,0x5b,0xca,0x88,0x29,0xa1,0x75,0x7f,0x34,0x19,0x4d,0xb4,0x16,0x53,0x5c,
0x92,0x3b,0x94,0xc3,0x0e,0x79,0x4d,0x1e,0x79,0x74,0x75,0xd7,0xb6,0xee,0xaf,0x3f
};
static
const unsigned char  E8_bitslice_roundconstant30[32] __attribute__((aligned(16))) = {
0xea,0xa8,0xd4,0xf7,0xbe,0x1a,0x39,0x21,0x5c,0xf4,0x7e,0x09,0x4c,0x23,0x27,0x51,
0x26,0xa3,0x24,0x53,0xba,0x32,0x3c,0xd2,0x44,0xa3,0x17,0x4a,0x6d,0xa6,0xd5,0xad
};
static
const unsigned char  E8_bitslice_roundconstant31[32] __attribute__((aligned(16))) = {
0xb5,0x1d,0x3e,0xa6,0xaf,0xf2,0xc9,0x08,0x83,0x59,0x3d,0x98,0x91,0x6b,0x3c,0x56,
0x4c,0xf8,0x7c,0xa1,0x72,0x86,0x60,0x4d,0x46,0xe2,0x3e,0xcc,0x08,0x6e,0xc7,0xf6
};
static
const unsigned char  E8_bitslice_roundconstant32[32] __attribute__((aligned(16))) = {
0x2f,0x98,0x33,0xb3,0xb1,0xbc,0x76,0x5e,0x2b,0xd6,0x66,0xa5,0xef,0xc4,0xe6,0x2a,
0x06,0xf4,0xb6,0xe8,0xbe,0xc1,0xd4,0x36,0x74,0xee,0x82,0x15,0xbc,0xef,0x21,0x63
};
static
const unsigned char  E8_bitslice_roundconstant33[32] __attribute__((aligned(16))) = {
0xfd,0xc1,0x4e,0x0d,0xf4,0x53,0xc9,0x69,0xa7,0x7d,0x5a,0xc4,0x06,0x58,0x58,0x26,
0x7e,0xc1,0x14,0x16,0x06,0xe0,0xfa,0x16,0x7e,0x90,0xaf,0x3d,0x28,0x63,0x9d,0x3f
};
static
const unsigned char  E8_bitslice_roundconstant34[32] __attribute__((aligned(16))) = {
0xd2,0xc9,0xf2,0xe3,0x00,0x9b,0xd2,0x0c,0x5f,0xaa,0xce,0x30,0xb7,0xd4,0x0c,0x30,
0x74,0x2a,0x51,0x16,0xf2,0xe0,0x32,0x98,0x0d,0xeb,0x30,0xd8,0xe3,0xce,0xf8,0x9a
};
static
const unsigned char  E8_bitslice_roundconstant35[32] __attribute__((aligned(16))) = {
0x4b,0xc5,0x9e,0x7b,0xb5,0xf1,0x79,0x92,0xff,0x51,0xe6,0x6e,0x04,0x86,0x68,0xd3,
0x9b,0x23,0x4d,0x57,0xe6,0x96,0x67,0x31,0xcc,0xe6,0xa6,0xf3,0x17,0x0a,0x75,0x05
};

/* The following defines are operations on 128-bit word(s) */
#define CONSTANT(b)	_mm_set1_epi8((b)) /* set each byte to be "b" */

#define XOR(x,y)	_mm_xor_si128((x),(y))		/*XOR(x,y) = x ^ y */
#define AND(x,y)	_mm_and_si128((x),(y))		/*AND(x,y) = x & y */
#define ANDNOT(x,y)	_mm_andnot_si128((x),(y))		/*ANDNOT(x,y) = (!x) & y */
#define OR(x,y)		_mm_or_si128((x),(y))		/*OR(x,y)  = x | y */

#define SHR1(x)		_mm_srli_epi16((x), 1)		/*SHR1(x)  = x >> 1 */
#define SHR2(x)		_mm_srli_epi16((x), 2)		/*SHR2(x)  = x >> 2 */
#define SHR4(x)		_mm_srli_epi16((x), 4)		/*SHR4(x)  = x >> 4 */
#define SHR8(x)		_mm_slli_epi16((x), 8)		/*SHR8(x)  = x >> 8 */
#define SHR16(x)	_mm_slli_epi32((x), 16)		/*SHR16(x) = x >> 16 */
#define SHR32(x)	_mm_slli_epi64((x), 32)		/*SHR32(x) = x >> 32 */
#define SHR64(x)	_mm_slli_si128((x), 8)		/*SHR64(x) = x >> 64 */

#define SHL1(x)		_mm_slli_epi16((x), 1)		/*SHL1(x)  = x << 1 */
#define SHL2(x)		_mm_slli_epi16((x), 2)		/*SHL2(x)  = x << 2 */
#define SHL4(x)		_mm_slli_epi16((x), 4)		/*SHL4(x)  = x << 4 */
#define SHL8(x)		_mm_srli_epi16((x), 8)		/*SHL8(x)  = x << 8 */
#define SHL16(x)	_mm_srli_epi32((x), 16)		/*SHL16(x) = x << 16 */
#define SHL32(x)	_mm_srli_epi64((x), 32)		/*SHL32(x) = x << 32 */
#define SHL64(x)	_mm_srli_si128((x), 8)		/*SHL64(x) = x << 64 */

/* swapping bits 64i||64i+1||...||64i+31 with bits 64i+32||64i+33||...||64i+63 of bit x */
#define SWAP32(x)	_mm_shuffle_epi32((x),_MM_SHUFFLE(2,3,0,1))
/* swapping bits 128i||128i+1||...||128i+63 with bits 128i+64||128i+65||...||128i+127 of bit x */
#define SWAP64(x)	_mm_shuffle_epi32((x),_MM_SHUFFLE(1,0,3,2))

/* store the word x into memory address p, where p is a multiple of 16 bytes */    
#define STORE(x,p)	_mm_store_si128((__m128i *)(p), (x))
/* load 16 bytes from the memory address p into word x, where p is the multiple of 16 bytes */
#define LOAD(x,p)	(x) = _mm_load_si128((__m128i *)(p))

/* The linear transformation L */
#define L(x0,x1,x2,x3,x4,x5,x6,x7) \
    (x4) = XOR((x4),(x1)); \
    (x5) = XOR((x5),(x2)); \
    (x6) = XOR((x6),(x3)); \
    (x6) = XOR((x6),(x0)); \
    (x7) = XOR((x7),(x0)); \
    (x0) = XOR((x0),(x5)); \
    (x1) = XOR((x1),(x6)); \
    (x2) = XOR((x2),(x7)); \
    (x2) = XOR((x2),(x4)); \
    (x3) = XOR((x3),(x4)); 

/* implements the Sbox layer */
#define SS(x0,x1,x2,x3,x4,x5,x6,x7,a0,a4) \
    a1 = ANDNOT(x2,a0); \
    a5 = ANDNOT(x6,a4); \
    a3 = ANDNOT(x3,x2); \
    a7 = ANDNOT(x7,x6); \
    x0 = XOR(x0,a1); \
    x4 = XOR(x4,a5); \
    a1 = ANDNOT(x1,x2); \
    a5 = ANDNOT(x5,x6); \
    x3 = XOR(x3,CONSTANT(0xff)); \
    x7 = XOR(x7,CONSTANT(0xff)); \
    a2 = AND(x0,x1); \
    a6 = AND(x4,x5); \
    x0 = XOR(x0,a3); \
    x4 = XOR(x4,a7); \
    x3 = XOR(x3,a1); \
    x7 = XOR(x7,a5); \
    a0 = XOR(a0,a2); \
    a4 = XOR(a4,a6); \
    a1 = AND(x0,x2); \
    a5 = AND(x4,x6); \
    a2 = ANDNOT(x3,x0); \
    a6 = ANDNOT(x7,x4); \
    x1 = XOR(x1,a1); \
    x5 = XOR(x5,a5); \
    x2 = XOR(x2,a2); \
    x6 = XOR(x6,a6); \
    a3 = OR(x1,x3); \
    a7 = OR(x5,x7); \
    a1 = AND(x1,x2); \
    a5 = AND(x5,x6); \
    x0 = XOR(x0,a3); \
    x4 = XOR(x4,a7); \
    x3 = XOR(x3,a1); \
    x7 = XOR(x7,a5); \
    a2 = AND(a0,x0); \
    a6 = AND(a4,x4); \
    x2 = XOR(x2,a0); \
    x6 = XOR(x6,a4); \
    x1 = XOR(x1,a2); \
    x5 = XOR(x5,a6);

/* The linear transform of the 0th round */
#define lineartransform_R00(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*set each byte in a3 as 0x55*/ \
    a3 = CONSTANT(0x55); \
    /*swapping bit 2i with bit 2i+1 for x0,x1,x2 and x3*/ \
    a0 = AND(x4,a3); \
    a1 = AND(x5,a3); \
    a2 = AND(x6,a3); \
    a3 = AND(x7,a3); \
    x4 = XOR(x4,a0); \
    x5 = XOR(x5,a1); \
    x6 = XOR(x6,a2); \
    x7 = XOR(x7,a3); \
    a0 = SHL1(a0);   \
    a1 = SHL1(a1);   \
    a2 = SHL1(a2);   \
    a3 = SHL1(a3);   \
    x4 = SHR1(x4);   \
    x5 = SHR1(x5);   \
    x6 = SHR1(x6);   \
    x7 = SHR1(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }

/* The linear transform of the 1st round */
#define lineartransform_R01(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*set each byte in a3 as 0x33*/ \
    a3 = CONSTANT(0x33); \
    /*swapping bits 4i||4i+1 with bits 4i+2||4i+3 for x0,x1,x2 and x3*/ \
    a0 = AND(x4,a3); \
    a1 = AND(x5,a3); \
    a2 = AND(x6,a3); \
    a3 = AND(x7,a3); \
    x4 = XOR(x4,a0); \
    x5 = XOR(x5,a1); \
    x6 = XOR(x6,a2); \
    x7 = XOR(x7,a3); \
    a0 = SHL2(a0);   \
    a1 = SHL2(a1);   \
    a2 = SHL2(a2);   \
    a3 = SHL2(a3);   \
    x4 = SHR2(x4);   \
    x5 = SHR2(x5);   \
    x6 = SHR2(x6);   \
    x7 = SHR2(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }

/* The linear transform of the 2nd round */
#define lineartransform_R02(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*set each byte in a3 as 0x0f*/ \
    a3 = CONSTANT(0xf); \
    /*swapping bits 8i||8i+1||8i+2||8i+3 with bits 8i+4||8i+5||8i+6||8i+7 for x0,x1,x2 and x3*/ \
    a0 = AND(x4,a3); \
    a1 = AND(x5,a3); \
    a2 = AND(x6,a3); \
    a3 = AND(x7,a3); \
    x4 = XOR(x4,a0); \
    x5 = XOR(x5,a1); \
    x6 = XOR(x6,a2); \
    x7 = XOR(x7,a3); \
    a0 = SHL4(a0);   \
    a1 = SHL4(a1);   \
    a2 = SHL4(a2);   \
    a3 = SHL4(a3);   \
    x4 = SHR4(x4);   \
    x5 = SHR4(x5);   \
    x6 = SHR4(x6);   \
    x7 = SHR4(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }

/* The linear transform of the 3rd round */
#define lineartransform_R03(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*swapping bits 16i||16i+1||...||16i+7 with bits 16i+8||16i+9||...||16i+15 for x0,x1,x2 and x3*/ \
    a0 = SHL8(x4);   \
    a1 = SHL8(x5);   \
    a2 = SHL8(x6);   \
    a3 = SHL8(x7);   \
    x4 = SHR8(x4);   \
    x5 = SHR8(x5);   \
    x6 = SHR8(x6);   \
    x7 = SHR8(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }

/* The linear transform of the 4th round*/
#define lineartransform_R04(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  \
    L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*swapping bits 32i||32i+1||...||32i+15 with bits 32i+16||32i+17||...||32i+31 for x0,x1,x2 and x3*/ \
    a0 = SHL16(x4);   \
    a1 = SHL16(x5);   \
    a2 = SHL16(x6);   \
    a3 = SHL16(x7);   \
    x4 = SHR16(x4);   \
    x5 = SHR16(x5);   \
    x6 = SHR16(x6);   \
    x7 = SHR16(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }

#ifdef	REFERENCE
/* The linear transform of the 5th round */
#define lineartransform_R05(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*swapping bits 64i||64i+1||...||64i+31 with bits 64i+32||64i+33||...||64i+63 for x0,x1,x2 and x3*/ \
    a0 = SHL32(x4);   \
    a1 = SHL32(x5);   \
    a2 = SHL32(x6);   \
    a3 = SHL32(x7);   \
    x4 = SHR32(x4);   \
    x5 = SHR32(x5);   \
    x6 = SHR32(x6);   \
    x7 = SHR32(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }
#else	/* REFERENCE */
/* The linear transform of the 5th round -- faster */
#define lineartransform_R05(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*swapping bits 64i||64i+1||...||64i+31 with bits 64i+32||64i+33||...||64i+63 for x0,x1,x2 and x3*/ \
    x4 = SWAP32(x4);   \
    x5 = SWAP32(x5);   \
    x6 = SWAP32(x6);   \
    x7 = SWAP32(x7);   \
 }
#endif	/* REFERENCE */

#ifdef	REFERENCE
/* The linear transform of the 6th round */
#define lineartransform_R06(x0,x1,x2,x3,x4,x5,x6,x7) \
{  L(x0,x1,x2,x3,x4,x5,x6,x7);     \
    /*swapping bits 128i||128i+1||...||128i+63 with bits 128i+64||128i+65||...||128i+127 for x0,x1,x2 and x3*/ \
    a0 = SHL64(x4);   \
    a1 = SHL64(x5);   \
    a2 = SHL64(x6);   \
    a3 = SHL64(x7);   \
    x4 = SHR64(x4);   \
    x5 = SHR64(x5);   \
    x6 = SHR64(x6);   \
    x7 = SHR64(x7);   \
    x4 = OR(x4,a0);  \
    x5 = OR(x5,a1);  \
    x6 = OR(x6,a2);  \
    x7 = OR(x7,a3);  \
 }
#else	/* REFERENCE */
/* The linear transform of the 6th round -- faster */
#define lineartransform_R06(x0,x1,x2,x3,x4,x5,x6,x7) \
 {  L(x0,x1,x2,x3,x4,x5,x6,x7); \
    /*swapping bits 128i||128i+1||...||128i+63 with bits 128i+64||128i+65||...||128i+127 for x0,x1,x2 and x3*/ \
    x4 = SWAP64(x4);   \
    x5 = SWAP64(x5);   \
    x6 = SWAP64(x6);   \
    x7 = SWAP64(x7);   \
 }
#endif	/* REFERENCE */

#define lineartransform_R07(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R00((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R08(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R01((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R09(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R02((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R10(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R03((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R11(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R04((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R12(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R05((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R13(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R06((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R14(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R00((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R15(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R01((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R16(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R02((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R17(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R03((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R18(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R04((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R19(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R05((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R20(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R06((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R21(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R00((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R22(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R01((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R23(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R02((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R24(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R03((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R25(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R04((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R26(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R05((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R27(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R06((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R28(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R00((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R29(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R01((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R30(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R02((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R31(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R03((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R32(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R04((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R33(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R05((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 
#define lineartransform_R34(x0,x1,x2,x3,x4,x5,x6,x7)  lineartransform_R06((x0),(x1),(x2),(x3),(x4),(x5),(x6),(x7)) 

/* the following defines the round function */
#define round_function(nn) \
 {  LOAD(a0, E8_bitslice_roundconstant##nn); \
    LOAD(a4, E8_bitslice_roundconstant##nn+16); \
    SS(sp->x0,sp->x2,sp->x4,sp->x6,sp->x1,sp->x3,sp->x5,sp->x7,a0,a4); \
    lineartransform_R##nn(sp->x0,sp->x2,sp->x4,sp->x6,sp->x1,sp->x3,sp->x5,sp->x7); \
 }

/* the following defines the last half round */
#define last_half_round(nn) \
 {  LOAD(a0, E8_bitslice_roundconstant##nn); \
    LOAD(a4, E8_bitslice_roundconstant##nn+16); \
    SS(sp->x0,sp->x2,sp->x4,sp->x6,sp->x1,sp->x3,sp->x5,sp->x7,a0,a4); \
 }

#else	/* defined(OPTIMIZE_SSE2) */

/* The constant for the Round 0 of E8 */
const unsigned char roundconstant_zero[64] = {
    0x6,0xa,0x0,0x9,0xe,0x6,0x6,0x7,0xf,0x3,0xb,0xc,0xc,0x9,0x0,0x8,
    0xb,0x2,0xf,0xb,0x1,0x3,0x6,0x6,0xe,0xa,0x9,0x5,0x7,0xd,0x3,0xe,
    0x3,0xa,0xd,0xe,0xc,0x1,0x7,0x5,0x1,0x2,0x7,0x7,0x5,0x0,0x9,0x9,
    0xd,0xa,0x2,0xf,0x5,0x9,0x0,0xb,0x0,0x6,0x6,0x7,0x3,0x2,0x2,0xa
};

/* The two Sboxes S0 and S1 */
unsigned char S[2][16] = {
    {9,0,4,11,13,12,3,15,1,10,2,6,7,5,8,14},
    {3,12,6,13,5,7,1,9,15,2,0,4,11,10,14,8}
};

/* The linear transformation L */
#define L(a, b) { \
    (b) ^= ( ( (a) << 1) ^ ( (a) >> 3) ^ (( (a) >> 2) & 2) ) & 0xf; \
    (a) ^= ( ( (b) << 1) ^ ( (b) >> 3) ^ (( (b) >> 2) & 2) ) & 0xf; \
}   

/* round function */
static
void R8(jhParam *sp) 
{
    unsigned char roundconstant_expanded[256];
    unsigned char tem[256];
    unsigned char t;
    unsigned int i;

    /* expand the round constant into 256 one-bit element */
    for (i = 0; i < 256; i++)
	roundconstant_expanded[i] = (sp->roundconstant[i >> 2] >> (3 - (i & 3)) ) & 1;

    /* S box layer, each constant bit selects one Sbox from S0 and S1 */
    /* constant bits are used to determine which Sbox to use */
    for (i = 0; i < 256; i++)
	tem[i] = S[roundconstant_expanded[i]][sp->A[i]];

    /* L Layer */
    for (i = 0; i < 256; i += 2)
	L(tem[i], tem[i+1]);

    /* The following is the permuation layer P_8$ */

    /* initial swap Pi_8 */
    for (i = 0; i < 256; i += 4) {
	t = tem[i+2];
	tem[i+2] = tem[i+3];
	tem[i+3] = t;
    }

    /* permutation P'_8 */
    for (i = 0; i < 128; i++) {
	sp->A[i] = tem[i<<1];
	sp->A[i+128] = tem[(i<<1)+1];
    }

    /* final swap Phi_8 */
    for (i = 128; i < 256; i += 2) {
	t = sp->A[i];
	sp->A[i] = sp->A[i+1];
	sp->A[i+1] = t;
    }
}

/* last half round of R8, only Sbox layer in this last half round */
static
void last_half_round_R8(jhParam *sp) 
{
    unsigned char roundconstant_expanded[256];
    unsigned int i;

    /* expand the round constant into one-bit element */
    for (i = 0; i < 256; i++)
	roundconstant_expanded[i] = (sp->roundconstant[i >>2] >> (3 - (i & 3)) ) & 1;

    /* S box layer */
    /* constant bits are used to determine which Sbox to use */
    for (i = 0; i < 256; i++)
	sp->A[i] = S[roundconstant_expanded[i]][sp->A[i]];
}

/* The following function generates the next round constant from 
 * the current round constant;  R6 is used for generating round constants for E8, and
 * the round constants of R6 is set as 0;
 */
static
void update_roundconstant(jhParam *sp)
{
    unsigned char tem[64];
    unsigned char t;
    int i;

    /* Sbox layer */
    for (i = 0; i < 64; i++)
	tem[i] = S[0][sp->roundconstant[i]];

    /* Linear transformation layer L */
    for (i = 0; i < 64; i += 2)
	L(tem[i], tem[i+1]);

    /* The following is the permutation layer P_6 */

    /* initial swap Pi_6 */
    for (i = 0; i < 64; i += 4) {
	t = tem[i+2];
	tem[i+2] = tem[i+3];
	tem[i+3] = t;
    }

    /* permutation P'_6 */
    for (i = 0; i < 32; i++) {
	sp->roundconstant[i]    = tem[i<<1];
	sp->roundconstant[i+32] = tem[(i<<1)+1];
    }

    /* final swap Phi_6 */
    for (i = 32; i < 64; i += 2) {
	t = sp->roundconstant[i];
	sp->roundconstant[i] = sp->roundconstant[i+1];
	sp->roundconstant[i+1] = t;
    }
}

/* the following defines the round function */
#define round_function(nn) \
 {  R8(sp); \
    update_roundconstant(sp); \
 }

/* the following defines the last half round */
#define last_half_round(nn) \
 {  last_half_round_R8(sp); \
 }

/*Bijective function E*/
static
void E8(jhParam *sp) 
{
    unsigned char tem[256];
    unsigned char t0,t1,t2,t3;
    unsigned int i;

    /* initial group at the beginning of E_8: */
    /* group the H value into 4-bit elements and store them in A */
    /* t0 is the i-th bit of H */
    /* t1 is the (i+2^d)-th bit of H */
    /* t2 is the (i+2*(2^d))-th bit of H */
    /* t3 is the (i+3*(2^d))-th bit of H */
    for (i = 0; i < 256; i++) {
	t0 = (sp->H[i>>3] >> (7 - (i & 7)) ) & 1;
	t1 = (sp->H[(i+256)>>3] >> (7 - (i & 7)) ) & 1;
	t2 = (sp->H[(i+ 512 )>>3] >> (7 - (i & 7)) ) & 1;
	t3 = (sp->H[(i+ 768 )>>3] >> (7 - (i & 7)) ) & 1;
	tem[i] = (t0 << 3) | (t1 << 2) | (t2 << 1) | (t3 << 0); 
    }

    for (i = 0; i < 128; i++) {
	sp->A[i << 1] = tem[i]; 
	sp->A[(i << 1)+1] = tem[i+128]; 
    } 

#ifdef	DYING
    /* 35 rounds */
    for (i = 0; i < 35; i++) {
	R8(sp);
	update_roundconstant(sp);
    }

    /* the final half round with only Sbox layer */
    last_half_round_R8(sp);
#else
    /* perform 35.5 rounds */
    round_function(00);
    round_function(01);
    round_function(02);
    round_function(03);
    round_function(04);
    round_function(05);
    round_function(06);
    round_function(07);
    round_function(08);
    round_function(09);
    round_function(10);
    round_function(11);
    round_function(12);
    round_function(13);
    round_function(14);
    round_function(15);
    round_function(16);
    round_function(17);
    round_function(18);
    round_function(19);
    round_function(20);
    round_function(21);
    round_function(22);
    round_function(23);
    round_function(24);
    round_function(25);
    round_function(26);
    round_function(27);
    round_function(28);
    round_function(29);
    round_function(30);
    round_function(31);
    round_function(32);
    round_function(33);
    round_function(34);

    last_half_round(35);    
#endif

    /* de-group at the end of E_8: */
    /* decompose the 4-bit elements of A into the 1024-bit H */
    for (i = 0; i < 128; i++)
	sp->H[i] = 0;

    for (i = 0; i < 128; i++) {
	tem[i] = sp->A[i << 1]; 
	tem[i+128] = sp->A[(i << 1)+1];
    } 

    for (i = 0; i < 256; i++) {
	t0 = (tem[i] >> 3) & 1;
	t1 = (tem[i] >> 2) & 1;
	t2 = (tem[i] >> 1) & 1;
	t3 = (tem[i] >> 0) & 1;

	sp->H[i>>3] |= t0 << (7 - (i & 7));
	sp->H[(i + 256)>>3] |= t1 << (7 - (i & 7));
	sp->H[(i + 512)>>3] |= t2 << (7 - (i & 7));
	sp->H[(i + 768)>>3] |= t3 << (7 - (i & 7));
    }
}
#endif	/* defined(OPTIMIZE_SSE2) */

/* compression function F8 */
static
void F8(jhParam *sp) 
{
#if	defined(OPTIMIZE_SSE2)
    __m128i  a0, a1, a2, a3, a4, a5, a6, a7;

    /* load the 512-bit message from the buffer into a0,a1,a2 and a3 */
    LOAD(a0, sp->buffer);
    LOAD(a1, sp->buffer+16);
    LOAD(a2, sp->buffer+32);
    LOAD(a3, sp->buffer+48);

    /* xor the 512-bit message with the fist half of the 1024-bit hash state */
    sp->x0 = XOR(sp->x0, a0);
    sp->x1 = XOR(sp->x1, a1);
    sp->x2 = XOR(sp->x2, a2);
    sp->x3 = XOR(sp->x3, a3);
#else	/* defined(OPTIMIZE_SSE2) */
    unsigned int i;

    /* initialize the round constant */
    for (i = 0; i < 64; i++)
	sp->roundconstant[i] = roundconstant_zero[i];  

    /* xor the message with the first half of H */
    for (i = 0; i < 64; i++)
	sp->H[i] ^= sp->buffer[i];
#endif	/* defined(OPTIMIZE_SSE2) */

    /* Bijective function E8 */
#if	defined(OPTIMIZE_SSE2)
    /* perform 35.5 rounds */
    round_function(00);
    round_function(01);
    round_function(02);
    round_function(03);
    round_function(04);
    round_function(05);
    round_function(06);
    round_function(07);
    round_function(08);
    round_function(09);
    round_function(10);
    round_function(11);
    round_function(12);
    round_function(13);
    round_function(14);
    round_function(15);
    round_function(16);
    round_function(17);
    round_function(18);
    round_function(19);
    round_function(20);
    round_function(21);
    round_function(22);
    round_function(23);
    round_function(24);
    round_function(25);
    round_function(26);
    round_function(27);
    round_function(28);
    round_function(29);
    round_function(30);
    round_function(31);
    round_function(32);
    round_function(33);
    round_function(34);

    last_half_round(35);    
#else	/* defined(OPTIMIZE_SSE2) */
    E8(sp);
#endif	/* defined(OPTIMIZE_SSE2) */

#if	defined(OPTIMIZE_SSE2)
    /* load the 512-bit message into a0,a1,a2 and a3 */ 
    LOAD(a0, sp->buffer);
    LOAD(a1, sp->buffer+16);
    LOAD(a2, sp->buffer+32);
    LOAD(a3, sp->buffer+48);

    /* xor the 512-bit message with the second half of the 1024-bit hash state */
    sp->x4 = XOR(sp->x4, a0);
    sp->x5 = XOR(sp->x5, a1);
    sp->x6 = XOR(sp->x6, a2);
    sp->x7 = XOR(sp->x7, a3);
#else	/* defined(OPTIMIZE_SSE2) */
    /* xor the message with the last half of H */
    for (i = 0; i < 64; i++)
	sp->H[i+64] ^= sp->buffer[i];
#endif	/* defined(OPTIMIZE_SSE2) */
}

int jhInit(jhParam *sp, int hashbitlen) 
{
    const unsigned char * H0;
#if	!defined(OPTIMIZE_SSE2)
    unsigned int i;
#endif

    switch (hashbitlen) {
    case 224:	H0 = JH224_H0;		break;
    case 256:	H0 = JH256_H0;		break;
    case 384:	H0 = JH384_H0;		break;
    case 512:	H0 = JH512_H0;		break;
    default:	return BAD_HASHLEN;	break;
    }

    sp->hashbitlen = hashbitlen;

#if	defined(OPTIMIZE_SSE2)
    LOAD(sp->x0, H0);
    LOAD(sp->x1, H0+16);
    LOAD(sp->x2, H0+32);
    LOAD(sp->x3, H0+48);
    LOAD(sp->x4, H0+64);
    LOAD(sp->x5, H0+80);
    LOAD(sp->x6, H0+96);
    LOAD(sp->x7, H0+112);
#else	/* defined(OPTIMIZE_SSE2) */
    for (i = 0; i < 64; i++)
	sp->buffer[i] = 0;
#ifdef	DYING
    for (i = 0; i < 128; i++)
	sp->H[i] = 0;

    /* initialize the initial hash value of JH */
    sp->H[1] = hashbitlen & 0xff;
    sp->H[0] = (hashbitlen >> 8) & 0xff;
	
    F8(sp);
#else
    memcpy(sp->H, H0, sizeof(sp->H));
#endif
#endif	/* defined(OPTIMIZE_SSE2) */

    return SUCCESS;
}

int
jhReset(jhParam *sp)
{
    return jhInit(sp, sp->hashbitlen);
}

int jhUpdate(jhParam *sp, const byte *data, size_t size) 
{
    uint64_t databitlen = 8 * size;
    uint64_t i;

    sp->databitlen = databitlen;

    /* compress the message blocks except the last partial block */
    for (i = 0; (i+512) <= databitlen; i += 512) {
	memcpy(sp->buffer, data + (i >> 3), 64);
	F8(sp);
    }

    /* storing the partial block into buffer */
    if ( (databitlen & 0x1ff) > 0) {
	for (i = 0; i < 64; i++)
	    sp->buffer[i] = 0;
	if ((databitlen & 7) == 0)
	    memcpy(sp->buffer, data + ((databitlen >> 9) << 6), (databitlen & 0x1ff) >> 3);
	else
	    memcpy(sp->buffer, data + ((databitlen >> 9) << 6), ((databitlen & 0x1ff) >> 3)+1);
    }

    return SUCCESS;
}

/* padding the message, truncate the hash value H and obtain the message digest */
int jhDigest(jhParam *sp, byte *digest) 
{
#if	defined(OPTIMIZE_SSE2)
    unsigned char t[64] __attribute__((aligned (16)));
#else	/* defined(OPTIMIZE_SSE2) */
    unsigned char * t = sp->H;
#endif	/* defined(OPTIMIZE_SSE2) */
    unsigned int i;

    if ((sp->databitlen & 0x1ff) == 0) {
	/* pad the message when databitlen is multiple of 512 bits, then process the padded block */
	for (i = 0; i < 64; i++)
	    sp->buffer[i] = 0;
	sp->buffer[0] = 0x80;
	sp->buffer[63] =  sp->databitlen        & 0xff;
	sp->buffer[62] = (sp->databitlen >>  8) & 0xff;
	sp->buffer[61] = (sp->databitlen >> 16) & 0xff;
	sp->buffer[60] = (sp->databitlen >> 24) & 0xff;
	sp->buffer[59] = (sp->databitlen >> 32) & 0xff;
	sp->buffer[58] = (sp->databitlen >> 40) & 0xff;
	sp->buffer[57] = (sp->databitlen >> 48) & 0xff;
	sp->buffer[56] = (sp->databitlen >> 56) & 0xff;
	F8(sp);
    } else {
	/* pad and process the partial block */
	sp->buffer[(sp->databitlen & 0x1ff) >> 3] |= 1 << (7 - (sp->databitlen & 7));
	F8(sp);
	for (i = 0; i < 64; i++)
	    sp->buffer[i] = 0;
	/* process the last padded block */
	sp->buffer[63] =  sp->databitlen        & 0xff;
	sp->buffer[62] = (sp->databitlen >>  8) & 0xff;
	sp->buffer[61] = (sp->databitlen >> 16) & 0xff;
	sp->buffer[60] = (sp->databitlen >> 24) & 0xff;
	sp->buffer[59] = (sp->databitlen >> 32) & 0xff;
	sp->buffer[58] = (sp->databitlen >> 40) & 0xff;
	sp->buffer[57] = (sp->databitlen >> 48) & 0xff;
	sp->buffer[56] = (sp->databitlen >> 56) & 0xff;
	F8(sp);
    }

    /* truncating the final hash value to generate the message digest */
#if	defined(OPTIMIZE_SSE2)
    STORE(sp->x4, t);
    STORE(sp->x5, t+16);
    STORE(sp->x6, t+32);
    STORE(sp->x7, t+48);
#else	/* defined(OPTIMIZE_SSE2) */
    t += 64;
#endif	/* defined(OPTIMIZE_SSE2) */

    if (sp->hashbitlen == 224) 
	memcpy(digest, t + (64-28), 28);

    if (sp->hashbitlen == 256)
	memcpy(digest, t + (64-32), 32);

    if (sp->hashbitlen == 384) 
	memcpy(digest, t + (64-48), 48);

    if (sp->hashbitlen == 512)
	memcpy(digest, t + (64-64), 64);

    return SUCCESS;
}
