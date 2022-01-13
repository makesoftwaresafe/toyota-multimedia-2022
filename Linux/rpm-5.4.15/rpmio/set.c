/*
 * set.c - base62, golomb and set-string routines
 *
 * Copyright (C) 2010, 2011, 2012  Alexey Tourbin <at@altlinux.org>
 *
 * License: GPLv2+ or LGPL, see RPM COPYING
 */

/* XXX FIXME: avoid <fcntl.h> borkage on RHEL for now. */
#define _FCNTL_H        1

#include "system.h"

#include <rpmio.h>
#ifdef SELF_TEST
#include <poptIO.h>
#endif

#define	_RPMSET_INTERNAL
#include "set.h"

#include "debug.h"

int _rpmset_debug = 0;

/*==============================================================*/

/*
 * Base62 routines - encode bits with alnum characters.
 *
 * This is a base64-based base62 implementation.  Values 0..61 are encoded
 * with '0'..'9', 'a'..'z', and 'A'..'Z'.  However, 'Z' is special: it will
 * also encode 62 and 63.  To achieve this, 'Z' will occupy two high bits in
 * the next character.  Thus 'Z' can be interpreted as an escape character
 * (which indicates that the next character must be handled specially).
 * Note that setting high bits to "00", "01" or "10" cannot contribute
 * to another 'Z' (which would require high bits set to "11").  This is
 * how multiple escapes are avoided.
 */

/* Estimate base62 buffer size required to encode a given number of bits. */
static inline
int encode_base62_size(int bitc)
{
    /*
     * In the worst case, which is ZxZxZx..., five bits can make a character;
     * the remaining bits can make a character, too.  And the string must be
     * null-terminated.
     */
    return bitc / 5 + 2;
}

    static
    void put_digit(char *base62, int c)
    {
	assert(c >= 0 && c <= 61);
	if (c < 10)
	    *base62++ = c + '0';
	else if (c < 36)
	    *base62++ = c - 10 + 'a';
	else if (c < 62)
	    *base62++ = c - 36 + 'A';
    }

/* Main base62 encoding routine: pack bitv into base62 string. */
static
int encode_base62(int bitc, const char *bitv, char *base62)
{
    char *base62_start = base62;
    int bits2 = 0; /* number of high bits set */
    int bits6 = 0; /* number of regular bits set */
    int num6b = 0; /* pending 6-bit number */
    while (bitc-- > 0) {
	num6b |= (*bitv++ << bits6++);
	if (bits6 + bits2 < 6)
	    continue;
	switch (num6b) {
	case 61:
	    /* escape */
	    put_digit(base62, 61);
	    /* extra "00...." high bits (in the next character) */
	    bits2 = 2;
	    bits6 = 0;
	    num6b = 0;
	    break;
	case 62:
	    put_digit(base62, 61);
	    /* extra "01...." high bits */
	    bits2 = 2;
	    bits6 = 0;
	    num6b = 16;
	    break;
	case 63:
	    put_digit(base62, 61);
	    /* extra "10...." high bits */
	    bits2 = 2;
	    bits6 = 0;
	    num6b = 32;
	    break;
	default:
	    assert(num6b < 61);
	    put_digit(base62, num6b);
	    bits2 = 0;
	    bits6 = 0;
	    num6b = 0;
	    break;
	}
    }
    if (bits6 + bits2) {
	assert(num6b < 61);
	put_digit(base62, num6b);
    }
    *base62 = '\0';
    return base62 - base62_start;
}

/* Estimate how many bits will result from decoding a base62 string. */
static inline
int decode_base62_size(int len)
{
    /* Each character will fill at most 6 bits. */
    return len * 6;
}

/* This table maps alnum characters to their numeric values. */
static
const int char_to_num[256] = {
    [0 ... 255] = 0xee,
    [0] = 0xff,
#define C1(c, b) [c] = c - b
#define C2(c, b) C1(c, b), C1(c + 1, b)
#define C5(c, b) C1(c, b), C2(c + 1, b), C2(c + 3, b)
#define C10(c, b) C5(c, b), C5(c + 5, b)
    C10('0', '0'),
#define C26(c, b) C1(c, b), C5(c + 1, b), C10(c + 6, b), C10(c + 16, b)
    C26('a', 'a' + 10),
    C26('A', 'A' + 36),
};

    static inline
    void put6bits(char *bitv, int c)
    {
	*bitv++ = (c >> 0) & 1;
	*bitv++ = (c >> 1) & 1;
	*bitv++ = (c >> 2) & 1;
	*bitv++ = (c >> 3) & 1;
	*bitv++ = (c >> 4) & 1;
	*bitv++ = (c >> 5) & 1;
    }
    static inline
    void put4bits(char *bitv, int c)
    {
	*bitv++ = (c >> 0) & 1;
	*bitv++ = (c >> 1) & 1;
	*bitv++ = (c >> 2) & 1;
	*bitv++ = (c >> 3) & 1;
    }

/* Main base62 decoding routine: unpack base62 string into bitv[]. */
static
int decode_base62(const char *base62, char *bitv)
{
    char *bitv_start = bitv;
    while (1) {
	long c = (unsigned char) *base62++;
	int num6b = char_to_num[c];
	while (num6b < 61) {
	    put6bits(bitv, num6b);
	    c = (unsigned char) *base62++;
	    num6b = char_to_num[c];
	}
	if (num6b == 0xff)
	    break;
	if (num6b == 0xee)
	    return -1;
	assert(num6b == 61);
	c = (unsigned char) *base62++;
	int num4b = char_to_num[c];
	if (num4b == 0xff)
	    return -2;
	if (num4b == 0xee)
	    return -3;
	switch (num4b & (16 + 32)) {
	case 0:
	    break;
	case 16:
	    num6b = 62;
	    num4b &= ~16;
	    break;
	case 32:
	    num6b = 63;
	    num4b &= ~32;
	    break;
	default:
	    return -4;
	}
	put6bits(bitv, num6b);
	put4bits(bitv, num4b);
    }
    return bitv - bitv_start;
}

