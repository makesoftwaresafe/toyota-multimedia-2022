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

/*!\file Security.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_SECURITY_H
#define _CLASS_BEE_SECURITY_SECURITY_H

#ifdef __cplusplus

#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;
#include "beecrypt/c++/security/cert/CertificateFactory.h"
using beecrypt::security::cert::CertificateFactory;
#include "beecrypt/c++/security/cert/CertPathValidator.h"
using beecrypt::security::cert::CertPathValidator;
#include "beecrypt/c++/crypto/Cipher.h"
using beecrypt::crypto::Cipher;
#include "beecrypt/c++/crypto/KeyAgreement.h"
using beecrypt::crypto::KeyAgreement;
#include "beecrypt/c++/crypto/Mac.h"
using beecrypt::crypto::Mac;
#include "beecrypt/c++/crypto/SecretKeyFactory.h"
using beecrypt::crypto::SecretKeyFactory;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI Security
		{
			friend class AlgorithmParameterGenerator;
			friend class AlgorithmParameters;
			friend class ::CertificateFactory;
			friend class ::CertPathValidator;
			friend class ::Cipher;
			friend class ::KeyAgreement;
			friend class KeyFactory;
			friend class KeyPairGenerator;
			friend class KeyStore;
			friend class ::Mac;
			friend class MessageDigest;
			friend class ::SecretKeyFactory;
			friend class SecureRandom;
			friend class Signature;

		private:
			struct spi
			{
				Object* cspi;
				String name;
				const Provider* prov;

				spi(Object* cspi, const Provider*, const String&);
			};

			static spi* getSpi(const String& name, const String& type) throw (NoSuchAlgorithmException);
			static spi* getSpi(const String& algo, const String& type, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static spi* getSpi(const String& algo, const String& type, const Provider&) throw (NoSuchAlgorithmException);
			static spi* getFirstSpi(const String& type);

			static const String& getKeyStoreDefault();

			static bool _init;
			static Properties _props;
			static ArrayList<Provider> _providers;

			static void initialize();
				
		public:
			static int addProvider(Provider* provider);
			static int insertProviderAt(Provider* provider, int position) throw (IndexOutOfBoundsException);
			static void removeProvider(const String& name);
			static Provider* getProvider(const String& name);
			static array<Provider*> getProviders();
			static const String* getProperty(const String& key) throw ();
		};
	}
}

#endif

#endif
