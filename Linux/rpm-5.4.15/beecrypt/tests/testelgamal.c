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

/*!\file testelgamal.c
 * \brief Unit test program for elgamal.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/beecrypt.h"
#include "beecrypt/dldp.h"
#include "beecrypt/dlkp.h"
#include "beecrypt/elgamal.h"

const char* message_to_sign = "73F6679451E5F98CA60235E6B4C58FC14043C56D";

int main()
{
	int failures = 0;

	dldp_p params;
	randomGeneratorContext rngc;

	dldp_pInit(&params);

	if (randomGeneratorContextInit(&rngc, randomGeneratorDefault()) == 0)
	{
		/* make parameters with p = 512 bits */
		if (dldp_pgonMakeSafe(&params, &rngc, 512) == 0)
		{
			/* params contains both p and n=(p-1) */
			dlkp_p keypair;

			dlkp_pInit(&keypair);

			if (dlkp_pPair(&keypair, &rngc, &params) == 0)
			{
				mpnumber hm;
				mpnumber r;
				mpnumber s;

				mpnzero(&hm);
				mpnzero(&r);
				mpnzero(&s);

				mpnsethex(&hm, message_to_sign);

				/* try to sign hm */
				if (elgv1sign(&params.p, &params.n, &params.g, &rngc, &hm, &keypair.x, &r, &s) == 0)
				{
					/* try the verification */
					if (elgv1vrfy(&params.p, &params.n, &params.g, &hm, &keypair.y, &r, &s) == 0)
					{
						/* failure in verification */
						failures++;
					}
				}
				else
				{
					/* failure in signing */
					failures++;
				}

				mpnfree(&hm);
				mpnfree(&r);
				mpnfree(&s);

				dlkp_pFree(&keypair);
			}

			dldp_pFree(&params);
		}
		else
		{
			printf("safe parameter generation failure\n");
			return -1;
		}
		
		randomGeneratorContextFree(&rngc);  
	}
	else
	{
		printf("random generator failure\n");
		return -1;
	}

	return failures;
}
