/*
 * Copyright (c) 2000, 2001, 2002 X-Way Rights BV
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

/*!\file hmacsha512.c
 * \brief HMAC-SHA-512 message digest algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HMAC_m HMAC_sha512_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacsha512.h"

/*!\addtogroup HMAC_sha512_m
 * \{
 */

const keyedHashFunction hmacsha512 = {
	"HMAC-SHA-512",
	sizeof(hmacsha512Param),
	128,
	64,
	64,
	512,
	32,
	(keyedHashFunctionSetup) hmacsha512Setup,
	(keyedHashFunctionReset) hmacsha512Reset,
	(keyedHashFunctionUpdate) hmacsha512Update,
	(keyedHashFunctionDigest) hmacsha512Digest
};

int hmacsha512Setup (hmacsha512Param* sp, const byte* key, size_t keybits)
{
	return hmacSetup(sp->kxi, sp->kxo, &sha512, &sp->sparam, key, keybits);
}

int hmacsha512Reset (hmacsha512Param* sp)
{
	return hmacReset(sp->kxi, &sha512, &sp->sparam);
}

int hmacsha512Update(hmacsha512Param* sp, const byte* data, size_t size)
{
	return hmacUpdate(&sha512, &sp->sparam, data, size);
}

int hmacsha512Digest(hmacsha512Param* sp, byte* data)
{
	return hmacDigest(sp->kxo, &sha512, &sp->sparam, data);
}

/*!\}
 */
