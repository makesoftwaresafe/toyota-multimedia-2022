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

/*!\file testripemd256.c
 * \brief Unit test program for the RIPEMD-256 algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/ripemd256.h"
#include "beecrypt/memchunk.h"

struct vector
{
	int input_size;
	byte* input;
	byte* expect;
};

struct vector table[3] = {
	{  0, (byte*) "",
	      (byte*) "\x02\xba\x4c\x4e\x5f\x8e\xcd\x18\x77\xfc\x52\xd6\x4d\x30\xe3\x7a\x2d\x97\x74\xfb\x1e\x5d\x02\x63\x80\xae\x01\x68\xe3\xc5\x52\x2d" },
	{  3, (byte*) "abc",
	      (byte*) "\xaf\xbd\x6e\x22\x8b\x9d\x8c\xbb\xce\xf5\xca\x2d\x03\xe6\xdb\xa1\x0a\xc0\xbc\x7d\xcb\xe4\x68\x0e\x1e\x42\xd2\xe9\x75\x45\x9b\x65" },
	{ 56, (byte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	      (byte*) "\x38\x43\x04\x55\x83\xaa\xc6\xc8\xc8\xd9\x12\x85\x73\xe7\xa9\x80\x9a\xfb\x2a\x0f\x34\xcc\xc3\x6e\xa9\xe7\x2f\x16\xf6\x36\x8e\x3f" }
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
	byte digest[32];
	ripemd256Param param;

	for (i = 0; i < 1; i++)
	{
		if (ripemd256Reset(&param))
			return -1;
		if (ripemd256Update(&param, table[i].input, table[i].input_size))
			return -1;
		if (ripemd256Digest(&param, digest))
			return -1;

		if (memcmp(digest, table[i].expect, 32))
		{
			printf("failed test vector %d\n", i+1);
			printf("expected:\n");
			hexdump(table[i].expect, 32);
			printf("got:\n");
			hexdump(digest, 32);
			failures++;
		}
	}
	return failures;
}
