/*
 * Copyright (c) 2004 X-Way Rights BV
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!\file sha512.c
 * \brief SHA-512 hash function, as specified by NIST FIPS 180-2.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_m HASH_sha512_m
 */
 
#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef OPTIMIZE_SSE2
# include <emmintrin.h>
#endif
#if HAVE_ENDIAN_H && HAVE_ASM_BYTEORDER_H
# include <endian.h>
#endif

#include "beecrypt/sha512.h"
#include "beecrypt/sha2k64.h"
#include "beecrypt/endianness.h"

/*!\addtogroup HASH_sha512_m
 * \{
 */

static const uint64_t hinit[8] = {
	#if (SIZEOF_UNSIGNED_LONG == 8) || !HAVE_UNSIGNED_LONG_LONG
	0x6a09e667f3bcc908UL,
	0xbb67ae8584caa73bUL,
	0x3c6ef372fe94f82bUL,
	0xa54ff53a5f1d36f1UL,
	0x510e527fade682d1UL,
	0x9b05688c2b3e6c1fUL,
	0x1f83d9abfb41bd6bUL,
	0x5be0cd19137e2179UL
	#else
	0x6a09e667f3bcc908ULL,
	0xbb67ae8584caa73bULL,
	0x3c6ef372fe94f82bULL,
	0xa54ff53a5f1d36f1ULL,
	0x510e527fade682d1ULL,
	0x9b05688c2b3e6c1fULL,
	0x1f83d9abfb41bd6bULL,
	0x5be0cd19137e2179ULL
	#endif
};

const hashFunction sha512 = {
	.name = "SHA-512",
	.paramsize = sizeof(sha512Param),
	.blocksize = 128,
	.digestsize = 64,
	.reset = (hashFunctionReset) sha512Reset,
	.update = (hashFunctionUpdate) sha512Update,
	.digest = (hashFunctionDigest) sha512Digest
};

int sha512Reset(register sha512Param* sp)
{
	memcpy(sp->h, hinit, 8 * sizeof(uint64_t));
	memset(sp->data, 0, 80 * sizeof(uint64_t));
	#if (MP_WBITS == 64)
	mpzero(2, sp->length);
	#elif (MP_WBITS == 32)
	mpzero(4, sp->length);
	#else
	# error
	#endif
	sp->offset = 0;

	return 0;
}

#ifdef OPTIMIZE_SSE2

# define R(x,s) _mm_srli_si64(x,s)
# define S(x,s) _mm_xor_si64(_mm_srli_si64(x,s),_mm_slli_si64(x,64-(s)))
# define CH(x,y,z) _mm_xor_si64(_m_pand(x,_mm_xor_si64(y,z)),z)
# define MAJ(x,y,z) _m_por(_m_pand(_m_por(x,y),z),_m_pand(x,y))
# define SIG0(x) _mm_xor_si64(_mm_xor_si64(S(x,28),S(x,34)),S(x,39))
# define SIG1(x) _mm_xor_si64(_mm_xor_si64(S(x,14),S(x,18)),S(x,41))
# define sig0(x) _mm_xor_si64(_mm_xor_si64(S(x,1),S(x,8)),R(x,7))
# define sig1(x) _mm_xor_si64(_mm_xor_si64(S(x,19),S(x,61)),R(x,6))

# define ROUND(a,b,c,d,e,f,g,h,w,k) \
	temp = _mm_add_si64(h, _mm_add_si64(_mm_add_si64(SIG1(e), CH(e,f,g)), _mm_add_si64(k, w))); \
	h = _mm_add_si64(temp, _mm_add_si64(SIG0(a), MAJ(a,b,c))); \
	d = _mm_add_si64(d, temp)

#else

# define R(x,s) ((x) >> (s))
# define S(x,s) ROTR64(x, s)
# define CH(x,y,z) ((x&(y^z))^z)
# define MAJ(x,y,z) (((x|y)&z)|(x&y))
# define SIG0(x) (S(x,28) ^ S(x,34) ^ S(x,39))
# define SIG1(x) (S(x,14) ^ S(x,18) ^ S(x,41))
# define sig0(x) (S(x,1) ^ S(x,8) ^ R(x,7))
# define sig1(x) (S(x,19) ^ S(x,61) ^ R(x,6))

# define ROUND(a,b,c,d,e,f,g,h,w,k)	\
	temp = h + SIG1(e) + CH(e,f,g) + k + w;	\
	h = temp + SIG0(a) + MAJ(a,b,c);	\
	d += temp

