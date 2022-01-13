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

#include "beecrypt/aes.h"
#include "beecrypt/pkcs12.h"
#include "beecrypt/sha256.h"

#include "beecrypt/c++/crypto/Mac.h"
using beecrypt::crypto::Mac;
#include "beecrypt/c++/crypto/MacInputStream.h"
using beecrypt::crypto::MacInputStream;
#include "beecrypt/c++/crypto/MacOutputStream.h"
using beecrypt::crypto::MacOutputStream;
#include "beecrypt/c++/io/ByteArrayInputStream.h"
using beecrypt::io::ByteArrayInputStream;
#include "beecrypt/c++/io/DataInputStream.h"
using beecrypt::io::DataInputStream;
#include "beecrypt/c++/io/DataOutputStream.h"
using beecrypt::io::DataOutputStream;
#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/security/SecureRandom.h"
using beecrypt::security::SecureRandom;
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;
#include "beecrypt/c++/beeyond/BeeCertificate.h"
using beecrypt::beeyond::BeeCertificate;
#include "beecrypt/c++/beeyond/PKCS12PBEKey.h"
using beecrypt::beeyond::PKCS12PBEKey;
#include "beecrypt/c++/provider/KeyProtector.h"
using beecrypt::provider::KeyProtector;
#include "beecrypt/c++/provider/BeeKeyStore.h"

#include <memory>
using std::auto_ptr;

using namespace beecrypt::provider;

namespace {
	const array<jchar> EMPTY_PASSWORD;
}

#define BKS_MAGIC				((jint) 0xbeecceec)
#define BKS_VERSION_1			((jint) 0x1)
#define BKS_PRIVATEKEY_ENTRY	((jint) 0x1)
#define BKS_CERTIFICATE_ENTRY	((jint) 0x2)

BeeKeyStore::Names::Names(Hashtable<String,Entry>& h) : _list(h.size())
{
	Iterator<class Map<String,Entry>::Entry>* it = h.entrySet().iterator();
	assert(it != 0);
	_pos = 0;
	while (it->hasNext())
	{
		_list[_pos++] = new String(*it->next()->getKey());
	}
	_pos = 0;
}

BeeKeyStore::Names::~Names()
{
	for (int i = 0; i < _list.size(); i++)
		delete _list[i];
}

bool BeeKeyStore::Names::hasMoreElements() throw ()
{
	return _pos < _list.size();
}

const String* BeeKeyStore::Names::nextElement() throw (NoSuchElementException)
{
	if (_pos >= _list.size())
		throw NoSuchElementException();

	return _list[_pos++];
}

BeeKeyStore::KeyEntry::KeyEntry() throw ()
{
}

BeeKeyStore::KeyEntry::KeyEntry(const bytearray& b, const array<Certificate*>& c) throw (CloneNotSupportedException) : chain(c)
{
	encryptedkey = b;
}

BeeKeyStore::KeyEntry::~KeyEntry()
{
	// delete all the certificates in the chain
	for (int i = 0; i < chain.size(); i++)
		delete chain[i];
}

BeeKeyStore::CertEntry::CertEntry() throw ()
{
	cert = 0;
}

BeeKeyStore::CertEntry::CertEntry(const Certificate& c) throw (CloneNotSupportedException)
{
	cert = BeeCertificate::cloneCertificate(c);
}

BeeKeyStore::CertEntry::~CertEntry()
{
	delete cert;
}

BeeKeyStore::BeeKeyStore()
{
}

Enumeration<const String>* BeeKeyStore::engineAliases()
{
	return new Names(_entries);
}

bool BeeKeyStore::engineContainsAlias(const String& alias)
{
	return _entries.containsKey(&alias);
}

void BeeKeyStore::engineDeleteEntry(const String& alias) throw (KeyStoreException)
{
	delete _entries.remove(&alias);
}

const Date* BeeKeyStore::engineGetCreationDate(const String& alias)
{
	const Date* result = 0;

	synchronized (this)
	{
		Entry* e = _entries.get(&alias);
		if (e)
			result = &e->date;
	}

	return result;
}

const Certificate* BeeKeyStore::engineGetCertificate(const String& alias)
{
	const Certificate* result = 0;

	synchronized (this)
	{
		Entry* e = _entries.get(&alias);
		if (e)
		{
			CertEntry* ce = dynamic_cast<CertEntry*>(e);
			if (ce)
			{
				result = ce->cert;
			}
			else
			{
				KeyEntry* ke = dynamic_cast<KeyEntry*>(e);
				if (ke)
					result = ke->chain[0];
			}
		}
	}

	return result;
}

