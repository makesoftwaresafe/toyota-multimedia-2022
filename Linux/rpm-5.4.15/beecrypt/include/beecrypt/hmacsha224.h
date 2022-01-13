/*
 * Copyright (c) 2009 Bob Deblier
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

/*!\file hmacsha224.h
 * \brief HMAC-SHA-224 message authentication code, headers.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HMAC_m HMAC_sha224_m
 */

#ifndef _HMACSHA224_H
#define _HMACSHA224_H

#include "beecrypt/hmac.h"
#include "beecrypt/sha224.h"

/*!\ingroup HMAC_sha224_m
 */
typedef struct
{
	sha224Param sparam;
	byte kxi[64];
	byte kxo[64];
} hmacsha224Param;

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const keyedHashFunction hmacsha224;

BEECRYPTAPI
int hmacsha224Setup (hmacsha224Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha224Reset (hmacsha224Param*);
BEECRYPTAPI
int hmacsha224Update(hmacsha224Param*, const byte*, size_t);
BEECRYPTAPI
int hmacsha224Digest(hmacsha224Param*, byte*);

#ifdef __cplusplus
}
#endif

#endif
