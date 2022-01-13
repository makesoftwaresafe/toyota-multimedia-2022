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

#include "beecrypt/c++/provider/DHIESCipher.h"
#include "beecrypt/c++/provider/DHPublicKeyImpl.h"
#include "beecrypt/c++/crypto/SecretKeyFactory.h"
using beecrypt::crypto::SecretKeyFactory;
#include "beecrypt/c++/crypto/spec/SecretKeySpec.h"
using beecrypt::crypto::spec::SecretKeySpec;
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;

#include <memory>
using std::auto_ptr;

using namespace beecrypt::provider;

DHIESCipher::DHIESCipher()
{
	_dspec = 0;
	_spec = 0;

	_srng = 0;
	_kpg = 0;
	_ka = 0;

	_d = 0;
	_c = 0;
	_m = 0;

	_msg = 0;
	_buf = 0;

	try
	{
		_kpg = KeyPairGenerator::getInstance("DiffieHellman");
		_ka = KeyAgreement::getInstance("DiffieHellman");
	}
	catch (NoSuchAlgorithmException& e)
	{
		throw ProviderException().initCause(e);
	}
}

DHIESCipher::~DHIESCipher()
{
	delete _dspec;
	delete _spec;
	delete _kpg;
	delete _ka;
	delete _m;
	delete _c;
	delete _d;
	delete _msg;
}

bytearray* DHIESCipher::engineDoFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalBlockSizeException, BadPaddingException)
{
	bytearray* tmp;

	if (_buf)
	{
		bytearray ciphertext;

		_buf->write(input, inputOffset, inputLength);
		_buf->toByteArray(ciphertext);

		if (_m->doFinal(ciphertext) == _dspec->getMac())
		{
			// MAC matches; we can decrypt
			tmp = _c->doFinal(ciphertext);
		}
		else
			tmp = 0;
	}
	else
	{
		tmp = _c->doFinal(input, inputOffset, inputLength);

		_m->update(*tmp);

		_dspec = new DHIESDecryptParameterSpec(*_spec, _msg->getY(), _m->doFinal());
	}

	reset();

	return tmp;
}

int DHIESCipher::engineDoFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException)
{
	int tmp;

	if (_buf)
	{
		bytearray ciphertext;

		_buf->write(input, inputOffset, inputLength);
		_buf->toByteArray(ciphertext);

		if (_m->doFinal(ciphertext) == _dspec->getMac())
		{
			// Mac matches; we can decrypt
			tmp = _c->doFinal(ciphertext.data(), 0, ciphertext.size(), output, outputOffset);
		}
		else
			tmp = 0;
	}
	else
	{
		tmp = _c->doFinal(input, inputOffset, inputLength, output, outputOffset);

		_m->update(output.data(), outputOffset, tmp);

		_dspec = new DHIESDecryptParameterSpec(*_spec, _msg->getY(), _m->doFinal());
	}

	reset();

	return tmp;
}

int DHIESCipher::engineGetBlockSize() const throw ()
{
	return _c->getBlockSize();
}

bytearray* DHIESCipher::engineGetIV()
{
	return _c->getIV();
}

int DHIESCipher::engineGetOutputSize(int inputLength) throw ()
{
	return _c->getOutputSize(inputLength + (_buf ? _buf->size() : 0));
}

AlgorithmParameters* DHIESCipher::engineGetParameters() throw ()
{
	try
	{
		auto_ptr<AlgorithmParameters> tmp(AlgorithmParameters::getInstance("DHIES"));

		tmp->init(*_dspec);

		return tmp.release();
	}
	catch (Exception& e)
	{
		throw ProviderException().initCause(e);
	}
}

void DHIESCipher::engineInit(int opmode, const Key& key, SecureRandom* random) throw (InvalidKeyException)
{
	throw InvalidKeyException("DHIESCipher must be initialized with a key and parameters");
}

