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

/*!\file BeeCertPathValidatorResult.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_BEECERTPATHVALIDATORRESULT_H
#define _CLASS_BEECERTPATHVALIDATORRESULT_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/security/cert/CertPathValidatorResult.h"
using beecrypt::security::cert::CertPathValidatorResult;
#include "beecrypt/c++/beeyond/BeeCertificate.h"
using beecrypt::beeyond::BeeCertificate;

namespace beecrypt {
	namespace beeyond {
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI BeeCertPathValidatorResult : public Object, public CertPathValidatorResult
		{
		private:
			BeeCertificate* _root;
			PublicKey* _pub;

		public:
			BeeCertPathValidatorResult(const BeeCertificate& root, const PublicKey& pub);
			virtual ~BeeCertPathValidatorResult();

			virtual BeeCertPathValidatorResult* clone() const throw ();

			const BeeCertificate& getRootCertificate() const;
			const PublicKey& getPublicKey() const;
		};
	}
}

#endif

#endif
