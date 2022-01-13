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

/*!\file pkcs1.h
 * \brief PKCS#1 utility routines
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup PKCS1_m
 */

#ifndef _PKCS1_H
#define _PKCS1_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!\brief This function computes the digest, and encodes it it according to PKCS#1 for signing
 * \param ctxt The hash function context
 * \param emdata
 * \param emsize
 */
BEECRYPTAPI
int pkcs1_emsa_encode_digest(hashFunctionContext* ctxt, byte* emdata, size_t emsize);

#ifdef __cplusplus
}
#endif

#endif
