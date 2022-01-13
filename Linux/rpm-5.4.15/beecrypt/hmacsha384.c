/*
 * Copyright (c) 2004 X-Way Rights BV
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

/*!\file hmacsha384.c
 * \brief HMAC-SHA-384 message digest algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HMAC_m HMAC_sha384_m
 */

#define BEECRYPT_DLL_EXPORT

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/hmacsha384.h"

/*!\addtogroup HMAC_sha384_m
 * \{
 */

const keyedHashFunction hmacsha384 = {
	"HMAC-SHA-384",
	sizeof(hmacsha384Param),
	128,
	48,
	64,
	512,
	32,
	(keyedHashFunctionSetup) hmacsha384Setup,
	(keyedHashFunctionReset) hmacsha384Reset,
	(keyedHashFunctionUpdate) hmacsha384Update,
	(keyedHashFunctionDigest) hmacsha384Digest
};

int hmacsha384Setup (hmacsha384Param* sp, const byte* key, size_t keybits)
{
	return hmacSetup(sp->kxi, sp->kxo, &sha384, &sp->sparam, key, keybits);
}

int hmacsha384Reset (hmacsha384Param* sp)
{
	return hmacReset(sp->kxi, &sha384, &sp->sparam);
}

int hmacsha384Update(hmacsha384Param* sp, const byte* data, size_t size)
{
	return hmacUpdate(&sha384, &sp->sparam, data, size);
}

int hmacsha384Digest(hmacsha384Param* sp, byte* data)
{
	return hmacDigest(sp->kxo, &sha384, &sp->sparam, data);
}

/*!\}
 */
