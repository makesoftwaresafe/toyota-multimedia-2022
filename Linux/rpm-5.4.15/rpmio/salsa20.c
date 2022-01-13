#include "system.h"

#include "salsa20.h"      
#include <beecrypt/mp.h>
#define       HAVE_INLINE     1       /* XXX needs swapu32() from BeeCrypt */
#include <beecrypt/endianness.h>

#include "debug.h"

/*!\addtogroup HASH_salsa20_m
 * \{
 */

/*@unchecked@*/ /*@observer@*/
static uint32_t salsa20hinit[16] = {
        0x6a09e667U, 0xf3bcc908U,
        0xbb67ae85U, 0x84caa73bU,
        0x3c6ef372U, 0xfe94f82bU,
        0xa54ff53aU, 0x5f1d36f1U,
        0x510e527fU, 0xade682d1U,
        0x9b05688cU, 0x2b3e6c1fU,
        0x1f83d9abU, 0xfb41bd6bU,
        0x5be0cd19U, 0x137e2179U
};


/*@-sizeoftype@*/
/*@unchecked@*/ /*@observer@*/
const hashFunction salsa20 = {
	"SALSA-10",
	sizeof(salsa20Param),
	64,
	512/8,
	(hashFunctionReset) salsa20Reset,
	(hashFunctionUpdate) salsa20Update,
	(hashFunctionDigest) salsa20Digest
};
/*@=sizeoftype@*/

int salsa20Reset(register salsa20Param* mp)
{
/*@-sizeoftype@*/
	memcpy(mp->h, salsa20hinit, 16 * sizeof(uint32_t));
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

#ifndef ASM_RMD128PROCESS
void salsa20Process(salsa20Param* mp)
{
	uint32_t X[16];
	int i;

	for (i = 0; i < 16; ++i) {
	    #ifdef WORDS_BIGENDIAN		/* XXX untested */
	    X[i] = swapu32(mp->data[i]);
	    #else
	    X[i] = mp->data[i];
	    #endif
	}
	for (i = 20; i > 0; i -= 2) {
	    X[ 4] ^= ROTL32(X[ 0]+X[12], 7);  X[ 8] ^= ROTL32(X[ 4]+X[ 0], 9);
	    X[12] ^= ROTL32(X[ 8]+X[ 4],13);  X[ 0] ^= ROTL32(X[12]+X[ 8],18);
	    X[ 9] ^= ROTL32(X[ 5]+X[ 1], 7);  X[13] ^= ROTL32(X[ 9]+X[ 5], 9);
	    X[ 1] ^= ROTL32(X[13]+X[ 9],13);  X[ 5] ^= ROTL32(X[ 1]+X[13],18);
	    X[14] ^= ROTL32(X[10]+X[ 6], 7);  X[ 2] ^= ROTL32(X[14]+X[10], 9);
	    X[ 6] ^= ROTL32(X[ 2]+X[14],13);  X[10] ^= ROTL32(X[ 6]+X[ 2],18);
	    X[ 3] ^= ROTL32(X[15]+X[11], 7);  X[ 7] ^= ROTL32(X[ 3]+X[15], 9);
	    X[11] ^= ROTL32(X[ 7]+X[ 3],13);  X[15] ^= ROTL32(X[11]+X[ 7],18);
	    X[ 1] ^= ROTL32(X[ 0]+X[ 3], 7);  X[ 2] ^= ROTL32(X[ 1]+X[ 0], 9);
	    X[ 3] ^= ROTL32(X[ 2]+X[ 1],13);  X[ 0] ^= ROTL32(X[ 3]+X[ 2],18);
	    X[ 6] ^= ROTL32(X[ 5]+X[ 4], 7);  X[ 7] ^= ROTL32(X[ 6]+X[ 5], 9);
	    X[ 4] ^= ROTL32(X[ 7]+X[ 6],13);  X[ 5] ^= ROTL32(X[ 4]+X[ 7],18);
	    X[11] ^= ROTL32(X[10]+X[ 9], 7);  X[ 8] ^= ROTL32(X[11]+X[10], 9);
	    X[ 9] ^= ROTL32(X[ 8]+X[11],13);  X[10] ^= ROTL32(X[ 9]+X[ 8],18);
	    X[12] ^= ROTL32(X[15]+X[14], 7);  X[13] ^= ROTL32(X[12]+X[15], 9);
	    X[14] ^= ROTL32(X[13]+X[12],13);  X[15] ^= ROTL32(X[14]+X[13],18);
	}
	for (i = 0; i < 16; ++i)
	    X[i] += mp->data[i];

	/* CBC chaining on stream cipher blocks. */
	for (i = 0; i < 16; i++)
	    mp->h[i] += X[i];
}
#endif

int salsa20Update(salsa20Param* mp, const byte* data, size_t size)
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
			salsa20Process(mp);
			mp->offset = 0;
		}
	}
	return 0;
}

