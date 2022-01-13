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

/*!\file KeyFactory.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_KEYFACTORY_H
#define _CLASS_BEE_SECURITY_KEYFACTORY_H

#ifdef __cplusplus

#include "beecrypt/c++/security/KeyFactorySpi.h"
using beecrypt::security::KeyFactorySpi;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/NoSuchProviderException.h"
using beecrypt::security::NoSuchProviderException;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI KeyFactory : public Object
		{
		public:
			static KeyFactory* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
			static KeyFactory* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static KeyFactory* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

		private:
			KeyFactorySpi*  _kspi;
			const Provider* _prov;
			String          _algo;

		protected:
			KeyFactory(KeyFactorySpi* spi, const Provider* provider, const String& algorithm);

		public:
			virtual ~KeyFactory();

			PrivateKey* generatePrivate(const KeySpec& spec) throw (InvalidKeySpecException);
			PublicKey* generatePublic(const KeySpec& spec) throw (InvalidKeySpecException);

			KeySpec* getKeySpec(const Key& key, const type_info& info) throw (InvalidKeySpecException);

			Key* translateKey(const Key& key) throw (InvalidKeyException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
