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

/*!\file RSAPrivateCrtKeyImpl.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_RSAPRIVATECRTKEYIMPL_H
#define _CLASS_RSAPRIVATECRTKEYIMPL_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/interfaces/RSAPrivateCrtKey.h"
using beecrypt::security::interfaces::RSAPrivateCrtKey;

namespace beecrypt {
	namespace provider {
		class RSAPrivateCrtKeyImpl : public Object, public RSAPrivateCrtKey, public Cloneable
		{
		private:
			BigInteger _n;
			BigInteger _e;
			BigInteger _d;
			BigInteger _p;
			BigInteger _q;
			BigInteger _dp;
			BigInteger _dq;
			BigInteger _qi;
			mutable bytearray* _enc;

		public:
			RSAPrivateCrtKeyImpl(const RSAPrivateCrtKey&);
			RSAPrivateCrtKeyImpl(const RSAPrivateCrtKeyImpl&);
			RSAPrivateCrtKeyImpl(const BigInteger& modulus, const BigInteger& publicExponent, const BigInteger& privateExponent, const BigInteger& primeP, const BigInteger& primeQ, const BigInteger& primeExponentP, const BigInteger& primeExponentQ, const BigInteger& crtCoefficient);
			virtual ~RSAPrivateCrtKeyImpl();

			virtual RSAPrivateCrtKeyImpl* clone() const throw ();

			virtual bool equals(const Object* obj) const throw ();

			virtual const BigInteger& getModulus() const throw ();
			virtual const BigInteger& getPrivateExponent() const throw ();
			virtual const BigInteger& getPublicExponent() const throw ();
			virtual const BigInteger& getPrimeP() const throw ();
			virtual const BigInteger& getPrimeQ() const throw ();
			virtual const BigInteger& getPrimeExponentP() const throw ();
			virtual const BigInteger& getPrimeExponentQ() const throw ();
			virtual const BigInteger& getCrtCoefficient() const throw ();

			virtual const bytearray* getEncoded() const throw ();
			virtual const String& getAlgorithm() const throw ();
			virtual const String* getFormat() const throw ();
		};
	}
}

#endif

#endif
