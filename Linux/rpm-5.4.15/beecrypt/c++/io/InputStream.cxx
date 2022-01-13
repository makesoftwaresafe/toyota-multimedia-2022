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

#include "beecrypt/c++/io/InputStream.h"
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::io;

jint InputStream::available() throw (IOException)
{
	return 0;
}

void InputStream::close() throw (IOException)
{
}

void InputStream::mark(jint readlimit) throw ()
{
}

bool InputStream::markSupported() throw ()
{
	return false;
}

jint InputStream::read(bytearray& b) throw (IOException)
{
	return read(b.data(), 0, b.size());
}

jint InputStream::read(byte* data, jint offset, jint length) throw (IOException)
{
	if (!data)
		throw NullPointerException();

	jint b = read();
	if (b < 0)
		return -1;

	data[offset] = (byte) b;

	jint i = 1;
	try
	{
		while (i < length)
		{
			b = read();
			if (b < 0)
				break;
			data[offset+i++] = (byte) b;
		}
	}
	catch (IOException&)
	{
		// ignore
	}
	return i;
}

jint InputStream::skip(jint n) throw (IOException)
{
	jint remaining = n;

	byte skip[2048];

	while (remaining > 0)
	{
		jint rc = read(skip, 0, remaining > 2048 ? 2048 : remaining);
		if (rc < 0)
			break;
		remaining -= rc;
	}

	return n - remaining;
}

void InputStream::reset() throw (IOException)
{
	throw IOException("reset not supported");
}
