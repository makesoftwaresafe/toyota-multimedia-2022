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

/*!\file Buffer.h
 * \ingroup CXX_NIO_m
 */

#ifndef _ABSTRACT_CLASS_BEE_NIO_BUFFER_H
#define _ABSTRACT_CLASS_BEE_NIO_BUFFER_H

#ifdef __cplusplus

#include "beecrypt/c++/lang/IllegalArgumentException.h"
using beecrypt::lang::IllegalArgumentException;
#include "beecrypt/c++/lang/Object.h"
using beecrypt::lang::Object;
#include "beecrypt/c++/nio/InvalidMarkException.h"
using beecrypt::nio::InvalidMarkException;

namespace beecrypt {
	namespace nio {
		class BEECRYPTCXXAPI Buffer : public Object
		{
		protected:
			size_t _capacity;
			size_t _limit;
			size_t _mark;
			size_t _position;
			bool   _marked;
			bool   _readonly;

			Buffer(size_t capacity, size_t limit, bool readonly);

		public:
			virtual ~Buffer() {};

			size_t capacity() const throw ();
			Buffer& clear() throw ();
			Buffer& flip() throw ();
			bool hasRemaining() const throw ();
			virtual bool isReadOnly() const throw () = 0;
			size_t limit() const throw ();
			Buffer& limit(size_t newLimit) throw (IllegalArgumentException);
			Buffer& mark() throw ();
			size_t position() const throw ();
			Buffer& position(size_t newPosition) throw (IllegalArgumentException);
			size_t remaining() const throw ();
			Buffer& reset() throw (InvalidMarkException);
			Buffer& rewind() throw ();
		};
	}
}

#endif

#endif
