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

#include "beecrypt/c++/lang/Integer.h"
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

#include <unicode/numfmt.h>

using namespace beecrypt::lang;

const jint Integer::MIN_VALUE = (((jint) 1) << 31);
const jint Integer::MAX_VALUE = ~MIN_VALUE;

String Integer::toString(jint i) throw ()
{
	char tmp[12];

	#if SIZE_LONG == 4
	sprintf(tmp, "%d", (int)i);
	#else
	sprintf(tmp, "%ld", (long)i);
	#endif

	return String(tmp);
}

String Integer::toHexString(jint i) throw ()
{
	char tmp[10];

	#if SIZEOF_LONG == 4
	sprintf(tmp, "%x", (int)i);
	#else
	sprintf(tmp, "%lx", (long)i);
	#endif

	return String(tmp);
}

String Integer::toOctalString(jint i) throw ()
{
	char tmp[13];

	#if SIZEOF_INT == 4
	sprintf(tmp, "%o", (int)i);
	#else
	sprintf(tmp, "%lo", (long)i);
	#endif

	return String(tmp);
}

jint Integer::parseInteger(const String& s) throw (NumberFormatException)
{
	UErrorCode status = U_ZERO_ERROR;

	NumberFormat* nf = NumberFormat::createInstance(status);

	if (nf)
	{
		Formattable fmt((int32_t) 0);

		nf->parse(s.toUnicodeString(), fmt, status);

		delete nf;

		if (U_FAILURE(status))
			throw NumberFormatException("unable to parse string to jint value");

		return fmt.getLong();
	}
	else
		throw RuntimeException("unable to create ICU NumberFormat instance");
}

Integer::Integer(jint value) throw () : _val(value)
{
}

Integer::Integer(const String& s) throw (NumberFormatException) : _val(parseInteger(s))
{
}

jint Integer::hashCode() const throw ()
{
	return _val;
}

jbyte Integer::byteValue() const throw ()
{
	return (jbyte) _val;
}

jshort Integer::shortValue() const throw ()
{
	return (jshort) _val;
}

jint Integer::intValue() const throw ()
{
	return _val;
}

jlong Integer::longValue() const throw ()
{
	return (jlong) _val;
}

jint Integer::compareTo(const Integer& i) const throw ()
{
	if (_val == i._val)
		return 0;
	else if (_val < i._val)
		return -1;
	else
		return 1;
}

String Integer::toString() const throw ()
{
	char tmp[12];

	#if SIZE_LONG == 4
	sprintf(tmp, "%d", (int)_val);
	#else
	sprintf(tmp, "%ld", (long)_val);
	#endif

	return String(tmp);
}
