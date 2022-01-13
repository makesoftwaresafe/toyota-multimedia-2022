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

#define BEECRYPT_CXX_DLL_EXPORT

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/beeyond/BeeCertificate.h"
#include "beecrypt/c++/beeyond/AnyEncodedKeySpec.h"
#include "beecrypt/c++/io/ByteArrayInputStream.h"
using beecrypt::io::ByteArrayInputStream;
#include "beecrypt/c++/io/ByteArrayOutputStream.h"
using beecrypt::io::ByteArrayOutputStream;
#include "beecrypt/c++/lang/ClassCastException.h"
using beecrypt::lang::ClassCastException;
#include "beecrypt/c++/lang/Cloneable.h"
using beecrypt::lang::Cloneable;
#include "beecrypt/c++/lang/Long.h"
using beecrypt::lang::Long;
#include "beecrypt/c++/lang/StringBuilder.h"
using beecrypt::lang::StringBuilder;
#include "beecrypt/c++/lang/NullPointerException.h"
using beecrypt::lang::NullPointerException;
#include "beecrypt/c++/security/KeyFactory.h"
using beecrypt::security::KeyFactory;
#include "beecrypt/c++/security/Signature.h"
using beecrypt::security::Signature;
#include "beecrypt/c++/security/cert/CertificateFactory.h"
using beecrypt::security::cert::CertificateFactory;

#include <memory>
using std::auto_ptr;

using namespace beecrypt::beeyond;

Certificate* BeeCertificate::cloneCertificate(const Certificate& cert) throw (CloneNotSupportedException)
{
	const Cloneable* c = dynamic_cast<const Cloneable*>(&cert);
	if (c)
	{
		return dynamic_cast<Certificate*>(c->clone());
	}
	else
	{
		ByteArrayInputStream bis(cert.getEncoded());

		try
		{
			auto_ptr<CertificateFactory> cf(CertificateFactory::getInstance(cert.getType()));

			return cf->generateCertificate(bis);
		}
		catch (CertificateException&)
		{
			throw CloneNotSupportedException("Unable to clone Certificate through its encoding");
		}
		catch (NoSuchAlgorithmException&)
		{
			throw CloneNotSupportedException("Unable to clone Certificate through CertificateFactory of type " + cert.getType());
		}
	}
}

PublicKey* BeeCertificate::clonePublicKey(const PublicKey& key) throw (CloneNotSupportedException)
{
	const Cloneable* c = dynamic_cast<const Cloneable*>(&key);
	if (c)
	{
		return dynamic_cast<PublicKey*>(c->clone());
	}
	else
	{
		try
		{
			auto_ptr<KeyFactory> kf(KeyFactory::getInstance(key.getAlgorithm()));

			PublicKey* p = dynamic_cast<PublicKey*>(kf->translateKey(key));

			if (p)
				return p;
			else
				throw ClassCastException("KeyFactory didn't translate key into PublicKey");
		}
		catch (NoSuchAlgorithmException&)
		{
			throw CloneNotSupportedException("Unable to clone key through KeyFactory of type " + key.getAlgorithm());
		}
		catch (InvalidKeyException&)
		{
			throw CloneNotSupportedException("Unable to clone PublicKey because KeyFactory says it's invalid");
		}
	}
}

BeeCertificate::UnknownField::UnknownField() throw ()
{
}

BeeCertificate::UnknownField::UnknownField(const UnknownField& copy) throw () : encoding(copy.encoding)
{
	type = copy.type;
}

BeeCertificate::Field* BeeCertificate::UnknownField::clone() const throw ()
{
	return new BeeCertificate::UnknownField(*this);
}

void BeeCertificate::UnknownField::decode(DataInputStream& in) throw (IOException)
{
	encoding.resize(in.available());

	in.readFully(encoding);
}

void BeeCertificate::UnknownField::encode(DataOutputStream& out) const throw (IOException)
{
	out.write(encoding);
}

const jint BeeCertificate::PublicKeyField::FIELD_TYPE = 0x5055424b; // 'PUBK'

BeeCertificate::PublicKeyField::PublicKeyField() throw ()
{
	type = BeeCertificate::PublicKeyField::FIELD_TYPE;
	pub = 0;
}

BeeCertificate::PublicKeyField::PublicKeyField(const PublicKey& key) throw (CloneNotSupportedException)
{
	type = BeeCertificate::PublicKeyField::FIELD_TYPE;
	pub = clonePublicKey(key);
}

BeeCertificate::PublicKeyField::~PublicKeyField()
{
	delete pub;
}

BeeCertificate::Field* BeeCertificate::PublicKeyField::clone() const throw (CloneNotSupportedException)
{
	return new BeeCertificate::PublicKeyField(*pub);
}

