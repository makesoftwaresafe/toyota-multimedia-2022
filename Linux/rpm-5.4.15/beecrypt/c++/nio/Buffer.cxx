/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/nio/Buffer.h"
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

using namespace beecrypt::nio;

Buffer::Buffer(size_t capacity, size_t limit, bool readonly)
{
	_capacity = capacity;
	_limit = limit;
	_position = 0;
	_marked = false;
	_readonly = readonly;
}

size_t Buffer::capacity() const throw ()
{
	return _capacity;
}

Buffer& Buffer::clear() throw ()
{
	_limit = _capacity;
	_position = 0;
	_marked = false;

	return *this;
}

Buffer& Buffer::flip() throw ()
{
	_limit = _position;
	_position = 0;
	_marked = false;

	return *this;
}

bool Buffer::hasRemaining() const throw ()
{
	return (_position < _limit);
}

bool Buffer::isReadOnly() const throw ()
{
	return _readonly;
}

Buffer& Buffer::limit(size_t newLimit) throw (IllegalArgumentException)
{
	if (newLimit > _capacity)
		throw IllegalArgumentException("limit greater than capacity");

	_limit = newLimit;

	if (_marked && (_mark > _limit))
		_marked = false;

	return *this;
}

Buffer& Buffer::mark() throw ()
{
	_marked = true;
	_mark = _position;

	return *this;
}

size_t Buffer::position() const throw ()
{
	return _position;
}

Buffer& Buffer::position(size_t newPosition) throw (IllegalArgumentException)
{
	if (newPosition > _limit)
		throw IllegalArgumentException("position greater than limit");

	_position = newPosition;

	if (_marked && (_mark > _limit))
		_marked = false;

	return *this;
}

size_t Buffer::remaining() const throw ()
{
	return (_limit - _position);
}

Buffer& Buffer::reset() throw (InvalidMarkException)
{
	if (!_marked)
		throw InvalidMarkException();

	_position = _mark;

	return *this;
}

Buffer& Buffer::rewind() throw ()
{
	_position = _mark = 0;

	return *this;
}
