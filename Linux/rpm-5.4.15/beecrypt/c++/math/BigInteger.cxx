/*
 * Copyright (c) 2005 Beeyond Software Holding BV
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

#include "beecrypt/c++/math/BigInteger.h"
#include "beecrypt/c++/lang/Character.h"
using beecrypt::lang::Character;
#include "beecrypt/c++/lang/StringBuilder.h"
using beecrypt::lang::StringBuilder;
#include "beecrypt/c++/lang/ArithmeticException.h"
using beecrypt::lang::ArithmeticException;
#include "beecrypt/c++/lang/OutOfMemoryError.h"
using beecrypt::lang::OutOfMemoryError;

#include <cstdlib>

using namespace beecrypt::math;

namespace {
	const String STRZERO("0");
}

const BigInteger BigInteger::ZERO;
const BigInteger BigInteger::ONE(1);
const BigInteger BigInteger::TEN(10);

BigInteger::BigInteger() : size(0), data(0), sign(0)
{
}

BigInteger::BigInteger(jlong val)
{
	if (val == 0)
	{
		size = 0;
		data = 0;
		sign = 0;
	}
	else
	{
		if (val < 0)
		{
			sign = -1;
			val = -val;
		}
		else
			sign = 1;

		if (sizeof(jlong) == sizeof(mpw))
			size = 1;
		else
			size = 2;

		data = (mpw*) malloc(size * sizeof(mpw));
		if (data == 0)
			throw OutOfMemoryError();

		if (sizeof(jlong) == sizeof(mpw))
		{
			data[0] = val;
		}
		else
		{
			data[0] = (val >> 32);
			data[1] = (val      );
		}
	}
}

BigInteger::BigInteger(size_t size, mpw* data, int sign) : size(size), data(data), sign(sign)
{
}

BigInteger::BigInteger(const bytearray& val)
{
	if (val.size() == 0)
	{
		size = 0;
		data = 0;
		sign = 0;
	}
	else
	{
		int skip = 0;
		while ((skip < val.size()) && (val[skip] == 0))
			skip++;

		size = MP_BYTES_TO_WORDS(val.size() - skip + MP_WBYTES - 1);

		data = (mpw*) malloc(size * sizeof(mpw));
		if (data == 0)
			throw OutOfMemoryError();

		os2ip(data, size, val.data(), val.size());

		if (val[0] & 0x80)
		{
			mpneg(size, data);
			sign = -1;
		}
		else
			sign = 1;
	}
}

BigInteger::BigInteger(const mpnumber& n)
{
	if (mpz(n.size, n.data))
	{
		size = 0;
		data = 0;
		sign = 0;
	}
	else
	{
		size_t sigbits = mpbits(n.size, n.data);

		size = MP_BITS_TO_WORDS(sigbits + MP_WBITS - 1);
		data = (mpw*) malloc(size * sizeof(mpw));
		if (data == 0)
			throw new OutOfMemoryError();

		// eliminate zero most-significant-words
		mpcopy(size, data, n.data + n.size - size);

		sign = 1;
	}
}

BigInteger::BigInteger(const mpbarrett& b)
{
	if (mpz(b.size, b.modl))
	{
		size = 0;
		data = 0;
		sign = 0;
	}
	else
	{
		size_t sigbits = mpbits(b.size, b.modl);

		size = MP_BITS_TO_WORDS(sigbits + MP_WBITS - 1);
		data = (mpw*) malloc(size * sizeof(mpw));
		if (data == 0)
			throw new OutOfMemoryError();

		// eliminate zero most-significant-words
		mpcopy(size, data, b.modl + b.size - size);

		sign = 1;
	}
}

BigInteger::BigInteger(const BigInteger& copy) : size(copy.size), sign(copy.sign)
{
	if (sign)
	{
		data = (mpw*) malloc(size * sizeof(mpw));
		if (data == 0)
			throw OutOfMemoryError();
		mpcopy(size, data, copy.data);
	}
	else
		data = 0;
}

BigInteger::~BigInteger()
{
	if (sign)
		free(data);
}

BigInteger& BigInteger::operator=(const BigInteger& copy)
{
	if (copy.sign == 0)
	{
		if (sign)
		{
			delete data;
			size = 0;
			data = 0;
			sign = 0;
		}
	}
	else
	{
		if (size != copy.size)
		{
			data = (mpw*) realloc(data, (size = copy.size) * sizeof(mpw));
			if (data == 0)
				throw OutOfMemoryError();
		}
		mpcopy(size, data, copy.data);
		sign = copy.sign;
	}

	return *this;
}

bool BigInteger::operator==(const BigInteger& val) const throw ()
{
	return (sign == val.sign) && ((sign == 0) || mpeqx(size, data, val.size, val.data));
}

bool BigInteger::operator!=(const BigInteger& val) const throw ()
{
	return (sign != val.sign) || ((sign != 0) && mpnex(size, data, val.size, val.data));
}

BigInteger BigInteger::valueOf(jlong val)
{
	return BigInteger(val);
}

jint BigInteger::hashCode() const throw ()
{
	return 0;
}

jbyte BigInteger::byteValue() const throw ()
{
	return (jbyte) (sign * data[size-1]);
}

jshort BigInteger::shortValue() const throw ()
{
	return (jshort) (sign * data[size-1]);
}

jint BigInteger::intValue() const throw ()
{
	return (jint) (sign * data[size-1]);
}

jlong BigInteger::longValue() const throw ()
{
	#if MP_WBITS == 64
	return (jlong) (sign * data[size-1]);
	#else
	if (size == 1)
		return (jlong) (sign * data[size-1]);
	else
		return (jlong) (sign * ((data[size-2] << 32) + data[size-1]));
	#endif
}

jint BigInteger::compareTo(const BigInteger& val) const throw ()
{
	if (sign == val.sign)
	{
		if (sign == 0)
			return 0;

		return sign * mpcmpx(size, data, val.size, val.data);
	}
	else
		return (sign > val.sign) ? 1 : -1;
}

bool BigInteger::equals(const Object* obj) const throw ()
{
	if (this == obj)
		return true;

	const BigInteger* cmp = dynamic_cast<const BigInteger*>(obj);
	if (cmp)
	{
		if (sign != cmp->sign)
			return false;

		if (sign && mpnex(size, data, cmp->size, cmp->data))
			return false;

		return true;
	}

	return false;
}

String BigInteger::toString() const throw ()
{
	return toString(10);
}

String BigInteger::toString(int radix) const
{
	if (radix < Character::MIN_RADIX || radix > Character::MAX_RADIX)
		radix = 10;

	if (sign == 0)
		return STRZERO;

	StringBuilder tmp;

	/* allocate enough space to hold a copy of this (size+1), result (size+2) and workspace (2) */
	mpw* rdata = (mpw*) malloc((2*size+5) * sizeof(mpw));
	if (rdata)
	{
		mpw* result = rdata+size+1;
		mpw* wksp = result+size+2;
		mpw nradix = radix;

		mpsetx(size+1, rdata, size, data);

		size_t shift = mpnorm(1, &nradix);
		mplshift(size+1, rdata, shift);

		do
		{
			int remainder;

			mpndivmod(result, size+1, rdata, 1, &nradix, wksp);
			remainder = (result[size+1] >> shift);
			mpcopy(size+1, rdata, result);
			mplshift(size+1, rdata, shift);
			if (remainder < 10)
				tmp.append((jchar)(48 + remainder));
			else
				tmp.append((jchar)(55 + remainder));
		} while (mpnz(size+1, rdata));

		free(rdata);

		if (sign < 0)
			tmp.append('-');

		return tmp.reverse().toString();
	}
	else
		throw OutOfMemoryError();
}