void BeeCertificate::PublicKeyField::decode(DataInputStream& in) throw (IOException)
{
	String algorithm = in.readUTF();
	String format = in.readUTF();

	jint encsize = in.readInt();

	if (encsize <= 0)
		throw IOException("Invalid key encoding size");

	bytearray enc(encsize);

	in.readFully(enc);

	AnyEncodedKeySpec spec(format, enc);

	try
	{
		auto_ptr<KeyFactory> kf(KeyFactory::getInstance(algorithm));

		pub = kf->generatePublic(spec);
	}
	catch (InvalidKeySpecException&)
	{
		throw IOException("Invalid key spec");
	}
	catch (NoSuchAlgorithmException&)
	{
		throw IOException("Invalid key format");
	}
}

void BeeCertificate::PublicKeyField::encode(DataOutputStream& out) const throw (IOException)
{
	out.writeUTF(pub->getAlgorithm());
	out.writeUTF(*pub->getFormat());

	const bytearray* pubenc = pub->getEncoded();
	if (!pubenc)
		throw NullPointerException("PublicKey has no encoding");

	out.writeInt(pubenc->size());
	out.write(*pubenc);
}

const jint BeeCertificate::ParentCertificateField::FIELD_TYPE = 0x43455254; // 'CERT'

BeeCertificate::ParentCertificateField::ParentCertificateField() throw ()
{
	type = BeeCertificate::ParentCertificateField::FIELD_TYPE;
	parent = 0;
}

BeeCertificate::ParentCertificateField::ParentCertificateField(Certificate* cert) throw ()
{
	type = BeeCertificate::ParentCertificateField::FIELD_TYPE;
	parent = cert;
}

BeeCertificate::ParentCertificateField::ParentCertificateField(const Certificate& cert) throw (CloneNotSupportedException)
{
	type = BeeCertificate::ParentCertificateField::FIELD_TYPE;
	parent = cloneCertificate(cert);
}

BeeCertificate::ParentCertificateField::~ParentCertificateField()
{
	delete parent;
}

BeeCertificate::Field* BeeCertificate::ParentCertificateField::clone() const throw (CloneNotSupportedException)
{
	return new BeeCertificate::ParentCertificateField(*parent);
}

void BeeCertificate::ParentCertificateField::decode(DataInputStream& in) throw (IOException)
{
	String type = in.readUTF();

	jint encsize = in.readInt();

	if (encsize <= 0)
		throw IOException("Invalid certificate encoding size");

	bytearray enc(encsize);

	in.readFully(enc);

	ByteArrayInputStream bin(enc);

	try
	{
		auto_ptr<CertificateFactory> cf(CertificateFactory::getInstance(type));

		parent = cf->generateCertificate(bin);
	}
	catch (CertificateException&)
	{
		throw IOException("Invalid certificate encoding");
	}
	catch (NoSuchAlgorithmException&)
	{
		throw IOException("Invalid certificate type");
	}
}

void BeeCertificate::ParentCertificateField::encode(DataOutputStream& out) const throw (IOException)
{
	out.writeUTF(parent->getType());

	try
	{
		const bytearray& parentenc = parent->getEncoded();
			
		out.writeInt(parentenc.size());
		out.write(parentenc);
	}
	catch (CertificateEncodingException&)
	{
		throw IOException("Couldn't encoding certificate");
	}
}

BeeCertificate::Field* BeeCertificate::instantiateField(jint type)
{
	switch (type)
	{
	case PublicKeyField::FIELD_TYPE:
		return new PublicKeyField();

	case ParentCertificateField::FIELD_TYPE:
		return new ParentCertificateField();

	default:
		return new UnknownField();
	}
}

const Date BeeCertificate::FOREVER(Long::MAX_VALUE);

BeeCertificate::BeeCertificate() : Certificate("BEE")
{
	enc = 0;
}

BeeCertificate::BeeCertificate(InputStream& in) throw (IOException) : Certificate("BEE")
{
	enc = 0;

	DataInputStream dis(in);

	issuer = dis.readUTF();
	subject = dis.readUTF();

	created.setTime(dis.readLong());
	expires.setTime(dis.readLong());

	jint fieldCount = dis.readInt();
	if (fieldCount < 0)
		throw IOException("field count < 0");

	for (jint i = 0; i < fieldCount; i++)
	{
		jint type = dis.readInt();
		jint fieldSize = dis.readInt();

		if (fieldSize < 0)
			throw IOException("field size < 0");

		bytearray fenc(fieldSize);

		dis.readFully(fenc);

		ByteArrayInputStream bis(fenc);
		DataInputStream fis(bis);

		Field* f = instantiateField(type);

		try
		{
			f->decode(fis);
			fields.add(f);
		}
		catch (...)
		{
			delete f;
			throw;
		}
	}

	signatureAlgorithm = dis.readUTF();

	jint sigSize = dis.readInt();
	if (sigSize < 0)
		throw IOException("signature size < 0");

	if (sigSize > 0)
	{
		signature.resize(sigSize);
		dis.readFully(signature);
	}
}

