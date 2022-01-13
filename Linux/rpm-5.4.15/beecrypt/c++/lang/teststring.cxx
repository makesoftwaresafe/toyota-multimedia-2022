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

#include "beecrypt/c++/lang/String.h"
using beecrypt::lang::String;

#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
	String a("The quick brown fox jumps");
	String b("over the lazy dog");

	std::cout << a.substring(0, 3) << std::endl;
	std::cout << a.indexOf((int) 'q') << std::endl;
	std::cout << a.indexOf((int) 'z') << std::endl;
	std::cout << a.indexOf("brown") << std::endl;
	std::cout << a.contains(String("cat")) << std::endl;
	std::cout << (a + ' ' + b) << std::endl;
	std::cout << a.equalsIgnoreCase("THE QUICK BROWN FOX JUMPS") << std::endl;
}
