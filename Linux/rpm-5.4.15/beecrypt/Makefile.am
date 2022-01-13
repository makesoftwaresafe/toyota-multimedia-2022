#
# Makefile.am contains the top-level automake definitions
#
# Copyright (c) 2001, 2002, 2004 X-Way Rights BV
# Copyright (c) 2009 Bob Deblier
#
# Author: Bob Deblier <bob.deblier@telenet.be>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

#
# 1. No interfaces changes (good): Increment REVISION
#
# 2. Interfaces added, none removed (good): Increment CURRENT, increment AGE and REVISION to 0.
#
# 3. Interfaces removed (bad): Increment CURRENT, set AGE and REVISION to 0.
#

LIBBEECRYPT_LT_CURRENT = 7
LIBBEECRYPT_LT_AGE = 0
LIBBEECRYPT_LT_REVISION = 0

AUTOMAKE_OPTIONS = gnu check-news no-dependencies
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . include $(MAYBE_SUB) tests docs gas masm

if WITH_CPLUSPLUS
SUBDIRS += c++
endif

if WITH_JAVA
SUBDIRS += java
endif

if WITH_PYTHON
SUBDIRS += python
endif

SUFFIXES = .s

AM_CFLAGS = $(OPENMP_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/include

.s.lo:
	$(LTCOMPILE) -c -o $@ `test -f $< || echo '$(srcdir)/'`$<

BEECRYPT_OBJECTS = aes.lo base64.lo beecrypt.lo blockmode.lo blockpad.lo blowfish.lo blowfishopt.lo dhies.lo dldp.lo dlkp.lo dlpk.lo dlsvdp-dh.lo dsa.lo elgamal.lo endianness.lo entropy.lo fips186.lo hmac.lo hmacmd5.lo hmacsha1.lo hmacsha224.lo hmacsha256.lo md4.lo md5.lo memchunk.lo mp.lo mpopt.lo mpbarrett.lo mpnumber.lo mpprime.lo mtprng.lo pkcs1.lo pkcs12.lo ripemd128.lo ripemd160.lo ripemd256.lo ripemd320.lo rsa.lo rsakp.lo rsapk.lo sha1.lo sha1opt.lo sha256.lo sha384.lo sha512.lo sha2k32.lo sha2k64.lo timestamp.lo

noinst_LTLIBRARIES = libbeecrypt.la

libbeecrypt_la_SOURCES = aes.c base64.c beecrypt.c blockmode.c blockpad.c blowfish.c dhies.c dldp.c dlkp.c dlpk.c dlsvdp-dh.c dsa.c elgamal.c endianness.c entropy.c fips186.c hmac.c hmacmd5.c hmacsha1.c hmacsha224.c hmacsha256.c md4.c md5.c hmacsha384.c hmacsha512.c memchunk.c mp.c mpbarrett.c mpnumber.c mpprime.c mtprng.c pkcs1.c pkcs12.c ripemd128.c ripemd160.c ripemd256.c ripemd320.c rsa.c rsakp.c rsapk.c sha1.c sha224.c sha256.c sha384.c sha512.c sha2k32.c sha2k64.c timestamp.c # cppglue.cxx
libbeecrypt_la_DEPENDENCIES = $(BEECRYPT_OBJECTS)
libbeecrypt_la_LIBADD = blowfishopt.lo mpopt.lo sha1opt.lo $(OPENMP_LIBS)
libbeecrypt_la_LDFLAGS = -no-undefined -version-info $(LIBBEECRYPT_LT_CURRENT):$(LIBBEECRYPT_LT_REVISION):$(LIBBEECRYPT_LT_AGE)

EXTRA_DIST = BENCHMARKS BUGS CONTRIBUTORS README.WIN32 autogen.sh

DISTCLEANFILES = mpopt.s blowfishopt.s sha1opt.s

bench:
	(cd tests && $(MAKE) $(AM_MAKEFLAGS) bench)
