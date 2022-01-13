/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Matt Bishop of Dartmouth College.
 *
 * The United States Government has rights in this work pursuant
 * to contract no. NAG 2-680 between the National Aeronautics and
 * Space Administration and Dartmouth College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * BDES -- DES encryption package for Berkeley Software Distribution 4.4
 * options:
 *	-a	key is in ASCII
 *	-b	use ECB (electronic code book) mode
 *	-d	invert (decrypt) input
 *	-f b	use b-bit CFB (cipher feedback) mode
 *	-F b	use b-bit CFB (cipher feedback) alternative mode
 *	-k key	use key as the cryptographic key
 *	-m b	generate a MAC of length b
 *	-o b	use b-bit OFB (output feedback) mode
 *	-p	don't reset the parity bit
 *	-v v	use v as the initialization vector (ignored for ECB)
 * note: the last character of the last block is the integer indicating
 * how many characters of that block are to be output
 *
 * Author: Matt Bishop
 *	   Department of Mathematics and Computer Science
 *	   Dartmouth College
 *	   Hanover, NH  03755
 * Email:  Matt.Bishop@dartmouth.edu
 *	   ...!decvax!dartvax!Matt.Bishop
 *
 * See Technical Report PCS-TR91-158, Department of Mathematics and Computer
 * Science, Dartmouth College, for a detailed description of the implemen-
 * tation and differences between it and Sun's.  The DES is described in
 * FIPS PUB 46, and the modes in FIPS PUB 81 (see either the manual page
 * or the technical report for a complete reference).
 */

#include "system.h"

#include <sys/cdefs.h>
#include <err.h>

#include <openssl/des.h>

#include <rpmio.h>
#include <poptIO.h>

#include "debug.h"

int _bdes_debug = 0;

#define	DES_XFORM(buf)							\
		DES_ecb_encrypt(buf, buf, &schedule, 			\
		    mode == MODE_ENCRYPT ? DES_ENCRYPT : DES_DECRYPT);

/*
 * this does an error-checking write
 */
FD_t ifd;
FILE * ifp;
FD_t ofd;
FILE * ofp;
#define	READ(buf, n)	fread(buf, sizeof(char), n, ifp)
#define WRITE(buf,n)						\
		if ((nw = fwrite(buf, sizeof(char), n, ofp)) != (size_t)n)	\
			warnx("fwrite error at %d", n);

/*
 * global variables and related macros
 */
#define KEY_DEFAULT		0	/* interpret radix of key from key */
#define KEY_ASCII		1	/* key is in ASCII characters */
int keybase = KEY_DEFAULT;	/* how to interpret the key */

enum {				/* encrypt, decrypt, authenticate */
    MODE_ENCRYPT, MODE_DECRYPT, MODE_AUTHENTICATE
} mode = MODE_ENCRYPT;

enum {				/* ecb, cbc, cfb, cfba, ofb? */
    ALG_ECB, ALG_CBC, ALG_CFB, ALG_OFB, ALG_CFBA
} alg = ALG_CBC;

DES_cblock ivec;		/* initialization vector */

char bits[] = {			/* used to extract bits from a char */
    '\200', '\100', '\040', '\020', '\010', '\004', '\002', '\001'
};

int inverse;			/* 0 to encrypt, 1 to decrypt */
int macbits = -1;		/* number of bits in authentication */
int fbbits = -1;		/* number of feedback bits */
int pflag;			/* 1 to preserve parity bits */

DES_key_schedule schedule;	/* expanded DES key */

/*
 * map a hex character to an integer
 */
