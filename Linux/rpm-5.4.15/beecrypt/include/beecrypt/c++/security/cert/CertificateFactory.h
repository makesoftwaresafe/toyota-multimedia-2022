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

/*!\file CertificateFactory.h
 * \ingroup CXX_SECURITY_CERT_m
 */

#ifndef _CLASS_BEE_SECURITY_CERT_CERTIFICATEFACTORY_H
#define _CLASS_BEE_SECURITY_CERT_CERTIFICATEFACTORY_H

#ifdef __cplusplus

#include "beecrypt/c++/security/Provider.h"
using beecrypt::security::Provider;
#include "beecrypt/c++/security/NoSuchAlgorithmException.h"
using beecrypt::security::NoSuchAlgorithmException;
#include "beecrypt/c++/security/cert/CertificateFactorySpi.h"
using beecrypt::security::cert::CertificateFactorySpi;

namespace beecrypt {
	namespace security {
		namespace cert {
			/*!\ingroup CXX_SECURITY_CERT_m
			 */
			class BEECRYPTCXXAPI CertificateFactory : public Object
			{
			public:
				static CertificateFactory* getInstance(const String& type) throw (NoSuchAlgorithmException);
				static CertificateFactory* getInstance(const String& type, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException);
				static CertificateFactory* getInstance(const String& type, const Provider& provider) throw (NoSuchAlgorithmException);

			private:
				CertificateFactorySpi* _cspi;
				const Provider*        _prov;
				String                 _type;

			protected:
				CertificateFactory(CertificateFactorySpi* spi, const Provider* provider, const String& type);

			public:
				virtual ~CertificateFactory();

				Certificate* generateCertificate(InputStream& in) throw (CertificateException);
				Collection<Certificate>* generateCertificates(InputStream& in) throw (CertificateException);

				const String& getType() const throw ();
				const Provider& getProvider() const throw ();
			};
		}
	}
}

#endif

#endif
