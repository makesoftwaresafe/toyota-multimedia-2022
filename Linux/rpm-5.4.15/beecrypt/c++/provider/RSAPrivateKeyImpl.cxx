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

#include "beecrypt/c++/provider/RSAPrivateKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_RSA("RSA");
}

RSAPrivateKeyImpl::RSAPrivateKeyImpl(const RSAPrivateKey& copy) : _n(copy.getModulus()), _d(copy.getPrivateExponent())
{
	_enc = 0;
}

RSAPrivateKeyImpl::RSAPrivateKeyImpl(const RSAPrivateKeyImpl& copy) : _n(copy._n), _d(copy._d)
{
	_enc = 0;
}

RSAPrivateKeyImpl::RSAPrivateKeyImpl(const BigInteger& n, const BigInteger& d) : _n(n), _d(d)
{
	_enc = 0;
}

RSAPrivateKeyImpl::~RSAPrivateKeyImpl()
{
	delete _enc;
}

RSAPrivateKeyImpl* RSAPrivateKeyImpl::clone() const throw ()
{
	return new RSAPrivateKeyImpl(*this);
}

bool RSAPrivateKeyImpl::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	if (obj)
	{
		const RSAPrivateKey* pri = dynamic_cast<const RSAPrivateKey*>(obj);
		if (pri)
		{
			if (pri->getModulus() != _n)
				return false;

			if (pri->getPrivateExponent() != _d)
				return false;

			return true;
		}
	}
	return false;
}

const BigInteger& RSAPrivateKeyImpl::getModulus() const throw ()
{
	return _n;
}

const BigInteger& RSAPrivateKeyImpl::getPrivateExponent() const throw ()
{
	return _d;
}

const bytearray* RSAPrivateKeyImpl::getEncoded() const throw ()
{
	if (!_enc)
	{
		try
		{
			ByteArrayOutputStream bos;
			BeeOutputStream bee(bos);

			bee.writeBigInteger(_n);
			bee.writeBigInteger(_d);
			bee.close();

			_enc = bos.toByteArray();
		}
		catch (IOException&)
		{
        }
	}

	return _enc;
}

const String& RSAPrivateKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_RSA;
}

const String* RSAPrivateKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
