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

/*!\file OutputStream.h
 * \ingroup CXX_IO_m
 */

#ifndef _ABSTRACT_CLASS_BEE_IO_OUTPUTSTREAM_H
#define _ABSTRACT_CLASS_BEE_IO_OUTPUTSTREAM_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::bytearray;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/io/Closeable.h"
using beecrypt::io::Closeable;
#include "beecrypt/c++/io/Flushable.h"
using beecrypt::io::Flushable;

namespace beecrypt {
	namespace io {
		/*!\ingroup CXX_IO_m
		 */
		class BEECRYPTCXXAPI OutputStream : public Object, public virtual Closeable, public virtual Flushable
		{
		public:
			virtual ~OutputStream() {}

			virtual void close() throw (IOException);
			virtual void flush() throw (IOException);
			virtual void write(byte b) throw (IOException) = 0;
			virtual void write(const byte* data, int offset, int length) throw (IOException);
			virtual void write(const bytearray& b) throw (IOException);
		};
	}
}

#endif

#endif
