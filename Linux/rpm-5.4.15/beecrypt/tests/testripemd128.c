/*
 * Copyright (c) 2009 Bob Deblier
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

/*!\file testripemd128.c
 * \brief Unit test program for the RIPEMD-128 algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/ripemd128.h"
#include "beecrypt/memchunk.h"

struct vector
{
	int input_size;
	byte* input;
	byte* expect;
};

struct vector table[3] = {
	{  0, (byte*) "",
	      (byte*) "\xcd\xf2\x62\x13\xa1\x50\xdc\x3e\xcb\x61\x0f\x18\xf6\xb3\x8b\x46" },
	{  3, (byte*) "abc",
	      (byte*) "\xc1\x4a\x12\x19\x9c\x66\xe4\xba\x84\x63\x6b\x0f\x69\x14\x4c\x77" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	      (byte*) "\xa1\xaa\x06\x89\xd0\xfa\xfa\x2d\xdc\x22\xe8\x8b\x49\x13\x3a\x06" }
};

void hexdump(const byte* b, int count)
{
        int i;

        for (i = 0; i < count; i++)
        {
                printf("%02x", b[i]);
                if ((i & 0xf) == 0xf)
                        printf("\n");
        }
        if (i & 0xf)
                printf("\n");
}

int main()
{
	int i, failures = 0;
	byte digest[16];
	ripemd128Param param;

	for (i = 0; i < 1; i++)
	{
		if (ripemd128Reset(&param))
			return -1;
		if (ripemd128Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (ripemd128Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 16))
		{
			printf("failed test vector %d\n", i+1);
			printf("expected:\n");
			hexdump(table[i].expect, 16);
			printf("got:\n");
			hexdump(digest, 16);
			failures++;
		}
	}
	return failures;
}