jint BigInteger::signum() const throw ()
{
	return sign;
}

BigInteger BigInteger::add(const BigInteger& val) const
{
	if (val.sign == 0)
		return *this;

	if (sign == 0)
		return val;

	if (sign == val.sign)
	{
		size_t rsize = size > val.size ? size : val.size;
		// allocate one extra word for addition carry-over
		mpw* rdata = (mpw*) malloc((rsize+1) * sizeof(mpw));
		if (rdata == 0)
			throw OutOfMemoryError();

		mpsetx(rsize, rdata, size, data);
		if (mpaddx(rsize, rdata, val.size, val.data))
		{
			// there was a carry-over; move result up by one word
			mpmove(rsize, rdata+1, rdata);
			rsize++;
			rdata[0] = 1;
		}

		return BigInteger(rsize, rdata, sign);
	}
	else
	{
		int cmp = mpcmpx(size, data, val.size, val.data);
		if (cmp == 0)
			return ZERO;

		// subtract the smallest from the biggest value

		size_t rsize = (cmp < 0) ? val.size : size;
		mpw* rdata = (mpw*) malloc(rsize * sizeof(mpw));
		if (rdata == 0)
			throw OutOfMemoryError();

		if (cmp < 0)
		{
			mpcopy(rsize, rdata, val.data);
			mpsubx(rsize, rdata, size, data);
		}
		else
		{
			mpcopy(rsize, rdata, data);
			mpsubx(rsize, rdata, val.size, val.data);
		}

		size_t skip = 0;

		while ((skip < rsize) && rdata[skip] == 0)
			skip++;

		if (skip == rsize)
		{
			free(rdata);
			return ZERO;
		}

		if (skip)
		{
			rsize -= skip;
			mpmove(rsize, rdata, rdata+skip);
		}

		return BigInteger(rsize, rdata, cmp * sign);
	}
}

