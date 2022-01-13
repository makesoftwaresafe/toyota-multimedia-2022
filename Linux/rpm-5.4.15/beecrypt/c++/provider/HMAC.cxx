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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/pkcs12.h"

#include "beecrypt/c++/crypto/interfaces/PBEKey.h"
using beecrypt::crypto::interfaces::PBEKey;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;
#include "beecrypt/c++/provider/HMAC.h"

using namespace beecrypt::provider;

HMAC::HMAC(const keyedHashFunction& mac, const hashFunction& hash) : _digest(mac.digestsize), _ctxt(&mac)
{
	_kf = &mac;
	_hf = &hash;
}

const bytearray& HMAC::engineDoFinal()
{
	keyedHashFunctionContextDigest(&_ctxt, _digest.data());

	return _digest;
}

int HMAC::engineDoFinal(byte* data, int offset, int length) throw (ShortBufferException)
{
	if (!data)
		throw NullPointerException();

	if (length < _digest.size())
		throw ShortBufferException();

	keyedHashFunctionContextDigest(&_ctxt, data);

	return _digest.size();
}

int HMAC::engineGetMacLength()
{
	return _digest.size();
}

void HMAC::engineReset()
{
	keyedHashFunctionContextReset(&_ctxt);
}

void HMAC::engineUpdate(byte b)
{
	keyedHashFunctionContextUpdate(&_ctxt, &b, 1);
}

void HMAC::engineUpdate(const byte* data, int offset, int length)
{
	keyedHashFunctionContextUpdate(&_ctxt, data+offset, length);
}

void HMAC::engineInit(const Key& key, const AlgorithmParameterSpec* spec) throw (InvalidKeyException, InvalidAlgorithmParameterException)
{
	if (spec)
		throw InvalidAlgorithmParameterException("No AlgorithmParameterSpec supported");

#if 0 // key derivation should be done by caller; we can't know whether it's PKCS#5 or PKCS#12
	const PBEKey* pbe = dynamic_cast<const PBEKey*>(&key);
	if (pbe)
	{
		bytearray _rawk, _salt, _mack(_digest.size());
		int _iter;

		if (pbe->getEncoded())
			_rawk = *(pbe->getEncoded());
		else
			throw InvalidKeyException("PBEKey must have an encoding");

		if (pbe->getSalt())
			_salt = *(pbe->getSalt());

		_iter = pbe->getIterationCount();

		if (pkcs12_derive_key(_hf, PKCS12_ID_MAC, _rawk.data(), _rawk.size(), _salt.data(), _salt.size(), _iter, _mack.data(), _mack.size()))
			throw InvalidKeyException("pkcs12_derive_key returned error");

		keyedHashFunctionContextSetup(&_ctxt, _mack.data(), _mack.size() << 3);

		return;
	}
#endif

	const SecretKey* sec = dynamic_cast<const SecretKey*>(&key);
	if (sec)
	{
		bytearray _rawk;
		if (sec->getEncoded())
			_rawk = *(sec->getEncoded());
		else
			throw InvalidKeyException("SecretKey must have an encoding");

		keyedHashFunctionContextSetup(&_ctxt, _rawk.data(), _rawk.size() << 3);

		return;
	}

	throw InvalidKeyException("expected a SecretKey or a PBEKey");
}
