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

/*!\file PKCS12PBEKey.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_PKCS12PBEKEY_H
#define _CLASS_PKCS12PBEKEY_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::array;
using beecrypt::bytearray;
#include "beecrypt/c++/crypto/interfaces/PBEKey.h"
using beecrypt::crypto::interfaces::PBEKey;

namespace beecrypt {
	namespace beeyond {
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI PKCS12PBEKey : public PBEKey
		{
		private:
			array<jchar>    _pswd;
			bytearray*         _salt;
			int                _iter;
			mutable bytearray* _enc;

		public:
			static bytearray* encode(const array<jchar>&);

		public:
			PKCS12PBEKey(const array<jchar>&, const bytearray*, int);
			virtual ~PKCS12PBEKey();

			virtual bool operator==(const Key& compare) const throw ();

			virtual PKCS12PBEKey* clone() const;

			virtual int getIterationCount() const throw ();
			virtual const array<jchar>& getPassword() const throw ();
			virtual const bytearray* getSalt() const throw ();

			virtual const bytearray* getEncoded() const throw ();

			virtual const String& getAlgorithm() const throw();
			virtual const String* getFormat() const throw ();
		};
	}
}

#endif

#endif
