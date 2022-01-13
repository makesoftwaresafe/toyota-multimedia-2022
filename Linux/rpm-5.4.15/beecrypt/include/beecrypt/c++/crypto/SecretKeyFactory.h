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

/*!\file SecretKeyFactory.h
 * \ingroup CXX_CRYPTO_m
 */

#ifndef _CLASS_BEE_CRYPTO_SECRETKEYFACTORY_H
#define _CLASS_BEE_CRYPTO_SECRETKEYFACTORY_H

#ifdef __cplusplus

#include "beecrypt/c++/crypto/SecretKeyFactorySpi.h"
using beecrypt::crypto::SecretKeyFactorySpi;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/NoSuchProviderException.h"
using beecrypt::security::NoSuchProviderException;

namespace beecrypt {
	namespace crypto {
		/*!\ingroup CXX_CRYPTO_m
		 */
		class BEECRYPTCXXAPI SecretKeyFactory : public Object
		{
		public:
			static SecretKeyFactory* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
			static SecretKeyFactory* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static SecretKeyFactory* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

		private:
			SecretKeyFactorySpi* _kspi;
			const Provider*      _prov;
			String               _algo;

		protected:
			SecretKeyFactory(SecretKeyFactorySpi* spi, const Provider* provider, const String& algorithm);

		public:
			virtual ~SecretKeyFactory();

			SecretKey* generateSecret(const KeySpec&) throw (InvalidKeySpecException);

			KeySpec* getKeySpec(const SecretKey& key, const type_info&) throw (InvalidKeySpecException);

			SecretKey* translateKey(const SecretKey&) throw (InvalidKeyException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
