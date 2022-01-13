/*
 * Copyright (c) 2004 X-Way Rights BV
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

/*!\file Provider.h
 * \ingroup CXX_SECURITY_m
 */

#ifndef _CLASS_BEE_SECURITY_PROVIDER_H
#define _CLASS_BEE_SECURITY_PROVIDER_H

#ifdef __cplusplus

#include "beecrypt/c++/util/Properties.h"
using beecrypt::util::Properties;

#include <unicode/ucnv.h>

namespace beecrypt {
	namespace security {
		/*!\ingroup CXX_SECURITY_m
		 */
		class Provider : public Properties
		{
			friend class Security;

		private:
			typedef Object* (*instantiator)();

			class Instantiator : public Object
			{
			private:
				instantiator _inst;

			public:
				Instantiator(instantiator inst);
				virtual ~Instantiator() {}

				Object* instantiate();
			};

			String _name;
			String _info;
			double _vers;

			UConverter* _conv;

			Hashtable<String,Instantiator> _imap;

			Object* instantiate(const String& name) const;

		protected:
			#if WIN32
			HANDLE  _dlhandle;
			#else
			void* _dlhandle;
			#endif

			BEECRYPTCXXAPI
			Provider(const String& name, double version, const String& info);

		public:
			BEECRYPTCXXAPI
			virtual ~Provider();

			BEECRYPTCXXAPI
			Object* setProperty(const String& key, const String& value);

			BEECRYPTCXXAPI
			const String& getName() const throw ();
			BEECRYPTCXXAPI
			const String& getInfo() const throw ();
			BEECRYPTCXXAPI
			double getVersion() const throw ();
		};
	}
}

#endif

#endif
