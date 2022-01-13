/*
 * Copyright (c) 2004 Beeyond Software Holding BV
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/security/AlgorithmParameterGenerator.h"
using beecrypt::security::AlgorithmParameterGenerator;
#include "beecrypt/c++/security/KeyPairGenerator.h"
using beecrypt::security::KeyPairGenerator;
#include "beecrypt/c++/crypto/Cipher.h"
using beecrypt::crypto::Cipher;
#include "beecrypt/c++/crypto/interfaces/DHPublicKey.h"
using beecrypt::crypto::interfaces::DHPublicKey;
#include "beecrypt/c++/crypto/interfaces/DHPrivateKey.h"
using beecrypt::crypto::interfaces::DHPrivateKey;
#include "beecrypt/c++/beeyond/DHIESParameterSpec.h"
using beecrypt::beeyond::DHIESParameterSpec;

#include <typeinfo>
using std::type_info;
#include <iostream>
using namespace std;
#include <unicode/ustream.h>

int main(int argc, char* argv[])
{
	int failures = 0;

	try
	{
		bytearray original(31415);

		SecureRandom::getSeed(original.data(), original.size());

		KeyPairGenerator* kpg = KeyPairGenerator::getInstance("DiffieHellman");

		kpg->initialize(1024);

		KeyPair* pair = kpg->generateKeyPair();

		Cipher* c = Cipher::getInstance("DHIES");

		c->init(Cipher::ENCRYPT_MODE, pair->getPublic(), DHIESParameterSpec("SHA-512", "AES", "HMAC-SHA-512"));

		size_t ciphertextsize = c->getOutputSize(original.size());

		bytearray ciphertext(ciphertextsize);

		size_t used = c->doFinal(original.data(), 0, original.size(), ciphertext, 0);

		ciphertext.resize(used);

		AlgorithmParameters* p = c->getParameters();

		c->init(Cipher::DECRYPT_MODE, pair->getPrivate(), p);

		size_t cleartextsize = c->getOutputSize(ciphertext.size());

		bytearray cleartext(cleartextsize);

		used = c->doFinal(ciphertext.data(), 0, ciphertext.size(), cleartext, 0);

		cleartext.resize(used);

		if (original != cleartext)
			failures++;

		delete p;
		delete c;
		delete pair;
		delete kpg;
	}
	catch (Exception& ex)
	{
		cerr << "Exception: " << *ex.getMessage() << endl;
		failures++;
	}
	catch (...)
	{
		cerr << "exception" << endl;
		failures++;
	}

	return failures;
}
