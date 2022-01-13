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

#include "beecrypt/c++/lang/StringBuffer.h"

using namespace beecrypt::lang;

#include <unicode/ustring.h>

StringBuffer::StringBuffer() : _buffer(16)
{
	_used = 0;
}

StringBuffer::StringBuffer(const char* s) : _buffer(16 + strlen(s))
{
	u_charsToUChars(s, _buffer.data(), _used = strlen(s));
}

StringBuffer::StringBuffer(const String& s) : _buffer(16 + s._value.size())
{
	memcpy(_buffer.data(), s._value.data(), (_used = s._value.size()) * sizeof(jchar));
}

StringBuffer& StringBuffer::append(bool b)
{
	return append(b ? "true" : "false");
}

StringBuffer& StringBuffer::append(char c)
{
	synchronized (this)
	{
		core_ensureCapacity(_used+1);
		u_charsToUChars(&c, _buffer.data() + _used++, 1);
	}
	return *this;
}

Appendable& StringBuffer::append(jchar c)
{
	synchronized (this)
	{
		core_ensureCapacity(_used+1);
		_buffer[_used++] = c;
	}
	return *this;
}

Appendable& StringBuffer::append(const CharSequence& c)
{
	synchronized (this)
	{
		jint need = c.length();

		core_ensureCapacity(_used + need);
		for (jint i = 0; i < need; i++)
			_buffer[_used++] = c.charAt(i);
	}
	return *this;
}

StringBuffer& StringBuffer::append(const char* s)
{
	synchronized (this)
	{
		jint need = strlen(s);

		core_ensureCapacity(_used + need);
		u_charsToUChars(s, _buffer.data() + _used, need);

		_used += need;
	}
	return *this;
}

StringBuffer& StringBuffer::append(const String& s)
{
	synchronized (this)
	{
		jint need = s._value.size();

		core_ensureCapacity(_used + need);
		memcpy(_buffer.data() + _used, s._value.data(), need * sizeof(jchar));

		_used += need;
	}
	return *this;
}

StringBuffer& StringBuffer::append(const StringBuffer& s)
{
	synchronized (this)
	{
		jint need = s._used;

		core_ensureCapacity(_used + need);
		memcpy(_buffer.data() + _used, s._buffer.data(), need * sizeof(jchar));

		_used += need;
	}
	return *this;
}

void StringBuffer::core_ensureCapacity(jint minimum)
{
	if (minimum > _buffer.size())
	{
		jint newcapacity = _buffer.size() * 2 + 2;

		if (minimum > newcapacity)
			newcapacity = minimum;

		_buffer.resize(newcapacity);
	}
}

void StringBuffer::ensureCapacity(jint minimum)
{
	synchronized (this)
	{
		core_ensureCapacity(minimum);
	}
}

jchar StringBuffer::charAt(jint index) const throw (IndexOutOfBoundsException)
{
	jchar result = 0;

	synchronized (this)
	{
		if (index >= _used)
			throw IndexOutOfBoundsException();

		result = _buffer[index];
	}
	return result;
}

jint StringBuffer::length() const throw ()
{
	jint result = 0;

	synchronized (this)
	{
		result = _used;
	}
	return result;
}

CharSequence* StringBuffer::subSequence(jint beginIndex, jint endIndex) const throw (IndexOutOfBoundsException)
{
	String* result = 0;

	synchronized (this)
	{
		if (beginIndex < 0 || endIndex < 0)
			throw IndexOutOfBoundsException();

		if (beginIndex > endIndex || endIndex > _used)
			throw IndexOutOfBoundsException();

		result = new String(_buffer.data(), beginIndex, endIndex - beginIndex);
	}
	return result;
}

String StringBuffer::toString() const throw ()
{
	synchronized (this)
	{
		return String(_buffer.data(), 0, _used);
	}
	return String();
}
