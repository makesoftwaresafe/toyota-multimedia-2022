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
#include "beecrypt/c++/provider/SHA384Digest.h"

using namespace beecrypt::provider;

SHA384Digest::SHA384Digest() : _digest(48)
{
	if (sha384Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha384Reset");
}

SHA384Digest::~SHA384Digest()
{
}

SHA384Digest* SHA384Digest::clone() const throw ()
{
	SHA384Digest* result = new SHA384Digest();

	memcpy(&result->_param, &_param, sizeof(sha384Param));

	return result;
}

const bytearray& SHA384Digest::engineDigest()
{
	if (sha384Digest(&_param, _digest.data()))
		throw ProviderException("BeeCrypt internal error in sha384Digest");

	return _digest;
}

int SHA384Digest::engineDigest(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < 48)
		throw ShortBufferException();

	if (sha384Digest(&_param, data))
		throw ProviderException("BeeCrypt internal error in sha384Digest");

	return 48;
}

int SHA384Digest::engineGetDigestLength()
{
	return 48;
}

void SHA384Digest::engineReset()
{
	if (sha384Reset(&_param))
		throw ProviderException("BeeCrypt internal error in sha384Reset");
}

void SHA384Digest::engineUpdate(byte b)
{
	if (sha384Update(&_param, &b, 1))
		throw ProviderException("BeeCrypt internal error in sha384Update");
}

void SHA384Digest::engineUpdate(const byte* data, int offset, int length)
{
	if (sha384Update(&_param, data+offset, length))
		throw ProviderException("BeeCrypt internal error in sha384Update");
}
