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

/*!\file KeyAgreementSpi.h
 * \ingroup CXX_CRYPTO_m
 */

#ifndef _CLASS_BEE_CRYPTO_KEYAGREEMENTSPI_H
#define _CLASS_BEE_CRYPTO_KEYAGREEMENTSPI_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::bytearray;
#include "beecrypt/c++/crypto/SecretKey.h"
using beecrypt::crypto::SecretKey;
#include "beecrypt/c++/lang/IllegalStateException.h"
using beecrypt::lang::IllegalStateException;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/SecureRandom.h"
using beecrypt::security::SecureRandom;
#include "beecrypt/c++/security/InvalidAlgorithmParameterException.h"
using beecrypt::security::InvalidAlgorithmParameterException;
#include "beecrypt/c++/security/InvalidKeyException.h"
using beecrypt::security::InvalidKeyException;
#include "beecrypt/c++/security/ShortBufferException.h"
using beecrypt::security::ShortBufferException;
#include "beecrypt/c++/security/spec/AlgorithmParameterSpec.h"
using beecrypt::security::spec::AlgorithmParameterSpec;

namespace beecrypt {
	namespace crypto {
		/*!\ingroup CXX_CRYPTO_m
		 */
		class BEECRYPTCXXAPI KeyAgreementSpi : public Object
		{
			friend class KeyAgreement;

		protected:
			virtual void engineInit(const Key&, SecureRandom*) throw (InvalidKeyException) = 0;
			virtual void engineInit(const Key&, const AlgorithmParameterSpec&, SecureRandom*) throw (InvalidKeyException, InvalidAlgorithmParameterException) = 0;
			virtual Key* engineDoPhase(const Key&, bool) = 0;
			virtual bytearray* engineGenerateSecret() throw (IllegalStateException) = 0;
			virtual int engineGenerateSecret(bytearray&, int) throw (IllegalStateException, ShortBufferException) = 0;
			virtual SecretKey* engineGenerateSecret(const String&) throw (IllegalStateException, NoSuchAlgorithmException, InvalidKeyException) = 0;
		public:
			virtual ~KeyAgreementSpi() {}
		};
	}
}

#endif

#endif