#ifdef SELF_TEST
static
void test_base62(void)
{
    const char rnd_bitv[] = {
	1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1,
	1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0,
	/* trigger some 'Z' */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    const int rnd_bitc = sizeof rnd_bitv;
    /* encode */
    char base62[encode_base62_size(rnd_bitc)];
    int len = encode_base62(rnd_bitc, rnd_bitv, base62);
    assert(len > 0);
    assert(len == (int)strlen(base62));
    fprintf(stderr, "len=%d base62=%s\n", len, base62);
    /* The length cannot be shorter than 6 bits per symbol. */
    assert(len >= rnd_bitc / 6);
    /* Neither too long: each second character must fill at least 4 bits. */
    assert(len <= rnd_bitc / 2 / 4 + rnd_bitc / 2 / 6 + 1);
    /* decode */
    char bitv[decode_base62_size(len)];
    int bitc = decode_base62(base62, bitv);
    fprintf(stderr, "rnd_bitc=%d bitc=%d\n", rnd_bitc, bitc);
    assert(bitc >= rnd_bitc);
    /* Decoded bits must match. */
    int i;
    for (i = 0; i < rnd_bitc; i++)
	assert(rnd_bitv[i] == bitv[i]);
    // The remaining bits must be zero bits.
    for (i = rnd_bitc; i < bitc; i++)
	assert(bitv[i] == 0);
    fprintf(stderr, "%s: base62 test OK\n", __FILE__);
}
#endif

/*
 * Golomb-Rice routines - compress integer values into bits.
 *
 * The idea is as follows.  Input values are assumed to be small integers.
 * Each value is split into two parts: an integer resulting from its higher
 * bits and an integer resulting from its lower bits (with the number of lower
 * bits specified by the Mshift parameter).  The frist integer is then stored
 * in unary coding (which is a variable-length sequence of '0' followed by a
 * terminating '1'); the second part is stored in normal binary coding (using
 * Mshift bits).
 *
 * The method is justified by the fact that, since most of the values are
 * small, their first parts will be short (typically 1..3 bits).  In particular,
 * the method is known to be optimal for uniformly distributed hash values,
 * after the values are sorted and delta-encoded.  See e.g.
 * Putze, F.; Sanders, P.; Singler, J. (2007),
 * "Cache-, Hash- and Space-Efficient Bloom Filters",
 * http://algo2.iti.uni-karlsruhe.de/singler/publications/cacheefficientbloomfilters-wea2007.pdf
 */

    static
    int log2i(int n)
    {
	int m = 0;
	while (n >>= 1)
	    m++;
	return m;
    }

/* Calculate Mshift paramter for encoding. */
static
int encode_golomb_Mshift(int c, int bpp)
{
    /*
     * XXX Slightly better Mshift estimations are probably possible.
     * Recheck "Compression and coding algorithms" by Moffat & Turpin.
     */
    int Mshift = bpp - log2i(c) - 1;
    /* Adjust out-of-range values. */
    if (Mshift < 7)
	Mshift = 7;
    if (Mshift > 31)
	Mshift = 31;
    assert(Mshift < bpp);
    return Mshift;
}

/* Estimate how many bits can be filled up. */
static inline
int encode_golomb_size(int c, int Mshift)
{
    /*
     * XXX No precise estimation.  However, we do not expect unary-encoded bits
     * to take more than binary-encoded Mshift bits.
     */
    return Mshift * 2 * c + 16;
}

/* Main golomb encoding routine: package integers into bits. */
static
int encode_golomb(int c, const unsigned *v, int Mshift, char *bitv)
{
    char *bitv_start = bitv;
    const unsigned mask = (1 << Mshift) - 1;
    while (c > 0) {
	c--;
	unsigned v0 = *v++;
	int i;
	/* first part: variable-length sequence */
	unsigned q = v0 >> Mshift;
	for (i = 0; i < (int)q; i++)
	    *bitv++ = 0;
	*bitv++ = 1;
	/* second part: lower Mshift bits */
	unsigned r = v0 & mask;
	for (i = 0; i < Mshift; i++)
	    *bitv++ = (r >> i) & 1;
    }
    return bitv - bitv_start;
}

/* Estimate how many values will emerge. */
static inline
int decode_golomb_size(int bitc, int Mshift)
{
    /*
     * Each (Mshift + 1) bits can make a value.
     * The remaining bits cannot make a value, though.
     */
    return bitc / (Mshift + 1);
}

/* Main golomb decoding routine: unpackage bits into values. */
static
int decode_golomb(int bitc, const char *bitv, int Mshift, unsigned *v)
{
    unsigned *v_start = v;
    /* next value */
    while (bitc > 0) {
	/* first part */
	unsigned q = 0;
	char bit = 0;
	while (bitc > 0) {
	    bitc--;
	    bit = *bitv++;
	    if (bit == 0)
		q++;
	    else
		break;
	}
	/* trailing zero bits in the input are okay */
	if (bitc == 0 && bit == 0) {
	    /* up to 5 bits can be used to complete last character */
	    if (q > 5)
		return -10;
	    break;
	}
	/* otherwise, incomplete value is not okay */
	if (bitc < Mshift)
	    return -11;
	/* second part */
	unsigned r = 0;
	int i;
	for (i = 0; i < Mshift; i++) {
	    bitc--;
	    if (*bitv++)
		r |= (1 << i);
	}
	/* the value */
	*v++ = (q << Mshift) | r;
    }
    return v - v_start;
}

#ifdef SELF_TEST
static
void test_golomb(void)
{
    const unsigned rnd_v[] = {
	/* do re mi fa sol la si */
	1, 2, 3, 4, 5, 6, 7,
	/* koshka sela na taksi */
	7, 6, 5, 4, 3, 2, 1,
    };
    const int rnd_c = sizeof rnd_v / sizeof *rnd_v;
    int bpp = 10;
    int Mshift = encode_golomb_Mshift(rnd_c, bpp);
    fprintf(stderr, "rnd_c=%d bpp=%d Mshift=%d\n", rnd_c, bpp, Mshift);
    assert(Mshift > 0);
    assert(Mshift < bpp);
    /* encode */
    int alloc_bitc = encode_golomb_size(rnd_c, Mshift);
    assert(alloc_bitc > rnd_c);
    char bitv[alloc_bitc];
    int bitc = encode_golomb(rnd_c, rnd_v, Mshift, bitv);
    fprintf(stderr, "alloc_bitc=%d bitc=%d\n", alloc_bitc, bitc);
    assert(bitc > rnd_c);
    assert(bitc <= alloc_bitc);
    /* decode */
    int alloc_c = decode_golomb_size(bitc, Mshift);
    assert(alloc_c >= rnd_c);
    unsigned v[alloc_c];
    int c = decode_golomb(bitc, bitv, Mshift, v);
    fprintf(stderr, "rnd_c=%d alloc_c=%d c=%d\n", rnd_c, alloc_c, c);
    assert(alloc_c >= c);
    /* Decoded values must match. */
    assert(rnd_c == c);
    int i;
    for (i = 0; i < c; i++)
	assert(rnd_v[i] == v[i]);
    /* At the end of the day, did it save your money? */
    int golomb_bpp = bitc / c;
    fprintf(stderr, "bpp=%d golomb_bpp=%d\n", bpp, golomb_bpp);
    assert(golomb_bpp < bpp);
    fprintf(stderr, "%s: golomb test OK\n", __FILE__);
}
#endif

/*
 * Combined base62+gololb decoding routine - implemented for efficiency.
 *
 * As Dmitry V. Levin once noticed, when it comes to speed, very few objections
 * can be made against complicating the code.  Which reminds me of Karl Marx,
 * who said that there is not a crime at which a capitalist will scruple for
 * the sake of 300 per cent profit, even at the chance of being hanged.  Anyway,
 * here Alexey Tourbin demonstrates that by using sophisticated - or should he
 * say "ridiculously complicated" - techniques it is indeed possible to gain
 * some profit, albeit of another kind.
 */

/* Word types (when two bytes from base62 string cast to unsigned short). */
enum {
    W_AA = 0x0000,
    W_AZ = 0x1000,
    W_ZA = 0x2000,
    W_A0 = 0x3000,
    W_0X = 0x4000,
    W_EE = 0xeeee,
};

/* Combine two characters into array index (with respect to endianness). */
#if BYTE_ORDER && BYTE_ORDER == LITTLE_ENDIAN
#define CCI(c1, c2) ((c1) | ((c2) << 8))
#elif BYTE_ORDER && BYTE_ORDER == BIG_ENDIAN
#define CCI(c1, c2) ((c2) | ((c1) << 8))
#else
#error "unknown byte order"
#endif

/* Maps base62 word into numeric value (decoded bits) ORed with word type. */
static
const unsigned short word_to_num[65536] = {
    [0 ... 65535] = W_EE,
#define AA1(c1, c2, b1, b2) [CCI(c1, c2)] = (c1 - b1) | ((c2 - b2) << 6)
#define AA1x2(c1, c2, b1, b2) AA1(c1, c2, b1, b2), AA1(c1, c2 + 1, b1, b2)
#define AA1x3(c1, c2, b1, b2) AA1(c1, c2, b1, b2), AA1x2(c1, c2 + 1, b1, b2)
#define AA1x5(c1, c2, b1, b2) AA1x2(c1, c2, b1, b2), AA1x3(c1, c2 + 2, b1, b2)
#define AA1x10(c1, c2, b1, b2) AA1x5(c1, c2, b1, b2), AA1x5(c1, c2 + 5, b1, b2)
#define AA1x20(c1, c2, b1, b2) AA1x10(c1, c2, b1, b2), AA1x10(c1, c2 + 10, b1, b2)
#define AA1x25(c1, c2, b1, b2) AA1x5(c1, c2, b1, b2), AA1x20(c1, c2 + 5, b1, b2)
#define AA2x1(c1, c2, b1, b2) AA1(c1, c2, b1, b2), AA1(c1 + 1, c2, b1, b2)
#define AA3x1(c1, c2, b1, b2) AA1(c1, c2, b1, b2), AA2x1(c1 + 1, c2, b1, b2)
#define AA5x1(c1, c2, b1, b2) AA2x1(c1, c2, b1, b2), AA3x1(c1 + 2, c2, b1, b2)
#define AA10x1(c1, c2, b1, b2) AA5x1(c1, c2, b1, b2), AA5x1(c1 + 5, c2, b1, b2)
#define AA20x1(c1, c2, b1, b2) AA10x1(c1, c2, b1, b2), AA10x1(c1 + 10, c2, b1, b2)
#define AA25x1(c1, c2, b1, b2) AA5x1(c1, c2, b1, b2), AA20x1(c1 + 5, c2, b1, b2)
#define AA26x1(c1, c2, b1, b2) AA1(c1, c2, b1, b2), AA25x1(c1 + 1, c2, b1, b2)
#define AA2x5(c1, c2, b1, b2) AA1x5(c1, c2, b1, b2), AA1x5(c1 + 1, c2, b1, b2)
#define AA3x5(c1, c2, b1, b2) AA1x5(c1, c2, b1, b2), AA2x5(c1 + 1, c2, b1, b2)
#define AA5x5(c1, c2, b1, b2) AA2x5(c1, c2, b1, b2), AA3x5(c1 + 2, c2, b1, b2)
#define AA5x10(c1, c2, b1, b2) AA5x5(c1, c2, b1, b2), AA5x5(c1, c2 + 5, b1, b2)
#define AA10x5(c1, c2, b1, b2) AA5x5(c1, c2, b1, b2), AA5x5(c1 + 5, c2, b1, b2)
#define AA20x5(c1, c2, b1, b2) AA10x5(c1, c2, b1, b2), AA10x5(c1 + 10, c2, b1, b2)
#define AA25x5(c1, c2, b1, b2) AA5x5(c1, c2, b1, b2), AA20x5(c1 + 5, c2, b1, b2)
#define AA10x10(c1, c2, b1, b2) AA5x10(c1, c2, b1, b2), AA5x10(c1 + 5, c2, b1, b2)
#define AA10x20(c1, c2, b1, b2) AA10x10(c1, c2, b1, b2), AA10x10(c1, c2 + 10, b1, b2)
#define AA10x25(c1, c2, b1, b2) AA10x5(c1, c2, b1, b2), AA10x20(c1, c2 + 5, b1, b2)
#define AA10x26(c1, c2, b1, b2) AA10x1(c1, c2, b1, b2), AA10x25(c1, c2 + 1, b1, b2)
#define AA20x10(c1, c2, b1, b2) AA10x10(c1, c2, b1, b2), AA10x10(c1 + 10, c2, b1, b2)
#define AA25x10(c1, c2, b1, b2) AA5x10(c1, c2, b1, b2), AA20x10(c1 + 5, c2, b1, b2)
#define AA26x10(c1, c2, b1, b2) AA1x10(c1, c2, b1, b2), AA25x10(c1 + 1, c2, b1, b2)
#define AA25x20(c1, c2, b1, b2) AA25x10(c1, c2, b1, b2), AA25x10(c1, c2 + 10, b1, b2)
#define AA25x25(c1, c2, b1, b2) AA25x5(c1, c2, b1, b2), AA25x20(c1, c2 + 5, b1, b2)
#define AA25x26(c1, c2, b1, b2) AA25x1(c1, c2, b1, b2), AA25x25(c1, c2 + 1, b1, b2)
#define AA26x25(c1, c2, b1, b2) AA1x25(c1, c2, b1, b2), AA25x25(c1 + 1, c2, b1, b2)
#define AA26x26(c1, c2, b1, b2) AA26x1(c1, c2, b1, b2), AA26x25(c1, c2 + 1, b1, b2)
    AA10x10('0', '0', '0', '0'),
    AA10x26('0', 'a', '0', 'a' + 10),
    AA10x25('0', 'A', '0', 'A' + 36),
    AA26x10('a', '0', 'a' + 10, '0'),
    AA25x10('A', '0', 'A' + 36, '0'),
    AA26x26('a', 'a', 'a' + 10, 'a' + 10),
    AA26x25('a', 'A', 'a' + 10, 'A' + 36),
    AA25x26('A', 'a', 'A' + 36, 'a' + 10),
    AA25x25('A', 'A', 'A' + 36, 'A' + 36),
#define AZ1(c, b) [CCI(c, 'Z')] = (c - b) | W_AZ
#define AZ2(c, b) AZ1(c, b), AZ1(c + 1, b)
#define AZ5(c, b) AZ1(c, b), AZ2(c + 1, b), AZ2(c + 3, b)
#define AZ10(c, b) AZ5(c, b), AZ5(c + 5, b)
#define AZ25(c, b) AZ5(c, b), AZ10(c + 5, b), AZ10(c + 15, b)
#define AZ26(c, b) AZ1(c, b), AZ25(c + 1, b)
    AZ10('0', '0'),
    AZ26('a', 'a' + 10),
    AZ25('A', 'A' + 36),
#define ZA1(c, b)  [CCI('Z', c)] = (61 + ((c - b) >> 4)) | (((c - b) & 0xf) << 6) | W_ZA
#define ZA2(c, b) ZA1(c, b), ZA1(c + 1, b)
#define ZA5(c, b) ZA1(c, b), ZA2(c + 1, b), ZA2(c + 3, b)
#define ZA10(c, b) ZA5(c, b), ZA5(c + 5, b)
#define ZA25(c, b) ZA5(c, b), ZA10(c + 5, b), ZA10(c + 15, b)
#define ZA26(c, b) ZA1(c, b), ZA25(c + 1, b)
    ZA10('0', '0'),
    ZA26('a', 'a' + 10),
    ZA25('A', 'A' + 36),
#define A01(c, b) [CCI(c, 0)] = (c - b) | W_A0
#define A02(c, b) A01(c, b), A01(c + 1, b)
#define A05(c, b) A01(c, b), A02(c + 1, b), A02(c + 3, b)
#define A010(c, b) A05(c, b), A05(c + 5, b)
#define A025(c, b) A05(c, b), A010(c + 5, b), A010(c + 15, b)
#define A026(c, b) A01(c, b), A025(c + 1, b)
    A010('0', '0'),
    A026('a', 'a' + 10),
    A025('A', 'A' + 36),
#define OX(c) [CCI(0, c)] = W_0X
#define OX4(c) OX(c), OX(c + 1), OX(c + 2), OX(c + 3)
#define OX16(c) OX4(c), OX4(c + 4), OX4(c + 8), OX4(c + 12)
#define OX64(c) OX16(c), OX16(c + 16), OX16(c + 32), OX16(c + 48)
#define OX256(c) OX64(c), OX64(c + 64), OX64(c + 128), OX64(c + 192)
    OX256('\0'),
};

/* Combined base62+golomb decoding routine. */
static
int decode_base62_golomb(const char *base62, int Mshift, unsigned *v)
{
    unsigned *v_start = v;
    unsigned mask = (1 << Mshift) - 1;
    unsigned q = 0;
    unsigned r = 0;
    int rfill = 0;
    long c, w;
    int n, vbits, left;
    unsigned bits, morebits;
    /* need align */
    if (1 & (long) base62) {
	c = (unsigned char) *base62++;
	bits = char_to_num[c];
	if (bits < 61)
	    goto put6q_align;
	else {
	    if (bits == 0xff)
		goto eolq;
	    if (bits == 0xee)
		return -1;
	    assert(bits == 61);
	    goto esc1q;
	}
    }
    /* regular mode, process two-byte words */
#define Get24(X) \
    w = *(unsigned short *) base62; \
    base62 += 2; \
    bits = word_to_num[w]; \
    if (bits >= 0x1000) \
	goto gotNN ## X; \
    w = *(unsigned short *) base62; \
    base62 += 2; \
    morebits = word_to_num[w]; \
    if (morebits >= 0x1000) \
	goto put12 ## X; \
    bits |= (morebits << 12); \
    goto put24 ## X
#define Get12(X) \
    bits = morebits
#define GotNN(X) \
    switch (bits & 0xf000) { \
    case W_AZ: \
	bits &= 0x0fff; \
	goto put6 ## X ## _AZ; \
    case W_ZA: \
	bits &= 0x0fff; \
	goto put10 ## X ## _ZA; \
    case W_A0: \
	bits &= 0x0fff; \
	goto put6 ## X ## _A0; \
    case W_0X: \
	goto eol ## X; \
    default: \
	return -2; \
    }
    /* make coroutines */
    get24q: Get24(q);
    get24r: Get24(r);
    get12q: Get12(q);
    gotNNq: GotNN(q);
    get12r: Get12(r);
    gotNNr: GotNN(r);
    /* escape mode, handle 2 bytes one by one */
#define Esc1(X) \
    bits = 61; \
    c = (unsigned char) *base62++; \
    morebits = char_to_num[c]; \
    if (morebits == 0xff) \
	return -3; \
    if (morebits == 0xee) \
	return -4; \
    switch (morebits & (16 + 32)) { \
    case 0: \
	break; \
    case 16: \
	bits = 62; \
	morebits &= ~16; \
	break; \
    case 32: \
	bits = 63; \
	morebits &= ~32; \
	break; \
    default: \
	return -5; \
    } \
    bits |= (morebits << 6); \
    goto put10 ## X ## _esc1
#define Esc2(X) \
    c = (unsigned char) *base62++; \
    bits = char_to_num[c]; \
    if (bits < 61) \
	goto put6 ## X ## _esc2; \
    else { \
	if (bits == 0xff) \
	    goto eol ## X; \
	if (bits == 0xee) \
	    return -6; \
	goto esc1 ## X; \
    }
    /* make coroutines */
    esc1q: Esc1(q);
    esc2q: Esc2(q);
    esc1r: Esc1(r);
    esc2r: Esc2(r);
    /* golomb pieces */
#define QInit(N) \
    n = N
#define RInit(N) \
    n = N; \
    r |= (bits << rfill); \
    rfill += n
#define RMake(Get) \
    left = rfill - Mshift; \
    if (left < 0) \
	goto Get ## r; \
    r &= mask; \
    *v++ = (q << Mshift) | r; \
    q = 0; \
    bits >>= n - left; \
    n = left
#define QMake(Get) \
    if (bits == 0) { \
	q += n; \
	goto Get ## q; \
    } \
    vbits = __builtin_ffs(bits); \
    n -= vbits; \
    bits >>= vbits; \
    q += vbits - 1; \
    r = bits; \
    rfill = n
    /* this assumes that minimum Mshift value is 7 */
#define Put24Q(Get) \
    QInit(24); \
    QMake(Get); RMake(Get); \
    QMake(Get); RMake(Get); \
    QMake(Get); RMake(Get); \
    goto Get ## q
#define Put24R(Get) \
    RInit(24); \
    RMake(Get); \
    QMake(Get); RMake(Get); \
    QMake(Get); RMake(Get); \
    QMake(Get); goto Get ## r
#define Put12Q(Get) \
    QInit(12); \
    QMake(Get); RMake(Get); \
    QMake(Get); goto Get ## r
#define Put12R(Get) \
    RInit(12); \
    RMake(Get); \
    QMake(Get); RMake(Get); \
    QMake(Get); goto Get ## r
#define Put10Q(Get) \
    QInit(10); \
    QMake(Get); RMake(Get); \
    QMake(Get); goto Get ## r
#define Put10R(Get) \
    RInit(10); \
    RMake(Get); \
    QMake(Get); RMake(Get); \
    QMake(Get); goto Get ## r
#define Put6Q(Get) \
    QInit(6); \
    QMake(Get); goto Get ## r
#define Put6R(Get) \
    RInit(6); \
    RMake(Get); \
    QMake(Get); goto Get ## r
    /* make coroutines */
    put24q: Put24Q(get24);
    put24r: Put24R(get24);
    put12q: Put12Q(get12);
    put12r: Put12R(get12);
    put6q_align:
    put6q_esc2: Put6Q(get24);
    put6r_esc2: Put6R(get24);
    put6q_AZ: Put6Q(esc1);
    put6r_AZ: Put6R(esc1);
    put10q_esc1: Put10Q(esc2);
    put10r_esc1: Put10R(esc2);
    put10q_ZA: Put10Q(get24);
    put10r_ZA: Put10R(get24);
    put6q_A0: Put6Q(eol);
    put6r_A0: Put6R(eol);
    /* handle end of line and return */
  eolq:
    if (q > 5)
	return -10;
    return v - v_start;
  eolr:
    return -11;
}

#ifdef SELF_TEST
static
void test_word_table(void)
{
    int i, j;
    for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++) {
	unsigned char u[2] __attribute__((aligned(2))) = { i, j };
	unsigned short ix = *(unsigned short *) u;
	int w = word_to_num[ix];
	if (w < 0x1000)
	    assert(w == (char_to_num[i] | (char_to_num[j] << 6)));
	else
	    assert(char_to_num[i] >= 61 || char_to_num[j] >= 61);
    }
    fprintf(stderr, "%s: word table test OK\n", __FILE__);
}

