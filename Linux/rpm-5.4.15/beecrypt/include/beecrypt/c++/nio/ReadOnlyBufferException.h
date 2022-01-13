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

/*!\file ReadOnlyBufferException.h
 * \ingroup CXX_NIO_m
 */

#ifndef _CLASS_BEE_NIO_READONLYBUFFEREXCEPTION_H
#define _CLASS_BEE_NIO_READONLYBUFFEREXCEPTION_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/UnsupportedOperationException.h"
using beecrypt::lang::String;

namespace beecrypt {
	namespace nio {
		 /* \ingroup CXX_NIO_m
		 */
		class ReadOnlyBufferException : public UnsupportedOperationException
		{
		public:
			inline ReadOnlyBufferException()
			{
			}
			inline ReadOnlyBufferException(const char* message) : UnsupportedOperationException(message)
			{
			}
			inline ReadOnlyBufferException(const String* message) : UnsupportedOperationException(message)
			{
			}
			inline ~ReadOnlyBufferException()
			{
			}
		};
	}
}

#endif

#endif
