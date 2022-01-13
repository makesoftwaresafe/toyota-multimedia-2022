/*
 * cal/lcrypto.h \
 *
 * $Id: lcrypto.h,v 1.3 2008/10/27 03:01:38 jbj Exp $
 *
 * Copyright © 2006 Keith Howe.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *  
 */

#ifndef _LUACRYPTO_
#define _LUACRYPTO_

#ifndef LUACRYPTO_API
#define LUACRYPTO_API   LUA_API
#endif

#define LUACRYPTO_PREFIX "LuaCrypto: "
#define LUACRYPTO_CORENAME "crypto"
#define LUACRYPTO_EVPNAME "crypto.evp"
#define LUACRYPTO_HMACNAME "crypto.hmac"
#define LUACRYPTO_RANDNAME "crypto.rand"

LUACRYPTO_API int luacrypto_createmeta (lua_State *L, const char *name, const luaL_reg *methods);
LUACRYPTO_API void luacrypto_setmeta (lua_State *L, const char *name);
LUACRYPTO_API void luacrypto_set_info (lua_State *L);


int luaopen_crypto(lua_State * L)
	/*@modifies L @*/;

#define	CRYPTO_OPENSSL	1

#endif