const String* BeeKeyStore::engineGetCertificateAlias(const Certificate& cert)
{
	const String* result = 0;

	synchronized (this)
	{
		Iterator<class Map<String,Entry>::Entry>* it = _entries.entrySet().iterator();
		assert(it != 0);
		while (it->hasNext())
		{
			class Map<String,Entry>::Entry* e = it->next();

			const CertEntry* ce = dynamic_cast<const CertEntry*>(e->getValue());
			if (ce)
			{
				if (cert.equals(ce->cert))
				{
					result = e->getKey();
					break;
				}
			}
		}
		delete it;
	}

	return result;
}

const array<Certificate*>* BeeKeyStore::engineGetCertificateChain(const String& alias)
{
	const array<Certificate*>* result = 0;

	synchronized (this)
	{
		KeyEntry* ke = dynamic_cast<KeyEntry*>(_entries.get(&alias));
		if (ke)
			result = &ke->chain;
	}

	return result;
}

bool BeeKeyStore::engineIsCertificateEntry(const String& alias)
{
	return dynamic_cast<CertEntry*>(_entries.get(&alias)) != 0;
}

void BeeKeyStore::engineSetCertificateEntry(const String& alias, const Certificate& cert) throw (KeyStoreException)
{
	delete _entries.put(new String(alias), new CertEntry(cert));
}

Key* BeeKeyStore::engineGetKey(const String& alias, const array<jchar>& password) throw (NoSuchAlgorithmException, UnrecoverableKeyException)
{
	Key* result = 0;

	synchronized (this)
	{
		Entry* e = _entries.get(&alias);
		if (e)
		{
			KeyEntry* ke = dynamic_cast<KeyEntry*>(e);
			if (ke)
			{
				PKCS12PBEKey pbekey(password, &_salt, _iter);

				try
				{
					KeyProtector p(pbekey);

					result = p.recover(ke->encryptedkey);
				}
				catch (InvalidKeyException& e)
				{
					throw UnrecoverableKeyException().initCause(e);
				}
			}
		}
	}

	return result;
}

bool BeeKeyStore::engineIsKeyEntry(const String& alias)
{
	return dynamic_cast<KeyEntry*>(_entries.get(&alias)) != 0;
}

void BeeKeyStore::engineSetKeyEntry(const String& alias, const bytearray& key, const array<Certificate*>& chain) throw (KeyStoreException)
{
	delete _entries.put(new String(alias), new KeyEntry(key, chain));
}

void BeeKeyStore::engineSetKeyEntry(const String& alias, const Key& key, const array<jchar>& password, const array<Certificate*>& chain) throw (KeyStoreException)
{
	const PrivateKey* pri = dynamic_cast<const PrivateKey*>(&key);
	if (pri)
	{
		PKCS12PBEKey pbekey(password, &_salt, _iter);
		KeyProtector p(pbekey);

		bytearray* tmp = p.protect(*pri);
		if (tmp)
		{
			engineSetKeyEntry(alias, *tmp, chain);
			delete tmp;
		}
		else
			throw KeyStoreException("BeeKeyStore failed to protect key");
	}
	else
			throw KeyStoreException("BeeKeyStore only supports storing of PrivateKey objects");
}

int BeeKeyStore::engineSize() const
{
	return _entries.size();
}

