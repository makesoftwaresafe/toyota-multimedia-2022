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

#include "beecrypt/c++/io/Writer.h"
#include "beecrypt/c++/lang/IllegalArgumentException.h"
using beecrypt::lang::IllegalArgumentException;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::io;

Writer::Writer()
{
	lock = this;
}

Writer::Writer(Object& lock) : lock(&lock)
{
}


Writer& Writer::append(jchar c) throw (IOException)
{
	write(c);

	return *this;
}

Writer& Writer::append(const CharSequence& cseq) throw (IOException)
{
	write(cseq.toString());

	return *this;
}

void Writer::write(jint c) throw (IOException)
{
	synchronized (lock)
	{
		jchar tmp = c;

		write(&tmp, 0, 1);
	}
}

void Writer::write(const array<jchar>& cbuf) throw (IOException)
{
	synchronized (lock)
	{
		write(cbuf.data(), 0, cbuf.size());
	}
}

void Writer::write(const String& str) throw (IOException)
{
	synchronized (lock)
	{
		const array<jchar>& tmp = str.toCharArray();

		write(tmp.data(), 0, tmp.size());
	}
}

void Writer::write(const String& str, jint off, jint len) throw (IOException)
{
	if (off < 0 || len < 0 || off + len >= str.length())
		throw IllegalArgumentException();

	synchronized (lock)
	{
		const array<jchar>& tmp = str.toCharArray();

		write(tmp.data(), off, len);
	}
}
