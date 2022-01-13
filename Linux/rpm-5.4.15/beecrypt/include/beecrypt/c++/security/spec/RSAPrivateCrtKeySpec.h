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

/*!\file RSAPrivateCrtKeySpec.h
 * \ingroup CXX_SECURITY_SPEC_m
 */

#ifndef _CLASS_BEE_SECURITY_SPEC_RSAPRIVATECRTKEYSPEC_H
#define _CLASS_BEE_SECURITY_SPEC_RSAPRIVATECRTKEYSPEC_H

#ifdef __cplusplus

#include "beecrypt/c++/security/spec/RSAPrivateKeySpec.h"
using beecrypt::security::spec::RSAPrivateKeySpec;

namespace beecrypt {
	namespace security {
		namespace spec {
			/*!\ingroup CXX_SECURITY_SPEC_m
			 */
			class BEECRYPTCXXAPI RSAPrivateCrtKeySpec : public RSAPrivateKeySpec
			{
				private:
					BigInteger _e;
					BigInteger _p;
					BigInteger _q;
					BigInteger _dp;
					BigInteger _dq;
					BigInteger _qi;

				public:
					RSAPrivateCrtKeySpec(const BigInteger& modulus, const BigInteger& publicExponent, const BigInteger& privateExponent, const BigInteger& primeP, const BigInteger& primeQ, const BigInteger& primeExponentP, const BigInteger& primeExponentQ, const BigInteger& crtCoefficient);
					virtual ~RSAPrivateCrtKeySpec() {}

					const BigInteger& getPublicExponent() const throw ();
					const BigInteger& getPrimeP() const throw ();
					const BigInteger& getPrimeQ() const throw ();
					const BigInteger& getPrimeExponentP() const throw ();
					const BigInteger& getPrimeExponentQ() const throw ();
					const BigInteger& getCrtCoefficient() const throw ();
			};
		}
	}
}

#endif

#endif
