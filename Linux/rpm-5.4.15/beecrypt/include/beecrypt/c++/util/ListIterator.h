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

/*!\file ListIterator.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _INTERFACE_BEE_UTIL_LISTITERATOR_H
#define _INTERFACE_BEE_UTIL_LISTITERATOR_H

#ifdef __cplusplus

#include "beecrypt/c++/util/Iterator.h"
using beecrypt::util::Iterator;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 */
		template<class E> class ListIterator : public virtual Iterator<E>
		{
		public:
			virtual void add(E* e) = 0;
			virtual bool hasNext() throw () = 0;
			virtual bool hasPrevious() throw () = 0;
			virtual E* next() throw (NoSuchElementException) = 0;
			virtual int nextIndex() throw () = 0;
			virtual E* previous() throw (NoSuchElementException) = 0;
			virtual int previousIndex() throw () = 0;
			virtual void remove() = 0;
			virtual void set(E* e) = 0;
		};
	}
}

#endif

#endif
