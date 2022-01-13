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

/*!\file BeeOutputStream.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_BEEOUTPUTSTREAM_H
#define _CLASS_BEEOUTPUTSTREAM_H

#ifdef __cplusplus

#include "beecrypt/c++/io/DataOutputStream.h"
using beecrypt::io::DataOutputStream;
#include "beecrypt/c++/math/BigInteger.h"
using beecrypt::math::BigInteger;

namespace beecrypt {
	namespace beeyond {
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI BeeOutputStream : public DataOutputStream
		{
		public:
			BeeOutputStream(OutputStream& out);
			virtual ~BeeOutputStream();

			void writeBigInteger(const BigInteger&) throw (IOException);
		};
	}
}

#endif

#endif
