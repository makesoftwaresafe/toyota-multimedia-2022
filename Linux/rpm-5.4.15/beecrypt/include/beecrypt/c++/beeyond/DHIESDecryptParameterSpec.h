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

/*!\file DHIESDecryptParameterSpec.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_DHIESDECRYPTPARAMETERSPEC_H
#define _CLASS_DHIESDECRYPTPARAMETERSPEC_H

#ifdef __cplusplus

#include "beecrypt/c++/beeyond/DHIESParameterSpec.h"
using beecrypt::beeyond::DHIESParameterSpec;

namespace beecrypt {
	namespace beeyond {
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI DHIESDecryptParameterSpec : public DHIESParameterSpec
		{
		private:
			BigInteger _pub;
			bytearray _mac;

		public:
			DHIESDecryptParameterSpec(const DHIESParameterSpec& copy, const BigInteger& key, const bytearray& mac);
			DHIESDecryptParameterSpec(const DHIESDecryptParameterSpec& copy);
			virtual ~DHIESDecryptParameterSpec() {}

			const BigInteger& getEphemeralPublicKey() const throw ();
			const bytearray& getMac() const throw ();
		};
	}
}

#endif

#endif
