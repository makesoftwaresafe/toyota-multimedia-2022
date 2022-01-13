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

/*!\file hmacsha224.c
 * \brief HMAC-SHA-224 message digest algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HMAC_m HMAC_sha224_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacsha224.h"

/*!\addtogroup HMAC_sha224_m
 * \{
 */

const keyedHashFunction hmacsha224 = {
	"HMAC-SHA-224",
	sizeof(hmacsha224Param),
	64,
	28,
	64,
	512,
	32,
	(keyedHashFunctionSetup) hmacsha224Setup,
	(keyedHashFunctionReset) hmacsha224Reset,
	(keyedHashFunctionUpdate) hmacsha224Update,
	(keyedHashFunctionDigest) hmacsha224Digest
};

int hmacsha224Setup (hmacsha224Param* sp, const byte* key, size_t keybits)
{
	return hmacSetup(sp->kxi, sp->kxo, &sha224, &sp->sparam, key, keybits);
}

int hmacsha224Reset (hmacsha224Param* sp)
{
	return hmacReset(sp->kxi, &sha224, &sp->sparam);
}

int hmacsha224Update(hmacsha224Param* sp, const byte* data, size_t size)
{
	return hmacUpdate(&sha224, &sp->sparam, data, size);
}

int hmacsha224Digest(hmacsha224Param* sp, byte* data)
{
	return hmacDigest(sp->kxo, &sha224, &sp->sparam, data);
}

/*!\}
 */
