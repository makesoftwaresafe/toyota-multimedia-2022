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

/*!\file CipherSpi.h
 * \ingroup CXX_CRYPTO_m
 */

#ifndef _CLASS_BEE_CRYPTO_CIPHERSPI_H
#define _CLASS_BEE_CRYPTO_CIPHERSPI_H

#include "beecrypt/api.h"

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::array;
#include "beecrypt/c++/crypto/BadPaddingException.h"
using beecrypt::crypto::BadPaddingException;
#include "beecrypt/c++/crypto/IllegalBlockSizeException.h"
using beecrypt::crypto::IllegalBlockSizeException;
#include "beecrypt/c++/crypto/NoSuchPaddingException.h"
using beecrypt::crypto::NoSuchPaddingException;
#include "beecrypt/c++/lang/IllegalStateException.h"
using beecrypt::lang::IllegalStateException;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/AlgorithmParameters.h"
using beecrypt::security::AlgorithmParameters;
#include "beecrypt/c++/security/InvalidAlgorithmParameterException.h"
using beecrypt::security::InvalidAlgorithmParameterException;
#include "beecrypt/c++/security/InvalidKeyException.h"
using beecrypt::security::InvalidKeyException;
#include "beecrypt/c++/security/Key.h"
using beecrypt::security::Key;
#include "beecrypt/c++/security/SecureRandom.h"
using beecrypt::security::SecureRandom;
#include "beecrypt/c++/security/ShortBufferException.h"
using beecrypt::security::ShortBufferException;

namespace beecrypt {
	namespace crypto {
		/*!\ingroup CXX_CRYPTO_m
		 */
		class BEECRYPTCXXAPI CipherSpi : public Object
		{
			friend class Cipher;

		protected:
			virtual bytearray* engineDoFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalBlockSizeException, BadPaddingException) = 0;
			virtual int engineDoFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException) = 0;
//			virtual int engineDoFinal(ByteBuffer& input, ByteBuffer& output) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException) = 0;

			virtual int engineGetBlockSize() const throw () = 0;
			virtual bytearray* engineGetIV() = 0;
			virtual int engineGetKeySize(const Key& key) const throw (InvalidKeyException);
			virtual int engineGetOutputSize(int inputLength) throw () = 0;
			virtual AlgorithmParameters* engineGetParameters() throw () = 0;

			virtual void engineInit(int opmode, const Key& key, SecureRandom* random) throw (InvalidKeyException) = 0;
			virtual void engineInit(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException) = 0;
			virtual void engineInit(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException) = 0;

			virtual void engineSetMode(const String& mode) throw (NoSuchAlgorithmException) = 0;
			virtual void engineSetPadding(const String& padding) throw (NoSuchPaddingException) = 0;

//			virtual Key* engineUnwrap(const bytearray& wrappedKey, const String& wrappedKeyAlgorithm, int wrappedKeyType) throw (InvalidKeyException, NoSuchAlgorithmException) = 0;

			virtual bytearray* engineUpdate(const byte* input, int inputOffset, int inputLength) = 0;
			virtual int engineUpdate(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException) = 0;
//			virtual int engineUpdate(ByteBuffer& input, ByteBuffer& output) throw (ShortBufferException) = 0;

//			virtual bytearray* engineWrap(const Key& key) throw (IllegalBlockSizeException, InvalidKeyException) = 0;

		public:
			virtual ~CipherSpi() {}
		};
	}
}

#endif

#endif
