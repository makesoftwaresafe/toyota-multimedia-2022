/*
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

/*!\file ripemd160.c
 * \brief RIPEMD-160 hash function.
 * \author Jeff Johnson <jbj@rpm5.org>
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_m HASH_ripemd160_m
 */
 
#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/ripemd160.h"
#include "beecrypt/endianness.h"

/*!\addtogroup HASH_ripemd160_m
 * \{
 */

/*@unchecked@*/ /*@observer@*/
static uint32_t ripemd160hinit[5] = {
	0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U
};

/*@-sizeoftype@*/
/*@unchecked@*/ /*@observer@*/
const hashFunction ripemd160 = {
	.name = "RIPEMD-160",
	.paramsize = sizeof(ripemd160Param),
	.blocksize = 64,
	.digestsize = 20,
	.reset = (hashFunctionReset) ripemd160Reset,
	.update = (hashFunctionUpdate) ripemd160Update,
	.digest = (hashFunctionDigest) ripemd160Digest
};
/*@=sizeoftype@*/

int ripemd160Reset(register ripemd160Param* mp)
{
/*@-sizeoftype@*/
        memcpy(mp->h, ripemd160hinit, 5 * sizeof(uint32_t));
        memset(mp->data, 0, 16 * sizeof(uint32_t));
/*@=sizeoftype@*/
        #if (MP_WBITS == 64)
        mpzero(1, mp->length);
        #elif (MP_WBITS == 32)
        mpzero(2, mp->length);
        #else
        # error
        #endif
        mp->offset = 0;
        return 0;
}

#define LSR1(a, b, c, d, e, x, s) \
	a = ROTL32((b^c^d) + a + x, s) + e;\
	c = ROTL32(c, 10);
#define LSR2(a, b, c, d, e, x, s) \
	a = ROTL32(((b&c)|(~b&d)) + a + x + 0x5a827999U, s) + e;\
	c = ROTL32(c, 10);
#define LSR3(a, b, c, d, e, x, s) \
	a = ROTL32(((b|~c)^d) + a + x + 0x6ed9eba1U, s) + e; \
	c = ROTL32(c, 10);
#define LSR4(a, b, c, d, e, x, s) \
	a = ROTL32(((b&d)|(c&~d)) + a + x + 0x8f1bbcdcU, s) + e; \
	c = ROTL32(c, 10);
#define LSR5(a, b, c, d, e, x, s) \
	a = ROTL32((b^(c|~d)) + a + x + 0xa953fd4eU, s) + e; \
	c = ROTL32(c, 10);

#define RSR5(a, b, c, d, e, x, s) \
	a = ROTL32((b^c^d) + a + x, s) + e;\
	c = ROTL32(c, 10);
#define RSR4(a, b, c, d, e, x, s) \
	a = ROTL32(((b&c)|(~b&d)) + a + x + 0x7a6d76e9U, s) + e;\
	c = ROTL32(c, 10);
#define RSR3(a, b, c, d, e, x, s) \
	a = ROTL32(((b|~c)^d) + a + x + 0x6d703ef3U, s) + e; \
	c = ROTL32(c, 10);
#define RSR2(a, b, c, d, e, x, s) \
	a = ROTL32(((b&d)|(c&~d)) + a + x + 0x5c4dd124U, s) + e; \
	c = ROTL32(c, 10);
#define RSR1(a, b, c, d, e, x, s) \
	a = ROTL32((b^(c|~d)) + a + x + 0x50a28be6U, s) + e; \
	c = ROTL32(c, 10);

