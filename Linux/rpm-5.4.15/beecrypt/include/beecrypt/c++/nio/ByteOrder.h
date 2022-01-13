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

/*!\file ByteOrder.h
 * \ingroup CXX_NIO_m
 */

#ifndef _CLASS_BEE_NIO_BYTEORDER_H
#define _CLASS_BEE_NIO_BYTEORDER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

#ifdef BIG_ENDIAN
# undef BIG_ENDIAN
#endif
#ifdef LITTLE_ENDIAN
# undef LITTLE_ENDIAN
#endif

namespace beecrypt {
	namespace nio {
		class BEECRYPTCXXAPI ByteOrder : public Object
		{
		private:
			String _name;

			ByteOrder(const String& name);

		public:
			static const ByteOrder BIG_ENDIAN;
			static const ByteOrder LITTLE_ENDIAN;

			static const ByteOrder& nativeOrder();

		public:
			virtual ~ByteOrder() {}

			virtual String toString() const throw ();
		};
	}
}

#endif

#endif
