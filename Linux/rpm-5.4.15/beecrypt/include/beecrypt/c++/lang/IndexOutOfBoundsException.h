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

/*!\file IndexOutOfBoundsException.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_INDEXOUTOFBOUNDSEXCEPTION_H
#define _CLASS_BEE_LANG_INDEXOUTOFBOUNDSEXCEPTION_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/RuntimeException.h"
using beecrypt::lang::RuntimeException;

namespace beecrypt {
	namespace lang {
		/* \ingroup CXX_LANG_m
		 */
		class IndexOutOfBoundsException : public RuntimeException
		{
		public:
			inline IndexOutOfBoundsException() {}
			inline IndexOutOfBoundsException(const char* message) : RuntimeException(message) {}
			inline IndexOutOfBoundsException(const String& message) : RuntimeException(message) {}
			inline ~IndexOutOfBoundsException() {}
		};
	}
}

#endif

#endif
