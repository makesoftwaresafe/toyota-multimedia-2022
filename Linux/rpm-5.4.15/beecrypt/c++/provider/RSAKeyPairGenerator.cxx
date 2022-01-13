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

#include "beecrypt/c++/adapter.h"
using beecrypt::randomGeneratorContextAdapter;
#include "beecrypt/c++/provider/RSAKeyPairGenerator.h"
#include "beecrypt/c++/provider/RSAPublicKeyImpl.h"
#include "beecrypt/c++/provider/RSAPrivateCrtKeyImpl.h"
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;

#include "beecrypt/rsakp.h"

using namespace beecrypt::provider;

RSAKeyPairGenerator::RSAKeyPairGenerator()
{
	_size = 1024;
	_e = RSAKeyGenParameterSpec::F4;
	_srng = 0;
}

KeyPair* RSAKeyPairGenerator::genpair(randomGeneratorContext* rngc)
{
	rsakp _pair;

	transform(_pair.e, _e);

	if (rsakpMake(&_pair, rngc, _size ? _size : 1024))
		throw ProviderException("unexpected error in rsakpMake");

	return new KeyPair(new RSAPublicKeyImpl(_pair.n, _pair.e), new RSAPrivateCrtKeyImpl(_pair.n, _pair.e, _pair.d, _pair.p, _pair.q, _pair.dp, _pair.dq, _pair.qi));
}

KeyPair* RSAKeyPairGenerator::engineGenerateKeyPair()
{
	if (_srng)
	{
		randomGeneratorContextAdapter rngc(_srng);

		return genpair(&rngc);
	}
	else
	{
		randomGeneratorContext rngc(randomGeneratorDefault());

		return genpair(&rngc);
	}
}

void RSAKeyPairGenerator::engineInitialize(const AlgorithmParameterSpec& spec, SecureRandom* random) throw (InvalidAlgorithmParameterException)
{
	const RSAKeyGenParameterSpec* rsaspec = dynamic_cast<const RSAKeyGenParameterSpec*>(&spec);

	if (rsaspec)
	{
		_size = rsaspec->getKeysize();
		_e = rsaspec->getPublicExponent();
	}
	else
		throw InvalidAlgorithmParameterException("not an RSAKeyGenParameterSpec");
}

void RSAKeyPairGenerator::engineInitialize(int keysize, SecureRandom* random) throw (InvalidParameterException)
{
	if (keysize < 512)
		throw InvalidParameterException("Modulus size must be at least 512 bits");

	_size = keysize;
	_e = RSAKeyGenParameterSpec::F4;
	_srng = random;
}