static
void test_base62_golomb(void)
{
    const char str[] = "set:hdf7q2P5VZwtLGr9TKxhrEM1";
    const char *base62 = str + 4 + 2;
    int Mshift = 10;
    char bitv[256];
    int bitc = decode_base62(base62, bitv);
    assert(bitc > 0);
    unsigned v1[32], v2[32];
    int c1 = decode_golomb(bitc, bitv, Mshift, v1);
    assert(c1 > 0);
    int c2 = decode_base62_golomb(base62, Mshift, v2);
    assert(c2 > 0);
    assert(c1 == c2);
    int i;
    for (i = 0; i < c1; i++)
	assert(v1[i] == v2[i]);
    fprintf(stderr, "%s: base62_golomb test OK\n", __FILE__);
}
#endif

/*
 * Delta encoding routines - replace an increasing sequence of integer values
 * by the sequence of their differences.
 */

static
void encode_delta(int c, unsigned *v)
{
    assert(c > 0);
    unsigned *v_end = v + c;
    unsigned v0 = *v++;
    while (v < v_end) {
	*v -= v0;
	v0 += *v++;
    }
}

static
void decode_delta(int c, unsigned *v)
{
    assert(c > 0);
    unsigned *v_end = v + c;
    unsigned v0 = *v++;
    while (v < v_end) {
	*v += v0;
	v0 = *v++;
    }
}

