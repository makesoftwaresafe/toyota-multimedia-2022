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
#include "beecrypt/c++/provider/SHA512Digest.h"

using namespace beecrypt::provider;

SHA512Digest::SHA512Digest() : _digest(64)
{
	if (sha512Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha512Reset");
}

SHA512Digest::~SHA512Digest()
{
}

SHA512Digest* SHA512Digest::clone() const throw ()
{
	SHA512Digest* result = new SHA512Digest();

	memcpy(&result->_param, &_param, sizeof(sha512Param));

	return result;
}

const bytearray& SHA512Digest::engineDigest()
{
	if (sha512Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in sha512Digest");

	return _digest;
}

int SHA512Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 64)
		throw ShortBufferException();

	if (sha512Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in sha512Digest");

	return 64;
}

int SHA512Digest::engineGetDigestLength()
{
	return 64;
}

void SHA512Digest::engineReset()
{
	if (sha512Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha512Reset");
}

void SHA512Digest::engineUpdate(byte b)
{
	if (sha512Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in sha512Digest");
}

void SHA512Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (sha512Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in sha512Digest");
}
