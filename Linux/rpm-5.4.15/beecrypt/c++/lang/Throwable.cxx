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

#include "beecrypt/c++/lang/Throwable.h"
#include "beecrypt/c++/lang/String.h"
#include "beecrypt/c++/lang/IllegalStateException.h"
using namespace beecrypt::lang;

Throwable::Throwable()
{
	_cause = this;
}

Throwable::Throwable(const char* message) : _msg(message ? new String(message) : 0)
{
	_cause = this;
}

Throwable::Throwable(const String& message) : _msg(new String(message))
{
	_cause = this;
}

Throwable::Throwable(const String* message, const Throwable* cause) : _msg(message ? new String(*message) : 0), _cause(cause)
{
}

Throwable::Throwable(const Throwable* cause) : _cause(cause)
{
	if (_cause && _cause->_msg)
		_msg = new String(*_cause->_msg);
	else
		_msg = 0;
}

Throwable::~Throwable()
{
	delete _msg;
}

const String* Throwable::getMessage() const throw ()
{
	return _msg;
}

const Throwable* Throwable::getCause() const throw ()
{
	return _cause;
}

Throwable& Throwable::initCause(const Throwable& cause)
{
	if (_cause)
		throw IllegalStateException("cause was already specified");

	_cause = &cause;

	return *this;
}
