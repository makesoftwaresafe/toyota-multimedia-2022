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

#include "beecrypt/c++/resource.h"
#include "beecrypt/c++/security/Security.h"
#include "beecrypt/c++/io/FileInputStream.h"
using beecrypt::io::FileInputStream;
#include "beecrypt/c++/lang/Integer.h"
using beecrypt::lang::Integer;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;

using namespace beecrypt::security;

namespace {
	const String KEYSTORE_DEFAULT_TYPE("BEE");
}

bool Security::_init = false;
Properties Security::_props;
ArrayList<Provider> Security::_providers;

/* Have to use lazy initialization here; static initialization doesn't work.
 * Initialization adds a provider, apparently in another copy of Security,
 * instead of where we would expect it.
 *
 * Don't dlclose the libraries or uninstall the providers. They'll
 * disappear when the program closes. Since this happens only once per
 * application which uses this library, that's acceptable.
 *
 * What we eventually need to do is the following:
 * - treat the beecrypt.conf file as a collection of Properties, loaded from
 *   file with loadProperties.
 * - get appropriate properties to do the initialization
 */

void Security::initialize()
{
	synchronized (_providers)
	{
		_init = true;
	}

	/* get the configuration file here and load providers */
	const char* path = getenv("BEECRYPT_CONF_FILE");

	FILE* props;

	if (path)
		props = fopen(path, "r");
	else
		props = fopen(BEECRYPT_CONF_FILE, "r");

	if (!props)
	{
		std::cerr << "couldn't open beecrypt configuration file" << std::endl;
	}
	else
	{
		FileInputStream fis(props);

		try
		{
			UErrorCode status;
			UConverter* _loc = 0;

			// load properties from fis
			_props.load(fis);

			for (jint index = 1; true; index++)
			{
				const String* value = _props.getProperty(String("provider.") + Integer::toString(index));

				if (value)
				{
					if (!_loc)
					{
						status = U_ZERO_ERROR;
						_loc = ucnv_open(0, &status);
						if (U_FAILURE(status))
							throw RuntimeException("ucnv_open failed");
					}

					const array<jchar>& src = value->toCharArray();

					int need = ucnv_fromUChars(_loc, 0, 0, src.data(), src.size(), &status);
					if (U_FAILURE(status))
						if (status != U_BUFFER_OVERFLOW_ERROR)
							throw RuntimeException("ucnv_fromUChars failed");

					char* shared_library = new char[need+1];

					status = U_ZERO_ERROR;
					ucnv_fromUChars(_loc, shared_library, need+1, src.data(), src.size(), &status);
					if (U_FAILURE(status))
						throw RuntimeException("ucnv_fromUChars failed");

					#if WIN32
					HANDLE handle = LoadLibraryEx(shared_library, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
					#elif HAVE_DLFCN_H
					void *handle = dlopen(shared_library, RTLD_NOW);
					#else
					# error
					#endif

					if (handle)
					{
						#if WIN32
						Provider* (*inst)(void*) = (Provider* (*)(void*)) GetProcAddress((HMODULE) handle, "provider_instantiate");
						#elif HAVE_PTHREAD_H
						Provider* (*inst)(void*) = (Provider* (*)(void*)) dlsym(handle, "provider_instantiate");
						#else
						# error
						#endif

						if (inst)
						{
							addProvider(inst(handle));
						}
						else
						{
							std::cerr << "library doesn't contain symbol provider_const_ref" << std::endl;
							#if HAVE_DLFCN_H
							std::cerr << "dlerror: " << dlerror() << std::endl;
							#endif
						}
					}
					else
					{
						std::cerr << "unable to open shared library " << shared_library << std::endl;
						#if HAVE_DLFCN_H
						std::cerr << "dlerror: " << dlerror() << std::endl;
						#endif
					}

					delete[] shared_library;
				}
				else
					break;
			}
		}
		catch (IOException&)
		{
		}
	}
}

Security::spi::spi(Object* cspi, const Provider* prov, const String& name) : cspi(cspi), name(name), prov(prov)
{
}

Security::spi* Security::getSpi(const String& name, const String& type) throw (NoSuchAlgorithmException)
{
	Object* inst = 0;

	const Provider* p = 0;

	if (!_init)
		initialize();

	String afind = type + "." + name;
	String alias = "Alg.Alias." + type + "." + name;

	synchronized (_providers)
	{
		for (int i = 0; i < _providers.size(); i++)
		{
			p = _providers.get(i);

			if (p->getProperty(afind))
			{
				if ((inst = p->instantiate(afind)))
					break;
			}
			else
			{
				const String* alias_of = p->getProperty(alias);

				if (alias_of && (inst = p->instantiate(*alias_of)))
					break;
			}
		}
	}

	if (inst)
		return new spi(inst, p, name);

	throw NoSuchAlgorithmException(name + " " + type + " not available");
}

Security::spi* Security::getSpi(const String& name, const String& type, const String& provider) throw (NoSuchAlgorithmException, NoSuchProviderException)
{
	Object* inst = 0;

	const Provider* p = 0;

	if (!_init)
		initialize();

	String afind = type + "." + name;
	String alias = "Alg.Alias." + type + "." + name;

	synchronized (_providers)
	{
		for (int i = 0; i < _providers.size(); i++)
		{
			p = _providers.get(i);

			if (p->getName().equals(provider))
			{
				if (p->getProperty(afind))
				{
					if ((inst = p->instantiate(afind)))
						break;
				}
				else
				{
					const String* alias_of = p->getProperty(alias);

					if (alias_of && (inst = p->instantiate(*alias_of)))
						break;
				}

				throw NoSuchAlgorithmException(name + " " + type + " not available");
			}
		}
	}

	if (inst)
		return new spi(inst, p, name);

	throw NoSuchProviderException(provider + " Provider not available");
}

Security::spi* Security::getSpi(const String& name, const String& type, const Provider& provider) throw (NoSuchAlgorithmException)
{
	Object* inst = 0;

	if (!_init)
		initialize();

	String afind = type + "." + name;
	String alias = "Alg.Alias." + type + "." + name;

	if (provider.getProperty(afind))
	{
		inst = provider.instantiate(afind);
	}
	else
	{
		const String* alias_of = provider.getProperty(alias);

		if (alias_of)
			inst = provider.instantiate(*alias_of);
	}

	if (inst)
		return new spi(inst, &provider, name);

	throw NoSuchAlgorithmException(name + " " + type + " not available");
}

Security::spi* Security::getFirstSpi(const String& type)
{
	Object* inst = 0;

	const Provider* p = 0;

	if (!_init)
		initialize();

	String afind = type + ".";

	synchronized (_providers)
	{
		for (int i = 0; i < _providers.size(); i++)
		{
			p = _providers.get(i);

			Iterator<class Map<Object,Object>::Entry>* it = p->entrySet().iterator();
			assert(it != 0);
			while (it->hasNext())
			{
				const String* s = dynamic_cast<const String*>(it->next()->getKey());

				if (s->startsWith(afind))
				{
					if ((inst = p->instantiate(*s)))
					{
						String name(*s);

						delete it;

						return new spi(inst, p, name);
					}
				}
			}
			delete it;
		}
	}
	return 0;
}

const String& Security::getKeyStoreDefault()
{
	return *_props.getProperty("keystore.default", KEYSTORE_DEFAULT_TYPE);
}

int Security::addProvider(Provider* provider)
{
	if (!provider)
		throw NullPointerException();

	int rc = 0;

	if (!_init)
		initialize();

	if (getProvider(provider->getName()))
		return -1;

	synchronized (_providers)
	{
		rc = (int) _providers.size();

		_providers.add(provider);
	}

	return rc;
}

int Security::insertProviderAt(Provider* provider, int position) throw (IndexOutOfBoundsException)
{
	if (!provider)
		throw NullPointerException();

	if (!_init)
		initialize();

	if (getProvider(provider->getName()))
		return -1;

	synchronized (_providers)
	{
		_providers.add(position, provider);
	}

	return position;
}

void Security::removeProvider(const String& name)
{
	if (!_init)
		initialize();

	synchronized (_providers)
	{
		for (int i = 0; i < _providers.size(); i++)
		{
			const Provider* p = _providers.get(i);

			if (p->getName().equals(name))
			{
				_providers.remove(i);
				return;
			}
		}
	}
}

array<Provider*> Security::getProviders()
{
	if (!_init)
		initialize();

	return _providers.toArray();
}

Provider* Security::getProvider(const String& name)
{
	if (!_init)
		initialize();

	for (int i = 0; i < _providers.size(); i++)
	{
		Provider* tmp = _providers.get(i);
		if (tmp->getName().equals(name))
			return tmp;
	}

	return 0;
}

const String* Security::getProperty(const String& key) throw ()
{
	return _props.getProperty(key);
}
