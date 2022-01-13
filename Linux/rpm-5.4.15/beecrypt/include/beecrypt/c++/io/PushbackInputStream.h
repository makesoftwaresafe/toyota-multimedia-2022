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

/*!\file PushbackInputStream.h
 * \ingroup CXX_IO_m
 */

#ifndef _CLASS_BEE_IO_PUSHBACKINPUTSTREAM_H
#define _CLASS_BEE_IO_PUSHBACKINPUTSTREAM_H

#ifdef __cplusplus

#include "beecrypt/c++/io/FilterInputStream.h"
using beecrypt::io::FilterInputStream;

namespace beecrypt {
	namespace io {
		/*!\ingroup CXX_IO_m
		 */
		class BEECRYPTCXXAPI PushbackInputStream : public FilterInputStream
		{
		private:
			bool _closed;

		protected:
			bytearray buf;
			jint pos;

		public:
			PushbackInputStream(InputStream& in, jint size = 1);
			virtual ~PushbackInputStream();

			virtual jint available() throw (IOException);
			virtual void close() throw (IOException);
			virtual bool markSupported() throw ();
			virtual jint read() throw (IOException);
			virtual jint read(byte* data, jint offset, jint length) throw (IOException);
			virtual jint skip(jint n) throw (IOException);

			void unread(byte) throw (IOException);
			void unread(const byte* data, jint offset, jint length) throw (IOException);
			void unread(const bytearray& b) throw (IOException);
		};
	}
}

#endif

#endif
