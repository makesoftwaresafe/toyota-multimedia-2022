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

#include "beecrypt/c++/io/ByteArrayInputStream.h"
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::io;

ByteArrayInputStream::ByteArrayInputStream(const bytearray& b) : _buf(b)
{
	_count = _buf.size();
	_mark = 0;
	_pos = 0;
}

ByteArrayInputStream::ByteArrayInputStream(const byte* data, jint offset, jint length) : _buf(data+offset, length)
{
	_count = _buf.size();
	_mark = 0;
	_pos = 0;
}

ByteArrayInputStream::~ByteArrayInputStream()
{
}

jint ByteArrayInputStream::available() throw (IOException)
{
	return _count - _pos;
}

void ByteArrayInputStream::close() throw (IOException)
{
}

void ByteArrayInputStream::mark(jint readlimit) throw ()
{
	_mark = _pos;
}

bool ByteArrayInputStream::markSupported() throw ()
{
	return true;
}

jint ByteArrayInputStream::read() throw (IOException)
{
	register jint rc = -1;

	synchronized (this)
	{
		if (_pos < _count)
			rc = _buf[_pos++];
	}

	return rc;
}

jint ByteArrayInputStream::read(byte* data, jint offset, jint length) throw (IOException)
{
	if (!data)
		throw NullPointerException();

	synchronized (this)
	{
		if (_pos >= _count)
			return -1;

		if (_pos + length > _count)
			length = _count - _pos;

		if (length == 0)
			return 0;

		memcpy(data+offset, _buf.data()+_pos, length);
		_pos += length;
	}

	return length;
}

jint ByteArrayInputStream::read(bytearray& b) throw (IOException)
{
	return read(b.data(), 0, b.size());
}

void ByteArrayInputStream::reset() throw (IOException)
{
	synchronized (this)
	{
		_pos = _mark;
	}
}

jint ByteArrayInputStream::skip(jint n) throw (IOException)
{
	synchronized (this)
	{
		if (_pos + n > _count)
			n = _count - _pos;
		_pos += n;
	}
	return n;
}
