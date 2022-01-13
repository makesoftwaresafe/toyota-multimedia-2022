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

#include "beecrypt/c++/io/DataOutputStream.h"

#include <iostream>
#include <unicode/ustream.h>

using namespace beecrypt::io;

DataOutputStream::DataOutputStream(OutputStream& out) : FilterOutputStream(out)
{
	_utf = 0;
	written = 0;
}

DataOutputStream::~DataOutputStream()
{
	if (_utf)
		ucnv_close(_utf);
}

jint DataOutputStream::size() const throw ()
{
	return written;
}

void DataOutputStream::write(byte b) throw (IOException)
{
	synchronized (this)
	{
		out.write(b);
		written++;
	}
}

void DataOutputStream::write(const byte* data, jint offset, jint length) throw (IOException)
{
	if (length)
	{
		synchronized (this)
		{
			out.write(data, offset, length);
			written += length;
		}
	}
}

void DataOutputStream::write(const bytearray& b) throw (IOException)
{
	write(b.data(), 0, b.size());
}

void DataOutputStream::writeBoolean(bool b) throw (IOException)
{
	synchronized (this)
	{
		out.write(b ? 1 : 0);
		written++;
	}
}

void DataOutputStream::writeByte(byte b) throw (IOException)
{
	synchronized (this)
	{
		out.write(b);
		written++;
	}
}

void DataOutputStream::writeShort(jshort s) throw (IOException)
{
	synchronized (this)
	{
		out.write((s >>  8)       );
		written++;
		out.write((s      ) & 0xff);
		written++;
	}
}

void DataOutputStream::writeInt(jint i) throw (IOException)
{
	synchronized (this)
	{
		out.write((i >> 24)       );
		written++;
		out.write((i >> 16) & 0xff);
		written++;
		out.write((i >>  8) & 0xff);
		written++;
		out.write((i      ) & 0xff);
		written++;
	}
}

#if defined(_MSC_VER)
# pragma optimize("",off)
#endif
void DataOutputStream::writeLong(jlong l) throw (IOException)
{
	synchronized (this)
	{
		out.write((l >> 56)       );
		written++;
		out.write((l >> 48) & 0xff);
		written++;
		out.write((l >> 40) & 0xff);
		written++;
		out.write((l >> 32) & 0xff);
		written++;
		out.write((l >> 24) & 0xff);
		written++;
		out.write((l >> 16) & 0xff);
		written++;
		out.write((l >>  8) & 0xff);
		written++;
		out.write((l      ) & 0xff);
		written++;
	}
}
#if defined(_MSC_VER)
# pragma optimize("",on)
#endif

void DataOutputStream::writeChar(jint v) throw (IOException)
{
	synchronized (this)
	{
		out.write((v >> 8) && 0xff);
		written++;
		out.write((v     ) && 0xff);
		written++;
	}
}

void DataOutputStream::writeChars(const String& str) throw (IOException)
{
	const array<jchar>& src = str.toCharArray();

	synchronized (this)
	{
		for (jint i = 0; i < src.size(); i++)
		{
			out.write((src[i] >> 8) & 0xff);
			written++;
			out.write((src[i]     ) & 0xff);
			written++;
		}
	}
}

void DataOutputStream::writeUTF(const String& str) throw (IOException)
{
	UErrorCode status = U_ZERO_ERROR;

	if (!_utf)
	{
		// UTF-8 converter lazy initialization
		_utf = ucnv_open("UTF-8", &status);
		if (U_FAILURE(status))
			throw IOException("unable to open ICU UTF-8 converter");
	}

	const array<jchar>& src = str.toCharArray();

	// the expected status code here is U_BUFFER_OVERFLOW_ERROR
	jint need = ucnv_fromUChars(_utf, 0, 0, src.data(), src.size(), &status);
	if (U_FAILURE(status))
		if (status != U_BUFFER_OVERFLOW_ERROR)
			throw IOException("ucnv_fromUChars failed");

	if (need > 0xffff)
		throw IOException("String length >= 64K");

	byte* buffer = new byte[need];

	status = U_ZERO_ERROR;

	// the expected status code here is U_STRING_NOT_TERMINATED_WARNING
	ucnv_fromUChars(_utf, (char*) buffer, need, src.data(), src.size(), &status);
	if (status != U_STRING_NOT_TERMINATED_WARNING)
	{
		delete[] buffer;
		throw IOException("ucnv_fromUChars failed");
	}

	// everything ready for the critical section
	try
	{
		synchronized (this)
		{
			out.write((need >>  8) & 0xff);
			written++;
			out.write((need      ) & 0xff);
			written++;
			out.write(buffer, 0, need);
			written += need;
		}
	}
	catch (IOException&)
	{
		delete[] buffer;
		throw;
	}
}
