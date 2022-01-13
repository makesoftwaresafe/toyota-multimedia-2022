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

#include "beecrypt/c++/io/FileInputStream.h"
using beecrypt::io::FileInputStream;
#include "beecrypt/c++/io/FileOutputStream.h"
using beecrypt::io::FileOutputStream;
#include "beecrypt/c++/security/KeyStore.h"
using beecrypt::security::KeyStore;
#include "beecrypt/c++/security/KeyPairGenerator.h"
using beecrypt::security::KeyPairGenerator;
#include "beecrypt/c++/beeyond/BeeCertificate.h"
using beecrypt::beeyond::BeeCertificate;

#include <iostream>
using std::cout;
using std::endl;
#include <unicode/ustream.h>

int main(int argc, char* argv[])
{
	try
	{
		array<jchar> password(4);

		password[0] = (jchar) 't';
		password[1] = (jchar) 'e';
		password[2] = (jchar) 's';
		password[3] = (jchar) 't';

		KeyStore* ks = KeyStore::getInstance(KeyStore::getDefaultType());

		if (argc == 2)
		{
			FileInputStream fin(fopen(argv[1], "rb"));

			ks->load(&fin, &password);

			Key* k = ks->getKey("rsa", password);

			cout << "k algorithm = " << k->getAlgorithm() << endl;

			delete k;
		}
		else
		{
			KeyPairGenerator* kpg = KeyPairGenerator::getInstance("RSA");

			kpg->initialize(1024);

			KeyPair* pair = kpg->generateKeyPair();

			array<Certificate*> chain(1);

			chain[0] = BeeCertificate::self(pair->getPublic(), pair->getPrivate(), "SHA1withRSA");

			FileOutputStream fos(fopen("keystore", "wb"));

			// create an empty stream
			ks->load((InputStream*) 0, &password);
			ks->setKeyEntry("rsa", pair->getPrivate(), password, chain);
			ks->store(fos, &password);
		}

		delete ks;
	}
	catch (Exception e)
	{
		if (e.getMessage())
			cout << "Exception: " + *e.getMessage() << endl;
	}
}
