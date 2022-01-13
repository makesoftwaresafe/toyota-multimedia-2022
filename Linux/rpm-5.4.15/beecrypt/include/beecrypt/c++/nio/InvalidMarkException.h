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

/*!\file InvalidMarkException.h
 * \ingroup CXX_NIO_m
 */

#ifndef _CLASS_BEE_NIO_INVALIDMARKEXCEPTION_H
#define _CLASS_BEE_NIO_INVALIDMARKEXCEPTION_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/IllegalStateException.h"
using beecrypt::lang::IllegalStateException;
using beecrypt::lang::String;

namespace beecrypt {
	namespace nio {
		/*!\ingroup CXX_NIO_m
		 */
		class InvalidMarkException : public IllegalStateException
		{
		public:
			inline InvalidMarkException() {}
			inline InvalidMarkException(const char* message) : IllegalStateException(message) {}
			inline InvalidMarkException(const String& message) : IllegalStateException(message) {}
			inline ~InvalidMarkException() {}
		};
	}
}

#endif

#endif
