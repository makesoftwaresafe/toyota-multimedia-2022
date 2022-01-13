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

/*!\file Character.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_CHARACTER_H
#define _CLASS_BEE_LANG_CHARACTER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI Character : public beecrypt::lang::Object, public virtual beecrypt::lang::Comparable<Character>
		{
		private:
			jchar _val;

		public:
			static const jchar MIN_VALUE;
			static const jchar MAX_VALUE;

			static const jchar MIN_HIGH_SURROGATE;
			static const jchar MAX_HIGH_SURROGATE;
			static const jchar MIN_LOW_SURROGATE;
			static const jchar MAX_LOW_SURROGATE;
			static const jchar MIN_SURROGATE;
			static const jchar MAX_SURROGATE;

			static const jint MIN_SUPPLEMENTARY_CODE_POINT;
			static const jint MIN_CODE_POINT;
			static const jint MAX_CODE_POINT;

			static const jint MIN_RADIX;
			static const jint MAX_RADIX;

			static String toString(jchar c) throw ();

			static bool isHighSurrogate(jchar ch) throw ();
			static bool isLowSurrogate(jchar ch) throw ();

		public:
			Character(jchar value) throw ();
			virtual ~Character() {}

			virtual jint hashCode() const throw ();
			virtual jint compareTo(const Character& anotherCharacter) const throw ();
			virtual String toString() const throw ();
		};
	}
}

#endif

#endif
