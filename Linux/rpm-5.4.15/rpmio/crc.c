/** \ingroup signature
 * \file rpmio/crc.c
 */

#include "system.h"
#include "crc.h"
#include "debug.h"

/* ========================================================================= */
rpmuint32_t __crc32(rpmuint32_t crc, const rpmuint8_t * data, size_t size)
{
    static rpmuint32_t polynomial = 0xedb88320;    /* reflected 0x04c11db7 */
    static rpmuint32_t xorout = 0xffffffff;
    static rpmuint32_t table[256];
    static int oneshot = 0;

    crc ^= xorout;

    if (!oneshot) {
	/* generate the table of CRC remainders for all possible bytes */
	rpmuint32_t c;
	rpmuint32_t i, j;
	for (i = 0;  i < 256;  i++) {
	    c = i;
	    for (j = 0;  j < 8;  j++) {
		if (c & 1)
		    c = polynomial ^ (c >> 1);
		else
		    c = (c >> 1);
	    }
	    table[i] = c;
	}
	oneshot = 1;
    }
    if (data != NULL)
    while (size) {
	crc = table[(crc ^ *data) & 0xff] ^ (crc >> 8);
	size--;
	data++;
    }

    crc ^= xorout;

    return crc;

}

/*
 * Swiped from zlib, using rpmuint32_t rather than unsigned long computation.
 */
/*@unchecked@*/
static int gf2_dim32 = 32;

/**
 */
static rpmuint32_t gf2_matrix_times32(rpmuint32_t *mat, rpmuint32_t vec)
	/*@*/
{
    rpmuint32_t sum;

    sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}

/**
 */
static void gf2_matrix_square32(/*@out@*/ rpmuint32_t *square, rpmuint32_t *mat)
	/*@modifies square @*/
{
    int n;

    for (n = 0; n < gf2_dim32; n++)
        square[n] = gf2_matrix_times32(mat, mat[n]);
}

