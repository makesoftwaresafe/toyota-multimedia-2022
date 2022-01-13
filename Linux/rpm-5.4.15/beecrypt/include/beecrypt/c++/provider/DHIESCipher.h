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

/*!\file DHIESCipher.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_DHIESCIPHER_H
#define _CLASS_DHIESCIPHER_H

#ifdef __cplusplus

#include "beecrypt/c++/crypto/Cipher.h"
using beecrypt::crypto::Cipher;
#include "beecrypt/c++/crypto/CipherSpi.h"
using beecrypt::crypto::CipherSpi;
#include "beecrypt/c++/crypto/KeyAgreement.h"
using beecrypt::crypto::KeyAgreement;
#include "beecrypt/c++/crypto/Mac.h"
using beecrypt::crypto::Mac;
#include "beecrypt/c++/crypto/interfaces/DHPrivateKey.h"
using beecrypt::crypto::interfaces::DHPrivateKey;
#include "beecrypt/c++/crypto/interfaces/DHPublicKey.h"
using beecrypt::crypto::interfaces::DHPublicKey;
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/security/KeyPairGenerator.h"
using beecrypt::security::KeyPairGenerator;
#include "beecrypt/c++/security/MessageDigest.h"
using beecrypt::security::MessageDigest;
#include "beecrypt/c++/beeyond/DHIESDecryptParameterSpec.h"
using beecrypt::beeyond::DHIESDecryptParameterSpec;

namespace beecrypt {
	namespace provider {
		class DHIESCipher : public CipherSpi
		{
		private:
			DHIESDecryptParameterSpec* _dspec;
			DHIESParameterSpec*        _spec;

			SecureRandom*     _srng;
			KeyPairGenerator* _kpg;
			KeyAgreement*     _ka;
			MessageDigest*    _d;
			Cipher*           _c;
			Mac*              _m;

			DHPublicKey* _msg;

			ByteArrayOutputStream* _buf;

			int _opmode;

			const DHPublicKey*  _enc;
			const DHPrivateKey* _dec;

			void reset();

		protected:
			virtual bytearray* engineDoFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalBlockSizeException, BadPaddingException);
			virtual int engineDoFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException);

			virtual int engineGetBlockSize() const throw ();
			virtual bytearray* engineGetIV();
			virtual int engineGetOutputSize(int inputLength) throw ();
			virtual AlgorithmParameters* engineGetParameters() throw ();

			virtual void engineInit(int opmode, const Key& key, SecureRandom* random) throw (InvalidKeyException);
			virtual void engineInit(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException);
			virtual void engineInit(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException);

			virtual bytearray* engineUpdate(const byte* input, int inputOffset, int inputLength);
			virtual int engineUpdate(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException);

            virtual void engineSetMode(const String& mode) throw (NoSuchAlgorithmException);
            virtual void engineSetPadding(const String& padding) throw (NoSuchPaddingException);

		public:
			DHIESCipher();
			virtual ~DHIESCipher();
		};
	}
}

#endif

#endif
