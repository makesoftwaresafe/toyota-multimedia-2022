AM_CPPFLAGS = -I$(top_srcdir)/include

LIBS = -licuuc -licuio

AUTOMAKE_OPTIONS = gnu

pkglibdir=$(libdir)/beecrypt
pkglib_LTLIBRARIES = base.la

nodist_sysconf_DATA = beecrypt.conf

base_la_SOURCES = \
AESCipher.cxx \
BeeCertificateFactory.cxx \
BeeCertPathValidator.cxx \
BaseProvider.cxx \
BeeKeyStore.cxx \
BeeSecureRandom.cxx \
BlockCipher.cxx \
BlowfishCipher.cxx \
DHIESCipher.cxx \
DHIESParameters.cxx \
DHKeyAgreement.cxx \
DHKeyFactory.cxx \
DHKeyPairGenerator.cxx \
DHParameterGenerator.cxx \
DHParameters.cxx \
DHPrivateKeyImpl.cxx \
DHPublicKeyImpl.cxx \
DSAKeyFactory.cxx \
DSAKeyPairGenerator.cxx \
DSAParameterGenerator.cxx \
DSAParameters.cxx \
DSAPrivateKeyImpl.cxx \
DSAPublicKeyImpl.cxx \
HMAC.cxx \
HMACMD5.cxx \
HMACSHA1.cxx \
HMACSHA256.cxx \
HMACSHA384.cxx \
HMACSHA512.cxx \
KeyProtector.cxx \
MD5Digest.cxx \
MD5withRSASignature.cxx \
PKCS1RSASignature.cxx \
PKCS12KeyFactory.cxx \
RSAKeyFactory.cxx \
RSAKeyPairGenerator.cxx \
RSAPrivateCrtKeyImpl.cxx \
RSAPrivateKeyImpl.cxx \
RSAPublicKeyImpl.cxx \
SHA1Digest.cxx \
SHA224Digest.cxx \
SHA256Digest.cxx \
SHA384Digest.cxx \
SHA512Digest.cxx \
SHA1withDSASignature.cxx \
SHA1withRSASignature.cxx \
SHA256withRSASignature.cxx \
SHA384withRSASignature.cxx \
SHA512withRSASignature.cxx

base_la_LDFLAGS = -module
base_la_LIBADD = $(top_builddir)/c++/libbeecrypt_cxx.la

CLEANFILES = beecrypt.conf

beecrypt.conf:
	@echo "provider.1=$(pkgaltlibdir)/base.so" > beecrypt.conf
