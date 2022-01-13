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

/*!\file testripemd320.c
 * \brief Unit test program for the RIPEMD-320 algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/ripemd320.h"
#include "beecrypt/memchunk.h"

struct vector
{
	int input_size;
	byte* input;
	byte* expect;
};

struct vector table[3] = {
	{  0, (byte*) "",
	      (byte*) "\x22\xd6\x5d\x56\x61\x53\x6c\xdc\x75\xc1\xfd\xf5\xc6\xde\x7b\x41\xb9\xf2\x73\x25\xeb\xc6\x1e\x85\x57\x17\x7d\x70\x5a\x0e\xc8\x80\x15\x1c\x3a\x32\xa0\x08\x99\xb8" },
	{  3, (byte*) "abc",
	      (byte*) "\xde\x4c\x01\xb3\x05\x4f\x89\x30\xa7\x9d\x09\xae\x73\x8e\x92\x30\x1e\x5a\x17\x08\x5b\xef\xfd\xc1\xb8\xd1\x16\x71\x3e\x74\xf8\x2f\xa9\x42\xd6\x4c\xdb\xc4\x68\x2d" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	      (byte*) "\xd0\x34\xa7\x95\x0c\xf7\x22\x02\x1b\xa4\xb8\x4d\xf7\x69\xa5\xde\x20\x60\xe2\x59\xdf\x4c\x9b\xb4\xa4\x26\x8c\x0e\x93\x5b\xbc\x74\x70\xa9\x69\xc9\xd0\x72\xa1\xac" }
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
	byte digest[40];
	ripemd320Param param;

	for (i = 0; i < 1; i++)
	{
		if (ripemd320Reset(&param))
			return -1;
		if (ripemd320Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (ripemd320Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 40))
		{
			printf("failed test vector %d\n", i+1);
			printf("expected:\n");
			hexdump(table[i].expect, 40);
			printf("got:\n");
			hexdump(digest, 40);
			failures++;
		}
	}
	return failures;
}
