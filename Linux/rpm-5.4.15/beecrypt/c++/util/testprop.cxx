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

// #define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/lang/System.h"
using beecrypt::lang::System;
#include "beecrypt/c++/util/Properties.h"
using beecrypt::util::Properties;

#include <unicode/ustream.h>
#include <iostream>
using namespace std;
#include <stdio.h>

int main(int argc, char* argv[])
{

	Properties p;

	p.setProperty("apple", "red");
	p.setProperty("yam", "orange");
	p.setProperty("lime", "green");
	p.setProperty("grape", "blue");

#if 0
	Enumeration<const String>* e = p.propertyNames();

	while (e->hasMoreElements())
	{
		const String* s = e->nextElement();

		cout << *s << endl;
	}

	delete e;
#endif

	p.store(System::out, "properties test");
}
