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

#include "beecrypt/c++/beeyond/BeeCertificate.h"
using beecrypt::beeyond::BeeCertificate;
#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;
#include "beecrypt/c++/provider/BeeCertificateFactory.h"

using namespace beecrypt::provider;

BeeCertificateFactory::BeeCertificateFactory()
{
}

Certificate* BeeCertificateFactory::engineGenerateCertificate(InputStream& in) throw (CertificateException)
{
	try
	{
		return new BeeCertificate(in);
	}
	catch (Exception& e)
	{
		throw CertificateException().initCause(e);
	}
}

Collection<Certificate>* BeeCertificateFactory::engineGenerateCertificates(InputStream& in) throw (CertificateException)
{
	ArrayList<Certificate>* result = new ArrayList<Certificate>();

	try
	{
		while (in.available())
		{
			result->add(new BeeCertificate(in));
		}
	}
	catch (...)
	{
		delete result;
		throw;
	}

	return result;
}
