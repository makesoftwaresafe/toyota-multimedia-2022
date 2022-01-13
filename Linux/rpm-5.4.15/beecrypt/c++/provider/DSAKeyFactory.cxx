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

#include "beecrypt/c++/provider/DSAKeyFactory.h"
#include "beecrypt/c++/provider/DSAPrivateKeyImpl.h"
#include "beecrypt/c++/provider/DSAPublicKeyImpl.h"
#include "beecrypt/c++/io/ByteArrayInputStream.h"
using beecrypt::io::ByteArrayInputStream;
#include "beecrypt/c++/security/KeyFactory.h"
using beecrypt::security::KeyFactory;
#include "beecrypt/c++/security/spec/DSAPrivateKeySpec.h"
using beecrypt::security::spec::DSAPrivateKeySpec;
#include "beecrypt/c++/security/spec/DSAPublicKeySpec.h"
using beecrypt::security::spec::DSAPublicKeySpec;
#include "beecrypt/c++/beeyond/AnyEncodedKeySpec.h"
using beecrypt::beeyond::AnyEncodedKeySpec;
#include "beecrypt/c++/beeyond/BeeInputStream.h"
using beecrypt::beeyond::BeeInputStream;

using namespace beecrypt::provider;

namespace {
	const String FORMAT_BEE("BEE");
	const String ALGORITHM_DSA("DSA");

	DSAPrivateKey* generatePrivate(const bytearray& enc)
	{
		try
		{
			ByteArrayInputStream bis(enc);
			BeeInputStream bee(bis);

			BigInteger p, q, g, x;

			p = bee.readBigInteger();
			q = bee.readBigInteger();
			g = bee.readBigInteger();
			x = bee.readBigInteger();

			return new DSAPrivateKeyImpl(p, q, g, x);
		}
		catch (IOException&)
		{
		}
		return 0;
	}

	DSAPublicKey* generatePublic(const bytearray& enc)
	{
		try
		{
			ByteArrayInputStream bis(enc);
			BeeInputStream bee(bis);

			BigInteger p, q, g, y;

			p = bee.readBigInteger();
			q = bee.readBigInteger();
			g = bee.readBigInteger();
			y = bee.readBigInteger();

			return new DSAPublicKeyImpl(p, q, g, y);
		}
		catch (IOException&)
		{
		}
		return 0;
	}
}

DSAKeyFactory::DSAKeyFactory()
{
}

PrivateKey* DSAKeyFactory::engineGeneratePrivate(const KeySpec& spec) throw (InvalidKeySpecException)
{
	const DSAPrivateKeySpec* dsa = dynamic_cast<const DSAPrivateKeySpec*>(&spec);
	if (dsa)
	{
		return new DSAPrivateKeyImpl(dsa->getP(), dsa->getQ(), dsa->getG(), dsa->getX());
	}

	const EncodedKeySpec* enc = dynamic_cast<const EncodedKeySpec*>(&spec);
	if (enc)
	{
		if (enc->getFormat().equals(FORMAT_BEE))
		{
			DSAPrivateKey* pri = generatePrivate(enc->getEncoded());
			if (pri)
				return pri;

			throw InvalidKeySpecException("Invalid KeySpec encoding");
		}
		throw InvalidKeySpecException("Unsupported KeySpec format");
	}
	throw InvalidKeySpecException("Unsupported KeySpec type");
}

PublicKey* DSAKeyFactory::engineGeneratePublic(const KeySpec& spec) throw (InvalidKeySpecException)
{
	const DSAPublicKeySpec* dsa = dynamic_cast<const DSAPublicKeySpec*>(&spec);
	if (dsa)
	{
		return new DSAPublicKeyImpl(dsa->getP(), dsa->getQ(), dsa->getG(), dsa->getY());
	}

	const EncodedKeySpec* enc = dynamic_cast<const EncodedKeySpec*>(&spec);
	if (enc)
	{
		if (enc->getFormat().equals(FORMAT_BEE))
		{
			DSAPublicKey* pub = generatePublic(enc->getEncoded());
			if (pub)
				return pub;

			throw InvalidKeySpecException("Invalid KeySpec encoding");
		}
		throw InvalidKeySpecException("Unsupported KeySpec format");
	}
	throw InvalidKeySpecException("Unsupported KeySpec type");
}

KeySpec* DSAKeyFactory::engineGetKeySpec(const Key& key, const type_info& info) throw (InvalidKeySpecException)
{
	const DSAPublicKey* pub = dynamic_cast<const DSAPublicKey*>(&key);
	if (pub)
	{
		if (info == typeid(KeySpec) || info == typeid(DSAPublicKeySpec))
		{
			const DSAParams& params = pub->getParams();

			return new DSAPublicKeySpec(pub->getY(), params.getP(), params.getQ(), params.getG());
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

	const DSAPrivateKey* pri = dynamic_cast<const DSAPrivateKey*>(&key);
	if (pri)
	{
		if (info == typeid(KeySpec) || info == typeid(DSAPrivateKeySpec))
		{
			const DSAParams& params = pri->getParams();

			return new DSAPrivateKeySpec(pri->getX(), params.getP(), params.getQ(), params.getG());
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

Key* DSAKeyFactory::engineTranslateKey(const Key& key) throw (InvalidKeyException)
{
	const DSAPublicKey* pub = dynamic_cast<const DSAPublicKey*>(&key);
	if (pub)
		return new DSAPublicKeyImpl(*pub);

	const DSAPrivateKey* pri = dynamic_cast<const DSAPrivateKey*>(&key);
	if (pri)
		return new DSAPrivateKeyImpl(*pri);

	throw InvalidKeyException("Unsupported Key type");
}