static int tobinhex(char c, int radix)
{
    switch (c) {
    case '0':
	return (0x0);
    case '1':
	return (0x1);
    case '2':
	return (radix > 2 ? 0x2 : -1);
    case '3':
	return (radix > 3 ? 0x3 : -1);
    case '4':
	return (radix > 4 ? 0x4 : -1);
    case '5':
	return (radix > 5 ? 0x5 : -1);
    case '6':
	return (radix > 6 ? 0x6 : -1);
    case '7':
	return (radix > 7 ? 0x7 : -1);
    case '8':
	return (radix > 8 ? 0x8 : -1);
    case '9':
	return (radix > 9 ? 0x9 : -1);
    case 'A':
    case 'a':
	return (radix > 10 ? 0xa : -1);
    case 'B':
    case 'b':
	return (radix > 11 ? 0xb : -1);
    case 'C':
    case 'c':
	return (radix > 12 ? 0xc : -1);
    case 'D':
    case 'd':
	return (radix > 13 ? 0xd : -1);
    case 'E':
    case 'e':
	return (radix > 14 ? 0xe : -1);
    case 'F':
    case 'f':
	return (radix > 15 ? 0xf : -1);
    }
    /*
     * invalid character
     */
    return (-1);
}

/*
 * convert the key to a bit pattern
 */
static void cvtkey(DES_cblock obuf, char *ibuf)
{
    int i, j;			/* counter in a for loop */
    int nbuf[64];		/* used for hex/key translation */

    /*
     * just switch on the key base
     */
    switch (keybase) {
    case KEY_ASCII:		/* ascii to integer */
	memmove(obuf, ibuf, 8);
	return;
    case KEY_DEFAULT:		/* tell from context */
	/*
	 * leading '0x' or '0X' == hex key
	 */
	if (ibuf[0] == '0' && (ibuf[1] == 'x' || ibuf[1] == 'X')) {
	    ibuf = &ibuf[2];
	    /*
	     * now translate it, bombing on any illegal hex digit
	     */
	    for (i = 0; ibuf[i] && i < 16; i++)
		if ((nbuf[i] = tobinhex(ibuf[i], 16)) == -1)
		    warnx("bad hex digit in key");
	    while (i < 16)
		nbuf[i++] = 0;
	    for (i = 0; i < 8; i++)
		obuf[i] =
		    ((nbuf[2 * i] & 0xf) << 4) | (nbuf[2 * i + 1] & 0xf);
	    /* preserve parity bits */
	    pflag = 1;
	    return;
	}
	/*
	 * leading '0b' or '0B' == binary key
	 */
	if (ibuf[0] == '0' && (ibuf[1] == 'b' || ibuf[1] == 'B')) {
	    ibuf = &ibuf[2];
	    /*
	     * now translate it, bombing on any illegal binary digit
	     */
	    for (i = 0; ibuf[i] && i < 16; i++)
		if ((nbuf[i] = tobinhex(ibuf[i], 2)) == -1)
		    warnx("bad binary digit in key");
	    while (i < 64)
		nbuf[i++] = 0;
	    for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		    obuf[i] = (obuf[i] << 1) | nbuf[8 * i + j];
	    /* preserve parity bits */
	    pflag = 1;
	    return;
	}
	/*
	 * no special leader -- ASCII
	 */
	memmove(obuf, ibuf, 8);
    }
}

/*
 * convert an ASCII string into a decimal number:
 * 1. must be between 0 and 64 inclusive
 * 2. must be a valid decimal number
 * 3. must be a multiple of mult
 */
static int setbits(char *s, int mult)
{
    char *p;			/* pointer in a for loop */
    int n = 0;			/* the integer collected */

    /*
     * skip white space
     */
    while (isspace(*s))
	s++;
    /*
     * get the integer
     */
    for (p = s; *p; p++) {
	if (isdigit(*p))
	    n = n * 10 + *p - '0';
	else {
	    warnx("bad decimal digit in MAC length");
	}
    }
    /*
     * be sure it's a multiple of mult
     */
    return ((n % mult != 0) ? -1 : n);
}

/*****************
 * DES FUNCTIONS *
 *****************/
/*
 * This sets the DES key and (if you're using the deszip version)
 * the direction of the transformation.  This uses the Sun
 * to map the 64-bit key onto the 56 bits that the key schedule
 * generation routines use: the old way, which just uses the user-
 * supplied 64 bits as is, and the new way, which resets the parity
 * bit to be the same as the low-order bit in each character.  The
 * new way generates a greater variety of key schedules, since many
 * systems set the parity (high) bit of each character to 0, and the
 * DES ignores the low order bit of each character.
 */
