/*
 * Copyright (c) 2009 Bob Deblier
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
#include "beecrypt/c++/provider/SHA224Digest.h"

using namespace beecrypt::provider;

SHA224Digest::SHA224Digest() : _digest(28)
{
	if (sha224Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha224Reset");
}

SHA224Digest::~SHA224Digest()
{
}

SHA224Digest* SHA224Digest::clone() const throw ()
{
	SHA224Digest* result = new SHA224Digest();

	memcpy(&result->_param, &_param, sizeof(sha224Param));

	return result;
}

const bytearray& SHA224Digest::engineDigest()
{
	if (sha224Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in sha224Digest");

	return _digest;
}

int SHA224Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 32)
		throw ShortBufferException();

	if (sha224Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in sha224Digest");

	return 32;
}

int SHA224Digest::engineGetDigestLength()
{
	return 32;
}

void SHA224Digest::engineReset()
{
	if (sha224Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha224Reset");
}

void SHA224Digest::engineUpdate(byte b)
{
	if (sha224Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in sha224Update");
}

void SHA224Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (sha224Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in sha224Update");
}
