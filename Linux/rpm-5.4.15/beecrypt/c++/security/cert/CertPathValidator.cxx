/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/security/cert/CertPathValidator.h"
#include "beecrypt/c++/security/Security.h"

using namespace beecrypt::security::cert;

CertPathValidator::CertPathValidator(CertPathValidatorSpi* spi, const Provider* provider, const String& algorithm)
{
	_cspi = spi;
	_prov = provider;
	_algo = algorithm;
}

CertPathValidator::~CertPathValidator()
{
	delete _cspi;
}

CertPathValidator* CertPathValidator::getInstance(const String& algorithm) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "CertPathValidator");

	assert(dynamic_cast<CertPathValidatorSpi*>((CertPathValidatorSpi*) tmp->cspi));

	CertPathValidator* result = new CertPathValidator((CertPathValidatorSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

CertPathValidator* CertPathValidator::getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "CertPathValidator", provider);

	assert(dynamic_cast<CertPathValidatorSpi*>((CertPathValidatorSpi*) tmp->cspi));

	CertPathValidator* result = new CertPathValidator((CertPathValidatorSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

CertPathValidator* CertPathValidator::getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "CertPathValidator", provider);

	assert(dynamic_cast<CertPathValidatorSpi*>((CertPathValidatorSpi*) tmp->cspi));

	CertPathValidator* result = new CertPathValidator((CertPathValidatorSpi*) tmp->cspi, tmp->prov, tmp->name);

	delete tmp;

	return result;
}

CertPathValidatorResult* CertPathValidator::validate(const CertPath& certPath, const CertPathParameters& params) throw (CertPathValidatorException, InvalidAlgorithmParameterException)
{
	return _cspi->engineValidate(certPath, params);
}