static void makekey(DES_cblock * buf)
{
    int i, j;			/* counter in a for loop */
    int par;			/* parity counter */

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, buf);
    /*
     * if the parity is not preserved, flip it
     */
    if (!pflag) {
	for (i = 0; i < 8; i++) {
	    par = 0;
	    for (j = 1; j < 8; j++)
		if ((bits[j] & (*buf)[i]) != 0)
		    par++;
	    if ((par & 0x01) == 0x01)
		(*buf)[i] &= 0x7f;
	    else
		(*buf)[i] = ((*buf)[i] & 0x7f) | 0x80;
	}
    }

    DES_set_odd_parity(buf);
    DES_set_key(buf, &schedule);
}

/*
 * This encrypts using the Electronic Code Book mode of DES
 */
static void ecbenc(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int bn;			/* block number */
    DES_cblock msgbuf;		/* I/O buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    for (bn = 0; (n = READ(msgbuf, 8)) == 8; bn++) {
	/*
	 * do the transformation
	 */
	DES_XFORM(&msgbuf);
	WRITE(&msgbuf, 8);
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    bn++;
    memset(&msgbuf[n], 0, 8 - n);
    msgbuf[7] = n;
    DES_XFORM(&msgbuf);
    WRITE(&msgbuf, 8);

}

/*
 * This decrypts using the Electronic Code Book mode of DES
 */
static void ecbdec(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int bn;			/* block number */
    int c;			/* used to test for EOF */
    DES_cblock msgbuf;		/* I/O buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    for (bn = 1; (n = READ(msgbuf, 8)) == 8; bn++) {
	/*
	 * do the transformation
	 */
	DES_XFORM(&msgbuf);
	/*
	 * if the last one, handle it specially
	 */
	if ((c = fgetc(ifp)) == EOF) {
	    n = msgbuf[7];
	    if (n < 0 || n > 7)
		warnx("decryption failed (block corrupt) at %d", bn);
	} else
	    (void) ungetc(c, ifp);
	WRITE(msgbuf, n);
    }
    if (n > 0)
	warnx("decryption failed (incomplete block) at %d", bn);
}

/*
 * This encrypts using the Cipher Block Chaining mode of DES
 */