#ifndef ASM_RIPEMD160PROCESS
void ripemd160Process(ripemd160Param* mp)
{
	register uint32_t la, lb, lc, ld, le;
	register uint32_t ra, rb, rc, rd, re;
	register uint32_t* x;
        #ifdef WORDS_BIGENDIAN
        register byte t;
        #endif

        x = mp->data;
        #ifdef WORDS_BIGENDIAN
        t = 16;
        while (t--)
        {
                register uint32_t temp = swapu32(*x);
                *(x++) = temp;
        }
        x = mp->data;
        #endif

        la = mp->h[0]; lb = mp->h[1]; lc = mp->h[2]; ld = mp->h[3]; le = mp->h[4];
        ra = mp->h[0]; rb = mp->h[1]; rc = mp->h[2]; rd = mp->h[3]; re = mp->h[4];

	/* In theory OpenMP would allows us to do the 'left' and 'right' sections in parallel,
	 * but in practice the overhead make the code much slower
	 */

	/* left round 1 */
	LSR1(la, lb, lc, ld, le, x[ 0], 11);
	LSR1(le, la, lb, lc, ld, x[ 1], 14);
	LSR1(ld, le, la, lb, lc, x[ 2], 15);
	LSR1(lc, ld, le, la, lb, x[ 3], 12);
	LSR1(lb, lc, ld, le, la, x[ 4],  5);
	LSR1(la, lb, lc, ld, le, x[ 5],  8);
	LSR1(le, la, lb, lc, ld, x[ 6],  7);
	LSR1(ld, le, la, lb, lc, x[ 7],  9);
	LSR1(lc, ld, le, la, lb, x[ 8], 11);
	LSR1(lb, lc, ld, le, la, x[ 9], 13);
	LSR1(la, lb, lc, ld, le, x[10], 14);
	LSR1(le, la, lb, lc, ld, x[11], 15);
	LSR1(ld, le, la, lb, lc, x[12],  6);
	LSR1(lc, ld, le, la, lb, x[13],  7);
	LSR1(lb, lc, ld, le, la, x[14],  9);
	LSR1(la, lb, lc, ld, le, x[15],  8);

	/* left round 2 */
	LSR2(le, la, lb, lc, ld, x[ 7],  7);
	LSR2(ld, le, la, lb, lc, x[ 4],  6);
	LSR2(lc, ld, le, la, lb, x[13],  8);
	LSR2(lb, lc, ld, le, la, x[ 1], 13);
	LSR2(la, lb, lc, ld, le, x[10], 11);
	LSR2(le, la, lb, lc, ld, x[ 6],  9);
	LSR2(ld, le, la, lb, lc, x[15],  7);
	LSR2(lc, ld, le, la, lb, x[ 3], 15);
	LSR2(lb, lc, ld, le, la, x[12],  7);
	LSR2(la, lb, lc, ld, le, x[ 0], 12);
	LSR2(le, la, lb, lc, ld, x[ 9], 15);
	LSR2(ld, le, la, lb, lc, x[ 5],  9);
	LSR2(lc, ld, le, la, lb, x[ 2], 11);
	LSR2(lb, lc, ld, le, la, x[14],  7);
	LSR2(la, lb, lc, ld, le, x[11], 13);
	LSR2(le, la, lb, lc, ld, x[ 8], 12);

	/* left round 3 */
	LSR3(ld, le, la, lb, lc, x[ 3], 11);
	LSR3(lc, ld, le, la, lb, x[10], 13);
	LSR3(lb, lc, ld, le, la, x[14],  6);
	LSR3(la, lb, lc, ld, le, x[ 4],  7);
	LSR3(le, la, lb, lc, ld, x[ 9], 14);
	LSR3(ld, le, la, lb, lc, x[15],  9);
	LSR3(lc, ld, le, la, lb, x[ 8], 13);
	LSR3(lb, lc, ld, le, la, x[ 1], 15);
	LSR3(la, lb, lc, ld, le, x[ 2], 14);
	LSR3(le, la, lb, lc, ld, x[ 7],  8);
	LSR3(ld, le, la, lb, lc, x[ 0], 13);
	LSR3(lc, ld, le, la, lb, x[ 6],  6);
	LSR3(lb, lc, ld, le, la, x[13],  5);
	LSR3(la, lb, lc, ld, le, x[11], 12);
	LSR3(le, la, lb, lc, ld, x[ 5],  7);
	LSR3(ld, le, la, lb, lc, x[12],  5);

	/* left round 4 */
	LSR4(lc, ld, le, la, lb, x[ 1], 11);
	LSR4(lb, lc, ld, le, la, x[ 9], 12);
	LSR4(la, lb, lc, ld, le, x[11], 14);
	LSR4(le, la, lb, lc, ld, x[10], 15);
	LSR4(ld, le, la, lb, lc, x[ 0], 14);
	LSR4(lc, ld, le, la, lb, x[ 8], 15);
	LSR4(lb, lc, ld, le, la, x[12],  9);
	LSR4(la, lb, lc, ld, le, x[ 4],  8);
	LSR4(le, la, lb, lc, ld, x[13],  9);
	LSR4(ld, le, la, lb, lc, x[ 3], 14);
	LSR4(lc, ld, le, la, lb, x[ 7],  5);
	LSR4(lb, lc, ld, le, la, x[15],  6);
	LSR4(la, lb, lc, ld, le, x[14],  8);
	LSR4(le, la, lb, lc, ld, x[ 5],  6);
	LSR4(ld, le, la, lb, lc, x[ 6],  5);
	LSR4(lc, ld, le, la, lb, x[ 2], 12);

	/* left round 5 */
	LSR5(lb, lc, ld, le, la, x[ 4],  9);
	LSR5(la, lb, lc, ld, le, x[ 0], 15);
	LSR5(le, la, lb, lc, ld, x[ 5],  5);
	LSR5(ld, le, la, lb, lc, x[ 9], 11);
	LSR5(lc, ld, le, la, lb, x[ 7],  6);
	LSR5(lb, lc, ld, le, la, x[12],  8);
	LSR5(la, lb, lc, ld, le, x[ 2], 13);
	LSR5(le, la, lb, lc, ld, x[10], 12);
	LSR5(ld, le, la, lb, lc, x[14],  5);
	LSR5(lc, ld, le, la, lb, x[ 1], 12);
	LSR5(lb, lc, ld, le, la, x[ 3], 13);
	LSR5(la, lb, lc, ld, le, x[ 8], 14);
	LSR5(le, la, lb, lc, ld, x[11], 11);
	LSR5(ld, le, la, lb, lc, x[ 6],  8);
	LSR5(lc, ld, le, la, lb, x[15],  5);
	LSR5(lb, lc, ld, le, la, x[13],  6);

	/* right round 1 */
	RSR1(ra, rb, rc, rd, re, x[ 5],  8);
	RSR1(re, ra, rb, rc, rd, x[14],  9);
	RSR1(rd, re, ra, rb, rc, x[ 7],  9);
	RSR1(rc, rd, re, ra, rb, x[ 0], 11);
	RSR1(rb, rc, rd, re, ra, x[ 9], 13);
	RSR1(ra, rb, rc, rd, re, x[ 2], 15);
	RSR1(re, ra, rb, rc, rd, x[11], 15);
	RSR1(rd, re, ra, rb, rc, x[ 4],  5);
	RSR1(rc, rd, re, ra, rb, x[13],  7);
	RSR1(rb, rc, rd, re, ra, x[ 6],  7);
	RSR1(ra, rb, rc, rd, re, x[15],  8);
	RSR1(re, ra, rb, rc, rd, x[ 8], 11);
	RSR1(rd, re, ra, rb, rc, x[ 1], 14);
	RSR1(rc, rd, re, ra, rb, x[10], 14);
	RSR1(rb, rc, rd, re, ra, x[ 3], 12);
	RSR1(ra, rb, rc, rd, re, x[12],  6);

	/* right round 2 */
	RSR2(re, ra, rb, rc, rd, x[ 6],  9);
	RSR2(rd, re, ra, rb, rc, x[11], 13);
	RSR2(rc, rd, re, ra, rb, x[ 3], 15);
	RSR2(rb, rc, rd, re, ra, x[ 7],  7);
	RSR2(ra, rb, rc, rd, re, x[ 0], 12);
	RSR2(re, ra, rb, rc, rd, x[13],  8);
	RSR2(rd, re, ra, rb, rc, x[ 5],  9);
	RSR2(rc, rd, re, ra, rb, x[10], 11);
	RSR2(rb, rc, rd, re, ra, x[14],  7);
	RSR2(ra, rb, rc, rd, re, x[15],  7);
	RSR2(re, ra, rb, rc, rd, x[ 8], 12);
	RSR2(rd, re, ra, rb, rc, x[12],  7);
	RSR2(rc, rd, re, ra, rb, x[ 4],  6);
	RSR2(rb, rc, rd, re, ra, x[ 9], 15);
	RSR2(ra, rb, rc, rd, re, x[ 1], 13);
	RSR2(re, ra, rb, rc, rd, x[ 2], 11);

	/* right round 3 */
	RSR3(rd, re, ra, rb, rc, x[15],  9);
	RSR3(rc, rd, re, ra, rb, x[ 5],  7);
	RSR3(rb, rc, rd, re, ra, x[ 1], 15);
	RSR3(ra, rb, rc, rd, re, x[ 3], 11);
	RSR3(re, ra, rb, rc, rd, x[ 7],  8);
	RSR3(rd, re, ra, rb, rc, x[14],  6);
	RSR3(rc, rd, re, ra, rb, x[ 6],  6);
	RSR3(rb, rc, rd, re, ra, x[ 9], 14);
	RSR3(ra, rb, rc, rd, re, x[11], 12);
	RSR3(re, ra, rb, rc, rd, x[ 8], 13);
	RSR3(rd, re, ra, rb, rc, x[12],  5);
	RSR3(rc, rd, re, ra, rb, x[ 2], 14);
	RSR3(rb, rc, rd, re, ra, x[10], 13);
	RSR3(ra, rb, rc, rd, re, x[ 0], 13);
	RSR3(re, ra, rb, rc, rd, x[ 4],  7);
	RSR3(rd, re, ra, rb, rc, x[13],  5);

	/* right round 4 */
	RSR4(rc, rd, re, ra, rb, x[ 8], 15);
	RSR4(rb, rc, rd, re, ra, x[ 6],  5);
	RSR4(ra, rb, rc, rd, re, x[ 4],  8);
	RSR4(re, ra, rb, rc, rd, x[ 1], 11);
	RSR4(rd, re, ra, rb, rc, x[ 3], 14);
	RSR4(rc, rd, re, ra, rb, x[11], 14);
	RSR4(rb, rc, rd, re, ra, x[15],  6);
	RSR4(ra, rb, rc, rd, re, x[ 0], 14);
	RSR4(re, ra, rb, rc, rd, x[ 5],  6);
	RSR4(rd, re, ra, rb, rc, x[12],  9);
	RSR4(rc, rd, re, ra, rb, x[ 2], 12);
	RSR4(rb, rc, rd, re, ra, x[13],  9);
	RSR4(ra, rb, rc, rd, re, x[ 9], 12);
	RSR4(re, ra, rb, rc, rd, x[ 7],  5);
	RSR4(rd, re, ra, rb, rc, x[10], 15);
	RSR4(rc, rd, re, ra, rb, x[14],  8);

	/* right round 5 */
	RSR5(rb, rc, rd, re, ra, x[12] ,  8);
	RSR5(ra, rb, rc, rd, re, x[15] ,  5);
	RSR5(re, ra, rb, rc, rd, x[10] , 12);
	RSR5(rd, re, ra, rb, rc, x[ 4] ,  9);
	RSR5(rc, rd, re, ra, rb, x[ 1] , 12);
	RSR5(rb, rc, rd, re, ra, x[ 5] ,  5);
	RSR5(ra, rb, rc, rd, re, x[ 8] , 14);
	RSR5(re, ra, rb, rc, rd, x[ 7] ,  6);
	RSR5(rd, re, ra, rb, rc, x[ 6] ,  8);
	RSR5(rc, rd, re, ra, rb, x[ 2] , 13);
	RSR5(rb, rc, rd, re, ra, x[13] ,  6);
	RSR5(ra, rb, rc, rd, re, x[14] ,  5);
	RSR5(re, ra, rb, rc, rd, x[ 0] , 15);
	RSR5(rd, re, ra, rb, rc, x[ 3] , 13);
	RSR5(rc, rd, re, ra, rb, x[ 9] , 11);
	RSR5(rb, rc, rd, re, ra, x[11] , 11);

        /* combine results */
        rd += lc + mp->h[1];
        mp->h[1] = mp->h[2] + ld + re;
        mp->h[2] = mp->h[3] + le + ra;
        mp->h[3] = mp->h[4] + la + rb;
        mp->h[4] = mp->h[0] + lb + rc;
        mp->h[0] = rd;
}
#endif

