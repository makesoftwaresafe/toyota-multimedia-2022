/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.com
 */

#include "system.h"

#include "md2.h"
#include <beecrypt/mp.h>
#include <beecrypt/endianness.h>

#include "debug.h"

/*@-sizeoftype@*/
/*@unchecked@*/ /*@observer@*/
const hashFunction md2 = {
	"MD2",
	sizeof(md2Param),
	16,
	128/8,
	(hashFunctionReset) md2Reset,
	(hashFunctionUpdate) md2Update,
	(hashFunctionDigest) md2Digest,
};
/*@=sizeoftype@*/

/*@unchecked@*/ /*@observer@*/
static const byte PI_SUBST[256] = {
  41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
  19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
  76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
  138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
  245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
  148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
  39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
  181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
  150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
  112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
  96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
  85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
  234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
  129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
  8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
  203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
  166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
  31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};

/* adds 16 bytes to the checksum */
static void md2_update_chksum(md2Param *mp)
	/*@modifies mp @*/
{
	byte L = mp->chksum[15];
	int j;

	for (j = 0; j < 16; j++) {

/* caution, the RFC says its "C[j] = S[M[i*16+j] xor L]" but the reference source code [and test vectors] say 
   otherwise.
*/
		L = (mp->chksum[j] ^= PI_SUBST[(int)(mp->buf[j] ^ L)] & 255);
	}
}

static void md2_compress(md2Param *mp)
	/*@modifies mp @*/
{
	int j, k;
	byte t;
   
	/* copy block */
	for (j = 0; j < 16; j++) {
		mp->X[16+j] = mp->buf[j];
		mp->X[32+j] = mp->X[j] ^ mp->X[16+j];
	}

	t = (byte)0;

	/* do 18 rounds */
	for (j = 0; j < 18; j++) {
		for (k = 0; k < 48; k++) {
			t = (mp->X[k] ^= PI_SUBST[(int)(t & 255)]);
		}
		t = (t + (byte)j) & 255;
	}
}

int md2Reset(md2Param *mp)
{
	/* MD2 uses a zero'ed state... */
	memset(mp->X, 0, sizeof(mp->X));
	memset(mp->chksum, 0, sizeof(mp->chksum));
	memset(mp->buf, 0, sizeof(mp->buf));

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

int md2Update(md2Param *mp, const byte *data, size_t size)
{
	uint32_t proclength;

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

	while (size > 0) {
		proclength = ((mp->offset + size) > 16U) ? (16U - mp->offset) : size;
/*@-mayaliasunique@*/
		memcpy(((byte *)mp->buf) + mp->offset, data, proclength);
/*@=mayaliasunique@*/
		size -= proclength;
		data += proclength;
		mp->offset += proclength;

		/* is 16 bytes full? */
		if (mp->offset == 16U) {
			md2_compress(mp);
			md2_update_chksum(mp);
			mp->offset = 0;
		}
	}
	return 0;
}

int md2Digest(md2Param * mp, byte *digest)
{
	uint32_t i, k;

	/* pad the message */
	k = 16 - mp->offset;
	for (i = mp->offset; i < 16; i++) {
		mp->buf[i] = (byte)k;
	}

	/* hash and update */
	md2_compress(mp);
	md2_update_chksum(mp);

	/* hash checksum */
	memcpy(mp->buf, mp->chksum, 16);
	md2_compress(mp);

	/* output is lower 16 bytes of X */
	memcpy(digest, mp->X, 16);

	memset(mp, 0, sizeof(*mp));

	return 0;
}

#ifdef	DYING
/**
  Self-test the hash
  @return 0 if successful, CRYPT_NOP if self-tests have been disabled
*/  
int md2_test(void)
{
   static const struct {
        char *msg;
        byte md[16];
   } tests[] = {
      { "",
        {0x83,0x50,0xe5,0xa3,0xe2,0x4c,0x15,0x3d,
         0xf2,0x27,0x5c,0x9f,0x80,0x69,0x27,0x73
        }
      },
      { "a",
        {0x32,0xec,0x01,0xec,0x4a,0x6d,0xac,0x72,
         0xc0,0xab,0x96,0xfb,0x34,0xc0,0xb5,0xd1
        }
      },
      { "message digest",
        {0xab,0x4f,0x49,0x6b,0xfb,0x2a,0x53,0x0b,
         0x21,0x9f,0xf3,0x30,0x31,0xfe,0x06,0xb0
        }
      },
      { "abcdefghijklmnopqrstuvwxyz",
        {0x4e,0x8d,0xdf,0xf3,0x65,0x02,0x92,0xab,
         0x5a,0x41,0x08,0xc3,0xaa,0x47,0x94,0x0b
        }
      },
      { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
        {0xda,0x33,0xde,0xf2,0xa4,0x2d,0xf1,0x39,
         0x75,0x35,0x28,0x46,0xc3,0x03,0x38,0xcd
        }
      },
      { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
        {0xd5,0x97,0x6f,0x79,0xd8,0x3d,0x3a,0x0d,
         0xc9,0x80,0x6c,0x3c,0x66,0xf3,0xef,0xd8
        }
      }
   };
   int i;
   md2Param mymd, * mp = &mymd;
   byte buf[16];

   for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
       md2Reset(mp);
       md2Update(mp, (byte*)tests[i].msg, strlen(tests[i].msg));
       md2Digest(mp, buf);
       if (memcmp(buf, tests[i].md, 16) != 0) {
          return 5;
       }
   }
   return 0;        
}

int main() {
    int rc = md2_test();
    switch(rc) {
    case 0:	printf("OK\n");		break;
    default:	printf("FAIL\n");	break;
    }
    return rc;
}
#endif
