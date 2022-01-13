/* crypto/ecdsa/ecdsatest.c */
/*
 * Written by Nils Larsch for the OpenSSL project.
 */
/* ====================================================================
 * Copyright (c) 2000-2005 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by 
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * The elliptic curve binary polynomial software is originally written by 
 * Sheueling Chang Shantz and Douglas Stebila of Sun Microsystems Laboratories.
 *
 */

#include "system.h"

#include <rpmio.h>
#include <rpmlog.h>
#include <rpmcb.h>

#define	_RPMPGP_INTERNAL
#include <poptIO.h>

#ifdef	NOTYET
#define	_RPMGC_INTERNAL
#include <rpmgc.h>

#define	_RPMLTC_INTERNAL
#include <rpmltc.h>

#define	_RPMBC_INTERNAL
#include <rpmbc.h>
#define	_RPMNSS_INTERNAL
#include <rpmnss.h>

#define	_RPMSSL_INTERNAL
#include <rpmssl.h>
#include <openssl/opensslconf.h>	/* XXX OPENSSL_NO_ECDSA */
#include <openssl/conf.h>		/* XXX CONF_* cleanup's */
#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>		/* XXX X509V3_EXT_cleanup() */
#endif	/* NOTYET */

#include "debug.h"

/*@access pgpDig @*/
/*@access pgpDigParams @*/

/*@-redecl@*/
/*@unchecked@*/
extern int _pgp_debug;

/*@unchecked@*/
extern int _pgp_print;
/*@=redecl@*/

static int use_fips;
static int selftest_only;

#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _pgp_debug < 0) \
	fprintf(stderr, "<-- %s(%p) %s\t%s\n", __FUNCTION__, (_dig), \
		((_rc) ? "OK" : "BAD"), (_dig)->pubkey_algoN); \
  }

/*==============================================================*/

typedef struct key_s {
/*@observer@*/
    const char * name;		/* key name */
    uint32_t value;
} KEY;

static int
keyCmp(const void * a, const void * b)
{
    return strcmp(((KEY *)a)->name, ((KEY *)b)->name);
}

static uint32_t
keyValue(KEY * keys, size_t nkeys, /*@null@*/ const char *name)
{
    uint32_t keyval = 0;

    if (name && *name) {
	/* XXX bsearch is overkill */
	KEY needle = { .name = name, .value = 0 };
	KEY *k = (KEY *)bsearch(&needle, keys, nkeys, sizeof(*keys), keyCmp);
	if (k)
	    keyval = k->value;
    }
    return keyval;
}

typedef struct keyVN_s {
    int V;
/*@observer@*/
    const char * N;		/* key name */
} keyVN_t;

static int
keyVNCmp(const void * a, const void * b)
{
    return (((keyVN_t *)a)->V - ((keyVN_t *)b)->V);
}

static const char *
keyVN(keyVN_t * keys, size_t nkeys, /*@null@*/ int V)
{
    const char * N = NULL;

    if (V) {
	/* XXX bsearch is overkill */
	keyVN_t needle = { .V = V, .N = NULL };
	keyVN_t *k = (keyVN_t *)
		bsearch(&needle, keys, nkeys, sizeof(*keys), keyVNCmp);
	if (k)
	    N = k->N;
    }
    return N;
}

static const char * _pgpHashAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpHashTbl, (rpmuint8_t)algo);
}

static const char * _pgpPubkeyAlgo2Name(uint32_t algo)
{
    return pgpValStr(pgpPubkeyTbl, (rpmuint8_t)algo);
}

/*==============================================================*/

#define FLAG_CRYPT (1 << 0)
#define FLAG_SIGN  (1 << 1)
#define FLAG_GRIP  (1 << 2)

typedef struct _KP_RSA_s {
    const char * n;
    const char * e;
    const char * d;
    const char * p;
    const char * q;
    const char * u;
} KP_RSA_t;
typedef struct _KP_DSA_s {
    const char * p;
    const char * q;
    const char * g;
    const char * y;
    const char * x;
} KP_DSA_t;
typedef struct _KP_ELG_s {
    const char * p;
    const char * g;
    const char * y;
    const char * x;
} KP_ELG_t;
typedef struct _KP_ECDSA_s {
    const char * curve;
    const char * p;
    const char * a;
    const char * b;
    const char * G;
    const char * n;
    const char * Q;
    const char * d;	/* FIXME: add to test vector. */
} KP_ECDSA_t;
typedef union _KP_u {
    void * sentinel;
    KP_RSA_t RSA;
    KP_DSA_t DSA;
    KP_ELG_t ELG;
    KP_ECDSA_t ECDSA;
} KP_t;
typedef struct _AFKP_s {
    int algo;
    int flags;
    int nbits;
    int qbits;
    KP_t KP;
    const unsigned char grip[20];
} AFKP_t;

static void fail(const char *format, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, format);
    vfprintf(stderr, format, arg_ptr);
    va_end(arg_ptr);
    _pgp_error_count++;
}

static void die(const char *format, ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, format);
    vfprintf(stderr, format, arg_ptr);
    va_end(arg_ptr);
    exit(1);
}

#define MAX_DATA_LEN 100

/*==============================================================*/

#if defined(_RPMBC_INTERNAL)

/*@unchecked@*/
static int _rpmbc_debug;

#undef	SPEW
#define	SPEW(_t, _rc, _dig)	\
  { if ((_t) || _rpmbc_debug || _pgp_debug < 0) \
	fprintf(stderr, "<-- %s(%p) %s\t%s\n", __FUNCTION__, (_dig), \
		((_rc) ? "OK" : "BAD"), (_dig)->pubkey_algoN); \
  }

static void rpmbcDumpRSA(const char * msg, rpmbc bc)
{
rsakp * kp = &bc->rsa_keypair;
if (msg) fprintf(stderr, "========== %s\n", msg);
fprintf(stderr, "\t n: %s\n", pgpHexStr((rpmuint8_t *)kp->n.modl, MP_WORDS_TO_BYTES(kp->n.size)));
fprintf(stderr, "\t e: %s\n", pgpHexStr((rpmuint8_t *)kp->e.data, MP_WORDS_TO_BYTES(kp->e.size)));
fprintf(stderr, "\t d: %s\n", pgpHexStr((rpmuint8_t *)kp->d.data, MP_WORDS_TO_BYTES(kp->d.size)));
fprintf(stderr, "\t p: %s\n", pgpHexStr((rpmuint8_t *)kp->p.modl, MP_WORDS_TO_BYTES(kp->p.size)));
fprintf(stderr, "\t q: %s\n", pgpHexStr((rpmuint8_t *)kp->q.modl, MP_WORDS_TO_BYTES(kp->q.size)));
fprintf(stderr, "\tdp: %s\n", pgpHexStr((rpmuint8_t *)kp->dp.data, MP_WORDS_TO_BYTES(kp->dp.size)));
fprintf(stderr, "\tdq: %s\n", pgpHexStr((rpmuint8_t *)kp->dq.data, MP_WORDS_TO_BYTES(kp->dq.size)));
fprintf(stderr, "\tqi: %s\n", pgpHexStr((rpmuint8_t *)kp->qi.data, MP_WORDS_TO_BYTES(kp->qi.size)));
fprintf(stderr, "\thm: %s\n", pgpHexStr((rpmuint8_t *)bc->hm.data, MP_WORDS_TO_BYTES(bc->hm.size)));
fprintf(stderr, "\t m: %s\n", pgpHexStr((rpmuint8_t *)bc->m.data, MP_WORDS_TO_BYTES(bc->m.size)));
fprintf(stderr, "\t c: %s\n", pgpHexStr((rpmuint8_t *)bc->c.data, MP_WORDS_TO_BYTES(bc->c.size)));
}

static void rpmbcDumpDSA(const char * msg, rpmbc bc)
{
dsakp * kp = &bc->dsa_keypair;
if (msg) fprintf(stderr, "========== %s\n", msg);
fprintf(stderr, "\t p: %s\n", pgpHexStr((rpmuint8_t *)kp->param.p.modl, MP_WORDS_TO_BYTES(kp->param.p.size)));
fprintf(stderr, "\t q: %s\n", pgpHexStr((rpmuint8_t *)kp->param.q.modl, MP_WORDS_TO_BYTES(kp->param.q.size)));
fprintf(stderr, "\t r: %s\n", pgpHexStr((rpmuint8_t *)kp->param.r.data, MP_WORDS_TO_BYTES(kp->param.r.size)));
fprintf(stderr, "\t g: %s\n", pgpHexStr((rpmuint8_t *)kp->param.g.data, MP_WORDS_TO_BYTES(kp->param.g.size)));
fprintf(stderr, "\t n: %s\n", pgpHexStr((rpmuint8_t *)kp->param.n.modl, MP_WORDS_TO_BYTES(kp->param.n.size)));
fprintf(stderr, "\t y: %s\n", pgpHexStr((rpmuint8_t *)kp->y.data, MP_WORDS_TO_BYTES(kp->y.size)));
fprintf(stderr, "\t x: %s\n", pgpHexStr((rpmuint8_t *)kp->x.data, MP_WORDS_TO_BYTES(kp->x.size)));
fprintf(stderr, "\thm: %s\n", pgpHexStr((rpmuint8_t *)bc->hm.data, MP_WORDS_TO_BYTES(bc->hm.size)));
fprintf(stderr, "\t r: %s\n", pgpHexStr((rpmuint8_t *)bc->r.data, MP_WORDS_TO_BYTES(bc->r.size)));
fprintf(stderr, "\t s: %s\n", pgpHexStr((rpmuint8_t *)bc->s.data, MP_WORDS_TO_BYTES(bc->s.size)));
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static
unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char) (c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return (unsigned char) '\0';
}

static
int rpmbcSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmbc bc = dig->impl;
    size_t nbits = 0;
    size_t nb = 0;
    const char * prefix = rpmDigestASN1(ctx);
    const char * hexstr;
    char * tt;
    int rc;
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    if (prefix == NULL)
	return 1;

    /*
     * The no. of bytes for hash + PKCS1 padding is needed.
     * Either n or c can be used as the size, but different code paths
     * populate n or c indeterminately. So try c, then n,
     * and error if the no. of bytes isn't sane.
     */
    if (bc->c.size)
	nbits = (unsigned) MP_WORDS_TO_BITS(bc->c.size);
    else if (bc->rsa_keypair.n.size)
	nbits = (unsigned) MP_WORDS_TO_BITS(bc->rsa_keypair.n.size);
    nb = (nbits + 7) >> 3;		/* XXX overkill */
    if (nb < 64/8 || nb > 65536/8)	/* XXX generous "sanity" check */
	return 1;

/* XXX FIXME: do PKCS1 padding in binary not hex */
/* XXX FIXME: should this lazy free be done elsewhere? */
bc->digest = _free(bc->digest);
bc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&bc->digest, &bc->digestlen, 1);
    hexstr = tt = xmalloc(2 * nb + 1);
    memset(tt, (int) 'f', (2 * nb));
    tt[0] = '0'; tt[1] = '0';
    tt[2] = '0'; tt[3] = '1';
    tt += (2 * nb) - strlen(prefix) - strlen(bc->digest) - 2;
    *tt++ = '0'; *tt++ = '0';
    tt = stpcpy(tt, prefix);
    tt = stpcpy(tt, bc->digest);

/*@-moduncon -noeffectuncon @*/
mpnfree(&bc->hm);
    mpnzero(&bc->hm);   (void) mpnsethex(&bc->hm, hexstr);
/*@=moduncon =noeffectuncon @*/

    hexstr = _free(hexstr);

    /* Compare leading 16 bits of digest for quick check. */
    {	const char *str = bc->digest;
	rpmuint8_t s[2];
	const rpmuint8_t *t = sigp->signhash16;
	s[0] = (rpmuint8_t) (nibble(str[0]) << 4) | nibble(str[1]);
	s[1] = (rpmuint8_t) (nibble(str[2]) << 4) | nibble(str[3]);
	rc = memcmp(s, t, sizeof(sigp->signhash16));
    }
SPEW(rc, !rc, dig);
    return rc;
}

static
int rpmbcVerifyRSA(pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc;

    rc = rsavrfy(&bc->rsa_keypair.n, &bc->rsa_keypair.e, &bc->c, &bc->hm);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSignRSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

    mpnzero(&bc->c);
#ifdef	SLOWER
    xx = rsapri(&bc->rsa_keypair.n, &bc->rsa_keypair.d, &bc->hm, &bc->c);
#else
    /* XXX RSA w CRT is ~3x faster for signing. */
    xx = rsapricrt(&bc->rsa_keypair.n, &bc->rsa_keypair.p, &bc->rsa_keypair.q,
		&bc->rsa_keypair.dp, &bc->rsa_keypair.dq, &bc->rsa_keypair.qi,
		&bc->hm, &bc->c);
#endif
    if (xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcGenerateRSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

if (bc->nbits == 0) bc->nbits = 1024;	/* XXX FIXME */

xx = randomGeneratorContextInit(&bc->rngc, randomGeneratorDefault());

rsakpFree(&bc->rsa_keypair);
    xx = rsakpMake(&bc->rsa_keypair, &bc->rngc, bc->nbits);
    if (xx) failures++;

    /* generate a random m in the range 0 < m < n */
    mpnzero(&bc->m);
    mpbnrnd(&bc->rsa_keypair.n, &bc->rngc, &bc->m);

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmbc bc = dig->impl;
    rpmuint8_t signhash16[2];
    int rc;
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
bc->digest = _free(bc->digest);
bc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&bc->digest, &bc->digestlen, 1);

    {	char * hm = bc->digest;
	char lastc = hm[40];
	/* XXX Truncate to 160bits. */
	hm[40] = '\0';
mpnfree(&bc->hm);
	mpnzero(&bc->hm);	(void) mpnsethex(&bc->hm, hm);
	hm[40] = lastc;
    }

    /* Compare leading 16 bits of digest for quick check. */
    signhash16[0] = (rpmuint8_t)((*bc->hm.data >> 24) & 0xff);
    signhash16[1] = (rpmuint8_t)((*bc->hm.data >> 16) & 0xff);
    rc = memcmp(signhash16, sigp->signhash16, sizeof(signhash16));
SPEW(rc, !rc, dig);
    return rc;
}

static
int rpmbcVerifyDSA(pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

    xx = dsavrfy(&bc->dsa_keypair.param.p, &bc->dsa_keypair.param.q,
		&bc->dsa_keypair.param.g, &bc->hm, &bc->dsa_keypair.y,
		&bc->r, &bc->s);
    if (!xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSignDSA(pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

    mpnzero(&bc->r);
    mpnzero(&bc->s);
    xx = dsasign(&bc->dsa_keypair.param.p, &bc->dsa_keypair.param.q,
		&bc->dsa_keypair.param.g, &bc->rngc, &bc->hm,
		&bc->dsa_keypair.x, &bc->r, &bc->s);
    if (xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcGenerateDSA(pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

if (bc->nbits == 0) bc->nbits = 1024;	/* XXX FIXME */

xx = randomGeneratorContextInit(&bc->rngc, randomGeneratorDefault());

    xx = dlkp_pInit(&bc->dsa_keypair);
    if (xx) failures++;
    xx = dsaparamMake(&bc->dsa_keypair.param, &bc->rngc, bc->nbits);
    if (xx) failures++;

    xx = dldp_pPair(&bc->dsa_keypair.param, &bc->rngc, &bc->dsa_keypair.x,
		&bc->dsa_keypair.y);
    if (xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    int rc = 1;		/* XXX always fail. */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)NULL, NULL, 0);

    /* Compare leading 16 bits of digest for quick check. */

SPEW(rc, !rc, dig);
    return rc;
}

static
int rpmbcVerifyELG(pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

if (bc->hm.size == 0) return rc;

    xx = elgv1vrfy(&bc->elg_keypair.param.p, &bc->elg_keypair.param.n,
		&bc->elg_keypair.param.g, &bc->hm, &bc->elg_keypair.y,
		&bc->r, &bc->s);
    if (xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSignELG(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

if (bc->hm.size == 0) return rc;

    mpnzero(&bc->r);
    mpnzero(&bc->s);
    xx = elgv1sign(&bc->elg_keypair.param.p, &bc->elg_keypair.param.n,
		&bc->elg_keypair.param.g, &bc->rngc, &bc->hm,
		&bc->elg_keypair.x, &bc->r, &bc->s);
    if (xx) failures++;

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcGenerateELG(/*@unused@*/pgpDig dig)
	/*@*/
{
static const char P_2048[] = "fd12e8b7e096a28a00fb548035953cf0eba64ceb5dff0f5672d376d59c196da729f6b5586f18e6f3f1a86c73c5b15662f59439613b309e52aa257488619e5f76a7c4c3f7a426bdeac66bf88343482941413cef06256b39c62744dcb97e7b78e36ec6b885b143f6f3ad0a1cd8a5713e338916613892a264d4a47e72b583fbdaf5bce2bbb0097f7e65cbc86d684882e5bb8196d522dcacd6ad00dfbcd8d21613bdb59c485a65a58325d792272c09ad1173e12c98d865adb4c4d676ada79830c58c37c42dff8536e28f148a23f296513816d3dfed0397a3d4d6e1fa24f07e1b01643a68b4274646a3b876e810206eddacea2b9ef7636a1da5880ef654288b857ea3";
static const char P_1024[] = "e64a3deeddb723e2e4db54c2b09567d196367a86b3b302be07e43ffd7f2e016f866de5135e375bdd2fba6ea9b4299010fafa36dc6b02ba3853cceea07ee94bfe30e0cc82a69c73163be26e0c4012dfa0b2839c97d6cd71eee59a303d6177c6a6740ca63bd04c1ba084d6c369dc2fbfaeebe951d58a4824de52b580442d8cae77";

    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

xx = randomGeneratorContextInit(&bc->rngc, randomGeneratorDefault());

    xx = 0;

    xx = dlkp_pInit(&bc->elg_keypair);
    if (xx) failures++;

#ifdef	DYING
    xx = dldp_pInit(&bc->elg_keypair.param);
    if (xx) failures++;
#endif

    switch (bc->nbits) {
#ifdef	NOTYET
    case 2048:
	mpbsethex(&bc->elg_keypair.param.p, P_2048);
	break;
    case 1024:
    case 0:
	mpbsethex(&bc->elg_keypair.param.p, P_1024);
	break;
#endif
    default:
	xx = dldp_pgonMakeSafe(&bc->elg_keypair.param, &bc->rngc, bc->nbits);
	break;
    }
#ifdef NOTYET
    if (bc->elg_keypair.param.q.size == 0) {
	mpnumber q;

	mpnzero(&q);
	/* set q to half of P */
	mpnset(&q, bc->elg_keypair.param.p.size, bc->elg_keypair.param.p.modl);
	mpdivtwo(q.size, q.data);
	mpbset(&bc->elg_keypair.param.q, q.size, q.data);
	/* set r to 2 */
	mpnsetw(&bc->elg_keypair.param.r, 2);

	/* make a generator, order n */
	xx = dldp_pgonGenerator(&bc->elg_keypair.param, &bc->rngc);

    }
#endif
    if (xx) failures++;

    xx = dldp_pPair(&bc->elg_keypair.param, &bc->rngc,
		&bc->elg_keypair.x, &bc->elg_keypair.y);
    if (xx) failures++;

    mpnfree(&bc->r);
    mpnfree(&bc->s);
    mpnfree(&bc->hm);

#ifdef	DYING
    dldp_pFree(&bc->elg_params);
#endif

    dlkp_pFree(&bc->elg_keypair);

    rc = (failures == 0);

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    int rc = 1;		/* XXX always fail. */
    int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)NULL, NULL, 0);

    /* Compare leading 16 bits of digest for quick check. */

SPEW(rc, !rc, dig);
    return rc;
}

static
int rpmbcVerifyECDSA(pgpDig dig)
	/*@*/
{
    int rc = 0;		/* XXX always fail. */

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcSignECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    int rc = 0;		/* XXX always fail. */

SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmbcGenerateECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmbc bc = dig->impl;
    int rc = 0;		/* Assume failure. */
    int failures = 0;
    int xx;

SPEW(!rc, rc, dig);
    return rc;
}

static int rpmbcErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	REFERENCE
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmbcStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmbcAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgbcvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmbcAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmgbcvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmbcAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	REFERENCE
    rc = rpmbcAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmbcVerify(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmbcVerifyRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmbcVerifyDSA(dig);
	break;
#ifdef	NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmbcVerifyELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmbcVerifyECDSA(dig);
	break;
#endif
    }
SPEW(1, rc, dig);
    return rc;
}

static int rpmbcSign(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmbcSignRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmbcSignDSA(dig);
	break;
#ifdef	NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmbcSignELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmbcSignECDSA(dig);
	break;
#endif
    }
SPEW(1, rc, dig);
    return rc;
}

static int rpmbcGenerate(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmbcGenerateRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmbcGenerateDSA(dig);
	break;
#ifdef	NOTYET
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmbcGenerateELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmbcGenerateECDSA(dig);
	break;
#endif
    }
SPEW(1, rc, dig);
    return rc;
}
#endif	/* _RPMBC_INTERNAL */

/*==============================================================*/

#if defined(_RPMGC_INTERNAL)

static void
rpmgcProgress(void *cb_data, const char *what, int printchar,
		 int current, int total)
{
    (void) cb_data;
    (void) what;
    (void) current;
    (void) total;

    if (printchar == '\n')
	fputs("<LF>", stdout);
    else
	putchar(printchar);
    fflush(stdout);
}

static
void rpmgcDump(const char * msg, gcry_sexp_t sexp)
	/*@*/
{
    if (_pgp_debug) {
	size_t nb = gcry_sexp_sprint(sexp, GCRYSEXP_FMT_ADVANCED, NULL, 0);
	char * buf = alloca(nb+1);

	nb = gcry_sexp_sprint(sexp, GCRYSEXP_FMT_ADVANCED, buf, nb);
	buf[nb] = '\0';
	fprintf(stderr, "========== %s:\n%s", msg, buf);
    }
    return;
}

static
gcry_error_t rpmgcErr(rpmgc gc, const char * msg, gcry_error_t err)
	/*@*/
{
    /* XXX Don't spew on expected failures ... */
    if (err && gcry_err_code(err) != gc->badok)
	fprintf (stderr, "rpmgc: %s(0x%0x): %s/%s\n",
		msg, (unsigned)err, gcry_strsource(err), gcry_strerror(err));
    return err;
}

static
int rpmgcSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmgc gc = dig->impl;
    gcry_error_t err;
    const char * hash_algo_name = NULL;
    int rc;
    int xx;

    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	hash_algo_name = "md5";
	break;
    case PGPHASHALGO_SHA1:
	hash_algo_name = "sha1";
	break;
    case PGPHASHALGO_RIPEMD160:
	hash_algo_name = "ripemd160";
	break;
    case PGPHASHALGO_MD2:
	hash_algo_name = "md2";
	break;
    case PGPHASHALGO_TIGER192:
	hash_algo_name = "tiger";
	break;
    case PGPHASHALGO_HAVAL_5_160:
#ifdef	NOTYET
	hash_algo_name = "haval";
#endif
	break;
    case PGPHASHALGO_SHA256:
	hash_algo_name = "sha256";
	break;
    case PGPHASHALGO_SHA384:
	hash_algo_name = "sha384";
	break;
    case PGPHASHALGO_SHA512:
	hash_algo_name = "sha512";
	break;
    case PGPHASHALGO_SHA224:
#ifdef	NOTYET
	hash_algo_name = "sha224";
#endif
	break;
    default:
	break;
    }
    if (hash_algo_name == NULL)
	return 1;

    xx = rpmDigestFinal(ctx, (void **)&gc->digest, &gc->digestlen, 0);

    /* Set RSA hash. */
    err = rpmgcErr(gc, "RSA c",
	    gcry_sexp_build(&gc->hash, NULL,
		"(data (flags pkcs1) (hash %s %b))", hash_algo_name, gc->digestlen, gc->digest) );
if (_pgp_debug < 0) rpmgcDump("gc->hash", gc->hash);

    /* Compare leading 16 bits of digest for quick check. */
    {	const rpmuint8_t *s = gc->digest;
	const rpmuint8_t *t = sigp->signhash16;
	rc = memcmp(s, t, sizeof(sigp->signhash16));
    }

    return rc;
}

static
int rpmgcSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmgc gc = dig->impl;
int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)&gc->digest, &gc->digestlen, 0);

    /* Set DSA hash. */
    {	gcry_mpi_t c = NULL;
	/* XXX truncate to 160 bits */
	gc->err = rpmgcErr(gc, "DSA c",
		gcry_mpi_scan(&c, GCRYMPI_FMT_USG, gc->digest, 160/8, NULL));
	gc->err = rpmgcErr(gc, "DSA gc->hash",
		gcry_sexp_build(&gc->hash, NULL,
			"(data (flags raw) (value %m))", c) );
	gcry_mpi_release(c);
if (_pgp_debug < 0) rpmgcDump("gc->hash", gc->hash);
    }

    /* Compare leading 16 bits of digest for quick check. */
    return memcmp(gc->digest, sigp->signhash16, sizeof(sigp->signhash16));
}

static
int rpmgcSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmgc gc = dig->impl;
    int rc = 1;		/* XXX always fail. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)&gc->digest, &gc->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */

    return rc;
}

static
int rpmgcSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmgc gc = dig->impl;
    int rc = 1;		/* assume failure. */
int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
gc->digest = _free(gc->digest);
gc->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&gc->digest, &gc->digestlen, 0);

    {   gcry_mpi_t c = NULL;
	gc->err = rpmgcErr(gc, "ECDSA c",
		gcry_mpi_scan(&c, GCRYMPI_FMT_USG, gc->digest, gc->digestlen, NULL));
if (gc->hash) {
    gcry_sexp_release(gc->hash);	gc->hash = NULL;
}
	gc->err = rpmgcErr(gc, "ECDSA gc->hash",
		gcry_sexp_build(&gc->hash, NULL,
			"(data (flags raw) (value %m))", c) );
	gcry_mpi_release(c);
if (_pgp_debug < 0) rpmgcDump("gc->hash", gc->hash);
    }

    /* Compare leading 16 bits of digest for quick check. */

if (_pgp_debug < 0)
fprintf(stderr, "<-- %s(%p,%p) rc %d\n", __FUNCTION__, dig, sigp, rc);

    return rc;
}
#endif	/* _RPMGC_INTERNAL */

/*==============================================================*/

#if defined(_RPMGC_INTERNAL)

