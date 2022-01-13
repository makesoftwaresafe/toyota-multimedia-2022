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

#include "beecrypt/c++/provider/RSAPublicKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
    const String FORMAT_BEE("BEE");
    const String ALGORITHM_RSA("RSA");
}

RSAPublicKeyImpl::RSAPublicKeyImpl(const RSAPublicKey& copy) : _n(copy.getModulus()), _e(copy.getPublicExponent())
{
	_enc = 0;
}

RSAPublicKeyImpl::RSAPublicKeyImpl(const RSAPublicKeyImpl& copy) : _n(copy._n), _e(copy._e)
{
	_enc = 0;
}

RSAPublicKeyImpl::RSAPublicKeyImpl(const BigInteger& n, const BigInteger& e) : _n(n), _e(e)
{
	_enc = 0;
}

RSAPublicKeyImpl::~RSAPublicKeyImpl()
{
	delete _enc;
}

RSAPublicKeyImpl* RSAPublicKeyImpl::clone() const throw ()
{
	return new RSAPublicKeyImpl(*this);
}

bool RSAPublicKeyImpl::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	if (obj)
	{
		const RSAPublicKey* pub = dynamic_cast<const RSAPublicKey*>(obj);
		if (pub)
		{
			if (pub->getModulus() != _n)
				return false;

			if (pub->getPublicExponent() != _e)
				return false;

			return true;
		}
	}
	return false;
}

const BigInteger& RSAPublicKeyImpl::getModulus() const throw ()
{
	return _n;
}

const BigInteger& RSAPublicKeyImpl::getPublicExponent() const throw ()
{
	return _e;
}

const bytearray* RSAPublicKeyImpl::getEncoded() const throw ()
{
	if (!_enc)
	{
		try
		{
			ByteArrayOutputStream bos;
			BeeOutputStream bee(bos);

			bee.writeBigInteger(_n);
			bee.writeBigInteger(_e);
			bee.close();

			_enc = bos.toByteArray();
		}
		catch (IOException&)
		{
        }
	}

	return _enc;
}

const String& RSAPublicKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_RSA;
}

const String* RSAPublicKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
