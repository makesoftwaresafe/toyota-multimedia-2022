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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;
#include "beecrypt/c++/provider/MD5Digest.h"

using namespace beecrypt::provider;

MD5Digest::MD5Digest() : _digest(16)
{
	if (md5Reset(&_param))
		throw ProviderException("BeeCrypt internal error in md5Reset");
}

MD5Digest::~MD5Digest()
{
}

MD5Digest* MD5Digest::clone() const throw ()
{
	MD5Digest* result = new MD5Digest();

	memcpy(&result->_param, &_param, sizeof(md5Param));

	return result;
}

const bytearray& MD5Digest::engineDigest()
{
	if (md5Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in md5Digest");

	return _digest;
}

int MD5Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 16)
		throw ShortBufferException();

	if (md5Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in md5Digest");

	return 16;
}

int MD5Digest::engineGetDigestLength()
{
	return 16;
}

void MD5Digest::engineReset()
{
	if (md5Reset(&_param))
		throw ProviderException("BeeCrypt internal error in md5Reset");
}

void MD5Digest::engineUpdate(byte b)
{
	if (md5Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in md5Update");
}

void MD5Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (md5Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in md5Update");
}