static void check_cbc_mac_cipher(pgpDig dig)
{
  struct tv {
    int algo;
    char key[MAX_DATA_LEN];
    unsigned char plaintext[MAX_DATA_LEN];
    size_t plaintextlen;
    char mac[MAX_DATA_LEN];
  } tv[] = {
      { PGPSYMKEYALGO_AES_128,
	"chicken teriyaki",
	"This is a sample plaintext for CBC MAC of sixtyfour bytes.......",
	0, "\x23\x8f\x6d\xc7\x53\x6a\x62\x97\x11\xc4\xa5\x16\x43\xea\xb0\xb6" },
      { PGPSYMKEYALGO_TRIPLE_DES,
	"abcdefghABCDEFGH01234567",
	"This is a sample plaintext for CBC MAC of sixtyfour bytes.......",
	0, "\x5c\x11\xf0\x01\x47\xbd\x3d\x3a" },
      { GCRY_CIPHER_DES,
	"abcdefgh",
	"This is a sample plaintext for CBC MAC of sixtyfour bytes.......",
	0, "\xfa\x4b\xdf\x9d\xfa\xab\x01\x70" }
    };
    unsigned char out[MAX_DATA_LEN];
    gcry_cipher_hd_t hd;
    gcry_error_t err = 0;
    int blklen;
    int keylen;
    size_t i;

    rpmlog(RPMLOG_INFO, "  Starting CBC MAC checks.\n");

    for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {

	/* Lookup & FIPS check. */
	if (pgpImplAvailableCipher(dig, tv[i].algo))
	    continue;

	err = gcry_cipher_open(&hd,
			       tv[i].algo,
			       GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_CBC_MAC);
	if (!hd) {
	    fail("cbc-mac algo %d, gcry_cipher_open failed: %s\n",
		 tv[i].algo, gpg_strerror(err));
	    return;
	}

	blklen = gcry_cipher_get_algo_blklen(tv[i].algo);
	if (!blklen) {
	    fail("cbc-mac algo %d, gcry_cipher_get_algo_blklen failed\n",
		 tv[i].algo);
	    gcry_cipher_close(hd);
	    return;
	}

	keylen = gcry_cipher_get_algo_keylen(tv[i].algo);
	if (!keylen) {
	    fail("cbc-mac algo %d, gcry_cipher_get_algo_keylen failed\n",
		 tv[i].algo);
	    return;
	}

	err = gcry_cipher_setkey(hd, tv[i].key, keylen);
	if (err) {
	    fail("cbc-mac algo %d, gcry_cipher_setkey failed: %s\n",
		 tv[i].algo, gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}

	err = gcry_cipher_setiv(hd, NULL, 0);
	if (err) {
	    fail("cbc-mac algo %d, gcry_cipher_setiv failed: %s\n",
		 tv[i].algo, gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}

	rpmlog(RPMLOG_INFO, "    checking CBC MAC for %s [%i]\n",
		    gcry_cipher_algo_name(tv[i].algo), tv[i].algo);
	err = gcry_cipher_encrypt(hd,
				  out, blklen,
				  tv[i].plaintext,
				  tv[i].plaintextlen ?
				  tv[i].plaintextlen :
				  strlen((char *) tv[i].plaintext));
	if (err) {
	    fail("cbc-mac algo %d, gcry_cipher_encrypt failed: %s\n",
		 tv[i].algo, gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}
#if 0
	{
	    int j;
	    for (j = 0; j < gcry_cipher_get_algo_blklen(tv[i].algo); j++)
		printf("\\x%02x", out[j] & 0xFF);
	    printf("\n");
	}
#endif

	if (memcmp(tv[i].mac, out, blklen))
	    fail("cbc-mac algo %d, encrypt mismatch entry %d\n",
		 tv[i].algo, i);

	gcry_cipher_close(hd);
    }
    rpmlog(RPMLOG_INFO, "  Completed CBC MAC checks.\n");
}

static void check_aes128_cbc_cts_cipher(pgpDig dig)
{
    char key[128 / 8] = "chicken teriyaki";
    unsigned char plaintext[] =
	"I would like the General Gau's Chicken, please, and wonton soup.";
  struct tv {
    unsigned char out[MAX_DATA_LEN];
    int inlen;
  } tv[] = {
      { "\xc6\x35\x35\x68\xf2\xbf\x8c\xb4\xd8\xa5\x80\x36\x2d\xa7\xff\x7f"
	"\x97",
	17 },
      { "\xfc\x00\x78\x3e\x0e\xfd\xb2\xc1\xd4\x45\xd4\xc8\xef\xf7\xed\x22"
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5",
	31 },
      { "\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8"
	"\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84",
	32 },
      { "\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\xb3\xff\xfd\x94\x0c\x16\xa1\x8c\x1b\x55\x49\xd2\xf8\x38\x02\x9e"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5",
	47 },
      { "\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\x9d\xad\x8b\xbb\x96\xc4\xcd\xc0\x3b\xc1\x03\xe1\xa1\x94\xbb\xd8"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8",
	48 },
      { "\x97\x68\x72\x68\xd6\xec\xcc\xc0\xc0\x7b\x25\xe2\x5e\xcf\xe5\x84"
	"\x39\x31\x25\x23\xa7\x86\x62\xd5\xbe\x7f\xcb\xcc\x98\xeb\xf5\xa8"
	"\x48\x07\xef\xe8\x36\xee\x89\xa5\x26\x73\x0d\xbc\x2f\x7b\xc8\x40"
	"\x9d\xad\x8b\xbb\x96\xc4\xcd\xc0\x3b\xc1\x03\xe1\xa1\x94\xbb\xd8",
	64 },
    };
    unsigned char out[MAX_DATA_LEN];
    gcry_cipher_hd_t hd;
    gcry_error_t err = 0;
    size_t i;

    rpmlog(RPMLOG_INFO, "  Starting AES128 CBC CTS checks.\n");
    err = gcry_cipher_open(&hd,
			   PGPSYMKEYALGO_AES_128,
			   GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_CBC_CTS);
    if (err) {
	fail("aes-cbc-cts, gcry_cipher_open failed: %s\n",
	     gpg_strerror(err));
	return;
    }

    err = gcry_cipher_setkey(hd, key, 128 / 8);
    if (err) {
	fail("aes-cbc-cts, gcry_cipher_setkey failed: %s\n",
	     gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
	err = gcry_cipher_setiv(hd, NULL, 0);
	if (err) {
	    fail("aes-cbc-cts, gcry_cipher_setiv failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}

	rpmlog(RPMLOG_INFO, "    checking encryption for length %i\n",
		    tv[i].inlen);
	err = gcry_cipher_encrypt(hd, out, MAX_DATA_LEN, plaintext,
				tv[i].inlen);
	if (err) {
	    fail("aes-cbc-cts, gcry_cipher_encrypt failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}

	if (memcmp(tv[i].out, out, tv[i].inlen))
	    fail("aes-cbc-cts, encrypt mismatch entry %d\n", i);

	err = gcry_cipher_setiv(hd, NULL, 0);
	if (err) {
	    fail("aes-cbc-cts, gcry_cipher_setiv failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}
	rpmlog(RPMLOG_INFO, "    checking decryption for length %i\n",
		    tv[i].inlen);
	err = gcry_cipher_decrypt(hd, out, tv[i].inlen, NULL, 0);
	if (err) {
	    fail("aes-cbc-cts, gcry_cipher_decrypt failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hd);
	    return;
	}

	if (memcmp(plaintext, out, tv[i].inlen))
	    fail("aes-cbc-cts, decrypt mismatch entry %d\n", i);
    }

    gcry_cipher_close(hd);
    rpmlog(RPMLOG_INFO, "  Completed AES128 CBC CTS checks.\n");
}

static void check_ctr_cipher(pgpDig dig)
{
  struct tv {
    int algo;
    char key[MAX_DATA_LEN];
    char ctr[MAX_DATA_LEN];
    struct data {
      unsigned char plaintext[MAX_DATA_LEN];
      int inlen;
      char out[MAX_DATA_LEN];
    }
    data[MAX_DATA_LEN];
  } tv[] =
    {
      /* http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf */
      {	PGPSYMKEYALGO_AES_128,
	"\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
	"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
	{ { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
	    16,
	    "\x87\x4d\x61\x91\xb6\x20\xe3\x26\x1b\xef\x68\x64\x99\x0d\xb6\xce" },
	  { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
	    16,
	    "\x98\x06\xf6\x6b\x79\x70\xfd\xff\x86\x17\x18\x7b\xb9\xff\xfd\xff" },
	  { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
	    16,
	    "\x5a\xe4\xdf\x3e\xdb\xd5\xd3\x5e\x5b\x4f\x09\x02\x0d\xb0\x3e\xab" },
	  { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
	    16,
	    "\x1e\x03\x1d\xda\x2f\xbe\x03\xd1\x79\x21\x70\xa0\xf3\x00\x9c\xee" },
	}
      },
      {	PGPSYMKEYALGO_AES_192,
	"\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b"
	"\x80\x90\x79\xe5\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
	"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
	{ { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
	    16,
	    "\x1a\xbc\x93\x24\x17\x52\x1c\xa2\x4f\x2b\x04\x59\xfe\x7e\x6e\x0b" },
	  { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
	    16,
	    "\x09\x03\x39\xec\x0a\xa6\xfa\xef\xd5\xcc\xc2\xc6\xf4\xce\x8e\x94" },
	  { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
	    16,
	    "\x1e\x36\xb2\x6b\xd1\xeb\xc6\x70\xd1\xbd\x1d\x66\x56\x20\xab\xf7" },
	  { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
	    16,
	    "\x4f\x78\xa7\xf6\xd2\x98\x09\x58\x5a\x97\xda\xec\x58\xc6\xb0\x50" },
	}
      },
      {	PGPSYMKEYALGO_AES_256,
	"\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81"
	"\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
	"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff",
	{ { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
	    16,
	    "\x60\x1e\xc3\x13\x77\x57\x89\xa5\xb7\xa7\xf5\x04\xbb\xf3\xd2\x28" },
	  { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
	    16,
	    "\xf4\x43\xe3\xca\x4d\x62\xb5\x9a\xca\x84\xe9\x90\xca\xca\xf5\xc5" },
	  { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
	    16,
	    "\x2b\x09\x30\xda\xa2\x3d\xe9\x4c\xe8\x70\x17\xba\x2d\x84\x98\x8d" },
	  { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
	    16,
	    "\xdf\xc9\xc5\x8d\xb6\x7a\xad\xa6\x13\xc2\xdd\x08\x45\x79\x41\xa6" }
	}
      }
    };
    unsigned char out[MAX_DATA_LEN];
    gcry_cipher_hd_t hdd;
    gcry_cipher_hd_t hde;
    gcry_error_t err = 0;
    int blklen;
    int keylen;
    size_t i;
    int j;

    rpmlog(RPMLOG_INFO, "  Starting CTR cipher checks.\n");
    for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
	err = gcry_cipher_open(&hde, tv[i].algo, GCRY_CIPHER_MODE_CTR, 0);
	if (!err)
	    err =
		gcry_cipher_open(&hdd, tv[i].algo, GCRY_CIPHER_MODE_CTR,
				 0);
	if (err) {
	    fail("aes-ctr, gcry_cipher_open failed: %s\n",
		 gpg_strerror(err));
	    return;
	}

	keylen = gcry_cipher_get_algo_keylen(tv[i].algo);
	if (!keylen) {
	    fail("aes-ctr, gcry_cipher_get_algo_keylen failed\n");
	    return;
	}

	err = gcry_cipher_setkey(hde, tv[i].key, keylen);
	if (!err)
	    err = gcry_cipher_setkey(hdd, tv[i].key, keylen);
	if (err) {
	    fail("aes-ctr, gcry_cipher_setkey failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	blklen = gcry_cipher_get_algo_blklen(tv[i].algo);
	if (!blklen) {
	    fail("aes-ctr, gcry_cipher_get_algo_blklen failed\n");
	    return;
	}

	err = gcry_cipher_setctr(hde, tv[i].ctr, blklen);
	if (!err)
	    err = gcry_cipher_setctr(hdd, tv[i].ctr, blklen);
	if (err) {
	    fail("aes-ctr, gcry_cipher_setctr failed: %s\n", gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	rpmlog(RPMLOG_INFO, "    checking CTR mode for %s [%i]\n",
		    gcry_cipher_algo_name(tv[i].algo), tv[i].algo);
	for (j = 0; tv[i].data[j].inlen; j++) {
	    err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
			tv[i].data[j].plaintext,
			tv[i].data[j].inlen == -1
				?  (int)strlen((char *)tv[i].data[j].plaintext)
				: tv[i].data[j].inlen);
	    if (err) {
		fail("aes-ctr, gcry_cipher_encrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].out, out, tv[i].data[j].inlen))
		fail("aes-ctr, encrypt mismatch entry %d:%d\n", i, j);

	    err =
		gcry_cipher_decrypt(hdd, out, tv[i].data[j].inlen, NULL, 0);
	    if (err) {
		fail("aes-ctr, gcry_cipher_decrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].plaintext, out, tv[i].data[j].inlen))
		fail("aes-ctr, decrypt mismatch entry %d:%d\n", i, j);

	}

	/* Now check that we get valid return codes back for good and
	   bad inputs.  */
	err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
				  "1234567890123456", 16);
	if (err)
	    fail("aes-ctr, encryption failed for valid input");

	err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
				  "1234567890123456", 15);
	if (gpg_err_code(err) != GPG_ERR_INV_LENGTH)
	    fail("aes-ctr, too short input returned wrong error: %s\n",
		 gpg_strerror(err));

	err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
				  "12345678901234567", 17);
	if (gpg_err_code(err) != GPG_ERR_INV_LENGTH)
	    fail("aes-ctr, too long input returned wrong error: %s\n",
		 gpg_strerror(err));

	err = gcry_cipher_encrypt(hde, out, 15, "1234567890123456", 16);
	if (gpg_err_code(err) != GPG_ERR_BUFFER_TOO_SHORT)
	    fail("aes-ctr, too short output buffer returned wrong error: %s\n", gpg_strerror(err));

	err = gcry_cipher_encrypt(hde, out, 0, "1234567890123456", 16);
	if (gpg_err_code(err) != GPG_ERR_BUFFER_TOO_SHORT)
	    fail("aes-ctr, 0 length output buffer returned wrong error: %s\n", gpg_strerror(err));

	err = gcry_cipher_encrypt(hde, out, 16, "1234567890123456", 16);
	if (err)
	    fail("aes-ctr, correct length output buffer returned error: %s\n", gpg_strerror(err));

	/* Again, now for decryption.  */
	err = gcry_cipher_decrypt(hde, out, MAX_DATA_LEN,
				  "1234567890123456", 16);
	if (err)
	    fail("aes-ctr, decryption failed for valid input");

	err = gcry_cipher_decrypt(hde, out, MAX_DATA_LEN,
				  "1234567890123456", 15);
	if (gpg_err_code(err) != GPG_ERR_INV_LENGTH)
	    fail("aes-ctr, too short input returned wrong error: %s\n",
		 gpg_strerror(err));

	err = gcry_cipher_decrypt(hde, out, MAX_DATA_LEN,
				  "12345678901234567", 17);
	if (gpg_err_code(err) != GPG_ERR_INV_LENGTH)
	    fail("aes-ctr, too long input returned wrong error: %s\n",
		 gpg_strerror(err));

	err = gcry_cipher_decrypt(hde, out, 15, "1234567890123456", 16);
	if (gpg_err_code(err) != GPG_ERR_BUFFER_TOO_SHORT)
	    fail("aes-ctr, too short output buffer returned wrong error: %s\n", gpg_strerror(err));

	err = gcry_cipher_decrypt(hde, out, 0, "1234567890123456", 16);
	if (gpg_err_code(err) != GPG_ERR_BUFFER_TOO_SHORT)
	    fail("aes-ctr, 0 length output buffer returned wrong error: %s\n", gpg_strerror(err));

	err = gcry_cipher_decrypt(hde, out, 16, "1234567890123456", 16);
	if (err)
	    fail("aes-ctr, correct length output buffer returned error: %s\n", gpg_strerror(err));

	gcry_cipher_close(hde);
	gcry_cipher_close(hdd);
    }
    rpmlog(RPMLOG_INFO, "  Completed CTR cipher checks.\n");
}

static void check_cfb_cipher(pgpDig dig)
{
  struct tv {
    int algo;
    char key[MAX_DATA_LEN];
    char iv[MAX_DATA_LEN];
    struct data {
      unsigned char plaintext[MAX_DATA_LEN];
      int inlen;
      char out[MAX_DATA_LEN];
    }
    data[MAX_DATA_LEN];
  } tv[] =
    {
      /* http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf */
      { PGPSYMKEYALGO_AES_128,
        "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\x3b\x3f\xd9\x2e\xb7\x2d\xad\x20\x33\x34\x49\xf8\xe8\x3c\xfb\x4a" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\xc8\xa6\x45\x37\xa0\xb3\xa9\x3f\xcd\xe3\xcd\xad\x9f\x1c\xe5\x8b"},
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\x26\x75\x1f\x67\xa3\xcb\xb1\x40\xb1\x80\x8c\xf1\x87\xa4\xf4\xdf" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\xc0\x4b\x05\x35\x7c\x5d\x1c\x0e\xea\xc4\xc6\x6f\x9f\xf7\xf2\xe6" },
        }
      },
      { PGPSYMKEYALGO_AES_192,
        "\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b"
        "\x80\x90\x79\xe5\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\xcd\xc8\x0d\x6f\xdd\xf1\x8c\xab\x34\xc2\x59\x09\xc9\x9a\x41\x74" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\x67\xce\x7f\x7f\x81\x17\x36\x21\x96\x1a\x2b\x70\x17\x1d\x3d\x7a" },
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\x2e\x1e\x8a\x1d\xd5\x9b\x88\xb1\xc8\xe6\x0f\xed\x1e\xfa\xc4\xc9" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\xc0\x5f\x9f\x9c\xa9\x83\x4f\xa0\x42\xae\x8f\xba\x58\x4b\x09\xff" },
        }
      },
      { PGPSYMKEYALGO_AES_256,
        "\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81"
        "\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\xdc\x7e\x84\xbf\xda\x79\x16\x4b\x7e\xcd\x84\x86\x98\x5d\x38\x60" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\x39\xff\xed\x14\x3b\x28\xb1\xc8\x32\x11\x3c\x63\x31\xe5\x40\x7b" },
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\xdf\x10\x13\x24\x15\xe5\x4b\x92\xa1\x3e\xd0\xa8\x26\x7a\xe2\xf9" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\x75\xa3\x85\x74\x1a\xb9\xce\xf8\x20\x31\x62\x3d\x55\xb1\xe4\x71" }
        }
      }
    };
    unsigned char out[MAX_DATA_LEN];
    gcry_cipher_hd_t hdd;
    gcry_cipher_hd_t hde;
    gcry_error_t err = 0;
    int blklen;
    int keylen;
    size_t i;
    int j;

    rpmlog(RPMLOG_INFO, "  Starting CFB checks.\n");

    for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
	rpmlog(RPMLOG_INFO, "    checking CFB mode for %s [%i]\n",
		    gcry_cipher_algo_name(tv[i].algo), tv[i].algo);
	err = gcry_cipher_open(&hde, tv[i].algo, GCRY_CIPHER_MODE_CFB, 0);
	if (!err)
	    err =
		gcry_cipher_open(&hdd, tv[i].algo, GCRY_CIPHER_MODE_CFB,
				 0);
	if (err) {
	    fail("aes-cfb, gcry_cipher_open failed: %s\n",
		 gpg_strerror(err));
	    return;
	}

	keylen = gcry_cipher_get_algo_keylen(tv[i].algo);
	if (!keylen) {
	    fail("aes-cfb, gcry_cipher_get_algo_keylen failed\n");
	    return;
	}

	err = gcry_cipher_setkey(hde, tv[i].key, keylen);
	if (!err)
	    err = gcry_cipher_setkey(hdd, tv[i].key, keylen);
	if (err) {
	    fail("aes-cfb, gcry_cipher_setkey failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	blklen = gcry_cipher_get_algo_blklen(tv[i].algo);
	if (!blklen) {
	    fail("aes-cfb, gcry_cipher_get_algo_blklen failed\n");
	    return;
	}

	err = gcry_cipher_setiv(hde, tv[i].iv, blklen);
	if (!err)
	    err = gcry_cipher_setiv(hdd, tv[i].iv, blklen);
	if (err) {
	    fail("aes-cfb, gcry_cipher_setiv failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	for (j = 0; tv[i].data[j].inlen; j++) {
	    err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
				      tv[i].data[j].plaintext,
				      tv[i].data[j].inlen);
	    if (err) {
		fail("aes-cfb, gcry_cipher_encrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].out, out, tv[i].data[j].inlen)) {
		fail("aes-cfb, encrypt mismatch entry %d:%d\n", i, j);
	    }
	    err =
		gcry_cipher_decrypt(hdd, out, tv[i].data[j].inlen, NULL,
				    0);
	    if (err) {
		fail("aes-cfb, gcry_cipher_decrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].plaintext, out, tv[i].data[j].inlen))
		fail("aes-cfb, decrypt mismatch entry %d:%d\n", i, j);
	}

	gcry_cipher_close(hde);
	gcry_cipher_close(hdd);
    }
    rpmlog(RPMLOG_INFO, "  Completed CFB checks.\n");
}

static void check_ofb_cipher(pgpDig dig)
{
  struct tv {
    int algo;
    char key[MAX_DATA_LEN];
    char iv[MAX_DATA_LEN];
    struct data {
      unsigned char plaintext[MAX_DATA_LEN];
      int inlen;
      char out[MAX_DATA_LEN];
    }
    data[MAX_DATA_LEN];
  } tv[] =
    {
      /* http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf */
      { PGPSYMKEYALGO_AES_128,
        "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\x3b\x3f\xd9\x2e\xb7\x2d\xad\x20\x33\x34\x49\xf8\xe8\x3c\xfb\x4a" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\x77\x89\x50\x8d\x16\x91\x8f\x03\xf5\x3c\x52\xda\xc5\x4e\xd8\x25"},
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\x97\x40\x05\x1e\x9c\x5f\xec\xf6\x43\x44\xf7\xa8\x22\x60\xed\xcc" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\x30\x4c\x65\x28\xf6\x59\xc7\x78\x66\xa5\x10\xd9\xc1\xd6\xae\x5e" },
        }
      },
      { PGPSYMKEYALGO_AES_192,
        "\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b"
        "\x80\x90\x79\xe5\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\xcd\xc8\x0d\x6f\xdd\xf1\x8c\xab\x34\xc2\x59\x09\xc9\x9a\x41\x74" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\xfc\xc2\x8b\x8d\x4c\x63\x83\x7c\x09\xe8\x17\x00\xc1\x10\x04\x01" },
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\x8d\x9a\x9a\xea\xc0\xf6\x59\x6f\x55\x9c\x6d\x4d\xaf\x59\xa5\xf2" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\x6d\x9f\x20\x08\x57\xca\x6c\x3e\x9c\xac\x52\x4b\xd9\xac\xc9\x2a" },
        }
      },
      { PGPSYMKEYALGO_AES_256,
        "\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81"
        "\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        { { "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
            16,
            "\xdc\x7e\x84\xbf\xda\x79\x16\x4b\x7e\xcd\x84\x86\x98\x5d\x38\x60" },
          { "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
            16,
            "\x4f\xeb\xdc\x67\x40\xd2\x0b\x3a\xc8\x8f\x6a\xd8\x2a\x4f\xb0\x8d" },
          { "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef",
            16,
            "\x71\xab\x47\xa0\x86\xe8\x6e\xed\xf3\x9d\x1c\x5b\xba\x97\xc4\x08" },
          { "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
            16,
            "\x01\x26\x14\x1d\x67\xf3\x7b\xe8\x53\x8f\x5a\x8b\xe7\x40\xe4\x84" }
        }
      }
    };
    unsigned char out[MAX_DATA_LEN];
    gcry_cipher_hd_t hdd;
    gcry_cipher_hd_t hde;
    gcry_error_t err = 0;
    int blklen;
    int keylen;
    size_t i;
    int j;

    rpmlog(RPMLOG_INFO, "  Starting OFB checks.\n");

    for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
	rpmlog(RPMLOG_INFO, "    checking OFB mode for %s [%i]\n",
		    gcry_cipher_algo_name(tv[i].algo), tv[i].algo);
	err = gcry_cipher_open(&hde, tv[i].algo, GCRY_CIPHER_MODE_OFB, 0);
	if (!err)
	    err =
		gcry_cipher_open(&hdd, tv[i].algo, GCRY_CIPHER_MODE_OFB,
				 0);
	if (err) {
	    fail("aes-ofb, gcry_cipher_open failed: %s\n",
		 gpg_strerror(err));
	    return;
	}

	keylen = gcry_cipher_get_algo_keylen(tv[i].algo);
	if (!keylen) {
	    fail("aes-ofb, gcry_cipher_get_algo_keylen failed\n");
	    return;
	}

	err = gcry_cipher_setkey(hde, tv[i].key, keylen);
	if (!err)
	    err = gcry_cipher_setkey(hdd, tv[i].key, keylen);
	if (err) {
	    fail("aes-ofb, gcry_cipher_setkey failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	blklen = gcry_cipher_get_algo_blklen(tv[i].algo);
	if (!blklen) {
	    fail("aes-ofb, gcry_cipher_get_algo_blklen failed\n");
	    return;
	}

	err = gcry_cipher_setiv(hde, tv[i].iv, blklen);
	if (!err)
	    err = gcry_cipher_setiv(hdd, tv[i].iv, blklen);
	if (err) {
	    fail("aes-ofb, gcry_cipher_setiv failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	for (j = 0; tv[i].data[j].inlen; j++) {
	    err = gcry_cipher_encrypt(hde, out, MAX_DATA_LEN,
				      tv[i].data[j].plaintext,
				      tv[i].data[j].inlen);
	    if (err) {
		fail("aes-ofb, gcry_cipher_encrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].out, out, tv[i].data[j].inlen))
		fail("aes-ofb, encrypt mismatch entry %d:%d\n", i, j);

	    err =
		gcry_cipher_decrypt(hdd, out, tv[i].data[j].inlen, NULL,
				    0);
	    if (err) {
		fail("aes-ofb, gcry_cipher_decrypt (%d, %d) failed: %s\n",
		     i, j, gpg_strerror(err));
		gcry_cipher_close(hde);
		gcry_cipher_close(hdd);
		return;
	    }

	    if (memcmp(tv[i].data[j].plaintext, out, tv[i].data[j].inlen))
		fail("aes-ofb, decrypt mismatch entry %d:%d\n", i, j);
	}

	err = gcry_cipher_reset(hde);
	if (!err)
	    err = gcry_cipher_reset(hdd);
	if (err) {
	    fail("aes-ofb, gcry_cipher_reset (%d, %d) failed: %s\n",
		 i, j, gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	/* gcry_cipher_reset clears the IV */
	err = gcry_cipher_setiv(hde, tv[i].iv, blklen);
	if (!err)
	    err = gcry_cipher_setiv(hdd, tv[i].iv, blklen);
	if (err) {
	    fail("aes-ofb, gcry_cipher_setiv failed: %s\n",
		 gpg_strerror(err));
	    gcry_cipher_close(hde);
	    gcry_cipher_close(hdd);
	    return;
	}

	/* this time we encrypt and decrypt one byte at a time */
	for (j = 0; tv[i].data[j].inlen; j++) {
	    int byteNum;
	    for (byteNum = 0; byteNum < tv[i].data[j].inlen; ++byteNum) {
		err = gcry_cipher_encrypt(hde, out + byteNum, 1,
					  (tv[i].data[j].plaintext) +
					  byteNum, 1);
		if (err) {
		    fail("aes-ofb, gcry_cipher_encrypt (%d, %d) failed: %s\n", i, j, gpg_strerror(err));
		    gcry_cipher_close(hde);
		    gcry_cipher_close(hdd);
		    return;
		}
	    }

	    if (memcmp(tv[i].data[j].out, out, tv[i].data[j].inlen))
		fail("aes-ofb, encrypt mismatch entry %d:%d\n", i, j);

	    for (byteNum = 0; byteNum < tv[i].data[j].inlen; ++byteNum) {
		err = gcry_cipher_decrypt(hdd, out + byteNum, 1, NULL, 0);
		if (err) {
		    fail("aes-ofb, gcry_cipher_decrypt (%d, %d) failed: %s\n", i, j, gpg_strerror(err));
		    gcry_cipher_close(hde);
		    gcry_cipher_close(hdd);
		    return;
		}
	    }

	    if (memcmp(tv[i].data[j].plaintext, out, tv[i].data[j].inlen))
		fail("aes-ofb, decrypt mismatch entry %d:%d\n", i, j);
	}

	gcry_cipher_close(hde);
	gcry_cipher_close(hdd);
    }
    rpmlog(RPMLOG_INFO, "  Completed OFB checks.\n");
}

static void check_one_cipher(pgpDig dig, int algo, int mode, int flags)
{
    gcry_cipher_hd_t hd;
    gcry_error_t err = 0;
    char key[32];
    unsigned char plain[16];
    unsigned char out[16];
    unsigned char in[16];
    int keylen;

    memcpy(key, "0123456789abcdef.,;/[]{}-=ABCDEF", 32);
    memcpy(plain, "foobar42FOOBAR17", 16);

    keylen = gcry_cipher_get_algo_keylen(algo);
    if (!keylen) {
	fail("algo %d, mode %d, gcry_cipher_get_algo_keylen failed\n",
	     algo, mode);
	return;
    }

    if (keylen < 40 / 8 || keylen > 32) {
	fail("algo %d, mode %d, keylength problem (%d)\n", algo, mode,
	     keylen);
	return;
    }

    err = gcry_cipher_open(&hd, algo, mode, flags);
    if (err) {
	fail("algo %d, mode %d, gcry_cipher_open failed: %s\n",
	     algo, mode, gpg_strerror(err));
	return;
    }

    err = gcry_cipher_setkey(hd, key, keylen);
    if (err) {
	fail("algo %d, mode %d, gcry_cipher_setkey failed: %s\n",
	     algo, mode, gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    err = gcry_cipher_encrypt(hd, out, 16, plain, 16);
    if (err) {
	fail("algo %d, mode %d, gcry_cipher_encrypt failed: %s\n",
	     algo, mode, gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    gcry_cipher_reset(hd);

    err = gcry_cipher_decrypt(hd, in, 16, out, 16);
    if (err) {
	fail("algo %d, mode %d, gcry_cipher_decrypt failed: %s\n",
	     algo, mode, gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    if (memcmp(plain, in, 16))
	fail("algo %d, mode %d, encrypt-decrypt mismatch\n", algo, mode);

    /* Again, using in-place encryption.  */
    gcry_cipher_reset(hd);

    memcpy(out, plain, 16);
    err = gcry_cipher_encrypt(hd, out, 16, NULL, 0);
    if (err) {
	fail("algo %d, mode %d, in-place, gcry_cipher_encrypt failed: %s\n", algo, mode, gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    gcry_cipher_reset(hd);

    err = gcry_cipher_decrypt(hd, out, 16, NULL, 0);
    if (err) {
	fail("algo %d, mode %d, in-place, gcry_cipher_decrypt failed: %s\n", algo, mode, gpg_strerror(err));
	gcry_cipher_close(hd);
	return;
    }

    if (memcmp(plain, out, 16))
	fail("algo %d, mode %d, in-place, encrypt-decrypt mismatch\n",
	     algo, mode);


    gcry_cipher_close(hd);

}

#define USE_AES 1
#define USE_ARCFOUR 1
#define USE_BLOWFISH 1
#define USE_CAMELLIA 1
/* #undef USE_CAPABILITIES */
#define USE_CAST5 1
#define USE_CRC 1
#define USE_DES 1
#define USE_DSA 1
#define USE_ECC 1
#define USE_ELGAMAL 1
/* #undef USE_GNU_PTH */
#define USE_MD4 1
#define USE_MD5 1
/* #undef USE_ONLY_8DOT3 */
/* #undef USE_RANDOM_DAEMON */
#define USE_RFC2268 1
#define USE_RMD160 1
/* #undef USE_RNDEGD */
#define USE_RNDLINUX 1
/* #undef USE_RNDUNIX */
/* #undef USE_RNDW32 */
/* #undef USE_RNDW32CE */
#define USE_RSA 1
#define USE_SEED 1
#define USE_SERPENT 1
#define USE_SHA1 1
#define USE_SHA256 1
#define USE_SHA512 1
#define USE_TIGER 1
#define USE_TWOFISH 1
#define USE_WHIRLPOOL 1

static void check_ciphers(pgpDig dig)
{
    static int algos[] = {
#if USE_BLOWFISH
	PGPSYMKEYALGO_BLOWFISH,
#endif
#if USE_DES
	GCRY_CIPHER_DES,
	PGPSYMKEYALGO_TRIPLE_DES,
#endif
#if USE_CAST5
	PGPSYMKEYALGO_CAST5,
#endif
#if USE_AES
	PGPSYMKEYALGO_AES_128,
	PGPSYMKEYALGO_AES_192,
	PGPSYMKEYALGO_AES_256,
#endif
#if USE_TWOFISH
	PGPSYMKEYALGO_TWOFISH,
	GCRY_CIPHER_TWOFISH128,
#endif
#if USE_SERPENT
	GCRY_CIPHER_SERPENT128,
	GCRY_CIPHER_SERPENT192,
	GCRY_CIPHER_SERPENT256,
#endif
#if USE_RFC2268
	GCRY_CIPHER_RFC2268_40,
#endif
#if USE_SEED
	GCRY_CIPHER_SEED,
#endif
#if USE_CAMELLIA
	GCRY_CIPHER_CAMELLIA128,
	GCRY_CIPHER_CAMELLIA192,
	GCRY_CIPHER_CAMELLIA256,
#endif
	0
    };
    static int algos2[] = {
#if USE_ARCFOUR
	GCRY_CIPHER_ARCFOUR,
#endif
	0
    };
    int i;

    rpmlog(RPMLOG_INFO, "Starting Cipher checks.\n");
    for (i = 0; algos[i]; i++) {

	/* Lookup & FIPS check. */
	if (pgpImplAvailableCipher(dig, algos[i]))
	    continue;

	rpmlog(RPMLOG_INFO, "  checking %s [%i]\n",
		    gcry_cipher_algo_name(algos[i]),
		    gcry_cipher_map_name(gcry_cipher_algo_name(algos[i])));

	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_ECB, 0);
	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_CFB, 0);
	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_OFB, 0);
	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_CBC, 0);
	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_CBC,
			 GCRY_CIPHER_CBC_CTS);
	check_one_cipher(dig, algos[i], GCRY_CIPHER_MODE_CTR, 0);
    }

    for (i = 0; algos2[i]; i++) {

	if (pgpImplAvailableCipher(dig, algos[i]))
	    continue;

	rpmlog(RPMLOG_INFO, "  checking `%s'\n",
		    gcry_cipher_algo_name(algos2[i]));

	check_one_cipher(dig, algos2[i], GCRY_CIPHER_MODE_STREAM, 0);
    }
    /* we have now run all cipher's selftests */

    rpmlog(RPMLOG_INFO, "Completed Cipher checks.\n");

    /* TODO: add some extra encryption to test the higher level functions */
}


static void check_cipher_modes(pgpDig dig)
{
    rpmlog(RPMLOG_INFO, "Starting Cipher Mode checks.\n");

    check_aes128_cbc_cts_cipher(dig);
    check_cbc_mac_cipher(dig);
    check_ctr_cipher(dig);
    check_cfb_cipher(dig);
    check_ofb_cipher(dig);

    rpmlog(RPMLOG_INFO, "Completed Cipher Mode checks.\n");
}
#endif	/* _RPMGC_INTERNAL */

/*==============================================================*/

static void
pgpTestDigest(pgpDig dig, int algo, int flags,
		const char *data, int datalen, const char *expect)
{
    DIGEST_CTX nctx = NULL;
    DIGEST_CTX ctx;
    rpmuint8_t * p = NULL;
    size_t plen = 0;
int xx;

    ctx = rpmDigestInit(algo, flags);
    if (ctx == NULL) {
	rpmlog(RPMLOG_ERR, "digest(%d) rpmDigestInit(%d,%d) failed\n",
		algo, flags);
	return;
    }

    rpmlog(RPMLOG_INFO, "  checking %s [%i] for length %d\n",
		    rpmDigestName(ctx), algo,
		    (data[0] == '!' && data[1] == '\0')
			?  1000 * 1000 : datalen);

    /* hash one million times an "a" */
    if (data[0] == '!' && data[1] == '\0') {
	char aaa[1000];
	int i;

	/* Write in odd size chunks so that we test the buffering.  */
	memset(aaa, 'a', sizeof(aaa));
	for (i = 0; i < 1000; i++)
	    xx = rpmDigestUpdate(ctx, aaa, sizeof(aaa));
    } else
	xx = rpmDigestUpdate(ctx, data, datalen);

    nctx = rpmDigestDup(ctx);
    if (nctx == NULL)
	rpmlog(RPMLOG_ERR, "rpmDigestDup failed\n");

    xx = rpmDigestFinal(ctx, &p, &plen, 0);
    if (plen < 1 || plen > 500) {
	rpmlog(RPMLOG_ERR, "digest(%d) length out-of-range\n", algo, plen);
    } else
    if (memcmp(p, expect, plen)) {
	const char * p_str = xstrdup(pgpHexStr(p, plen));
	const char * expected_str = xstrdup(pgpHexStr((rpmuint8_t *)expect, plen));
	rpmlog(RPMLOG_ERR, "\
digest(%d) mismatch\n\
computed: %s\n\
expected: %s\n", algo, p_str, expected_str);
	p_str = _free(p_str);
	expected_str = _free(expected_str);
    }

p = _free(p);
plen = 0;
    xx = rpmDigestFinal(nctx, &p, &plen, 0);
    if (plen < 1 || plen > 500) {
	rpmlog(RPMLOG_ERR, "dup digest(%d) length out-of-range\n", algo, plen);
    } else
    if (memcmp(p, expect, plen)) {
	const char * p_str = xstrdup(pgpHexStr(p, plen));
	const char * expected_str = xstrdup(pgpHexStr((rpmuint8_t *)expect, plen));
	rpmlog(RPMLOG_ERR, "\
dup digest(%d) mismatch\n\
computed: %s\n\
expected: %s\n", algo, p_str, expected_str);
	p_str = _free(p_str);
	expected_str = _free(expected_str);
    }
p = _free(p);
plen = 0;

}

static void pgpTestDigests(pgpDig dig)
{
  static struct algos {
    int md;
    const char *data;
    const char *expect;
  } algos[] =
    {
      { PGPHASHALGO_MD4, "",
	"\x31\xD6\xCF\xE0\xD1\x6A\xE9\x31\xB7\x3C\x59\xD7\xE0\xC0\x89\xC0" },
      { PGPHASHALGO_MD4, "a",
	"\xbd\xe5\x2c\xb3\x1d\xe3\x3e\x46\x24\x5e\x05\xfb\xdb\xd6\xfb\x24" },
      {	PGPHASHALGO_MD4, "message digest",
	"\xd9\x13\x0a\x81\x64\x54\x9f\xe8\x18\x87\x48\x06\xe1\xc7\x01\x4b" },
      {	PGPHASHALGO_MD5, "",
	"\xD4\x1D\x8C\xD9\x8F\x00\xB2\x04\xE9\x80\x09\x98\xEC\xF8\x42\x7E" },
      {	PGPHASHALGO_MD5, "a",
	"\x0C\xC1\x75\xB9\xC0\xF1\xB6\xA8\x31\xC3\x99\xE2\x69\x77\x26\x61" },
      { PGPHASHALGO_MD5, "abc",
	"\x90\x01\x50\x98\x3C\xD2\x4F\xB0\xD6\x96\x3F\x7D\x28\xE1\x7F\x72" },
      { PGPHASHALGO_MD5, "message digest",
	"\xF9\x6B\x69\x7D\x7C\xB7\x93\x8D\x52\x5A\x2F\x31\xAA\xF1\x61\xD0" },
      { PGPHASHALGO_SHA1, "abc",
	"\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E"
	"\x25\x71\x78\x50\xC2\x6C\x9C\xD0\xD8\x9D" },
      {	PGPHASHALGO_SHA1,
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	"\x84\x98\x3E\x44\x1C\x3B\xD2\x6E\xBA\xAE"
	"\x4A\xA1\xF9\x51\x29\xE5\xE5\x46\x70\xF1" },
      { PGPHASHALGO_SHA1, "!" /* kludge for "a"*1000000 */ ,
	"\x34\xAA\x97\x3C\xD4\xC4\xDA\xA4\xF6\x1E"
	"\xEB\x2B\xDB\xAD\x27\x31\x65\x34\x01\x6F" },
      /* From RFC3874 */
      {	PGPHASHALGO_SHA224, "abc",
	"\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55\xb3"
	"\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7" },
      {	PGPHASHALGO_SHA224,
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	"\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01\x50"
	"\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25" },
      {	PGPHASHALGO_SHA224, "!",
	"\x20\x79\x46\x55\x98\x0c\x91\xd8\xbb\xb4\xc1\xea\x97\x61\x8a\x4b"
	"\xf0\x3f\x42\x58\x19\x48\xb2\xee\x4e\xe7\xad\x67" },
      {	PGPHASHALGO_SHA256, "abc",
	"\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23"
	"\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad" },
      {	PGPHASHALGO_SHA256,
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	"\x24\x8d\x6a\x61\xd2\x06\x38\xb8\xe5\xc0\x26\x93\x0c\x3e\x60\x39"
	"\xa3\x3c\xe4\x59\x64\xff\x21\x67\xf6\xec\xed\xd4\x19\xdb\x06\xc1" },
      {	PGPHASHALGO_SHA256, "!",
	"\xcd\xc7\x6e\x5c\x99\x14\xfb\x92\x81\xa1\xc7\xe2\x84\xd7\x3e\x67"
	"\xf1\x80\x9a\x48\xa4\x97\x20\x0e\x04\x6d\x39\xcc\xc7\x11\x2c\xd0" },
      {	PGPHASHALGO_SHA384, "abc",
	"\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07"
	"\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed"
	"\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7" },
      {	PGPHASHALGO_SHA512, "abc",
	"\xDD\xAF\x35\xA1\x93\x61\x7A\xBA\xCC\x41\x73\x49\xAE\x20\x41\x31"
	"\x12\xE6\xFA\x4E\x89\xA9\x7E\xA2\x0A\x9E\xEE\xE6\x4B\x55\xD3\x9A"
	"\x21\x92\x99\x2A\x27\x4F\xC1\xA8\x36\xBA\x3C\x23\xA3\xFE\xEB\xBD"
	"\x45\x4D\x44\x23\x64\x3C\xE8\x0E\x2A\x9A\xC9\x4F\xA5\x4C\xA4\x9F" },
      {	PGPHASHALGO_RIPEMD160, "",
	"\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28"
	"\x08\x97\x7e\xe8\xf5\x48\xb2\x25\x8d\x31" },
      {	PGPHASHALGO_RIPEMD160, "a",
	"\x0b\xdc\x9d\x2d\x25\x6b\x3e\xe9\xda\xae"
	"\x34\x7b\xe6\xf4\xdc\x83\x5a\x46\x7f\xfe" },
      {	PGPHASHALGO_RIPEMD160, "abc",
	"\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04"
	"\x4a\x8e\x98\xc6\xb0\x87\xf1\x5a\x0b\xfc" },
      {	PGPHASHALGO_RIPEMD160, "message digest",
	"\x5d\x06\x89\xef\x49\xd2\xfa\xe5\x72\xb8"
	"\x81\xb1\x23\xa8\x5f\xfa\x21\x59\x5f\x36" },
      {	PGPHASHALGO_CRC32, "", "\x00\x00\x00\x00" },
      {	PGPHASHALGO_CRC32, "foo", "\x8c\x73\x65\x21" },

#ifdef	NOTYET
      { GCRY_MD_CRC32_RFC1510, "", "\x00\x00\x00\x00" },
      {	GCRY_MD_CRC32_RFC1510, "foo", "\x73\x32\xbc\x33" },
      {	GCRY_MD_CRC32_RFC1510, "test0123456789", "\xb8\x3e\x88\xd6" },
      {	GCRY_MD_CRC32_RFC1510, "MASSACHVSETTS INSTITVTE OF TECHNOLOGY",
	"\xe3\x41\x80\xf7" },
#if 0
      {	GCRY_MD_CRC32_RFC1510, "\x80\x00", "\x3b\x83\x98\x4b" },
      {	GCRY_MD_CRC32_RFC1510, "\x00\x08", "\x0e\xdb\x88\x32" },
      {	GCRY_MD_CRC32_RFC1510, "\x00\x80", "\xed\xb8\x83\x20" },
#endif
      {	GCRY_MD_CRC32_RFC1510, "\x80", "\xed\xb8\x83\x20" },
#if 0
      {	GCRY_MD_CRC32_RFC1510, "\x80\x00\x00\x00", "\xed\x59\xb6\x3b" },
      {	GCRY_MD_CRC32_RFC1510, "\x00\x00\x00\x01", "\x77\x07\x30\x96" },
#endif
      {	GCRY_MD_CRC24_RFC2440, "", "\xb7\x04\xce" },
      {	GCRY_MD_CRC24_RFC2440, "foo", "\x4f\xc2\x55" },
#endif	/* NOTYET */

#ifdef	NOTNOW	/* XXX RFC 2440/4880 mismatch */
/* This is the old TIGER variant based on the unfixed reference
 *    implementation.  IT was used in GnupG up to 1.3.2.  We don't provide
 *       an OID anymore because that would not be correct.
 */

      {	GCRY_MD_TIGER, "",
	"\x24\xF0\x13\x0C\x63\xAC\x93\x32\x16\x16\x6E\x76"
	"\xB1\xBB\x92\x5F\xF3\x73\xDE\x2D\x49\x58\x4E\x7A" },
      {	GCRY_MD_TIGER, "abc",
	"\xF2\x58\xC1\xE8\x84\x14\xAB\x2A\x52\x7A\xB5\x41"
	"\xFF\xC5\xB8\xBF\x93\x5F\x7B\x95\x1C\x13\x29\x51" },
      {	GCRY_MD_TIGER, "Tiger",
	"\x9F\x00\xF5\x99\x07\x23\x00\xDD\x27\x6A\xBB\x38"
	"\xC8\xEB\x6D\xEC\x37\x79\x0C\x11\x6F\x9D\x2B\xDF" },
      {	GCRY_MD_TIGER, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefg"
	"hijklmnopqrstuvwxyz0123456789+-",
	"\x87\xFB\x2A\x90\x83\x85\x1C\xF7\x47\x0D\x2C\xF8"
	"\x10\xE6\xDF\x9E\xB5\x86\x44\x50\x34\xA5\xA3\x86" },
      {	GCRY_MD_TIGER, "ABCDEFGHIJKLMNOPQRSTUVWXYZ=abcdef"
	"ghijklmnopqrstuvwxyz+0123456789",
	"\x46\x7D\xB8\x08\x63\xEB\xCE\x48\x8D\xF1\xCD\x12"
	"\x61\x65\x5D\xE9\x57\x89\x65\x65\x97\x5F\x91\x97" },
      {	GCRY_MD_TIGER, "Tiger - A Fast New Hash Function, "
	"by Ross Anderson and Eli Biham",
	"\x0C\x41\x0A\x04\x29\x68\x86\x8A\x16\x71\xDA\x5A"
	"\x3F\xD2\x9A\x72\x5E\xC1\xE4\x57\xD3\xCD\xB3\x03" },
      {	GCRY_MD_TIGER, "Tiger - A Fast New Hash Function, "
	"by Ross Anderson and Eli Biham, proceedings of Fa"
	"st Software Encryption 3, Cambridge.",
	"\xEB\xF5\x91\xD5\xAF\xA6\x55\xCE\x7F\x22\x89\x4F"
	"\xF8\x7F\x54\xAC\x89\xC8\x11\xB6\xB0\xDA\x31\x93" },
      {	GCRY_MD_TIGER, "Tiger - A Fast New Hash Function, "
	"by Ross Anderson and Eli Biham, proceedings of Fa"
	"st Software Encryption 3, Cambridge, 1996.",
	"\x3D\x9A\xEB\x03\xD1\xBD\x1A\x63\x57\xB2\x77\x4D"
	"\xFD\x6D\x5B\x24\xDD\x68\x15\x1D\x50\x39\x74\xFC" },
      {	GCRY_MD_TIGER, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefgh"
	"ijklmnopqrstuvwxyz0123456789+-ABCDEFGHIJKLMNOPQRS"
	"TUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-",
	"\x00\xB8\x3E\xB4\xE5\x34\x40\xC5\x76\xAC\x6A\xAE"
	"\xE0\xA7\x48\x58\x25\xFD\x15\xE7\x0A\x59\xFF\xE4" },
#endif	/* NOTNOW */

      {	PGPHASHALGO_TIGER192, "",
        "\x32\x93\xAC\x63\x0C\x13\xF0\x24\x5F\x92\xBB\xB1"
        "\x76\x6E\x16\x16\x7A\x4E\x58\x49\x2D\xDE\x73\xF3" },
      {	PGPHASHALGO_TIGER192, "a",
	"\x77\xBE\xFB\xEF\x2E\x7E\xF8\xAB\x2E\xC8\xF9\x3B"
        "\xF5\x87\xA7\xFC\x61\x3E\x24\x7F\x5F\x24\x78\x09" },
      {	PGPHASHALGO_TIGER192, "abc",
        "\x2A\xAB\x14\x84\xE8\xC1\x58\xF2\xBF\xB8\xC5\xFF"
        "\x41\xB5\x7A\x52\x51\x29\x13\x1C\x95\x7B\x5F\x93" },
      {	PGPHASHALGO_TIGER192, "message digest",
	"\xD9\x81\xF8\xCB\x78\x20\x1A\x95\x0D\xCF\x30\x48"
        "\x75\x1E\x44\x1C\x51\x7F\xCA\x1A\xA5\x5A\x29\xF6" },
      {	PGPHASHALGO_TIGER192, "abcdefghijklmnopqrstuvwxyz",
	"\x17\x14\xA4\x72\xEE\xE5\x7D\x30\x04\x04\x12\xBF"
        "\xCC\x55\x03\x2A\x0B\x11\x60\x2F\xF3\x7B\xEE\xE9" },
      {	PGPHASHALGO_TIGER192, 
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	"\x0F\x7B\xF9\xA1\x9B\x9C\x58\xF2\xB7\x61\x0D\xF7"
        "\xE8\x4F\x0A\xC3\xA7\x1C\x63\x1E\x7B\x53\xF7\x8E" },
      {	PGPHASHALGO_TIGER192, 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz" "0123456789",
        "\x8D\xCE\xA6\x80\xA1\x75\x83\xEE\x50\x2B\xA3\x8A"
        "\x3C\x36\x86\x51\x89\x0F\xFB\xCC\xDC\x49\xA8\xCC" },
      {	PGPHASHALGO_TIGER192, 
        "1234567890" "1234567890" "1234567890" "1234567890"
        "1234567890" "1234567890" "1234567890" "1234567890",
        "\x1C\x14\x79\x55\x29\xFD\x9F\x20\x7A\x95\x8F\x84"
        "\xC5\x2F\x11\xE8\x87\xFA\x0C\xAB\xDF\xD9\x1B\xFD" },
      {	PGPHASHALGO_TIGER192, "!",
	"\x6D\xB0\xE2\x72\x9C\xBE\xAD\x93\xD7\x15\xC6\xA7"
        "\xD3\x63\x02\xE9\xB3\xCE\xE0\xD2\xBC\x31\x4B\x41" },

#ifdef	NOTYET
      {	GCRY_MD_TIGER2, "",
        "\x44\x41\xBE\x75\xF6\x01\x87\x73\xC2\x06\xC2\x27"
        "\x45\x37\x4B\x92\x4A\xA8\x31\x3F\xEF\x91\x9F\x41" },
      {	GCRY_MD_TIGER2, "a",
        "\x67\xE6\xAE\x8E\x9E\x96\x89\x99\xF7\x0A\x23\xE7"
        "\x2A\xEA\xA9\x25\x1C\xBC\x7C\x78\xA7\x91\x66\x36" },
      {	GCRY_MD_TIGER2, "abc",
        "\xF6\x8D\x7B\xC5\xAF\x4B\x43\xA0\x6E\x04\x8D\x78"
        "\x29\x56\x0D\x4A\x94\x15\x65\x8B\xB0\xB1\xF3\xBF" },
      {	GCRY_MD_TIGER2, "message digest",
        "\xE2\x94\x19\xA1\xB5\xFA\x25\x9D\xE8\x00\x5E\x7D"
        "\xE7\x50\x78\xEA\x81\xA5\x42\xEF\x25\x52\x46\x2D" },
      {	GCRY_MD_TIGER2, "abcdefghijklmnopqrstuvwxyz",
        "\xF5\xB6\xB6\xA7\x8C\x40\x5C\x85\x47\xE9\x1C\xD8"
        "\x62\x4C\xB8\xBE\x83\xFC\x80\x4A\x47\x44\x88\xFD" },
      {	GCRY_MD_TIGER2, 
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        "\xA6\x73\x7F\x39\x97\xE8\xFB\xB6\x3D\x20\xD2\xDF"
        "\x88\xF8\x63\x76\xB5\xFE\x2D\x5C\xE3\x66\x46\xA9" },
      {	GCRY_MD_TIGER2, 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz" "0123456789",
        "\xEA\x9A\xB6\x22\x8C\xEE\x7B\x51\xB7\x75\x44\xFC"
        "\xA6\x06\x6C\x8C\xBB\x5B\xBA\xE6\x31\x95\x05\xCD" },
      {	GCRY_MD_TIGER2, 
        "1234567890" "1234567890" "1234567890" "1234567890"
        "1234567890" "1234567890" "1234567890" "1234567890",
        "\xD8\x52\x78\x11\x53\x29\xEB\xAA\x0E\xEC\x85\xEC"
        "\xDC\x53\x96\xFD\xA8\xAA\x3A\x58\x20\x94\x2F\xFF" },
      {	GCRY_MD_TIGER2, "!",
        "\xE0\x68\x28\x1F\x06\x0F\x55\x16\x28\xCC\x57\x15"
        "\xB9\xD0\x22\x67\x96\x91\x4D\x45\xF7\x71\x7C\xF4" },

      { GCRY_MD_WHIRLPOOL, "",
	"\x19\xFA\x61\xD7\x55\x22\xA4\x66\x9B\x44\xE3\x9C\x1D\x2E\x17\x26"
	"\xC5\x30\x23\x21\x30\xD4\x07\xF8\x9A\xFE\xE0\x96\x49\x97\xF7\xA7"
	"\x3E\x83\xBE\x69\x8B\x28\x8F\xEB\xCF\x88\xE3\xE0\x3C\x4F\x07\x57"
	"\xEA\x89\x64\xE5\x9B\x63\xD9\x37\x08\xB1\x38\xCC\x42\xA6\x6E\xB3" },
      { GCRY_MD_WHIRLPOOL, "a",
	"\x8A\xCA\x26\x02\x79\x2A\xEC\x6F\x11\xA6\x72\x06\x53\x1F\xB7\xD7"
	"\xF0\xDF\xF5\x94\x13\x14\x5E\x69\x73\xC4\x50\x01\xD0\x08\x7B\x42"
	"\xD1\x1B\xC6\x45\x41\x3A\xEF\xF6\x3A\x42\x39\x1A\x39\x14\x5A\x59"
	"\x1A\x92\x20\x0D\x56\x01\x95\xE5\x3B\x47\x85\x84\xFD\xAE\x23\x1A" },
      { GCRY_MD_WHIRLPOOL, "a",
	"\x8A\xCA\x26\x02\x79\x2A\xEC\x6F\x11\xA6\x72\x06\x53\x1F\xB7\xD7"
	"\xF0\xDF\xF5\x94\x13\x14\x5E\x69\x73\xC4\x50\x01\xD0\x08\x7B\x42"
	"\xD1\x1B\xC6\x45\x41\x3A\xEF\xF6\x3A\x42\x39\x1A\x39\x14\x5A\x59"
	"\x1A\x92\x20\x0D\x56\x01\x95\xE5\x3B\x47\x85\x84\xFD\xAE\x23\x1A" },
      { GCRY_MD_WHIRLPOOL,
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
	"\xDC\x37\xE0\x08\xCF\x9E\xE6\x9B\xF1\x1F\x00\xED\x9A\xBA\x26\x90"
	"\x1D\xD7\xC2\x8C\xDE\xC0\x66\xCC\x6A\xF4\x2E\x40\xF8\x2F\x3A\x1E"
	"\x08\xEB\xA2\x66\x29\x12\x9D\x8F\xB7\xCB\x57\x21\x1B\x92\x81\xA6"
	"\x55\x17\xCC\x87\x9D\x7B\x96\x21\x42\xC6\x5F\x5A\x7A\xF0\x14\x67" },
      { GCRY_MD_WHIRLPOOL,
	"!",
	"\x0C\x99\x00\x5B\xEB\x57\xEF\xF5\x0A\x7C\xF0\x05\x56\x0D\xDF\x5D"
	"\x29\x05\x7F\xD8\x6B\x20\xBF\xD6\x2D\xEC\xA0\xF1\xCC\xEA\x4A\xF5"
	"\x1F\xC1\x54\x90\xED\xDC\x47\xAF\x32\xBB\x2B\x66\xC3\x4F\xF9\xAD"
	"\x8C\x60\x08\xAD\x67\x7F\x77\x12\x69\x53\xB2\x26\xE4\xED\x8B\x01" },
#endif	/* NOTYET */
      {	0 },
    };
    int i;

    rpmlog(RPMLOG_INFO, "Starting hash checks.\n");

    for (i = 0; algos[i].md; i++) {

	/* Lookup & FIPS check. */
	if (pgpImplAvailableDigest(dig, algos[i].md))
	    continue;

	pgpTestDigest(dig, algos[i].md, 0,
		algos[i].data, strlen(algos[i].data), algos[i].expect);

    }

    rpmlog(RPMLOG_INFO, "Completed hash checks.\n");
}

static void
pgpTestHMAC(pgpDig dig, int algo, int flags, const char *data, int datalen,
	       const char *key, int keylen, const char *expect)
{
    DIGEST_CTX nctx = NULL;
    DIGEST_CTX ctx;
    rpmuint8_t * p = NULL;
    size_t plen = 0;
int xx;

    ctx = rpmDigestInit(algo, flags);
    if (ctx == NULL) {
	rpmlog(RPMLOG_ERR, "digest(%d) rpmDigestInit(%d,%d) failed\n",
		algo, flags);
	return;
    }

    rpmlog(RPMLOG_INFO, "  checking %s [%i] for %d byte key and %d byte data\n",
		    rpmDigestName(ctx), algo, keylen,
		    (data[0] == '!' && data[1] == '\0')
			?  1000 * 1000 : datalen);

    xx = rpmHmacInit(ctx, key, keylen);

    /* hash one million times an "a" */
    if (data[0] == '!' && data[1] == '\0') {
	char aaa[1000];
	int i;

	/* Write in odd size chunks so that we test the buffering.  */
	memset(aaa, 'a', sizeof(aaa));
	for (i = 0; i < 1000; i++)
	    xx = rpmDigestUpdate(ctx, aaa, sizeof(aaa));
    } else
	xx = rpmDigestUpdate(ctx, data, datalen);

    nctx = rpmDigestDup(ctx);
    if (nctx == NULL)
	rpmlog(RPMLOG_ERR, "rpmDigestDup failed\n");

    xx = rpmDigestFinal(ctx, &p, &plen, 0);
    if (plen < 1 || plen > 500) {
	rpmlog(RPMLOG_ERR, "digest(%d) length out-of-range\n", algo, plen);
    } else
    if (memcmp(p, expect, plen)) {
	const char * p_str = xstrdup(pgpHexStr(p, plen));
	const char * expected_str = xstrdup(pgpHexStr((rpmuint8_t *)expect, plen));
	rpmlog(RPMLOG_ERR, "\
digest(%d) mismatch\n\
computed: %s\n\
expected: %s\n", algo, p_str, expected_str);
	p_str = _free(p_str);
	expected_str = _free(expected_str);
    }

p = _free(p);
plen = 0;
    xx = rpmDigestFinal(nctx, &p, &plen, 0);
    if (plen < 1 || plen > 500) {
	rpmlog(RPMLOG_ERR, "dup digest(%d) length out-of-range\n", algo, plen);
    } else
    if (memcmp(p, expect, plen)) {
	const char * p_str = xstrdup(pgpHexStr(p, plen));
	const char * expected_str = xstrdup(pgpHexStr((rpmuint8_t *)expect, plen));
	rpmlog(RPMLOG_ERR, "\
dup digest(%d) mismatch\n\
computed: %s\n\
expected: %s\n", algo, p_str, expected_str);
	p_str = _free(p_str);
	expected_str = _free(expected_str);
    }
p = _free(p);
plen = 0;

}

static void pgpTestHMACS(pgpDig dig)
{
  static struct algos
  {
    int md;
    const char *data;
    const char *key;
    const char *expect;
  } algos[] =
    {
      { PGPHASHALGO_MD5, "what do ya want for nothing?", "Jefe",
	"\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7\x38" },
      { PGPHASHALGO_MD5,
	"Hi There",
	"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
	"\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc\x9d" },
      { PGPHASHALGO_MD5,
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd",
	"\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA",
	"\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3\xf6" },
      { PGPHASHALGO_MD5,
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd",
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
	"\x69\x7e\xaf\x0a\xca\x3a\x3a\xea\x3a\x75\x16\x47\x46\xff\xaa\x79" },
      { PGPHASHALGO_MD5, "Test With Truncation",
	"\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c",
	"\x56\x46\x1e\xf2\x34\x2e\xdc\x00\xf9\xba\xb9\x95\x69\x0e\xfd\x4c" },
      { PGPHASHALGO_MD5, "Test Using Larger Than Block-Size Key - Hash Key First",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa",
	"\x6b\x1a\xb7\xfe\x4b\xd7\xbf\x8f\x0b\x62\xe6\xce\x61\xb9\xd0\xcd" },
      { PGPHASHALGO_MD5,
	"Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa",
	"\x6f\x63\x0f\xad\x67\xcd\xa0\xee\x1f\xb1\xf5\x62\xdb\x3a\xa5\x3e", },
      { PGPHASHALGO_SHA256, "what do ya want for nothing?", "Jefe",
	"\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7\x5a"
	"\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43" },
      { PGPHASHALGO_SHA256,
	"Hi There",
	"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
	"\x0b\x0b\x0b",
	"\xb0\x34\x4c\x61\xd8\xdb\x38\x53\x5c\xa8\xaf\xce\xaf\x0b\xf1\x2b\x88"
	"\x1d\xc2\x00\xc9\x83\x3d\xa7\x26\xe9\x37\x6c\x2e\x32\xcf\xf7" },
      { PGPHASHALGO_SHA256,
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd",
	"\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
	"\xAA\xAA\xAA\xAA",
	"\x77\x3e\xa9\x1e\x36\x80\x0e\x46\x85\x4d\xb8\xeb\xd0\x91\x81\xa7"
	"\x29\x59\x09\x8b\x3e\xf8\xc1\x22\xd9\x63\x55\x14\xce\xd5\x65\xfe" },
      { PGPHASHALGO_SHA256,
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd",
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
	"\x82\x55\x8a\x38\x9a\x44\x3c\x0e\xa4\xcc\x81\x98\x99\xf2\x08"
	"\x3a\x85\xf0\xfa\xa3\xe5\x78\xf8\x07\x7a\x2e\x3f\xf4\x67\x29\x66\x5b" },
      { PGPHASHALGO_SHA256, 
	"Test Using Larger Than Block-Size Key - Hash Key First",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x60\xe4\x31\x59\x1e\xe0\xb6\x7f\x0d\x8a\x26\xaa\xcb\xf5\xb7\x7f"
	"\x8e\x0b\xc6\x21\x37\x28\xc5\x14\x05\x46\x04\x0f\x0e\xe3\x7f\x54" },
      { PGPHASHALGO_SHA256, 
	"This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x9b\x09\xff\xa7\x1b\x94\x2f\xcb\x27\x63\x5f\xbc\xd5\xb0\xe9\x44"
	"\xbf\xdc\x63\x64\x4f\x07\x13\x93\x8a\x7f\x51\x53\x5c\x3a\x35\xe2" },
      { PGPHASHALGO_SHA224, "what do ya want for nothing?", "Jefe",
	"\xa3\x0e\x01\x09\x8b\xc6\xdb\xbf\x45\x69\x0f\x3a\x7e\x9e\x6d\x0f"
	"\x8b\xbe\xa2\xa3\x9e\x61\x48\x00\x8f\xd0\x5e\x44" },
      { PGPHASHALGO_SHA224,
	"Hi There",
	"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
	"\x0b\x0b\x0b",
	"\x89\x6f\xb1\x12\x8a\xbb\xdf\x19\x68\x32\x10\x7c\xd4\x9d\xf3\x3f\x47"
	"\xb4\xb1\x16\x99\x12\xba\x4f\x53\x68\x4b\x22" },
      { PGPHASHALGO_SHA224,
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd",
	"\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
	"\xAA\xAA\xAA\xAA",
	"\x7f\xb3\xcb\x35\x88\xc6\xc1\xf6\xff\xa9\x69\x4d\x7d\x6a\xd2\x64"
	"\x93\x65\xb0\xc1\xf6\x5d\x69\xd1\xec\x83\x33\xea" },
      { PGPHASHALGO_SHA224,
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd",
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
	"\x6c\x11\x50\x68\x74\x01\x3c\xac\x6a\x2a\xbc\x1b\xb3\x82\x62"
	"\x7c\xec\x6a\x90\xd8\x6e\xfc\x01\x2d\xe7\xaf\xec\x5a" },
      { PGPHASHALGO_SHA224, 
	"Test Using Larger Than Block-Size Key - Hash Key First",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x95\xe9\xa0\xdb\x96\x20\x95\xad\xae\xbe\x9b\x2d\x6f\x0d\xbc\xe2"
	"\xd4\x99\xf1\x12\xf2\xd2\xb7\x27\x3f\xa6\x87\x0e" },
      { PGPHASHALGO_SHA224, 
	"This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x3a\x85\x41\x66\xac\x5d\x9f\x02\x3f\x54\xd5\x17\xd0\xb3\x9d\xbd"
	"\x94\x67\x70\xdb\x9c\x2b\x95\xc9\xf6\xf5\x65\xd1" },
      { PGPHASHALGO_SHA384, "what do ya want for nothing?", "Jefe",
	"\xaf\x45\xd2\xe3\x76\x48\x40\x31\x61\x7f\x78\xd2\xb5\x8a\x6b\x1b"
	"\x9c\x7e\xf4\x64\xf5\xa0\x1b\x47\xe4\x2e\xc3\x73\x63\x22\x44\x5e"
	"\x8e\x22\x40\xca\x5e\x69\xe2\xc7\x8b\x32\x39\xec\xfa\xb2\x16\x49" },
      { PGPHASHALGO_SHA384,
	"Hi There",
	"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
	"\x0b\x0b\x0b",
	"\xaf\xd0\x39\x44\xd8\x48\x95\x62\x6b\x08\x25\xf4\xab\x46\x90\x7f\x15"
	"\xf9\xda\xdb\xe4\x10\x1e\xc6\x82\xaa\x03\x4c\x7c\xeb\xc5\x9c\xfa\xea"
	"\x9e\xa9\x07\x6e\xde\x7f\x4a\xf1\x52\xe8\xb2\xfa\x9c\xb6" },
      { PGPHASHALGO_SHA384,
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd",
	"\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
	"\xAA\xAA\xAA\xAA",
	"\x88\x06\x26\x08\xd3\xe6\xad\x8a\x0a\xa2\xac\xe0\x14\xc8\xa8\x6f"
	"\x0a\xa6\x35\xd9\x47\xac\x9f\xeb\xe8\x3e\xf4\xe5\x59\x66\x14\x4b"
	"\x2a\x5a\xb3\x9d\xc1\x38\x14\xb9\x4e\x3a\xb6\xe1\x01\xa3\x4f\x27" },
      { PGPHASHALGO_SHA384,
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd",
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
	"\x3e\x8a\x69\xb7\x78\x3c\x25\x85\x19\x33\xab\x62\x90\xaf\x6c\xa7"
	"\x7a\x99\x81\x48\x08\x50\x00\x9c\xc5\x57\x7c\x6e\x1f\x57\x3b\x4e"
	"\x68\x01\xdd\x23\xc4\xa7\xd6\x79\xcc\xf8\xa3\x86\xc6\x74\xcf\xfb" },
      { PGPHASHALGO_SHA384, 
	"Test Using Larger Than Block-Size Key - Hash Key First",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x4e\xce\x08\x44\x85\x81\x3e\x90\x88\xd2\xc6\x3a\x04\x1b\xc5\xb4"
	"\x4f\x9e\xf1\x01\x2a\x2b\x58\x8f\x3c\xd1\x1f\x05\x03\x3a\xc4\xc6"
	"\x0c\x2e\xf6\xab\x40\x30\xfe\x82\x96\x24\x8d\xf1\x63\xf4\x49\x52" },
      { PGPHASHALGO_SHA384, 
	"This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x66\x17\x17\x8e\x94\x1f\x02\x0d\x35\x1e\x2f\x25\x4e\x8f\xd3\x2c"
	"\x60\x24\x20\xfe\xb0\xb8\xfb\x9a\xdc\xce\xbb\x82\x46\x1e\x99\xc5"
	"\xa6\x78\xcc\x31\xe7\x99\x17\x6d\x38\x60\xe6\x11\x0c\x46\x52\x3e" },
      { PGPHASHALGO_SHA512, "what do ya want for nothing?", "Jefe",
	"\x16\x4b\x7a\x7b\xfc\xf8\x19\xe2\xe3\x95\xfb\xe7\x3b\x56\xe0\xa3"
	"\x87\xbd\x64\x22\x2e\x83\x1f\xd6\x10\x27\x0c\xd7\xea\x25\x05\x54"
	"\x97\x58\xbf\x75\xc0\x5a\x99\x4a\x6d\x03\x4f\x65\xf8\xf0\xe6\xfd"
	"\xca\xea\xb1\xa3\x4d\x4a\x6b\x4b\x63\x6e\x07\x0a\x38\xbc\xe7\x37" },
      { PGPHASHALGO_SHA512,
	"Hi There",
	"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
	"\x0b\x0b\x0b",
	"\x87\xaa\x7c\xde\xa5\xef\x61\x9d\x4f\xf0\xb4\x24\x1a\x1d\x6c\xb0"
	"\x23\x79\xf4\xe2\xce\x4e\xc2\x78\x7a\xd0\xb3\x05\x45\xe1\x7c\xde"
	"\xda\xa8\x33\xb7\xd6\xb8\xa7\x02\x03\x8b\x27\x4e\xae\xa3\xf4\xe4"
	"\xbe\x9d\x91\x4e\xeb\x61\xf1\x70\x2e\x69\x6c\x20\x3a\x12\x68\x54" },
      { PGPHASHALGO_SHA512,
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
	"\xdd\xdd\xdd\xdd\xdd",
	"\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
	"\xAA\xAA\xAA\xAA",
	"\xfa\x73\xb0\x08\x9d\x56\xa2\x84\xef\xb0\xf0\x75\x6c\x89\x0b\xe9"
	"\xb1\xb5\xdb\xdd\x8e\xe8\x1a\x36\x55\xf8\x3e\x33\xb2\x27\x9d\x39"
	"\xbf\x3e\x84\x82\x79\xa7\x22\xc8\x06\xb4\x85\xa4\x7e\x67\xc8\x07"
	"\xb9\x46\xa3\x37\xbe\xe8\x94\x26\x74\x27\x88\x59\xe1\x32\x92\xfb"  },
      { PGPHASHALGO_SHA512,
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
	"\xcd\xcd\xcd\xcd\xcd",
	"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
	"\xb0\xba\x46\x56\x37\x45\x8c\x69\x90\xe5\xa8\xc5\xf6\x1d\x4a\xf7"
	"\xe5\x76\xd9\x7f\xf9\x4b\x87\x2d\xe7\x6f\x80\x50\x36\x1e\xe3\xdb"
	"\xa9\x1c\xa5\xc1\x1a\xa2\x5e\xb4\xd6\x79\x27\x5c\xc5\x78\x80\x63"
	"\xa5\xf1\x97\x41\x12\x0c\x4f\x2d\xe2\xad\xeb\xeb\x10\xa2\x98\xdd" },
      { PGPHASHALGO_SHA512, 
	"Test Using Larger Than Block-Size Key - Hash Key First",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\x80\xb2\x42\x63\xc7\xc1\xa3\xeb\xb7\x14\x93\xc1\xdd\x7b\xe8\xb4"
	"\x9b\x46\xd1\xf4\x1b\x4a\xee\xc1\x12\x1b\x01\x37\x83\xf8\xf3\x52"
	"\x6b\x56\xd0\x37\xe0\x5f\x25\x98\xbd\x0f\xd2\x21\x5d\x6a\x1e\x52"
	"\x95\xe6\x4f\x73\xf6\x3f\x0a\xec\x8b\x91\x5a\x98\x5d\x78\x65\x98" },
      { PGPHASHALGO_SHA512, 
	"This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.",
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
	"\xaa\xaa\xaa",
	"\xe3\x7b\x6a\x77\x5d\xc8\x7d\xba\xa4\xdf\xa9\xf9\x6e\x5e\x3f\xfd"
	"\xde\xbd\x71\xf8\x86\x72\x89\x86\x5d\xf5\xa3\x2d\x20\xcd\xc9\x44"
	"\xb6\x02\x2c\xac\x3c\x49\x82\xb1\x0d\x5e\xeb\x55\xc3\xe4\xde\x15"
	"\x13\x46\x76\xfb\x6d\xe0\x44\x60\x65\xc9\x74\x40\xfa\x8c\x6a\x58" },
      {	0 },
    };
    int i;

    rpmlog(RPMLOG_INFO, "Starting hashed MAC checks.\n");

    for (i = 0; algos[i].md; i++) {

	/* Lookup & FIPS check. */
	if (pgpImplAvailableDigest(dig, algos[i].md))
	    continue;

	pgpTestHMAC(dig, algos[i].md, 0,
			algos[i].data, strlen(algos[i].data),
			algos[i].key, strlen(algos[i].key),
			algos[i].expect);
    }

    rpmlog(RPMLOG_INFO, "Completed hashed MAC checks.\n");
}

/*==============================================================*/

#if defined(_RPMGC_INTERNAL)

static const char * rpmgcSecSexpr(int algo, KP_t * kp)
{
    char * t = NULL;

    switch (algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	t = rpmExpand(
		"(private-key\n",
		" (RSA\n",
		"  (n #", kp->RSA.n, "#)\n",
		"  (e #", kp->RSA.e, "#)\n",
		"  (d #", kp->RSA.d, "#)\n",
		"  (p #", kp->RSA.p, "#)\n",
		"  (q #", kp->RSA.q, "#)\n",
		"  (u #", kp->RSA.u, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_DSA:
	t = rpmExpand(
		"(private-key\n",
		" (DSA\n",
		"  (p #", kp->DSA.p, "#)\n",
		"  (q #", kp->DSA.q, "#)\n",
		"  (g #", kp->DSA.g, "#)\n",
		"  (y #", kp->DSA.y, "#)\n",
		"  (x #", kp->DSA.x, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	t = rpmExpand(
		"(private-key\n",
		" (ELG\n",
		"  (p #", kp->ELG.p, "#)\n",
		"  (g #", kp->ELG.g, "#)\n",
		"  (y #", kp->ELG.y, "#)\n",
		"  (x #", kp->ELG.x, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_ECDSA:
/* XXX FIXME: curve (same memory as sentinel in AFKP_t) is never NULL. */
	if (kp->ECDSA.curve == NULL || kp->ECDSA.curve[0] == '\0') {
/* XXX FIXME */
	    t = rpmExpand(
		"(public-key\n",
		" (ECDSA\n",
		"  (p #", kp->ECDSA.p, "#)\n",
		"  (a #", kp->ECDSA.a, "#)\n",
		"  (b #", kp->ECDSA.b, "#)\n",
		"  (G #", kp->ECDSA.G, "#)\n",
		"  (n #", kp->ECDSA.n, "#)\n",
		"  (Q #", kp->ECDSA.Q, "#)))\n",
#ifdef	NOTYET
    const char * d;	/* FIXME: add to test vector for sec_key. */
#endif
		NULL);
	} else {
/* XXX FIXME */
	    t = rpmExpand(
		"(public-key\n",
		" (ECDSA\n",
		"  (curve ", kp->ECDSA.curve, ")\n",
#ifdef	NOTYET	/* XXX optional overrides? */
		"  (b #", kp->ECDSA.b, "#)\n",
		"  (G #", kp->ECDSA.G, "#)\n",
		"  (n #", kp->ECDSA.n, "#)\n",
#endif
		"  (Q #", kp->ECDSA.Q, "#)))\n",
		NULL);
	}
	break;
    }
    return t;
}

static const char * rpmgcPubSexpr(int algo, KP_t * kp)
{
    char * t = NULL;

    switch (algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	t = rpmExpand(
		"(public-key\n",
		" (RSA\n",
		"  (n #", kp->RSA.n, "#)\n",
		"  (e #", kp->RSA.e, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_DSA:
	t = rpmExpand(
		"(public-key\n",
		" (DSA\n",
		"  (p #", kp->DSA.p, "#)\n",
		"  (q #", kp->DSA.q, "#)\n",
		"  (g #", kp->DSA.g, "#)\n",
		"  (y #", kp->DSA.y, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	t = rpmExpand(
		"(public-key\n",
		" (ELG\n",
		"  (p #", kp->ELG.p, "#)\n",
		"  (g #", kp->ELG.g, "#)\n",
		"  (y #", kp->ELG.y, "#)))\n",
		NULL);
	break;
    case PGPPUBKEYALGO_ECDSA:
/* XXX FIXME: curve (same memory as sentinel in AFKP_t) is never NULL. */
	if (kp->ECDSA.curve == NULL || kp->ECDSA.curve[0] == '\0') {
	    t = rpmExpand(
		"(public-key\n",
		" (ECDSA\n",
		"  (p #", kp->ECDSA.p, "#)\n",
		"  (a #", kp->ECDSA.a, "#)\n",
		"  (b #", kp->ECDSA.b, "#)\n",
		"  (G #", kp->ECDSA.G, "#)\n",
		"  (n #", kp->ECDSA.n, "#)\n",
		"  (Q #", kp->ECDSA.Q, "#)))\n",
		NULL);
	} else {
	    t = rpmExpand(
		"(public-key\n",
		" (ECDSA\n",
		"  (curve ", kp->ECDSA.curve, ")\n",
#ifdef	NOTYET	/* XXX optional overrides? */
		"  (b #", kp->ECDSA.b, "#)\n",
		"  (G #", kp->ECDSA.G, "#)\n",
		"  (n #", kp->ECDSA.n, "#)\n",
#endif
		"  (Q #", kp->ECDSA.Q, "#)))\n",
		NULL);
	}
	break;
    }
    return t;
}
#endif	/* _RPMGC_INTERNAL */

/* Check that the signature SIG matches the hash HASH. PKEY is the
   public key used for the verification. BADHASH is a hashvalue which
   should; result in a bad signature status. */
static int
pgpCheckVerify(pgpDig dig, void * _badhash)
{
    int rc = 0;		/* assume success */
const char * msg = rpmExpand(dig->pubkey_algoN, "-", dig->hash_algoN, " verify", NULL);
int xx;

#if !defined(_RPMGC_INTERNAL)
{
    DIGEST_CTX ctx = NULL;
    uint8_t * digest = NULL;
    size_t digestlen = 0;
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);

    ctx = rpmDigestInit(sigp->hash_algo, 0);
    xx = rpmDigestUpdate(ctx, "abc", sizeof("abc")-1);
    xx = rpmDigestFinal(rpmDigestDup(ctx), &digest, &digestlen, 0);
    sigp->signhash16[0] = digest[0];
    sigp->signhash16[1] = digest[1];
    switch (pubp->pubkey_algo) {
    case PGPPUBKEYALGO_RSA:
	xx = pgpImplSetRSA(ctx, dig, sigp);
	break;
    case PGPPUBKEYALGO_DSA:
	xx = pgpImplSetDSA(ctx, dig, sigp);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	xx = pgpImplSetELG(ctx, dig, sigp);
#else
	xx = rpmDigestFinal(ctx, NULL, NULL, 0);
	xx = 1;
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
	xx = pgpImplSetECDSA(ctx, dig, sigp);
	break;
    }
    digest = _free(digest);
if (xx && !rc) rc = 1;
}
#endif	/* !defined(_RPMGC_INTERNAL) */

xx = pgpImplErrChk(dig, "verify GOOD", pgpImplVerify(dig), 0);
if (xx && !rc) rc = 1;

#if defined(_RPMGC_INTERNAL)
    {
	rpmgc gc = dig->impl;
	gcry_sexp_t badhash = _badhash;
	gcry_sexp_t hash;
	gc->badok = GPG_ERR_BAD_SIGNATURE;
	hash = gc->hash;
	gc->hash = badhash;
	xx = pgpImplErrChk(dig, "detect BAD", pgpImplVerify(dig), gc->badok);
	if (xx && !rc) rc = 1;
	gc->hash = hash;
	gc->badok = 0;
    }
#endif	/* _RPMGC_INTERNAL */

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p,%p) %s\t%s\n", __FUNCTION__, dig, _badhash, (!rc ? "OK" : "BAD"), msg);

    msg = _free(msg);

    return rc;
}

/* Test the public key sign function using the private ket SKEY. PKEY
   is used for verification. */
static int pgpCheckSignVerify(pgpDig dig, int n)
{
    int rc = 0;		/* assume success */
const char * msg = NULL;
uint32_t dalgo = PGPHASHALGO_SHA1;	/* XXX FIXME */
pgpDigParams sigp = pgpGetSignature(dig);

#if !defined(_RPMGC_INTERNAL)
{
    DIGEST_CTX ctx = NULL;
    uint8_t * digest = NULL;
    size_t digestlen = 0;
int xx;
pgpDigParams pubp = pgpGetPubkey(dig);
dalgo = (pubp->pubkey_algo == PGPPUBKEYALGO_ECDSA
		? PGPHASHALGO_SHA256 : PGPHASHALGO_SHA1);
#if defined(_RPMNSS_INTERNAL)	/* XXX Grrr only SHA1 "supported" */
dalgo = PGPHASHALGO_SHA1;
#endif
sigp->hash_algo = dalgo;
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);
msg = rpmExpand(dig->pubkey_algoN, "-", dig->hash_algoN, " sign", NULL);

    ctx = rpmDigestInit(sigp->hash_algo, 0);
    xx = rpmDigestUpdate(ctx, "abc", sizeof("abc")-1);
    xx = rpmDigestFinal(rpmDigestDup(ctx), &digest, &digestlen, 0);
    sigp->signhash16[0] = digest[0];
    sigp->signhash16[1] = digest[1];
    switch (pubp->pubkey_algo) {
    case PGPPUBKEYALGO_RSA:
	xx = pgpImplSetRSA(ctx, dig, sigp);
	break;
    case PGPPUBKEYALGO_DSA:
	xx = pgpImplSetDSA(ctx, dig, sigp);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
#ifdef	NOTYET
	xx = pgpImplSetELG(ctx, dig, sigp);
#else
	xx = rpmDigestFinal(ctx, NULL, NULL, 0);
	xx = 1;
#endif
	break;
    case PGPPUBKEYALGO_ECDSA:
	xx = pgpImplSetECDSA(ctx, dig, sigp);
	break;
    }
    digest = _free(digest);
if (xx && !rc) rc = 1;

    xx = pgpImplSign(dig);
if (!xx && !rc) rc = 1;		/* XXX 1 on success */
    xx = pgpCheckVerify(dig, NULL);
if (xx && !rc) rc = 1;	
}
#endif	/* !defined(_RPMGC_INTERNAL) */

#if defined(_RPMGC_INTERNAL)
rpmgc gc = dig->impl;
    gcry_error_t err;
    gcry_sexp_t badhash;
    int dataidx;
    static const char baddata[] =
	"(data\n (flags pkcs1)\n"
	" (hash sha1 #11223344556677889900AABBCCDDEEFF10203041#))\n";
    static struct {
	const char *data;
	int expected_rc;
    } datas[] = {
      {	"(data\n (flags pkcs1)\n"
	" (hash sha1 #11223344556677889900AABBCCDDEEFF10203040#))\n", 0 },
	/* This test is to see whether hash algorithms not hard wired in
	       pubkey.c are detected:  */
      { "(data\n (flags pkcs1)\n"
	" (hash oid.1.3.14.3.2.29 "
	"       #11223344556677889900AABBCCDDEEFF10203040#))\n", 0 },
      { "(data\n (flags )\n"
	" (hash sha1 #11223344556677889900AABBCCDDEEFF10203040#))\n",
		GPG_ERR_CONFLICT },
      { "(data\n (flags pkcs1)\n"
	" (hash foo #11223344556677889900AABBCCDDEEFF10203040#))\n",
		GPG_ERR_DIGEST_ALGO },
      { "(data\n (flags )\n" " (value #11223344556677889900AA#))\n", 0 },
      { "(data\n (flags )\n" " (value #0090223344556677889900AA#))\n", 0 },
      { "(data\n (flags raw)\n" " (value #11223344556677889900AA#))\n", 0 },
      { "(data\n (flags pkcs1)\n"
	" (value #11223344556677889900AA#))\n",
		GPG_ERR_CONFLICT },
      { "(data\n (flags raw foo)\n"
	" (value #11223344556677889900AA#))\n",
		GPG_ERR_INV_FLAG },
      { NULL }
    };
int xx;

    (void) n;

    err = rpmgcErr(gc, "badhash",
		gcry_sexp_sscan(&badhash, NULL, baddata, strlen(baddata)));
    if (err)
	die("converting data failed: %s\n", gpg_strerror(err));

dig->hash_algoN = _pgpHashAlgo2Name(dalgo);
msg = rpmExpand(dig->pubkey_algoN, "-", dig->hash_algoN, " sign", NULL);

    for (dataidx = 0; datas[dataidx].data; dataidx++) {

	rpmlog(RPMLOG_INFO, "  signature test %d\n", dataidx);

sigp->hash_algo = dalgo;	/* XXX FIXME */
dig->hash_algoN = _pgpHashAlgo2Name(dalgo);
	err = rpmgcErr(gc, "gc->hash",
		gcry_sexp_sscan(&gc->hash, NULL, datas[dataidx].data,
			     strlen(datas[dataidx].data)));
	if (err)
	    die("converting data failed: %s\n", gpg_strerror(err));

gc->badok = datas[dataidx].expected_rc;
xx = pgpImplErrChk(dig, msg, pgpImplSign(dig), datas[dataidx].expected_rc);
gc->badok = 0;
/* XXX FIXME: test pgpImplErrChk() rc to prevent error cascade or not? */
	if (!xx && !datas[dataidx].expected_rc) {
	    xx = pgpCheckVerify(dig, badhash);
if (xx && !rc) rc = 1;
	}

	gcry_sexp_release(gc->sig);
	gc->sig = NULL;
	gcry_sexp_release(gc->hash);
	gc->hash = NULL;

    }

    gcry_sexp_release(badhash);
    badhash = NULL;
#endif	/* _RPMGC_INTERNAL */

#if defined(_RPMLTC_INTERNAL)
#endif	/* _RPMLTC_INTERNAL */

#if defined(_RPMNSS_INTERNAL)
#endif	/* _RPMNSS_INTERNAL */

#if defined(_RPMSSL_INTERNAL)
#endif	/* _RPMSSL_INTERNAL */

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p,%d) %s\t%s\n", __FUNCTION__, dig, n, (!rc ? "OK" : "BAD"), msg);

    msg = _free(msg);

    return rc;
}

static int
pgpCheckGrip(pgpDig dig, int n, const unsigned char *grip)
{
    int rc = 0;		/* assume success */
    unsigned char pgrip[20] = "";
    unsigned char sgrip[20] = "";

#if defined(_RPMGC_INTERNAL)
rpmgc gc = dig->impl;
    if (!gcry_pk_get_keygrip(gc->sec_key, sgrip))
	die("get keygrip for private %s key failed\n", dig->pubkey_algoN);
    if (!gcry_pk_get_keygrip(gc->pub_key, pgrip))
	die("[%i] get keygrip for public %s key failed\n", n, dig->pubkey_algoN);
#endif	/* _RPMGC_INTERNAL */

    if (memcmp(sgrip, pgrip, sizeof(sgrip))) {
	fail("[%i] keygrips for %s key don't match\n", n, dig->pubkey_algoN);
	rc = 1;
    }
    if (memcmp(sgrip, grip, sizeof(sgrip))) {
	fail("wrong keygrip for %s key\n", dig->pubkey_algoN);
	rc = 1;
    }

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p,%d,%p) %s\t%s\n", __FUNCTION__, dig, n, grip, (!rc ? "OK" : "BAD"), dig->pubkey_algoN);

    return rc;
}

static int
pgpCheckSignVerifyGrip(pgpDig dig, int n,
		    const unsigned char *grip, int flags)
{
    int rc = 0;		/* assume success */
int xx;

    if (flags & FLAG_SIGN) {
	xx = pgpCheckSignVerify(dig, n);
if (xx && !rc) rc = 1;
    }

/* XXX FLAG_CRYPT */

    if (grip && (flags & FLAG_GRIP)) {
	xx = pgpCheckGrip(dig, n, grip);
if (xx && !rc) rc = 1;
    }

if (_pgp_debug < 0)
fprintf(stderr, "<== %s(%p,%d,%p,0x%x) %s\t%s\n", __FUNCTION__, dig, n, grip, flags, (!rc ? "OK" : "BAD"), dig->pubkey_algoN);

    return rc;
}

static int
pgpCheckLoadSignVerify(pgpDig dig, int n, AFKP_t * afkp)
{
    int rc = 0;		/* assume success */
#if defined(_RPMGC_INTERNAL)
    rpmgc gc = dig->impl;

    /* Load the private key. */
    {	const char * sec = rpmgcSecSexpr(afkp->algo, &afkp->KP);
	gc->err = rpmgcErr(gc, "gc->sec_key",
			gcry_sexp_sscan(&gc->sec_key, NULL, sec, strlen(sec)));
	sec = _free(sec);
    }

    /* Load the public key. */
    if (!gc->err) {
	const char * pub = rpmgcPubSexpr(afkp->algo, &afkp->KP);
	gc->err = rpmgcErr(gc, "gc->pub_key",
			gcry_sexp_sscan(&gc->pub_key, NULL, pub, strlen(pub)));
	pub = _free(pub);
    }

    /* Run tests on the key pair.. */
    if (gc->err)
	die("converting sample key failed: %s\n", gpg_strerror(gc->err));
    else {
int xx;
	xx = pgpCheckSignVerifyGrip(dig, n, afkp->grip, afkp->flags);
if (xx && !rc) rc = 1;
    }
#endif	/* _RPMGC_INTERNAL */

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p,%d,%p) %s\t%s\n", __FUNCTION__, dig, n, afkp, (!rc ? "OK" : "BAD"), dig->pubkey_algoN);

pgpDigClean(dig);

    return rc;
}

static int pgpCheckGenerate(pgpDig dig)
{
    int rc = 0;		/* assume success */
int xx;

    rpmlog(RPMLOG_INFO, "  generating %s key:", dig->pubkey_algoN);

    xx = pgpImplGenerate(dig);	/* XXX 1 on success */
if (!xx && !rc) rc = 1;

    if (!xx) {
#if defined(_RPMGC_INTERNAL)
rpmgc gc = dig->impl;
	if (gc->err)
	    die("error generating %s key: %s\n", dig->pubkey_algoN, gpg_strerror(gc->err));
	else if (gc->pub_key == NULL)
	    die("public part missing in %s key\n", dig->pubkey_algoN);
	else if (gc->sec_key == NULL)
	    die("private part missing in %s key\n", dig->pubkey_algoN);
#endif	/* _RPMGC_INTERNAL */
    }

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p) %s\n", __FUNCTION__, dig, (!rc ? "OK" : "BAD"));

    return rc;
}

static int pgpCheckGenerateSignVerifyGrip(pgpDig dig, int n)
{
    int rc = 0;		/* assume success */
int xx;

    xx = pgpCheckGenerate(dig);
if (xx && !rc) rc = 1;
    xx = pgpCheckSignVerifyGrip(dig, n, NULL, FLAG_SIGN | FLAG_CRYPT);
if (xx && !rc) rc = 1;
pgpDigClean(dig);

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p) %s\n", __FUNCTION__, dig, (!rc ? "OK" : "BAD"));

    return rc;
}

static AFKP_t AFKP[] = {
 {.algo = PGPPUBKEYALGO_RSA,
  .flags = FLAG_CRYPT | FLAG_SIGN | FLAG_GRIP,
  .nbits = 1024,
  .qbits = 0,
  .grip =	"\x32\x10\x0c\x27\x17\x3e\xf6\xe9\xc4\xe9"
		"\xa2\x5d\x3d\x69\xf8\x6d\x37\xa4\xf9\x39",
  .KP = {
  .RSA = {
   .n =	"00e0ce96f90b6c9e02f3922beada93fe50a875eac6bcc18bb9a9cf2e84965caa"
	"2d1ff95a7f542465c6c0c19d276e4526ce048868a7a914fd343cc3a87dd74291"
	"ffc565506d5bbb25cbac6a0e2dd1f8bcaab0d4a29c2f37c950f363484bf269f7"
	"891440464baf79827e03a36e70b814938eebdc63e964247be75dc58b014b7ea251",
   .e =	"010001",
   .d =	"046129F2489D71579BE0A75FE029BD6CDB574EBF57EA8A5B0FDA942CAB943B11"
	"7D7BB95E5D28875E0F9FC5FCC06A72F6D502464DABDED78EF6B716177B83D5BD"
	"C543DC5D3FED932E59F5897E92E6F58A0F33424106A3B6FA2CBF877510E4AC21"
	"C3EE47851E97D12996222AC3566D4CCB0B83D164074ABF7DE655FC2446DA1781",
   .p =	"00e861b700e17e8afe6837e7512e35b6ca11d0ae47d8b85161c67baf64377213"
	"fe52d772f2035b3ca830af41d8a4120e1c1c70d12cc22f00d28d31dd48a8d424f1",
   .q =	"00f7a7ca5367c661f8e62df34f0d05c10c88e5492348dd7bddc942c9a8f369f9"
	"35a07785d2db805215ed786e4285df1658eed3ce84f469b81b50d358407b4ad361",
   .u =	"304559a9ead56d2309d203811a641bb1a09626bc8eb36fffa23c968ec5bd891e"
	"ebbafc73ae666e01ba7c8990bae06cc2bbe10b75e69fcacb353a6473079d8e9b"
   }	/* .RSA */
  }	/* .KP */
 },
 {.algo = PGPPUBKEYALGO_DSA,
  .flags = FLAG_SIGN | FLAG_GRIP,
  .grip =	"\xc6\x39\x83\x1a\x43\xe5\x05\x5d\xc6\xd8"
		"\x4a\xa6\xf9\xeb\x23\xbf\xa9\x12\x2d\x5b",
  .nbits = 1024,
  .qbits = 0,
  .KP = {
  .DSA = {
   .p =	"00AD7C0025BA1A15F775F3F2D673718391D00456978D347B33D7B49E7F32EDAB"
	"96273899DD8B2BB46CD6ECA263FAF04A28903503D59062A8865D2AE8ADFB5191"
	"CF36FFB562D0E2F5809801A1F675DAE59698A9E01EFE8D7DCFCA084F4C6F5A44"
	"44D499A06FFAEA5E8EF5E01F2FD20A7B7EF3F6968AFBA1FB8D91F1559D52D8777B",
   .q =	"00EB7B5751D25EBBB7BD59D920315FD840E19AEBF9",
   .g =	"1574363387FDFD1DDF38F4FBE135BB20C7EE4772FB94C337AF86EA8E49666503"
	"AE04B6BE81A2F8DD095311E0217ACA698A11E6C5D33CCDAE71498ED35D13991E"
	"B02F09AB40BD8F4C5ED8C75DA779D0AE104BC34C960B002377068AB4B5A1F984"
	"3FBA91F537F1B7CAC4D8DD6D89B0D863AF7025D549F9C765D2FC07EE208F8D15",
   .y =	"64B11EF8871BE4AB572AA810D5D3CA11A6CDBC637A8014602C72960DB135BF46"
	"A1816A724C34F87330FC9E187C5D66897A04535CC2AC9164A7150ABFA8179827"
	"6E45831AB811EEE848EBB24D9F5F2883B6E5DDC4C659DEF944DCFD80BF4D0A20"
	"42CAA7DC289F0C5A9D155F02D3D551DB741A81695B74D4C8F477F9C7838EB0FB",
   .x =	"11D54E4ADBD3034160F2CED4B7CD292A4EBF3EC0"
   }	/* .DSA */
  }	/* .KP */
 },
 {.algo = PGPPUBKEYALGO_ELGAMAL,
  .flags = FLAG_SIGN | FLAG_CRYPT | FLAG_GRIP,
  .grip =	"\xa7\x99\x61\xeb\x88\x83\xd2\xf4\x05\xc8"
		"\x4f\xba\x06\xf8\x78\x09\xbc\x1e\x20\xe5",
  .nbits = 1024,
  .qbits = 0,
  .KP = {
  .ELG = {
   .p =	"00B93B93386375F06C2D38560F3B9C6D6D7B7506B20C1773F73F8DE56E6CD65D"
	"F48DFAAA1E93F57A2789B168362A0F787320499F0B2461D3A4268757A7B27517"
	"B7D203654A0CD484DEC6AF60C85FEB84AAC382EAF2047061FE5DAB81A20A0797"
	"6E87359889BAE3B3600ED718BE61D4FC993CC8098A703DD0DC942E965E8F18D2A7",
   .g =	"05",
   .y =	"72DAB3E83C9F7DD9A931FDECDC6522C0D36A6F0A0FEC955C5AC3C09175BBFF2B"
	"E588DB593DC2E420201BEB3AC17536918417C497AC0F8657855380C1FCF11C5B"
	"D20DB4BEE9BDF916648DE6D6E419FA446C513AAB81C30CB7B34D6007637BE675"
	"56CE6473E9F9EE9B9FADD275D001563336F2186F424DEC6199A0F758F6A00FF4",
   .x =	"03C28900087B38DABF4A0AB98ACEA39BB674D6557096C01D72E31C16BDD32214"
   }	/* .ELG */
  }	/* .KP */
 },
 {.algo = PGPPUBKEYALGO_ECDSA,
  .flags = FLAG_SIGN | FLAG_GRIP,
  .grip =	"\xE6\xDF\x94\x2D\xBD\x8C\x77\x05\xA3\xDD"
		"\x41\x6E\xFC\x04\x01\xDB\x31\x0E\x99\xB6",
  .nbits = 1024,	/* XXX W2DO? */
  .qbits = 0,
  .KP = {
  .ECDSA = {
/* XXX FIXME: curve (same memory as sentinel in AFKP_t) is never NULL. */
.curve = "",
   .p =	"00FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF",
   .a =	"00FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC",
   .b =	"5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B",
   .G =	"046B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C2"
	"964FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5",
   .n =	"00FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551",
   .Q =	"04C8A4CEC2E9A9BC8E173531A67B0840DF345C32E261ADD780E6D83D56EFADFD"
	"5DE872F8B854819B59543CE0B7F822330464FBC4E6324DADDCD9D059554F63B344"
   }	/* .ECDSA */
  }	/* .KP */
 },
 { .algo = 0, .flags = 0, .KP = { .sentinel = NULL } }
};

/* Run all tests for the public key functions. */
static int pgpTestPubkeys(pgpDig dig)
{
    int rc = 0;		/* assume success */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
    AFKP_t * afkp;
    size_t i;
int xx;

    rpmlog(RPMLOG_INFO, "Starting public key checks.\n");

    for (i = 0, afkp = AFKP; afkp->KP.sentinel; i++, afkp++) {

	if (afkp->algo <= 0)
	    continue;

if (1 || _pgp_debug < 0)
fprintf(stderr, "==> %s #1\n", _pgpPubkeyAlgo2Name(afkp->algo));
pubp->pubkey_algo = sigp->pubkey_algo = afkp->algo;
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(afkp->algo);

#if defined(_RPMBC_INTERNAL)
{   rpmbc bc = dig->impl;
    bc->nbits = afkp->nbits;
    bc->qbits = afkp->qbits;
}
#endif
#if defined(_RPMGC_INTERNAL)
{   rpmgc gc = dig->impl;
    gc->nbits = afkp->nbits;
    gc->qbits = afkp->qbits;
}
#endif

#ifndef	DYING
/* XXX FIXME: no sec_key parameters. */
	if (afkp->algo == PGPPUBKEYALGO_ECDSA)
	    continue;
#endif

	/* Lookup & FIPS check. */
	if (pgpImplAvailablePubkey(dig, afkp->algo))
	    continue;

	xx = pgpCheckLoadSignVerify(dig, i, afkp);
if (xx && !rc) rc = 1;
/* XXX FIXME: pgpDigClean */

    }

    rpmlog(RPMLOG_INFO, "Completed public key checks.\n");

    rpmlog(RPMLOG_INFO, "Starting additional public key checks.\n");

    for (i = 0, afkp = AFKP; afkp->KP.sentinel; i++, afkp++) {

	if (afkp->algo <= 0)
	    continue;

if (1 || _pgp_debug < 0)
fprintf(stderr, "==> %s #2\n", _pgpPubkeyAlgo2Name(afkp->algo));
pubp->pubkey_algo = sigp->pubkey_algo = afkp->algo;
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(afkp->algo);

#if defined(_RPMBC_INTERNAL)
{   rpmbc bc = dig->impl;
    bc->nbits = afkp->nbits;
    bc->qbits = afkp->qbits;
}
#endif
#if defined(_RPMGC_INTERNAL)
{   rpmgc gc = dig->impl;
    gc->nbits = afkp->nbits;
    gc->qbits = afkp->qbits;
}
#endif
#if defined(_RPMLTC_INTERNAL)
{   rpmltc ltc = dig->impl;
    ltc->nbits = afkp->nbits;
    ltc->qbits = afkp->qbits;
}
#endif
#if defined(_RPMNSS_INTERNAL)
{   rpmnss nss = dig->impl;
    nss->nbits = afkp->nbits;
    nss->qbits = afkp->qbits;
}
#endif
#if defined(_RPMSSL_INTERNAL)
{   rpmssl ssl = dig->impl;
    ssl->nbits = afkp->nbits;
    ssl->qbits = afkp->qbits;
}
#endif

	/* Lookup & FIPS check. */
	if (pgpImplAvailablePubkey(dig, afkp->algo))
	    continue;

	xx = pgpCheckGenerateSignVerifyGrip(dig, i);
if (xx && !rc) rc = 1;
/* XXX FIXME: pgpDigClean */

    }

    rpmlog(RPMLOG_INFO, "Completed additional public key checks.\n");

if (1 || _pgp_debug < 0)
fprintf(stderr, "<== %s(%p) %s\n", __FUNCTION__, dig, (!rc ? "OK" : "BAD"));

    return rc;

}

static int pgpBasicTests(pgpDig dig)
{
#if defined(_RPMGC_INTERNAL)
rpmgc gc = dig->impl;
#endif	/* _RPMGC_INTERNAL */
int xx;

fprintf(stderr, "%s: use_fips %d selftest_only %d\n",
__FUNCTION__, use_fips, selftest_only);

    if (selftest_only) {
	rpmIncreaseVerbosity();
	rpmIncreaseVerbosity();
    }

#if defined(_RPMGC_INTERNAL)
    if (rpmIsVerbose())
	gcry_control(GCRYCTL_SET_VERBOSITY, 3);

    if (use_fips)
	gcry_control(GCRYCTL_FORCE_FIPS_MODE, 0);

    if (!gcry_check_version(GCRYPT_VERSION))
	die("version mismatch\n");

    if (gcry_fips_mode_active())
	gc->in_fips_mode = 1;

    if (!gc->in_fips_mode)
	gcry_control(GCRYCTL_DISABLE_SECMEM, 0);

    if (rpmIsVerbose())
	gcry_set_progress_handler(rpmgcProgress, NULL);

    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    if (rpmIsDebug() || _pgp_debug)
	gcry_control(GCRYCTL_SET_DEBUG_FLAGS, 1u, 0);

    /* No valuable keys are created, so we can speed up our RNG. */
    gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#endif	/* _RPMGC_INTERNAL */

    if (!selftest_only) {
#if defined(_RPMGC_INTERNAL)
	check_ciphers(dig);
	check_cipher_modes(dig);
#endif	/* _RPMGC_INTERNAL */
	pgpTestDigests(dig);
	pgpTestHMACS(dig);
	xx = pgpTestPubkeys(dig);
    }

    /* If we are in fips mode do some more tests. */
#if defined(_RPMGC_INTERNAL)
    if (gc->in_fips_mode && !selftest_only) {
	gpg_error_t err;

	gcry_md_hd_t md;

	/* First trigger a self-test.  */
	gcry_control(GCRYCTL_FORCE_FIPS_MODE, 0);
	if (!gcry_control(GCRYCTL_OPERATIONAL_P, 0))
	    fail("not in operational state after self-test\n");

	/* Get us into the error state.  */
	err = gcry_md_open(&md, PGPHASHALGO_SHA1, 0);
	if (err)
	    fail("failed to open SHA-1 hash context: %s\n",
		 gpg_strerror(err));
	else {
	    err = gcry_md_enable(md, PGPHASHALGO_SHA256);
	    if (err)
		fail("failed to add SHA-256 hash context: %s\n",
		     gpg_strerror(err));
	    else {
		/* gcry_md_get_algo is only defined for a context with
		   just one digest algorithm.  With our setup it should
		   put the library into an error state.  */
		fputs("Note: Two lines with error messages follow "
		      "- this is expected\n", stderr);
		gcry_md_get_algo(md);
		gcry_md_close(md);
		if (gcry_control(GCRYCTL_OPERATIONAL_P, 0))
		    fail("expected error state but still in operational state\n");
		else {
		    /* Now run a self-test and to get back into
		       operational state.  */
		    gcry_control(GCRYCTL_FORCE_FIPS_MODE, 0);
		    if (!gcry_control(GCRYCTL_OPERATIONAL_P, 0))
			fail("did not reach operational after error "
			     "and self-test\n");
		}
	    }
	}

    } else {
	/* If in standard mode, run selftests.  */
	if (gcry_control(GCRYCTL_SELFTEST, 0))
	    fail("running self-test failed\n");
    }
#endif	/* _RPMGC_INTERNAL */

    rpmlog(RPMLOG_INFO, "\nAll tests completed. Errors: %i\n", _pgp_error_count);

#if defined(_RPMGC_INTERNAL)
    if (gc->in_fips_mode && !gcry_fips_mode_active())
	fprintf(stderr, "FIPS mode is not anymore active\n");
#endif	/* _RPMGC_INTERNAL */

    return _pgp_error_count ? 1 : 0;		/* XXX 0 on success */
}

/*==============================================================*/

#if defined(_RPMNSS_INTERNAL)
extern SECStatus
EC_DecodeParams(const SECItem *encodedParams, ECParams **ecparams);

/* XXX sorted table */
static KEY rpmnssOIDS[] = {
  { "c2onb191v4", SEC_OID_ANSIX962_EC_C2ONB191V4 },
  { "c2onb191v5", SEC_OID_ANSIX962_EC_C2ONB191V5 },
  { "c2onb239v4", SEC_OID_ANSIX962_EC_C2ONB239V4 },
  { "c2onb239v5", SEC_OID_ANSIX962_EC_C2ONB239V5 },
  { "c2pnb163v1", SEC_OID_ANSIX962_EC_C2PNB163V1 },
  { "c2pnb163v2", SEC_OID_ANSIX962_EC_C2PNB163V2 },
  { "c2pnb163v3", SEC_OID_ANSIX962_EC_C2PNB163V3 },
  { "c2pnb176v1", SEC_OID_ANSIX962_EC_C2PNB176V1 },
  { "c2pnb208w1", SEC_OID_ANSIX962_EC_C2PNB208W1 },
  { "c2pnb272w1", SEC_OID_ANSIX962_EC_C2PNB272W1 },
  { "c2pnb304w1", SEC_OID_ANSIX962_EC_C2PNB304W1 },
  { "c2pnb368w1", SEC_OID_ANSIX962_EC_C2PNB368W1 },
  { "c2tnb191v1", SEC_OID_ANSIX962_EC_C2TNB191V1 },
  { "c2tnb191v2", SEC_OID_ANSIX962_EC_C2TNB191V2 },
  { "c2tnb191v3", SEC_OID_ANSIX962_EC_C2TNB191V3 },
  { "c2tnb239v1", SEC_OID_ANSIX962_EC_C2TNB239V1 },
  { "c2tnb239v2", SEC_OID_ANSIX962_EC_C2TNB239V2 },
  { "c2tnb239v3", SEC_OID_ANSIX962_EC_C2TNB239V3 },
  { "c2tnb359v1", SEC_OID_ANSIX962_EC_C2TNB359V1 },
  { "c2tnb431r1", SEC_OID_ANSIX962_EC_C2TNB431R1 },
  { "nistb163", SEC_OID_SECG_EC_SECT163R2},
  { "nistb233", SEC_OID_SECG_EC_SECT233R1},
  { "nistb283", SEC_OID_SECG_EC_SECT283R1},
  { "nistb409", SEC_OID_SECG_EC_SECT409R1},
  { "nistb571", SEC_OID_SECG_EC_SECT571R1},
  { "nistk163", SEC_OID_SECG_EC_SECT163K1},
  { "nistk233", SEC_OID_SECG_EC_SECT233K1},
  { "nistk283", SEC_OID_SECG_EC_SECT283K1},
  { "nistk409", SEC_OID_SECG_EC_SECT409K1},
  { "nistk571", SEC_OID_SECG_EC_SECT571K1},
  { "nistp192", SEC_OID_SECG_EC_SECP192R1},
  { "nistp224", SEC_OID_SECG_EC_SECP224R1},
  { "nistp256", SEC_OID_SECG_EC_SECP256R1},
  { "nistp384", SEC_OID_SECG_EC_SECP384R1},
  { "nistp521", SEC_OID_SECG_EC_SECP521R1},
  { "prime192v1", SEC_OID_ANSIX962_EC_PRIME192V1 },
  { "prime192v2", SEC_OID_ANSIX962_EC_PRIME192V2 },
  { "prime192v3", SEC_OID_ANSIX962_EC_PRIME192V3 },
  { "prime239v1", SEC_OID_ANSIX962_EC_PRIME239V1 },
  { "prime239v2", SEC_OID_ANSIX962_EC_PRIME239V2 },
  { "prime239v3", SEC_OID_ANSIX962_EC_PRIME239V3 },
  { "secp112r1", SEC_OID_SECG_EC_SECP112R1},
  { "secp112r2", SEC_OID_SECG_EC_SECP112R2},
  { "secp128r1", SEC_OID_SECG_EC_SECP128R1},
  { "secp128r2", SEC_OID_SECG_EC_SECP128R2},
  { "secp160k1", SEC_OID_SECG_EC_SECP160K1},
  { "secp160r1", SEC_OID_SECG_EC_SECP160R1},
  { "secp160r2", SEC_OID_SECG_EC_SECP160R2},
  { "secp192k1", SEC_OID_SECG_EC_SECP192K1},
  { "secp192r1", SEC_OID_SECG_EC_SECP192R1},
  { "secp224k1", SEC_OID_SECG_EC_SECP224K1},
  { "secp224r1", SEC_OID_SECG_EC_SECP224R1},
  { "secp256k1", SEC_OID_SECG_EC_SECP256K1},
  { "secp256r1", SEC_OID_SECG_EC_SECP256R1},
  { "secp384r1", SEC_OID_SECG_EC_SECP384R1},
  { "secp521r1", SEC_OID_SECG_EC_SECP521R1},
  { "sect113r1", SEC_OID_SECG_EC_SECT113R1},
  { "sect113r2", SEC_OID_SECG_EC_SECT113R2},
  { "sect131r1", SEC_OID_SECG_EC_SECT131R1},
  { "sect131r2", SEC_OID_SECG_EC_SECT131R2},
  { "sect163k1", SEC_OID_SECG_EC_SECT163K1},
  { "sect163r1", SEC_OID_SECG_EC_SECT163R1},
  { "sect163r2", SEC_OID_SECG_EC_SECT163R2},
  { "sect193r1", SEC_OID_SECG_EC_SECT193R1},
  { "sect193r2", SEC_OID_SECG_EC_SECT193R2},
  { "sect233k1", SEC_OID_SECG_EC_SECT233K1},
  { "sect233r1", SEC_OID_SECG_EC_SECT233R1},
  { "sect239k1", SEC_OID_SECG_EC_SECT239K1},
  { "sect283k1", SEC_OID_SECG_EC_SECT283K1},
  { "sect283r1", SEC_OID_SECG_EC_SECT283R1},
  { "sect409k1", SEC_OID_SECG_EC_SECT409K1},
  { "sect409r1", SEC_OID_SECG_EC_SECT409R1},
  { "sect571k1", SEC_OID_SECG_EC_SECT571K1},
  { "sect571r1", SEC_OID_SECG_EC_SECT571R1},
};
static size_t nrpmnssOIDS = sizeof(rpmnssOIDS) / sizeof(rpmnssOIDS[0]);

#define _ENTRY(_v)      { SEC_ERROR_##_v, #_v }
/* XXX sorted table */
static keyVN_t rpmnssERRS[] = {
    _ENTRY(IO),
    _ENTRY(LIBRARY_FAILURE),
    _ENTRY(BAD_DATA),
    _ENTRY(OUTPUT_LEN),
    _ENTRY(INPUT_LEN),
    _ENTRY(INVALID_ARGS),
    _ENTRY(INVALID_ALGORITHM),
    _ENTRY(INVALID_AVA),
    _ENTRY(INVALID_TIME),
    _ENTRY(BAD_DER),
    _ENTRY(BAD_SIGNATURE),
    _ENTRY(EXPIRED_CERTIFICATE),
    _ENTRY(REVOKED_CERTIFICATE),
    _ENTRY(UNKNOWN_ISSUER),
    _ENTRY(BAD_KEY),
    _ENTRY(BAD_PASSWORD),
    _ENTRY(RETRY_PASSWORD),
    _ENTRY(NO_NODELOCK),
    _ENTRY(BAD_DATABASE),
    _ENTRY(NO_MEMORY),
    _ENTRY(UNTRUSTED_ISSUER),
    _ENTRY(UNTRUSTED_CERT),
    _ENTRY(DUPLICATE_CERT),
    _ENTRY(DUPLICATE_CERT_NAME),
    _ENTRY(ADDING_CERT),
    _ENTRY(FILING_KEY),
    _ENTRY(NO_KEY),
    _ENTRY(CERT_VALID),
    _ENTRY(CERT_NOT_VALID),
    _ENTRY(CERT_NO_RESPONSE),
    _ENTRY(EXPIRED_ISSUER_CERTIFICATE),
    _ENTRY(CRL_EXPIRED),
    _ENTRY(CRL_BAD_SIGNATURE),
    _ENTRY(CRL_INVALID),
    _ENTRY(EXTENSION_VALUE_INVALID),
    _ENTRY(EXTENSION_NOT_FOUND),
    _ENTRY(CA_CERT_INVALID),
    _ENTRY(PATH_LEN_CONSTRAINT_INVALID),
    _ENTRY(CERT_USAGES_INVALID),
/* SEC_INTERNAL_ONLY */
    _ENTRY(INVALID_KEY),
    _ENTRY(UNKNOWN_CRITICAL_EXTENSION),
    _ENTRY(OLD_CRL),
    _ENTRY(NO_EMAIL_CERT),
    _ENTRY(NO_RECIPIENT_CERTS_QUERY),
    _ENTRY(NOT_A_RECIPIENT),
    _ENTRY(PKCS7_KEYALG_MISMATCH),
    _ENTRY(PKCS7_BAD_SIGNATURE),
    _ENTRY(UNSUPPORTED_KEYALG),
    _ENTRY(DECRYPTION_DISALLOWED),
/* Fortezza Alerts */
/* XP_SEC_FORTEZZA_BAD_CARD */
/* XP_SEC_FORTEZZA_NO_CARD */
/* XP_SEC_FORTEZZA_NONE_SELECTED */
/* XP_SEC_FORTEZZA_MORE_INFO */
/* XP_SEC_FORTEZZA_PERSON_NOT_FOUND */
/* XP_SEC_FORTEZZA_NO_MORE_INFO */
/* XP_SEC_FORTEZZA_BAD_PIN */
/* XP_SEC_FORTEZZA_PERSON_ERROR */
    _ENTRY(NO_KRL),
    _ENTRY(KRL_EXPIRED),
    _ENTRY(KRL_BAD_SIGNATURE),
    _ENTRY(REVOKED_KEY),
    _ENTRY(KRL_INVALID),
    _ENTRY(NEED_RANDOM),
    _ENTRY(NO_MODULE),
    _ENTRY(NO_TOKEN),
    _ENTRY(READ_ONLY),
    _ENTRY(NO_SLOT_SELECTED),
    _ENTRY(CERT_NICKNAME_COLLISION),
    _ENTRY(KEY_NICKNAME_COLLISION),
    _ENTRY(SAFE_NOT_CREATED),
    _ENTRY(BAGGAGE_NOT_CREATED),
/* XP_JAVA_REMOVE_PRINCIPAL_ERROR */
/* XP_JAVA_DELETE_PRIVILEGE_ERROR */
/* XP_JAVA_CERT_NOT_EXISTS_ERROR */
    _ENTRY(BAD_EXPORT_ALGORITHM),
    _ENTRY(EXPORTING_CERTIFICATES),
    _ENTRY(IMPORTING_CERTIFICATES),
    _ENTRY(PKCS12_DECODING_PFX),
    _ENTRY(PKCS12_INVALID_MAC),
    _ENTRY(PKCS12_UNSUPPORTED_MAC_ALGORITHM),
    _ENTRY(PKCS12_UNSUPPORTED_TRANSPORT_MODE),
    _ENTRY(PKCS12_CORRUPT_PFX_STRUCTURE),
    _ENTRY(PKCS12_UNSUPPORTED_PBE_ALGORITHM),
    _ENTRY(PKCS12_UNSUPPORTED_VERSION),
    _ENTRY(PKCS12_PRIVACY_PASSWORD_INCORRECT),
    _ENTRY(PKCS12_CERT_COLLISION),
    _ENTRY(USER_CANCELLED),
    _ENTRY(PKCS12_DUPLICATE_DATA),
    _ENTRY(MESSAGE_SEND_ABORTED),
    _ENTRY(INADEQUATE_KEY_USAGE),
    _ENTRY(INADEQUATE_CERT_TYPE),
    _ENTRY(CERT_ADDR_MISMATCH),
    _ENTRY(PKCS12_UNABLE_TO_IMPORT_KEY),
    _ENTRY(PKCS12_IMPORTING_CERT_CHAIN),
    _ENTRY(PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME),
    _ENTRY(PKCS12_UNABLE_TO_EXPORT_KEY),
    _ENTRY(PKCS12_UNABLE_TO_WRITE),
    _ENTRY(PKCS12_UNABLE_TO_READ),
    _ENTRY(PKCS12_KEY_DATABASE_NOT_INITIALIZED),
    _ENTRY(KEYGEN_FAIL),
    _ENTRY(INVALID_PASSWORD),
    _ENTRY(RETRY_OLD_PASSWORD),
    _ENTRY(BAD_NICKNAME),
    _ENTRY(NOT_FORTEZZA_ISSUER),
    _ENTRY(CANNOT_MOVE_SENSITIVE_KEY),
    _ENTRY(JS_INVALID_MODULE_NAME),
    _ENTRY(JS_INVALID_DLL),
    _ENTRY(JS_ADD_MOD_FAILURE),
    _ENTRY(JS_DEL_MOD_FAILURE),
    _ENTRY(OLD_KRL),
    _ENTRY(CKL_CONFLICT),
    _ENTRY(CERT_NOT_IN_NAME_SPACE),
    _ENTRY(KRL_NOT_YET_VALID),
    _ENTRY(CRL_NOT_YET_VALID),
    _ENTRY(UNKNOWN_CERT),
    _ENTRY(UNKNOWN_SIGNER),
    _ENTRY(CERT_BAD_ACCESS_LOCATION),
    _ENTRY(OCSP_UNKNOWN_RESPONSE_TYPE),
    _ENTRY(OCSP_BAD_HTTP_RESPONSE),
    _ENTRY(OCSP_MALFORMED_REQUEST),
    _ENTRY(OCSP_SERVER_ERROR),
    _ENTRY(OCSP_TRY_SERVER_LATER),
    _ENTRY(OCSP_REQUEST_NEEDS_SIG),
    _ENTRY(OCSP_UNAUTHORIZED_REQUEST),
    _ENTRY(OCSP_UNKNOWN_RESPONSE_STATUS),
    _ENTRY(OCSP_UNKNOWN_CERT),
    _ENTRY(OCSP_NOT_ENABLED),
    _ENTRY(OCSP_NO_DEFAULT_RESPONDER),
    _ENTRY(OCSP_MALFORMED_RESPONSE),
    _ENTRY(OCSP_UNAUTHORIZED_RESPONSE),
    _ENTRY(OCSP_FUTURE_RESPONSE),
    _ENTRY(OCSP_OLD_RESPONSE),
/* smime stuff */
    _ENTRY(DIGEST_NOT_FOUND),
    _ENTRY(UNSUPPORTED_MESSAGE_TYPE),
    _ENTRY(MODULE_STUCK),
    _ENTRY(BAD_TEMPLATE),
    _ENTRY(CRL_NOT_FOUND),
    _ENTRY(REUSED_ISSUER_AND_SERIAL),
    _ENTRY(BUSY),
    _ENTRY(EXTRA_INPUT),
/* error codes used by elliptic curve code */
    _ENTRY(UNSUPPORTED_ELLIPTIC_CURVE),
    _ENTRY(UNSUPPORTED_EC_POINT_FORM),
    _ENTRY(UNRECOGNIZED_OID),
    _ENTRY(OCSP_INVALID_SIGNING_CERT),
/* new revocation errors */
    _ENTRY(REVOKED_CERTIFICATE_CRL),
    _ENTRY(REVOKED_CERTIFICATE_OCSP),
    _ENTRY(CRL_INVALID_VERSION),
    _ENTRY(CRL_V1_CRITICAL_EXTENSION),
    _ENTRY(CRL_UNKNOWN_CRITICAL_EXTENSION),
    _ENTRY(UNKNOWN_OBJECT_TYPE),
    _ENTRY(INCOMPATIBLE_PKCS11),
    _ENTRY(NO_EVENT),
    _ENTRY(CRL_ALREADY_EXISTS),
    _ENTRY(NOT_INITIALIZED),
    _ENTRY(TOKEN_NOT_LOGGED_IN),
    _ENTRY(OCSP_RESPONDER_CERT_INVALID),
    _ENTRY(OCSP_BAD_SIGNATURE),
    _ENTRY(OUT_OF_SEARCH_LIMITS),
    _ENTRY(INVALID_POLICY_MAPPING),
    _ENTRY(POLICY_VALIDATION_FAILED),
    _ENTRY(UNKNOWN_AIA_LOCATION_TYPE),
    _ENTRY(BAD_HTTP_RESPONSE),
    _ENTRY(BAD_LDAP_RESPONSE),
    _ENTRY(FAILED_TO_ENCODE_DATA),
    _ENTRY(BAD_INFO_ACCESS_LOCATION),
    _ENTRY(LIBPKIX_INTERNAL),
    _ENTRY(PKCS11_GENERAL_ERROR),
    _ENTRY(PKCS11_FUNCTION_FAILED),
    _ENTRY(PKCS11_DEVICE_ERROR),
    _ENTRY(BAD_INFO_ACCESS_METHOD),
    _ENTRY(CRL_IMPORT_FAILED),
};
static size_t nrpmnssERRS = sizeof(rpmnssERRS) / sizeof(rpmnssERRS[0]);
#undef _ENTRY

static void rpmnssDumpSECITEM(const char * msg, SECItem * p)
{
    fprintf(stderr, "%s: %p", msg, p);
    if (p)
	fprintf(stderr, " type %d %p[%u]\n\t%s",
		p->type, p->data, (unsigned)p->len, pgpHexStr(p->data, p->len));
    fprintf(stderr, "\n");
}

static void rpmnssDumpPUBKEY(const char * msg, SECKEYPublicKey * p)
{
    fprintf(stderr, "%s: %p", msg, p);
    if (p)
	fprintf(stderr, " type %d slot %p id %p",
		p->keyType, p->pkcs11Slot, (void *)p->pkcs11ID);
    fprintf(stderr, "\n");
}

static void rpmnssDumpPRVKEY(const char * msg, SECKEYPrivateKey * p)
{
    fprintf(stderr, "%3s: %p", msg, p);
    if (p)
	fprintf(stderr, " type %d slot %p id %p",
		p->keyType, p->pkcs11Slot, (void *)p->pkcs11ID);
    fprintf(stderr, "\n");
}

static void rpmnssDumpSLOTINFO(const char * msg,
		PK11SlotInfo * p, CK_MECHANISM_TYPE _type)
{
    fprintf(stderr, "%3s: %p type 0x%04X\n", msg, p, (unsigned)_type);
    if (p) {
fprintf(stderr, "\t\tIsReadOnly(%s)\n", (PK11_IsReadOnly(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tIsInternal(%s)\n", (PK11_IsInternal(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tIsInternalKeySlot(%s)\n", (PK11_IsInternalKeySlot(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tGetTokenName(%s)\n", PK11_GetTokenName(p));
fprintf(stderr, "\t\tGetSlotName(%s)\n", PK11_GetSlotName(p));
fprintf(stderr, "\t\tNeedLogin(%s)\n", (PK11_NeedLogin(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tIsFriendly(%s)\n", (PK11_IsFriendly(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tIsHW(%s)\n", (PK11_IsHW(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tIsRemovable(%s)\n", (PK11_IsRemovable(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tNeedUserInit(%s)\n", (PK11_NeedUserInit(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tProtectedAuthenticationPath(%s)\n", (PK11_ProtectedAuthenticationPath(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tGetSlotSeries(%d)\n", PK11_GetSlotSeries(p));
fprintf(stderr, "\t\tGetCurrentWrapIndex(%d)\n", PK11_GetCurrentWrapIndex(p));
fprintf(stderr, "\t\tGetDefaultFlags(0x%lX)\n", PK11_GetDefaultFlags(p));
fprintf(stderr, "\t\tGetSlotId(%p)\n", (void *)PK11_GetSlotID(p));
fprintf(stderr, "\t\tGetModuleID(%p)\n", (void *)PK11_GetModuleID(p));
fprintf(stderr, "\t\tIsDisabled(%s)\n", (PK11_IsDisabled(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tHasRootCerts(%s)\n", (PK11_HasRootCerts(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tGetDisabledReason(%p)\n", (void *)PK11_GetDisabledReason(p));
fprintf(stderr, "\t\tGetModule(%p)\n", PK11_GetModule(p));
fprintf(stderr, "\t\tIsPresent(%s)\n", (PK11_IsPresent(p) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tDoesMechanism(%s)\n", (PK11_DoesMechanism(p, _type) ? "TRUE" : "FALSE"));
fprintf(stderr, "\t\tGetBestWrapMechanism(%p)\n", (void *)PK11_GetBestWrapMechanism(p));
fprintf(stderr, "\t\tGetBestKeyLength(%d)\n", PK11_GetBestKeyLength(p, _type));

    }
}

static uint32_t curve2oid(const char * name)
{
    uint32_t oid = keyValue(rpmnssOIDS, nrpmnssOIDS, name);
    if (oid == 0)
	oid = SEC_OID_UNKNOWN;

if (_pgp_debug)
fprintf(stderr, "<-- %s(%s) oid %u\n", __FUNCTION__, name, oid);

    return oid;
}

static const char * rpmnssStrerror(int err)
{
    static char buf[64];
    const char * errN = keyVN(rpmnssERRS, nrpmnssERRS, err);
    if (errN == NULL) {
	snprintf(buf, sizeof(buf), "SEC_ERROR(%d)", err);
	errN = buf;
    }
    return errN;
}

static
int rpmnssErr(rpmnss nss, const char * msg, int rc)
        /*@*/
{
#ifdef	REFERENCE
    /* XXX Don't spew on expected failures ... */
    if (err && gcry_err_code(err) != gc->badok)
        fprintf (stderr, "rpmgc: %s(0x%0x): %s/%s\n",
                msg, (unsigned)err, gcry_strsource(err), gcry_strerror(err));
#endif
    if (rc != SECSuccess) {
	int err = PORT_GetError();
        fprintf (stderr, "rpmnss: %s rc(%d) err(%d) %s\n",
                msg, rc, err, rpmnssStrerror(err));
    }
    return rc;
}

static void rpmnssLoadParams(pgpDig dig, const char * name)
{
    rpmnss nss = dig->impl;
    SECOidTag curveOidTag = curve2oid(name);
    SECOidData * oidData = SECOID_FindOIDByTag(curveOidTag);
    
    if (curveOidTag == SEC_OID_UNKNOWN || oidData == NULL) {
	nss->sigalg = curveOidTag;
	goto exit;
    }
    nss->sigalg = curveOidTag;

    nss->ecparams = SECITEM_AllocItem(NULL, NULL, (2 + oidData->oid.len));
    nss->ecparams->data[0] = SEC_ASN1_OBJECT_ID;
    nss->ecparams->data[1] = oidData->oid.len;
    memcpy(nss->ecparams->data + 2, oidData->oid.data, oidData->oid.len);

exit:
if (_pgp_debug)
fprintf(stderr, "<-- %s(%p,%s) oid %u params %p\n", __FUNCTION__, dig, name, nss->sigalg, nss->ecparams);
    return;
}

static
int rpmnssSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmnss nss = dig->impl;
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    nss->sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	nss->sigalg = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA1:
	nss->sigalg = SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_RIPEMD160:
	break;
    case PGPHASHALGO_MD2:
	nss->sigalg = SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_MD4:
	nss->sigalg = SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_TIGER192:
	break;
    case PGPHASHALGO_HAVAL_5_160:
	break;
    case PGPHASHALGO_SHA256:
	nss->sigalg = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA384:
	nss->sigalg = SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA512:
	nss->sigalg = SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA224:
	break;
    default:
	break;
    }
    if (nss->sigalg == SEC_OID_UNKNOWN)
	return 1;

    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    return memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));
}

static
int rpmnssVerifyRSA(pgpDig dig)
	/*@*/
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure. */

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	sigalg = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_RIPEMD160:
	break;
    case PGPHASHALGO_MD2:
	sigalg = SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_MD4:
	sigalg = SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_TIGER192:
	break;
    case PGPHASHALGO_HAVAL_5_160:
	break;
    case PGPHASHALGO_SHA256:
	sigalg = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA384:
	sigalg = SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA512:
	sigalg = SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION;
	break;
    case PGPHASHALGO_SHA224:
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

    rc = rpmnssErr(nss, "VFY_VerifyDigest",
	    VFY_VerifyDigest(&nss->item, nss->pub_key, nss->sig, sigalg, NULL));
    rc = (rc == SECSuccess);

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssSignRSA(pgpDig dig)
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure. */

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	sigalg = SEC_OID_MD5;
	break;
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_SHA1;
	break;
    case PGPHASHALGO_RIPEMD160:
	break;
    case PGPHASHALGO_MD2:
	sigalg = SEC_OID_MD2;
	break;
    case PGPHASHALGO_MD4:
	sigalg = SEC_OID_MD4;
	break;
    case PGPHASHALGO_TIGER192:
	break;
    case PGPHASHALGO_HAVAL_5_160:
	break;
    case PGPHASHALGO_SHA256:
	sigalg = SEC_OID_SHA256;
	break;
    case PGPHASHALGO_SHA384:
	sigalg = SEC_OID_SHA384;
	break;
    case PGPHASHALGO_SHA512:
	sigalg = SEC_OID_SHA512;
	break;
    case PGPHASHALGO_SHA224:
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

if (nss->sig != NULL) {
    SECITEM_ZfreeItem(nss->sig, PR_TRUE);
    nss->sig = NULL;
}
nss->sig = SECITEM_AllocItem(NULL, NULL, 0);
nss->sig->type = siBuffer;

    rc = rpmnssErr(nss, "SGN_Digest",
	    SGN_Digest(nss->sec_key, sigalg, nss->sig, &nss->item));
    rc = (rc == SECSuccess);

if (_pgp_debug) {
rpmnssDumpSECITEM("hash", &nss->item);
rpmnssDumpPRVKEY( " sec", nss->sec_key);
rpmnssDumpSECITEM(" sig", nss->sig);
}

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssGenerateRSA(pgpDig dig)
{
    rpmnss nss = dig->impl;
    int rc = 0;		/* assume failure */

    {	CK_MECHANISM_TYPE _type = CKM_RSA_PKCS_KEY_PAIR_GEN;
	PK11SlotInfo * _slot = PK11_GetBestSlot(_type, NULL);
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_FALSE;
	void * _cx = NULL;

assert(nss->nbits);

	if (_slot) {
	    static unsigned _pe = 0x10001;	/* XXX FIXME: pass in e */
	    PK11RSAGenParams rsaparams =
		{ .keySizeInBits = nss->nbits, .pe = _pe };
	    void * params = &rsaparams;

#ifdef	DYING
rpmnssDumpSLOTINFO("\tGetBestSlot", _slot, _type);
#endif

	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, params,
			&nss->pub_key, _isPerm, _isSensitive, _cx);

#ifdef	DYING
fprintf(stderr, "<-- %p = PK11_GenerateKeyPair(%p, 0x%04X, %p, %p, %s,%s,%p)\n",
nss->sec_key, _slot, (unsigned)_type, params,
&nss->pub_key,
(_isPerm ? "TRUE" : "FALSE"),
(_isSensitive ? "TRUE" : "FALSE"),
_cx);
#endif

	    PK11_FreeSlot(_slot);
	}
    }

    rc = (nss->sec_key && nss->pub_key);

if (1 || _pgp_debug) {
rpmnssDumpPRVKEY(" sec", nss->sec_key);
rpmnssDumpPUBKEY(" pub", nss->pub_key);
}

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmnssSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmnss nss = dig->impl;
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    nss->sigalg = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;

    /* Compare leading 16 bits of digest for quick check. */
    return memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));
}

static
int rpmnssVerifyDSA(pgpDig dig)
	/*@*/
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure. */

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	break;
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;
	break;
    case PGPHASHALGO_RIPEMD160:
	break;
    case PGPHASHALGO_MD2:
	break;
    case PGPHASHALGO_MD4:
	break;
    case PGPHASHALGO_TIGER192:
	break;
    case PGPHASHALGO_HAVAL_5_160:
	break;
    case PGPHASHALGO_SHA256:
	break;
    case PGPHASHALGO_SHA384:
	break;
    case PGPHASHALGO_SHA512:
	break;
    case PGPHASHALGO_SHA224:
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

    rc = rpmnssErr(nss, "VFY_VerifyDigest",
	    VFY_VerifyDigest(&nss->item, nss->pub_key, nss->sig, sigalg, NULL));
    rc = (rc == SECSuccess);

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssSignDSA(pgpDig dig)
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure. */
SECItem sig = { .type = siBuffer, .len = 0, .data = NULL };

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_MD5:
	break;
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_SHA1;
	break;
    case PGPHASHALGO_RIPEMD160:
	break;
    case PGPHASHALGO_MD2:
	break;
    case PGPHASHALGO_MD4:
	break;
    case PGPHASHALGO_TIGER192:
	break;
    case PGPHASHALGO_HAVAL_5_160:
	break;
    case PGPHASHALGO_SHA256:
	break;
    case PGPHASHALGO_SHA384:
	break;
    case PGPHASHALGO_SHA512:
	break;
    case PGPHASHALGO_SHA224:
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

if (nss->sig != NULL) {
    SECITEM_ZfreeItem(nss->sig, PR_TRUE);
    nss->sig = NULL;
}

nss->sig = SECITEM_AllocItem(NULL, NULL, 0);
nss->sig->type = siBuffer;

    rc = rpmnssErr(nss, "SGN_Digest",
	    SGN_Digest(nss->sec_key, sigalg, &sig, &nss->item));

    if (rc == SECSuccess)
	rc = rpmnssErr(nss, "DSAU_EncodeDerSig",
		DSAU_EncodeDerSig(nss->sig, &sig));

    sig.data = _free(sig.data);

    rc = (rc == SECSuccess);

if (_pgp_debug) {
rpmnssDumpSECITEM("hash", &nss->item);
rpmnssDumpPRVKEY( " sec", nss->sec_key);
rpmnssDumpSECITEM(" sig", nss->sig);
}

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssGenerateDSA(pgpDig dig)
{
    rpmnss nss = dig->impl;
    int rc = 0;		/* assume failure */

    {	CK_MECHANISM_TYPE _type = CKM_DSA_KEY_PAIR_GEN;
	PK11SlotInfo * _slot = PK11_GetBestSlot(_type, NULL);
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_FALSE;
	void * _cx = NULL;

	if (_slot) {

static const unsigned char P[] = { 0,
       0x98, 0xef, 0x3a, 0xae, 0x70, 0x98, 0x9b, 0x44,
       0xdb, 0x35, 0x86, 0xc1, 0xb6, 0xc2, 0x47, 0x7c,
       0xb4, 0xff, 0x99, 0xe8, 0xae, 0x44, 0xf2, 0xeb,
       0xc3, 0xbe, 0x23, 0x0f, 0x65, 0xd0, 0x4c, 0x04,
       0x82, 0x90, 0xa7, 0x9d, 0x4a, 0xc8, 0x93, 0x7f,
       0x41, 0xdf, 0xf8, 0x80, 0x6b, 0x0b, 0x68, 0x7f,
       0xaf, 0xe4, 0xa8, 0xb5, 0xb2, 0x99, 0xc3, 0x69,
       0xfb, 0x3f, 0xe7, 0x1b, 0xd0, 0x0f, 0xa9, 0x7a,
       0x4a, 0x04, 0xbf, 0x50, 0x9e, 0x22, 0x33, 0xb8,
       0x89, 0x53, 0x24, 0x10, 0xf9, 0x68, 0x77, 0xad,
       0xaf, 0x10, 0x68, 0xb8, 0xd3, 0x68, 0x5d, 0xa3,
       0xc3, 0xeb, 0x72, 0x3b, 0xa0, 0x0b, 0x73, 0x65,
       0xc5, 0xd1, 0xfa, 0x8c, 0xc0, 0x7d, 0xaa, 0x52,
       0x29, 0x34, 0x44, 0x01, 0xbf, 0x12, 0x25, 0xfe,
       0x18, 0x0a, 0xc8, 0x3f, 0xc1, 0x60, 0x48, 0xdb,
       0xad, 0x93, 0xb6, 0x61, 0x67, 0xd7, 0xa8, 0x2d };
static const unsigned char Q[] = { 0,
       0xb5, 0xb0, 0x84, 0x8b, 0x44, 0x29, 0xf6, 0x33,
       0x59, 0xa1, 0x3c, 0xbe, 0xd2, 0x7f, 0x35, 0xa1,
       0x76, 0x27, 0x03, 0x81                         };
static const unsigned char G[] = {
       0x04, 0x0e, 0x83, 0x69, 0xf1, 0xcd, 0x7d, 0xe5,
       0x0c, 0x78, 0x93, 0xd6, 0x49, 0x6f, 0x00, 0x04,
       0x4e, 0x0e, 0x6c, 0x37, 0xaa, 0x38, 0x22, 0x47,
       0xd2, 0x58, 0xec, 0x83, 0x12, 0x95, 0xf9, 0x9c,
       0xf1, 0xf4, 0x27, 0xff, 0xd7, 0x99, 0x57, 0x35,
       0xc6, 0x64, 0x4c, 0xc0, 0x47, 0x12, 0x31, 0x50,
       0x82, 0x3c, 0x2a, 0x07, 0x03, 0x01, 0xef, 0x30,
       0x09, 0x89, 0x82, 0x41, 0x76, 0x71, 0xda, 0x9e,
       0x57, 0x8b, 0x76, 0x38, 0x37, 0x5f, 0xa5, 0xcd,
       0x32, 0x84, 0x45, 0x8d, 0x4c, 0x17, 0x54, 0x2b,
       0x5d, 0xc2, 0x6b, 0xba, 0x3e, 0xa0, 0x7b, 0x95,
       0xd7, 0x00, 0x42, 0xf7, 0x08, 0xb8, 0x83, 0x87,
       0x60, 0xe1, 0xe5, 0xf4, 0x1a, 0x54, 0xc2, 0x20,
       0xda, 0x38, 0x3a, 0xd1, 0xb6, 0x10, 0xf4, 0xcb,
       0x35, 0xda, 0x97, 0x92, 0x87, 0xd6, 0xa5, 0x37,
       0x62, 0xb4, 0x93, 0x4a, 0x15, 0x21, 0xa5, 0x10 };
static const SECKEYPQGParams default_pqg_params = {
    NULL,
    { 0, (unsigned char *)P, sizeof(P) },
    { 0, (unsigned char *)Q, sizeof(Q) },
    { 0, (unsigned char *)G, sizeof(G) }
};
void * params = (void *)&default_pqg_params;

#ifdef	DYING
rpmnssDumpSLOTINFO("\tGetBestSlot", _slot, _type);
#endif

	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, params,
			&nss->pub_key, _isPerm, _isSensitive, _cx);

	    PK11_FreeSlot(_slot);
	}
    }

    rc = (nss->sec_key && nss->pub_key);

if (1 || _pgp_debug) {
rpmnssDumpPRVKEY(" sec", nss->sec_key);
rpmnssDumpPUBKEY(" pub", nss->pub_key);
}
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmnss nss = dig->impl;
    int rc = 1;		/* XXX always fail. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */

SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssVerifyELG(pgpDig dig)
{
    int rc = 0;		/* assume failure */
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssSignELG(pgpDig dig)
{
    int rc = 0;		/* assume failure */
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssGenerateELG(pgpDig dig)
{
    int rc = 0;		/* assume failure */
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmnss nss = dig->impl;
    int rc = 1;		/* assume failure */;
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

nss->digest = _free(nss->digest);
nss->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&nss->digest, &nss->digestlen, 0);

#ifdef	DYING
    nss->sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_SHA1:
	nss->sigalg = SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE;
	break;
    case PGPHASHALGO_SHA224:
	nss->sigalg = SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE;
	break;
    case PGPHASHALGO_SHA256:
	nss->sigalg = SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE;
	break;
    case PGPHASHALGO_SHA384:
	nss->sigalg = SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE;
	break;
    case PGPHASHALGO_SHA512:
	nss->sigalg = SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE;
	break;
    default:
	break;
    }
#endif
    if (nss->sigalg == SEC_OID_UNKNOWN)
	goto exit;
assert(nss->sigalg != 0);

#ifdef	NOTYET
    /* Compare leading 16 bits of digest for quick check. */
    rc = memcmp(nss->digest, sigp->signhash16, sizeof(sigp->signhash16));
#else
    rc = 0;
#endif

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssVerifyECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure */;

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE;
	break;
    case PGPHASHALGO_SHA224:
	sigalg = SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE;
	break;
    case PGPHASHALGO_SHA256:
	sigalg = SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE;
	break;
    case PGPHASHALGO_SHA384:
	sigalg = SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE;
	break;
    case PGPHASHALGO_SHA512:
	sigalg = SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE;
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

    rc = rpmnssErr(nss, "VFY_VerifyDigest",
	    VFY_VerifyDigest(&nss->item, nss->pub_key, nss->sig, sigalg, NULL));
    rc = (rc == SECSuccess);

exit:
if (1 || _pgp_debug) {
rpmnssDumpSECITEM("hash", &nss->item);
rpmnssDumpPUBKEY( " pub", nss->pub_key);
rpmnssDumpSECITEM(" sig", nss->sig);
}
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssSignECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmnss nss = dig->impl;
pgpDigParams sigp = pgpGetSignature(dig);
    int rc = 0;		/* assume failure. */
#ifdef	NOTYET
SECItem sig = { .type = siBuffer, .len = 0, .data = NULL };
#endif

SECOidTag sigalg = SEC_OID_UNKNOWN;
    switch (sigp->hash_algo) {
    case PGPHASHALGO_SHA1:
	sigalg = SEC_OID_SHA1;
	break;
    case PGPHASHALGO_SHA224:
	break;
    case PGPHASHALGO_SHA256:
	sigalg = SEC_OID_SHA256;
	break;
    case PGPHASHALGO_SHA384:
	sigalg = SEC_OID_SHA384;
	break;
    case PGPHASHALGO_SHA512:
	sigalg = SEC_OID_SHA512;
	break;
    default:
	break;
    }
    if (sigalg == SEC_OID_UNKNOWN)
	goto exit;

    nss->item.type = siBuffer;
    nss->item.data = nss->digest;
    nss->item.len = (unsigned) nss->digestlen;

if (nss->sig != NULL) {
    SECITEM_ZfreeItem(nss->sig, PR_TRUE);
    nss->sig = NULL;
}

nss->sig = SECITEM_AllocItem(NULL, NULL, 0);
nss->sig->type = siBuffer;

#ifdef	NOTYET
    rc = rpmnssErr(nss, "SGN_Digest",
	    SGN_Digest(nss->sec_key, sigalg, &sig, &nss->item));

    if (rc == SECSuccess)
	rc = rpmnssErr(nss, "DSAU_EncodeDerSig",
		DSAU_EncodeDerSig(nss->sig, &sig));

    sig.data = _free(sig.data);
#else
    rc = rpmnssErr(nss, "SGN_Digest",
	    SGN_Digest(nss->sec_key, sigalg, nss->sig, &nss->item));
#endif

    rc = (rc == SECSuccess);

if (1 || _pgp_debug) {
rpmnssDumpSECITEM("hash", &nss->item);
rpmnssDumpPRVKEY( " sec", nss->sec_key);
rpmnssDumpSECITEM(" sig", nss->sig);
}

exit:
SPEW(!rc, rc, dig);
    return rc;
}

static
int rpmnssGenerateECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmnss nss = dig->impl;
    int rc = 0;		/* assume failure. */

    {	CK_MECHANISM_TYPE _type = CKM_EC_KEY_PAIR_GEN;
	PK11SlotInfo * _slot = PK11_GetBestSlot(_type, NULL);
	int _isPerm = PR_FALSE;
	int _isSensitive = PR_FALSE;
	void * _cx = NULL;

	if (_slot) {

#ifdef	DYING
rpmnssDumpSLOTINFO("\tGetBestSlot", _slot, _type);
#endif

	    nss->sec_key = PK11_GenerateKeyPair(_slot, _type, nss->ecparams,
			&nss->pub_key, _isPerm, _isSensitive, _cx);

	    PK11_FreeSlot(_slot);
	}
    }

    rc = (nss->sec_key && nss->pub_key);

if (1 || _pgp_debug) {
rpmnssDumpPRVKEY(" sec", nss->sec_key);
rpmnssDumpPUBKEY(" pub", nss->pub_key);
}
SPEW(!rc, rc, dig);

    return rc;
}

static int rpmnssErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	NOTYET
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmnssStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmnssAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgnssvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmnssAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgnssvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmnssAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmnssAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmnssVerify(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmnssVerifyRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmnssVerifyDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmnssVerifyELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmnssVerifyECDSA(dig);
	break;
    }
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssSign(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmnssSignRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmnssSignDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmnssSignELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmnssSignECDSA(dig);
	break;
    }
SPEW(!rc, rc, dig);
    return rc;
}

static int rpmnssGenerate(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);

{   rpmnss nss = dig->impl;
    if (nss->sec_key != NULL) {
	SECKEY_DestroyPrivateKey(nss->sec_key);
	nss->sec_key = NULL;
    }
    if (nss->pub_key != NULL) {
	SECKEY_DestroyPublicKey(nss->pub_key);
	nss->pub_key = NULL;
    }
}

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmnssGenerateRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmnssGenerateDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmnssGenerateELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
rpmnssLoadParams(dig, "nistp256");
	rc = rpmnssGenerateECDSA(dig);
	break;
    }

SPEW(!rc, rc, dig);

    return rc;
}

#endif	/* _RPMNSS_INTERNAL */

/*==============================================================*/

#if defined(_RPMSSL_INTERNAL)
/* XXX sorted table */
static KEY rpmsslNIDS[] = {
  { "c2pnb163v1", 684 },	/* X9.62 curve over a 163 bit binary field */
  { "c2pnb163v2", 685 },	/* X9.62 curve over a 163 bit binary field */
  { "c2pnb163v3", 686 },	/* X9.62 curve over a 163 bit binary field */
  { "c2pnb176v1", 687 },	/* X9.62 curve over a 176 bit binary field */
  { "c2pnb208w1", 693 },	/* X9.62 curve over a 208 bit binary field */
  { "c2pnb272w1", 699 },	/* X9.62 curve over a 272 bit binary field */
  { "c2pnb304w1", 700 },	/* X9.62 curve over a 304 bit binary field */
  { "c2pnb368w1", 702 },	/* X9.62 curve over a 368 bit binary field */
  { "c2tnb191v1", NID_X9_62_c2tnb191v1 },	/* X9.62 curve over a 191 bit binary field */
  { "c2tnb191v2", 689 },	/* X9.62 curve over a 191 bit binary field */
  { "c2tnb191v3", 690 },	/* X9.62 curve over a 191 bit binary field */
  { "c2tnb239v1", NID_X9_62_c2tnb239v1 },	/* X9.62 curve over a 239 bit binary field */
  { "c2tnb239v2", 695 },	/* X9.62 curve over a 239 bit binary field */
  { "c2tnb239v3", 696 },	/* X9.62 curve over a 239 bit binary field */
  { "c2tnb359v1", 701 },	/* X9.62 curve over a 359 bit binary field */
  { "c2tnb431r1", 703 },	/* X9.62 curve over a 431 bit binary field */

  { "nistp192", 711 },			/* XXX */
  { "nistp224", NID_secp224r1 },	/* XXX NIST/SECG */
  { "nistp256", NID_X9_62_prime256v1 },	/* XXX */
  { "nistp384", NID_secp384r1 },	/* XXX NIST/SECG */
  { "nistp521", NID_secp521r1 },	/* XXX NIST/SECG */

  { "prime192v1", NID_X9_62_prime192v1 }, /* NIST/X9.62/SECG curve over a 192 bit prime field */
  { "prime192v2", 410 },	/* X9.62 curve over a 192 bit prime field */
  { "prime192v3", 411 },	/* X9.62 curve over a 192 bit prime field */
  { "prime239v1", NID_X9_62_prime239v1 },	/* X9.62 curve over a 239 bit prime field */
  { "prime239v2", 413 },	/* X9.62 curve over a 239 bit prime field */
  { "prime239v3", 414 },	/* X9.62 curve over a 239 bit prime field */
  { "prime256v1", NID_X9_62_prime256v1 },	/* X9.62/SECG curve over a 256 bit prime field */
  { "secp112r1", 704 },	/* SECG/WTLS curve over a 112 bit prime field */
  { "secp112r2", 705 },	/* SECG curve over a 112 bit prime field */
  { "secp128r1", 706 },	/* SECG curve over a 128 bit prime field */
  { "secp128r2", 707 },	/* SECG curve over a 128 bit prime field */
  { "secp160k1", 708 },	/* SECG curve over a 160 bit prime field */
  { "secp160r1", 709 },	/* SECG curve over a 160 bit prime field */
  { "secp160r2", 710 },	/* SECG/WTLS curve over a 160 bit prime field */
  { "secp192k1", 711 },	/* SECG curve over a 192 bit prime field */
  { "secp224k1", 712 },	/* SECG curve over a 224 bit prime field */
  { "secp224r1", NID_secp224r1 },	/* NIST/SECG curve over a 224 bit prime field */
  { "secp256k1", 714 },	/* SECG curve over a 256 bit prime field */
  { "secp384r1", NID_secp384r1 },	/* NIST/SECG curve over a 384 bit prime field */
  { "secp521r1", NID_secp521r1 },	/* NIST/SECG curve over a 521 bit prime field */
  { "sect113r1", 717 },	/* SECG curve over a 113 bit binary field */
  { "sect113r2", 718 },	/* SECG curve over a 113 bit binary field */
  { "sect131r1", 719 },	/* SECG/WTLS curve over a 131 bit binary field */
  { "sect131r2", 720 },	/* SECG curve over a 131 bit binary field */
  { "sect163k1", 721 },	/* NIST/SECG/WTLS curve over a 163 bit binary field */
  { "sect163r1", 722 },	/* SECG curve over a 163 bit binary field */
  { "sect163r2", 723 },	/* NIST/SECG curve over a 163 bit binary field */
  { "sect193r1", 724 },	/* SECG curve over a 193 bit binary field */
  { "sect193r2", 725 },	/* SECG curve over a 193 bit binary field */
  { "sect233k1", 726 },	/* NIST/SECG/WTLS curve over a 233 bit binary field */
  { "sect233r1", 727 },	/* NIST/SECG/WTLS curve over a 233 bit binary field */
  { "sect239k1", 728 },	/* SECG curve over a 239 bit binary field */
  { "sect283k1", 729 },	/* NIST/SECG curve over a 283 bit binary field */
  { "sect283r1", 730 },	/* NIST/SECG curve over a 283 bit binary field */
  { "sect409k1", 731 },	/* NIST/SECG curve over a 409 bit binary field */
  { "sect409r1", 732 },	/* NIST/SECG curve over a 409 bit binary field */
  { "sect571k1", 733 },	/* NIST/SECG curve over a 571 bit binary field */
  { "sect571r1", 734 },	/* NIST/SECG curve over a 571 bit binary field */
  { "wap-wsg-idm-ecid-wtls10", 743 },	/* NIST/SECG/WTLS curve over a 233 bit binary field */
  { "wap-wsg-idm-ecid-wtls11", 744 },	/* NIST/SECG/WTLS curve over a 233 bit binary field */
  { "wap-wsg-idm-ecid-wtls12", 745 },	/* WTLS curvs over a 224 bit prime field */
  { "wap-wsg-idm-ecid-wtls1", 735 },	/* WTLS curve over a 113 bit binary field */
  { "wap-wsg-idm-ecid-wtls3", 736 },	/* NIST/SECG/WTLS curve over a 163 bit binary field */
  { "wap-wsg-idm-ecid-wtls4", 737 },	/* SECG curve over a 113 bit binary field */
  { "wap-wsg-idm-ecid-wtls5", 738 },	/* X9.62 curve over a 163 bit binary field */
  { "wap-wsg-idm-ecid-wtls6", 739 },	/* SECG/WTLS curve over a 112 bit prime field */
  { "wap-wsg-idm-ecid-wtls7", 740 },	/* SECG/WTLS curve over a 160 bit prime field */
  { "wap-wsg-idm-ecid-wtls8", 741 },	/* WTLS curve over a 112 bit prime field */
  { "wap-wsg-idm-ecid-wtls9", 742 },	/* WTLS curve over a 160 bit prime field */
};
static size_t nrpmsslNIDS = sizeof(rpmsslNIDS) / sizeof(rpmsslNIDS[0]);

static int curve2nid(const char * name)
{
    int nid = keyValue(rpmsslNIDS, nrpmsslNIDS, name);
fprintf(stderr, "<-- %s(%s) nid %u\n", __FUNCTION__, name, nid);
    return nid;
}

/**
 * Convert hex to binary nibble.
 * @param c            hex character
 * @return             binary nibble
 */
static
unsigned char nibble(char c)
	/*@*/
{
    if (c >= '0' && c <= '9')
	return (unsigned char) (c - '0');
    if (c >= 'A' && c <= 'F')
	return (unsigned char)((int)(c - 'A') + 10);
    if (c >= 'a' && c <= 'f')
	return (unsigned char)((int)(c - 'a') + 10);
    return (unsigned char) '\0';
}

/*@-modfilesys@*/
static
void hexdump(const char * msg, unsigned char * b, size_t blen)
	/*@*/
{
    static const char hex[] = "0123456789ABCDEF";

    fprintf(stderr, "*** %s:", msg);
    if (b != NULL)
    while (blen > 0) {
	fprintf(stderr, "%c%c",
		hex[ (unsigned)((*b >> 4) & 0x0f) ],
		hex[ (unsigned)((*b     ) & 0x0f) ]);
	blen--;
	b++;
    }
    fprintf(stderr, "\n");
    return;
}
/*@=modfilesys@*/

static int _rpmssl_spew;

static int rpmsslLoadBN(BIGNUM ** bnp, const char * bnstr, int spew)
{
    int rc;

    *bnp = NULL;
    if (bnstr[0] == '0' && bnstr[1] == 'x')
	rc = BN_hex2bn(bnp, bnstr + 2);
    else
	rc = BN_dec2bn(bnp, bnstr);

    if (rc && (spew || _rpmssl_spew)) {
	char * t;
	rpmlog(RPMLOG_DEBUG, "\t%p: %s\n", *bnp, t=BN_bn2dec(*bnp));
	OPENSSL_free(t);
	rpmlog(RPMLOG_DEBUG, "\t%p: 0x%s\n", *bnp, t=BN_bn2hex(*bnp));
	OPENSSL_free(t);
    }

    return rc;
}

#define SetKey \
  BIGNUM * cex = NULL; \
  int xx; \
  xx = rpmsslLoadBN(&key->n, n, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->e, e, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->d, d, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->p, p, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->q, q, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->dmp1, dmp1, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->dmq1, dmq1, _rpmssl_spew); \
  xx = rpmsslLoadBN(&key->iqmp, iqmp, _rpmssl_spew); \
  xx = rpmsslLoadBN(&cex, ctext_ex, _rpmssl_spew); \
  xx = BN_bn2bin(cex, c); \
  BN_free(cex); \
  return xx;

static int key1(RSA *key, unsigned char *c)
{
    static char n[] = "0x"
	"00AA36ABCE88ACFDFF55523C7FC4523F90EFA00DF3774A259F2E62B4C5D99CB5"
	"ADB300A0285E5301930E0C70FB6876939CE616CE624A11E0086D341EBCACA0A1"
	"F5";
    static char e[] = "0x" "11";
    static char d[] =
	"0A033748626487695F5F30BC38B98B44C2CD2DFF434098CD20D8A138D090BF64"
	"797C3FA7A2CDCB3CD1E0BDBA2654B4F9DF8E8AE59D733D9F33B301624AFD1D51";
    static char p[] = "0x"
	"00D840B41666B42E92EA0DA3B43204B5CFCE3352524D0416A5A441E700AF4612"
	"0D";
    static char q[] = "0x"
	"00C97FB1F027F453F6341233EAAAD1D9353F6C42D08866B1D05A0F2035028B9D"
	"89";
    static char dmp1[] = "0x"
	"590B9572A2C2A9C406059DC2AB2F1DAFEB7E8B4F10A7549E8EEDF5B4FCE09E05";
    static char dmq1[] = "0x"
	"008E3C0521FE15E0EA06A36FF0F10C9952C35B7A7514FD3238B80AAD5298628D"
	"51";
    static char iqmp[] = "0x"
	"363FF7189DA8E90B1D341F71D09B76A8A943E11D10B24D249F2DEAFEF80C1826";
    static char ctext_ex[] = "0x"
	"1b8f05f9ca1a79526e53f3cc514fdb892bfb9193231e78b992e68d50a480cb52"
	"33895c74958d5d02ab8c0fd040eb5844b005c39ed8274a9dbfa80671409439d2";

    SetKey;
}

static int key2(RSA *key, unsigned char *c)
{
    static char n[] = "0x"
	"00A3079A90DF0DFD72AC090CCC2A78B87413133E40759C98FAF8204F358A0B26"
	"3C6770E783A93B6971B73779D2717BE83477CF";
    static char e[] = "0x" "3";
    static char d[] = "0x"
	"6CAFBC6094B3FE4C72B0B332C6FB25A2B76229804E6865FCA45A74DF0F8FB841"
	"3B52C0D0E53D9B590FF19BE79F49DD21E5EB";
    static char p[] = "0x"
	"00CF2035028B9D869840B41666B42E92EA0DA3B43204B5CFCE91";
    static char q[] = "0x"
	"00C97FB1F027F453F6341233EAAAD1D9353F6C42D08866B1D05F";
    static char dmp1[] = "0x"
	"008A1578AC5D13AF102B22B999CD7461F15E6D22CC0323DFDF0B";
    static char dmq1[] = "0x"
	"008655214AC54D8D4ECD6177F1C73690CE2A482C8B0599CBE03F";
    static char iqmp[] = "0x"
	"0083EFEFB8A9A40D1DB6ED98AD84ED1335DCC108F322D057CF8D";
    static char ctext_ex[] = "0x"
	"14bddd28c98335192380e8e549b1582a8b40b4486d03a6a5311f1fd5f0a180e4"
	"17530329a9349074b1521354290824526251";

    SetKey;
}

static int key3(RSA *key, unsigned char *c)
{
    static char n[] = "0x"
	"00BBF82F090682CE9C2338AC2B9DA871F7368D07EED41043A440D6B6F07454F5"
	"1FB8DFBAAF035C02AB61EA48CEEB6FCD4876ED520D60E1EC4619719D8A5B8B80"
	"7FAFB8E0A3DFC737723EE6B4B7D93A2584EE6A649D060953748834B245459839"
	"4EE0AAB12D7B61A51F527A9A41F6C1687FE2537298CA2A8F5946F8E5FD091DBD"
	"CB";
    static char e[] = "0x" "11";
    static char d[] = "0x"
	"00A5DAFC5341FAF289C4B988DB30C1CDF83F31251E0668B42784813801579641"
	"B29410B3C7998D6BC465745E5C392669D6870DA2C082A939E37FDCB82EC93EDA"
	"C97FF3AD5950ACCFBC111C76F1A9529444E56AAF68C56C092CD38DC3BEF5D20A"
	"939926ED4F74A13EDDFBE1A1CECC4894AF9428C2B7B8883FE4463A4BC85B1CB3"
	"C1";
    static char p[] = "0x"
	"00EECFAE81B1B9B3C908810B10A1B5600199EB9F44AEF4FDA493B81A9E3D84F6"
	"32124EF0236E5D1E3B7E28FAE7AA040A2D5B252176459D1F397541BA2A58FB65"
	"99";
    static char q[] = "0x"
	"00C97FB1F027F453F6341233EAAAD1D9353F6C42D08866B1D05A0F2035028B9D"
	"869840B41666B42E92EA0DA3B43204B5CFCE3352524D0416A5A441E700AF4615"
	"03";
    static char dmp1[] = "0x"
	"54494CA63EBA0337E4E24023FCD69A5AEB07DDDC0183A4D0AC9B54B051F2B13E"
	"D9490975EAB77414FF59C1F7692E9A2E202B38FC910A474174ADC93C1F67C981";
    static char dmq1[] = "0x"
	"471E0290FF0AF0750351B7F878864CA961ADBD3A8A7E991C5C0556A94C3146A7"
	"F9803F8F6F8AE342E931FD8AE47A220D1B99A495849807FE39F9245A9836DA3D";
    static char iqmp[] = "0x"
	"00B06C4FDABB6301198D265BDBAE9423B380F271F73453885093077FCD39E211"
	"9FC98632154F5883B167A967BF402B4E9E2E0F9656E698EA3666EDFB25798039"
	"F7";
    static char ctext_ex[] = "0x"
	"b8246b56a6ed5881aeb585d9a25b2ad790c417e080681bf1ac2bc3deb69d8bce"
	"f0c4366fec400af052a72e9b0effb5b3f2f192dbeaca03c12740057113bf1f06"
	"69ac22e9f3a7852e3c15d913cab0b8863a95c99294ce8674214954610346f4d4"
	"74b26f7c48b42ee68e1f572a1fc4026ac456b4f59f7b621ea1b9d88f64202fb1";

    SetKey;
}

static unsigned char ptext_ex[] = "\x54\x85\x9b\x34\x2c\x49\xea\x2a";
static size_t plen = sizeof(ptext_ex) - 1;
static unsigned char ptext[256];
static unsigned char ctext[256];
static unsigned char ctext_ex[256];
static size_t clen = 0;

static void rpmsslLoadKEY(pgpDig dig, int v)
{
    rpmssl ssl = dig->impl;

    ssl->rsa = RSA_new();
    switch (v%3) {
    case 0:
	clen = key1(ssl->rsa, ctext_ex);
	break;
    case 1:
	clen = key2(ssl->rsa, ctext_ex);
	break;
    case 2:
	clen = key3(ssl->rsa, ctext_ex);
	break;
    }
    if (v/3 >= 1) ssl->rsa->flags |= RSA_FLAG_NO_CONSTTIME;
}

static int rpmsslTestPKCS(pgpDig dig)
{
    rpmssl ssl = dig->impl;
    const char * msg;
    int num;
    int rc = 0;		/* assume failure */

ERR_clear_error();
    num = RSA_public_encrypt(plen, ptext_ex, ctext, ssl->rsa,
				 RSA_PKCS1_PADDING);
    msg = "encryption";
    if (num != (int)clen)
	goto exit;
  
    num = RSA_private_decrypt(num, ctext, ptext, ssl->rsa,
				  RSA_PKCS1_PADDING);
    msg = "decryption";
    if (num != (int)plen || memcmp(ptext, ptext_ex, num) != 0)
	goto exit;
    msg = "encryption/decryption";
    rc = 1;

exit:
    fprintf(stderr, "PKCS #1 v1.5 %s %s\n", msg, (!rc ? "failed" : "ok"));
    return rc;		/* XXX 1 on success */
}

static int pad_unknown(void)
{
    unsigned long l;
    while ((l = ERR_get_error()) != 0)
      if (ERR_GET_REASON(l) == RSA_R_UNKNOWN_PADDING_TYPE)
	return 1;
    return 0;
}

static int rpmsslTestOAEP(pgpDig dig)
{
    rpmssl ssl = dig->impl;
    const char * msg;
    int num;
    size_t n;
    int rc = 0;		/* assume failure */

ERR_clear_error();

    num = RSA_public_encrypt(plen, ptext_ex, ctext, ssl->rsa,
				 RSA_PKCS1_OAEP_PADDING);
    msg = "unsupported";
    if (num == -1 && pad_unknown())
	goto exit;
    msg = "encryption";
    if (num != (int)clen)
	goto exit;

    num = RSA_private_decrypt(num, ctext, ptext, ssl->rsa,
				  RSA_PKCS1_OAEP_PADDING);
    msg = "decryption (encrypted data)";
    if (num != (int)plen || memcmp(ptext, ptext_ex, num) != 0)
	goto exit;
    if (memcmp(ctext, ctext_ex, num) != 0) {
	/* Different ciphertexts (rsa_oaep.c w/o -DPKCS_TESTVECT). */
	num = RSA_private_decrypt(clen, ctext_ex, ptext, ssl->rsa,
				  RSA_PKCS1_OAEP_PADDING);

	msg = "decryption (test vector data)";
	if (num != (int)plen || memcmp(ptext, ptext_ex, num) != 0)
	    goto exit;
    }

    /* Try decrypting corrupted ciphertexts */
    msg = "decryption (corrupt data)";
    for (n = 0 ; n < clen ; ++n) {
	unsigned char saved = ctext[n];
	int b;
	for (b = 0 ; b < 256 ; ++b) {
	    if (b == saved)
		continue;
	    ctext[n] = b;
	    num = RSA_private_decrypt(num, ctext, ptext, ssl->rsa,
					  RSA_PKCS1_OAEP_PADDING);
	    if (num > 0)
		goto exit;
	}
    }

    msg = "encryption/decryption";
    rc = 1;

exit:
    fprintf(stderr, "OAEP %s %s\n", msg, (!rc ? "failed" : "ok"));
    return rc;		/* XXX 1 on success */
}

#define	_spewBN(_N, _BN)	\
  { BIGNUM * bn = _BN; \
    char * t = bn ? BN_bn2hex(bn) : ""; \
    fprintf(stderr, "\t " _N ": %s\n", t); \
    if (bn) OPENSSL_free(t); \
  }

static void rpmsslDumpRSA(const char * msg, rpmssl ssl)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    if (ssl->rsa) {
	_spewBN(" n", ssl->rsa->n);
	_spewBN(" e", ssl->rsa->e);
	_spewBN(" d", ssl->rsa->d);
	_spewBN(" p", ssl->rsa->p);
	_spewBN(" q", ssl->rsa->q);
	_spewBN("dp", ssl->rsa->dmp1);
	_spewBN("dq", ssl->rsa->dmq1);
	_spewBN("qi", ssl->rsa->iqmp);
    }

    _spewBN("hm", ssl->hm);
    _spewBN(" c", ssl->c);
}

static void rpmsslDumpDSA(const char * msg, rpmssl ssl)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    if (ssl->dsa) {
	_spewBN(" p", ssl->dsa->p);
	_spewBN(" q", ssl->dsa->q);
	_spewBN(" g", ssl->dsa->g);
	_spewBN(" x", ssl->dsa->priv_key);
	_spewBN(" y", ssl->dsa->pub_key);
    }

    if (ssl->dsasig) {
	_spewBN(" r", ssl->dsasig->r);
	_spewBN(" s", ssl->dsasig->s);
    }

    _spewBN("hm", ssl->hm);
}

static void rpmsslDumpECDSA(const char * msg, rpmssl ssl)
{
    if (msg) fprintf(stderr, "========== %s\n", msg);

    if (ssl->ecdsakey) {
	const EC_GROUP * group = EC_KEY_get0_group(ssl->ecdsakey);
	const EC_METHOD * method = EC_GROUP_method_of(group);
	BIGNUM * n = BN_new();
	BIGNUM * h = BN_new();
	BIGNUM * p = BN_new();
	BIGNUM * a = BN_new();
	BIGNUM * b = BN_new();
	const EC_POINT * G = EC_GROUP_get0_generator(group);
	BIGNUM * Gx = BN_new();
	BIGNUM * Gy = BN_new();
	const BIGNUM * d = EC_KEY_get0_private_key(ssl->ecdsakey);
	const EC_POINT * Q = EC_KEY_get0_public_key(ssl->ecdsakey);
	BIGNUM * Qx = BN_new();
	BIGNUM * Qy = BN_new();
	unsigned char * seed = EC_GROUP_get0_seed(group);
	size_t seed_len = EC_GROUP_get_seed_len(group);
	int xx;

	fprintf(stderr, "\tfield_type: %d\n", EC_METHOD_get_field_type(method));
	fprintf(stderr, "\tcurve_name: %d\n", EC_GROUP_get_curve_name(group));
	fprintf(stderr, "\t asn1_flag: %d\n", EC_GROUP_get_asn1_flag(group));
	fprintf(stderr, "\t    degree: %d\n", EC_GROUP_get_degree(group));
	if (seed)
	fprintf(stderr, "\t      seed: %s\n", pgpHexStr(seed, seed_len));

	xx = EC_GROUP_get_curve_GFp(group, p, a, b, NULL);
	if (xx) {
	    _spewBN(" p", p);
	    xx = EC_GROUP_get_order(group, n, NULL);
	    if (xx) _spewBN(" n", n);
	    _spewBN(" b", b);
	    _spewBN(" a", a);
	    xx = EC_GROUP_get_cofactor(group, h, NULL);
	    if (xx) _spewBN(" h", h);
	}

	xx = EC_POINT_get_affine_coordinates_GFp(group, G, Gx, Gy, NULL);
	if (xx) {
	    _spewBN("Gx", Gx);
	    _spewBN("Gy", Gy);
	}

	_spewBN(" d", (BIGNUM *)d);
	xx = EC_POINT_get_affine_coordinates_GFp(group, Q, Qx, Qy, NULL);
	if (xx) {
	    _spewBN("Qx", Qx);
	    _spewBN("Qy", Qy);
	}

	if (Qy) BN_free(Qy);
	if (Qx) BN_free(Qx);
	if (Gy) BN_free(Gy);
	if (Gx) BN_free(Gx);
	if (b) BN_free(b);
	if (a) BN_free(a);
	if (p) BN_free(p);
	if (h) BN_free(h);
	if (n) BN_free(n);
    }

    if (ssl->exdsasig) {
	_spewBN(" r", ssl->ecdsasig->r);
	_spewBN(" s", ssl->ecdsasig->s);
    }
}

#undef	_spewBN

static
int rpmsslSetRSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmssl ssl = dig->impl;
    unsigned int nb = 0;
    const char * prefix = rpmDigestASN1(ctx);
    const char * hexstr;
    const char * s;
    rpmuint8_t signhash16[2];
    char * tt;
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    if (prefix == NULL)
	return 1;

    /*
     * The no. of bytes for hash + PKCS1 padding is needed.
     * Either n or c can be used as the size, but different code paths
     * populate n or c indeterminately. So try c, then n,
     * and error if the no. of bytes isn't sane.
     */
#ifdef	DYING
    if (ssl->c)
	nb = BN_num_bytes(ssl->c);
    else if (ssl->rsa->n)
	nb = BN_num_bytes(ssl->rsa->n);
#else
    nb = RSA_size(ssl->rsa);
#endif
    if (nb < 64/8 || nb > 65536/8)	/* XXX generous "sanity" check */
	return 1;

/* XXX FIXME: do PKCS1 padding in binary not hex */
/* XXX FIXME: should this lazy free be done elsewhere? */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 1);
    hexstr = tt = xmalloc(2 * nb + 1);
    memset(tt, (int) 'f', (2 * nb));
    tt[0] = '0'; tt[1] = '0';
    tt[2] = '0'; tt[3] = '1';
    tt += (2 * nb) - strlen(prefix) - strlen(ssl->digest) - 2;
    *tt++ = '0'; *tt++ = '0';
    tt = stpcpy(tt, prefix);
    tt = stpcpy(tt, ssl->digest);

    /* Set RSA hash. */
/*@-moduncon -noeffectuncon @*/
if (ssl->hm) {
BN_free(ssl->hm);
ssl->hm = NULL;
}
    xx = BN_hex2bn(&ssl->hm, hexstr);
/*@=moduncon =noeffectuncon @*/

    hexstr = _free(hexstr);

    /* Compare leading 16 bits of digest for quick check. */
    s = ssl->digest;
/*@-type@*/
    signhash16[0] = (rpmuint8_t) (nibble(s[0]) << 4) | nibble(s[1]);
    signhash16[1] = (rpmuint8_t) (nibble(s[2]) << 4) | nibble(s[3]);
/*@=type@*/
    return memcmp(signhash16, sigp->signhash16, sizeof(sigp->signhash16));
}

static unsigned char * rpmsslBN2bin(const char * msg, const BIGNUM * s, size_t maxn)
{
    unsigned char * t = xcalloc(1, maxn);
/*@-modunconnomods@*/
    size_t nt = BN_bn2bin(s, t);
/*@=modunconnomods@*/

    if (nt < maxn) {
	size_t pad = (maxn - nt);
/*@-modfilesys@*/
if (_pgp_debug < 0) fprintf(stderr, "\tmemmove(%p, %p, %u)\n", t+pad, t, (unsigned)nt);
/*@=modfilesys@*/
	memmove(t+pad, t, nt);
/*@-modfilesys@*/
if (_pgp_debug < 0) fprintf(stderr, "\tmemset(%p, 0, %u)\n", t, (unsigned)pad);
/*@=modfilesys@*/
	memset(t, 0, pad);
    }
/*@-modfilesys@*/
if (_pgp_debug < 0) hexdump(msg, t, maxn);
/*@=modfilesys@*/
    return t;
}

#ifdef	NOTYET
static BIO *bio_err = NULL;
static int rpmssl_cb(int p, int n, BN_GENCB *arg)
{
    static char stuff[] = ".+*\n";
    if (p < 0 || p > 3) p = 2;
    BIO_write(arg->arg, stuff+p, 1);
    (void)BIO_flush(arg->arg);
    return 1;
}
#endif

static
int rpmsslVerifyRSA(pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    size_t maxn;
    unsigned char * hm;
    unsigned char *  c;
    size_t nb;
    size_t i;
    int rc = 0;		/* assume failure */
    int xx;

#ifdef	DYING
assert(ssl->rsa);	/* XXX ensure lazy malloc with parameter set. */
#else
if (!(ssl->rsa && ssl->hm)) return rc;
#endif

/*@-moduncon@*/
    maxn = BN_num_bytes(ssl->rsa->n);
    hm = rpmsslBN2bin("hm", ssl->hm, maxn);
    c = rpmsslBN2bin(" c", ssl->c, maxn);
    nb = RSA_public_decrypt((int)maxn, c, c, ssl->rsa, RSA_PKCS1_PADDING);
/*@=moduncon@*/

    /* Verify RSA signature. */
    /* XXX This is _NOT_ the correct openssl function to use:
     *	rc = RSA_verify(type, m, m_len, sigbuf, siglen, ssl->rsa)
     *
     * Here's what needs doing (from OpenPGP reference sources in 1999):
     *	static u32_t checkrsa(BIGNUM * a, RSA * key, u8_t * hash, int hlen)
     *	{
     *	  u8_t dbuf[MAXSIGM];
     *	  int j, ll;
     *
     *	  j = BN_bn2bin(a, dbuf);
     *	  ll = BN_num_bytes(key->n);
     *	  while (j < ll)
     *	    memmove(&dbuf[1], dbuf, j++), dbuf[0] = 0;
     *	  j = RSA_public_decrypt(ll, dbuf, dbuf, key, RSA_PKCS1_PADDING);
     *	  RSA_free(key);
     *	  return (j != hlen || memcmp(dbuf, hash, j));
     *	}
     */
    for (i = 2; i < maxn; i++) {
	if (hm[i] == 0xff)
	    continue;
	i++;
/*@-modfilesys@*/
if (_pgp_debug < 0) hexdump("HM", hm + i, (maxn - i));
/*@=modfilesys@*/
	break;
    }

/*@-modfilesys@*/
if (_pgp_debug < 0) hexdump("HM", hm + (maxn - nb), nb);
if (_pgp_debug < 0) hexdump(" C",  c, nb);
/*@=modfilesys@*/

    rc = ((maxn - i) == nb && (xx = memcmp(hm+i, c, nb)) == 0);

    c = _free(c);
    hm = _free(hm);

    return rc;
}

static
int rpmsslSignRSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */
    unsigned char *  c = NULL;
    unsigned char * hm = NULL;
    size_t maxn;
#ifdef	DYING
static const char _asn1[] = {
    0x30,0x21,0x30,0x09,0x06,0x05,0x2b, 0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14
};
    size_t nb;
#endif
int xx;

#ifdef	DYING
assert(ssl->rsa);	/* XXX ensure lazy malloc with parameter set. */
#else
if (ssl->rsa == NULL) return rc;
#endif

    maxn = RSA_size(ssl->rsa);
#ifdef	DYING

    nb = maxn - ssl->digestlen;

    hm = xmalloc(maxn);
    hm[0] = 0x00;
    hm[1] = 0x01;
    memset(hm+2, 0xff, nb - 2 - sizeof(_asn1));
    hm[nb - sizeof(_asn1) - 1] = 0x00;
    memcpy(hm + nb - sizeof(_asn1), _asn1, sizeof(_asn1));
    memcpy(hm + nb, ssl->digest, ssl->digestlen);

ssl->hm = BN_bin2bn(hm, maxn, NULL);
#else
assert(ssl->hm);
hm = xmalloc(BN_num_bytes(ssl->hm));
hm = rpmsslBN2bin("hm", ssl->hm, maxn);
#endif

    c = xmalloc(RSA_size(ssl->rsa));
    xx = RSA_private_encrypt((int)maxn, hm, c, ssl->rsa, RSA_NO_PADDING);
    ssl->c = BN_bin2bn(c, maxn, NULL);

    c = _free(c);
    hm = _free(hm);

    rc = (ssl->c != NULL);

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslGenerateRSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */
static unsigned long _e = 0x10001;
BIGNUM * bn = BN_new();

assert(bn);
assert(ssl->nbits);

    if ((ssl->rsa = RSA_new()) != NULL
     && BN_set_word(bn, _e)
     && RSA_generate_key_ex(ssl->rsa, ssl->nbits, bn, NULL))
	rc = 1;
    if (!rc && ssl->rsa) {
	RSA_free(ssl->rsa);
	ssl->rsa = NULL;
    }
if (bn) BN_free(bn);

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSetDSA(/*@only@*/ DIGEST_CTX ctx, pgpDig dig, pgpDigParams sigp)
	/*@modifies dig @*/
{
    rpmssl ssl = dig->impl;
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    /* Set DSA hash. */
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
    xx = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */
    return memcmp(ssl->digest, sigp->signhash16, sizeof(sigp->signhash16));
}

static
int rpmsslVerifyDSA(pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure */

#ifdef	DYING
assert(ssl->dsa);	/* XXX ensure lazy malloc with parameter set. */
#else
if (!(ssl->digest && ssl->dsasig && ssl->dsa)) return rc;
#endif

    /* Verify DSA signature. */
    rc = DSA_do_verify(ssl->digest, (int)ssl->digestlen, ssl->dsasig, ssl->dsa);
    rc = (rc == 1);

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSignDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure */

#ifdef	DYING
assert(ssl->dsa);	/* XXX ensure lazy malloc with parameter set. */
#else
if (ssl->dsa == NULL) return rc;
#endif

    ssl->dsasig = DSA_do_sign(ssl->digest, ssl->digestlen, ssl->dsa);
    rc = (ssl->dsasig != NULL);

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslGenerateDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */

assert(ssl->nbits);

    if ((ssl->dsa = DSA_new()) != NULL
     && DSA_generate_parameters_ex(ssl->dsa, ssl->nbits,
		NULL, 0, NULL, NULL, NULL)
     && DSA_generate_key(ssl->dsa))
	rc = 1;
    if (!rc && ssl->dsa) {
	DSA_free(ssl->dsa);
	ssl->dsa = NULL;
    }

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSetELG(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 1;		/* XXX always fail. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));
    xx = rpmDigestFinal(ctx, (void **)&ssl->digest, &ssl->digestlen, 0);

    /* Compare leading 16 bits of digest for quick check. */

    return rc;
}

static
int rpmsslVerifyELG(/*@unused@*/pgpDig dig)
	/*@*/
{
    int rc = 0;		/* XXX always fail. */

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSignELG(/*@unused@*/pgpDig dig)
	/*@*/
{
    int rc = 0;		/* XXX always fail. */

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslGenerateELG(/*@unused@*/pgpDig dig)
	/*@*/
{
    int rc = 0;		/* XXX always fail. */

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSetECDSA(/*@only@*/ DIGEST_CTX ctx, /*@unused@*/pgpDig dig, pgpDigParams sigp)
	/*@*/
{
    int rc = 1;		/* assume failure. */
    int xx;

assert(sigp->hash_algo == rpmDigestAlgo(ctx));

#if !defined(OPENSSL_NO_ECDSA)
    {	rpmssl ssl = dig->impl;
ssl->digest = _free(ssl->digest);
ssl->digestlen = 0;
	xx = rpmDigestFinal(ctx, &ssl->digest, &ssl->digestlen, 0);
    }

    /* Compare leading 16 bits of digest for quick check. */
    rc = 0;
#else
    xx = rpmDigestFinal(ctx, (void **)NULL, NULL, 0);
#endif

    return rc;
}

static
int rpmsslVerifyECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */

#ifdef	DYING
assert(ssl->ecdsakey);	/* XXX ensure lazy malloc with parameter set. */
#else
if (ssl->ecdsakey == NULL) return rc;
#endif

#if !defined(OPENSSL_NO_ECDSA)
    rc = ECDSA_do_verify(ssl->digest, ssl->digestlen, ssl->ecdsasig, ssl->ecdsakey);
    rc = (rc == 1);
#endif

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslSignECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */

#ifdef	DYING
assert(ssl->ecdsakey);	/* XXX ensure lazy malloc with parameter set. */
#else
if (ssl->ecdsakey == NULL) return rc;
#endif

#if !defined(OPENSSL_NO_ECDSA)
    ssl->ecdsasig = ECDSA_do_sign(ssl->digest, ssl->digestlen, ssl->ecdsakey);
    rc = (ssl->ecdsasig != NULL);
#endif

SPEW(!rc, rc, dig);

    return rc;
}

static
int rpmsslGenerateECDSA(/*@unused@*/pgpDig dig)
	/*@*/
{
    rpmssl ssl = dig->impl;
    int rc = 0;		/* assume failure. */

#if !defined(OPENSSL_NO_ECDSA)
    if ((ssl->ecdsakey = EC_KEY_new_by_curve_name(ssl->nid)) != NULL
     && EC_KEY_generate_key(ssl->ecdsakey))
        rc = 1;
#endif

SPEW(!rc, rc, dig);

    return rc;
}

static int rpmsslErrChk(pgpDig dig, const char * msg, int rc, unsigned expected)
{
#ifdef	NOTYET
rpmgc gc = dig->impl;
    /* Was the return code the expected result? */
    rc = (gcry_err_code(gc->err) != expected);
    if (rc)
	fail("%s failed: %s\n", msg, gpg_strerror(gc->err));
/* XXX FIXME: rpmsslStrerror */
#else
    rc = (rc == 0);	/* XXX impedance match 1 -> 0 on success */
#endif
    return rc;	/* XXX 0 on success */
}

static int rpmsslAvailableCipher(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmsslAvailableDigest(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgcAvailable(dig->impl, algo,
    	(gcry_md_test_algo(algo) || algo == PGPHASHALGO_MD5));
#endif
    return rc;
}

static int rpmsslAvailablePubkey(pgpDig dig, int algo)
{
    int rc = 0;	/* assume available */
#ifdef	NOTYET
    rc = rpmgcAvailable(dig->impl, algo, gcry_pk_test_algo(algo));
#endif
    return rc;
}

static int rpmsslVerify(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
pgpDigParams sigp = pgpGetSignature(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);
dig->hash_algoN = _pgpHashAlgo2Name(sigp->hash_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmsslVerifyRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmsslVerifyDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmsslVerifyELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmsslVerifyECDSA(dig);
	break;
    }

SPEW(!rc, rc, dig);

    return rc;
}

static int rpmsslSign(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmsslSignRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmsslSignDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmsslSignELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
	rc = rpmsslSignECDSA(dig);
	break;
    }

SPEW(!rc, rc, dig);

    return rc;
}

static int rpmsslGenerate(pgpDig dig)
{
    int rc = 0;		/* assume failure */
pgpDigParams pubp = pgpGetPubkey(dig);
dig->pubkey_algoN = _pgpPubkeyAlgo2Name(pubp->pubkey_algo);

    switch (pubp->pubkey_algo) {
    default:
	break;
    case PGPPUBKEYALGO_RSA:
	rc = rpmsslGenerateRSA(dig);
	break;
    case PGPPUBKEYALGO_DSA:
	rc = rpmsslGenerateDSA(dig);
	break;
    case PGPPUBKEYALGO_ELGAMAL:
	rc = rpmsslGenerateELG(dig);
	break;
    case PGPPUBKEYALGO_ECDSA:
{ rpmssl ssl = dig->impl;
ssl->nid = curve2nid("nistp256");
}
	rc = rpmsslGenerateECDSA(dig);
	break;
    }

SPEW(!rc, rc, dig);

    return rc;
}
#endif	/* _RPMSSL_INTERNAL */

/*==============================================================*/

#ifdef	NOTYET
static int rpmsslTestOne(pgpDig dig,
		const char * msg, const char *r_in, const char *s_in)
{
    pgpDigParams sigp = pgpGetSignature(dig);
    DIGEST_CTX ctx;
    int rc;
int bingo = 0;

    /* create the key */
    rc = pgpImplGenerateECDSA(dig);
    if (!rc)
	goto exit;
bingo++;

    /* generate the hash */
    ctx = rpmDigestInit(sigp->hash_algo, RPMDIGEST_NONE);
    rc = rpmDigestUpdate(ctx, msg, strlen(msg));

    /* create the signature */
    rc = pgpImplSetECDSA(rpmDigestDup(ctx), dig, sigp);
    rc = pgpImplSignECDSA(dig);
    if (!rc)
	goto exit;
bingo++;

#if defined(_RPMSSL_INTERNAL)
    /* check the {r,s} parameters */
    if (pgpImplVecs == &rpmsslImplVecs) {
	rpmssl ssl = dig->impl;
	if (!rpmsslLoadBN(&ssl->r, r_in, _rpmssl_spew)
	 || BN_cmp(ssl->ecdsasig->r, ssl->r))
	    goto exit;
	if (!rpmsslLoadBN(&ssl->s, s_in, _rpmssl_spew)
	 || BN_cmp(ssl->ecdsasig->s, ssl->s))
	    goto exit;
    }
#endif	/* _RPMSSL_INTERNAL */
bingo++;

    /* verify the signature */
    rc = pgpImplSetECDSA(ctx, dig, sigp);
    ctx = NULL;

    rc = pgpImplVerifyECDSA(dig);
    if (rc != 1)
	goto exit;
bingo++;

exit:

    return bingo;		/* XXX 1 on success */
}
#endif	/* NOTYET */

#if defined(_RPMSSL_INTERNAL)
static int rpmsslRSATests(pgpDig dig)
{
    int ret = -1;	/* assume failure */

#ifdef	DYING
    pgpDigParams sigp = pgpGetSignature(dig);
static const char dots[] = "..........";
    struct ECDSAvec_s * v;

    rpmlog(RPMLOG_NOTICE, "========== X9.62/NIST/NSA ECDSA vectors:\n");

    for (v = ECDSAvecs; v->r != NULL; v++) {
	int rc;

	pgpDigClean(dig);

#if defined(_RPMBC_INTERNAL)
if (pgpImplVecs == &rpmbcImplVecs) {
    bc->nbits = v->nbits;
}
#endif
#if defined(_RPMGC_INTERNAL)
if (pgpImplVecs == &rpmgcImplVecs) {
    rpmgc gc = dig->impl;
    gc->nbits = v->nbits;
}
#endif
#if defined(_RPMLTC_INTERNAL)
if (pgpImplVecs == &rpmltcImplVecs) {
    ltc->nbits = v->nbits;
    rpmltcLoadParams(dig, v->name);
}
#endif
#if defined(_RPMNSS_INTERNAL)
if (pgpImplVecs == &rpmnssImplVecs) {
    nss->nbits = v->nbits;
    rpmnssLoadParams(dig, v->name);
}
#endif
#if defined(_RPMSSL_INTERNAL)
if (pgpImplVecs == &rpmsslImplVecs) {
    rpmssl ssl = dig->impl;
    ssl->nbits = v->nbits;
    ssl->nid = curve2nid(v->name);
}
#endif

sigp->hash_algo = v->dalgo;
dig->hash_algoN = _pgpHashAlgo2Name(v->dalgo);
_ix = 0;
_numbers = &v->d;
_numbers_max = 2;

	rpmlog(RPMLOG_INFO, "%s:\t", v->name);

	rc = rpmsslTestOne(dig, v->msg, v->r, v->s);

	rpmlog(RPMLOG_INFO, "%s %s\n", &dots[strlen(dots)-rc],
			(rc >= 4 ? " ok" : " failed"));
	if (rc < 4)
	    ret = 0;

	pgpImplClean(dig->impl);	/* XXX needed for memleaks */

    }

#else	/* DYING */

#if defined(_RPMSSL_INTERNAL)
    int v;

    for (v = 0; v < 6; v++) {

	rpmsslLoadKEY(dig, v);

	if (!rpmsslTestPKCS(dig))
	    ret = 0;

	if (!rpmsslTestOAEP(dig))
	    ret = 0;

	pgpImplClean(dig->impl);

    }
#endif	/* _RPMSSL_INTERNAL */

#endif	/* DYING */

    if (ret == -1)
	ret = 1;

    return ret;
}
#endif	/* _RPMSSL_INTERNAL */

/*==============================================================*/

static struct poptOption optionsTable[] = {
 { "fips", '\0', POPT_ARG_VAL,		&use_fips, 1,
        N_("test in FIPS 140-2 mode"), NULL },
 { "selftest", '\0', POPT_ARG_VAL,	&selftest_only, 1,
        N_("test in FIPS 140-2 mode"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options:"), NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

#if defined(_RPMBC_INTERNAL)
static pgpDig _rpmbcFini(pgpDig dig)
{
rpmbc bc = (dig ? dig->impl : NULL);

    if (bc) {
    }
dig = pgpDigFree(dig);
    return NULL;
}

static pgpDig _rpmbcInit(void)
{
pgpDig dig;
rpmbc bc;

rpmbcImplVecs._pgpSetRSA = rpmbcSetRSA;
rpmbcImplVecs._pgpSetDSA = rpmbcSetDSA;
rpmbcImplVecs._pgpSetELG = rpmbcSetELG;
rpmbcImplVecs._pgpSetECDSA = rpmbcSetECDSA;

rpmbcImplVecs._pgpErrChk = rpmbcErrChk;
rpmbcImplVecs._pgpAvailableCipher = rpmbcAvailableCipher;
rpmbcImplVecs._pgpAvailableDigest = rpmbcAvailableDigest;
rpmbcImplVecs._pgpAvailablePubkey = rpmbcAvailablePubkey;

rpmbcImplVecs._pgpVerify = rpmbcVerify;
rpmbcImplVecs._pgpSign = rpmbcSign;
rpmbcImplVecs._pgpGenerate = rpmbcGenerate;

    pgpImplVecs = &rpmbcImplVecs;

dig = pgpDigNew(RPMVSF_DEFAULT, 0);
bc = dig->impl;

    return dig;
}
#endif	/* _RPMBC_INTERNAL */

#if defined(_RPMGC_INTERNAL)
static pgpDig _rpmgcFini(pgpDig dig)
{
rpmgc gc = (dig ? dig->impl : NULL);
    if (gc) {
    }
dig = pgpDigFree(dig);
    return NULL;
}

static pgpDig _rpmgcInit(void)
{
pgpDig dig;
rpmgc gc;

rpmgcImplVecs._pgpSetRSA = rpmgcSetRSA;
rpmgcImplVecs._pgpSetDSA = rpmgcSetDSA;
rpmgcImplVecs._pgpSetELG = rpmgcSetELG;
rpmgcImplVecs._pgpSetECDSA = rpmgcSetECDSA;

    pgpImplVecs = &rpmgcImplVecs;

dig = pgpDigNew(RPMVSF_DEFAULT, 0);
gc = dig->impl;

gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);

    return dig;
}
#endif	/* _RPMGC_INTERNAL */

#if defined(_RPMNSS_INTERNAL)
static pgpDig _rpmnssFini(pgpDig dig)
{
rpmnss nss = (dig ? dig->impl : NULL);
    if (nss) {
    }
dig = pgpDigFree(dig);
    return NULL;
}

static pgpDig _rpmnssInit(void)
{
pgpDig dig;
rpmnss nss;

rpmnssImplVecs._pgpSetRSA = rpmnssSetRSA;
rpmnssImplVecs._pgpSetDSA = rpmnssSetDSA;
rpmnssImplVecs._pgpSetELG = rpmnssSetELG;
rpmnssImplVecs._pgpSetECDSA = rpmnssSetECDSA;

rpmnssImplVecs._pgpErrChk = rpmnssErrChk;
rpmnssImplVecs._pgpAvailableCipher = rpmnssAvailableCipher;
rpmnssImplVecs._pgpAvailableDigest = rpmnssAvailableDigest;
rpmnssImplVecs._pgpAvailablePubkey = rpmnssAvailablePubkey;

rpmnssImplVecs._pgpVerify = rpmnssVerify;
rpmnssImplVecs._pgpSign = rpmnssSign;
rpmnssImplVecs._pgpGenerate = rpmnssGenerate;

    pgpImplVecs = &rpmnssImplVecs;

dig = pgpDigNew(RPMVSF_DEFAULT, 0);
nss = dig->impl;

    return dig;
}
#endif	/* _RPMNSS_INTERNAL */

#if defined(_RPMSSL_INTERNAL)
static pgpDig _rpmsslFini(pgpDig dig)
{
rpmssl ssl = (dig ? dig->impl : NULL);
    if (ssl) {
ERR_clear_error();
ERR_remove_state(0);
	ERR_remove_thread_state(NULL);
	ERR_free_strings();

RAND_cleanup();
ENGINE_cleanup();
EVP_cleanup();
OBJ_cleanup();
X509V3_EXT_cleanup();

CONF_modules_finish();
CONF_modules_free();
CONF_modules_unload(1);

	CRYPTO_cleanup_all_ex_data();

	if (ssl->out) {
	    CRYPTO_mem_leaks(ssl->out);
	    BIO_free(ssl->out);
	}
	ssl->out = NULL;

    }
dig = pgpDigFree(dig);
    return NULL;
}

static pgpDig _rpmsslInit(void)
{
pgpDig dig;
rpmssl ssl;

rpmsslImplVecs._pgpSetRSA = rpmsslSetRSA;
rpmsslImplVecs._pgpSetDSA = rpmsslSetDSA;
rpmsslImplVecs._pgpSetELG = rpmsslSetELG;
rpmsslImplVecs._pgpSetECDSA = rpmsslSetECDSA;

rpmsslImplVecs._pgpErrChk = rpmsslErrChk;
rpmsslImplVecs._pgpAvailableCipher = rpmsslAvailableCipher;
rpmsslImplVecs._pgpAvailableDigest = rpmsslAvailableDigest;
rpmsslImplVecs._pgpAvailablePubkey = rpmsslAvailablePubkey;

rpmsslImplVecs._pgpVerify = rpmsslVerify;
rpmsslImplVecs._pgpSign = rpmsslSign;
rpmsslImplVecs._pgpGenerate = rpmsslGenerate;

    pgpImplVecs = &rpmsslImplVecs;

    /* enable memory leak checking unless explicitly disabled */
    if (!((getenv("OPENSSL_DEBUG_MEMORY") != NULL) &&
	  (0 == strcmp(getenv("OPENSSL_DEBUG_MEMORY"), "off")))) {
	    CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
    } else {
	/* OPENSSL_DEBUG_MEMORY=off */
	CRYPTO_set_mem_debug_functions(0, 0, 0, 0, 0);
    }
    CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

    /* initialize the prng */
    {   static const char rnd_seed[] =
	      "string to make the random number generator think it has entropy";
	RAND_seed(rnd_seed, sizeof(rnd_seed));
    }

dig = pgpDigNew(RPMVSF_DEFAULT, 0);
ssl = dig->impl;
ssl->out = BIO_new_fp(stdout, BIO_NOCLOSE);

    return dig;
}
#endif	/* _RPMSSL_INTERNAL */

int main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, optionsTable);
pgpDig dig;
    int ec = 1;	/* assume failure */
int rc;

#if defined(_RPMBC_INTERNAL)
dig = _rpmbcInit();
    rc = 1;	/* assume success */
    if (pgpBasicTests(dig))
	rc = 0;
dig = _rpmbcFini(dig);
    if (rc <= 0)
	goto exit;
#endif	/* _RPMBC_INTERNAL */

#if defined(_RPMGC_INTERNAL)
dig = _rpmgcInit();
    rc = 1;	/* assume success */
    if (pgpBasicTests(dig))
	rc = 0;
dig = _rpmgcFini(dig);
    if (rc <= 0)
	goto exit;
#endif	/* _RPMGC_INTERNAL */

#if defined(_RPMLTC_INTERNAL)
dig = _rpmltcInit();
    rc = 1;	/* assume success */
    if (pgpBasicTests(dig))
	rc = 0;
dig = _rpmltcFini(dig);
    if (rc <= 0)
	goto exit;
#endif	/* _RPMNSS_INTERNAL */

#if defined(_RPMNSS_INTERNAL)
dig = _rpmnssInit();
    rc = 1;	/* assume success */
    if (pgpBasicTests(dig))
	rc = 0;
dig = _rpmnssFini(dig);
    if (rc <= 0)
	goto exit;
#endif	/* _RPMNSS_INTERNAL */

#if defined(_RPMSSL_INTERNAL)
dig = _rpmsslInit();
    rc = 1;	/* assume success */
    if (rpmsslRSATests(dig) <= 0)
	rc = 0;
    if (pgpBasicTests(dig))
	rc = 0;
dig = _rpmsslFini(dig);
    if (rc <= 0)
	goto exit;
#endif

    ec = 0;

exit:
    rpmlog(RPMLOG_INFO, "exit code: %s(%d)\n", (ec ? "failed" : "passed"), ec);

    con = rpmioFini(con);

    return ec;
}
