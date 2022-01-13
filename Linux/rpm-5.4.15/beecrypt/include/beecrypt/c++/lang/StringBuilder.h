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

/*!\file StringBuilder.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_STRINGBUILDER_H
#define _CLASS_BEE_LANG_STRINGBUILDER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Appendable.h"
using beecrypt::lang::Appendable;
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI StringBuilder : public beecrypt::lang::Object, public virtual beecrypt::lang::Appendable, public virtual beecrypt::lang::CharSequence
		{
			friend class String;

		private:
			array<jchar> _buffer;
			int _used;

		public:
			StringBuilder();
			StringBuilder(const char*);
			StringBuilder(const String&);
			virtual ~StringBuilder() {}

			virtual Appendable& append(jchar c);
			virtual Appendable& append(const CharSequence& cseq);

			StringBuilder& append(bool b);
			StringBuilder& append(char c);
			StringBuilder& append(jint i);
			StringBuilder& append(jlong l);
			StringBuilder& append(const char*);
			StringBuilder& append(const String& s);
			StringBuilder& append(const StringBuilder& s);
			StringBuilder& append(const Object* obj);

			StringBuilder& reverse();

			virtual jchar charAt(jint index) const throw (IndexOutOfBoundsException);
			void ensureCapacity(jint minimum);
			virtual jint length() const throw ();
			virtual CharSequence* subSequence(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException);
			virtual String toString() const throw ();
		};
	}
}

#endif

#endif