void DHIESCipher::engineInit(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
	if (params)
	{
		try
		{
			auto_ptr<AlgorithmParameterSpec> tmp(params->getParameterSpec((opmode == Cipher::DECRYPT_MODE) ? typeid(DHIESDecryptParameterSpec) : typeid(DHIESParameterSpec)));

			engineInit(opmode, key, *tmp.get(), random);
		}
		catch (InvalidParameterSpecException& e)
		{
			throw InvalidAlgorithmParameterException().initCause(e);
		}
	}
	else
		engineInit(opmode, key, random);
}

void DHIESCipher::engineInit(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom *random) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
	const DHIESParameterSpec* spec;
	const DHIESDecryptParameterSpec* dspec = 0;

	spec = dynamic_cast<const DHIESParameterSpec*>(&params);
	if (!spec)
		throw InvalidAlgorithmParameterException("not a DHIESParameterSpec");

	if (opmode == Cipher::DECRYPT_MODE)
	{
		dspec = dynamic_cast<const DHIESDecryptParameterSpec*>(spec);
		if (!dspec)
			throw InvalidAlgorithmParameterException("not a DHIESParameterSpec");
	}

	delete _spec;
	delete _dspec;
	delete _d;
	delete _c;
	delete _m;

	_spec = 0;
	_dspec = 0;
	_d = 0;
	_c = 0;
	_m = 0;

	try
	{
		_d = MessageDigest::getInstance(spec->getMessageDigestAlgorithm());
		_c = Cipher::getInstance(spec->getCipherAlgorithm() + "/CBC/PKCS5Padding");
		_m = Mac::getInstance(spec->getMacAlgorithm());
	}
	catch (NoSuchAlgorithmException& e)
	{
		throw InvalidAlgorithmParameterException().initCause(e);
	}

	if (spec->getCipherKeyLength())
	{
		int keybits = _d->getDigestLength() << 3;

		if (spec->getCipherKeyLength() >= keybits)
			throw new InvalidAlgorithmParameterException("DHIESParameterSpec invalid: cipher key length must be less than digest size");

		if (spec->getMacKeyLength() >= keybits)
			throw new InvalidAlgorithmParameterException("DHIESParameterSpec invalid: mac key length must be less than digest size");

		if (spec->getCipherKeyLength() + spec->getMacKeyLength() > keybits)
			throw new InvalidAlgorithmParameterException("DHIESParameterSpec invalid: sum of cipher and mac key lengths exceeds digest size");
	}

	if (opmode == Cipher::ENCRYPT_MODE)
	{
		const DHPublicKey* pub = dynamic_cast<const DHPublicKey*>(&key);
		if (pub)
		{
			_enc = pub;
			_dec = 0;
			_buf = 0;
		}
		else
		{
			std::cout << "Not a DHPublicKey; algorithm = " << key.getAlgorithm() << std::endl;

			throw InvalidKeyException("not a DHPublicKey");
		}

		_spec = new DHIESParameterSpec(*spec);
		_dspec = 0;
	}
	else if (opmode == Cipher::DECRYPT_MODE)
	{
		const DHPrivateKey* pri = dynamic_cast<const DHPrivateKey*>(&key);
		if (pri)
		{
			_enc = 0;
			_dec = pri;
			_buf = new ByteArrayOutputStream();
		}
		else
			throw InvalidKeyException("DHPrivateKey expected when decrypting");

		_spec = new DHIESParameterSpec(*spec);
		_dspec = new DHIESDecryptParameterSpec(*dspec);
	}
	else
		throw ProviderException("unsupported opmode");

	_opmode = opmode;
	_srng = random;

	reset();
}

bytearray* DHIESCipher::engineUpdate(const byte* input, int inputOffset, int inputLength)
{
	if (_buf)
	{
		_buf->write(input, inputOffset, inputLength);

		return 0;
	}
	else
	{
		bytearray* tmp = _c->update(input, inputOffset, inputLength);
		if (tmp)
			_m->update(*tmp);

		return tmp;
	}
}

int DHIESCipher::engineUpdate(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException)
{
	if (_buf)
	{
		_buf->write(input, inputOffset, inputLength);

		return 0;
	}
	else
	{
		int tmp = _c->update(input, inputOffset, inputLength, output, outputOffset);
		if (tmp)
			_m->update(output.data(), outputOffset, tmp);

		return tmp;
	}
}

