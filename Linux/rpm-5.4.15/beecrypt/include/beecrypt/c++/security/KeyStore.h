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

/*!\file KeyStore.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_KEYSTORE_H
#define _CLASS_BEE_SECURITY_KEYSTORE_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::array;
#include "beecrypt/c++/security/KeyStoreSpi.h"
using beecrypt::security::KeyStoreSpi;
#include "beecrypt/c++/security/PrivateKey.h"
using beecrypt::security::PrivateKey;
#include "beecrypt/c++/crypto/SecretKey.h"
using beecrypt::crypto::SecretKey;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/auth/Destroyable.h"
using beecrypt::security::auth::Destroyable;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI KeyStore : public Object
		{
		public:
			class BEECRYPTCXXAPI ProtectionParameter
			{
			public:
				virtual ~ProtectionParameter() {}
			};

			class BEECRYPTCXXAPI LoadStoreParameter
			{
			public:
				virtual ~LoadStoreParameter() {}

				virtual KeyStore::ProtectionParameter* getProtectionParameter() = 0;
			};


			class BEECRYPTCXXAPI PasswordProtection : public beecrypt::lang::Object, public virtual ProtectionParameter, public virtual beecrypt::security::auth::Destroyable
			{
			private:
				array<jchar>* _pwd;
				bool _destroyed;

			public:
				PasswordProtection(const array<jchar>* password);
				virtual ~PasswordProtection();

				virtual void destroy() throw (DestroyFailedException);

				const array<jchar>* getPassword() const;

				virtual bool isDestroyed() const throw ();
			};

			class BEECRYPTCXXAPI Entry
			{
			public:
				virtual ~Entry() {}
			};

			class BEECRYPTCXXAPI PrivateKeyEntry : public beecrypt::lang::Object, public virtual beecrypt::security::KeyStore::Entry
			{
			private:
				PrivateKey* _pri;
				array<Certificate*> _chain;

			public:
				PrivateKeyEntry(PrivateKey* privateKey, const array<Certificate*>& chain);
				virtual ~PrivateKeyEntry();

				const Certificate& getCertificate() const;
				const array<Certificate*>& getCertificateChain() const;
				const PrivateKey& getPrivateKey() const;

				virtual String toString() const throw ();
			};

			class BEECRYPTCXXAPI SecretKeyEntry : public beecrypt::lang::Object, public virtual beecrypt::security::KeyStore::Entry
			{
			private:
				SecretKey* _sec;

			public:
				SecretKeyEntry(SecretKey* secretKey);
				virtual ~SecretKeyEntry();

				const SecretKey& getSecretKey() const;

				virtual String toString() const throw ();
			};

			class TrustedCertificateEntry : public beecrypt::lang::Object, public virtual beecrypt::security::KeyStore::Entry
			{
			private:
				Certificate* _cert;

			public:
				TrustedCertificateEntry(Certificate* cert);
				virtual ~TrustedCertificateEntry();

				const Certificate& getTrustedCertificate() const;

				virtual String toString() const throw ();
			};

		public:
			static KeyStore* getInstance(const String& type) throw (KeyStoreException);
			static KeyStore* getInstance(const String& type, const String& provider) throw (KeyStoreException, NoSuchProviderException);
			static KeyStore* getInstance(const String& type, const Provider& provider) throw (KeyStoreException);

			static const String& getDefaultType();

		private:
			KeyStoreSpi*    _kspi;
			const Provider* _prov;
			String          _type;
			bool            _init;

		protected:
			KeyStore(KeyStoreSpi* spi, const Provider* provider, const String& type);

		public:
			virtual ~KeyStore();

			Enumeration<const String>* aliases();
			bool containsAlias(const String& alias) throw (KeyStoreException);

			const Certificate* getCertificate(const String& alias) throw (KeyStoreException);
			const String* getCertificateAlias(const Certificate& cert) throw (KeyStoreException);
			const array<Certificate*>* getCertificateChain(const String& alias) throw (KeyStoreException);
			bool isCertificateEntry(const String& alias) throw (KeyStoreException);
			void setCertificateEntry(const String& alias, const Certificate& cert) throw (KeyStoreException);
				
			void deleteEntry(const String& alias) throw (KeyStoreException);

	//		KeyStore::Entry* getEntry(const String& alias, );
			Key* getKey(const String& alias, const array<jchar>& password) throw (KeyStoreException, NoSuchAlgorithmException, UnrecoverableKeyException);
			bool isKeyEntry(const String& alias) throw (KeyStoreException);
			/*!\brief Assigns the given key to the specified alias
			 * \warning A shallow copy of the chain array contents is made;
			 *  consider the contents absorbed by the entry, and hence the
			 *  certificate objects will be deleted in the entry destructor.
			 */
			void setKeyEntry(const String& alias, const bytearray& key, const array<Certificate*>& chain) throw (KeyStoreException);
			/*!\brief Assigns the given key to the specified alias
			 * \warning A shallow copy of the chain array contents is made;
			 *  consider the contents absorbed by the entry, and hence the
			 *  certificate objects will be deleted in the entry destructor.
			 */
			void setKeyEntry(const String& alias, const Key& key, const array<jchar>& password, const array<Certificate*>& chain) throw (KeyStoreException);

			jint size() const throw (KeyStoreException);

			void load(InputStream* in, const array<jchar>* password) throw (IOException, NoSuchAlgorithmException, CertificateException);
			void store(OutputStream& out, const array<jchar>* password) throw (KeyStoreException, IOException, NoSuchAlgorithmException, CertificateException);

			const String& getType() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
