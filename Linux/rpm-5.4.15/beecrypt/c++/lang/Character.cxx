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

#include "beecrypt/c++/lang/Character.h"

using namespace beecrypt::lang;

const jchar Character::MIN_VALUE = (jchar) 0;
const jchar Character::MAX_VALUE = (jchar) 0xFFFF;

const jchar Character::MIN_HIGH_SURROGATE = (jchar) 0xD800;
const jchar Character::MAX_HIGH_SURROGATE = (jchar) 0xDBFF;
const jchar Character::MIN_LOW_SURROGATE = (jchar) 0xDC00;
const jchar Character::MAX_LOW_SURROGATE = (jchar) 0xDFFF;
const jchar Character::MIN_SURROGATE = MIN_HIGH_SURROGATE;
const jchar Character::MAX_SURROGATE = MAX_LOW_SURROGATE;

const jint Character::MIN_SUPPLEMENTARY_CODE_POINT = 0x10000;
const jint Character::MIN_CODE_POINT = 0;
const jint Character::MAX_CODE_POINT = 0x10FFFF;

const jint Character::MIN_RADIX = 2;
const jint Character::MAX_RADIX = 36;

String Character::toString(jchar c) throw ()
{
	return String(&c, 0, 1);
}

bool Character::isHighSurrogate(jchar c) throw ()
{
	return c >= MIN_HIGH_SURROGATE && c <= MAX_HIGH_SURROGATE;
}

bool Character::isLowSurrogate(jchar c) throw ()
{
	return c >= MIN_LOW_SURROGATE && c <= MAX_LOW_SURROGATE;
}

Character::Character(jchar value) throw () : _val(value)
{
}

jint Character::hashCode() const throw ()
{
	return _val;
}

jint Character::compareTo(const Character& c) const throw ()
{
	if (_val == c._val)
		return 0;
	else if (_val < c._val)
		return -1;
	else
		return 1;
}

String Character::toString() const throw ()
{
	return String(_val);
}
