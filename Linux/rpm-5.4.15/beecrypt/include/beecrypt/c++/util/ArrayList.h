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

/*!\file ArrayList.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _CLASS_BEE_UTIL_ARRAYLIST_H
#define _CLASS_BEE_UTIL_ARRAYLIST_H

#ifdef __cplusplus

#include "beecrypt/c++/util/AbstractList.h"
using beecrypt::util::AbstractList;
#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/lang/IllegalArgumentException.h"
using beecrypt::lang::IllegalArgumentException;
#include "beecrypt/c++/util/RandomAccess.h"
using beecrypt::util::RandomAccess;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 * \warning class E must be a subclass of Object
		 */
		template<class E> class ArrayList : public AbstractList<E>, public virtual RandomAccess, public virtual Cloneable
		{
		private:
			array<E*> _table;
			jint _count;

			void fastInsert(jint index)
			{
				register jint move = _count - index;
				if (move)
					::memmove(_table.data() + index + 1, _table.data() + index, move * sizeof(E*));
				_table[index] = 0;
				_count++;
			}
			void fastRemove(jint index)
			{
				register jint move = _count - index - 1;
				if (move)
					::memmove(_table.data() + index, _table.data() + index + 1, move * sizeof(E*));
				_table[--_count] = 0;
			}

		public:
			ArrayList(jint initialCapacity = 10) :_table(initialCapacity)
			{
				if (initialCapacity < 0)
					throw IllegalArgumentException();

				_count = 0;
			}
			ArrayList(Collection<E>& c) : _table(c.size())
			{
				addAll(c);
			}
			ArrayList(const ArrayList& copy) : _table(copy._table), _count(copy._count)
			{
			}
			virtual ~ArrayList()
			{
				clear();
			}

			virtual bool add(E* e)
			{
				ensureCapacity(_count+1);
				if ((_table[_count++] = e))
					collection_attach(e);
				return true;
			}
			virtual void add(jint index, E* e)
			{
				if (index < 0 || index > _count)
					throw IndexOutOfBoundsException();

				ensureCapacity(_count+1);
				fastInsert(index);
				if ((_table[index] = e))
					collection_attach(e);
			}
			virtual bool addAll(const Collection<E>& c)
			{
				jint clen = c.size();

				if (clen > 0)
				{
					ensureCapacity(_count + clen);
					Iterator<E>* cit = c.iterator();
					assert(cit != 0);
					while (cit->hasNext())
					{
						E* tmp = cit->next();
						if (tmp)
							collection_attach(tmp);
						_table[_count++] = tmp;
					}
					delete cit;
					return true;
				}
				return false;
			}
			virtual Object* clone() const throw ()
			{
				return new ArrayList<E>(*this);
			}
			virtual void clear()
			{
				for (jint i = 0; i < _count; i++)
				{
					E* tmp = _table[i];
					if (tmp)
					{
						collection_remove(tmp);
						_table[i] = 0;
					}
				}
				_count = 0;
				_table.resize(10);
			}
			virtual bool contains(const E* e) const
			{
				return indexOf(e) >= 0;
			}
			virtual void ensureCapacity(jint minimum)
			{
				this->modCount++;

				if (minimum > _table.size())
				{
					jint newcapacity = (_table.size() * 3) / 2 + 2;

					if (minimum > newcapacity)
						newcapacity = minimum;

					_table.resize(newcapacity);
				}
			}
			virtual E* get(jint index) const throw (IndexOutOfBoundsException)
			{
				if (index < 0 || index >= _count)
					throw IndexOutOfBoundsException();

				return _table[index];
			}
			virtual jint indexOf(const E* e) const
			{
				if (e)
				{
					for (jint i = 0; i < _count; i++)
						if (_table[i] && _table[i]->equals(e))
							return i;
				}
				else
				{
					for (jint i = 0; i < _count; i++)
						if (!_table[i])
							return i;
				}
				return -1;
			}
			virtual bool isEmpty() const throw ()
			{
				return _count == 0;
			}
			virtual bool remove(const E* e)
			{
				for (jint i = 0; i < _count; i++)
				{
					E* tmp = _table[i];
					if (AbstractCollection<E>::equals(e, tmp))
					{
						fastRemove(i);
						if (tmp)
							collection_remove(tmp);
						return true;
					}
				}
				return false;
			}
			virtual E* remove(jint index) throw (IndexOutOfBoundsException)
			{
				if (index < 0 || index >= _count)
					throw IndexOutOfBoundsException();

				E* tmp = _table[index];
				fastRemove(index);
				if (tmp)
					collection_detach(tmp);

				return tmp;
			}
			virtual jint size() const throw ()
			{
				return _count;
			}
			virtual array<E*> toArray() const
			{
				return array<E*>(_table.data(), _count);
			}
			virtual void trimToSize()
			{
				_table.resize(_count);
			}
		};
	}
}

#endif

#endif
