/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/crypto/NullCipher.h"

using namespace beecrypt::crypto;

bytearray* NullCipher::NullCipherSpi::engineDoFinal(const byte* input, int inputOffset, int inputLength) throw (IllegalBlockSizeException, BadPaddingException)
{
	if (inputLength == 0)
		return 0;

	return new bytearray(input+inputOffset, inputLength);
}

int NullCipher::NullCipherSpi::engineDoFinal(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException, IllegalBlockSizeException, BadPaddingException)
{
	if (inputLength > 0)
	{
		if (output.size() - outputOffset > inputLength)
			throw ShortBufferException("output buffer too short");

		memmove(output.data() + outputOffset, input+inputOffset, inputLength);
	}

	return inputLength;
}

int NullCipher::NullCipherSpi::engineGetBlockSize() const throw ()
{
	return 1;
}

bytearray* NullCipher::NullCipherSpi::engineGetIV()
{
	return 0;
}

int NullCipher::NullCipherSpi::engineGetOutputSize(int inputLength) throw ()
{
	return inputLength;
}

AlgorithmParameters* NullCipher::NullCipherSpi::engineGetParameters() throw ()
{
	return 0;
}

void NullCipher::NullCipherSpi::engineInit(int opmode, const Key& key, SecureRandom* random) throw (InvalidKeyException)
{
}

void NullCipher::NullCipherSpi::engineInit(int opmode, const Key& key, AlgorithmParameters* params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
}

void NullCipher::NullCipherSpi::engineInit(int opmode, const Key& key, const AlgorithmParameterSpec& params, SecureRandom* random) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
}

void NullCipher::NullCipherSpi::engineSetMode(const String& mode) throw (NoSuchAlgorithmException)
{
}

void NullCipher::NullCipherSpi::engineSetPadding(const String& mode) throw (NoSuchPaddingException)
{
}

bytearray* NullCipher::NullCipherSpi::engineUpdate(const byte* input, int inputOffset, int inputLength)
{
	if (inputLength == 0)
		return 0;

	return new bytearray(input+inputOffset, inputLength);
}

int NullCipher::NullCipherSpi::engineUpdate(const byte* input, int inputOffset, int inputLength, bytearray& output, int outputOffset) throw (ShortBufferException)
{
	if (inputLength > 0)
	{
		if (output.size() - outputOffset > inputLength)
			throw ShortBufferException("output buffer too short");

		memmove(output.data() + outputOffset, input+inputOffset, inputLength);
	}

	return inputLength;
}

NullCipher::NullCipher() : Cipher(new NullCipherSpi(), (const Provider*) 0, "")
{
}
