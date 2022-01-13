#
# Copyright (c) 2003, 2005 X-Way Rights BV
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

LIBBEECRYPT_JAVA_LT_CURRENT = 7
LIBBEECRYPT_JAVA_LT_AGE = 0
LIBBEECRYPT_JAVA_LT_REVISION = 0

AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libbeecrypt_java.la

libbeecrypt_java_la_SOURCES = \
beecrypt_tools.c \
beecrypt_provider_AES.c \
beecrypt_provider_MD4.c \
beecrypt_provider_MD5.c \
beecrypt_provider_SHA1.c \
beecrypt_provider_SHA224.c \
beecrypt_provider_SHA256.c \
beecrypt_provider_SHA384.c \
beecrypt_provider_SHA512.c \
beecrypt_provider_HMACMD5.c \
beecrypt_provider_HMACSHA1.c \
beecrypt_provider_HMACSHA256.c \
beecrypt_provider_HMACSHA384.c \
beecrypt_provider_HMACSHA512.c \
beecrypt_provider_DHKeyPairGenerator.c \
beecrypt_provider_RSAKeyPairGenerator.c

libbeecrypt_java_la_LIBADD = $(top_builddir)/libbeecrypt.la
libbeecrypt_java_la_LDFLAGS = -no-undefined -version-info $(LIBBEECRYPT_JAVA_LT_CURRENT):$(LIBBEECRYPT_JAVA_LT_REVISION):$(LIBBEECRYPT_JAVA_LT_AGE)

EXTRA_DIST = \
src/beecrypt/beeyond/AnyEncodedKeySpec.java \
src/beecrypt/beeyond/BeeCertificate.java \
src/beecrypt/beeyond/BeeEncodedKeySpec.java \
src/beecrypt/beeyond/PKCS12PBEKey.java \
src/beecrypt/crypto/DHPrivateKeyImpl.java \
src/beecrypt/crypto/DHPublicKeyImpl.java \
src/beecrypt/crypto/spec/DHIESDecryptParameterSpec.java \
src/beecrypt/crypto/spec/DHIESParameterSpec.java \
src/beecrypt/io/BeeInputStream.java \
src/beecrypt/io/BeeOutputStream.java \
src/beecrypt/io/MacInputStream.java \
src/beecrypt/io/MacOutputStream.java \
src/beecrypt/provider/BaseProvider.java \
src/beecrypt/provider/BeeKeyStore.java \
src/beecrypt/provider/DHIESParameters.java \
src/beecrypt/provider/DHKeyFactory.java \
src/beecrypt/provider/DSAKeyFactory.java \
src/beecrypt/provider/KeyProtector.java \
src/beecrypt/provider/MD4.java \
src/beecrypt/provider/MD5.java \
src/beecrypt/provider/PKCS12.java \
src/beecrypt/provider/RSAKeyFactory.java \
src/beecrypt/provider/RSAKeyPairGenerator.java \
src/beecrypt/provider/SHA1.java \
src/beecrypt/provider/SHA224.java \
src/beecrypt/provider/SHA256.java \
src/beecrypt/provider/SHA384.java \
src/beecrypt/provider/SHA512.java \
src/beecrypt/security/DSAPrivateKeyImpl.java \
src/beecrypt/security/DSAPublicKeyImpl.java \
src/beecrypt/security/RSAPrivateCrtKeyImpl.java \
src/beecrypt/security/RSAPrivateKeyImpl.java \
src/beecrypt/security/RSAPublicKeyImpl.java
