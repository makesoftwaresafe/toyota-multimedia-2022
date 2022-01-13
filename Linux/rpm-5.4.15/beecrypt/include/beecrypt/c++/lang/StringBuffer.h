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

/*!\file StringBuffer.h
 * \ingroup CXX_LANG_m
 */

#ifndef _CLASS_BEE_LANG_STRINGBUFFER_H
#define _CLASS_BEE_LANG_STRINGBUFFER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Appendable.h"
using beecrypt::lang::Appendable;
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

namespace beecrypt {
	namespace lang {
		/*!\ingroup CXX_LANG_m
		 */
		class BEECRYPTCXXAPI StringBuffer : public beecrypt::lang::Object, public virtual beecrypt::lang::Appendable, public virtual beecrypt::lang::CharSequence
		{
			friend class String;

		private:
			array<jchar> _buffer;
			jint _used;

			void core_ensureCapacity(int minimum);

		public:
			StringBuffer();
			StringBuffer(const char*);
			StringBuffer(const String&);
			virtual ~StringBuffer() {}

			virtual Appendable& append(jchar c);
			virtual Appendable& append(const CharSequence& cseq);

			StringBuffer& append(bool b);
			StringBuffer& append(char c);
			StringBuffer& append(const char*);
			StringBuffer& append(const String& s);
			StringBuffer& append(const StringBuffer& s);
			StringBuffer& append(const Object* obj);

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
