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

#include "beecrypt/hmacsha384.h"

#include "beecrypt/c++/provider/HMACSHA384.h"

using namespace beecrypt::provider;

HMACSHA384::HMACSHA384() : HMAC(hmacsha384, sha384)
{
}

HMACSHA384* HMACSHA384::clone() const throw ()
{
	HMACSHA384* result = new HMACSHA384();

	memcpy(result->_ctxt.param, _ctxt.param, _ctxt.algo->paramsize);

	return result;
}