#endif

#ifndef ASM_SHA512PROCESS
void sha512Process(register sha512Param* sp)
{
	#ifdef OPTIMIZE_SSE2
	# if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	static const __m64 MASK = { 0x00FF00FF00FF00FF };
	# elif defined(__GNUC__)
	static const __m64 MASK = { 0x00FF00FF, 0x00FF00FF };
	# else
	#  error
	# endif

	__m64 a, b, c, d, e, f, g, h, temp;
	register       __m64 *w;
	register const __m64 *k;
	register byte t;

	w = (__m64*) sp->data;
	t = 16;
	while (t--)
	{
		temp = *w;
		*(w++) = _mm_xor_si64(
				_mm_slli_si64(_m_pshufw(_m_pand(temp, MASK), 27), 8),
				_m_pshufw(_m_pand(_mm_srli_si64(temp, 8), MASK), 27)
			);
	}

	t = 64;
	while (t--)
	{
		temp = _mm_add_si64(_mm_add_si64(sig1(w[-2]), w[-7]), _mm_add_si64(sig0(w[-15]), w[-16]));
		*(w++) = temp;
	}

	w = (__m64*) sp->h;

	a = w[0]; b = w[1]; c = w[2]; d = w[3];
	e = w[4]; f = w[5]; g = w[6]; h = w[7];

	w = (__m64*) sp->data;
	k = (__m64*) SHA2_64BIT_K;

	#else

	register uint64_t a, b, c, d, e, f, g, h, temp;
	register       uint64_t *w;
	register const uint64_t *k;
	register byte t;

	# if WORDS_BIGENDIAN
	w = sp->data + 16;
	# else
	w = sp->data;
	t = 16;
	while (t--)
	{
		temp = swapu64(*w);
		*(w++) = temp;
	}
	# endif

	t = 64;
	while (t--)
	{
		temp = sig1(w[-2]) + w[-7] + sig0(w[-15]) + w[-16];
		*(w++) = temp;
	}

	w = sp->data;

	a = sp->h[0]; b = sp->h[1]; c = sp->h[2]; d = sp->h[3];
	e = sp->h[4]; f = sp->h[5]; g = sp->h[6]; h = sp->h[7];

	k = SHA2_64BIT_K;
	#endif

	ROUND(a,b,c,d,e,f,g,h,w[ 0],k[ 0]);
	ROUND(h,a,b,c,d,e,f,g,w[ 1],k[ 1]);
	ROUND(g,h,a,b,c,d,e,f,w[ 2],k[ 2]);
	ROUND(f,g,h,a,b,c,d,e,w[ 3],k[ 3]);
	ROUND(e,f,g,h,a,b,c,d,w[ 4],k[ 4]);
	ROUND(d,e,f,g,h,a,b,c,w[ 5],k[ 5]);
	ROUND(c,d,e,f,g,h,a,b,w[ 6],k[ 6]);
	ROUND(b,c,d,e,f,g,h,a,w[ 7],k[ 7]);
	ROUND(a,b,c,d,e,f,g,h,w[ 8],k[ 8]);
	ROUND(h,a,b,c,d,e,f,g,w[ 9],k[ 9]);
	ROUND(g,h,a,b,c,d,e,f,w[10],k[10]);
	ROUND(f,g,h,a,b,c,d,e,w[11],k[11]);
	ROUND(e,f,g,h,a,b,c,d,w[12],k[12]);
	ROUND(d,e,f,g,h,a,b,c,w[13],k[13]);
	ROUND(c,d,e,f,g,h,a,b,w[14],k[14]);
	ROUND(b,c,d,e,f,g,h,a,w[15],k[15]);
	ROUND(a,b,c,d,e,f,g,h,w[16],k[16]);
	ROUND(h,a,b,c,d,e,f,g,w[17],k[17]);
	ROUND(g,h,a,b,c,d,e,f,w[18],k[18]);
	ROUND(f,g,h,a,b,c,d,e,w[19],k[19]);
	ROUND(e,f,g,h,a,b,c,d,w[20],k[20]);
	ROUND(d,e,f,g,h,a,b,c,w[21],k[21]);
	ROUND(c,d,e,f,g,h,a,b,w[22],k[22]);
	ROUND(b,c,d,e,f,g,h,a,w[23],k[23]);
	ROUND(a,b,c,d,e,f,g,h,w[24],k[24]);
	ROUND(h,a,b,c,d,e,f,g,w[25],k[25]);
	ROUND(g,h,a,b,c,d,e,f,w[26],k[26]);
	ROUND(f,g,h,a,b,c,d,e,w[27],k[27]);
	ROUND(e,f,g,h,a,b,c,d,w[28],k[28]);
	ROUND(d,e,f,g,h,a,b,c,w[29],k[29]);
	ROUND(c,d,e,f,g,h,a,b,w[30],k[30]);
	ROUND(b,c,d,e,f,g,h,a,w[31],k[31]);
	ROUND(a,b,c,d,e,f,g,h,w[32],k[32]);
	ROUND(h,a,b,c,d,e,f,g,w[33],k[33]);
	ROUND(g,h,a,b,c,d,e,f,w[34],k[34]);
	ROUND(f,g,h,a,b,c,d,e,w[35],k[35]);
	ROUND(e,f,g,h,a,b,c,d,w[36],k[36]);
	ROUND(d,e,f,g,h,a,b,c,w[37],k[37]);
	ROUND(c,d,e,f,g,h,a,b,w[38],k[38]);
	ROUND(b,c,d,e,f,g,h,a,w[39],k[39]);
	ROUND(a,b,c,d,e,f,g,h,w[40],k[40]);
	ROUND(h,a,b,c,d,e,f,g,w[41],k[41]);
	ROUND(g,h,a,b,c,d,e,f,w[42],k[42]);
	ROUND(f,g,h,a,b,c,d,e,w[43],k[43]);
	ROUND(e,f,g,h,a,b,c,d,w[44],k[44]);
	ROUND(d,e,f,g,h,a,b,c,w[45],k[45]);
	ROUND(c,d,e,f,g,h,a,b,w[46],k[46]);
	ROUND(b,c,d,e,f,g,h,a,w[47],k[47]);
	ROUND(a,b,c,d,e,f,g,h,w[48],k[48]);
	ROUND(h,a,b,c,d,e,f,g,w[49],k[49]);
	ROUND(g,h,a,b,c,d,e,f,w[50],k[50]);
	ROUND(f,g,h,a,b,c,d,e,w[51],k[51]);
	ROUND(e,f,g,h,a,b,c,d,w[52],k[52]);
	ROUND(d,e,f,g,h,a,b,c,w[53],k[53]);
	ROUND(c,d,e,f,g,h,a,b,w[54],k[54]);
	ROUND(b,c,d,e,f,g,h,a,w[55],k[55]);
	ROUND(a,b,c,d,e,f,g,h,w[56],k[56]);
	ROUND(h,a,b,c,d,e,f,g,w[57],k[57]);
	ROUND(g,h,a,b,c,d,e,f,w[58],k[58]);
	ROUND(f,g,h,a,b,c,d,e,w[59],k[59]);
	ROUND(e,f,g,h,a,b,c,d,w[60],k[60]);
	ROUND(d,e,f,g,h,a,b,c,w[61],k[61]);
	ROUND(c,d,e,f,g,h,a,b,w[62],k[62]);
	ROUND(b,c,d,e,f,g,h,a,w[63],k[63]);
	ROUND(a,b,c,d,e,f,g,h,w[64],k[64]);
	ROUND(h,a,b,c,d,e,f,g,w[65],k[65]);
	ROUND(g,h,a,b,c,d,e,f,w[66],k[66]);
	ROUND(f,g,h,a,b,c,d,e,w[67],k[67]);
	ROUND(e,f,g,h,a,b,c,d,w[68],k[68]);
	ROUND(d,e,f,g,h,a,b,c,w[69],k[69]);
	ROUND(c,d,e,f,g,h,a,b,w[70],k[70]);
	ROUND(b,c,d,e,f,g,h,a,w[71],k[71]);
	ROUND(a,b,c,d,e,f,g,h,w[72],k[72]);
	ROUND(h,a,b,c,d,e,f,g,w[73],k[73]);
	ROUND(g,h,a,b,c,d,e,f,w[74],k[74]);
	ROUND(f,g,h,a,b,c,d,e,w[75],k[75]);
	ROUND(e,f,g,h,a,b,c,d,w[76],k[76]);
	ROUND(d,e,f,g,h,a,b,c,w[77],k[77]);
	ROUND(c,d,e,f,g,h,a,b,w[78],k[78]);
	ROUND(b,c,d,e,f,g,h,a,w[79],k[79]);

	#ifdef OPTIMIZE_SSE2
	w = (__m64*) sp->h;
	w[0] = _mm_add_si64(w[0], a);
	w[1] = _mm_add_si64(w[1], b);
	w[2] = _mm_add_si64(w[2], c);
	w[3] = _mm_add_si64(w[3], d);
	w[4] = _mm_add_si64(w[4], e);
	w[5] = _mm_add_si64(w[5], f);
	w[6] = _mm_add_si64(w[6], g);
	w[7] = _mm_add_si64(w[7], h);
	_mm_empty();
	#else
	sp->h[0] += a;
	sp->h[1] += b;
	sp->h[2] += c;
	sp->h[3] += d;
	sp->h[4] += e;
	sp->h[5] += f;
	sp->h[6] += g;
	sp->h[7] += h;
	#endif
}
#endif

