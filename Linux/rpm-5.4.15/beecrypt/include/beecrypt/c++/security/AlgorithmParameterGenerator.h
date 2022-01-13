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

/*!\file AlgorithmParameterGenerator.h
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_ALGORITHMPARAMETERGENERATOR_H
#define _CLASS_BEE_SECURITY_ALGORITHMPARAMETERGENERATOR_H

#ifdef __cplusplus

#include "beecrypt/c++/security/AlgorithmParameterGeneratorSpi.h"
using beecrypt::security::AlgorithmParameterGeneratorSpi;
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
		class BEECRYPTCXXAPI AlgorithmParameterGenerator : public Object
		{
		public:
			static AlgorithmParameterGenerator* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
			static AlgorithmParameterGenerator* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
			static AlgorithmParameterGenerator* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

		private:
			AlgorithmParameterGeneratorSpi* _aspi;
			const Provider*                 _prov;
			String                          _algo;

		protected:
			AlgorithmParameterGenerator(AlgorithmParameterGeneratorSpi* spi, const Provider* provider, const String& algorithm);

		public:
			virtual ~AlgorithmParameterGenerator();

			AlgorithmParameters* generateParameters() throw (InvalidAlgorithmParameterException);

			void init(const AlgorithmParameterSpec& genParamSpec) throw (InvalidAlgorithmParameterException);
			void init(const AlgorithmParameterSpec&, SecureRandom* random) throw (InvalidAlgorithmParameterException);
			void init(int size) throw (InvalidParameterException);
			void init(int size, SecureRandom* random) throw (InvalidParameterException);

			const String& getAlgorithm() const throw ();
			const Provider& getProvider() const throw ();
		};
	}
}

#endif

#endif