static void cbcenc(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int bn;			/* block number */
    DES_cblock msgbuf;		/* I/O buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(msgbuf, 8)) == 8; bn++) {
	for (n = 0; n < 8; n++)
	    msgbuf[n] ^= ivec[n];
	DES_XFORM(&msgbuf);
	memcpy(ivec, msgbuf, 8);
	WRITE(msgbuf, 8);
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    bn++;
    memset(&msgbuf[n], 0, 8 - n);
    msgbuf[7] = n;
    for (n = 0; n < 8; n++)
	msgbuf[n] ^= ivec[n];
    DES_XFORM(&msgbuf);
    WRITE(msgbuf, 8);

}

/*
 * This decrypts using the Cipher Block Chaining mode of DES
 */
static void cbcdec(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    DES_cblock msgbuf;		/* I/O buffer */
    DES_cblock ibuf;		/* temp buffer for initialization vector */
    int c;			/* used to test for EOF */
    int bn;			/* block number */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    for (bn = 0; (n = READ(msgbuf, 8)) == 8; bn++) {
	/*
	 * do the transformation
	 */
	memcpy(ibuf, msgbuf, 8);
	DES_XFORM(&msgbuf);
	for (c = 0; c < 8; c++)
	    msgbuf[c] ^= ivec[c];
	memcpy(ivec, ibuf, 8);
	/*
	 * if the last one, handle it specially
	 */
	if ((c = fgetc(ifp)) == EOF) {
	    n = msgbuf[7];
	    if (n < 0 || n > 7)
		warnx("decryption failed (block corrupt) at %d", bn);
	} else
	    (void) ungetc(c, ifp);
	WRITE(msgbuf, n);
    }
    if (n > 0)
	warnx("decryption failed (incomplete block) at %d", bn);
}

/*
 * This authenticates using the Cipher Block Chaining mode of DES
 */
static void cbcauth(CIPHER_CTX cph)
{
    int n, j;			/* number of bytes actually read */
    DES_cblock msgbuf;		/* I/O buffer */
    DES_cblock encbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do the transformation
     * note we DISCARD the encrypted block;
     * we only care about the last one
     */
    while ((n = READ(msgbuf, 8)) == 8) {
	for (n = 0; n < 8; n++)
	    encbuf[n] = msgbuf[n] ^ ivec[n];
	DES_XFORM(&encbuf);
	memcpy(ivec, encbuf, 8);
    }
    /*
     * now compute the last one, right padding with '\0' if need be
     */
    if (n > 0) {
	memset(&msgbuf[n], 0, 8 - n);
	for (n = 0; n < 8; n++)
	    encbuf[n] = msgbuf[n] ^ ivec[n];
	DES_XFORM(&encbuf);
    }
    /*
     * drop the bits
     * we write chars until fewer than 7 bits,
     * and then pad the last one with 0 bits
     */
    for (n = 0; macbits > 7; n++, macbits -= 8)
	nw = fwrite(&encbuf[n], 1, 1, ofp);
    if (macbits > 0) {
	msgbuf[0] = 0x00;
	for (j = 0; j < macbits; j++)
	    msgbuf[0] |= encbuf[n] & bits[j];
	nw = fwrite(&msgbuf[0], 1, 1, ofp);
    }
}

/*
 * This encrypts using the Cipher FeedBack mode of DES
 */
static void cfbenc(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 8;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (n = 0; n < 8 - nbytes; n++)
	    ivec[n] = ivec[n + nbytes];
	for (n = 0; n < nbytes; n++)
	    ivec[8 - nbytes + n] = ibuf[n] ^ msgbuf[n];
	WRITE(&ivec[8 - nbytes], nbytes);
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    bn++;
    memset(&ibuf[n], 0, nbytes - n);
    ibuf[nbytes - 1] = n;
    memcpy(msgbuf, ivec, 8);
    DES_XFORM(&msgbuf);
    for (n = 0; n < nbytes; n++)
	ibuf[n] ^= msgbuf[n];
    WRITE(ibuf, nbytes);
}

/*
 * This decrypts using the Cipher Block Chaining mode of DES
 */
static void cfbdec(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int c;			/* used to test for EOF */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    char obuf[8];		/* output buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 8;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (c = 0; c < 8 - nbytes; c++)
	    ivec[c] = ivec[c + nbytes];
	for (c = 0; c < nbytes; c++) {
	    ivec[8 - nbytes + c] = ibuf[c];
	    obuf[c] = ibuf[c] ^ msgbuf[c];
	}
	/*
	 * if the last one, handle it specially
	 */
	if ((c = fgetc(ifp)) == EOF) {
	    n = obuf[nbytes - 1];
	    if (n < 0 || n > nbytes - 1)
		warnx("decryption failed (block corrupt) at %d", bn);
	} else
	    (void) ungetc(c, ifp);
	WRITE(obuf, n);
    }
    if (n > 0)
	warnx("decryption failed (incomplete block) at %d", bn);
}

/*
 * This encrypts using the alternative Cipher FeedBack mode of DES
 */
static void cfbaenc(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    char obuf[8];		/* output buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 7;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (n = 0; n < 8 - nbytes; n++)
	    ivec[n] = ivec[n + nbytes];
	for (n = 0; n < nbytes; n++)
	    ivec[8 - nbytes + n] = (ibuf[n] ^ msgbuf[n]) | 0x80;
	for (n = 0; n < nbytes; n++)
	    obuf[n] = ivec[8 - nbytes + n] & 0x7f;
	WRITE(obuf, nbytes);
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    bn++;
    memset(&ibuf[n], 0, nbytes - n);
    ibuf[nbytes - 1] = ('0' + n) | 0200;
    memcpy(msgbuf, ivec, 8);
    DES_XFORM(&msgbuf);
    for (n = 0; n < nbytes; n++)
	ibuf[n] ^= msgbuf[n];
    WRITE(ibuf, nbytes);
}

/*
 * This decrypts using the alternative Cipher Block Chaining mode of DES
 */
static void cfbadec(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int c;			/* used to test for EOF */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    char obuf[8];		/* output buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 7;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (c = 0; c < 8 - nbytes; c++)
	    ivec[c] = ivec[c + nbytes];
	for (c = 0; c < nbytes; c++) {
	    ivec[8 - nbytes + c] = ibuf[c] | 0x80;
	    obuf[c] = (ibuf[c] ^ msgbuf[c]) & 0x7f;
	}
	/*
	 * if the last one, handle it specially
	 */
	if ((c = fgetc(ifp)) == EOF) {
	    if ((n = (obuf[nbytes - 1] - '0')) < 0 || n > nbytes - 1)
		warnx("decryption failed (block corrupt) at %d", bn);
	} else
	    (void) ungetc(c, ifp);
	WRITE(obuf, n);
    }
    if (n > 0)
	warnx("decryption failed (incomplete block) at %d", bn);
}


