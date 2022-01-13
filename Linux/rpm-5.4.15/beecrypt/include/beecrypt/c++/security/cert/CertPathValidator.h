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

/*!\file CertPathValidator.h
 * \ingroup CXX_SECURITY_CERT_m
 */

#ifndef _CLASS_BEE_SECURITY_CERT_CERTPATHVALIDATOR_H
#define _CLASS_BEE_SECURITY_CERT_CERTPATHVALIDATOR_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/NoSuchProviderException.h"
using beecrypt::security::NoSuchProviderException;
#include "beecrypt/c++/security/cert/CertPathValidatorSpi.h"
using beecrypt::security::cert::CertPathValidatorSpi;

namespace beecrypt {
	namespace security {
		namespace cert {
			/*!\ingroup CXX_SECURITY_CERT_m
			 */
			class BEECRYPTCXXAPI CertPathValidator : public Object
			{
			public:
				static CertPathValidator* getInstance(const String& algorithm) throw (NoSuchAlgorithmException);
				static CertPathValidator* getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
				static CertPathValidator* getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException);

				static const String& getDefaultType() throw ();

			private:
				CertPathValidatorSpi* _cspi;
				const Provider*       _prov;
				String                _algo;

			protected:
				CertPathValidator(CertPathValidatorSpi* spi, const Provider* provider, const String& algorithm);

			public:
				virtual ~CertPathValidator();

				CertPathValidatorResult* validate(const CertPath& certPath, const CertPathParameters& params) throw (CertPathValidatorException, InvalidAlgorithmParameterException);

				const String& getAlgorithm() const throw ();
				const Provider& getProvider() const throw ();
			};
		}
	}
}

#endif

#endif
