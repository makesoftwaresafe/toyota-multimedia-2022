/*
 * Copyright (c) 2005 Bob Deblier
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

/*!\file benchrsa.c
 * \brief Benchmark program for raw RSA performance.
 * \author Bob Deblier <bob.deblier@telenet.be>
 */

#include "beecrypt/beecrypt.h"
#include "beecrypt/rsa.h"
#include "beecrypt/timestamp.h"

#include <stdio.h>

#define SECONDS	10

const char* hm = "0001FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF03021300906052B0E03021A05000414993E364706816ABA3E25717850C26C9CD0D89D";

int main()
{
	randomGeneratorContext rngc;
	rsakp pair;
	mpnumber m, c;
	jlong start, now;
	int iterations = 0;

	randomGeneratorContextInit(&rngc, randomGeneratorDefault());

	rsakpInit(&pair);
	rsakpMake(&pair, &rngc, 1024);

	mpnzero(&m);
	mpnsethex(&m, hm);
	mpnzero(&c);
	mpnsetw(&c, 0);

	/* pub operation */
	iterations = 0;
	start = timestamp();
	do
	{
		rsapub(&pair.n, &pair.e, &m, &c);
		now = timestamp();
		iterations++;
	} while (now < (start + (SECONDS * ONE_SECOND)));

	printf("%d bits pub:  %d times in %d seconds\n",
		(int) mpbits(pair.n.size, pair.n.modl),
		iterations,
		SECONDS);

	/* pri operation */
	iterations = 0;
	start = timestamp();
	do
	{
		rsapri(&pair.n, &pair.d, &m, &c);
		now = timestamp();
		iterations++;
	} while (now < (start + (SECONDS * ONE_SECOND)));

	printf("%d bits pri:  %d times in %d seconds\n",
		(int) mpbits(pair.n.size, pair.n.modl),
		iterations,
		SECONDS);

	/* crt operation */
	iterations = 0;
	start = timestamp();
	do
	{
		rsapricrt(&pair.n, &pair.p, &pair.q, &pair.dp, &pair.dq, &pair.qi, &c, &m);
		now = timestamp();
		iterations++;
	} while (now < (start + (SECONDS * ONE_SECOND)));

	printf("%d bits crt:  %d times in %d seconds\n",
		(int) mpbits(pair.n.size, pair.n.modl),
		iterations,
		SECONDS);

	rsakpFree(&pair);

	mpnfree(&c);
	mpnfree(&m);

	return 0;
}