int sha512Update(register sha512Param* sp, const byte* data, size_t size)
{
	register size_t proclength;

	#if (MP_WBITS == 64)
	mpw add[2];
	mpsetw(2, add, size);
	mplshift(2, add, 3);
	mpadd(2, sp->length, add);
	#elif (MP_WBITS == 32)
	mpw add[4];
	mpsetws(4, add, size);
	mplshift(4, add, 3);
	mpadd(4, sp->length, add);
	#else
	# error
	#endif

	while (size > 0)
	{
		proclength = ((sp->offset + size) > 128U) ? (128U - sp->offset) : size;
		memcpy(((byte *) sp->data) + sp->offset, data, proclength);
		size -= proclength;
		data += proclength;
		sp->offset += proclength;

		if (sp->offset == 128U)
		{
			sha512Process(sp);
			sp->offset = 0;
		}
	}
	return 0;
}

static void sha512Finish(register sha512Param* sp)
{
	register byte *ptr = ((byte *) sp->data) + sp->offset++;

	*(ptr++) = 0x80;

	if (sp->offset > 112)
	{
		while (sp->offset++ < 128)
			*(ptr++) = 0;

		sha512Process(sp);
		sp->offset = 0;
	}

	ptr = ((byte *) sp->data) + sp->offset;
	while (sp->offset++ < 112)
		*(ptr++) = 0;

	#if (MP_WBITS == 64)
	ptr[ 0] = (byte)(sp->length[0] >> 56);
	ptr[ 1] = (byte)(sp->length[0] >> 48);
	ptr[ 2] = (byte)(sp->length[0] >> 40);
	ptr[ 3] = (byte)(sp->length[0] >> 32);
	ptr[ 4] = (byte)(sp->length[0] >> 24);
	ptr[ 5] = (byte)(sp->length[0] >> 16);
	ptr[ 6] = (byte)(sp->length[0] >>  8);
	ptr[ 7] = (byte)(sp->length[0]      );
	ptr[ 8] = (byte)(sp->length[1] >> 56);
	ptr[ 9] = (byte)(sp->length[1] >> 48);
	ptr[10] = (byte)(sp->length[1] >> 40);
	ptr[11] = (byte)(sp->length[1] >> 32);
	ptr[12] = (byte)(sp->length[1] >> 24);
	ptr[13] = (byte)(sp->length[1] >> 16);
	ptr[14] = (byte)(sp->length[1] >>  8);
	ptr[15] = (byte)(sp->length[1]      );
	#elif (MP_WBITS == 32)
	ptr[ 0] = (byte)(sp->length[0] >> 24);
	ptr[ 1] = (byte)(sp->length[0] >> 16);
	ptr[ 2] = (byte)(sp->length[0] >>  8);
	ptr[ 3] = (byte)(sp->length[0]      );
	ptr[ 4] = (byte)(sp->length[1] >> 24);
	ptr[ 5] = (byte)(sp->length[1] >> 16);
	ptr[ 6] = (byte)(sp->length[1] >>  8);
	ptr[ 7] = (byte)(sp->length[1]      );
	ptr[ 8] = (byte)(sp->length[2] >> 24);
	ptr[ 9] = (byte)(sp->length[2] >> 16);
	ptr[10] = (byte)(sp->length[2] >>  8);
	ptr[11] = (byte)(sp->length[2]      );
	ptr[12] = (byte)(sp->length[3] >> 24);
	ptr[13] = (byte)(sp->length[3] >> 16);
	ptr[14] = (byte)(sp->length[3] >>  8);
	ptr[15] = (byte)(sp->length[3]      );
	#else
	# error
	#endif

	sha512Process(sp);
	sp->offset = 0;
}

