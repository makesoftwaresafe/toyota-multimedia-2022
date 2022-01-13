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

#include "beecrypt/c++/io/DataInputStream.h"
#include "beecrypt/c++/io/EOFException.h"
#include "beecrypt/c++/io/PushbackInputStream.h"
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

#define MAX_BYTES_PER_CHARACTER	8

using namespace beecrypt::io;

DataInputStream::DataInputStream(InputStream& in) : FilterInputStream(in)
{
	_pin = &in;
	_del = false;
	_utf = 0;
	_loc = 0;
}

DataInputStream::~DataInputStream()
{
	if (_utf)
	{
		ucnv_close(_utf);
		_utf = 0;
	}

	if (_loc)
	{
		ucnv_close(_loc);
		_loc = 0;
	}

	if (_del)
	{
		delete _pin;
		_pin = 0;
	}
}

bool DataInputStream::readBoolean() throw (IOException)
{
	register jint b = _pin->read();

	if (b < 0)
		throw EOFException();

	return (b != 0);
}

jbyte DataInputStream::readByte() throw (IOException)
{
	register jint b = _pin->read();

	if (b < 0)
		throw EOFException();

	return static_cast<jbyte>(b);
}

jint DataInputStream::readUnsignedByte() throw (IOException)
{
	register jint b = _pin->read();

	if (b < 0)
		throw EOFException();

	return b;
}

jshort DataInputStream::readShort() throw (IOException)
{
	register jshort tmp = 0;
	register jint rc;

	for (register unsigned i = 0; i < 2; i++)
	{
		if ((rc = _pin->read()) < 0)
			throw EOFException();

		tmp = (tmp << 8) + rc;
	}

	return tmp;
}

jint DataInputStream::readUnsignedShort() throw (IOException)
{
	register jint tmp = 0, rc;

	for (register unsigned i = 0; i < 2; i++)
	{
		if ((rc = _pin->read()) < 0)
			throw EOFException();

		tmp = (tmp << 8) + rc;
	}

	return tmp;
}

jchar DataInputStream::readChar() throw (IOException)
{
	register jchar tmp = 0;
	register jint rc;

	for (register unsigned i = 0; i < 2; i++)
	{
		if ((rc = _pin->read()) < 0)
			throw EOFException();

		tmp = (tmp << 8) + rc;
	}

	return tmp;
}

jint DataInputStream::readInt() throw (IOException)
{
	register jint tmp = 0;
	register jint rc;

	for (register unsigned i = 0; i < 4; i++)
	{
		if ((rc = _pin->read()) < 0)
			throw EOFException();

		tmp = (tmp << 8) + rc;
	}

	return tmp;
}

jlong DataInputStream::readLong() throw (IOException)
{
	register jlong tmp = 0;
	register jint rc;

	for (register unsigned i = 0; i < 8; i++)
	{
		if ((rc = _pin->read()) < 0)
			throw EOFException();

		tmp = (tmp << 8) + rc;
	}

	return tmp;
}

String DataInputStream::readUTF() throw (IOException)
{
	UErrorCode status = U_ZERO_ERROR;

	if (!_utf)
	{
		// UTF-8 converter lazy initialization
		_utf = ucnv_open("UTF-8", &status);
		if (U_FAILURE(status))
			throw IOException("unable to open ICU UTF-8 converter");
	}

	jint utflen = readUnsignedShort();

	if (utflen > 0)
	{
		byte* data = new byte[utflen];

		readFully(data, 0, utflen);

		status = U_ZERO_ERROR;
		jint ulen = ucnv_toUChars(_utf, 0, 0, (const char*) data, (jint) utflen, &status);
		if (status != U_BUFFER_OVERFLOW_ERROR)
		{
			delete[] data;
			throw IOException("ucnv_toUChars failed");
		}

		jchar* buffer = new jchar[ulen+1];

		status = U_ZERO_ERROR;
		ucnv_toUChars(_utf, buffer, ulen+1, (const char*) data, (jint) utflen, &status);

		delete[] data;

		if (status != U_ZERO_ERROR)
			throw IOException("error in ucnv_toUChars");

		String result(buffer, 0, ulen);

		delete[] buffer;

		return result;
	}
	else
		return String();
}

String DataInputStream::readLine() throw (IOException)
{
	UErrorCode status = U_ZERO_ERROR;

	if (!_loc)
	{
		// default locale converter lazy initialization
		_loc = ucnv_open(0, &status);
		if (U_FAILURE(status))
			throw IOException("unable to open ICU default locale converter");
	}

	array<jchar> target_buffer(80);
	jint         target_offset = 0;
	      UChar* target = target_buffer.data();
	const UChar* target_limit = target+1;
	      char  source_buffer[MAX_BYTES_PER_CHARACTER];
	const char* source = source_buffer;
	      char* source_limit = source_buffer;

	bool cr = false;

	jint ch;

	do
	{
		ch = _pin->read();

		if (ch >= 0)
		{
			if ((source_limit-source_buffer) == MAX_BYTES_PER_CHARACTER)
				throw IOException("fubar in readLine");

			*(source_limit++) = (byte) ch;
		}

		// use the default locale converter; flush if ch == -1
		ucnv_toUnicode(_loc, &target, target_limit, &source, source_limit, NULL, (UBool) (ch == -1), &status);
		if (U_FAILURE(status))
			throw IOException("ucnv_toUnicode failed");

		if (target == target_limit)
		{
			// we got a whole character from the converter
			if (cr)
			{
				// last character read was ASCII <CR>; is this one a <LF>?
				if (target[-1] != 0x0A)
				{
					// unread the right number of bytes 
					PushbackInputStream* p = dynamic_cast<PushbackInputStream*>(_pin);
					if (p)
						p->unread((const byte*) source_buffer, 0, source-source_buffer);
					else
						throw IOException("fubar in dynamic_cast");
				}
				// we're now officially at the end of the line
				break;
			}

			// did we get an ASCII <LF>?
			if (target[-1] == 0x0A)
				break;

			// did we get an ASCII <CR>?
			if (target[-1] == 0x0D)
			{
				cr = true;

				// the next character may be a <LF> but if not we'll have to 'unread' it
				if (!_del)
				{
					// lazy push
					_pin = new PushbackInputStream(in, MAX_BYTES_PER_CHARACTER);
					_del = true;
				}
			}
			else
			{
				// reset source pointers
				source = source_limit = source_buffer;
				// advance target_limit and target_offset
				target_limit++;
				target_offset++;
				// check if we have room left in the buffer
				if (target_offset == target_buffer.size())
				{
					// no; double the size
					target_buffer.resize(target_buffer.size() * 2);
				}
			}
		}
	} while (ch >= 0);

	return String(target_buffer.data(), 0, target_offset);
}

void DataInputStream::readFully(byte* data, jint offset, jint length) throw (IOException)
{
	if (!data)
		throw NullPointerException();

	jint total = 0;

	while (total < length)
	{
		jint rc = _pin->read(data, offset+total, length-total);
		if (rc < 0)
			throw EOFException();
		total += rc;
	}
}

void DataInputStream::readFully(bytearray& b) throw (IOException)
{
	readFully(b.data(), 0, b.size());
}

jint DataInputStream::skipBytes(jint n) throw (IOException)
{
	jint total = 0, rc;

	while ((total < n) && ((rc = _pin->skip(n - total)) > 0))
		total += rc;

	return total;
}
