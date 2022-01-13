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

#include "beecrypt/c++/provider/DSAPublicKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
    const String FORMAT_BEE("BEE");
    const String ALGORITHM_DSA("DSA");
}

DSAPublicKeyImpl::DSAPublicKeyImpl(const DSAPublicKey& copy)
{
	_params = new DSAParameterSpec(copy.getParams());
	_y = copy.getY();
	_enc = 0;
}

DSAPublicKeyImpl::DSAPublicKeyImpl(const DSAPublicKeyImpl& copy) : _y(copy._y)
{
	_params = new DSAParameterSpec(*copy._params);
	_enc = 0;
}

DSAPublicKeyImpl::DSAPublicKeyImpl(const DSAParams& params, const BigInteger& y) : _y(y)
{
	_params = new DSAParameterSpec(params.getP(), params.getQ(), params.getG());
	_enc = 0;
}

DSAPublicKeyImpl::DSAPublicKeyImpl(const dsaparam& params, const mpnumber& y) : _y(y)
{
	_params = new DSAParameterSpec(BigInteger(params.p), BigInteger(params.q), BigInteger(params.g));
	_enc = 0;
}

DSAPublicKeyImpl::DSAPublicKeyImpl(const BigInteger& p, const BigInteger& q, const BigInteger& g, const BigInteger& y)
{
	_params = new DSAParameterSpec(p, q, g);
	_y = y;
	_enc = 0;
}

DSAPublicKeyImpl::~DSAPublicKeyImpl()
{
	delete _params;
	delete _enc;
}

DSAPublicKeyImpl* DSAPublicKeyImpl::clone() const throw ()
{
	return new DSAPublicKeyImpl(*this);
}

bool DSAPublicKeyImpl::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	if (obj)
	{
		const DSAPublicKey* pub = dynamic_cast<const DSAPublicKey*>(obj);
		if (pub)
		{
			if (pub->getParams().getP() != _params->getP())
				return false;

			if (pub->getParams().getQ() != _params->getQ())
				return false;

			if (pub->getParams().getG() != _params->getG())
				return false;

			if (pub->getY() != _y)
				return false;

			return true;
		}
	}

	return false;
}

const DSAParams& DSAPublicKeyImpl::getParams() const throw ()
{
	return *_params;
}

const BigInteger& DSAPublicKeyImpl::getY() const throw ()
{
	return _y;
}

const bytearray* DSAPublicKeyImpl::getEncoded() const throw ()
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
			bee.writeBigInteger(_y);
			bee.close();

			_enc = bos.toByteArray();
		}
		catch (IOException&)
		{
		}
	}

	return _enc;
}

const String& DSAPublicKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_DSA;
}

const String* DSAPublicKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
