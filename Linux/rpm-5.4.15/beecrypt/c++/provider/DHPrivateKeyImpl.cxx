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

#include "beecrypt/c++/provider/DHPrivateKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_DH("DH");
}

DHPrivateKeyImpl::DHPrivateKeyImpl(const DHPrivateKey& copy) : _x(copy.getX())
{
	_params = new DHParameterSpec(copy.getParams());
	_enc = 0;
}


DHPrivateKeyImpl::DHPrivateKeyImpl(const DHPrivateKeyImpl& copy) : _x(copy._x)
{
	_params = new DHParameterSpec(*copy._params);
	_enc = 0;
}

DHPrivateKeyImpl::DHPrivateKeyImpl(const DHParams& params, const BigInteger& x) : _x(x)
{
	_params = new DHParameterSpec(params.getP(), params.getG(), params.getL());
	_enc = 0;
}

DHPrivateKeyImpl::DHPrivateKeyImpl(const dhparam& params, const mpnumber& x) : _x(x)
{
	_params = new DHParameterSpec(BigInteger(params.p), BigInteger(params.g));
	_enc = 0;
}

DHPrivateKeyImpl::DHPrivateKeyImpl(const BigInteger& p, const BigInteger& g, const BigInteger& x) : _x(x)
{
	_params = new DHParameterSpec(p, g);
	_enc = 0;
}

DHPrivateKeyImpl::~DHPrivateKeyImpl()
{
	delete _params;
	delete _enc;
}

DHPrivateKeyImpl* DHPrivateKeyImpl::clone() const throw ()
{
	return new DHPrivateKeyImpl(*this);
}

bool DHPrivateKeyImpl::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	const DHPrivateKey* pri = dynamic_cast<const DHPrivateKey*>(obj);
	if (pri)
	{
		if (pri->getParams().getP() != _params->getP())
			return false;

		if (pri->getParams().getG() != _params->getG())
			return false;

		if (pri->getX() != _x)
			return false;

		return true;
	}

	return false;
}

const DHParams& DHPrivateKeyImpl::getParams() const throw ()
{
	return *_params;
}

const BigInteger& DHPrivateKeyImpl::getX() const throw ()
{
	return _x;
}

const bytearray* DHPrivateKeyImpl::getEncoded() const throw ()
{
	if (!_enc)
	{
		try
		{
			ByteArrayOutputStream bos;
			BeeOutputStream bee(bos);

			bee.writeBigInteger(_params->getP());
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

const String& DHPrivateKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_DH;
}

const String* DHPrivateKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
