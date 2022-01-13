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

/*!\file sha2k32.h
 * \brief SHA-256 and SHA-224 shared constants, headers.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_sha256_m HASH_sha224_m
 */

#ifndef _SHA2K32_H
#define _SHA2K32_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BEECRYPTAPI const uint32_t SHA2_32BIT_K[64];

#ifdef __cplusplus
}
#endif

#endif
