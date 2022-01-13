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

/*!\file CertPathParameters.h
 * \ingroup CXX_SECURITY_CERT_m
 */

#ifndef _INTERFACE_BEE_SECURITY_CERT_CERTPATHPARAMETERS_H
#define _INTERFACE_BEE_SECURITY_CERT_CERTPATHPARAMETERS_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;

namespace beecrypt {
	namespace security {
		namespace cert {
			/*!\ingroup CXX_SECURITY_CERT_m
			 */
			class BEECRYPTCXXAPI CertPathParameters : public virtual Cloneable
			{
			public:
				virtual ~CertPathParameters() {}
			};
		}
	}
}

#endif

#endif