BeeCertificate::BeeCertificate(const BeeCertificate& copy) throw (CloneNotSupportedException) : Certificate("BEE")
{
	issuer = copy.issuer;
	subject = copy.subject;
	created = copy.created;
	expires = copy.expires;

	for (int i = 0; i < copy.fields.size(); i++)
		fields.add(copy.fields.get(i)->clone());

	signatureAlgorithm = copy.signatureAlgorithm;
	signature = copy.signature;
	enc = 0;
}

BeeCertificate::~BeeCertificate()
{
	delete enc;
}

BeeCertificate* BeeCertificate::clone() const throw ()
{
	return new BeeCertificate(*this);
}

const bytearray& BeeCertificate::getEncoded() const throw (CertificateEncodingException)
{
	if (!enc)
	{
		ByteArrayOutputStream bos;
		DataOutputStream dos(bos);

		try
		{
			encodeTBS(dos);

			dos.writeUTF(signatureAlgorithm);
			dos.writeInt(signature.size());
			dos.write(signature);

			dos.close();
			bos.close();

			enc = bos.toByteArray();
		}
		catch (IOException& e)
		{
			throw CertificateEncodingException().initCause(e);
		}
	}

	return *enc;
}

const PublicKey& BeeCertificate::getPublicKey() const
{
	for (int i = 0; i < fields.size(); i++)
	{
		Field* f = fields.get(i);
		if (f->type == PublicKeyField::FIELD_TYPE)
		{
			const PublicKeyField* pkf = dynamic_cast<const PublicKeyField*>(f);

			if (pkf)
				return *pkf->pub;
			else
				throw GeneralSecurityException("Somebody's trying to cheat with a new Field subclass");
		}
	}

	throw CertificateException("no PublicKeyField in this certificate; test first with hasPublicKey()");
}

const Certificate& BeeCertificate::getParentCertificate() const
{
	for (int i = 0; i < fields.size(); i++)
	{
		Field* f = fields.get(i);
		if (f->type == ParentCertificateField::FIELD_TYPE)
		{
			const ParentCertificateField* pcf = dynamic_cast<const ParentCertificateField*>(f);

			if (pcf)
				return *pcf->parent;
			else
				throw GeneralSecurityException("Somebody's trying to cheat with a new Field subclass");
		}
	}

	throw CertificateException("no ParentCertificateField in this certificate; test first with hasParentCertificate()");
}

void BeeCertificate::verify(const PublicKey& pub) const throw (CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException, SignatureException)
{
	auto_ptr<Signature> sig(Signature::getInstance(signatureAlgorithm));
	auto_ptr<bytearray> tmp(encodeTBS());

	sig->initVerify(pub);
	sig->update(*tmp.get());
	if (!sig->verify(signature))
		throw CertificateException("signature doesn't match");
}

void BeeCertificate::verify(const PublicKey& pub, const String& sigProvider) const throw (CertificateException, NoSuchAlgorithmException, InvalidKeyException, NoSuchProviderException, SignatureException)
{
	auto_ptr<Signature> sig(Signature::getInstance(signatureAlgorithm, sigProvider));
	auto_ptr<bytearray> tmp(encodeTBS());

	sig->initVerify(pub);
	sig->update(*tmp.get());
	if (!sig->verify(signature))
		throw CertificateException("signature doesn't match");
}

String BeeCertificate::toString() const throw ()
{
	StringBuilder tmp("Bee Certificate issued by ");

	tmp.append(issuer).append(" subject ").append(subject).append(" valid from ").append(created.toString()).append(" until ").append(expires.equals(FOREVER) ? "-" : expires.toString());

	/*!\todo add fields
	*/

	return tmp.toString();
}

void BeeCertificate::checkValidity() const throw (CertificateExpiredException, CertificateNotYetValidException)
{
	checkValidity(Date());
}

void BeeCertificate::checkValidity(const Date& at) const throw (CertificateExpiredException, CertificateNotYetValidException)
{
	if (at.before(created))
		throw CertificateNotYetValidException();

	if (!expires.equals(FOREVER))
		if (at.after(expires))
			throw CertificateExpiredException();

	if (hasParentCertificate())
	{
		// parent certificate had to be valid when this one was created
		const BeeCertificate* tmp = dynamic_cast<const BeeCertificate*>(&getParentCertificate());
		if (tmp)
			tmp->checkValidity(created);
	}
}

const Date& BeeCertificate::getNotAfter() const throw ()
{
	return expires;
}

