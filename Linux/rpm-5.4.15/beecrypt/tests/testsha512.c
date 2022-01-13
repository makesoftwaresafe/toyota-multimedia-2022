/*
 * testsha512.c
 *
 * Unit test program for SHA-512; it implements the test vectors from FIPS 180-2.
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

#include "beecrypt/sha512.h"

struct vector
{
	int		input_size;
	byte*	input;
	byte*	expect;
};

struct vector table[1] = {
	{  3, (byte*) "abc",
	      (byte*) "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f\xa5\x4c\xa4\x9f" },
};

int main()
{
	int i, failures = 0;
	sha512Param param;
	byte digest[64];

	for (i = 0; i < 1; i++)
	{
		if (sha512Reset(&param))
			return -1;
		if (sha512Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (sha512Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 64))
		{
			printf("failed test vector %d\n", i+1);
			failures++;
		}
	}
	return failures;
}
