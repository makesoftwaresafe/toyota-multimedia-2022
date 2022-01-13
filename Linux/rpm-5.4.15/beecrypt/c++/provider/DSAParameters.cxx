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

#include "beecrypt/c++/provider/DSAParameters.h"
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;

using namespace beecrypt::provider;

DSAParameters::DSAParameters()
{
	_spec = 0;
}

DSAParameters::~DSAParameters()
{
	delete _spec;
}

const bytearray& DSAParameters::engineGetEncoded(const String* format) throw (IOException)
{
	throw IOException("not implemented");
}

AlgorithmParameterSpec* DSAParameters::engineGetParameterSpec(const type_info& info) throw (InvalidParameterSpecException)
{
	if (info == typeid(AlgorithmParameterSpec) || info == typeid(DSAParameterSpec))
	{
		if (_spec)
		{
			return new DSAParameterSpec(*_spec);
		}
		else
			throw InvalidParameterSpecException("not initialized");
	}
	else
		throw InvalidParameterSpecException("expected a DSAParameterSpec");
}

void DSAParameters::engineInit(const AlgorithmParameterSpec& spec) throw (InvalidParameterSpecException)
{
	const DSAParameterSpec* tmp = dynamic_cast<const DSAParameterSpec*>(&spec);

	if (tmp)
	{
		if (_spec)
		{
			delete _spec;
			_spec = 0;
		}
		_spec = new DSAParameterSpec(*tmp);
	}
	else
		throw InvalidParameterSpecException("expected a DSAParameterSpec");
}

void DSAParameters::engineInit(const byte*, int, const String* format)
{
	throw ProviderException("not implemented");
}

String DSAParameters::engineToString() throw ()
{
	return String("(not implemented");
}