int ripemd160Update(ripemd160Param* mp, const byte* data, size_t size)
{
        register uint32_t proclength;

        #if (MP_WBITS == 64)
        mpw add[1];
        mpsetw(1, add, size);
        mplshift(1, add, 3);
        mpadd(1, mp->length, add);
        #elif (MP_WBITS == 32)
        mpw add[2];
        mpsetw(2, add, size);
        mplshift(2, add, 3);
        (void) mpadd(2, mp->length, add);
        #else
        # error
        #endif

        while (size > 0)
        {
                proclength = ((mp->offset + size) > 64U) ? (64U - mp->offset) : size;
/*@-mayaliasunique@*/
                memcpy(((byte *) mp->data) + mp->offset, data, proclength);
/*@=mayaliasunique@*/
                size -= proclength;
                data += proclength;
                mp->offset += proclength;

                if (mp->offset == 64U)
                {
                        ripemd160Process(mp);
                        mp->offset = 0;
                }
        }
        return 0;
}

static void ripemd160Finish(ripemd160Param* mp)
        /*@modifies mp @*/
{
        register byte *ptr = ((byte *) mp->data) + mp->offset++;

        *(ptr++) = 0x80;

        if (mp->offset > 56)
        {
                while (mp->offset++ < 64)
                        *(ptr++) = 0;

                ripemd160Process(mp);
                mp->offset = 0;
        }

        ptr = ((byte *) mp->data) + mp->offset;
        while (mp->offset++ < 56)
                *(ptr++) = 0;

        #if (MP_WBITS == 64)
        ptr[0] = (byte)(mp->length[0]      );
        ptr[1] = (byte)(mp->length[0] >>  8);
        ptr[2] = (byte)(mp->length[0] >> 16);
        ptr[3] = (byte)(mp->length[0] >> 24);
        ptr[4] = (byte)(mp->length[0] >> 32);
        ptr[5] = (byte)(mp->length[0] >> 40);
        ptr[6] = (byte)(mp->length[0] >> 48);
        ptr[7] = (byte)(mp->length[0] >> 56);
        #elif (MP_WBITS == 32)
        ptr[0] = (byte)(mp->length[1]      );
        ptr[1] = (byte)(mp->length[1] >>  8);
        ptr[2] = (byte)(mp->length[1] >> 16);
        ptr[3] = (byte)(mp->length[1] >> 24);
        ptr[4] = (byte)(mp->length[0]      );
        ptr[5] = (byte)(mp->length[0] >>  8);
        ptr[6] = (byte)(mp->length[0] >> 16);
        ptr[7] = (byte)(mp->length[0] >> 24);
        #else
        # error
        #endif

        ripemd160Process(mp);

        mp->offset = 0;
}

