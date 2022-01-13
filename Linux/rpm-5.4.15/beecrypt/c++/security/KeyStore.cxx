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

#include "beecrypt/c++/security/KeyStore.h"
#include "beecrypt/c++/security/Security.h"
#include "beecrypt/c++/lang/StringBuilder.h"
using beecrypt::lang::StringBuilder;
#include "beecrypt/c++/lang/IllegalArgumentException.h"
using beecrypt::lang::IllegalArgumentException;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::security;

KeyStore::PasswordProtection::PasswordProtection(const array<jchar>* password) : _pwd(password ? new array<jchar>(*password) : 0), _destroyed(false)
{
}

KeyStore::PasswordProtection::~PasswordProtection()
{
	try
	{
		destroy();
	}
	catch (DestroyFailedException&)
	{
	}
}

void KeyStore::PasswordProtection::destroy() throw (DestroyFailedException)
{
	synchronized (this)
	{
		if (!_destroyed)
		{
			_destroyed = true;
			if (_pwd)
			{
				_pwd->fill((jchar) ' ');

				delete _pwd;

				_pwd = 0;
			}
		}
	}
}

const array<jchar>* KeyStore::PasswordProtection::getPassword() const
{
	const array<jchar>* result = 0;

	synchronized (this)
	{
		if (_destroyed)
			throw IllegalStateException("password was destroyed");

		result = _pwd;
	}

	return result;
}

bool KeyStore::PasswordProtection::isDestroyed() const throw ()
{
	bool result = FALSE;

	synchronized (this)
	{
		result = _destroyed;
	}

	return result;
}

KeyStore::PrivateKeyEntry::PrivateKeyEntry(PrivateKey* privateKey, const array<Certificate*>& chain) : _pri(privateKey), _chain(chain)
{
	if (_pri == 0)
		throw NullPointerException("private key is null");

	if (_chain.size() == 0)
		throw IllegalArgumentException("chain must contain at least one certificate");

	for (int i = 0; i < _chain.size(); i++)
		if (_chain[i] == 0)
			throw IllegalArgumentException("chain contains null");

	for (int i = 1; i < _chain.size(); i++)
		if (!_chain[i]->getType().equals(_chain[0]->getType()))
			throw IllegalArgumentException("chain contains certificates of different types");

	if (!_pri->getAlgorithm().equals(_chain[0]->getPublicKey().getAlgorithm()))
		throw IllegalArgumentException("private key algorithm does not match public key algorithm in first certificate");
}

KeyStore::PrivateKeyEntry::~PrivateKeyEntry()
{
	delete _pri;

	for (int i = 0; i < _chain.size(); i++)
		delete _chain[i];
}

const Certificate& KeyStore::PrivateKeyEntry::getCertificate() const
{
	return *(_chain[0]);
}

const array<Certificate*>& KeyStore::PrivateKeyEntry::getCertificateChain() const
{
	return _chain;
}

const PrivateKey& KeyStore::PrivateKeyEntry::getPrivateKey() const
{
	return *_pri;
}

String KeyStore::PrivateKeyEntry::toString() const throw ()
{
	StringBuilder tmp("private key entry and certificate chain with ");

	tmp.append(_chain.size()).append(" elements:\r\n");

	for (int i = 0; i < _chain.size(); i++)
		tmp.append(_chain[i]->toString()).append("\r\n");

	return tmp.toString();
}

KeyStore::SecretKeyEntry::SecretKeyEntry(SecretKey* secretKey) : _sec(secretKey)
{
	if (_sec == 0)
		throw NullPointerException("secret key is null");
}

KeyStore::SecretKeyEntry::~SecretKeyEntry()
{
	delete _sec;
}

const SecretKey& KeyStore::SecretKeyEntry::getSecretKey() const
{
	return *_sec;
}

String KeyStore::SecretKeyEntry::toString() const throw ()
{
	return String("secret key entry with algorithm ") + _sec->getAlgorithm();
}

KeyStore::TrustedCertificateEntry::TrustedCertificateEntry(Certificate* cert) : _cert(cert)
{
	if (_cert == 0)
		throw NullPointerException("certificate is null");
}

KeyStore::TrustedCertificateEntry::~TrustedCertificateEntry()
{
	delete _cert;
}

const Certificate& KeyStore::TrustedCertificateEntry::getTrustedCertificate() const
{
	return *_cert;
}

String KeyStore::TrustedCertificateEntry::toString() const throw ()
{
	return String("trusted certificate entry:\r\n") + _cert->toString();
}

