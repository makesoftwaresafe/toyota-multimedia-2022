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

/*!\file DataInput.h
 * \ingroup CXX_IO_m
 */

#ifndef _INTERFACE_BEE_IO_DATAINPUT_H
#define _INTERFACE_BEE_IO_DATAINPUT_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/io/IOException.h"
using beecrypt::io::IOException;

namespace beecrypt {
	namespace io {
		/*!\ingroup CXX_IO_m
		 */
		class BEECRYPTCXXAPI DataInput
		{
		public:
			virtual ~DataInput() {}

			virtual bool readBoolean() throw (IOException) = 0;
			virtual jbyte readByte() throw (IOException) = 0;
			virtual jchar readChar() throw (IOException) = 0;
			virtual void readFully(byte* data, jint offset, jint length) = 0;
			virtual void readFully(bytearray& b) = 0;
			virtual jint readInt() throw (IOException) = 0;
			virtual String readLine() throw (IOException) = 0;
			virtual jlong readLong() throw (IOException) = 0;
			virtual jshort readShort() throw (IOException) = 0;
			virtual jint readUnsignedByte() throw (IOException) = 0;
			virtual jint readUnsignedShort() throw (IOException) = 0;
			virtual String readUTF() throw (IOException) = 0;
			virtual jint skipBytes(jint n) throw (IOException) = 0;
		};
	}
}

#endif

#endif