#ifdef SELF_TEST
static
void test_delta(void)
{
    unsigned v[] = {
	1, 3, 7, 0
    };
    int c = 3;
    encode_delta(c, v);
    assert(v[0] == 1);
    assert(v[1] == 2);
    assert(v[2] == 4);
    assert(v[3] == 0);
    decode_delta(c, v);
    assert(v[0] == 1);
    assert(v[1] == 3);
    assert(v[2] == 7);
    assert(v[3] == 0);
    fprintf(stderr, "%s: delta test OK\n", __FILE__);
}
#endif

/*
 * Higher-level set-string routines - serialize integers into set-string.
 *
 * A set-string looks like this: "set:bMxyz..."
 *
 * The "set:" prefix marks set-versions in rpm (to distinguish them between
 * regular rpm versions).  It is assumed to be stripped here.
 *
 * The next two characters (denoted 'b' and 'M') encode two small integers
 * in the range 7..32 using 'a'..'z'.  The first character encodes bpp.
 * Valid bpp range is 10..32.  The second character encodes Mshift.  Valid
 * Mshift range is 7..31.  Also, valid Mshift must be less than bpp.
 *
 * The rest ("xyz...") is a variable-length sequence of alnum characters.
 * It encodes a (sorted) set of (non-negative) integer values, as follows:
 * integers are delta-encoded, golomb-compressed and base62-serialized.
 */