KeyStore::KeyStore(KeyStoreSpi* spi, const Provider* provider, const String& type)
{
	_kspi = spi;
	_prov = provider;
	_type = type;
	_init = false;
}

KeyStore::~KeyStore()
{
	delete _kspi;
}

KeyStore* KeyStore::getInstance(const String& type) throw (KeyStoreException)
{
	try
	{
		Security::spi* tmp = Security::getSpi(type, "KeyStore");

		assert(dynamic_cast<KeyStoreSpi*>(tmp->cspi));

		KeyStore* result = new KeyStore(reinterpret_cast<KeyStoreSpi*>(tmp->cspi), tmp->prov, tmp->name);

		delete tmp;

		return result;
	}
	catch (NoSuchAlgorithmException& e)
	{
		throw KeyStoreException().initCause(e);
	}
}

KeyStore* KeyStore::getInstance(const String& type, const String& provider) throw (KeyStoreException, NoSuchProviderException)
{
	try
	{
		Security::spi* tmp = Security::getSpi(type, "KeyStore", provider);

		assert(dynamic_cast<KeyStoreSpi*>(tmp->cspi));

		KeyStore* result = new KeyStore(reinterpret_cast<KeyStoreSpi*>(tmp->cspi), tmp->prov, tmp->name);

		delete tmp;

		return result;
	}
	catch (NoSuchAlgorithmException& e)
	{
		throw KeyStoreException().initCause(e);
	}
}

KeyStore* KeyStore::getInstance(const String& type, const Provider& provider) throw (KeyStoreException)
{
	try
	{
		Security::spi* tmp = Security::getSpi(type, "KeyStore", provider);

		assert(dynamic_cast<KeyStoreSpi*>(tmp->cspi));

		KeyStore* result = new KeyStore(reinterpret_cast<KeyStoreSpi*>(tmp->cspi), tmp->prov, tmp->name);

		delete tmp;

		return result;
	}
	catch (NoSuchAlgorithmException& e)
	{
		throw KeyStoreException().initCause(e);
	}
}

const String& KeyStore::getDefaultType()
{
	return Security::getKeyStoreDefault();
}

Key* KeyStore::getKey(const String& alias, const array<jchar>& password) throw (KeyStoreException, NoSuchAlgorithmException, UnrecoverableKeyException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineGetKey(alias, password);
}

void KeyStore::setKeyEntry(const String& alias, const bytearray& key, const array<Certificate*>& chain) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	if (chain.size() == 0)
		throw IllegalArgumentException();

	_kspi->engineSetKeyEntry(alias, key, chain);
}

void KeyStore::setKeyEntry(const String& alias, const Key& key, const array<jchar>& password, const array<Certificate*>& chain) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	if (chain.size() == 0)
		throw IllegalArgumentException("chain should contain at least one certificate");

	_kspi->engineSetKeyEntry(alias, key, password, chain);
}

Enumeration<const String>* KeyStore::aliases()
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineAliases();
}

bool KeyStore::containsAlias(const String& alias) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineContainsAlias(alias);
}

const Certificate* KeyStore::getCertificate(const String& alias) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineGetCertificate(alias);
}

const String* KeyStore::getCertificateAlias(const Certificate& cert) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineGetCertificateAlias(cert);
}

const array<Certificate*>* KeyStore::getCertificateChain(const String& alias) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineGetCertificateChain(alias);
}

void KeyStore::setCertificateEntry(const String& alias, const Certificate& cert) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	_kspi->engineSetCertificateEntry(alias, cert);
}

bool KeyStore::isCertificateEntry(const String& alias) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineIsCertificateEntry(alias);
}

bool KeyStore::isKeyEntry(const String& alias) throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineIsKeyEntry(alias);
}

void KeyStore::load(InputStream* in, const array<jchar>* password) throw (IOException, NoSuchAlgorithmException, CertificateException)
{
	_kspi->engineLoad(in, password);

	_init = true;
}

jint KeyStore::size() const throw (KeyStoreException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	return _kspi->engineSize();
}

void KeyStore::store(OutputStream& out, const array<jchar>* password) throw (KeyStoreException, IOException, NoSuchAlgorithmException, CertificateException)
{
	if (!_init)
		throw KeyStoreException("uninitialized keystore");

	_kspi->engineStore(out, password);
}

const String& KeyStore::getType() const throw ()
{
	return _type;
}

const Provider& KeyStore::getProvider() const throw ()
{
	return *_prov;
}
