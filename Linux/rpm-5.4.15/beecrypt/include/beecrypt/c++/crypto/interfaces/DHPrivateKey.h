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

/*!\file DHPrivateKey.h
 * \ingroup CXX_CRYPTO_INTERFACES_m
 */

#ifndef _INTERFACE_BEE_CRYPTO_INTERFACES_DHPRIVATEKEY_H
#define _INTERFACE_BEE_CRYPTO_INTERFACES_DHPRIVATEKEY_H

#ifdef __cplusplus

#include "beecrypt/c++/security/PrivateKey.h"
using beecrypt::security::PrivateKey;
#include "beecrypt/c++/crypto/interfaces/DHKey.h"
using beecrypt::crypto::interfaces::DHKey;

namespace beecrypt {
	namespace crypto {
		namespace interfaces {
			/*!\brief Diffie-Hellman private key interface
			 * \ingroup CXX_CRYPTO_INTERFACES_m
			 */
			class DHPrivateKey : public virtual PrivateKey, public DHKey
			{
			public:
				virtual ~DHPrivateKey() {}

				virtual const BigInteger& getX() const throw () = 0;
			};
		}
	}
}

#endif

#endif