/*
 * This encrypts using the Output FeedBack mode of DES
 */
static void ofbenc(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int c;			/* used to test for EOF */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    char obuf[8];		/* output buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 8;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (n = 0; n < 8 - nbytes; n++)
	    ivec[n] = ivec[n + nbytes];
	for (n = 0; n < nbytes; n++) {
	    ivec[8 - nbytes + n] = msgbuf[n];
	    obuf[n] = ibuf[n] ^ msgbuf[n];
	}
	WRITE(obuf, nbytes);
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    bn++;
    memset(&ibuf[n], 0, nbytes - n);
    ibuf[nbytes - 1] = n;
    memcpy(msgbuf, ivec, 8);
    DES_XFORM(&msgbuf);
    for (c = 0; c < nbytes; c++)
	ibuf[c] ^= msgbuf[c];
    WRITE(ibuf, nbytes);
}

/*
 * This decrypts using the Output Block Chaining mode of DES
 */
static void ofbdec(CIPHER_CTX cph)
{
    int n;			/* number of bytes actually read */
    int c;			/* used to test for EOF */
    int nbytes;			/* number of bytes to read */
    int bn;			/* block number */
    char ibuf[8];		/* input buffer */
    char obuf[8];		/* output buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 8;
    /*
     * do the transformation
     */
    for (bn = 1; (n = READ(ibuf, nbytes)) == nbytes; bn++) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (c = 0; c < 8 - nbytes; c++)
	    ivec[c] = ivec[c + nbytes];
	for (c = 0; c < nbytes; c++) {
	    ivec[8 - nbytes + c] = msgbuf[c];
	    obuf[c] = ibuf[c] ^ msgbuf[c];
	}
	/*
	 * if the last one, handle it specially
	 */
	if ((c = fgetc(ifp)) == EOF) {
	    n = obuf[nbytes - 1];
	    if (n < 0 || n > nbytes - 1)
		warnx("decryption failed (block corrupt) at %d", bn);
	} else
	    (void) ungetc(c, ifp);
	/*
	 * dump it
	 */
	WRITE(obuf, n);
    }
    if (n > 0)
	warnx("decryption failed (incomplete block) at %d", bn);
}

/*
 * This authenticates using the Cipher FeedBack mode of DES
 */
static void cfbauth(CIPHER_CTX cph)
{
    int n, j;			/* number of bytes actually read */
    int nbytes;			/* number of bytes to read */
    char ibuf[8];		/* input buffer */
    DES_cblock msgbuf;		/* encryption buffer */
    size_t nw;

if (_bdes_debug)
fprintf(stderr, "--> %s(%p)\n", __FUNCTION__, cph);
    /*
     * do things in bytes, not bits
     */
    nbytes = fbbits / 8;
    /*
     * do the transformation
     */
    while ((n = READ(ibuf, nbytes)) == nbytes) {
	memcpy(msgbuf, ivec, 8);
	DES_XFORM(&msgbuf);
	for (n = 0; n < 8 - nbytes; n++)
	    ivec[n] = ivec[n + nbytes];
	for (n = 0; n < nbytes; n++)
	    ivec[8 - nbytes + n] = ibuf[n] ^ msgbuf[n];
    }
    /*
     * at EOF or last block -- in either case, the last byte contains
     * the character representation of the number of bytes in it
     */
    memset(&ibuf[n], 0, nbytes - n);
    ibuf[nbytes - 1] = '0' + n;
    memcpy(msgbuf, ivec, 8);
    DES_XFORM(&msgbuf);
    for (n = 0; n < nbytes; n++)
	ibuf[n] ^= msgbuf[n];
    /*
     * drop the bits
     * we write chars until fewer than 7 bits,
     * and then pad the last one with 0 bits
     */
    for (n = 0; macbits > 7; n++, macbits -= 8)
	nw = fwrite(&msgbuf[n], 1, 1, ofp);
    if (macbits > 0) {
	msgbuf[0] = 0x00;
	for (j = 0; j < macbits; j++)
	    msgbuf[0] |= msgbuf[n] & bits[j];
	nw = fwrite(&msgbuf[0], 1, 1, ofp);
    }
}

