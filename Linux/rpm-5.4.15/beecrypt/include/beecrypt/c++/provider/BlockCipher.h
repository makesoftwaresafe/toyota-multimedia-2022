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

/*!\file BlockCipher.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_BLOCKCIPHER_H
#define _CLASS_BLOCKCIPHER_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus

#include "beecrypt/c++/crypto/CipherSpi.h"
using beecrypt::crypto::CipherSpi;

namespace beecrypt {
	namespace provider {
		class BlockCipher : public CipherSpi
		{
		private:
			static const int MODE_ECB;
			static const int MODE_CBC;
			static const int MODE_CTR;

			static const int PADDING_NONE;
			static const int PADDING_PKCS5;

		private:
			blockCipherContext _ctxt;
			int _opmode;
			int _blmode;
			int _padding;
			bytearray _key;
			int _keybits;
			bytearray _buffer;
			int _bufcnt;
			int _buflwm;
			bytearray _iv;

			int process(const byte* input, int inputLength, byte* output, int outputLength) throw (ShortBufferException);
			void engineReset();

		protected:
			virtual bytearray* engineDoFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalBlockSizeException, BadPaddingException);
			virtual int engineDoFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException);

			virtual int engineGetBlockSize() const throw ();
			virtual bytearray* engineGetIV();
			virtual int engineGetKeySize(const Key& key) const throw (InvalidKeyException);
			virtual int engineGetOutputSize(int inputLength) throw ();
			virtual AlgorithmParameters* engineGetParameters() throw ();

			virtual void engineInit(int opmode, const Key& key, SecureRandom* random) throw (InvalidKeyException);
			virtual void engineInit(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException);
			virtual void engineInit(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException);

			virtual bytearray* engineUpdate(const byte* input, int inputOffset, int inputLength);
			virtual int engineUpdate(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException);

            virtual void engineSetMode(const String& mode) throw (NoSuchAlgorithmException);
            virtual void engineSetPadding(const String& padding) throw (NoSuchPaddingException);

			BlockCipher(const blockCipher&);

		public:
			virtual ~BlockCipher() {}
		};
	}
}

#endif

#endif