int sha512Digest(register sha512Param* sp, byte* data)
{
	sha512Finish(sp);

	/* encode 8 integers big-endian style */
	data[ 0] = (byte)(sp->h[0] >> 56);
	data[ 1] = (byte)(sp->h[0] >> 48);
	data[ 2] = (byte)(sp->h[0] >> 40);
	data[ 3] = (byte)(sp->h[0] >> 32);
	data[ 4] = (byte)(sp->h[0] >> 24);
	data[ 5] = (byte)(sp->h[0] >> 16);
	data[ 6] = (byte)(sp->h[0] >>  8);
	data[ 7] = (byte)(sp->h[0] >>  0);

	data[ 8] = (byte)(sp->h[1] >> 56);
	data[ 9] = (byte)(sp->h[1] >> 48);
	data[10] = (byte)(sp->h[1] >> 40);
	data[11] = (byte)(sp->h[1] >> 32);
	data[12] = (byte)(sp->h[1] >> 24);
	data[13] = (byte)(sp->h[1] >> 16);
	data[14] = (byte)(sp->h[1] >>  8);
	data[15] = (byte)(sp->h[1] >>  0);

	data[16] = (byte)(sp->h[2] >> 56);
	data[17] = (byte)(sp->h[2] >> 48);
	data[18] = (byte)(sp->h[2] >> 40);
	data[19] = (byte)(sp->h[2] >> 32);
	data[20] = (byte)(sp->h[2] >> 24);
	data[21] = (byte)(sp->h[2] >> 16);
	data[22] = (byte)(sp->h[2] >>  8);
	data[23] = (byte)(sp->h[2] >>  0);

	data[24] = (byte)(sp->h[3] >> 56);
	data[25] = (byte)(sp->h[3] >> 48);
	data[26] = (byte)(sp->h[3] >> 40);
	data[27] = (byte)(sp->h[3] >> 32);
	data[28] = (byte)(sp->h[3] >> 24);
	data[29] = (byte)(sp->h[3] >> 16);
	data[30] = (byte)(sp->h[3] >>  8);
	data[31] = (byte)(sp->h[3] >>  0);

	data[32] = (byte)(sp->h[4] >> 56);
	data[33] = (byte)(sp->h[4] >> 48);
	data[34] = (byte)(sp->h[4] >> 40);
	data[35] = (byte)(sp->h[4] >> 32);
	data[36] = (byte)(sp->h[4] >> 24);
	data[37] = (byte)(sp->h[4] >> 16);
	data[38] = (byte)(sp->h[4] >>  8);
	data[39] = (byte)(sp->h[4] >>  0);

	data[40] = (byte)(sp->h[5] >> 56);
	data[41] = (byte)(sp->h[5] >> 48);
	data[42] = (byte)(sp->h[5] >> 40);
	data[43] = (byte)(sp->h[5] >> 32);
	data[44] = (byte)(sp->h[5] >> 24);
	data[45] = (byte)(sp->h[5] >> 16);
	data[46] = (byte)(sp->h[5] >>  8);
	data[47] = (byte)(sp->h[5] >>  0);

	data[48] = (byte)(sp->h[6] >> 56);
	data[49] = (byte)(sp->h[6] >> 48);
	data[50] = (byte)(sp->h[6] >> 40);
	data[51] = (byte)(sp->h[6] >> 32);
	data[52] = (byte)(sp->h[6] >> 24);
	data[53] = (byte)(sp->h[6] >> 16);
	data[54] = (byte)(sp->h[6] >>  8);
	data[55] = (byte)(sp->h[6] >>  0);

	data[56] = (byte)(sp->h[7] >> 56);
	data[57] = (byte)(sp->h[7] >> 48);
	data[58] = (byte)(sp->h[7] >> 40);
	data[59] = (byte)(sp->h[7] >> 32);
	data[60] = (byte)(sp->h[7] >> 24);
	data[61] = (byte)(sp->h[7] >> 16);
	data[62] = (byte)(sp->h[7] >>  8);
	data[63] = (byte)(sp->h[7] >>  0);

	sha512Reset(sp);
	return 0;
}

/*!\}
 */
