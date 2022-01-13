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

/*!\file Integer.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_INTEGER_H
#define _CLASS_BEE_LANG_INTEGER_H

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
		class BEECRYPTCXXAPI Integer : public beecrypt::lang::Number, public virtual beecrypt::lang::Comparable<Integer>
		{
		private:
			jint _val;

		public:
			static const jint MIN_VALUE;
			static const jint MAX_VALUE;

			static String toHexString(jint l) throw ();
			static String toOctalString(jint l) throw ();
			static String toString(jint l) throw ();

			static jint parseInteger(const String& s) throw (NumberFormatException);

		public:
			Integer(jint value) throw ();
			Integer(const String& s) throw (NumberFormatException);
			virtual ~Integer() {}

			virtual jint hashCode() const throw ();
			virtual jbyte byteValue() const throw ();
			virtual jshort shortValue() const throw ();
			virtual jint intValue() const throw ();
			virtual jlong longValue() const throw ();
			virtual jint compareTo(const Integer& anotherInteger) const throw ();
			virtual String toString() const throw ();
		};
	}
}

#endif

#endif
