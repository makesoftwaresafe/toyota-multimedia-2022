/*
 * Copyright (c) 2004 X-Way Rights BV
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

/*!\file BeeKeyStore.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_BEEKEYSTORE_H
#define _CLASS_BEEKEYSTORE_H

#ifdef __cplusplus

#include "beecrypt/c++/security/KeyStoreSpi.h"
using beecrypt::security::KeyStoreSpi;
#include "beecrypt/c++/security/KeyFactory.h"
using beecrypt::security::KeyFactory;
#include "beecrypt/c++/security/cert/CertificateFactory.h"
using beecrypt::security::cert::CertificateFactory;
#include "beecrypt/c++/util/Enumeration.h"
using beecrypt::util::Enumeration;
#include "beecrypt/c++/util/Hashtable.h"
using beecrypt::util::Hashtable;

namespace beecrypt {
	namespace provider {
		/*!\brief The default BeeCrypt KeyStore.
		 * \ingroup CXX_PROVIDER_m
		 */
		class BeeKeyStore : public KeyStoreSpi
		{
		private:
			class Entry : public beecrypt::lang::Object
			{
			public:
				Date date;
				virtual ~Entry() {}
			};

			class KeyEntry : public Entry
			{
			public:
				bytearray encryptedkey;
				array<Certificate*> chain;

				KeyEntry() throw ();
				KeyEntry(const bytearray& key, const array<Certificate*>&) throw (CloneNotSupportedException);
				virtual ~KeyEntry();
			};

			class CertEntry : public Entry
			{
			public:
				Certificate* cert;

				CertEntry() throw ();
				CertEntry(const Certificate&) throw (CloneNotSupportedException);
				virtual ~CertEntry();
			};

			class Names : public beecrypt::lang::Object, public virtual beecrypt::util::Enumeration<const String>
			{
			private:
				array<String*> _list;
				int _pos;

			public:
				Names(Hashtable<String,Entry>& h);
				virtual ~Names();

				virtual bool hasMoreElements() throw ();
				virtual const String* nextElement() throw (NoSuchElementException);
			};

			#if 0
			typedef map<String, KeyFactory*> keyfactory_map;
			keyfactory_map _keyfactories;

			typedef map<String, CertificateFactory*> certfactory_map;
			certfactory_map _certfactories;

			typedef map<String, Entry*> entry_map;
			entry_map _entries;

			class AliasEnum : public beecrypt::util::Enumeration<const String>
			{
			private:
				entry_map::const_iterator _it;
				entry_map::const_iterator _end;

			public:
				AliasEnum(const entry_map&);
				virtual ~AliasEnum() throw ();

				virtual bool hasMoreElements() throw ();
				virtual const String* nextElement() throw (NoSuchElementException);
			};

			void clearall();
			#else
			#endif

		private:
			bytearray _bmac;
			bytearray _salt;
			int _iter;

			Hashtable<String,Entry> _entries;

		protected:
			virtual Enumeration<const String>* engineAliases();

			virtual bool engineContainsAlias(const String& alias);

			virtual void engineDeleteEntry(const String& alias) throw (KeyStoreException);
			virtual const Date* engineGetCreationDate(const String& alias);

			virtual const Certificate* engineGetCertificate(const String& alias);
			virtual const String* engineGetCertificateAlias(const Certificate& cert);
			virtual const array<Certificate*>* engineGetCertificateChain(const String& alias);
			virtual bool engineIsCertificateEntry(const String& alias);
			virtual void engineSetCertificateEntry(const String& alias, const Certificate& cert) throw (KeyStoreException);

			virtual Key* engineGetKey(const String& alias, const array<jchar>& password) throw (NoSuchAlgorithmException, UnrecoverableKeyException);
			virtual bool engineIsKeyEntry(const String& alias);
			virtual void engineSetKeyEntry(const String& alias, const bytearray& key, const array<Certificate*>&) throw (KeyStoreException);
			virtual void engineSetKeyEntry(const String& alias, const Key& key, const array<jchar>& password, const array<Certificate*>&) throw (KeyStoreException);

			virtual int engineSize() const;

			virtual void engineLoad(InputStream* in, const array<jchar>* password) throw (IOException, CertificateException, NoSuchAlgorithmException);
			virtual void engineStore(OutputStream& out, const array<jchar>* password) throw (IOException, CertificateException, NoSuchAlgorithmException);

		public:
			BeeKeyStore();
			virtual ~BeeKeyStore() {}
		};
	}
}

#endif

#endif
