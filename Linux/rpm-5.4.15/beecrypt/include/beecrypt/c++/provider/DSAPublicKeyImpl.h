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

/*!\file DSAPublicKeyImpl.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_DSAPUBLICKEYIMPL_H
#define _CLASS_DSAPUBLICKEYIMPL_H

#include "beecrypt/dsa.h"

#ifdef __cplusplus

#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/interfaces/DSAPublicKey.h"
using beecrypt::security::interfaces::DSAPublicKey;
#include "beecrypt/c++/security/spec/DSAParameterSpec.h"
using beecrypt::security::spec::DSAParameterSpec;

namespace beecrypt {
	namespace provider {
		class DSAPublicKeyImpl : public Object, public DSAPublicKey, public Cloneable
		{
		private:
			DSAParameterSpec* _params;
			BigInteger _y;
			mutable bytearray* _enc;

		public:
			DSAPublicKeyImpl(const DSAPublicKey&);
			DSAPublicKeyImpl(const DSAPublicKeyImpl&);
			DSAPublicKeyImpl(const DSAParams&, const BigInteger&);
			DSAPublicKeyImpl(const dsaparam&, const mpnumber&);
			DSAPublicKeyImpl(const BigInteger&, const BigInteger&, const BigInteger&, const BigInteger&);
			virtual ~DSAPublicKeyImpl();

			virtual DSAPublicKeyImpl* clone() const throw ();

			virtual bool equals(const Object* obj) const throw ();

			virtual const DSAParams& getParams() const throw ();
			virtual const BigInteger& getY() const throw ();

			virtual const bytearray* getEncoded() const throw ();
			virtual const String& getAlgorithm() const throw ();
			virtual const String* getFormat() const throw ();
		};
	}
}

#endif

#endif
