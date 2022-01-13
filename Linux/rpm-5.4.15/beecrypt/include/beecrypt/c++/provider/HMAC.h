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

/*!\file HMAC.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_HMAC_H
#define _CLASS_HMAC_H

#include "beecrypt/beecrypt.h"

#ifdef __cplusplus

#include "beecrypt/c++/crypto/MacSpi.h"
using beecrypt::crypto::MacSpi;
#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;

namespace beecrypt {
	namespace provider {
		class HMAC : public MacSpi
		{
		private:
			bytearray _key;
			bytearray _digest;

			const keyedHashFunction* _kf;
			const hashFunction* _hf;

		protected:
			keyedHashFunctionContext _ctxt;

		protected:
			virtual const bytearray& engineDoFinal();
			virtual int engineDoFinal(byte*, int, int) throw (ShortBufferException);
			virtual int engineGetMacLength();
			virtual void engineInit(const Key&, const AlgorithmParameterSpec* spec) throw (InvalidKeyException, InvalidAlgorithmParameterException);
			virtual void engineReset();
			virtual void engineUpdate(byte);
			virtual void engineUpdate(const byte*, int, int);

		protected:
			HMAC(const keyedHashFunction&, const hashFunction&);

		public:
			virtual ~HMAC() {}
		};
	}
}

#endif

#endif
