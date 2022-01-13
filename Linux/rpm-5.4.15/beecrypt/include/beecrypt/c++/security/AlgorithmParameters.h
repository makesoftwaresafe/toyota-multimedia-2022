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

/*!\file AlgorithmParameters.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_ALGORITHMPARAMETERS_H
#define _CLASS_BEE_SECURITY_ALGORITHMPARAMETERS_H

#ifdef __cplusplus

#include "beecrypt/c++/security/AlgorithmParametersSpi.h"
using beecrypt::security::AlgorithmParametersSpi;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/NoSuchProviderException.h"
using beecrypt::security::NoSuchProviderException;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI AlgorithmParameters : public Object
		{
		public:
			static AlgorithmParameters* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
			static AlgorithmParameters* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static AlgorithmParameters* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

		private:
			AlgorithmParametersSpi* _aspi;
			const Provider*         _prov;
			String                  _algo;

		protected:
			AlgorithmParameters(AlgorithmParametersSpi* spi, const Provider* provider, const String& algorithm);

		public:
			virtual ~AlgorithmParameters();

			const bytearray& getEncoded(const String* format = 0) throw (IOException);
			AlgorithmParameterSpec* getParameterSpec(const type_info&) throw (InvalidParameterSpecException);

			void init(const AlgorithmParameterSpec& spec) throw (InvalidParameterSpecException);
			void init(const byte* data, int size, const String* format = 0);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();

			String toString() throw ();
		};
	}
}

#endif

#endif
