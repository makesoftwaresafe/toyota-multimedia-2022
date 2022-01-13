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

/*!\file PKCS1RSASignature.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_PKCS1RSASIGNATURE_H
#define _CLASS_PKCS1RSASIGNATURE_H

#include "beecrypt/api.h"
#include "beecrypt/rsa.h"

#ifdef __cplusplus

#include "beecrypt/c++/security/SignatureSpi.h"
using beecrypt::security::SecureRandom;
using beecrypt::security::SignatureSpi;
using beecrypt::security::AlgorithmParameters;
using beecrypt::security::InvalidAlgorithmParameterException;
using beecrypt::security::InvalidKeyException;
using beecrypt::security::PrivateKey;
using beecrypt::security::PublicKey;
using beecrypt::security::ShortBufferException;
using beecrypt::security::SignatureException;
using beecrypt::security::spec::AlgorithmParameterSpec;

namespace beecrypt {
	namespace provider {
		class PKCS1RSASignature : SignatureSpi
		{
		private:
			rsakp _pair;
			bool _crt;
			hashFunctionContext _hfc;
			SecureRandom* _srng;

		protected:
			PKCS1RSASignature(const hashFunction*);

			virtual AlgorithmParameters* engineGetParameters() const;
			virtual void engineSetParameter(const AlgorithmParameterSpec&) throw (InvalidAlgorithmParameterException);

			virtual void engineInitSign(const PrivateKey&, SecureRandom*) throw (InvalidKeyException);
			virtual void engineInitVerify(const PublicKey&) throw (InvalidKeyException);

			virtual bytearray* engineSign() throw (SignatureException);
			virtual int engineSign(byte*, int, int) throw (ShortBufferException, SignatureException);
			virtual int engineSign(bytearray&) throw (SignatureException);
			virtual bool engineVerify(const byte*, int, int) throw (SignatureException);

			virtual void engineUpdate(byte);
			virtual void engineUpdate(const byte*, int, int);

		public:
			virtual ~PKCS1RSASignature() {}
		};
	}
}

#endif

#endif
