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

/*!\file AbstractMap.h
 * \ingroup CXX_UTIL_m
 */

#ifndef _ABSTRACT_CLASS_BEE_UTIL_ABSTRACTMAP_H
#define _ABSTRACT_CLASS_BEE_UTIL_ABSTRACTMAP_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;
#include "beecrypt/c++/util/Map.h"
using beecrypt::util::Map;
#include "beecrypt/c++/util/AbstractSet.h"
using beecrypt::util::AbstractSet;

namespace beecrypt {
	namespace util {
		/*!\ingroup CXX_UTIL_m
		 */
		template <class K, class V> class AbstractMap : public Object, public virtual Map<K,V>
		{
		private:
			class KeySet : public AbstractSet<K>
			{
			private:
				class Iter : public Object, virtual public Iterator<K>
				{
				private:
					Iterator<class Map<K,V>::Entry>* _it;
		
				public:
					Iter(AbstractMap* m) : _it(m->entrySet().iterator())
					{
					}
					Iter(const AbstractMap* m) : _it(m->entrySet().iterator())
					{
					}
					virtual ~Iter()
					{
						delete _it;
					}

					virtual bool hasNext() throw ()
					{
						return _it->hasNext();
					}
					virtual K* next() throw (NoSuchElementException)
					{
						return _it->next()->getKey();
					}
					virtual void remove()
					{
						_it->remove();
					}
				};

			private:
				AbstractMap* _m;

			public:
				KeySet(AbstractMap* m) : _m(m)
				{
				}
				virtual ~KeySet() {}

				virtual Iterator<K>* iterator()
				{
					return new Iter(_m);
				}
				virtual Iterator<K>* iterator() const
				{
					return new Iter(_m);
				}
				virtual jint size() const throw ()
				{
					return _m->size();
				}
			};

			class Values : public AbstractCollection<V>
			{
			private:
				class Iter : public Object, virtual public Iterator<V>
				{
				private:
					Iterator<class Map<K,V>::Entry>* _it;
		
				public:
					Iter(AbstractMap* m) : _it(m->entrySet().iterator())
					{
					}
					Iter(const AbstractMap* m) : _it(m->entrySet().iterator())
					{
					}
					virtual ~Iter()
					{
						delete _it;
					}

					virtual bool hasNext() throw ()
					{
						return _it->hasNext();
					}
					virtual V* next() throw (NoSuchElementException)
					{
						return _it->next()->getValue();
					}
					virtual void remove()
					{
						_it->remove();
					}
				};

			private:
				AbstractMap* _m;

			public:
				Values(AbstractMap* m) : _m(m)
				{
				}
				virtual ~Values() {}

				virtual Iterator<V>* iterator()
				{
					return new Iter(_m);
				}
				virtual Iterator<V>* iterator() const
				{
					return new Iter(_m);
				}
				virtual jint size() const throw ()
				{
					return _m->size();
				}
			};

		private:
			mutable Set<K>* _keys;
			mutable Collection<V>* _values;
  
		protected:
			AbstractMap()
			{
				_keys = 0;
				_values = 0;
			}

		public:
			virtual ~AbstractMap()
			{
				clear();

				delete _keys;
				delete _values;
			}

			virtual void clear()
			{
				entrySet().clear();
			}
			virtual bool containsKey(const K* key) const
			{
				bool result = false;
				jint pos = size();
				Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
				assert(it != 0);
				if (key)
				{
					while (--pos >= 0)
					{
						class Map<K,V>::Entry* e = it->next();
						if (e->getKey->equals(key))
						{
							result = true;
							break;
						}
					}
				}
				else
				{
					while (--pos >= 0)
						if (!it->next())
						{
							result = true;
							break;
						}
				}
				delete it;
				return result;
			}
			virtual bool containsValue(const V* value) const
			{
				bool result = false;
				jint pos = size();
				Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
				assert(it != 0);
				if (value)
				{
					while (--pos >= 0)
					{
						V* tmp = it->next()->getValue();
						if (tmp && tmp->equals(value))
						{
							result = true;
							break;
						}
					}
				}
				else
				{
					while (--pos >= 0)
						if (!it->next())
						{
							result = true;
							break;
						}
				}
				delete it;
				return false;
			}
			virtual Set<class Map<K,V>::Entry>& entrySet() = 0;
			virtual const Set<class Map<K,V>::Entry>& entrySet() const = 0;
			virtual bool equals(const Object* obj) const throw ()
			{
				if (this == obj)
					return true;

				if (obj)
				{
					const Map<K,V>* map = dynamic_cast<const Map<K,V> >(obj);
					if (map)
					{
						if (map->size() != size())
							return false;

						bool result = true;
						jint pos = size();
						Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
						assert(it != 0);
						while (--pos >= 0)
						{
							class Map<K,V>::Entry* e = it.next();
							K* k = e->getKey();
							V* v = e->getValue();
							if (v)
							{
								if (!v->equals(map->get(k)))
								{
									result = false;
									break;
								}
							}
							else
							{
								if (map->get(k) || !map->containsKey(k))
								{
									result = false;
									break;
								}
							}
						}
						delete it;
						return result;
					}
				}
				return false;
			}
			virtual V* get(const Object* key) const
			{
				V* result = 0;
				jint pos = size();
				Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
				assert(it != 0);
				if (key)
				{
					while (--pos >= 0)
					{
						class Map<K,V>::Entry* e = it->next();
						if (key->equals(e->getKey()))
						{
							result = e->getValue();
							break;
						}
					}
				}
				else
				{
					while (--pos >= 0)
					{
						class Map<K,V>::Entry* e = it->next();
						if (!e->getKey())
						{
							result = e->getValue();
							break;
						}
					}
				}
				delete it;
				return result;
			}
			virtual jint hashCode() const throw ()
			{
				jint pos = size(), result = 0;
				Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
				assert(it != 0);
				while (--pos >= 0)
					result += it->next()->hashCode();
				delete it;
				return result;
			}
			virtual bool isEmpty()
			{
				return entrySet().size() == 0;
			}
			virtual Set<K>& keySet()
			{
				if (!_keys)
					_keys = new KeySet(this);

				return *_keys;
			}
			virtual const Set<K>& keySet() const
			{
				if (!_keys)
					_keys = new KeySet(const_cast<AbstractMap*>(this));

				return *_keys;
			}
			virtual V* put(K* key, V* value)
			{
				throw UnsupportedOperationException();
			}
			virtual void putAll(const Map<K,V>& m)
			{
				jint pos = m.size();
				Iterator<class Map<K,V>::Entry>* mit = m.entrySet().iterator();
				assert(mit != 0);
				while (--pos >= 0)
				{
					class Map<K,V>::Entry* e = mit->next();
					V* tmp = put(e->getKey(), e->getValue());
					collection_rcheck(tmp);
				}
				delete mit;
			}
			virtual V* remove(const K* key)
			{
				V* result = 0;
				jint pos = size();
				Iterator<class Map<K,V>::Entry>* it = entrySet().iterator();
				assert(it != 0);
				if (key)
				{
					while (--pos >= 0)
					{
						class Map<K,V>::Entry* e = it->next();
						if (key->equals(e->getKey()))
						{
							result = e->getValue();
							break;
						}
					}
				}
				else
				{
					while (--pos >= 0)
					{
						class Map<K,V>::Entry* e = it->next();
						if (!e->getKey())
						{
							result = e->getValue();
							break;
						}
					}
				}

				if (result)
					it->remove();

				delete it;

				return result;
			}
			virtual jint size()
			{
				return entrySet().size();
			}
			virtual String toString() const throw ()
			{
				Iterator<class Map<K,V>::Entry>& it = entrySet.iterator();

				StringBuilder buf("{");

				for (jint pos = size(); pos > 0; pos--)
				{
					class Map<K,V>::Entry* e = it.next();
					K* k = e->getKey();
					V* v = e->getValue();

					if (k == this)
						buf.append("(this map)");
					else
						buf.append(k);

					buf.append("=");

					if (v == this)
						buf.append("(this map)");
					else
						buf.append(v);

					if (pos > 1)
						buf.append(", ");
				}

				buf.append("}");

				return buf.toString();
			}
			virtual Collection<V>& values()
			{
				if (!_values)
					_values = new Values(this);

				return *_values;
			}
			virtual const Collection<V>& values() const
			{
				if (!_values)
					_values = new Values(const_cast<AbstractMap*>(this));

				return *_values;
			}
		};
	}
}

#endif

#endif
