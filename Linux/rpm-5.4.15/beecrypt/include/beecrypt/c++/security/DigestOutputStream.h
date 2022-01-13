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

/*!\file DigestOutputStream.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_DIGESTOUTPUTSTREAM_H
#define _CLASS_BEE_SECURITY_DIGESTOUTPUTSTREAM_H

#ifdef __cplusplus

#include "beecrypt/c++/io/FilterOutputStream.h"
using beecrypt::io::FilterOutputStream;
#include "beecrypt/c++/security/MessageDigest.h"
using beecrypt::security::MessageDigest;

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class BEECRYPTCXXAPI DigestOutputStream : public FilterOutputStream
		{
		private:
			bool _on;

		protected:
			MessageDigest& digest;

		public:
			DigestOutputStream(OutputStream& out, MessageDigest& m);
			virtual ~DigestOutputStream() {}

			virtual void write(byte b) throw (IOException);
			virtual void write(const byte* data, int offset, int length) throw (IOException);

			void on(bool on);

			MessageDigest& getMessageDigest();
			void setMessageDigest(MessageDigest& m);
		};
	}
}

#endif

#endif