void BeeKeyStore::engineLoad(InputStream* in, const array<jchar>* password) throw (IOException, CertificateException, NoSuchAlgorithmException)
{
	synchronized (this)
	{
		if (!in)
		{
			randomGeneratorContext rngc;

			/* salt size default is 64 bytes */
			_salt.resize(64);
			/* generate a new salt */
			randomGeneratorContextNext(&rngc, _salt.data(), _salt.size());
			/* set default iteration count */
			_iter = 1024;

			return;
		}

		auto_ptr<Mac> m(Mac::getInstance("HMAC-SHA-256"));

		MacInputStream mis(*in, *m.get());
		DataInputStream dis(mis);

		mis.on(false);

		jint magic = dis.readInt();
		jint version = dis.readInt();

		if (magic != BKS_MAGIC || version != BKS_VERSION_1)
			throw IOException("invalid BeeKeyStore format");

		_entries.clear();

		jint saltsize = dis.readInt();
		if (saltsize <= 0)
			throw IOException("invalid BeeKeyStore salt size");

		_salt.resize(saltsize);
		dis.readFully(_salt);

		_iter = dis.readInt();
		if (_iter <= 0)
			throw IOException("invalid BeeKeyStore iteration count");

		PKCS12PBEKey pbekey(password ? *password : EMPTY_PASSWORD, &_salt, _iter);

		m->init(pbekey);

		mis.on(true);

		jint entrycount = dis.readInt();

		if (entrycount <= 0)
			throw IOException("invalid BeeKeyStore entry count");

		for (jint i = 0; i < entrycount; i++)
		{
			String alias;

			switch (dis.readInt())
			{
			case BKS_PRIVATEKEY_ENTRY:
				{
					alias = dis.readUTF();

					auto_ptr<KeyEntry> e(new KeyEntry);

					e->date.setTime(dis.readLong());

					jint keysize = dis.readInt();

					if (keysize <= 0)
						throw IOException("invalid BeeKeyStore key length");

					e->encryptedkey.resize((int) keysize);

					dis.readFully(e->encryptedkey);

					jint certcount = dis.readInt();

					if (certcount <= 0)
						throw IOException("invalid BeeKeyStore certificate count");

					e->chain.resize(certcount);

					for (jint j = 0; j < certcount; j++)
					{
						String type = dis.readUTF();

						auto_ptr<CertificateFactory> cf(CertificateFactory::getInstance(type));

						jint certsize = dis.readInt();

						if (certsize <= 0)
							throw IOException("invalid BeeKeyStore certificate size");
					
						bytearray cert(certsize);

						dis.readFully(cert);

						ByteArrayInputStream bis(cert);

						e->chain[j] = cf->generateCertificate(bis);
					}

					delete _entries.put(new String(alias), e.release());
				}
				break;

			case BKS_CERTIFICATE_ENTRY:
				{
					alias = dis.readUTF();

					auto_ptr<CertEntry> e(new CertEntry);

					e->date.setTime(dis.readLong());

					String type = dis.readUTF();

					auto_ptr<CertificateFactory> cf(CertificateFactory::getInstance(type));

					jint certsize = dis.readInt();

					if (certsize <= 0)
						throw IOException("invalid BeeKeyStore certificate size");

					bytearray cert(certsize);

					dis.readFully(cert);

					ByteArrayInputStream bis(cert);

					e->cert = cf->generateCertificate(bis);

					delete _entries.put(new String(alias), e.release());
				}
				break;

			default:
				throw IOException("invalid BeeKeyStore entry tag");
			}
		}

		bytearray computed_mac, original_mac;

		mis.on(false);

		jint macsize = dis.available();
		if (macsize <= 0)
			throw IOException("invalid BeeKeyStore MAC size");

		computed_mac = m->doFinal();
		// we can safely cast, since we've excluded negative numbers
		if (macsize != computed_mac.size())
			throw IOException("BeeKeyStore has been tampered with, or password was incorrect; incorrect mac size");

		original_mac.resize(macsize);
		dis.readFully(original_mac);

		if (computed_mac != original_mac)
			throw IOException("BeeKeyStore has been tampered with, or password was incorrect; incorrect mac");
	}
}

void BeeKeyStore::engineStore(OutputStream& out, const array<jchar>* password) throw (IOException, CertificateException, NoSuchAlgorithmException)
{
	synchronized (this)
	{
		auto_ptr<Mac> m(Mac::getInstance("HMAC-SHA-256"));

		PKCS12PBEKey pbekey(password ? *password : EMPTY_PASSWORD, &_salt, _iter);

		m->init(pbekey);

		MacOutputStream mos(out, *m.get());
		DataOutputStream dos(mos);

		mos.on(false);
		dos.writeInt(BKS_MAGIC);
		dos.writeInt(BKS_VERSION_1);
		dos.writeInt(_salt.size());
		dos.write(_salt);
		dos.writeInt(_iter);
		mos.on(true);
		dos.writeInt(_entries.size());

		Iterator<class Map<String,Entry>::Entry>* it = _entries.entrySet().iterator();
		assert(it != 0);
		while (it->hasNext())
		{
			class Map<String,Entry>::Entry* e = it->next();

			const KeyEntry* ke = dynamic_cast<const KeyEntry*>(e->getValue());
			if (ke)
			{
				dos.writeInt(BKS_PRIVATEKEY_ENTRY);
				dos.writeUTF(*e->getKey());
				dos.writeLong(ke->date.getTime());
				dos.writeInt(ke->encryptedkey.size());
				dos.write(ke->encryptedkey);
				/* next do all the certificates for this key */
				dos.writeInt(ke->chain.size());
				for (int i = 0; i < ke->chain.size(); i++)
				{
					const Certificate* cert = ke->chain[i];

					dos.writeUTF(cert->getType());
					dos.writeInt(cert->getEncoded().size());
					dos.write(cert->getEncoded());
				}
				continue;
			}

			const CertEntry* ce = dynamic_cast<const CertEntry*>(e->getValue());
			if (ce)
			{
				dos.writeInt(BKS_CERTIFICATE_ENTRY);
				dos.writeUTF(*e->getKey());
				dos.writeLong(ce->date.getTime());
				dos.writeUTF(ce->cert->getType());
				dos.writeInt(ce->cert->getEncoded().size());
				dos.write(ce->cert->getEncoded());
				continue;
			}

			throw ProviderException("entry is neither KeyEntry nor CertEntry");
		}

		/* don't call close on a FilterOutputStream because the
		 * underlying stream still has to write data!
		 */
		dos.flush();
		mos.flush();

		out.write(m->doFinal());
		out.close();
	}
}
