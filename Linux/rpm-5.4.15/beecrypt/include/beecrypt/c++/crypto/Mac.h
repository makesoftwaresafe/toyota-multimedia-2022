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

/*!\file Mac.h
 * \ingroup CXX_CRYPTO_m
 */

#ifndef _CLASS_BEE_CRYPTO_MAC_H
#define _CLASS_BEE_CRYPTO_MAC_H

#ifdef __cplusplus

#include "beecrypt/c++/crypto/MacSpi.h"
using beecrypt::crypto::MacSpi;
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
		class BEECRYPTCXXAPI Mac : public Object
		{
		public:
			static Mac* getInstance(const String&) throw (NoSuchAlgorithmException);
			static Mac* getInstance(const String&, const String&) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static Mac* getInstance(const String&, const Provider&) throw (NoSuchAlgorithmException);

		private:
			MacSpi*         _mspi;
			String          _algo;
			const Provider* _prov;
			bool            _init;

		protected:
			Mac(MacSpi* macSpi, const Provider* provider, const String& algorithm);

		public:
			virtual ~Mac();

			Mac* clone() const throw ();

			const bytearray& doFinal() throw (IllegalStateException);
			const bytearray& doFinal(const bytearray&) throw (IllegalStateException);
			int doFinal(byte* data, int offset, int length) throw (IllegalStateException, ShortBufferException);
			int getMacLength();
			void init(const Key&) throw (InvalidKeyException);
			void init(const Key&, const AlgorithmParameterSpec*) throw (InvalidKeyException, InvalidAlgorithmParameterException);
			void reset();
			void update(byte) throw (IllegalStateException);
			void update(const byte* data, int offset, int length) throw (IllegalStateException);
			void update(const bytearray&) throw (IllegalStateException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
