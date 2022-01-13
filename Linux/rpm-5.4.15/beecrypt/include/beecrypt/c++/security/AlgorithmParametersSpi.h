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

/*!\file AlgorithmParametersSpi.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_ALGORITHMPARAMETERSSPI_H
#define _CLASS_BEE_SECURITY_ALGORITHMPARAMETERSSPI_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::bytearray;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/io/IOException.h"
using beecrypt::io::IOException;
#include "beecrypt/c++/security/spec/AlgorithmParameterSpec.h"
using beecrypt::security::spec::AlgorithmParameterSpec;
#include "beecrypt/c++/security/spec/InvalidParameterSpecException.h"
using beecrypt::security::spec::InvalidParameterSpecException;

#include <typeinfo>
using std::type_info;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI AlgorithmParametersSpi : public Object
		{
			friend class AlgorithmParameters;

		protected:
			virtual const bytearray& engineGetEncoded(const String* format = 0) throw (IOException) = 0;
			virtual AlgorithmParameterSpec* engineGetParameterSpec(const type_info& info) = 0;

			virtual void engineInit(const AlgorithmParameterSpec& spec) throw (InvalidParameterSpecException) = 0;
			virtual void engineInit(const byte* data, int size, const String* format = 0) = 0;

			virtual String engineToString() throw () = 0;

		public:
			virtual ~AlgorithmParametersSpi() {}
		};
	}
}

#endif

#endif