void DHIESCipher::engineSetMode(const String& mode) throw (NoSuchAlgorithmException)
{
	throw NoSuchAlgorithmException();
}

void DHIESCipher::engineSetPadding(const String& padding) throw (NoSuchPaddingException)
{
	throw NoSuchPaddingException();
}

void DHIESCipher::reset()
{
	delete _msg;

	try
	{
		if (_buf)
		{
			_msg = new DHPublicKeyImpl(_dec->getParams(), _dspec->getEphemeralPublicKey());

			_ka->init(*_dec, _srng);
			_ka->doPhase(*_msg, true);
		}
		else
		{
			// generate an ephemeral keypair
			_kpg->initialize(DHParameterSpec(_enc->getParams()), _srng);

			try
			{
				auto_ptr<KeyPair> pair(_kpg->generateKeyPair());

				_msg = new DHPublicKeyImpl(dynamic_cast<const DHPublicKey&>(pair->getPublic()));

				_ka->init(pair->getPrivate(), _srng);
				_ka->doPhase(*_enc, true);
			}
			catch (Exception&)
			{
				throw ProviderException();
			}
		}

		bytearray tmp;

		_msg->getY().toByteArray(tmp);

		_d->reset();
		_d->update(tmp);
		_d->update(*_ka->generateSecret());

		bytearray key(_d->getDigestLength());

		_d->digest(key.data(), 0, key.size());

		int cl = _spec->getCipherKeyLength() >> 3, ml = _spec->getMacKeyLength() >> 3;
		SecretKeySpec *cipherKeySpec, *macKeySpec;

		if (cl == 0 && ml == 0)
		{
			// both key lengths are zero; divide available key in two equal halves
			int half = key.size() >> 1;

			cipherKeySpec = new SecretKeySpec(key.data(), 0, half, "RAW");
			macKeySpec = new SecretKeySpec(key.data(), half, half, "RAW");
		}
		else if (cl == 0)
		{
			throw InvalidAlgorithmParameterException("if cipherKeyLength equals 0, then macKeyLength must also be 0");
		}
		else if (ml == 0)
		{
			if (cl >= key.size())
				throw InvalidAlgorithmParameterException("requested key size for cipher exceeds total key size");

			cipherKeySpec = new SecretKeySpec(key.data(), 0, cl, "RAW");
			macKeySpec = new SecretKeySpec(key.data(), cl, key.size() - cl, "RAW");
		}
		else
		{
			if ((cl + ml) > key.size())
				throw InvalidAlgorithmParameterException("requested key sizes for cipher and mac exceed total key size");

			cipherKeySpec = new SecretKeySpec(key.data(), 0, cl, "RAW");
			macKeySpec = new SecretKeySpec(key.data(), cl, ml, "RAW");
		}

		try
		{
			// first try initializing the Cipher with the SecretKeySpec
			_c->init(_opmode, *cipherKeySpec, _srng);
		}
		catch (InvalidKeyException&)
		{
			// on failure, let's see if there's a SecretKeyFactory for the Cipher
			try
			{
				auto_ptr<SecretKeyFactory> skf(SecretKeyFactory::getInstance(_c->getAlgorithm()));
				auto_ptr<SecretKey> s(skf->generateSecret(*cipherKeySpec));

				_c->init(_opmode, *s.get(), _srng);
			}
			catch (Exception& e)
			{
				throw InvalidKeyException().initCause(e);
			}
		}

		try
		{
			// first try initializing the Mac with the SecretKeySpec
			_m->init(*macKeySpec);
		}
		catch (InvalidKeyException&)
		{
			// on failure, let's see if there's a SecretKeyFactory for the Mac
			try
			{
				auto_ptr<SecretKeyFactory> skf(SecretKeyFactory::getInstance(_m->getAlgorithm()));
				auto_ptr<SecretKey> s(skf->generateSecret(*macKeySpec));

				_m->init(*s.get());
			}
			catch (Exception& e)
			{
				throw InvalidKeyException().initCause(e);
			}
		}
	}
	catch (InvalidAlgorithmParameterException& e)
	{
		throw ProviderException().initCause(e);
	}
}
