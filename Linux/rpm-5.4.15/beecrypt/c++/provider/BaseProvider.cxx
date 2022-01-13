/*
 * Copyright (c) 2004 Beeyond Software Holding BV
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/provider/AESCipher.h"
#include "beecrypt/c++/provider/BeeCertificateFactory.h"
#include "beecrypt/c++/provider/BeeCertPathValidator.h"
#include "beecrypt/c++/provider/BaseProvider.h"
#include "beecrypt/c++/provider/BeeKeyStore.h"
#include "beecrypt/c++/provider/BeeSecureRandom.h"
#include "beecrypt/c++/provider/BlowfishCipher.h"
#include "beecrypt/c++/provider/DHIESCipher.h"
#include "beecrypt/c++/provider/DHIESParameters.h"
#include "beecrypt/c++/provider/DHKeyAgreement.h"
#include "beecrypt/c++/provider/DHKeyFactory.h"
#include "beecrypt/c++/provider/DHKeyPairGenerator.h"
#include "beecrypt/c++/provider/DHParameterGenerator.h"
#include "beecrypt/c++/provider/DHParameters.h"
#include "beecrypt/c++/provider/DSAKeyFactory.h"
#include "beecrypt/c++/provider/DSAKeyPairGenerator.h"
#include "beecrypt/c++/provider/DSAParameterGenerator.h"
#include "beecrypt/c++/provider/DSAParameters.h"
#include "beecrypt/c++/provider/HMACMD5.h"
#include "beecrypt/c++/provider/HMACSHA1.h"
#include "beecrypt/c++/provider/HMACSHA256.h"
#include "beecrypt/c++/provider/HMACSHA384.h"
#include "beecrypt/c++/provider/HMACSHA512.h"
#include "beecrypt/c++/provider/MD5Digest.h"
#include "beecrypt/c++/provider/MD5withRSASignature.h"
#include "beecrypt/c++/provider/PKCS12KeyFactory.h"
#include "beecrypt/c++/provider/RSAKeyFactory.h"
#include "beecrypt/c++/provider/RSAKeyPairGenerator.h"
#include "beecrypt/c++/provider/SHA1Digest.h"
#include "beecrypt/c++/provider/SHA224Digest.h"
#include "beecrypt/c++/provider/SHA256Digest.h"
#include "beecrypt/c++/provider/SHA384Digest.h"
#include "beecrypt/c++/provider/SHA512Digest.h"
#include "beecrypt/c++/provider/SHA1withDSASignature.h"
#include "beecrypt/c++/provider/SHA1withRSASignature.h"
#include "beecrypt/c++/provider/SHA256withRSASignature.h"
#include "beecrypt/c++/provider/SHA384withRSASignature.h"
#include "beecrypt/c++/provider/SHA512withRSASignature.h"

namespace {
	const String PROVIDER_NAME("BeeCrypt++");
	const String PROVIDER_INFO("Copyright (c) 2004 Beeyond Software Holding BV");
	const double PROVIDER_VERSION = 4.2;
}

extern "C" {

#if WIN32
# define PROVAPI __declspec(dllexport)
#else
# define PROVAPI
#endif

PROVAPI
void* beecrypt_AESCipher_create()
{
	return new beecrypt::provider::AESCipher();
}

PROVAPI
void* beecrypt_BeeCertificateFactory_create()
{
	return new beecrypt::provider::BeeCertificateFactory();
}

PROVAPI
void* beecrypt_BeeCertPathValidator_create()
{
	return new beecrypt::provider::BeeCertPathValidator();
}

PROVAPI
void* beecrypt_BeeKeyStore_create()
{
	return new beecrypt::provider::BeeKeyStore();
}

PROVAPI
void* beecrypt_BeeSecureRandom_create()
{
	return new beecrypt::provider::BeeSecureRandom();
}

PROVAPI
void* beecrypt_BlowfishCipher_create()
{
	return new beecrypt::provider::BlowfishCipher();
}

PROVAPI
void* beecrypt_DHIESCipher_create()
{
	return new beecrypt::provider::DHIESCipher();
}

PROVAPI
void* beecrypt_DHIESParameters_create()
{
	return new beecrypt::provider::DHIESParameters();
}

PROVAPI
void* beecrypt_DHKeyAgreement_create()
{
	return new beecrypt::provider::DHKeyAgreement();
}

PROVAPI
void* beecrypt_DHKeyFactory_create()
{
	return new beecrypt::provider::DHKeyFactory();
}

PROVAPI
void* beecrypt_DHKeyPairGenerator_create()
{
	return new beecrypt::provider::DHKeyPairGenerator();
}

PROVAPI
void* beecrypt_HParameterGenerator_create()
{
	return new beecrypt::provider::DHParameterGenerator();
}

PROVAPI
void* beecrypt_DHParameters_create()
{
	return new beecrypt::provider::DHParameters();
}

PROVAPI
void* beecrypt_DSAKeyFactory_create()
{
	return new beecrypt::provider::DSAKeyFactory();
}

PROVAPI
void* beecrypt_DSAKeyPairGenerator_create()
{
	return new beecrypt::provider::DSAKeyPairGenerator();
}

PROVAPI
void* beecrypt_DSAParameterGenerator_create()
{
	return new beecrypt::provider::DSAParameterGenerator();
}

PROVAPI
void* beecrypt_DSAParameters_create()
{
	return new beecrypt::provider::DSAParameters();
}

PROVAPI
void* beecrypt_HMACMD5_create()
{
	return new beecrypt::provider::HMACMD5();
}

PROVAPI
void* beecrypt_HMACSHA1_create()
{
	return new beecrypt::provider::HMACSHA1();
}

PROVAPI
void* beecrypt_HMACSHA256_create()
{
	return new beecrypt::provider::HMACSHA256();
}

PROVAPI
void* beecrypt_HMACSHA384_create()
{
	return new beecrypt::provider::HMACSHA384();
}

PROVAPI
void* beecrypt_HMACSHA512_create()
{
	return new beecrypt::provider::HMACSHA512();
}

PROVAPI
void* beecrypt_MD5Digest_create()
{
	return new beecrypt::provider::MD5Digest();
}

PROVAPI
void* beecrypt_MD5withRSASignature_create()
{
	return new beecrypt::provider::MD5withRSASignature();
}

PROVAPI
void* beecrypt_PKCS12KeyFactory_create()
{
	return new beecrypt::provider::PKCS12KeyFactory();
}

PROVAPI
void* beecrypt_RSAKeyFactory_create()
{
	return new beecrypt::provider::RSAKeyFactory();
}

PROVAPI
void* beecrypt_RSAKeyPairGenerator_create()
{
	return new beecrypt::provider::RSAKeyPairGenerator();
}

PROVAPI
void* beecrypt_SHA1Digest_create()
{
	return new beecrypt::provider::SHA1Digest();
}

PROVAPI
void* beecrypt_SHA224Digest_create()
{
	return new beecrypt::provider::SHA224Digest();
}

PROVAPI
void* beecrypt_SHA256Digest_create()
{
	return new beecrypt::provider::SHA256Digest();
}

PROVAPI
void* beecrypt_SHA384Digest_create()
{
	return new beecrypt::provider::SHA384Digest();
}

PROVAPI
void* beecrypt_SHA512Digest_create()
{
	return new beecrypt::provider::SHA512Digest();
}

PROVAPI
void* beecrypt_SHA1withDSASignature_create()
{
	return new beecrypt::provider::SHA1withDSASignature();
}

PROVAPI
void* beecrypt_SHA1withRSASignature_create()
{
	return new beecrypt::provider::SHA1withRSASignature();
}

PROVAPI
void* beecrypt_SHA256withRSASignature_create()
{
	return new beecrypt::provider::SHA256withRSASignature();
}

PROVAPI
void* beecrypt_SHA384withRSASignature_create()
{
	return new beecrypt::provider::SHA384withRSASignature();
}

PROVAPI
void* beecrypt_SHA512withRSASignature_create()
{
	return new beecrypt::provider::SHA512withRSASignature();
}

}

using namespace beecrypt::provider;

BaseProvider::BaseProvider() : Provider(PROVIDER_NAME, PROVIDER_VERSION, PROVIDER_INFO)
{
	_dlhandle = 0;

	init();
}

BaseProvider::BaseProvider(void* dlhandle) : Provider(PROVIDER_NAME, PROVIDER_VERSION, PROVIDER_INFO)
{
	_dlhandle = dlhandle;

	init();
}

void BaseProvider::init()
{
	setProperty("AlgorithmParameterGenerator.DH"           , "beecrypt_DHParameterGenerator_create");
	setProperty("AlgorithmParameterGenerator.DSA"          , "beecrypt_DSAParameterGenerator_create");
	setProperty("AlgorithmParameters.DH"                   , "beecrypt_DHParameters_create");
	setProperty("AlgorithmParameters.DHIES"                , "beecrypt_DHIESParameters_create");
	setProperty("AlgorithmParameters.DSA"                  , "beecrypt_DSAParameters_create");
	setProperty("CertificateFactory.BEE"                   , "beecrypt_BeeCertificateFactory_create");
	setProperty("CertPathValidator.BEE"                    , "beecrypt_BeeCertPathValidator_create");
	setProperty("Cipher.AES"                               , "beecrypt_AESCipher_create");
	setProperty("Cipher.AES SupportedPaddings"             , "NOPADDING,PKCS5Padding");
	setProperty("Cipher.Blowfish"                          , "beecrypt_BlowfishCipher_create");
	setProperty("Cipher.Blowfish SupportedPaddings"        , "NOPADDING,PKCS5Padding");
	setProperty("Cipher.DHIES"                             , "beecrypt_DHIESCipher_create");
	setProperty("KeyAgreement.DH"                          , "beecrypt_DHKeyAgreement_create");
	setProperty("KeyFactory.DH"                            , "beecrypt_DHKeyFactory_create");
	setProperty("KeyFactory.DSA"                           , "beecrypt_DSAKeyFactory_create");
	setProperty("KeyFactory.RSA"                           , "beecrypt_RSAKeyFactory_create");
	setProperty("KeyStore.BEE"                             , "beecrypt_BeeKeyStore_create");
	setProperty("KeyPairGenerator.DH"                      , "beecrypt_DHKeyPairGenerator_create");
	setProperty("KeyPairGenerator.DSA"                     , "beecrypt_DSAKeyPairGenerator_create");
	setProperty("KeyPairGenerator.RSA"                     , "beecrypt_RSAKeyPairGenerator_create");
	setProperty("Mac.HmacMD5"                              , "beecrypt_HMACMD5_create");
	setProperty("Mac.HmacSHA1"                             , "beecrypt_HMACSHA1_create");
	setProperty("Mac.HmacSHA256"                           , "beecrypt_HMACSHA256_create");
	setProperty("Mac.HmacSHA384"                           , "beecrypt_HMACSHA384_create");
	setProperty("Mac.HmacSHA512"                           , "beecrypt_HMACSHA512_create");
	setProperty("MessageDigest.MD5"                        , "beecrypt_MD5Digest_create");
	setProperty("MessageDigest.SHA-1"                      , "beecrypt_SHA1Digest_create");
	setProperty("MessageDigest.SHA-256"                    , "beecrypt_SHA256Digest_create");
	setProperty("MessageDigest.SHA-384"                    , "beecrypt_SHA384Digest_create");
	setProperty("MessageDigest.SHA-512"                    , "beecrypt_SHA512Digest_create");
	setProperty("SecretKeyFactory.PKCS#12/PBE"             , "beecrypt_PKCS12KeyFactory_create");
	setProperty("SecureRandom.BEE"                         , "beecrypt_BeeSecureRandom_create");
	setProperty("Signature.MD5withRSA"                     , "beecrypt_MD5withRSASignature_create");
	setProperty("Signature.SHA1withDSA"                    , "beecrypt_SHA1withDSASignature_create");
	setProperty("Signature.SHA1withRSA"                    , "beecrypt_SHA1withRSASignature_create");
	setProperty("Signature.SHA256withRSA"                  , "beecrypt_SHA256withRSASignature_create");
	setProperty("Signature.SHA384withRSA"                  , "beecrypt_SHA384withRSASignature_create");
	setProperty("Signature.SHA512withRSA"                  , "beecrypt_SHA512withRSASignature_create");
	setProperty("Alg.Alias.Cipher.DHAES"                   , "Cipher.DHIES");
	setProperty("Alg.Alias.Cipher.DHES"                    , "Cipher.DHIES");
	setProperty("Alg.Alias.KeyAgreement.DiffieHellman"     , "KeyAgreement.DH");
	setProperty("Alg.Alias.KeyFactory.DiffieHellman"       , "KeyFactory.DH");
	setProperty("Alg.Alias.KeyPairGenerator.DiffieHellman" , "KeyPairGenerator.DH");
	setProperty("Alg.Alias.Mac.HMAC-MD5"                   , "Mac.HmacMD5");
	setProperty("Alg.Alias.Mac.HMAC-SHA-1"                 , "Mac.HmacSHA1");
	setProperty("Alg.Alias.Mac.HMAC-SHA-256"               , "Mac.HmacSHA256");
	setProperty("Alg.Alias.Mac.HMAC-SHA-384"               , "Mac.HmacSHA384");
	setProperty("Alg.Alias.Mac.HMAC-SHA-512"               , "Mac.HmacSHA512");
	setProperty("Alg.Alias.Signature.DSS"                  , "Signature.SHA1withDSA");
	setProperty("Alg.Alias.Signature.SHAwithDSA"           , "Signature.SHA1withDSA");
	setProperty("Alg.Alias.Signature.SHA/DSA"              , "Signature.SHA1withDSA");
	setProperty("Alg.Alias.Signature.SHA-1/DSA"            , "Signature.SHA1withDSA");
}

extern "C" {

#if WIN32
__declspec(dllexport)
#endif
Provider* provider_instantiate(void* dlhandle)
{
	return new BaseProvider(dlhandle);
}

}
