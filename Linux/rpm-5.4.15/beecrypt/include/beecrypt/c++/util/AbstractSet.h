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

/*!\file AbstractSet.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _ABSTRACT_CLASS_BEE_UTIL_ABSTRACTSET_H
#define _ABSTRACT_CLASS_BEE_UTIL_ABSTRACTSET_H

#ifdef __cplusplus

#include "beecrypt/c++/util/AbstractCollection.h"
using beecrypt::util::AbstractCollection;
#include "beecrypt/c++/util/Set.h"
using beecrypt::util::Set;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 * \warning See the description of beecrypt::util:Collection for limitations
         *  on template parameter class E.
		 */
		template<class E> class AbstractSet : public AbstractCollection<E>, public virtual Set<E>
		{
		protected:
			AbstractSet() {}

		public:
			virtual bool equals(const Object* obj) const throw ()
			{
				if (this == obj)
					return true;

				if (obj)
				{
					if (!dynamic_cast<const Set<E>*>(obj))
						return false;

					const Collection<E>* c = dynamic_cast<const Collection<E>*>(obj);
					if (c->size() != size())
						return false;

					return containsAll(*c);
				}
				return false;
			}
			virtual jint hashCode() const throw ()
			{
				jint pos = size(), result = 0;
				Iterator<E>* it = iterator();
				assert(it != 0);
				while (--pos >= 0)
				{
					E* e = it->next();
					result += e->hashCode();
				}
				delete it;
				return result;
			}
			virtual Iterator<E>* iterator() = 0;
			virtual Iterator<E>* iterator() const = 0;
			virtual bool removeAll(const Collection<E>& c)
			{
				bool result = false;
				jint pos = size(), cpos = c.size();
				if (pos > cpos)
				{
					Iterator<E>* it = c.iterator();
					assert(it != 0);
					while (--cpos >= 0)
						result |= AbstractCollection<E>::remove(it->next());
					delete it;
				}
				else
				{
					Iterator<E>* it = iterator();
					assert(it != 0);
					while (--pos >= 0)
						if (c.contains(it->next()))
						{
							it->remove();
							result = true;
						}
					delete it;
				}
				return result;
			}
			virtual jint size() const throw () = 0;
		};
	}
}

#endif

#endif
