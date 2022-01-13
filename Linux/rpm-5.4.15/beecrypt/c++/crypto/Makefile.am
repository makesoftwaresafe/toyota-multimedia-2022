AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

SUBDIRS = spec

noinst_LTLIBRARIES = libcxxcrypto.la

cxxcryptodir=$(pkgincludedir)/c++/crypto

libcxxcrypto_la_SOURCES = \
Cipher.cxx \
CipherSpi.cxx \
KeyAgreement.cxx \
Mac.cxx \
MacInputStream.cxx \
MacOutputStream.cxx \
NullCipher.cxx \
SecretKeyFactory.cxx

libcxxcrypto_la_LIBADD = spec/libcxxcryptospec.la
