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

#include "beecrypt/c++/provider/DHKeyAgreement.h"
#include "beecrypt/c++/provider/DHPublicKeyImpl.h"

#include "beecrypt/c++/crypto/interfaces/DHPrivateKey.h"
using beecrypt::crypto::interfaces::DHPrivateKey;
#include "beecrypt/c++/crypto/interfaces/DHPublicKey.h"
using beecrypt::crypto::interfaces::DHPublicKey;
#include "beecrypt/c++/crypto/SecretKeyFactory.h"
using beecrypt::crypto::SecretKeyFactory;
#include "beecrypt/c++/crypto/spec/SecretKeySpec.h"
using beecrypt::crypto::spec::SecretKeySpec;
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;

#include <memory>
using std::auto_ptr;

using namespace beecrypt::provider;

DHKeyAgreement::DHKeyAgreement()
{
	_state = UNINITIALIZED;
	_secret = 0;
}

DHKeyAgreement::~DHKeyAgreement()
{
	delete _secret;
}

Key* DHKeyAgreement::engineDoPhase(const Key& key, bool lastPhase) throw (InvalidKeyException, IllegalStateException)
{
	if (_state == INITIALIZED)
	{
		const DHPublicKey* pub = dynamic_cast<const DHPublicKey*>(&key);
		if (pub)
		{
			transform(_y, pub->getY());

			if (mpz(_y.size, _y.data) || mpgex(_y.size, _y.data, _param.p.size, _param.p.modl))
				throw InvalidKeyException("y must be in range [1 .. (p-1)]");

			mpnumber _s;

			if (dlsvdp_pDHSecret(&_param, &_x, &_y, &_s))
			   throw ProviderException("BeeCrypt internal error in dlsvdp_pDHSecret");

			if (lastPhase)
			{
				size_t req = (_s.bitlength() + 8) >> 3;

				_secret = new bytearray(req);

				i2osp(_secret->data(), _secret->size(), _s.data, _s.size);

				_state = SHARED;

				return 0;
			}
			else
			{
				// multi-party DH
				return new DHPublicKeyImpl(_param, _s);
			}
		}
		else
			throw InvalidKeyException("not a DHPublicKey");
	}
	else
		throw IllegalStateException("DHKeyAgreement wasn't initialized");
}


void DHKeyAgreement::engineInit(const Key& key, SecureRandom* random) throw (InvalidKeyException)
{
	const DHPrivateKey* pri = dynamic_cast<const DHPrivateKey*>(&key);
	if (pri)
	{
		transform(_param.p, pri->getParams().getP());
		transform(_param.g, pri->getParams().getG());

		transform(_x, pri->getX());

		_state = INITIALIZED;
	}
	else
		throw InvalidKeyException("not a DHPrivateKey");
}

bytearray* DHKeyAgreement::engineGenerateSecret() throw (IllegalStateException)
{
	if (_state == SHARED)
	{
		bytearray* tmp = _secret;
		_secret = 0;

		_state = INITIALIZED;

		return tmp;
	}
	else
		throw IllegalStateException();
}

int DHKeyAgreement::engineGenerateSecret(bytearray& b, int offset) throw (IllegalStateException, ShortBufferException)
{
	if (_state == SHARED)
	{
		int size = _secret->size();

		if ((b.size() - offset) > size)
			throw ShortBufferException();

		memcpy(b.data() + offset, _secret->data(), size);

		delete _secret;
		_secret = 0;

		_state = INITIALIZED;

		return size;
	}
	else
		throw IllegalStateException();
}

SecretKey* DHKeyAgreement::engineGenerateSecret(const String& algorithm) throw (IllegalStateException, NoSuchAlgorithmException, InvalidKeyException)
{
	if (_state == SHARED)
	{
		_state = INITIALIZED;

		SecretKeyFactory* skf = SecretKeyFactory::getInstance(algorithm);

		SecretKeySpec spec(*_secret, algorithm);

		delete _secret;
		_secret = 0;

		try
		{
			auto_ptr<SecretKey> tmp(skf->generateSecret(spec));

			return tmp.release();
		}
		catch (InvalidKeySpecException& e)
		{
			throw InvalidKeyException().initCause(e);
		}
	}
	else
		throw IllegalStateException();
}

void DHKeyAgreement::engineInit(const Key& key, const AlgorithmParameterSpec& spec, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
	engineInit(key, random);
}
