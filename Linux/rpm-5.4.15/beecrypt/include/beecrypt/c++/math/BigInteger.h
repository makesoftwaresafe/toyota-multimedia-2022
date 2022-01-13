/*
 * Copyright (c) 2005 Bob Deblier
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

/*!\file BigInteger.h
 * \ingroup CXX_MATH_m
 */

#ifndef _CLASS_BEE_MATH_BIGINTEGER_H
#define _CLASS_BEE_MATH_BIGINTEGER_H

#include "beecrypt/mpbarrett.h"

#ifdef __cplusplus

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;
#include "beecrypt/c++/lang/Number.h"
using beecrypt::lang::Number;
#include "beecrypt/c++/lang/ArithmeticException.h"
using beecrypt::lang::ArithmeticException;
#include "beecrypt/c++/lang/NumberFormatException.h"
using beecrypt::lang::NumberFormatException;

namespace beecrypt {
	namespace math {
		/*!\ingroup CXX_MATH_m
		 */
		class BEECRYPTCXXAPI BigInteger : public Number, public virtual Comparable<BigInteger>
		{
			friend BEECRYPTCXXAPI void transform(mpnumber& m, const BigInteger&);
			friend BEECRYPTCXXAPI void transform(mpbarrett& b, const BigInteger&);

		private:
			size_t size;
			mpw*   data;
			int    sign;

			BigInteger(size_t, mpw*, int);

		public:
			static BigInteger valueOf(jlong val);

			static const BigInteger ZERO;
			static const BigInteger ONE;
			static const BigInteger TEN;

		public:
			BigInteger();
			BigInteger(jlong val);
			BigInteger(const bytearray& val);
			BigInteger(const mpnumber&);
			BigInteger(const mpbarrett&);
			BigInteger(const BigInteger& val);
			virtual ~BigInteger();

			BigInteger& operator=(const BigInteger& copy);

			bool operator==(const BigInteger& cmp) const throw ();
			bool operator!=(const BigInteger& cmp) const throw ();

			virtual jbyte byteValue() const throw ();
			virtual jint compareTo(const BigInteger& val) const throw ();
			virtual bool equals(const Object* obj) const throw ();
			virtual jint hashCode() const throw ();
			virtual jint intValue() const throw ();
			virtual jlong longValue() const throw ();
			virtual jshort shortValue() const throw ();
			virtual String toString() const throw ();

			jint signum() const throw ();

			BigInteger add(const BigInteger& val) const;
			BigInteger subtract(const BigInteger& val) const;
			BigInteger multiply(const BigInteger& val) const;
			BigInteger mod(const BigInteger& val) const throw (ArithmeticException);
			BigInteger modInverse(const BigInteger& m) const throw (ArithmeticException);
		//	BigInteger modPow(const BigInteger& exponent, const BigInteger& m) const throw (ArithmeticException);
			BigInteger negate() const;

			bytearray* toByteArray() const;
			void toByteArray(bytearray& b) const;
			String toString(jint radix) const;
		};
	}
}

#endif

#endif
