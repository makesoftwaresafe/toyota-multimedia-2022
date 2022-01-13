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

#include "beecrypt/c++/provider/RSAPrivateCrtKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/beeyond/BeeOutputStream.h"
using beecrypt::beeyond::BeeOutputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_RSA("RSA");
}

RSAPrivateCrtKeyImpl::RSAPrivateCrtKeyImpl(const RSAPrivateCrtKey& copy) : _n(copy.getModulus()), _e(copy.getPublicExponent()), _d(copy.getPrivateExponent()), _p(copy.getPrimeP()), _q(copy.getPrimeQ()), _dp(copy.getPrimeExponentP()), _dq(copy.getPrimeExponentQ()), _qi(copy.getCrtCoefficient())
{
	_enc = 0;
}

RSAPrivateCrtKeyImpl::RSAPrivateCrtKeyImpl(const RSAPrivateCrtKeyImpl& copy) : _n(copy._n), _e(copy._e), _d(copy._d), _p(copy._p), _q(copy._q), _dp(copy._dp), _dq(copy._dq), _qi(copy._qi)
{
	_enc = 0;
}

RSAPrivateCrtKeyImpl::RSAPrivateCrtKeyImpl(const BigInteger& n, const BigInteger& e, const BigInteger& d, const BigInteger& p, const BigInteger& q, const BigInteger& dp, const BigInteger& dq, const BigInteger& qi) : _n(n), _e(e), _d(d), _p(p), _q(q), _dp(dp), _dq(dq), _qi(qi)
{
	_enc = 0;
}

RSAPrivateCrtKeyImpl::~RSAPrivateCrtKeyImpl()
{
	delete _enc;
}

bool RSAPrivateCrtKeyImpl::equals(const Object* obj) const throw ()
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

RSAPrivateCrtKeyImpl* RSAPrivateCrtKeyImpl::clone() const throw ()
{
	return new RSAPrivateCrtKeyImpl(*this);
}

const BigInteger& RSAPrivateCrtKeyImpl::getModulus() const throw ()
{
	return _n;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPrivateExponent() const throw ()
{
	return _d;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPublicExponent() const throw ()
{
	return _e;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPrimeP() const throw ()
{
	return _p;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPrimeQ() const throw ()
{
	return _q;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPrimeExponentP() const throw ()
{
	return _dp;
}

const BigInteger& RSAPrivateCrtKeyImpl::getPrimeExponentQ() const throw ()
{
	return _dq;
}

const BigInteger& RSAPrivateCrtKeyImpl::getCrtCoefficient() const throw ()
{
	return _qi;
}

const bytearray* RSAPrivateCrtKeyImpl::getEncoded() const throw ()
{
	if (!_enc)
	{
        try
        {
            ByteArrayOutputStream bos;
            BeeOutputStream bee(bos);

			bee.writeBigInteger(_n);
			bee.writeBigInteger(_d);
			bee.writeBigInteger(_e);
			bee.writeBigInteger(_p);
			bee.writeBigInteger(_q);
			bee.writeBigInteger(_dp);
			bee.writeBigInteger(_dq);
			bee.writeBigInteger(_qi);
            bee.close();

            _enc = bos.toByteArray();
        }
        catch (IOException&)
        {
        }
	}

	return _enc;
}

const String& RSAPrivateCrtKeyImpl::getAlgorithm() const throw ()
{
	return ALGORITHM_RSA;
}

const String* RSAPrivateCrtKeyImpl::getFormat() const throw ()
{
	return &FORMAT_BEE;
}