BigInteger BigInteger::subtract(const BigInteger& val) const
{
	if (sign == 0)
		return val.negate();

	if (val.sign == 0)
		return *this;

	if (sign != val.sign)
	{
		size_t rsize = size > val.size ? size : val.size;
		// allocate one extra word for addition carry-over
		mpw* rdata = (mpw*) malloc((rsize+1) * sizeof(mpw));
		if (rdata == 0)
			throw OutOfMemoryError();

		mpsetx(rsize, rdata, size, data);
		if (mpaddx(rsize, rdata, val.size, val.data))
		{
			// there was a carry-over; move result up by one word
			mpmove(rsize, rdata+1, rdata);
			rsize++;
			rdata[0] = 1;
		}

		return BigInteger(rsize, rdata, sign);
	}
	else
	{
		int cmp = mpcmpx(size, data, val.size, val.data);
		if (cmp == 0)
			return ZERO;

		// subtract the smallest from the biggest value, so we don't get a carry
		size_t rsize = (cmp < 0) ? val.size : size;
		mpw* rdata = (mpw*) malloc(rsize * sizeof(mpw));
		if (rdata == 0)
			throw OutOfMemoryError();

		if (cmp < 0)
		{
			mpcopy(rsize, rdata, val.data);
			mpsubx(rsize, rdata, size, data);
		}
		else
		{
			mpcopy(rsize, rdata, data);
			mpsubx(rsize, rdata, val.size, val.data);
		}

		size_t skip = 0;
		while ((skip < rsize) && (rdata[skip] == 0))
			skip++;

		if (skip)
		{
			rsize -= skip;
			mpmove(rsize, rdata, rdata+skip);
		}

		return BigInteger(rsize, rdata, cmp * sign);
	}
}

BigInteger BigInteger::multiply(const BigInteger& val) const
{
	if (sign == 0 || val.sign == 0)
		return BigInteger();

	size_t rsize = size + val.size;
	mpw* rdata = (mpw*) malloc(rsize * sizeof(mpw));
	if (rdata == 0)
		throw OutOfMemoryError();

	mpmul(rdata, size, data, val.size, val.data);
	if (rdata[0] == 0)
	{
		rsize--;
		mpmove(rsize, rdata, rdata+1);
	}

	return BigInteger(rsize, rdata, sign * val.sign);
}