/*==============================================================*/
static char * bits_cfba;
static char * bits_cfb;
static char * bits_ofb;
static char * ivec_str;
static char * key_str;
static char * mac_str;

static struct poptOption bdesOptionsTable[] = {
 { "encrypt", 'e', POPT_ARG_VAL,	&mode,		MODE_ENCRYPT,
        N_("Encrypt the input message."), NULL },
 { "decrypt", 'd', POPT_ARG_VAL,	&mode,		MODE_DECRYPT,
        N_("Decrypt the input message."), NULL },
 { "authenticate", 'A', POPT_ARG_VAL,	&mode,		MODE_AUTHENTICATE,
        N_("Generate MAC for the input message."), NULL },

 { "ecb", 'b', POPT_ARG_VAL,		&alg,		ALG_ECB,
        N_("Use ECB mode."), NULL },
 { "cbc", 'c', POPT_ARG_VAL,		&alg,		ALG_CBC,
        N_("Use CBC mode."), NULL },
 { "cfba", 'F', POPT_ARG_STRING,	&bits_cfba,	0,
        N_(" Use N-bit alternative CFBA mode."), N_("N[7,56,7]") },
 { "cfb", 'f', POPT_ARG_STRING,		&bits_cfb,	0,
        N_("Use N-bit CFB mode."), N_("N[8,64,8]") },
 { "ofb", 'o', POPT_ARG_STRING,		&bits_ofb,	0,
        N_("Use N-bit OFB mode."), N_("N[8,64,8]") },

 { "ascii", 'a', POPT_ARG_VAL,		&keybase,	KEY_ASCII,
        N_("Key and iv strings are in ASCII"), NULL },
 { "parity", 'p', POPT_ARG_VAL,		&pflag,		1,
        N_("Disable resetting parity."), NULL },
 { "key", 'k', POPT_ARG_STRING,		&key_str,	0,
        N_("Use <key> as cryptographic key."), N_("<key>") },
 { "mac", 'm', POPT_ARG_STRING,		&mac_str,	0,
        N_("Compute message authentication code (MAC)"), N_("N[1,64]") },
	/* XXX -v collides with --verbose */
 { "iv", 'v', POPT_ARG_STRING,		&ivec_str,	0,
        N_("Set the initialization <vector>."), N_("<vector>") },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
        N_("Common options for all rpmio executables:"),
        NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    poptContext con;
    CIPHER_CTX cph = NULL;
    DES_cblock msgbuf;		/* I/O buffer */
    int ec = -1;	/* assume error */
    int xx;

    setproctitle("-");		/* Hide command-line arguments */
    ifd = Fdopen(fdDup(STDIN_FILENO), "rb.fpio");
    ofd = Fdopen(fdDup(STDOUT_FILENO), "wb.fpio");
    ifp = stdin;
    ofp = stdout;

    /* initialize the initialization vector */
    memset(ivec, 0, 8);

    /* parse options */
    con = rpmioInit(argc, argv, bdesOptionsTable);
	/* XXX check for args? */

