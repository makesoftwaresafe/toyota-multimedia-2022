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

/*!\file BeeCertPath.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_BEECERTPATH_H
#define _CLASS_BEECERTPATH_H

#ifdef __cplusplus

#include "beecrypt/c++/security/cert/CertPath.h"
using beecrypt::security::cert::CertPath;
#include "beecrypt/c++/beeyond/BeeCertificate.h"
using beecrypt::beeyond::BeeCertificate;
#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;

namespace beecrypt {
	namespace beeyond {
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI BeeCertPath : public CertPath
		{
		private:
			ArrayList<Certificate> _cert;

		public:
			BeeCertPath(const BeeCertificate& cert);
			virtual ~BeeCertPath() {}

			virtual bool equals(const Object* obj) const throw ();

			virtual const List<Certificate>& getCertificates() const;

			virtual const bytearray& getEncoded() const;
		};
	}
}

#endif

#endif
