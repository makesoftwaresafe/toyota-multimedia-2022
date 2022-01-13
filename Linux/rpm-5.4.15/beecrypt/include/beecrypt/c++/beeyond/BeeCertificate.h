/*
 * Copyright (c) 2004 X-Way Rights BV
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

/*!\file BeeCertificate.h
 * \ingroup CXX_BEEYOND_m
 */

#ifndef _CLASS_BEE_BEEYOND_BEECERTIFICATE_H
#define _CLASS_BEE_BEEYOND_BEECERTIFICATE_H

#ifdef __cplusplus

#include "beecrypt/c++/array.h"
using beecrypt::array;
#include "beecrypt/c++/io/DataInputStream.h"
using beecrypt::io::DataInputStream;
#include "beecrypt/c++/io/DataOutputStream.h"
using beecrypt::io::DataOutputStream;
#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/provider/BeeCertificateFactory.h"
using beecrypt::provider::BeeCertificateFactory;
#include "beecrypt/c++/security/PublicKey.h"
using beecrypt::security::PublicKey;
#include "beecrypt/c++/security/PrivateKey.h"
using beecrypt::security::PrivateKey;
#include "beecrypt/c++/security/cert/Certificate.h"
using beecrypt::security::cert::Certificate;
#include "beecrypt/c++/security/cert/CertificateExpiredException.h"
using beecrypt::security::cert::CertificateExpiredException;
#include "beecrypt/c++/security/cert/CertificateNotYetValidException.h"
using beecrypt::security::cert::CertificateNotYetValidException;
#include "beecrypt/c++/util/Date.h"
using beecrypt::util::Date;
#include "beecrypt/c++/util/ArrayList.h"
using beecrypt::util::ArrayList;

namespace beecrypt {
	namespace beeyond {
		/* We use short certificate chains, embedded in the certificate as parent certificates
		* Issuer is informational
		* Subject is used to identify the type of certificate
		*/
		/*!\ingroup CXX_BEEYOND_m
		 */
		class BEECRYPTCXXAPI BeeCertificate : public Certificate, public Cloneable
		{
			friend class ::BeeCertificateFactory;

		public:
			static const Date FOREVER;

			static Certificate* cloneCertificate(const Certificate& cert) throw (CloneNotSupportedException);
			static PublicKey* clonePublicKey(const PublicKey& pub) throw (CloneNotSupportedException);

		protected:
			class BEECRYPTCXXAPI Field : public beecrypt::lang::Object
			{
			public:
				jint type;

				virtual ~Field() {}

				virtual Field* clone() const throw (CloneNotSupportedException) = 0;

				virtual void decode(DataInputStream&) throw (IOException) = 0;
				virtual void encode(DataOutputStream&) const throw (IOException) = 0;
			};

			class BEECRYPTCXXAPI UnknownField : public Field
			{
			public:
				bytearray encoding;

				UnknownField() throw ();
				UnknownField(const UnknownField&) throw ();
				virtual ~UnknownField() {}
					
				virtual Field* clone() const throw ();

				virtual void decode(DataInputStream&) throw (IOException);
				virtual void encode(DataOutputStream&) const throw (IOException);
			};

			class BEECRYPTCXXAPI PublicKeyField : public Field
			{
			public:
				static const jint FIELD_TYPE;

				PublicKey* pub;

				PublicKeyField() throw ();
				PublicKeyField(PublicKey* key) throw ();
				PublicKeyField(const PublicKey& key) throw (CloneNotSupportedException);
				virtual ~PublicKeyField();

				virtual Field* clone() const throw (CloneNotSupportedException);

				virtual void decode(DataInputStream&) throw (IOException);
				virtual void encode(DataOutputStream&) const throw (IOException);
			};

			class BEECRYPTCXXAPI ParentCertificateField : public Field
			{
			public:
				static const jint FIELD_TYPE;

				Certificate* parent;

				ParentCertificateField() throw ();
				ParentCertificateField(Certificate*) throw ();
				ParentCertificateField(const Certificate&) throw (CloneNotSupportedException);
				virtual ~ParentCertificateField();

				virtual Field* clone() const throw (CloneNotSupportedException);

				virtual void decode(DataInputStream&) throw (IOException);
				virtual void encode(DataOutputStream&) const throw (IOException);
			};

			virtual Field* instantiateField(jint type);

		protected:
			String           issuer;
			String           subject;
			Date             created;
			Date             expires;
			ArrayList<Field> fields;
			String           signatureAlgorithm;
			bytearray        signature;

			mutable bytearray* enc;

			BeeCertificate();
			BeeCertificate(InputStream& in) throw (IOException);

			void encodeTBS(DataOutputStream& out) const throw (IOException);

			bytearray* encodeTBS() const throw (CertificateEncodingException);

		public:
			BeeCertificate(const BeeCertificate&) throw (CloneNotSupportedException);
			virtual ~BeeCertificate();

			virtual BeeCertificate* clone() const throw ();

			virtual const bytearray& getEncoded() const throw (CertificateEncodingException);
			virtual const PublicKey& getPublicKey() const;

			virtual void verify(const PublicKey&) const throw (CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException, SignatureException);
			virtual void verify(const PublicKey&, const String&) const throw (CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException, SignatureException);
			virtual String toString() const throw ();

			void checkValidity() const throw (CertificateExpiredException, CertificateNotYetValidException);
			void checkValidity(const Date&) const throw (CertificateExpiredException, CertificateNotYetValidException);

			const String& getIssuer() const throw ();
			const String& getSubject() const throw ();

			const Date& getNotAfter() const throw ();
			const Date& getNotBefore() const throw ();

			const bytearray& getSignature() const throw ();
			const String& getSigAlgName() const throw ();

			bool hasPublicKey() const;
			bool hasParentCertificate() const;

			bool isSelfSignedCertificate() const;

			const Certificate& getParentCertificate() const;

		public:
			static BeeCertificate* self(const PublicKey& pub, const PrivateKey& pri, const String& signatureAlgorithm) throw (InvalidKeyException, CertificateEncodingException, SignatureException, NoSuchAlgorithmException);
			static BeeCertificate* make(const PublicKey& pub, const PrivateKey& pri, const String& signatureAlgorithm, const Certificate& parent) throw (InvalidKeyException, CertificateEncodingException, SignatureException, NoSuchAlgorithmException);

		};

	}
}

#endif

#endif
