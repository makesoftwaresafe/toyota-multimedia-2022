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

#include "beecrypt/c++/security/spec/RSAPrivateCrtKeySpec.h"

using namespace beecrypt::security::spec;

RSAPrivateCrtKeySpec::RSAPrivateCrtKeySpec(const BigInteger& modulus, const BigInteger& publicExponent, const BigInteger& privateExponent, const BigInteger& primeP, const BigInteger& primeQ, const BigInteger& primeExponentP, const BigInteger& primeExponentQ, const BigInteger& crtCoefficient) : RSAPrivateKeySpec(modulus, privateExponent)
{
	_e = publicExponent;
	_p = primeP;
	_q = primeQ;
	_dp = primeExponentP;
	_dq = primeExponentQ;
	_qi = crtCoefficient;
}

const BigInteger& RSAPrivateCrtKeySpec::getPublicExponent() const throw ()
{
	return _e;
}

const BigInteger& RSAPrivateCrtKeySpec::getPrimeP() const throw ()
{
	return _p;
}

const BigInteger& RSAPrivateCrtKeySpec::getPrimeQ() const throw ()
{
	return _q;
}

const BigInteger& RSAPrivateCrtKeySpec::getPrimeExponentP() const throw ()
{
	return _dp;
}

const BigInteger& RSAPrivateCrtKeySpec::getPrimeExponentQ() const throw ()
{
	return _dq;
}

const BigInteger& RSAPrivateCrtKeySpec::getCrtCoefficient() const throw ()
{
	return _qi;
}