static void salsa20Finish(salsa20Param* mp)
	/*@modifies mp @*/
{
	register byte *ptr = ((byte *) mp->data) + mp->offset++;

	*(ptr++) = 0x80;

	if (mp->offset > 56)
	{
		while (mp->offset++ < 64)
			*(ptr++) = 0;

		salsa20Process(mp);
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

	salsa20Process(mp);

	mp->offset = 0;
}

/*@-protoparammatch@*/
int salsa20Digest(salsa20Param* mp, byte* data)
{
	salsa20Finish(mp);

	/* encode 16 integers little-endian style */
	data[ 0] = (byte)(mp->h[ 0] >>  0);
	data[ 1] = (byte)(mp->h[ 0] >>  8);
	data[ 2] = (byte)(mp->h[ 0] >> 16);
	data[ 3] = (byte)(mp->h[ 0] >> 24);
	data[ 4] = (byte)(mp->h[ 1] >>  0);
	data[ 5] = (byte)(mp->h[ 1] >>  8);
	data[ 6] = (byte)(mp->h[ 1] >> 16);
	data[ 7] = (byte)(mp->h[ 1] >> 24);
	data[ 8] = (byte)(mp->h[ 2] >>  0);
	data[ 9] = (byte)(mp->h[ 2] >>  8);
	data[10] = (byte)(mp->h[ 2] >> 16);
	data[11] = (byte)(mp->h[ 2] >> 24);
	data[12] = (byte)(mp->h[ 3] >>  0);
	data[13] = (byte)(mp->h[ 3] >>  8);
	data[14] = (byte)(mp->h[ 3] >> 16);
	data[15] = (byte)(mp->h[ 3] >> 24);

	data[16] = (byte)(mp->h[ 4] >>  0);
	data[17] = (byte)(mp->h[ 4] >>  8);
	data[18] = (byte)(mp->h[ 4] >> 16);
	data[19] = (byte)(mp->h[ 4] >> 24);
	data[20] = (byte)(mp->h[ 5] >>  0);
	data[21] = (byte)(mp->h[ 5] >>  8);
	data[22] = (byte)(mp->h[ 5] >> 16);
	data[23] = (byte)(mp->h[ 5] >> 24);
	data[24] = (byte)(mp->h[ 6] >>  0);
	data[25] = (byte)(mp->h[ 6] >>  8);
	data[26] = (byte)(mp->h[ 6] >> 16);
	data[27] = (byte)(mp->h[ 6] >> 24);
	data[28] = (byte)(mp->h[ 7] >>  0);
	data[29] = (byte)(mp->h[ 7] >>  8);
	data[30] = (byte)(mp->h[ 7] >> 16);
	data[31] = (byte)(mp->h[ 7] >> 24);

	data[32] = (byte)(mp->h[ 8] >>  0);
	data[33] = (byte)(mp->h[ 8] >>  8);
	data[34] = (byte)(mp->h[ 8] >> 16);
	data[35] = (byte)(mp->h[ 8] >> 24);
	data[36] = (byte)(mp->h[ 9] >>  0);
	data[37] = (byte)(mp->h[ 9] >>  8);
	data[38] = (byte)(mp->h[ 9] >> 16);
	data[39] = (byte)(mp->h[ 9] >> 24);
	data[40] = (byte)(mp->h[10] >>  0);
	data[41] = (byte)(mp->h[10] >>  8);
	data[42] = (byte)(mp->h[10] >> 16);
	data[43] = (byte)(mp->h[10] >> 24);
	data[44] = (byte)(mp->h[11] >>  0);
	data[45] = (byte)(mp->h[11] >>  8);
	data[46] = (byte)(mp->h[11] >> 16);
	data[47] = (byte)(mp->h[11] >> 24);

	data[48] = (byte)(mp->h[12] >>  0);
	data[49] = (byte)(mp->h[12] >>  8);
	data[50] = (byte)(mp->h[12] >> 16);
	data[51] = (byte)(mp->h[12] >> 24);
	data[52] = (byte)(mp->h[13] >>  0);
	data[53] = (byte)(mp->h[13] >>  8);
	data[54] = (byte)(mp->h[13] >> 16);
	data[55] = (byte)(mp->h[13] >> 24);
	data[56] = (byte)(mp->h[14] >>  0);
	data[57] = (byte)(mp->h[14] >>  8);
	data[58] = (byte)(mp->h[14] >> 16);
	data[59] = (byte)(mp->h[14] >> 24);
	data[60] = (byte)(mp->h[15] >>  0);
	data[61] = (byte)(mp->h[15] >>  8);
	data[62] = (byte)(mp->h[15] >> 16);
	data[63] = (byte)(mp->h[15] >> 24);

	(void) salsa20Reset(mp);
	return 0;
}
/*@=protoparammatch@*/

/*!\}
 */
