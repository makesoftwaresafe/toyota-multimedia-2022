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

/*!\file RSAKeyGenParameterSpec.h
 * \ingroup CXX_SECURITY_SPEC_m
 */

#ifndef _CLASS_BEE_SECURITY_SPEC_RSAKEYGENPARAMETERSPEC
#define _CLASS_BEE_SECURITY_SPEC_RSAKEYGENPARAMETERSPEC

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/math/BigInteger.h"
using beecrypt::math::BigInteger;
#include "beecrypt/c++/security/spec/AlgorithmParameterSpec.h"
using beecrypt::security::spec::AlgorithmParameterSpec;

namespace beecrypt {
	namespace security {
		namespace spec {
			/*!\ingroup CXX_SECURITY_SPEC_m
			 */
			class BEECRYPTCXXAPI RSAKeyGenParameterSpec : public Object, public virtual AlgorithmParameterSpec
			{
			public:
				static const BigInteger F0;
				static const BigInteger F4;

			private:
				int _keysize;
				BigInteger _e;
				
			public:
				RSAKeyGenParameterSpec(int, const BigInteger&);
				virtual ~RSAKeyGenParameterSpec() {}

				int getKeysize() const throw ();
				const BigInteger& getPublicExponent() const throw ();
			};
		}
	}
}

#endif

#endif
