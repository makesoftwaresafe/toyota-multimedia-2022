AM_CPPFLAGS = -I$(top_srcdir)/include

LIBS = -licuuc -licuio

AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libcxxbeeyond.la

libcxxbeeyond_la_SOURCES = \
AnyEncodedKeySpec.cxx \
BeeCertificate.cxx \
BeeCertPath.cxx \
BeeCertPathParameters.cxx \
BeeCertPathValidatorResult.cxx \
BeeEncodedKeySpec.cxx \
BeeInputStream.cxx \
BeeOutputStream.cxx \
DHIESParameterSpec.cxx \
DHIESDecryptParameterSpec.cxx \
PKCS12PBEKey.cxx

noinst_DATA = beecrypt-test.conf

TESTS_ENVIRONMENT = BEECRYPT_CONF_FILE=beecrypt-test.conf

TESTS = testcert

CLEANFILES = beecrypt-test.conf

check_PROGRAMS = testcert

testcert_SOURCES = testcert.cxx
testcert_LDADD = ../libbeecrypt_cxx.la

beecrypt-test.conf:
	@echo "provider.1=../provider/.libs/base.so" > beecrypt-test.conf
