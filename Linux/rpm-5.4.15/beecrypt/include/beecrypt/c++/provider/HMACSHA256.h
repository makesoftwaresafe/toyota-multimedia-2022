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

/*!\file HMACSHA256.h
 * \ingroup CXX_PROVIDER_m
 */

#ifndef _CLASS_HMACSHA256_H
#define _CLASS_HMACSHA256_H

#ifdef __cplusplus

#include "beecrypt/c++/provider/HMAC.h"

namespace beecrypt {
	namespace provider {
		class HMACSHA256 : public HMAC, public Cloneable
		{
		public:
			HMACSHA256();
			virtual ~HMACSHA256() {}

			virtual HMACSHA256* clone() const throw ();
		};
	}
}

#endif

#endif
