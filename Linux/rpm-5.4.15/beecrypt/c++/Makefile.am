LIBBEECRYPT_CXX_LT_CURRENT = 7
LIBBEECRYPT_CXX_LT_AGE = 0
LIBBEECRYPT_CXX_LT_REVISION = 0

AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

SUBDIRS = lang math io nio util security crypto beeyond . provider

cxxdir=$(pkgincludedir)/c++

noinst_LTLIBRARIES = libbeecrypt_cxx.la

libbeecrypt_cxx_la_SOURCES = \
adapter.cxx \
resource.cxx

libbeecrypt_cxx_la_LIBADD = ../libbeecrypt.la lang/libcxxlang.la math/libcxxmath.la io/libcxxio.la nio/libcxxnio.la util/libcxxutil.la security/libcxxsecurity.la crypto/libcxxcrypto.la beeyond/libcxxbeeyond.la -licuuc -licuio -licui18n
libbeecrypt_cxx_la_LDFLAGS = -no-undefined -version-info $(LIBBEECRYPT_CXX_LT_CURRENT):$(LIBBEECRYPT_CXX_LT_REVISION):$(LIBBEECRYPT_CXX_LT_AGE)

noinst_DATA = beecrypt-test.conf

TESTS_ENVIRONMENT = BEECRYPT_CONF_FILE=beecrypt-test.conf

TESTS = testks testdsa testrsa testdhies 

CLEANFILES = beecrypt-test.conf

check_PROGRAMS = testks testdsa testrsa testdhies 

testks_SOURCES = testks.cxx
testks_LDADD = libbeecrypt_cxx.la

testdsa_SOURCES = testdsa.cxx
testdsa_LDADD = libbeecrypt_cxx.la

testrsa_SOURCES = testrsa.cxx
testrsa_LDADD = libbeecrypt_cxx.la

testdhies_SOURCES = testdhies.cxx
testdhies_LDADD = libbeecrypt_cxx.la

beecrypt-test.conf:
	@echo "provider.1=provider/.libs/base.so" > beecrypt-test.conf
