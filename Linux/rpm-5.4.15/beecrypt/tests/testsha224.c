/*
 * testsha224.c
 *
 * Unit test program for SHA-224; it implements the test vectors from IETF RFC 3874.
 *
 * Copyright (c) 2009 Bob Deblier <bob.deblier@telenet.be>
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

#include "beecrypt/sha224.h"

struct vector
{
	int		input_size;
	byte*	input;
	byte*	expect;
};


struct vector table[2] = {
	{  3, (byte*) "abc",
	      (byte*) "\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55\xb3\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	      (byte*) "\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01\x50\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25" }
};

int main()
{
	int i, failures = 0;
	sha224Param param;
	byte digest[32];

	for (i = 0; i < 2; i++)
	{
		if (sha224Reset(&param))
			return -1;
		if (sha224Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (sha224Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 28))
		{
			printf("failed test vector %d\n", i+1);
			failures++;
		}
	}
	return failures;
}
