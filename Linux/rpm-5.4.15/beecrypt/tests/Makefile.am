#
# Makefile.am's purpose is to build the unit test programs and benchmarks.
#
# Copyright (c) 2001, 2002, 2003 X-Way Rights BV
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

AUTOMAKE_OPTIONS = gnu no-dependencies

AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = $(top_builddir)/libbeecrypt.la

TESTS = testmd5 testripemd128 testripemd160 testripemd256 testripemd320 testsha1 testsha224 testsha256 testsha384 testsha512 testhmacmd5 testhmacsha1 testaes testblowfish testmp testmpinv testdsa testrsa testrsacrt testdldp testelgamal

check_PROGRAMS = testmd5 testripemd128 testripemd160 testripemd256 testripemd320 testsha1 testsha224 testsha256 testsha384 testsha512 testhmacmd5 testhmacsha1 testaes testblowfish testmp testmpinv testdsa testrsa testrsacrt testdldp testelgamal

testmd5_SOURCES = testmd5.c

testripemd128_SOURCES = testripemd128.c

testripemd160_SOURCES = testripemd160.c

testripemd256_SOURCES = testripemd256.c

testripemd320_SOURCES = testripemd320.c

testsha1_SOURCES = testsha1.c

testsha224_SOURCES = testsha224.c

testsha256_SOURCES = testsha256.c

testsha384_SOURCES = testsha384.c

testsha512_SOURCES = testsha512.c

testhmacmd5_SOURCES = testhmacmd5.c

testhmacsha1_SOURCES = testhmacsha1.c

testaes_SOURCES = testaes.c testutil.c

testblowfish_SOURCES = testblowfish.c testutil.c

testmp_SOURCES = testmp.c

testmpinv_SOURCES = testmpinv.c

testdsa_SOURCES = testdsa.c

testrsa_SOURCES = testrsa.c

testrsacrt_SOURCES = testrsacrt.c

testdldp_SOURCES = testdldp.c

testelgamal_SOURCES = testelgamal.c

EXTRA_PROGRAMS = benchme benchrsa benchhf benchbc

benchme_SOURCES = benchme.c

benchrsa_SOURCES = benchrsa.c

benchhf_SOURCES = benchhf.c

benchbc_SOURCES = benchbc.c

# Run every benchmark test twice
bench: benchme benchrsa benchhf benchbc
	./benchme
	./benchme
	./benchrsa
	./benchrsa
	./benchhf MD5
	./benchhf MD5
	./benchhf SHA-1
	./benchhf SHA-1
	./benchhf SHA-256
	./benchhf SHA-256
	./benchhf SHA-512
	./benchhf SHA-512
	./benchbc AES 128
	./benchbc AES 128
	./benchbc Blowfish 128
	./benchbc Blowfish 128