/*@-protoparammatch@*/
int ripemd160Digest(ripemd160Param* mp, byte* data)
{
        ripemd160Finish(mp);

        /* encode 5 integers little-endian style */
        data[ 0] = (byte)(mp->h[0]      );
        data[ 1] = (byte)(mp->h[0] >>  8);
        data[ 2] = (byte)(mp->h[0] >> 16);
        data[ 3] = (byte)(mp->h[0] >> 24);
        data[ 4] = (byte)(mp->h[1]      );
        data[ 5] = (byte)(mp->h[1] >>  8);
        data[ 6] = (byte)(mp->h[1] >> 16);
        data[ 7] = (byte)(mp->h[1] >> 24);
        data[ 8] = (byte)(mp->h[2]      );
        data[ 9] = (byte)(mp->h[2] >>  8);
        data[10] = (byte)(mp->h[2] >> 16);
        data[11] = (byte)(mp->h[2] >> 24);
        data[12] = (byte)(mp->h[3]      );
        data[13] = (byte)(mp->h[3] >>  8);
        data[14] = (byte)(mp->h[3] >> 16);
        data[15] = (byte)(mp->h[3] >> 24);
        data[16] = (byte)(mp->h[4]      );
        data[17] = (byte)(mp->h[4] >>  8);
        data[18] = (byte)(mp->h[4] >> 16);
        data[19] = (byte)(mp->h[4] >> 24);

        (void) ripemd160Reset(mp);

        return 0;
}
/*@=protoparammatch@*/

/*!\}
 */

