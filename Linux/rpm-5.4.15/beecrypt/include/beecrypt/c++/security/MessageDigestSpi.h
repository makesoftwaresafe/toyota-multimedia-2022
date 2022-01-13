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
 */

/*!\file MessageDigestSpi.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_MESSAGEDIGESTSPI_H
#define _CLASS_BEE_SECURITY_MESSAGEDIGESTSPI_H

#include "beecrypt/api.h"

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::bytearray;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/ShortBufferException.h"
using beecrypt::security::ShortBufferException;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI MessageDigestSpi : public Object
		{
			friend class MessageDigest;

		protected:
			virtual const bytearray& engineDigest() = 0;
			virtual int engineDigest(byte* data, int offset, int length) throw (ShortBufferException) = 0;
			virtual int engineGetDigestLength() = 0;
			virtual void engineReset() = 0;
			virtual void engineUpdate(byte b) = 0;
			virtual void engineUpdate(const byte* data, int offset, int length) = 0;

		public:
			virtual ~MessageDigestSpi() {}

			virtual MessageDigestSpi* clone() const throw (CloneNotSupportedException) = 0;
		};
	}
}

#endif

#endif
