/*
 * testsha384.c
 *
 * Unit test program for SHA-384; it implements the test vectors from FIPS 180-2.
 *
 * Copyright (c) 2004 Bob Deblier <bob.deblier@telenet.be>
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
 *
 */

#include <stdio.h>

#include "beecrypt/sha384.h"

struct vector
{
	int		input_size;
	byte*	input;
	byte*	expect;
};

struct vector table[1] = {
	{  3, (byte*) "abc",
	      (byte*) "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7" },
};

int main()
{
	int i, failures = 0;
	sha384Param param;
	byte digest[48];

	for (i = 0; i < 1; i++)
	{
		if (sha384Reset(&param))
			return -1;
		if (sha384Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (sha384Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 48))
		{
			printf("failed test vector %d\n", i+1);
			failures++;
		}
	}
	return failures;
}
