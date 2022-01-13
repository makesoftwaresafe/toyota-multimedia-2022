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

/*!\file List.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _INTERFACE_BEE_UTIL_LIST_H
#define _INTERFACE_BEE_UTIL_LIST_H

#ifdef __cplusplus

#include "beecrypt/c++/util/Collection.h"
using beecrypt::util::Collection;
#include "beecrypt/c++/util/ListIterator.h"
using beecrypt::util::ListIterator;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 */
		template <class E> class List : public virtual Collection<E>
		{
		public:
			virtual ~List() {}

			virtual void add(jint index, E* e) = 0;
			virtual bool addAll(jint index, const Collection<E>& c) = 0;
			virtual E* get(jint index) const throw (IndexOutOfBoundsException) = 0;
			virtual jint indexOf(const E* e) const = 0;
			virtual jint lastIndexOf(const E* e) const = 0;
			virtual ListIterator<E>* listIterator(jint index = 0) throw (IndexOutOfBoundsException) = 0;
			virtual ListIterator<E>* listIterator(jint index = 0) const throw (IndexOutOfBoundsException) = 0;
			virtual E* remove(jint index) = 0;
			virtual E* set(jint index, E* e) = 0;
		//	virtual List<E> subList(jint fromIndex, jint toIndex) const = 0;
		};
	}
}

#endif

#endif
