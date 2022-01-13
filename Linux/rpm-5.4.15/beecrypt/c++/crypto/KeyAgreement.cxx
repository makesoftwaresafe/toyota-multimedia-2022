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

#include "beecrypt/c++/crypto/KeyAgreement.h"
#include "beecrypt/c++/security/Security.h"
using beecrypt::security::Security;

using namespace beecrypt::crypto;

KeyAgreement::KeyAgreement(KeyAgreementSpi* spi, const Provider* provider, const String& algorithm)
{
	_kspi = spi;
	_prov = provider;
	_algo = algorithm;
}

KeyAgreement::~KeyAgreement()
{
	delete _kspi;
}

KeyAgreement* KeyAgreement::getInstance(const String& algorithm) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "KeyAgreement");

	assert(dynamic_cast<KeyAgreementSpi*>((KeyAgreementSpi*) tmp->cspi));

	KeyAgreement* result = new KeyAgreement((KeyAgreementSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

KeyAgreement* KeyAgreement::getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "KeyAgreement", provider);

	assert(dynamic_cast<KeyAgreementSpi*>((KeyAgreementSpi*) tmp->cspi));

	KeyAgreement* result = new KeyAgreement((KeyAgreementSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

KeyAgreement* KeyAgreement::getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "KeyAgreement", provider);

	assert(dynamic_cast<KeyAgreementSpi*>((KeyAgreementSpi*) tmp->cspi));

	KeyAgreement* result = new KeyAgreement((KeyAgreementSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

void KeyAgreement::init(const Key& key, SecureRandom* random) throw (InvalidKeyException)
{
	_kspi->engineInit(key, random);
}

void KeyAgreement::init(const Key& key, const AlgorithmParameterSpec& spec, SecureRandom* random) throw (InvalidKeyException)
{
	_kspi->engineInit(key, spec, random);
}

Key* KeyAgreement::doPhase(const Key& key, bool lastPhase) throw (InvalidKeyException, IllegalStateException)
{
	return _kspi->engineDoPhase(key, lastPhase);
}

bytearray* KeyAgreement::generateSecret() throw (IllegalStateException)
{
	return _kspi->engineGenerateSecret();
}

int KeyAgreement::generateSecret(bytearray& sharedSecret, int offset) throw (IllegalStateException, ShortBufferException)
{
	return _kspi->engineGenerateSecret(sharedSecret, offset);
}

SecretKey* KeyAgreement::generateSecret(const String& algorithm) throw (IllegalStateException, NoSuchAlgorithmException, InvalidKeyException)
{
	return _kspi->engineGenerateSecret(algorithm);
}
