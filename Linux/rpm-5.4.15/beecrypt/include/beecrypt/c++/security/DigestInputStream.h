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

/*!\file DigestInputStream.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_DIGESTINPUTSTREAM_H
#define _CLASS_BEE_SECURITY_DIGESTINPUTSTREAM_H

#ifdef __cplusplus

#include "beecrypt/c++/io/FilterInputStream.h"
using beecrypt::io::FilterInputStream;
#include "beecrypt/c++/security/MessageDigest.h"
using beecrypt::security::MessageDigest;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI DigestInputStream : public FilterInputStream
		{
		private:
			bool _on;

		protected:
			MessageDigest& digest;

		public:
			DigestInputStream(InputStream& in, MessageDigest& m);
			virtual ~DigestInputStream() {}

			virtual int read() throw (IOException);
			virtual int read(byte* data, int offset, int length) throw (IOException);

			void on(bool on);

			MessageDigest& getMessageDigest();
			void setMessageDigest(MessageDigest& m);
		};
	}
}

#endif

#endif

