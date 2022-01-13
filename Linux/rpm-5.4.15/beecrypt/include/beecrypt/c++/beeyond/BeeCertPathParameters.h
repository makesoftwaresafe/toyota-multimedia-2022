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

/*!\file BeeCertPathParameters.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_BEECERTPATHPARAMETERS_H
#define _CLASS_BEECERTPATHPARAMETERS_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/InvalidAlgorithmParameterException.h"
using beecrypt::security::InvalidAlgorithmParameterException;
#include "beecrypt/c++/security/KeyStore.h"
using beecrypt::security::KeyStore;
#include "beecrypt/c++/security/KeyStoreException.h"
using beecrypt::security::KeyStoreException;
#include "beecrypt/c++/security/cert/Certificate.h"
using beecrypt::security::cert::Certificate;
#include "beecrypt/c++/security/cert/CertPathParameters.h"
using beecrypt::security::cert::CertPathParameters;
#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;

namespace beecrypt {
	namespace beeyond {
		class BEECRYPTCXXAPI BeeCertPathParameters : public Object, public CertPathParameters
		{
		private:
			ArrayList<Certificate> _cert;

		protected:
			BeeCertPathParameters();

		public:
			BeeCertPathParameters(KeyStore& keystore) throw (KeyStoreException, InvalidAlgorithmParameterException);
			virtual ~BeeCertPathParameters() {};

			virtual BeeCertPathParameters* clone() const throw ();

			const List<Certificate>& getTrustedCertificates() const;
		};
	}
}

#endif

#endif
