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

#include "beecrypt/c++/security/Provider.h"
#include "beecrypt/c++/lang/RuntimeException.h"
using beecrypt::lang::RuntimeException;

using namespace beecrypt::security;

Provider::Instantiator::Instantiator(instantiator inst) : _inst(inst)
{
}

Object* Provider::Instantiator::instantiate()
{
	return _inst();
}

Provider::Provider(const String& name, double version, const String& info)
{
	_name = name;
	_info = info;
	_vers = version;

	UErrorCode status = U_ZERO_ERROR;

	_conv = ucnv_open(NULL, &status);
	if (U_FAILURE(status))
		throw RuntimeException("failed to create default unicode converter");

	#if WIN32
	_dlhandle = NULL;
	#elif defined(RTLD_DEFAULT)
	_dlhandle = RTLD_DEFAULT;
	#else
	_dlhandle = (void*) 0;
	#endif
}

Provider::~Provider()
{
	ucnv_close(_conv);
}

Object* Provider::instantiate(const String& key) const
{
	Instantiator* inst = _imap.get(&key);
	if (inst)
		return inst->instantiate();
	else
		return 0;
}

Object* Provider::setProperty(const String& key, const String& value)
{
	Object* result = 0;

	synchronized (this)
	{
		// add it in the properties
		result = Properties::setProperty(key, value);

		// add it in the instantiator map only if there is no space in the value (i.e. it's a property instead of a class)
		if (value.indexOf((UChar) 0x20) == -1)
		{
			const array<jchar>& src = value.toCharArray();

			char symname[1024];

			UErrorCode status = U_ZERO_ERROR;

			ucnv_fromUChars(_conv, symname, 1024, src.data(), src.size(), &status);

			if (status != U_ZERO_ERROR)
					throw RuntimeException("error in ucnv_fromUChars");

			instantiator i;

			#if WIN32
			if (!_dlhandle)
				_dlhandle = GetModuleHandle(NULL);
			i = (instantiator) GetProcAddress((HMODULE) _dlhandle, symname);
			#elif HAVE_DLFCN_H
			i = (instantiator) dlsym(_dlhandle, symname);
			#else
			# error
			#endif

			if (i)
			{
				Instantiator* tmp = _imap.put(new String(key), new Instantiator(i));
				assert(tmp == 0);
			}
		}
	}
	return result;
}

const String& Provider::getInfo() const throw ()
{
	return _info;
}

const String& Provider::getName() const throw ()
{
	return _name;
}

double Provider::getVersion() const throw ()
{
	return _vers;
}
