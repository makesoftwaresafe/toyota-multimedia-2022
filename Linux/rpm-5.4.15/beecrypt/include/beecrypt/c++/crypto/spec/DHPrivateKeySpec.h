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

/*!\file DHPrivateKeySpec.h
 * \ingroup CXX_CRYPTO_SPEC_m
 */

#ifndef _CLASS_BEE_CRYPTO_SPEC_DHPRIVATEKEYSPEC_H
#define _CLASS_BEE_CRYPTO_SPEC_DHPRIVATEKEYSPEC_H

#ifdef __cplusplus

#include "beecrypt/c++/security/spec/KeySpec.h"
using beecrypt::security::spec::KeySpec;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/math/BigInteger.h"
using beecrypt::math::BigInteger;

namespace beecrypt {
	namespace crypto {
		namespace spec {
			/*!\ingroup CXX_CRYPTO_SPEC_m
			 */
			class BEECRYPTCXXAPI DHPrivateKeySpec : public Object, public virtual KeySpec
			{
			private:
				BigInteger _p;
				BigInteger _g;
				BigInteger _x;

			public:
				DHPrivateKeySpec(const BigInteger& x, const BigInteger& p, const BigInteger& g);
				virtual ~DHPrivateKeySpec() {}

				const BigInteger& getP() const throw ();
				const BigInteger& getG() const throw ();
				const BigInteger& getX() const throw ();
			};
		}
	}
}

#endif

#endif
