/*
 * Copyright (c) 2000, 2001, 2002, 2005 X-Way Rights BV
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

/*!\file dhies.h
 * \brief DHIES (formerly known as DHAES or DHES) encryption scheme.
 *
 * This code implements the encryption scheme from the paper:
 *
 * "DHIES: An Encryption Scheme Based on the Diffie-Hellman Problem"
 * Michel Abdalla, Mihir Bellare, Phillip Rogaway
 * September 18, 2001
 *
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup DL_m DL_dh_m
 */

#ifndef _DHIES_H
#define _DHIES_H
 
#include "beecrypt/beecrypt.h"
#include "beecrypt/dldp.h"

#ifdef __cplusplus
struct BEECRYPTAPI dhies_pParameters
#else
struct _dhies_pParameters
#endif
{
	const dldp_p*				param;
	const hashFunction*			hash;
	const blockCipher*			cipher;
	const keyedHashFunction*	mac;
	size_t						cipherkeybits;
	size_t						mackeybits;
};

#ifndef __cplusplus
typedef struct _dhies_pParameters dhies_pParameters;
#endif

#ifdef __cplusplus
struct BEECRYPTAPI dhies_pContext
#else
struct _dhies_pContext
#endif
{
	dldp_p						param;
	mpnumber					pub;
	mpnumber					pri;
	hashFunctionContext			hash;
	blockCipherContext			cipher;
	keyedHashFunctionContext	mac;
	size_t						cipherkeybits;
	size_t						mackeybits;
};

#ifndef __cplusplus
typedef struct _dhies_pContext dhies_pContext;
#endif

#ifdef __cplusplus
extern "C" {
#endif

BEECRYPTAPI
int dhies_pUsable(const dhies_pParameters*);

BEECRYPTAPI
int dhies_pContextInit       (dhies_pContext*, const dhies_pParameters*);
BEECRYPTAPI
int dhies_pContextInitDecrypt(dhies_pContext*, const dhies_pParameters*, const mpnumber*);
BEECRYPTAPI
int dhies_pContextInitEncrypt(dhies_pContext*, const dhies_pParameters*, const mpnumber*);
BEECRYPTAPI
int dhies_pContextFree       (dhies_pContext*);

BEECRYPTAPI
memchunk* dhies_pContextEncrypt(dhies_pContext*,       mpnumber*,       mpnumber*, const memchunk*, randomGeneratorContext*);
BEECRYPTAPI
memchunk* dhies_pContextDecrypt(dhies_pContext*, const mpnumber*, const mpnumber*, const memchunk*);

#ifdef __cplusplus
}
#endif

#endif
