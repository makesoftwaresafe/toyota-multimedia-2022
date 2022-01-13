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

/*!\file DSAParameterSpec.h
 * \ingroup CXX_SECURITY_SPEC_m
 */

#ifndef _CLASS_BEE_SECURITY_SPEC_DSAPARAMETERSPEC_H
#define _CLASS_BEE_SECURITY_SPEC_DSAPARAMETERSPEC_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/math/BigInteger.h"
using beecrypt::math::BigInteger;
#include "beecrypt/c++/security/interfaces/DSAParams.h"
using beecrypt::security::interfaces::DSAParams;
#include "beecrypt/c++/security/spec/AlgorithmParameterSpec.h"
using beecrypt::security::spec::AlgorithmParameterSpec;

namespace beecrypt {
	namespace security {
		namespace spec {
			/*!\brief DSA parameter specification
			 * \ingroup CXX_SECURITY_SPEC_m
			 */
			class BEECRYPTCXXAPI DSAParameterSpec : public Object, public virtual AlgorithmParameterSpec, public virtual DSAParams
			{
			private:
				BigInteger _p;
				BigInteger _q;
				BigInteger _g;

			public:
				DSAParameterSpec(const DSAParams&);
				DSAParameterSpec(const BigInteger& p, const BigInteger& q, const BigInteger& g);
				virtual ~DSAParameterSpec() {}

				const BigInteger& getP() const throw ();
				const BigInteger& getQ() const throw ();
				const BigInteger& getG() const throw ();
			};
		}
	}
}

#endif

#endif
