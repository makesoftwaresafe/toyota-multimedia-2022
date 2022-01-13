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

/*!\file Date.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _CLASS_DATE_H
#define _CLASS_DATE_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 */
		class BEECRYPTCXXAPI Date : public Object, public Cloneable, public Comparable<Date>
		{
		private:
			jlong _time;

		public:
			Date() throw ();
			Date(jlong) throw ();
			virtual ~Date() {}

			virtual Date* clone() const throw ();

			virtual bool equals(const Object* obj) const throw ();
			bool equals(const Date& d) const throw ();

			virtual jint compareTo(const Date& anotherDate) const throw ();
			virtual jint hashCode() const throw ();
			virtual String toString() const throw ();

			bool after(const Date&) const throw ();
			bool before(const Date&) const throw ();

			jlong getTime() const throw ();
			void setTime(jlong) throw ();
		};
	}
}

#endif

#endif
