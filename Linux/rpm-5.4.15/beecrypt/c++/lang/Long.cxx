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

#include "beecrypt/c++/lang/Long.h"
#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

#include <unicode/numfmt.h>

using namespace beecrypt::lang;

const jlong Long::MIN_VALUE = (((jlong) 1) << 63);
const jlong Long::MAX_VALUE = ~MIN_VALUE;

String Long::toString(jlong l) throw ()
{
	char tmp[21];

	#if WIN32
	sprintf(tmp, "%I64d", l);
	#elif SIZE_LONG == 8
	sprintf(tmp, "%ld", (long)l);
	#elif HAVE_LONG_LONG
	sprintf(tmp, "%lld", (long long)l);
	#else
	# error
	#endif

	return String(tmp);
}

String Long::toHexString(jlong l) throw ()
{
	char tmp[18];

	#if WIN32
	sprintf(tmp, "%I64x", l);
	#elif SIZEOF_LONG == 8
	sprintf(tmp, "%lx", (long)l);
	#elif HAVE_LONG_LONG
	sprintf(tmp, "%llx", (long long)l);
	#else
	# error
	#endif

	return String(tmp);
}

String Long::toOctalString(jlong l) throw ()
{
	char tmp[23];

	#if WIN32
	sprintf(tmp, "%I64o", l);
	#elif SIZEOF_LONG == 8
	sprintf(tmp, "%lo", (long)l);
	#elif HAVE_LONG_LONG
	sprintf(tmp, "%llo", (long long)l);
	#else
	# error
	#endif

	return String(tmp);
}

jlong Long::parseLong(const String& s) throw (NumberFormatException)
{
	UErrorCode status = U_ZERO_ERROR;

	NumberFormat* nf = NumberFormat::createInstance(status);

	if (nf)
	{
		Formattable fmt((int64_t) 0);

		nf->parse(s.toUnicodeString(), fmt, status);

		delete nf;

		if (U_FAILURE(status))
			throw NumberFormatException("unable to parse string to jlong value");

		return fmt.getInt64();
	}
	else
		throw RuntimeException("unable to create ICU NumberFormat instance");
}

Long::Long(jlong value) throw () : _val(value)
{
}

Long::Long(const String& s) throw (NumberFormatException) : _val(parseLong(s))
{
}

jint Long::hashCode() const throw ()
{
	return (jint) _val ^ (jint)(_val >> 32);
}

jbyte Long::byteValue() const throw ()
{
	return (jbyte) _val;
}

jshort Long::shortValue() const throw ()
{
	return (jshort) _val;
}

jint Long::intValue() const throw ()
{
	return (jint) _val;
}

jlong Long::longValue() const throw ()
{
	return _val;
}

jint Long::compareTo(const Long& l) const throw ()
{
	if (_val == l._val)
		return 0;
	else if (_val < l._val)
		return -1;
	else
		return 1;
}

String Long::toString() const throw ()
{
	char tmp[21];

	#if WIN32
	sprintf(tmp, "%I64d", _val);
	#elif SIZE_LONG == 8
	sprintf(tmp, "%ld", (long)_val);
	#elif HAVE_LONG_LONG
	sprintf(tmp, "%lld", (long long)_val);
	#else
	# error
	#endif

	return String(tmp);
}
