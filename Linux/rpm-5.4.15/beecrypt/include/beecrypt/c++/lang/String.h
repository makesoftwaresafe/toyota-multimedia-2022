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

/*!\file String.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_STRING_H
#define _CLASS_BEE_LANG_STRING_H

#include "beecrypt/api.h"

#ifdef __cplusplus

#include "beecrypt/c++/lang/CharSequence.h"
using beecrypt::lang::CharSequence;
#include "beecrypt/c++/lang/Comparable.h"
using beecrypt::lang::Comparable;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/array.h"
using beecrypt::array;
using beecrypt::bytearray;

#include <iostream>
#include <unicode/unistr.h>

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 * \brief This class represents Unicode character strings.
		 *
		 */
		class BEECRYPTCXXAPI String : public beecrypt::lang::Object, public virtual beecrypt::lang::CharSequence, public virtual beecrypt::lang::Comparable<String>
		{
			friend class Character;
			friend class StringBuffer;
			friend class StringBuilder;

		private:
			array<jchar> _value;

			String(array<jchar>&);
			String(const array<jchar>&, const array<jchar>&);

		public:
			static String valueOf(bool b);
			static String valueOf(jchar c);
			static String valueOf(jint i);
			static String valueOf(jlong l);

		public:
			String();
			String(char);
			String(jchar);
			String(const char*);
			String(const jchar*, jint offset, jint length);
			String(const bytearray&);
			String(const array<jchar>&);
			String(const String& copy);
			String(const UnicodeString& copy);
			virtual ~String() {}

			String& operator=(const String& copy);
			String& operator=(const UnicodeString& copy);

			virtual jchar charAt(jint index) const throw (IndexOutOfBoundsException);
			virtual jint compareTo(const String& str) const throw ();
			jint compareToIgnoreCase(const String& str) const throw ();
			String concat(const String& str) const throw ();
			bool contains(const CharSequence& seq) const throw ();
			bool contentEquals(const CharSequence& seq) const throw ();
			bool endsWith(const String& suffix) const throw ();
			virtual bool equals(const Object* obj) const throw ();
			bool equals(const String& str) const throw ();
			bool equalsIgnoreCase(const String& str) const throw ();
			virtual jint hashCode() const throw ();
			jint indexOf(jint ch, jint fromIndex = 0) const throw ();
			jint indexOf(const String& str, jint fromIndex = 0) const throw ();
			virtual jint length() const throw ();
			bool startsWith(const String& prefix, jint offset = 0) const throw ();
			virtual CharSequence* subSequence(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException);
			String substring(jint beginIndex) const throw (IndexOutOfBoundsException);
			String substring(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException);
			String toLowerCase() const throw ();
			String toUpperCase() const throw ();
			const array<jchar>& toCharArray() const throw ();
			virtual String toString() const throw ();
			UnicodeString toUnicodeString() const throw ();
		};

		BEECRYPTCXXAPI
		beecrypt::lang::String operator+(const beecrypt::lang::String& s1, const beecrypt::lang::String& s2);

		BEECRYPTCXXAPI
		bool operator<(const beecrypt::lang::String& s1, const beecrypt::lang::String& s2);

		BEECRYPTCXXAPI
		std::ostream& operator<<(std::ostream& stream, const beecrypt::lang::String& str);
		BEECRYPTCXXAPI
		std::ostream& operator<<(std::ostream& stream, const beecrypt::lang::String* str);
	}
}

#endif

#endif