#if 0
BigInteger BigInteger::mod(const BigInteger& m) const throw (ArithmeticException)
{
	if (m.compareTo(ZERO) <= 0)
		throw ArithmeticException("m must be > 0");

	if (mpltx(size, data, m.size, m.data))
	{
		if (sign == -1)
			return m.subtract(*this);
		else
			return *this;
	}
	else
	{
		mpw* tmp = (mpw*) malloc(size + 2*m.size+1);
		if (tmp == 0)
			throw OutOfMemoryError();

		mpmod(tmp, size, data, m.size, m.data, tmp+size);
		if (sign == -1)
		{
			mpneg(size, tmp);
			mpaddx(size, tmp, m.size, m.data);
		}

		size_t skip = 0;

		while ((skip < size) && (tmp[skip] == 0))
			skip++;

		if (skip == size)
		{
			free(tmp);
			return ZERO;
		}

		if (skip)
			mpmove(size - skip, tmp, tmp + skip);

		return BigInteger(size - skip, tmp, 1);
	}
}

BigInteger BigInteger::modPow(const BigInteger& exponent, const BigInteger& m) const
{
	// if the modulus is not positive, bail out
	if (m.sign <= 0)
		throw ArithmeticException("modulus must be > 0");

	// this ^ 0 mod m is 1 except when m == 1, when it is 0
	// 1 ^ exponent mod m is 1 except when m == 1, when it is 0
	if (exponent.sign == 0 || equals(&ONE))
	{
		if (m.equals(&ONE))
			return ZERO;
		else
			return ONE;
	}

	// 0 ^ exponent mod m is 0 when exponent != 0 (which was already excluded earlier)
	if (equals(&ZERO))
		return ZERO;

	// we need to bring this into the range 0 .. (m-1)

	// if the exponent is negative, compute the modular inverse of this before 
}
#endif

BigInteger BigInteger::negate() const
{
	if (mpz(size, data))
		return BigInteger();

	mpw* rdata = (mpw*) malloc(size * sizeof(mpw));
	if (rdata == 0)
		throw OutOfMemoryError();

	mpcopy(size, rdata, data);

	return BigInteger(size, rdata, -sign);
}

bytearray* BigInteger::toByteArray() const
{
	bytearray* result = new bytearray();

	toByteArray(*result);

	return result;
}

void BigInteger::toByteArray(bytearray& b) const
{
	if (sign == 0)
	{
		b.resize(1);
		b[0] = 0;
	}
	else if (sign == 1)
	{
		size_t sigbits = mpbits(size, data);
		size_t req = (sigbits+8) >> 3;

		b.resize(req);

		i2osp(b.data(), req, data, size);
	}
	else
	{
		size_t sigbits = mpbits(size, data);
		size_t req = (sigbits+7) >> 3;

		b.resize(req);

		mpw* tmp = (mpw*) malloc(size * sizeof(mpw));
		if (tmp == 0)
			throw OutOfMemoryError();

		mpcopy(size, tmp, data);
		mpneg(size, tmp);

		i2osp(b.data(), req, tmp, size);

		free(tmp);
	}
}

#if 0
void beecrypt::math::transform(BigInteger& b, const mpnumber& n)
{
	if (mpz(n.size, n.data))
	{
		if (b.sign)
		{
			free(b.data);
			b.size = 0;
			b.data = 0;
			b.sign = 0;
		}
	}
	else
	{
		if (b.size != n.size)
		{
			b.data = (mpw*) realloc(b.data, b.size * sizeof(mpw));
			if (b.data == 0)
				throw OutOfMemoryError();
		}
		mpcopy(b.size, b.data, n.data);
		b.sign = 1;
	}
}
#endif

void beecrypt::math::transform(mpnumber& n, const BigInteger& val)
{
	switch (val.sign)
	{
	case 0:
		mpnfree(&n);
		break;
	case 1:
		mpnset(&n, val.size, val.data);
		break;
	default:
		throw IllegalArgumentException("can only transform non-negative numbers");
	}
}

void beecrypt::math::transform(mpbarrett& b, const BigInteger& val)
{
	switch (val.sign)
	{
	case 0:
		mpbfree(&b);
		break;
	case 1:
		mpbset(&b, val.size, val.data);
		break;
	default:
		throw IllegalArgumentException("can only transform non-negative numbers");
	}
}
