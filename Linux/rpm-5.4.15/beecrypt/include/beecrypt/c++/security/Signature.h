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

/*!\file Signature.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_SIGNATURE_H
#define _CLASS_BEE_SECURITY_SIGNATURE_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/SignatureSpi.h"
using beecrypt::security::SignatureSpi;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/NoSuchProviderException.h"
using beecrypt::security::NoSuchProviderException;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI Signature : public Object
		{
		protected:
			static const int UNINITIALIZED = 0;
			static const int VERIFY = 1;
			static const int SIGN = 2;

		public:
			/*!\brief Returns a Signature object that implements the requested algorithm.
			 *
			 * If the default provider has an implementation of the requested
			 * algorithm that one is used; otherwise other providers are
			 * searched.
			 *
			 * \param algorithm the standard name of the requested algorithm.
			 * \throw NoSuchAlgorithmException if the requested algorithm is not available.
			 */
			static Signature* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
			/*!\brief Returns a Signature object that implements the requested algorithm, from the requested provider.
			 *
			 * \param algorithm the standard algorithm name.
			 * \param provider the name of the provider.
			 * \throw NoSuchAlgorithmException if the requested algorithm is not available in the requested provider.
			 * \throw NoSuchProviderException if the requested provider is not available.
			 */
			static Signature* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static Signature* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

		protected:
			int state;

		private:
			SignatureSpi*   _sspi;
			const Provider* _prov;
			String          _algo;

		protected:
			Signature(SignatureSpi* spi, const Provider* provider, const String& algorithm);

		public:
			virtual ~Signature();

			AlgorithmParameters* getParameters() const;
			void setParameter(const AlgorithmParameterSpec&) throw (InvalidAlgorithmParameterException);

			void initSign(const PrivateKey&) throw (InvalidKeyException);
			void initSign(const PrivateKey&, SecureRandom*) throw (InvalidKeyException);

			void initVerify(const PublicKey&) throw (InvalidKeyException);

			bytearray* sign() throw (IllegalStateException, SignatureException);
			int sign(byte*, int, int) throw (ShortBufferException, IllegalStateException, SignatureException);
			int sign(bytearray&) throw (IllegalStateException, SignatureException);
			bool verify(const bytearray&) throw (IllegalStateException, SignatureException);
			bool verify(const byte*, int, int) throw (IllegalStateException, SignatureException);

			void update(byte) throw (IllegalStateException);
			void update(const byte*, int, int) throw (IllegalStateException);
			void update(const bytearray&) throw (IllegalStateException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
