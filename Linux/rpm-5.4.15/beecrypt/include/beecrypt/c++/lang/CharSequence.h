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

/*!\file CharSequence.h
 * \ingroup CXX_LANG_m
 */

#ifndef _INTERFACE_BEE_LANG_CHARSEQUENCE_H
#define _INTERFACE_BEE_LANG_CHARSEQUENCE_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/IndexOutOfBoundsException.h"
using beecrypt::lang::IndexOutOfBoundsException;

namespace beecrypt {
	namespace lang {
		class BEECRYPTCXXAPI CharSequence
		{
		public:
			virtual ~CharSequence() {}

			virtual jchar charAt(jint index) const throw (IndexOutOfBoundsException) = 0;
			virtual jint length() const throw () = 0;
			virtual CharSequence* subSequence(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException) = 0;
			virtual String toString() const throw () = 0;
		};
	}
}

#endif

#endif
