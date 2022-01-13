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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/beeyond/BeeCertPath.h"
using beecrypt::beeyond::BeeCertPath;
#include "beecrypt/c++/beeyond/BeeCertPathParameters.h"
using beecrypt::beeyond::BeeCertPathParameters;
#include "beecrypt/c++/beeyond/BeeCertPathValidatorResult.h"
using beecrypt::beeyond::BeeCertPathValidatorResult;
#include "beecrypt/c++/provider/BeeCertPathValidator.h"

using namespace beecrypt::provider;

BeeCertPathValidator::~BeeCertPathValidator()
{
}

CertPathValidatorResult* BeeCertPathValidator::engineValidate(const CertPath& path, const CertPathParameters& params) throw (CertPathValidatorException, InvalidAlgorithmParameterException)
{
	const BeeCertPathParameters* beeparams = dynamic_cast<const BeeCertPathParameters*>(&params);
	if (beeparams)
	{
		const List<Certificate>& cert = path.getCertificates();
		const List<Certificate>& root = beeparams->getTrustedCertificates();

		for (int i = 0; i < cert.size(); i++)
		{
			const BeeCertificate* beecert = dynamic_cast<const BeeCertificate*>(cert.get(i));
			if (beecert)
			{
				const BeeCertificate* tmp = beecert;

				while (tmp->hasParentCertificate())
				{
					const Certificate* parent = &tmp->getParentCertificate();

					try
					{
						Date created(tmp->getNotBefore());

						// a certificate must be signed with its parent's public key
						tmp->verify(parent->getPublicKey());

						// if the parent is not a BeeCertificate we can't validate
						tmp = dynamic_cast<const BeeCertificate*>(parent);
						if (tmp)
						{
							// a certificate must have a creation date within the validity range of its parent
							tmp->checkValidity(created);
						}
						else
							throw CertPathValidatorException("Parent is not a BeeCertificate");
					}
					catch (CertificateExpiredException&)
					{
						throw CertPathValidatorException("Parent expired before certificate was created");
					}
					catch (CertificateNotYetValidException&)
					{
						throw CertPathValidatorException("Certificate was created before its parent");
					}
					catch (CertPathValidatorException&)
					{
						throw;
					}
					catch (Exception& e)
					{
						// on any exception, the certificate path failed to validate
						throw CertPathValidatorException().initCause(e);
					}
				}

				// check if the final certificate we have is one of the root certificates
				for (int j = 0; j < root.size(); j++)
				{
					if (tmp->equals(root.get(j)))
						return new BeeCertPathValidatorResult(*tmp, beecert->getPublicKey());
				}
			}
			else
				throw InvalidAlgorithmParameterException("CertPath should only contains BeeCertificate objects");
		}

		throw CertPathValidatorException("CertPath does not lead to one of the trusted parent certificates");
	}
	else
		throw InvalidAlgorithmParameterException("Invalid CertPathParameters subclass; only BeeCertPathParameters is accepted");
}
