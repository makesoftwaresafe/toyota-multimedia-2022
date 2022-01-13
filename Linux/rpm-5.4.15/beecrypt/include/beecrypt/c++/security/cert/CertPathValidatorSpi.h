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

/*!\file CertPathValidatorSpi.h
 * \ingroup CXX_SECURITY_SPEC_m
 */

#ifndef _CLASS_BEE_SECURITY_CERT_CERTPATHVALIDATORSPI_H
#define _CLASS_BEE_SECURITY_CERT_CERTPATHVALIDATORSPI_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/InvalidAlgorithmParameterException.h"
using beecrypt::security::InvalidAlgorithmParameterException;
#include "beecrypt/c++/security/cert/CertPath.h"
using beecrypt::security::cert::CertPath;
#include "beecrypt/c++/security/cert/CertPathParameters.h"
using beecrypt::security::cert::CertPathParameters;
#include "beecrypt/c++/security/cert/CertPathValidatorException.h"
using beecrypt::security::cert::CertPathValidatorException;
#include "beecrypt/c++/security/cert/CertPathValidatorResult.h"
using beecrypt::security::cert::CertPathValidatorResult;

namespace beecrypt {
	namespace security {
		namespace cert {
			/*!\ingroup CXX_SECURITY_CERT_m
			 */
			class BEECRYPTCXXAPI CertPathValidatorSpi : public Object
			{
				friend class CertPathValidator;

			protected:
				virtual CertPathValidatorResult* engineValidate(const CertPath& path, const CertPathParameters& params) throw (CertPathValidatorException, InvalidAlgorithmParameterException) = 0;

			public:
				virtual ~CertPathValidatorSpi() {}
			};
		}
	}
}

#endif

#endif
