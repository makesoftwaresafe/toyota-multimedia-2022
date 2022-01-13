AM_CPPFLAGS = -I$(top_srcdir)/include

AUTOMAKE_OPTIONS = gnu

SUBDIRS = auth cert spec

noinst_LTLIBRARIES = libcxxsecurity.la

cxxsecuritydir=$(pkgincludedir)/c++/security

libcxxsecurity_la_SOURCES =\
AlgorithmParameterGenerator.cxx \
AlgorithmParameters.cxx \
DigestInputStream.cxx \
DigestOutputStream.cxx \
KeyFactory.cxx \
KeyPair.cxx \
KeyPairGenerator.cxx \
KeyStore.cxx \
MessageDigest.cxx \
Provider.cxx \
SecureRandom.cxx \
Security.cxx \
Signature.cxx

libcxxsecurity_la_LIBADD = cert/libcxxsecuritycert.la spec/libcxxsecurityspec.la
