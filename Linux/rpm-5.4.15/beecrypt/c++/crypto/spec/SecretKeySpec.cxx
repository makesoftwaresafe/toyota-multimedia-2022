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

#include "beecrypt/c++/crypto/spec/SecretKeySpec.h"

using namespace beecrypt::crypto::spec;

SecretKeySpec::SecretKeySpec(const byte* data, size_t offset, size_t length, const String& algorithm) : _data(data+offset, length), _algo(algorithm)
{
}

SecretKeySpec::SecretKeySpec(const bytearray& b, const String& algorithm) : _data(b), _algo(algorithm)
{
}

const String& SecretKeySpec::getAlgorithm() const throw ()
{
	return _algo;
}

const String* SecretKeySpec::getFormat() const throw ()
{
	static const String FORMAT("RAW");

	return &FORMAT;
}

const bytearray* SecretKeySpec::getEncoded() const throw ()
{
	return &_data;
}