static
int encode_set_size(int c, int bpp)
{
    int Mshift = encode_golomb_Mshift(c, bpp);
    int bitc = encode_golomb_size(c, Mshift);
    /* two leading characters are special */
    return 2 + encode_base62_size(bitc);
}

static
int encode_set(int c, unsigned *v, int bpp, char *base62)
{
    /* XXX v is non-const due to encode_delta */
    int Mshift = encode_golomb_Mshift(c, bpp);
    int bitc = encode_golomb_size(c, Mshift);
    char bitv[bitc];
    /* bpp */
    if (bpp < 10 || bpp > 32)
	return -1;
    *base62++ = bpp - 7 + 'a';
    /* golomb parameter */
    if (Mshift < 7 || Mshift > 31)
	return -2;
    *base62++ = Mshift - 7 + 'a';
    /* delta */
    encode_delta(c, v);
    /* golomb */
    bitc = encode_golomb(c, v, Mshift, bitv);
#ifdef SELF_TEST
    decode_delta(c, v);
#endif
    if (bitc < 0)
	return -3;
    /* base62 */
    int len = encode_base62(bitc, bitv, base62);
    if (len < 0)
	return -4;
    return 2 + len;
}

static
int decode_set_init(const char *str, int *pbpp, int *pMshift)
{
    /* 7..32 values encoded with 'a'..'z' */
    int bpp = *str++ + 7 - 'a';
    if (bpp < 10 || bpp > 32)
	return -1;
    /* golomb parameter */
    int Mshift = *str++ + 7 - 'a';
    if (Mshift < 7 || Mshift > 31)
	return -2;
    if (Mshift >= bpp)
	return -3;
    /* no empty sets for now */
    if (*str == '\0')
	return -4;
    *pbpp = bpp;
    *pMshift = Mshift;
    return 0;
}

