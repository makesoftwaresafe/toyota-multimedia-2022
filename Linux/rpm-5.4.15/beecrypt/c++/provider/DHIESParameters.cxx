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

#include "beecrypt/c++/provider/DHIESParameters.h"
#include "beecrypt/c++/security/ProviderException.h"
using beecrypt::security::ProviderException;

using namespace beecrypt::provider;

DHIESParameters::DHIESParameters()
{
	_spec = 0;
	_dspec = 0;
}

DHIESParameters::~DHIESParameters()
{
	delete _spec;
	delete _dspec;
}

const bytearray& DHIESParameters::engineGetEncoded(const String* format) throw (IOException)
{
	throw IOException("not implemented");
}

AlgorithmParameterSpec* DHIESParameters::engineGetParameterSpec(const type_info& info) throw (InvalidParameterSpecException)
{
	if (info == typeid(DHIESDecryptParameterSpec))
	{
		if (_dspec)
			return new DHIESDecryptParameterSpec(*_dspec);
	}
	else if (info == typeid(DHIESParameterSpec) || info == typeid(AlgorithmParameterSpec))
	{
		if (_spec)
			return new DHIESParameterSpec(*_spec);
	}
	throw InvalidParameterSpecException();
}

void DHIESParameters::engineInit(const AlgorithmParameterSpec& param) throw (InvalidParameterSpecException)
{
	delete _spec;
	delete _dspec;

	_spec = 0;
	_dspec = 0;

	const DHIESParameterSpec* spec = dynamic_cast<const DHIESParameterSpec*>(&param);
	if (spec)
	{
		_spec = new DHIESParameterSpec(*spec);

		const DHIESDecryptParameterSpec* dspec = dynamic_cast<const DHIESDecryptParameterSpec*>(spec);
		if (dspec)
			_dspec = new DHIESDecryptParameterSpec(*dspec);
	}
	else
		throw InvalidParameterSpecException("Expected a DHIESParameterSpec");
}

void DHIESParameters::engineInit(const byte*, int, const String* format)
{
	throw ProviderException("Not implemented");
}

String DHIESParameters::engineToString() throw ()
{
	if (_dspec)
		return _dspec->toString();
	if (_spec)
		return _spec->toString();

	return String("(uninitialized)");
}