rpmuint32_t __crc32_combine(rpmuint32_t crc1, rpmuint32_t crc2, size_t len2)
{
    int n;
    rpmuint32_t row;
    size_t nb = gf2_dim32 * sizeof(row);
    rpmuint32_t * even = (rpmuint32_t *) alloca(nb);	/* even-power-of-two zeros operator */
    rpmuint32_t * odd = (rpmuint32_t *) alloca(nb);	/* odd-power-of-two zeros operator */

    /* degenerate case */
    if (len2 == 0)
        return crc1;

    /* put operator for one zero bit in odd */
    odd[0] = 0xedb88320UL;	/* CRC-32 polynomial */
    row = 1;
    for (n = 1; n < gf2_dim32; n++) {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square32(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square32(odd, even);

    /* apply len2 zeros to crc1 (first square will put the operator for one
       zero byte, eight zero bits, in even) */
    do {
        /* apply zeros operator for this bit of len2 */
        gf2_matrix_square32(even, odd);
        if (len2 & 1)
            crc1 = gf2_matrix_times32(even, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
        if (len2 == 0)
            break;

        /* another iteration of the loop with odd and even swapped */
        gf2_matrix_square32(odd, even);
        if (len2 & 1)
            crc1 = gf2_matrix_times32(odd, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
    } while (len2 != 0);

    /* return combined crc */
    crc1 ^= crc2;
    return crc1;
}

/* ========================================================================= */
/*
 * ECMA-182 polynomial, see
 *     http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-182.pdf
 */
rpmuint64_t __crc64(rpmuint64_t crc, const rpmuint8_t * data, size_t size)
{
    static rpmuint64_t polynomial =
	0xc96c5795d7870f42ULL;	/* reflected 0x42f0e1eba9ea3693ULL */
    static rpmuint64_t xorout = 0xffffffffffffffffULL;
    static rpmuint64_t table[256];
    static int oneshot = 0;

    crc ^= xorout;

    if (!oneshot) {
	/* generate the table of CRC remainders for all possible bytes */
	rpmuint64_t c;
	rpmuint32_t i, j;
	for (i = 0;  i < 256;  i++) {
	    c = i;
	    for (j = 0;  j < 8;  j++) {
		if (c & 1)
		    c = polynomial ^ (c >> 1);
		else
		    c = (c >> 1);
	    }
	    table[i] = c;
	}
	oneshot = 1;
    }
    if (data != NULL)
    while (size) {
	crc = table[(crc ^ *data) & 0xff] ^ (crc >> 8);
	size--;
	data++;
    }

    crc ^= xorout;

    return crc;

}

/*
 * Swiped from zlib, using rpmuint64_t rather than unsigned long computation.
 */
/*@unchecked@*/
static int gf2_dim64 = 64;

/**
 */
static rpmuint64_t gf2_matrix_times64(rpmuint64_t *mat, rpmuint64_t vec)
	/*@*/
{
    rpmuint64_t sum;

    sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}

/**
 */
static void gf2_matrix_square64(/*@out@*/ rpmuint64_t *square, rpmuint64_t *mat)
	/*@modifies square @*/
{
    int n;

    for (n = 0; n < gf2_dim64; n++)
        square[n] = gf2_matrix_times64(mat, mat[n]);
}

rpmuint64_t __crc64_combine(rpmuint64_t crc1, rpmuint64_t crc2, size_t len2)
{
    int n;
    rpmuint64_t row;
    size_t nb = gf2_dim64 * sizeof(row);
    rpmuint64_t * even = (rpmuint64_t *) alloca(nb);	/* even-power-of-two zeros operator */
    rpmuint64_t * odd = (rpmuint64_t *) alloca(nb);	/* odd-power-of-two zeros operator */

    /* degenerate case */
    if (len2 == 0)
        return crc1;

    /* put operator for one zero bit in odd */
    odd[0] = 0xc96c5795d7870f42ULL;	/* reflected 0x42f0e1eba9ea3693ULL */
    row = 1;
    for (n = 1; n < gf2_dim64; n++) {
        odd[n] = row;
        row <<= 1;
    }

    /* put operator for two zero bits in even */
    gf2_matrix_square64(even, odd);

    /* put operator for four zero bits in odd */
    gf2_matrix_square64(odd, even);

    /* apply len2 zeros to crc1 (first square will put the operator for one
       zero byte, eight zero bits, in even) */
    do {
        /* apply zeros operator for this bit of len2 */
        gf2_matrix_square64(even, odd);
        if (len2 & 1)
            crc1 = gf2_matrix_times64(even, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
        if (len2 == 0)
            break;

        /* another iteration of the loop with odd and even swapped */
        gf2_matrix_square64(odd, even);
        if (len2 & 1)
            crc1 = gf2_matrix_times64(odd, crc1);
        len2 >>= 1;

        /* if no more bits set, then done */
    } while (len2 != 0);

    /* return combined crc */
    crc1 ^= crc2;
    return crc1;
}

/* ========================================================================= */
/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2004 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define BASE 65521UL    /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (rpmuint32_t) (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

/* use NO_DIVIDE if your processor does not do division in hardware */
#ifdef NO_DIVIDE
#  define MOD(a) \
    do { \
        if (a >= (BASE << 16)) a -= (BASE << 16); \
        if (a >= (BASE << 15)) a -= (BASE << 15); \
        if (a >= (BASE << 14)) a -= (BASE << 14); \
        if (a >= (BASE << 13)) a -= (BASE << 13); \
        if (a >= (BASE << 12)) a -= (BASE << 12); \
        if (a >= (BASE << 11)) a -= (BASE << 11); \
        if (a >= (BASE << 10)) a -= (BASE << 10); \
        if (a >= (BASE << 9)) a -= (BASE << 9); \
        if (a >= (BASE << 8)) a -= (BASE << 8); \
        if (a >= (BASE << 7)) a -= (BASE << 7); \
        if (a >= (BASE << 6)) a -= (BASE << 6); \
        if (a >= (BASE << 5)) a -= (BASE << 5); \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#  define MOD4(a) \
    do { \
        if (a >= (BASE << 4)) a -= (BASE << 4); \
        if (a >= (BASE << 3)) a -= (BASE << 3); \
        if (a >= (BASE << 2)) a -= (BASE << 2); \
        if (a >= (BASE << 1)) a -= (BASE << 1); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD4(a) a %= BASE
#endif

rpmuint32_t __adler32(rpmuint32_t adler, const rpmuint8_t * buf, rpmuint32_t len)
{
    rpmuint32_t sum2;
    unsigned n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += (rpmuint32_t) buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == NULL)
        return 1UL;

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len--) {
            adler += (rpmuint32_t) *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD4(sum2);             /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len--) {
            adler += (rpmuint32_t) *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}

rpmuint32_t __adler32_combine(rpmuint32_t adler1, rpmuint32_t adler2, size_t len2)
	/*@*/
{
    rpmuint32_t sum1;
    rpmuint32_t sum2;
    unsigned rem;

    /* the derivation of this formula is left as an exercise for the reader */
    rem = (unsigned)(len2 % BASE);
    sum1 = adler1 & 0xffff;
    sum2 = rem * sum1;
    MOD(sum2);
    sum1 += (adler2 & 0xffff) + BASE - 1;
    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
    if (sum1 > BASE) sum1 -= BASE;
    if (sum1 > BASE) sum1 -= BASE;
    if (sum2 > (BASE << 1)) sum2 -= (BASE << 1);
    if (sum2 > BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}

int sum32Reset(register sum32Param * mp)
{
    if (mp->update)
	mp->crc = (*mp->update) (0, NULL, 0);
    return 0;
}

int sum32Update(sum32Param * mp, const rpmuint8_t * data, size_t size)
{
    if (mp->update)
	mp->crc = (*mp->update) (mp->crc, data, size);
    return 0;
}

int sum32Digest(sum32Param * mp, rpmuint8_t * data)
{
	rpmuint32_t c = mp->crc;

	data[ 0] = (rpmuint8_t)(c >> 24);
	data[ 1] = (rpmuint8_t)(c >> 16);
	data[ 2] = (rpmuint8_t)(c >>  8);
	data[ 3] = (rpmuint8_t)(c      );

	(void) sum32Reset(mp);

	return 0;
}

int sum64Reset(register sum64Param * mp)
{
    if (mp->update)
	mp->crc = (*mp->update) (0, NULL, 0);
    return 0;
}

int sum64Update(sum64Param * mp, const rpmuint8_t * data, size_t size)
{
    if (mp->update)
	mp->crc = (*mp->update) (mp->crc, data, size);
    return 0;
}

int sum64Digest(sum64Param * mp, rpmuint8_t * data)
{
	rpmuint64_t c = mp->crc;

	data[ 0] = (rpmuint8_t)(c >> 56);
	data[ 1] = (rpmuint8_t)(c >> 48);
	data[ 2] = (rpmuint8_t)(c >> 40);
	data[ 3] = (rpmuint8_t)(c >> 32);
	data[ 4] = (rpmuint8_t)(c >> 24);
	data[ 5] = (rpmuint8_t)(c >> 16);
	data[ 6] = (rpmuint8_t)(c >>  8);
	data[ 7] = (rpmuint8_t)(c      );

	(void) sum64Reset(mp);

	return 0;
}
