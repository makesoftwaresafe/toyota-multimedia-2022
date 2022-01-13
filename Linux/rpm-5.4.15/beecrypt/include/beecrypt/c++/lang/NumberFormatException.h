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

/*!\file NumberFormatException.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_NUMBERFORMATEXCEPTION_H
#define _CLASS_BEE_LANG_NUMBERFORMATEXCEPTION_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/IllegalArgumentException.h"
using beecrypt::lang::IllegalArgumentException;

namespace beecrypt {
	namespace lang {
		/*!\brief Thrown to indicate that the conversion from a String to a
		 *        numeric value due to an inappropriate format.
		 * \ingroup CXX_LANG_m
		 */
		class NumberFormatException : public IllegalArgumentException
		{
		public:
			inline NumberFormatException() {}
			inline NumberFormatException(const char* message) : IllegalArgumentException(message) {}
			inline NumberFormatException(const String& message) : IllegalArgumentException(message) {}
			inline ~NumberFormatException() {}
		};
	}
}

#endif

#endif
