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

#include "beecrypt/c++/io/ByteArrayOutputStream.h"
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::io;

ByteArrayOutputStream::ByteArrayOutputStream() : buf(32)
{
	count = 0;
}

ByteArrayOutputStream::ByteArrayOutputStream(jint size) : buf(size)
{
	count = 0;
}

ByteArrayOutputStream::~ByteArrayOutputStream()
{
}

void ByteArrayOutputStream::reset() throw ()
{
	count = 0;
}

jint ByteArrayOutputStream::size() throw ()
{
	return count;
}

bytearray* ByteArrayOutputStream::toByteArray()
{
	bytearray* result = new bytearray();

	toByteArray(*result);

	return result;
}

void ByteArrayOutputStream::toByteArray(bytearray& b)
{
	synchronized (this)
	{
		b.resize(count);
		memcpy(b.data(), buf.data(), count);
	}
}

void ByteArrayOutputStream::toByteArray(byte* data, jint offset, jint length)
{
	if (!data)
		throw NullPointerException();

	synchronized (this)
	{
		memcpy(data+offset, buf.data(), length < count ? length : count);
	}
}

void ByteArrayOutputStream::close() throw (IOException)
{
}

void ByteArrayOutputStream::flush() throw (IOException)
{
}

void ByteArrayOutputStream::write(byte b) throw (IOException)
{
	synchronized (this)
	{
		jint newcount = count+1;
		jint actualsz = buf.size();

		if (actualsz < newcount)
		{
			if (actualsz == 0)
				buf.resize(32);
			else
				buf.resize(actualsz << 1);
		}

		buf[count++] = b;
	}
}

void ByteArrayOutputStream::write(const byte* data, jint offset, jint length) throw (IOException)
{
	if (length)
	{
		if (!data)
			throw NullPointerException();

		synchronized (this)
		{
			jint newcount = count + length;
			jint actualsz = buf.size();

			if (newcount > actualsz)
				buf.resize(newcount > (actualsz << 1) ? newcount : (actualsz << 1));
	
			memcpy(buf.data()+count, data, length);
			count += length;
		}
	}
}

void ByteArrayOutputStream::write(const bytearray& b) throw (IOException)
{
	write(b.data(), 0, b.size());
}

void ByteArrayOutputStream::writeTo(OutputStream& out) throw (IOException)
{
	if (count)
	{
		synchronized (this)
		{
			out.write(buf.data(), 0, count);
		}
	}
}
