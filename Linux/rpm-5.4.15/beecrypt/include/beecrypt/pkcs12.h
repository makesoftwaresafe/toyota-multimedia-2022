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

/*!\file pkcs12.h
 * \brief PKCS#12 utility routines
 * \ingroup PKCS12_m
 * \author Bob Deblier <bob.deblier@telenet.be>
 */

#ifndef _PKCS12_H
#define _PKCS12_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PKCS12_ID_CIPHER	0x1
#define PKCS12_ID_IV		0x2
#define PKCS12_ID_MAC		0x3

BEECRYPTAPI
int pkcs12_derive_key(const hashFunction* h, byte id, const byte* pdata, size_t psize, const byte* sdata, size_t ssize, size_t iterationcount, byte* ndata, size_t nsize);

#ifdef __cplusplus
}
#endif

#endif
