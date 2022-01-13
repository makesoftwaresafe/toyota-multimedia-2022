/*
 * Copyright (c) 2003 Bob Deblier
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

/*!\file testrsa.c
 * \brief Unit test program for the RSA algorithm.
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup UNIT_m
 */

#include <stdio.h>

#include "beecrypt/beecrypt.h"
#include "beecrypt/rsa.h"

static const char* rsa_n  = "91e0fc15be36c499f2e2e36d77619c646cacf0cf6657fa136fea748b8e153f1d0cc4d38104da5273655c771282c77fd63061f360d8031406f5c9899f5ca590a19ba07d0bb3cf28c21d6d5fbf27149d2f817353c875f171a4eca9fd427feb4fc9e5f073cd68208423936ed0c3e71ada976fbec43b435dcaa3cfa7fafd973d2eaf";
static const char* rsa_d = "54d7a9456c0fa66073270a66cc1bf53d63076236fdab0542f0c0477032fea06a60d6c8bc2cfa5d21c83df2f2cd250270ac4b0ba5b37c76d565760598ade58d2bcaefb61845002c5557911603cb13db23abd21a4b1981e49d74d4f37fe0d1ee83b98bca722e11aac9d268c014bbf18b276343d7a63ae8275fa21a43be76c241a1";
static const char* rsa_e  = "10001";
static const char* rsa_p  = "9c88d074a22c27770422f2cf13e497fce54eda094ef42aa3ace900981b21fe79bef26a7c58f6effb602aea5b9776a0a9b2a018d9807eec968e19c82a40674f53";
static const char* rsa_q  = "ee92d84cd8925dceb67d9109f73ffd8472e46a137be772ccbe91acc33032e04787334f79c89d717a2f6ede2d3be833a2cb8659a4bf0a28b648727609abc763b5";
static const char* rsa_dp = "7e80668e4b5d098bba51100edf91b66e8f5639089ac0e210a2352ee0bdd4ac15f185711f0aba8d5885f048b33a6589137b22bcd25170c17c2e5c9191ebb851b7";
static const char* rsa_dq = "676abe5aa972e1392f40453415bae67198c04cff3f31b840eac70925df69de71033989d517d2b01330269626f396177415579ada6079cde61e8787856fb25215";
static const char* rsa_qi = "352b5d9d650e5e8ba1bf075e1b6ea6413cdcd419d16ae446a97f28a6b9f7aa38230b5834ee6daaf9adedee85ddc95fce36f16d4fd01eee9fdfefdcae7b152d8";

static const char* rsa_m  = "0001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff003021300906052b0e03021a05000414da39a3ee5e6b4b0d3255bfef95601890afd80709";

int main()
{
	int failures = 0;

	rsakp keypair;
	mpnumber m, sig, sigcrt;
	randomGeneratorContext rngc;

	if (randomGeneratorContextInit(&rngc, randomGeneratorDefault()) == 0)
	{
		/* First we do the fixed value verification */
		rsakpInit(&keypair);

		mpbsethex(&keypair.n, rsa_n);
		mpnsethex(&keypair.d, rsa_d);
		mpnsethex(&keypair.e, rsa_e);
		mpbsethex(&keypair.p, rsa_p);
		mpbsethex(&keypair.q, rsa_q);
		mpnsethex(&keypair.dp, rsa_dp);
		mpnsethex(&keypair.dq, rsa_dq);
		mpnsethex(&keypair.qi, rsa_qi);

		mpnzero(&m);
		mpnzero(&sig);
		mpnzero(&sigcrt);

		mpnsethex(&m, rsa_m);

		if (rsapri(&keypair.n, &keypair.d, &m, &sig))
		{
			printf("rsapri failed\n");
			failures++;
		}

		if (rsapricrt(&keypair.n, &keypair.p, &keypair.q, &keypair.dp, &keypair.dq, &keypair.qi, &m, &sigcrt))
		{
			printf("rsapricrt failed\n");
			failures++;
		}

		if (mpnex(sig.size, sig.data, sigcrt.size, sigcrt.data))
		{
			mpprintln(sig.size, sig.data);
			mpprintln(sigcrt.size, sigcrt.data);
			failures++;
		}

		mpnfree(&sigcrt);
		mpnfree(&sig);
		mpnfree(&m);

		rsakpFree(&keypair);
	}
	return failures;
}
