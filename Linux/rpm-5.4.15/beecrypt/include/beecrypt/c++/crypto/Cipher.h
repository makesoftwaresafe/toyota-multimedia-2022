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

/*!\file Cipher.h
 * \ingroup CXX_CRYPTO_m
 */

#ifndef _CLASS_BEE_CRYPTO_CIPHER_H
#define _CLASS_BEE_CRYPTO_CIPHER_H

#ifdef __cplusplus

#include "beecrypt/c++/crypto/CipherSpi.h"
using beecrypt::crypto::CipherSpi;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/cert/Certificate.h"
using beecrypt::security::cert::Certificate;

namespace beecrypt {
	namespace crypto {
		/*!\ingroup CXX_CRYPTO_m
		 */
		class BEECRYPTCXXAPI Cipher : public Object
		{
		public:
			static Cipher* getInstance(const String& transformation) throw (NoSuchAlgorithmException, NoSuchPaddingException);
			static Cipher* getInstance(const String& transformation, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException, NoSuchPaddingException);
			static Cipher* getInstance(const String& transformation, const Provider& provider) throw (NoSuchAlgorithmException, NoSuchPaddingException);

			static const int ENCRYPT_MODE;
			static const int DECRYPT_MODE;
			static const int WRAP_MODE;
			static const int UNWRAP_MODE;

			static int getMaxAllowedKeyLength(const String& transformation) throw (NoSuchAlgorithmException);
			static AlgorithmParameterSpec* getMaxAllowedParameterSpec(const String& transformation) throw (NoSuchAlgorithmException);

		private:
			CipherSpi*      _cspi;
			String          _algo;
			const Provider* _prov;
			bool            _init;

		protected:
			Cipher(CipherSpi* cipherSpi, const Provider* provider, const String& transformation);

		public:
			virtual ~Cipher();

			bytearray* doFinal() throw (IllegalStateException, IllegalBlockSizeException, BadPaddingException);
			bytearray* doFinal(const bytearray& input) throw (IllegalStateException, IllegalBlockSizeException, BadPaddingException);
			int doFinal(bytearray& output, int outputOffset) throw (IllegalStateException, IllegalBlockSizeException, ShortBufferException, BadPaddingException);
			bytearray* doFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalStateException, IllegalBlockSizeException, BadPaddingException);
			int doFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset = 0) throw (IllegalStateException, IllegalBlockSizeException, ShortBufferException, BadPaddingException);
//			virtual int doFinal(ByteBuffer& input, ByteBuffer& output) throw (IllegalStateException, ShortBufferException, BadPaddingException);

			int getBlockSize() const throw ();
			int getKeySize() const throw ();
			int getOutputSize(int inputLength) throw ();
			AlgorithmParameters* getParameters() throw ();

			bytearray* getIV();

			void init(int opmode, const Certificate& certificate, SecureRandom* random = 0) throw (InvalidKeyException);
			void init(int opmode, const Key& key, SecureRandom* random = 0) throw (InvalidKeyException);
			void init(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random = 0) throw (InvalidKeyException, InvalidAlgorithmParameterException);
			void init(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom* random = 0) throw (InvalidKeyException, InvalidAlgorithmParameterException);

			bytearray* update(const bytearray& input) throw (IllegalStateException);
			bytearray* update(const byte* input, int inputOffset, int inputLength) throw (IllegalStateException);
			int update(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset = 0) throw (IllegalStateException, ShortBufferException);
//			int update(ByteBuffer& input, ByteBuffer& output) throw (IllegalStateException, ShortBufferException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