static inline
int decode_set_size(int len, int Mshift)
{
    int bitc = decode_base62_size(len - 2);
    return decode_golomb_size(bitc, Mshift);
}

static
int decode_set(const char *str, int Mshift, unsigned *v)
{
    const char *base62 = str + 2;
    /* separate base62+golomb stages, for reference */
    if (0) {
	/* base62 */
	int len = strlen(base62);
	char bitv[decode_base62_size(len)];
	int bitc = decode_base62(base62, bitv);
	if (bitc < 0)
	    return bitc;
	/* golomb */
	int c = decode_golomb(bitc, bitv, Mshift, v);
	if (c < 0)
	    return c;
	/* delta */
	decode_delta(c, v);
	return c;
    }
    /* combined base62+golomb stage */
    int c = decode_base62_golomb(base62, Mshift, v);
    if (c < 0)
	return c;
    /* delta */
    decode_delta(c, v);
    return c;
}

/* Special decode_set version with LRU caching. */
static
int cache_decode_set(const char *str, int Mshift, const unsigned **pv)
{
    struct cache_ent {
	char *str;
	int len;
	int c;
	unsigned v[];
    };
#define CACHE_SIZE 256
#define PIVOT_SIZE 243
    static int hc;
    static unsigned hv[CACHE_SIZE];
    static struct cache_ent *ev[CACHE_SIZE];
    /* look up in the cache */
    int i;
    unsigned *hp;
    struct cache_ent *ent;
    unsigned hash = str[0] | (str[2] << 8) | (str[3] << 16);
    for (hp = hv; hp < hv + hc; hp++) {
	if (hash == *hp) {
	    i = hp - hv;
	    ent = ev[i];
	    if (memcmp(str, ent->str, ent->len + 1) == 0) {
		/* hit, move to front */
		if (i) {
		    memmove(hv + 1, hv, i * sizeof(hv[0]));
		    memmove(ev + 1, ev, i * sizeof(ev[0]));
		    hv[0] = hash;
		    ev[0] = ent;
		}
		*pv = ent->v;
		return ent->c;
	    }
	}
    }
    /* decode */
    int len = strlen(str);
    int c = decode_set_size(len, Mshift);
#define SENTINELS 8
    ent = malloc(sizeof(*ent) + len + 1 + (c + SENTINELS) * sizeof(unsigned));
    assert(ent);
    c = ent->c = decode_set(str, Mshift, ent->v);
    if (c <= 0) {
	free(ent);
	return c;
    }
    for (i = 0; i < SENTINELS; i++)
	ent->v[c + i] = ~0u;
    ent->str = (char *)(ent->v + c + SENTINELS);
    memcpy(ent->str, str, len + 1);
    ent->len = len;
    /* insert */
    if (hc < CACHE_SIZE)
	i = hc++;
    else {
	/* free last entry */
	free(ev[CACHE_SIZE - 1]);
	/* position at midpoint */
	i = PIVOT_SIZE;
	memmove(hv + i + 1, hv + i, (CACHE_SIZE - i - 1) * sizeof(hv[0]));
	memmove(ev + i + 1, ev + i, (CACHE_SIZE - i - 1) * sizeof(ev[0]));
    }
    hv[i] = hash;
    ev[i] = ent;
    *pv = ent->v;
    return c;
}

/* Reduce a set of (bpp + 1) values to a set of bpp values. */
static
int downsample_set(int c, const unsigned *v, unsigned *w, int bpp)
{
    unsigned mask = (1 << bpp) - 1;
    /* find the first element with high bit set */
    int l = 0;
    int u = c;
    while (l < u) {
	int i = (l + u) / 2;
	if (v[i] <= mask)
	    l = i + 1;
	else
	    u = i;
    }
    /* initialize parts */
    const unsigned *w_start = w;
    const unsigned *v1 = v + 0, *v1end = v + u;
    const unsigned *v2 = v + u, *v2end = v + c;
    /* merge v1 and v2 into w */
    if (v1 < v1end && v2 < v2end) {
	unsigned v1val = *v1;
	unsigned v2val = *v2 & mask;
	while (1) {
	    if (v1val < v2val) {
		*w++ = v1val;
		v1++;
		if (v1 == v1end)
		    break;
		v1val = *v1;
	    }
	    else if (v2val < v1val) {
		*w++ = v2val;
		v2++;
		if (v2 == v2end)
		    break;
		v2val = *v2 & mask;
	    }
	    else {
		*w++ = v1val;
		v1++;
		v2++;
		if (v1 == v1end)
		    break;
		if (v2 == v2end)
		    break;
		v1val = *v1;
		v2val = *v2 & mask;
	    }
	}
    }
    /* append what's left */
    while (v1 < v1end)
	*w++ = *v1++;
    while (v2 < v2end)
	*w++ = *v2++ & mask;
    return w - w_start;
}

