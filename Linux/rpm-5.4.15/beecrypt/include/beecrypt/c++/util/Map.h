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

/*!\file Map.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _INTERFACE_BEE_UTIL_MAP_H
#define _INTERFACE_BEE_UTIL_MAP_H

#ifdef __cplusplus

#include "beecrypt/c++/util/Set.h"
using beecrypt::util::Set;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 */
		template<class K, class V> class Map
		{
		public:
			class Entry
			{
			public:
				virtual ~Entry() {}

				virtual bool equals(const Entry* e) const throw () = 0;
				virtual bool equals(const Object* obj) const throw () = 0;
				virtual K* getKey() const = 0;
				virtual V* getValue() const = 0;
				virtual jint hashCode() const throw () = 0;
				virtual V* setValue(V*) = 0;
			};

		public:
			virtual ~Map() {}

			virtual void clear() = 0;
			virtual bool containsKey(const K* key) const = 0;
			virtual bool containsValue(const V* value) const = 0;
			virtual Set<class Entry>& entrySet() = 0;
			virtual const Set<class Entry>& entrySet() const = 0;
			virtual Set<K>& keySet() = 0;
			virtual const Set<K>& keySet() const = 0;
			virtual bool equals(const Object* obj) const throw () = 0;
			virtual V* get(const K* key) const = 0;
			virtual jint hashCode() const throw () = 0;
			virtual bool isEmpty() const = 0;
			virtual V* put(K* key, V* value) = 0;
			virtual void putAll(const Map<K,V>& m) = 0;
			virtual V* remove(const K* key) = 0;
			virtual jint size() const throw () = 0;
			virtual Collection<V>& values() = 0;
			virtual const Collection<V>& values() const = 0;
		};
	}
}

#endif

#endif
