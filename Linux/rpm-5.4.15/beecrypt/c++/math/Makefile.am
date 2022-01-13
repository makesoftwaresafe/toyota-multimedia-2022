AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libcxxmath.la

cxxmathdir=$(pkgincludedir)/c++/math

libcxxmath_la_SOURCES = \
BigInteger.cxx

TESTS = testbigint

check_PROGRAMS = testbigint

testbigint_SOURCES = testbigint.cxx
testbigint_LDADD = ../libbeecrypt_cxx.la ../../libbeecrypt.la
