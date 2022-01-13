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

/*!\file DataOutput.h
 * \ingroup CXX_IO_m
 */

#ifndef _INTERFACE_BEE_IO_DATAOUTPUT_H
#define _INTERFACE_BEE_IO_DATAOUTPUT_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/io/IOException.h"
using beecrypt::io::IOException;

namespace beecrypt {
	namespace io {
		/*!\ingroup CXX_IO_m
		 */
		class BEECRYPTCXXAPI DataOutput
		{
		public:
			virtual ~DataOutput() {}

			virtual void write(const bytearray& b) throw (IOException) = 0;
			virtual void write(const byte* data, jint offset, jint length) throw (IOException) = 0;
			virtual void write(byte v) throw (IOException) = 0;
			virtual void writeBoolean(bool v) throw (IOException) = 0;
			virtual void writeByte(byte v) throw (IOException) = 0;
			virtual void writeChars(const String& s) throw (IOException) = 0;
			virtual void writeInt(jint v) throw (IOException) = 0;
			virtual void writeLong(jlong v) throw (IOException) = 0;
			virtual void writeShort(jshort v) throw (IOException) = 0;
			virtual void writeUTF(const String& str) throw (IOException) = 0;
		};
	}
}

#endif

#endif