    if (bits_cfba) {	/* use alternative CFB mode */
	alg = ALG_CFBA;
	if ((fbbits = setbits(bits_cfba, 7)) > 56 || fbbits == 0)
	    errx(1, "-F: number must be 1-56 inclusive");
	else if (fbbits == -1)
	    errx(1, "-F: number must be a multiple of 7");
	bits_cfba = _free(bits_cfba);
    } else
    if (bits_cfb) {	/* use CFB mode */
	alg = ALG_CFB;
	if ((fbbits = setbits(bits_cfb, 8)) > 64 || fbbits == 0)
	    errx(1, "-f: number must be 1-64 inclusive");
	else if (fbbits == -1)
	    errx(1, "-f: number must be a multiple of 8");
	bits_cfb = _free(bits_cfb);
    } else
    if (bits_ofb) {	/* use OFB mode */
	alg = ALG_OFB;
	if ((fbbits = setbits(bits_ofb, 8)) > 64 || fbbits == 0)
	    errx(1, "-o: number must be 1-64 inclusive");
	else if (fbbits == -1)
	    errx(1, "-o: number must be a multiple of 8");
	bits_ofb = _free(bits_ofb);
    }

    if (mac_str) {
	mode = MODE_AUTHENTICATE;
	if ((macbits = setbits(mac_str, 1)) > 64)
	    errx(1, "-m: number must be 0-64 inclusive");
	mac_str = _free(mac_str);
    }

    inverse = (alg == ALG_CBC || alg == ALG_ECB) && mode == MODE_DECRYPT;

    if (ivec_str) {
	cvtkey(ivec, ivec_str);
	ivec_str = _free(ivec_str);
    }

    if (key_str == NULL) {
	/* if the key's not ASCII, assume it is */
	keybase = KEY_ASCII;
	/* get the key */
	key_str = getpass("Enter key: ");
	key_str = xstrdup(key_str);
    }

    /* copy key_str, nul-padded, into the key area */
    cvtkey(msgbuf, key_str);
    makekey(&msgbuf);

    cph = rpmCipherInit(PGPSYMKEYALGO_DES, 0);

    switch (alg) {
    case ALG_CBC:
	switch (mode) {
	case MODE_AUTHENTICATE:	/* authenticate using CBC mode */
	    cbcauth(cph);
	    break;
	case MODE_DECRYPT:	/* decrypt using CBC mode */
	    cbcdec(cph);
	    break;
	case MODE_ENCRYPT:	/* encrypt using CBC mode */
	    cbcenc(cph);
	    break;
	}
	break;
    case ALG_CFB:
	switch (mode) {
	case MODE_AUTHENTICATE:	/* authenticate using CFB mode */
	    cfbauth(cph);
	    break;
	case MODE_DECRYPT:	/* decrypt using CFB mode */
	    cfbdec(cph);
	    break;
	case MODE_ENCRYPT:	/* encrypt using CFB mode */
	    cfbenc(cph);
	    break;
	}
	break;
    case ALG_CFBA:
	switch (mode) {
	case MODE_AUTHENTICATE:	/* authenticate using CFBA mode */
	    errx(1, "can't authenticate with CFBA mode");
	    break;
	case MODE_DECRYPT:	/* decrypt using CFBA mode */
	    cfbadec(cph);
	    break;
	case MODE_ENCRYPT:	/* encrypt using CFBA mode */
	    cfbaenc(cph);
	    break;
	}
	break;
    case ALG_ECB:
	switch (mode) {
	case MODE_AUTHENTICATE:	/* authenticate using ECB mode */
	    errx(1, "can't authenticate with ECB mode");
	    break;
	case MODE_DECRYPT:	/* decrypt using ECB mode */
	    ecbdec(cph);
	    break;
	case MODE_ENCRYPT:	/* encrypt using ECB mode */
	    ecbenc(cph);
	    break;
	}
	break;
    case ALG_OFB:
	switch (mode) {
	case MODE_AUTHENTICATE:	/* authenticate using OFB mode */
	    errx(1, "can't authenticate with OFB mode");
	    break;
	case MODE_DECRYPT:	/* decrypt using OFB mode */
	    ofbdec(cph);
	    break;
	case MODE_ENCRYPT:	/* encrypt using OFB mode */
	    ofbenc(cph);
	    break;
	}
	break;
    }

    ec = 0;

exit:

    xx = rpmCipherFinal(cph);

    if (ofd)
	xx = Fclose(ofd);
    if (ifd)
	xx = Fclose(ifd);
    con = rpmioFini(con);
    return ec;
}
