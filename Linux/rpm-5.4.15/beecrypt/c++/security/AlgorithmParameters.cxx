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

#include "beecrypt/c++/security/AlgorithmParameters.h"
#include "beecrypt/c++/security/AlgorithmParametersSpi.h"
#include "beecrypt/c++/security/Provider.h"
#include "beecrypt/c++/security/Security.h"
#include "beecrypt/c++/security/spec/AlgorithmParameterSpec.h"
using beecrypt::security::spec::AlgorithmParameterSpec;

using namespace beecrypt::security;

AlgorithmParameters::AlgorithmParameters(AlgorithmParametersSpi* spi, const Provider* provider, const String& algorithm)
{
	_aspi = spi;
	_prov = provider;
	_algo = algorithm;
}

AlgorithmParameters::~AlgorithmParameters()
{
	delete _aspi;
}

AlgorithmParameters* AlgorithmParameters::getInstance(const String& algorithm) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "AlgorithmParameters");

	assert(dynamic_cast<AlgorithmParametersSpi*>(tmp->cspi));

	AlgorithmParameters* result = new AlgorithmParameters(reinterpret_cast<AlgorithmParametersSpi*>(tmp->cspi), tmp->prov, tmp->name);

	delete tmp;

	return result;
}

AlgorithmParameters* AlgorithmParameters::getInstance(const String& algorithm, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "AlgorithmParameters", provider);

	assert(dynamic_cast<AlgorithmParametersSpi*>(tmp->cspi));

	AlgorithmParameters* result = new AlgorithmParameters(reinterpret_cast<AlgorithmParametersSpi*>(tmp->cspi), tmp->prov, tmp->name);

	delete tmp;

	return result;
}

AlgorithmParameters* AlgorithmParameters::getInstance(const String& algorithm, const Provider& provider) throw (NoSuchAlgorithmException)
{
	Security::spi* tmp = Security::getSpi(algorithm, "AlgorithmParameters", provider);

	assert(dynamic_cast<AlgorithmParametersSpi*>(tmp->cspi));

	AlgorithmParameters* result = new AlgorithmParameters(reinterpret_cast<AlgorithmParametersSpi*>(tmp->cspi), tmp->prov, tmp->name);

	delete tmp;

	return result;
}

const bytearray& AlgorithmParameters::getEncoded(const String* format) throw (IOException)
{
	return _aspi->engineGetEncoded(format);
}

AlgorithmParameterSpec* AlgorithmParameters::getParameterSpec(const type_info& info) throw (InvalidParameterSpecException)
{
	return _aspi->engineGetParameterSpec(info);
}

void AlgorithmParameters::init(const AlgorithmParameterSpec& spec) throw (InvalidParameterSpecException)
{
	_aspi->engineInit(spec);
}

void AlgorithmParameters::init(const byte* data, int size, const String* format)
{
	_aspi->engineInit(data, size, format);
}

const String& AlgorithmParameters::getAlgorithm() const throw ()
{
	return _algo;
}

const Provider& AlgorithmParameters::getProvider() const throw ()
{
	return *_prov;
}

String AlgorithmParameters::toString() throw ()
{
	return _aspi->engineToString();
}
