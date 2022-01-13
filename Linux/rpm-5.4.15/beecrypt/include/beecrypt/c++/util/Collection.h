/*
 * Copyright (c) 2005 X-Way Rights BV
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

/*!\file Collection.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _INTERFACE_BEE_UTIL_COLLECTION_H
#define _INTERFACE_BEE_UTIL_COLLECTION_H

#ifdef __cplusplus

#include "beecrypt/c++/util/Iterable.h"
using beecrypt::util::Iterable;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
 		 *
		 * \note In deviation from the Java API, the add(), contains() and remove()
		 *  methods take the template parameter class E as parameter rather than
		 *  Object.
		 *
		 *  There is a very important reason for this: in Java, even an interface is
		 *  an Object, but in this API it isn't. Using Object as parameter would
		 *  mean a great deal of dynamic_cast operations.
		 *
		 *  From the OO point-of-view it makes much more sense that a collection of E
		 *  should only accept elements of class E as parameters.
		 *
		 * \warning If you do not use a parameter that is a subclass of Object (e.g.
		 *  an interface), then you must guarantee that it has a method which matches
		 *  Object::equals().
		 *
		 */
		template<class E> class Collection : public virtual Iterable<E>
		{
		public:
			virtual ~Collection() {}

			virtual bool add(E* e) = 0;
			virtual bool addAll(const Collection<E>& c) = 0;
			virtual void clear() = 0;
			virtual bool contains(const E* e) const = 0;
			virtual bool containsAll(const Collection<E>& c) const = 0;
			virtual bool equals(const Object* obj) const throw () = 0;
			virtual jint hashCode() const throw () = 0;
			virtual bool isEmpty() const throw () = 0;
			virtual Iterator<E>* iterator() = 0;
			virtual Iterator<E>* iterator() const = 0;
			virtual bool remove(const E* e) = 0;
			virtual bool removeAll(const Collection<E>& c) = 0;
			virtual bool retainAll(const Collection<E>& c) = 0;
			virtual jint size() const throw () = 0;
			virtual array<E*> toArray() const = 0;
		};
	}
}

#endif

#endif