const Date& BeeCertificate::getNotBefore() const throw ()
{
	return created;
}

const bytearray& BeeCertificate::getSignature() const throw ()
{
	return signature;
}

const String& BeeCertificate::getSigAlgName() const throw ()
{
	return signatureAlgorithm;
}

bool BeeCertificate::hasPublicKey() const
{
	for (int i = 0; i < fields.size(); i++)
	{
		Field* f = fields.get(i);
		if (f->type == PublicKeyField::FIELD_TYPE)
		{
			// do an extra check with dynamic_cast
			if (dynamic_cast<PublicKeyField*>(f))
				return true;
			else
				throw GeneralSecurityException("Somebody's trying to cheat with a new Field subclass");
		}
	}
	return false;
}

bool BeeCertificate::hasParentCertificate() const
{
	for (int i = 0; i < fields.size(); i++)
	{
		Field* f = fields.get(i);
		if (f->type == ParentCertificateField::FIELD_TYPE)
		{
			// do an extra check with dynamic_cast
			if (dynamic_cast<ParentCertificateField*>(f))
				return true;
			else
				throw GeneralSecurityException("Somebody's trying to cheat with a new Field subclass");
		}
	}
	return false;
}

bool BeeCertificate::isSelfSignedCertificate() const
{
	// if there's a parent certificate, this certificate isn't self-signed
	if (hasParentCertificate())
		return false;

	// no public key means that we cannot verify
	if (!hasPublicKey())
		return false;

	try
	{
		verify(getPublicKey());

		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

void BeeCertificate::encodeTBS(DataOutputStream& out) const throw (IOException)
{
	out.writeUTF(issuer);
	out.writeUTF(subject);
	out.writeLong(created.getTime());
	out.writeLong(expires.getTime());
	out.writeInt(fields.size());

	for (int i = 0; i < fields.size(); i++)
	{
		ByteArrayOutputStream bout;
		DataOutputStream dout(bout);

		Field* f = fields.get(i);

		f->encode(dout);
		dout.close();

		bytearray fenc;

		bout.toByteArray(fenc);

		out.writeInt(f->type);
		out.writeInt(fenc.size());
		out.write(fenc);
	}
}

bytearray* BeeCertificate::encodeTBS() const throw (CertificateEncodingException)
{
	ByteArrayOutputStream bos;
	DataOutputStream dos(bos);

	try
	{
		encodeTBS(dos);
		dos.close();
	}
	catch (IOException& e)
	{
		throw CertificateEncodingException().initCause(e);
	}

	return bos.toByteArray();
}

BeeCertificate* BeeCertificate::self(const PublicKey& pub, const PrivateKey& pri, const String& signatureAlgorithm) throw (InvalidKeyException, CertificateEncodingException, SignatureException, NoSuchAlgorithmException)
{
	// if the public key doesn't have an encoding, it's not worth going through the effort
	if (!pub.getEncoded())
		throw InvalidKeyException("PublicKey doesn't have an encoding");

	auto_ptr<Signature> sig(Signature::getInstance(signatureAlgorithm));

	sig->initSign(pri);

	BeeCertificate* cert = new BeeCertificate();

	try
	{
		// issuer is kept blank
		cert->subject = String("Public Key Certificate");
		cert->expires = FOREVER;
		cert->signatureAlgorithm = signatureAlgorithm;
		cert->fields.add(new PublicKeyField(pub));

		auto_ptr<bytearray> tmp(cert->encodeTBS());

		sig->update(*tmp.get());
		sig->sign(cert->signature);
	}
	catch (...)
	{
		delete cert;
		throw;
	}

	return cert;
}

BeeCertificate* BeeCertificate::make(const PublicKey& pub, const PrivateKey& pri, const String& signatureAlgorithm, const Certificate& parent) throw (InvalidKeyException, CertificateEncodingException, SignatureException, NoSuchAlgorithmException)
{
	// if the public key doesn't have an encoding, it's not worth going through the effort
	if (!pub.getEncoded())
		throw InvalidKeyException("PublicKey doesn't have an encoding");

	auto_ptr<Signature> sig(Signature::getInstance(signatureAlgorithm));

	sig->initSign(pri);

	BeeCertificate* cert = new BeeCertificate();

	try
	{
		// issuer is kept blank
		cert->subject = String("Public Key Certificate");
		cert->expires = FOREVER;
		cert->signatureAlgorithm = signatureAlgorithm;
		cert->fields.add(new PublicKeyField(pub));
		cert->fields.add(new ParentCertificateField(parent));

		auto_ptr<bytearray> tmp(cert->encodeTBS());

		sig->update(*tmp);
		sig->sign(cert->signature);
	}
	catch (...)
	{
		delete cert;
		throw;
	}

	return cert;
}
