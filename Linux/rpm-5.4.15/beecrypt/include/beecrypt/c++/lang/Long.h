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

/*!\file Long.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_LONG_H
#define _CLASS_BEE_LANG_LONG_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/lang/Number.h"
using beecrypt::lang::Number;
#include "beecrypt/c++/lang/NumberFormatException.h"
using beecrypt::lang::NumberFormatException;

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI Long : public beecrypt::lang::Number, public virtual beecrypt::lang::Comparable<Long>
		{
		private:
			jlong _val;

		public:
			static const jlong MIN_VALUE;
			static const jlong MAX_VALUE;

			static String toHexString(jlong l) throw ();
			static String toOctalString(jlong l) throw ();
			static String toString(jlong l) throw ();

			static jlong parseLong(const String& s) throw (NumberFormatException);

		public:
			Long(jlong value) throw ();
			Long(const String& s) throw (NumberFormatException);
			virtual ~Long() {}

			virtual jbyte byteValue() const throw ();
			virtual jint compareTo(const Long& anotherLong) const throw ();
			virtual jint hashCode() const throw ();
			virtual jint intValue() const throw ();
			virtual jlong longValue() const throw ();
			virtual jshort shortValue() const throw ();
			virtual String toString() const throw ();
		};
	}
}

#endif

#endif
