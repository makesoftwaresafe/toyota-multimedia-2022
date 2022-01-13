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

#include "beecrypt/c++/provider/DSAPrivateKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_DSA("DSA");
}

DSAPrivateKeyImpl::DSAPrivateKeyImpl(const DSAPrivateKey& copy)
{
	_params = new DSAParameterSpec(copy.getParams());
	_x = copy.getX();
	_enc = 0;
}

DSAPrivateKeyImpl::DSAPrivateKeyImpl(const DSAPrivateKeyImpl& copy)
{
	_params = new DSAParameterSpec(*copy._params);
	_x = copy._x;
	_enc = 0;
}

DSAPrivateKeyImpl::DSAPrivateKeyImpl(const DSAParams& params, const BigInteger& x) : _x(x)
{
	_params = new DSAParameterSpec(params.getP(), params.getQ(), params.getG());
	_enc = 0;
}

DSAPrivateKeyImpl::DSAPrivateKeyImpl(const dsaparam& params, const mpnumber& x) : _x(x)
{
	_params = new DSAParameterSpec(BigInteger(params.p), BigInteger(params.q), BigInteger(params.g));
	_enc = 0;
}

DSAPrivateKeyImpl::DSAPrivateKeyImpl(const BigInteger& p, const BigInteger& q, const BigInteger& g, const BigInteger& x) : _x(x)
{
	_params = new DSAParameterSpec(p, q, g);
	_enc = 0;
}

DSAPrivateKeyImpl::~DSAPrivateKeyImpl()
{
	delete _params;
	delete _enc;
}

DSAPrivateKeyImpl* DSAPrivateKeyImpl::clone() const throw ()
{
	return new DSAPrivateKeyImpl(*this);
}

bool DSAPrivateKeyImpl::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	if (obj)
	{
		const DSAPrivateKey* pri = dynamic_cast<const DSAPrivateKey*>(obj);
		if (pri)
		{
			if (pri->getParams().getP() != _params->getP())
				return false;

			if (pri->getParams().getQ() != _params->getQ())
				return false;

			if (pri->getParams().getG() != _params->getG())
				return false;

			if (pri->getX() != _x)
				return false;

			return true;
		}
	}

	return false;
}

const DSAParams& DSAPrivateKeyImpl::getParams() const throw ()
{
	return *_params;
}

const BigInteger& DSAPrivateKeyImpl::getX() const throw ()
{
	return _x;
}

const bytearray* DSAPrivateKeyImpl::getEncoded() const throw ()
{
	if (!_enc)
	{
		try
		{
			ByteArrayOutputStream bos;
			BeeOutputStream bee(bos);

			bee.writeBigInteger(_params->getP());
			bee.writeBigInteger(_params->getQ());
			bee.writeBigInteger(_params->getG());
			bee.writeBigInteger(_x);
			bee.close();

			_enc = bos.toByteArray();
		}
		catch (IOException&)
		{
		}
	}

	return _enc;
}

const String& DSAPrivateKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_DSA;
}

const String* DSAPrivateKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
