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
#include "beecrypt/c++/provider/SHA1Digest.h"

using namespace beecrypt::provider;

SHA1Digest::SHA1Digest() : _digest(20)
{
	if (sha1Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha1Reset");
}

SHA1Digest::~SHA1Digest()
{
}

SHA1Digest* SHA1Digest::clone() const throw ()
{
	SHA1Digest* result = new SHA1Digest();

	memcpy(&result->_param, &_param, sizeof(sha1Param));

	return result;
}

const bytearray& SHA1Digest::engineDigest()
{
	if (sha1Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in sha1Digest");

	return _digest;
}

int SHA1Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 20)
		throw ShortBufferException();

	if (sha1Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in sha1Digest");

	return 20;
}

int SHA1Digest::engineGetDigestLength()
{
	return 20;
}

void SHA1Digest::engineReset()
{
	if (sha1Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha1Reset");
}

void SHA1Digest::engineUpdate(byte b)
{
	if (sha1Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in sha1Update");
}

void SHA1Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (sha1Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in sha1Update");
}
