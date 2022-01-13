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

/*!\file testripemd160.c
 * \brief Unit test program for the RIPEMD-160 algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/ripemd160.h"
#include "beecrypt/memchunk.h"

struct vector
{
	int input_size;
	byte* input;
	byte* expect;
};

struct vector table[3] = {
	{  0, (byte*) "",
	      (byte*) "\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28\x08\x97\x7e\xe8\xf5\x48\xb2\x25\x8d\x31" },
	{  3, (byte*) "abc",
	      (byte*) "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04\x4a\x8e\x98\xc6\xb0\x87\xf1\x5a\x0b\xfc" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	      (byte*) "\x12\xa0\x53\x38\x4a\x9c\x0c\x88\xe4\x05\xa0\x6c\x27\xdc\xf4\x9a\xda\x62\xeb\x2b" }
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
	byte digest[20];
	ripemd160Param param;

	for (i = 0; i < 1; i++)
	{
		if (ripemd160Reset(&param))
			return -1;
		if (ripemd160Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (ripemd160Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 20))
		{
			printf("failed test vector %d\n", i+1);
			printf("expected:\n");
			hexdump(table[i].expect, 20);
			printf("got:\n");
			hexdump(digest, 20);
			failures++;
		}
	}
	return failures;
}