#ifdef SELF_TEST
static
void test_set(void)
{
    unsigned rnd_v[] = {
	0x020a, 0x07e5, 0x3305, 0x35f5,
	0x4980, 0x4c4f, 0x74ef, 0x7739,
	0x82ae, 0x8415, 0xa3e7, 0xb07e,
	0xb584, 0xb89f, 0xbb40, 0xf39e,
    };
    int rnd_c = sizeof rnd_v / sizeof *rnd_v;

    /* encode */
    int bpp = 16;
    char base62[encode_set_size(rnd_c, bpp)];
    int len = encode_set(rnd_c, rnd_v, bpp, base62);
    assert(len > 0);
    fprintf(stderr, "len=%d set=%s\n", len, base62);

    /* decode */
    int Mshift = bpp;
    int rc = decode_set_init(base62, &bpp, &Mshift);
    assert(rc == 0);
    assert(bpp == 16);
    assert(Mshift < bpp);
    int c = decode_set_size(len, Mshift);
    assert(c >= rnd_c);
    unsigned vbuf[c];
    const unsigned *v = vbuf;
    c = decode_set(base62, Mshift, vbuf);

    /* Decoded values must match. */
    assert(c == rnd_c);
    int i;
    for (i = 0; i < c; i++)
	assert(v[i] == rnd_v[i]);

    /* Cached version. */
    c = cache_decode_set(base62, Mshift, &v);
    assert(c == rnd_c);
    for (i = 0; i < c; i++)
	assert(v[i] == rnd_v[i]);
    fprintf(stderr, "%s: set test OK\n", __FILE__);
}
#endif

/*==============================================================*/

int rpmsetCmp(const char * str1, const char * str2)
{
    int rc = 0;

    if (strncmp(str1, "set:", 4) == 0)
	str1 += 4;
    if (strncmp(str2, "set:", 4) == 0)
	str2 += 4;

    /* initialize decoding */
    int bpp1, Mshift1;
    int bpp2, Mshift2;
    if (decode_set_init(str1, &bpp1, &Mshift1) < 0) {
	rc = -3;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }
    if (decode_set_init(str2, &bpp2, &Mshift2) < 0) {
	rc = -4;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }

    /* decode set1 (comes on behalf of provides) */
    const unsigned *v1 = NULL;
    int c1 = cache_decode_set(str1, Mshift1, &v1);
    if (c1 < 0) {
	rc = -3;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }
    unsigned v1bufA[c1 + 1];
    unsigned v1bufB[c1 + 1];

    /* decode set2 (on the stack) */
    int len2 = strlen(str2);
    int c2 = decode_set_size(len2, Mshift2);
    unsigned v2bufA[c2];
    unsigned v2bufB[c2];
    const unsigned *v2 = v2bufA;
    c2 = decode_set(str2, Mshift2, v2bufA);
    if (c2 < 0) {
	rc = -4;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }

    /* adjust for comparison */
    int i;
    while (bpp1 > bpp2) {
	unsigned *v1buf = v1bufA;
	if (v1 == v1buf)
	    v1buf = v1bufB;
	bpp1--;
	c1 = downsample_set(c1, v1, v1buf, bpp1);
	for (i = 0; i < SENTINELS; i++)
	    v1buf[c1 + i] = ~0u;
	v1 = v1buf;
    }
    while (bpp2 > bpp1) {
	unsigned *v2buf = v2bufA;
	if (v2 == v2buf)
	    v2buf = v2bufB;
	bpp2--;
	c2 = downsample_set(c2, v2, v2buf, bpp2);
	v2 = v2buf;
    }

    /* compare */
    int ge = 1;
    int le = 1;
    const unsigned *v1end = v1 + c1;
    const unsigned *v2end = v2 + c2;
    for (i = 0; i < SENTINELS; i++)
	assert(v1end[i] == ~0u);
    unsigned v2val = *v2;

    /* loop pieces */
#define IFLT4 \
    if (*v1 < v2val) { \
	le = 0; \
	v1 += 4; \
	while (*v1 < v2val) \
	    v1 += 4; \
	v1 -= 2; \
	if (*v1 < v2val) \
	    v1++; \
	else \
	    v1--; \
	if (*v1 < v2val) \
	    v1++; \
	if (v1 == v1end) \
	    break; \
    }
#define IFLT8 \
    if (*v1 < v2val) { \
	le = 0; \
	v1 += 8; \
	while (*v1 < v2val) \
	    v1 += 8; \
	v1 -= 4; \
	if (*v1 < v2val) \
	    v1 += 2; \
	else \
	    v1 -= 2; \
	if (*v1 < v2val) \
	    v1++; \
	else \
	    v1--; \
	if (*v1 < v2val) \
	    v1++; \
	if (v1 == v1end) \
	    break; \
    }
#define IFGE \
    if (*v1 == v2val) { \
	v1++; \
	v2++; \
	if (v1 == v1end) \
	    break; \
	if (v2 == v2end) \
	    break; \
	v2val = *v2; \
    } \
    else { \
	ge = 0; \
	v2++; \
	if (v2 == v2end) \
	    break; \
	v2val = *v2; \
    }

    /* choose the right stepper */
    if (c1 >= 16 * c2) {
	while (1) {
	    IFLT8;
	    IFGE;
	}
    }
    else {
	while (1) {
	    IFLT4;
	    IFGE;
	}
    }

    /* return */
    if (v1 < v1end)
	le = 0;
    if (v2 < v2end)
	ge = 0;
    if (le && ge) {
	rc = 0;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }
    if (ge) {
	rc = 1;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }
    if (le) {
	rc = -1;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
	return rc;
    }
    rc = -2;
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%s,%s) rc %d\n", __FUNCTION__, str1, str2, rc);
    return rc;
}

