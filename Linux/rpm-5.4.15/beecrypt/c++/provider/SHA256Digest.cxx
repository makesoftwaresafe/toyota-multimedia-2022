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
#include "beecrypt/c++/provider/SHA256Digest.h"

using namespace beecrypt::provider;

SHA256Digest::SHA256Digest() : _digest(32)
{
	if (sha256Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha256Reset");
}

SHA256Digest::~SHA256Digest()
{
}

SHA256Digest* SHA256Digest::clone() const throw ()
{
	SHA256Digest* result = new SHA256Digest();

	memcpy(&result->_param, &_param, sizeof(sha256Param));

	return result;
}

const bytearray& SHA256Digest::engineDigest()
{
	if (sha256Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in sha256Digest");

	return _digest;
}

int SHA256Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 32)
		throw ShortBufferException();

	if (sha256Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in sha256Digest");

	return 32;
}

int SHA256Digest::engineGetDigestLength()
{
	return 32;
}

void SHA256Digest::engineReset()
{
	if (sha256Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha256Reset");
}

void SHA256Digest::engineUpdate(byte b)
{
	if (sha256Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in sha256Update");
}

void SHA256Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (sha256Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in sha256Update");
}
