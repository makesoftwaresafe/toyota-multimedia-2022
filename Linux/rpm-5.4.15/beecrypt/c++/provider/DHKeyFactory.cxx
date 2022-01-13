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

#include "beecrypt/c++/provider/DHKeyFactory.h"
#include "beecrypt/c++/provider/DHPrivateKeyImpl.h"
#include "beecrypt/c++/provider/DHPublicKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayInputStream.h"
using beecrypt::io::ByteArrayInputStream;
#include "beecrypt/c++/security/KeyFactory.h"
using beecrypt::security::KeyFactory;
#include "beecrypt/c++/crypto/spec/DHPrivateKeySpec.h"
using beecrypt::crypto::spec::DHPrivateKeySpec;
#include "beecrypt/c++/crypto/spec/DHPublicKeySpec.h"
using beecrypt::crypto::spec::DHPublicKeySpec;
#include "beecrypt/c++/beeyond/AnyEncodedKeySpec.h"
using beecrypt::beeyond::AnyEncodedKeySpec;
#include "beecrypt/c++/beeyond/BeeInputStream.h"
using beecrypt::beeyond::BeeInputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_DH("DH");

	DHPrivateKey* generatePrivate(const bytearray& enc)
	{
		try
		{
			ByteArrayInputStream bis(enc);
			BeeInputStream bee(bis);

			BigInteger p, g, x;

			p = bee.readBigInteger();
			g = bee.readBigInteger();
			x = bee.readBigInteger();

			return new DHPrivateKeyImpl(p, g, x);
		}
		catch (IOException&)
		{
		}
		return 0;
	}

	DHPublicKey* generatePublic(const bytearray& enc)
	{
		try
		{
			ByteArrayInputStream bis(enc);
			BeeInputStream bee(bis);

			BigInteger p, g, y;

			p = bee.readBigInteger();
			g = bee.readBigInteger();
			y = bee.readBigInteger();

			return new DHPublicKeyImpl(p, g, y);
		}
		catch (IOException&)
		{
		}
		return 0;
	}
}

DHKeyFactory::DHKeyFactory()
{
}

PrivateKey* DHKeyFactory::engineGeneratePrivate(const KeySpec& spec) throw (InvalidKeySpecException)
{
	const DHPrivateKeySpec* dh = dynamic_cast<const DHPrivateKeySpec*>(&spec);
	if (dh)
	{
		return new DHPrivateKeyImpl(dh->getP(), dh->getG(), dh->getX());
	}

	const EncodedKeySpec* enc = dynamic_cast<const EncodedKeySpec*>(&spec);
	if (enc)
	{
		if (enc->getFormat().equals(FORMAT_BEE))
		{
			DHPrivateKey* pri = generatePrivate(enc->getEncoded());
			if (pri)
				return pri;

			throw InvalidKeySpecException("Invalid KeySpec encoding");
		}
		throw InvalidKeySpecException("Unsupported KeySpec format");
	}
	throw InvalidKeySpecException("Unsupported KeySpec type");
}

PublicKey* DHKeyFactory::engineGeneratePublic(const KeySpec& spec) throw (InvalidKeySpecException)
{
	const DHPublicKeySpec* dh = dynamic_cast<const DHPublicKeySpec*>(&spec);
	if (dh)
	{
		return new DHPublicKeyImpl(dh->getP(), dh->getG(), dh->getY());
	}

	const EncodedKeySpec* enc = dynamic_cast<const EncodedKeySpec*>(&spec);
	if (enc)
	{
		if (enc->getFormat().equals(FORMAT_BEE))
		{
			DHPublicKey* pub = generatePublic(enc->getEncoded());
			if (pub)
				return pub;

			throw InvalidKeySpecException("Invalid KeySpec encoding");
		}
		throw InvalidKeySpecException("Unsupported KeySpec format");
	}
	throw InvalidKeySpecException("Unsupported KeySpec type");
}

KeySpec* DHKeyFactory::engineGetKeySpec(const Key& key, const type_info& info) throw (InvalidKeySpecException)
{
	const DHPublicKey* pub = dynamic_cast<const DHPublicKey*>(&key);
	if (pub)
	{
		if (info == typeid(KeySpec) || info == typeid(DHPublicKeySpec))
		{
			const DHParams& params = pub->getParams();

			return new DHPublicKeySpec(pub->getY(), params.getP(), params.getG());
		}
		if (info == typeid(EncodedKeySpec))
		{
			const String* format = pub->getFormat();
			if (format)
			{
				const bytearray* enc = pub->getEncoded();
				if (enc)
					return new AnyEncodedKeySpec(*format, *enc);
			}
		}

		throw InvalidKeySpecException("Unsupported KeySpec type");
	}

	const DHPrivateKey* pri = dynamic_cast<const DHPrivateKey*>(&key);
	if (pri)
	{
		if (info == typeid(KeySpec) || info == typeid(DHPrivateKeySpec))
		{
			const DHParams& params = pri->getParams();

			return new DHPrivateKeySpec(pri->getX(), params.getP(), params.getG());
		}
		if (info == typeid(EncodedKeySpec))
		{
			const String* format = pri->getFormat();
			if (format)
			{
				const bytearray* enc = pri->getEncoded();
				if (enc)
					return new AnyEncodedKeySpec(*format, *enc);
			}
		}

		throw InvalidKeySpecException("Unsupported KeySpec type");
	}

	throw InvalidKeySpecException("Unsupported Key type");
}

Key* DHKeyFactory::engineTranslateKey(const Key& key) throw (InvalidKeyException)
{
	const DHPublicKey* pub = dynamic_cast<const DHPublicKey*>(&key);
	if (pub)
		return new DHPublicKeyImpl(*pub);

	const DHPrivateKey* pri = dynamic_cast<const DHPrivateKey*>(&key);
	if (pri)
		return new DHPrivateKeyImpl(*pri);

	throw InvalidKeyException("Unsupported Key type");
}
