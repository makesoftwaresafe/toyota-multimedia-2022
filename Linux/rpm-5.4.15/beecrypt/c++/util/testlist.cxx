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

#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;

template class ArrayList<String>;

#include <iostream>

int main()
{
	try
	{
		ArrayList<String> l;

		String* s1 = new String("abc");
		String* s2 = new String("cde");

		std::cout << "l.size() = " << l.size() << std::endl;

		l.add(s1);
		l.add(s2);

		std::cout << "the lot: " << l.toString() << std::endl;

		ArrayList<String> m;

		m.addAll(l);

		Iterator<String>* it = l.iterator();

		if (it)
		{
			while (it->hasNext())
			{
				String* s = it->next();

				std::cout << "got String [" << *s << "]" << std::endl;
			}
			std::cout << "no more strings available" << std::endl;

			delete it;
		}

		if (l.remove(s1))
			std::cout << "removed first string" << std::endl;
		else
			std::cout << "failed to remove first string" << std::endl;

		std::cout << "l.size() = " << l.size() << std::endl;

		l.clear();

		std::cout << "l.size() = " << l.size() << std::endl;

		std::cout << "m.size() = " << m.size() << std::endl;

		std::cout << "that was all, folks!" << std::endl;
	}
	catch (Exception& e)
	{
		std::cout << "Exception:" << e.getMessage() << std::endl;
	}
}
