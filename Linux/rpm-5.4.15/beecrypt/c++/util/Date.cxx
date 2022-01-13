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

#include "beecrypt/timestamp.h"
#include "beecrypt/c++/util/Date.h"

#include <unicode/datefmt.h>

namespace {
	#if WIN32
	__declspec(thread) DateFormat* format = 0;
	#else
	__thread DateFormat* format = 0;
	#endif
}

using namespace beecrypt::util;

Date::Date() throw ()
{
	_time = timestamp();
}

Date::Date(jlong time) throw ()
{
	_time = time;
}

bool Date::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	if (obj)
	{
		const Date* d = dynamic_cast<const Date*>(obj);
		if (d)
			return _time == d->_time;
	}

	return false;
}

bool Date::equals(const Date& d) const throw ()
{
	return _time == d._time;
}

Date* Date::clone() const throw ()
{
	return new Date(_time);
}

jint Date::compareTo(const Date& d) const throw ()
{
	if (_time == d._time)
		return 0;
	else if (_time < d._time)
		return -1;
	else
		return 1;
}

jint Date::hashCode() const throw ()
{
	return (jint) _time ^ (jint)(_time >> 32);
}

bool Date::after(const Date& cmp) const throw ()
{
	return _time > cmp._time;
}

bool Date::before(const Date& cmp) const throw ()
{
	return _time < cmp._time;
}

jlong Date::getTime() const throw ()
{
	return _time;
}

void Date::setTime(jlong time) throw ()
{
	_time = time;
}

String Date::toString() const throw ()
{
	String result;

	if (!format)
		format = DateFormat::createDateTimeInstance();

	UnicodeString tmp;

	result = format->format((UDate) _time, tmp);

	return result;
}
