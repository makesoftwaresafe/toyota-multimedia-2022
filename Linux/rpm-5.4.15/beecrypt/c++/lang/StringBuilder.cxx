/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/lang/StringBuilder.h"
#include "beecrypt/c++/lang/Character.h"
#include "beecrypt/c++/lang/Integer.h"
#include "beecrypt/c++/lang/Long.h"

using namespace beecrypt::lang;

#include <unicode/ustring.h>

StringBuilder::StringBuilder() : _buffer(16)
{
	_used = 0;
}

StringBuilder::StringBuilder(const char* s) : _buffer(16 + strlen(s))
{
	u_charsToUChars(s, _buffer.data(), _used = strlen(s));
}

StringBuilder::StringBuilder(const String& s) : _buffer(16 + s._value.size())
{
	memcpy(_buffer.data(), s._value.data(), (_used = s._value.size()) * sizeof(jchar));
}

StringBuilder& StringBuilder::append(bool b)
{
	return append(b ? "true" : "false");
}

StringBuilder& StringBuilder::append(char c)
{
	ensureCapacity(_used+1);

	u_charsToUChars(&c, _buffer.data() + _used++, 1);

	return *this;
}

Appendable& StringBuilder::append(jchar c)
{
	ensureCapacity(_used+1);

	_buffer[_used++] = c;

	return *this;
}

Appendable& StringBuilder::append(const CharSequence& c)
{
	jint need = c.length();

	ensureCapacity(_used + need);

	for (jint i = 0; i < need; i++)
		_buffer[_used++] = c.charAt(i);

	return *this;
}

StringBuilder& StringBuilder::append(jint i)
{
	return append(Integer::toString(i));
}

StringBuilder& StringBuilder::append(jlong l)
{
	return append(Long::toString(l));
}

StringBuilder& StringBuilder::append(const char* s)
{
	jint need = strlen(s);

	ensureCapacity(_used + need);

	u_charsToUChars(s, _buffer.data() + _used, need);

	_used += need;

	return *this;
}

StringBuilder& StringBuilder::append(const String& s)
{
	jint need = s._value.size();

	ensureCapacity(_used + need);

	memcpy(_buffer.data() + _used, s._value.data(), need * sizeof(jchar));

	_used += need;

	return *this;
}

StringBuilder& StringBuilder::append(const StringBuilder& s)
{
	jint need = s._used;

	ensureCapacity(_used + need);

	memcpy(_buffer.data() + _used, s._buffer.data(), need * sizeof(jchar));

	_used += need;

	return *this;
}

StringBuilder& StringBuilder::append(const Object* obj)
{
	if (obj)
		return append(obj->toString());
	else
		return append("(null)");
}

StringBuilder& StringBuilder::reverse()
{
	/*!\todo needed for BigInteger::toString() functions
	 */
	bool hasSurrogate = false;
	jint n = _used - 1;

	for (jint i = (n-1) >> 1; i >= 0; --i)
	{
		jchar c1 = _buffer[i];
		jchar c2 = _buffer[n-i];
		if (!hasSurrogate)
			hasSurrogate = (c1 >= Character::MIN_SURROGATE && c1 <= Character::MAX_SURROGATE) ||
				(c2 >= Character::MIN_SURROGATE && c2 <= Character::MAX_SURROGATE);
		_buffer[i] = c2;
		_buffer[n-i] = c1;
	}
	if (hasSurrogate)
	{
		for (jint i = 0; i < _used-1; i++)
		{
			jchar c1 = _buffer[i];
			if (Character::isLowSurrogate(c1))
			{
				jchar c2 = _buffer[i+1];
				if (Character::isHighSurrogate(c2))
				{
					_buffer[i++] = c2;
					_buffer[i] = c1;
				}
			}
		}
	}
	return *this;
}

void StringBuilder::ensureCapacity(jint minimum)
{
	if (minimum > _buffer.size())
	{
		jint newcapacity = _buffer.size() * 2 + 2;

		if (minimum > newcapacity)
			newcapacity = minimum;

		_buffer.resize(newcapacity);
	}
}

jchar StringBuilder::charAt(jint index) const throw (IndexOutOfBoundsException)
{
	if (index >= _used)
		throw IndexOutOfBoundsException();

	return _buffer[index];
}

jint StringBuilder::length() const throw ()
{
	return _used;
}

CharSequence* StringBuilder::subSequence(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException)
{
	if (beginIndex < 0 || endIndex < 0)
		throw IndexOutOfBoundsException();

	if (beginIndex > endIndex || endIndex > _used)
		throw IndexOutOfBoundsException();

	return new String(_buffer.data(), beginIndex, endIndex - beginIndex);
}

String StringBuilder::toString() const throw ()
{
	return String(_buffer.data(), 0, _used);
}
