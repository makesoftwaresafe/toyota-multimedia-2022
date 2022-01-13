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

#include "beecrypt/c++/io/PushbackInputStream.h"
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::io;

PushbackInputStream::PushbackInputStream(InputStream& in, int size) : FilterInputStream(in), buf(size)
{
	_closed = false;
	pos = 0;
}

PushbackInputStream::~PushbackInputStream()
{
}

int PushbackInputStream::available() throw (IOException)
{
	if (_closed)
		throw IOException("Stream closed");

	return in.available() + (buf.size() - pos);
}

void PushbackInputStream::close() throw (IOException)
{
	if (!_closed)
	{
		in.close();
		_closed = true;
	}
}

int PushbackInputStream::read() throw (IOException)
{
	if (_closed)
		throw IOException("Stream closed");

	if (pos < buf.size())
		return buf[pos++];

	return in.read();
}

bool PushbackInputStream::markSupported() throw ()
{
	return false;
}

int PushbackInputStream::read(byte* data, int offset, int length) throw (IOException)
{
	if (!data)
		throw NullPointerException();

	if (_closed)
		throw IOException("Stream closed");

	if (length == 0)
		return 0;

	int buffered = buf.size() - pos;

	if (buffered > 0)
	{
		if (length < buffered)
			buffered = length;

		memcpy(data+offset, buf.data()+pos, buffered);

		pos += buffered;
		offset += buffered;
		length -= buffered;
	}

	if (length > 0)
	{
		int rc = in.read(data, offset, length);
		if (rc < 0) {
			if (buffered == 0)
				return -1; // nothing in buffer and nothing read
			else
				return buffered; // something in buffer and nothing read
		}

		return buffered + rc; // something in buffer and something read
	}

	return buffered; // everything was in buffer
}

int PushbackInputStream::skip(int n) throw (IOException)
{
	if (_closed)
		throw IOException("Stream closed");

	if (n == 0)
		return 0;

	int canskip = buf.size() - pos;

	if (canskip > 0)
	{
		if (n < canskip)
		{
			// more in buffer than we need to skip
			canskip = n;
		}
		pos += canskip;
		n -= canskip;
	}

	if (n > 0)
	{
		// apparently we didn't have enough in the buffer
		canskip += in.skip(n);
	}

	return canskip;
}

void PushbackInputStream::unread(byte b) throw (IOException)
{
	if (_closed)
		throw IOException("Stream closed");

	if (pos == 0)
		throw IOException("Pushback buffer is full");

	buf[--pos] = b;
}

void PushbackInputStream::unread(const bytearray& b) throw (IOException)
{
	unread(b.data(), 0, b.size());
}

void PushbackInputStream::unread(const byte* data, int offset, int length) throw (IOException)
{
	if (!data)
		throw NullPointerException();

	pos -= length;

	memcpy(buf.data()+pos, data+offset, length);
}
