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

/*!\file ByteBuffer.h
 * \ingroup CXX_NIO_m
 */

#ifndef _CLASS_BYTEBUFFER_H
#define _CLASS_BYTEBUFFER_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::bytearray;
#include "beecrypt/c++/lang/Comparable.h"
using beecrypt::lang::Comparable;
#include "beecrypt/c++/nio/Buffer.h"
using beecrypt::nio::Buffer;
#include "beecrypt/c++/nio/ByteOrder.h"
using beecrypt::nio::ByteOrder;
#include "beecrypt/c++/nio/ReadOnlyBufferException.h"
using beecrypt::nio::ReadOnlyBufferException;

/* Note: be careful; we have to derive a MappedByteBuffer class from this one
 * (which contains an mmap-ed fd)
 */

namespace beecrypt {
	namespace nio {
		class ByteBuffer : public Buffer, public Comparable<ByteBuffer>
		{
		private:
			class fakebytearray : public bytearray
			{
			public:
				~fakebytearray();

				void set(byte* data, size_t size) throw ();
			};

			fakebytearray _fake;
			byte* _data;
			bool _free;

			const ByteOrder* _order;

			ByteBuffer(const byte*, size_t);
			ByteBuffer(size_t capacity) throw (std::bad_alloc);

		public:
			virtual ~ByteBuffer();

			static ByteBuffer* allocate(size_t capacity) throw (std::bad_alloc);
			static ByteBuffer* allocateDirect(size_t capacity) throw (std::bad_alloc);

			bytearray& array() throw (ReadOnlyBufferException, UnsupportedOperationException);
			const bytearray& array() const throw (UnsupportedOperationException);
			size_t arrayOffset() const throw (ReadOnlyBufferException, UnsupportedOperationException);

			ByteBuffer* asReadOnlyBuffer();

			virtual bool isDirect() const throw ();
			bool hasArray() const throw ();

			const ByteOrder& order() const throw ();

			virtual int compareTo(const ByteBuffer& compare) const throw ();
		};
	}
}

#endif

#endif