/*==============================================================*/

static void rpmsetFini(void * _set)
	/*@globals fileSystem @*/
	/*@modifies *_set, fileSystem @*/
{
    rpmset set = _set;

    if (set) {
	int i;
	for (i = 0; i < set->c; i++)
	    set->sv[i].s = _free(set->sv[i].s);
	set->sv = _free(set->sv);
    }
}

/*@unchecked@*/ /*@only@*/ /*@null@*/
rpmioPool _rpmsetPool = NULL;

static rpmset rpmsetGetPool(/*@null@*/ rpmioPool pool)
	/*@globals _rpmsetPool, fileSystem @*/
	/*@modifies pool, _rpmsetPool, fileSystem @*/
{
    rpmset set;

    if (_rpmsetPool == NULL) {
	_rpmsetPool = rpmioNewPool("set", sizeof(*set), -1, _rpmset_debug,
			NULL, NULL, rpmsetFini);
	pool = _rpmsetPool;
    }
    set = (rpmset) rpmioGetPool(pool, sizeof(*set));
    memset(((char *)set)+sizeof(set->_item), 0, sizeof(*set)-sizeof(set->_item));
    return set;
}

rpmset rpmsetNew(const char * fn, int flags)
{
    rpmset set = rpmsetGetPool(_rpmsetPool);

    return rpmsetLink(set);
}

void rpmsetAdd(rpmset set, const char * sym)
{
    const int delta = 1024;
    if ((set->c & (delta - 1)) == 0)
	set->sv = xrealloc(set->sv, sizeof(*set->sv) * (set->c + delta));
    set->sv[set->c].s = xstrdup(sym);
    set->sv[set->c].v = 0;
    set->c++;
}

    /* Jenkins' one-at-a-time hash */
    static
    unsigned int hash(const char *str)
    {
	unsigned int hash = 0x9e3779b9;
	const unsigned char *p = (const unsigned char *) str;
	while (*p) {
	    hash += *p++;
	    hash += (hash << 10);
	    hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
    }

    /* sort by hash value */
    static
    int cmp(const void *arg1, const void *arg2)
    {
	struct sv *sv1 = (struct sv *) arg1;
	struct sv *sv2 = (struct sv *) arg2;
	if (sv1->v > sv2->v)
	    return 1;
	if (sv2->v > sv1->v)
	    return -1;
	return 0;
    }

    static
    int uniqv(int c, unsigned *v)
    {
	int i, j;
	for (i = 0, j = 0; i < c; i++) {
	    while (i + 1 < c && v[i] == v[i+1])
		i++;
	    v[j++] = v[i];
	}
	return j;
    }

/* This routine does the whole job. */
const char * rpmsetFinish(rpmset set, int bpp)
{
    char * t = NULL;

    if (set->c < 1 || bpp < 10 || bpp > 32) {
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%p,%d) rc %s\n", __FUNCTION__, set, bpp, t);
    }

    unsigned mask = (bpp < 32) ? (1u << bpp) - 1 : ~0u;

    /* hash sv strings */
    int i;
    for (i = 0; i < set->c; i++)
	set->sv[i].v = hash(set->sv[i].s) & mask;

    qsort(set->sv, set->c, sizeof *set->sv, cmp);

    /* warn on hash collisions */
    for (i = 0; i < set->c - 1; i++) {
	if (set->sv[i].v != set->sv[i+1].v)
	    continue;
	if (strcmp(set->sv[i].s, set->sv[i+1].s) == 0)
	    continue;
	fprintf(stderr, "warning: hash collision: %s %s\n",
		set->sv[i].s, set->sv[i+1].s);
    }

    /* encode */
    unsigned v[set->c];
    for (i = 0; i < set->c; i++)
	v[i] = set->sv[i].v;
    int c = uniqv(set->c, v);
    char base62[encode_set_size(c, bpp)];
    int len = encode_set(c, v, bpp, base62);
    if (len >= 0)
	t = xstrdup(base62);
	
if (_rpmset_debug)
fprintf(stderr, "<-- %s(%p,%d) rc %s\n", __FUNCTION__, set, bpp, t);
    return t;
}

#ifdef SELF_TEST
static
void test_api(void)
{
    rpmset set1 = rpmsetNew(NULL, 0);
    rpmsetAdd(set1, "mama");
    rpmsetAdd(set1, "myla");
    rpmsetAdd(set1, "ramu");
    const char *str10 = rpmsetFinish(set1, 16);
    fprintf(stderr, "set10=%s\n", str10);

    int cmp;
    rpmset set2 = rpmsetNew(NULL, 0);
    rpmsetAdd(set2, "myla");
    rpmsetAdd(set2, "mama");
    const char *str20 = rpmsetFinish(set2, 16);
    fprintf(stderr, "set20=%s\n", str20);
    cmp = rpmsetCmp(str10, str20);
    assert(cmp == 1);

    rpmsetAdd(set2, "ramu");
    const char *str21 = rpmsetFinish(set2, 16);
    fprintf(stderr, "set21=%s\n", str21);
    cmp = rpmsetCmp(str10, str21);
    assert(cmp == 0);

    rpmsetAdd(set2, "baba");
    const char *str22 = rpmsetFinish(set2, 16);
    cmp = rpmsetCmp(str10, str22);
    assert(cmp == -1);

    rpmsetAdd(set1, "deda");
    const char *str11 = rpmsetFinish(set1, 16);
    cmp = rpmsetCmp(str11, str22);
    assert(cmp == -2);

    set1 = rpmsetFree(set1);
    set2 = rpmsetFree(set2);
    str10 = _free(str10);
    str11 = _free(str11);
    str20 = _free(str20);
    str21 = _free(str21);
    str22 = _free(str22);

    fprintf(stderr, "%s: api test OK\n", __FILE__);
}
#endif

#ifdef SELF_TEST
/*==============================================================*/
static struct poptOption rpmsetOptionsTable[] = {
 { "debug", 'd', POPT_ARG_VAL,	&_rpmset_debug, -1,		NULL, NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
    N_("Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, rpmsetOptionsTable);
    int rc = 0;

    test_base62();
    test_golomb();
    test_word_table();
    test_base62_golomb();
    test_delta();
    test_set();
    test_api();

    con = rpmioFini(con);
    return rc;
}
#endif
