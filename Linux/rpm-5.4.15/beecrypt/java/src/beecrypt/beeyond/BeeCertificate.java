package beecrypt.beeyond;

import java.io.*;
import java.security.*;
import java.security.cert.*;
import java.security.cert.Certificate;
import java.security.spec.*;
import java.util.*;

public class BeeCertificate extends Certificate {
	public static final Date FOREVER = new Date(Long.MAX_VALUE);

	protected static abstract class Field {
		public int type;

		public abstract void decode(DataInputStream in) throws IOException;

		public abstract void encode(DataOutputStream out) throws IOException;
	}

	protected static class UnknownField extends Field implements Cloneable {
		byte[] encoding;

		public UnknownField() {
		}

		public UnknownField(int type, byte[] encoding) {
			this.type = type;
			this.encoding = encoding;
		}

		public Object clone() {
			return new UnknownField(type, encoding);
		}

		public void decode(DataInputStream in) throws IOException {
			encoding = new byte[in.available()];
			in.readFully(encoding);
		}

		public void encode(DataOutputStream out) throws IOException {
			out.write(encoding);
		}
	}

	protected static class PublicKeyField extends Field implements Cloneable {
		public static final int FIELD_TYPE = 0x5055424b;

		public PublicKey pub;

		public PublicKeyField() {
			this.type = FIELD_TYPE;
		}

		public PublicKeyField(PublicKey pub) {
			this.type = FIELD_TYPE;
			this.pub = pub;
		}

		public Object clone() {
			return new PublicKeyField(pub);
		}

		public void decode(DataInputStream in) throws IOException {
			String algorithm = in.readUTF();
			String format = in.readUTF();

			int encsize = in.readInt();

			if (encsize <= 0)
				throw new IOException("Invalid key encoding size");

			byte[] enc = new byte[encsize];

			in.readFully(enc);

			AnyEncodedKeySpec spec = new AnyEncodedKeySpec(format, enc);

			try {
				KeyFactory kf = KeyFactory.getInstance(algorithm);

				pub = kf.generatePublic(spec);
			} catch (InvalidKeySpecException e) {
				throw new IOException("Invalid key spec");
			} catch (NoSuchAlgorithmException e) {
				throw new IOException("Invalid key format");
			}
		}

		public void encode(DataOutputStream out) throws IOException {
			out.writeUTF(pub.getAlgorithm());
			out.writeUTF(pub.getFormat());

			byte[] enc = pub.getEncoded();
			if (enc == null)
				throw new IOException("Couldn't encode key");

			out.writeInt(enc.length);
			out.write(enc);
		}
	}

	protected static class ParentCertificateField extends Field {
		public static final int FIELD_TYPE = 0x43455254;

		public Certificate parent;

		public ParentCertificateField() {
			this.type = FIELD_TYPE;
		}

		public ParentCertificateField(Certificate parent) {
			this.type = FIELD_TYPE;
			this.parent = parent;
		}

		public Object clone() {
			return new ParentCertificateField(parent);
		}

		public void decode(DataInputStream in) throws IOException {
			String type = in.readUTF();
			int encsize = in.readInt();

			if (encsize <= 0)
				throw new IOException("Invalid key encoding size");

			byte[] enc = new byte[encsize];

			in.readFully(enc);

			ByteArrayInputStream bin = new ByteArrayInputStream(enc);

			try {
				CertificateFactory cf = CertificateFactory.getInstance(type);

				parent = cf.generateCertificate(bin);
			} catch (CertificateException e) {
				throw new IOException("Invalid certificate encoding or type");
			}
		}

		public void encode(DataOutputStream out) throws IOException {
			out.writeUTF(parent.getType());

			try {
				byte[] enc = parent.getEncoded();

				out.writeInt(enc.length);
				out.write(enc);
			} catch (CertificateEncodingException e) {
				throw new IOException("Couldn't encoding certificate");
			}
		}
	}

	protected static Field instantiateField(int type) {
		switch (type) {
		case PublicKeyField.FIELD_TYPE:
			return new PublicKeyField();
		case ParentCertificateField.FIELD_TYPE:
			return new ParentCertificateField();
		default:
			return new UnknownField();
		}
	}

	private byte[] _enc;

	protected String issuer = null;
	protected String subject = null;
	protected Date created = null;
	protected Date expires = null;
	protected ArrayList fields = new ArrayList();
	protected String signatureAlgorithm = null;
	protected byte[] signature = null;

	protected void encodeTBS(DataOutputStream out) throws IOException {
		out.writeUTF(issuer);
		out.writeUTF(subject);
		out.writeLong(created.getTime());
		out.writeLong(expires.getTime());
		out.writeInt(fields.size());

		for (int i = 0; i < fields.size(); i++) {
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			DataOutputStream dos = new DataOutputStream(bos);

			Object obj = fields.get(i);
			if (obj instanceof Field) {
				Field f = (Field) obj;

				f.encode(dos);
				dos.close();

				byte[] fenc = bos.toByteArray();

				out.writeInt(f.type);
				out.writeInt(fenc.length);
				out.write(fenc);
			}
		}
	}

	protected byte[] encodeTBS() throws CertificateEncodingException {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		DataOutputStream dos = new DataOutputStream(bos);

		try {
			encodeTBS(dos);
			dos.close();
		} catch (IOException e) {
			throw new CertificateEncodingException(e.getMessage());
		}

		return bos.toByteArray();
	}

	protected BeeCertificate() {
		super("BEE");
	}

	public BeeCertificate(BeeCertificate copy) {
		super("BEE");

		issuer = copy.issuer;
		subject = copy.subject;
		created = new Date(copy.created.getTime());
		expires = new Date(copy.expires.getTime());
		fields.addAll(copy.fields);
		signatureAlgorithm = copy.signatureAlgorithm;
		if (copy.signature != null)
			signature = (byte[]) copy.signature.clone();
	}

	public BeeCertificate(InputStream in) throws IOException {
		super("BEE");

		DataInputStream din = new DataInputStream(in);

		issuer = din.readUTF();
		subject = din.readUTF();

		created = new Date(din.readLong());
		expires = new Date(din.readLong());

		int fieldCount = din.readInt();
		if (fieldCount < 0)
			throw new IOException("field count < 0");

		fields.ensureCapacity(fieldCount);

		for (int i = 0; i < fieldCount; i++) {
			int type = din.readInt();

			int fieldSize = din.readInt();
			if (fieldSize < 0)
				throw new IOException("field size < 0");

			byte[] fenc = new byte[fieldSize];

			din.readFully(fenc);

			ByteArrayInputStream bis = new ByteArrayInputStream(fenc);
			DataInputStream fis = new DataInputStream(bis);

			Field f = instantiateField(type);

			f.decode(fis);

			fields.add(f);
		}

		signatureAlgorithm = din.readUTF();

		int sigSize = din.readInt();
		if (sigSize < 0)
			throw new IOException("signature size < 0");

		signature = new byte[sigSize];

		din.readFully(signature);
	}

	public byte[] getEncoded() throws CertificateEncodingException {
		if (_enc == null) {
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			DataOutputStream dos = new DataOutputStream(bos);

			try {
				encodeTBS(dos);

				dos.writeUTF(signatureAlgorithm);
				dos.writeInt(signature.length);
				dos.write(signature);
				dos.close();

				_enc = bos.toByteArray();
			} catch (IOException e) {
				throw new CertificateEncodingException(e.getMessage());
			}
		}
		return _enc;
	}

	public PublicKey getPublicKey() {
		for (int i = 0; i < fields.size(); i++) {
			Object obj = fields.get(i);
			if (obj instanceof PublicKeyField)
				return ((PublicKeyField) obj).pub;
		}
		return null;
	}

	public Certificate getParentCertificate() {
		for (int i = 0; i < fields.size(); i++) {
			Object obj = fields.get(i);
			if (obj instanceof ParentCertificateField)
				return ((ParentCertificateField) obj).parent;
		}
		return null;
	}

	public void verify(PublicKey pub) throws CertificateException,
			NoSuchAlgorithmException, InvalidKeyException, SignatureException {
		Signature sig = Signature.getInstance(signatureAlgorithm);

		sig.initVerify(pub);
		sig.update(encodeTBS());

		if (!sig.verify(signature))
			throw new CertificateException("signature doesn't match");
	}

	public void verify(PublicKey pub, String algorithm)
			throws CertificateException, NoSuchAlgorithmException,
			InvalidKeyException, SignatureException {
		Signature sig = Signature.getInstance(algorithm);

		sig.initVerify(pub);
		sig.update(encodeTBS());

		if (!sig.verify(signature))
			throw new CertificateException("signature doesn't match");
	}

	public String toString() {
		StringBuffer tmp = new StringBuffer();

		tmp.append("BEE ").append(subject);

		if (issuer.length() > 0)
			tmp.append(" issued by ").append(issuer);

		tmp.append(" valid from ").append(created.toString()).append(" until ")
				.append(expires.equals(FOREVER) ? "-" : expires.toString());

		/** todo add fields */

		return tmp.toString();
	}

	public void checkValidity() throws CertificateExpiredException,
			CertificateNotYetValidException {
		checkValidity(new Date());
	}

	public void checkValidity(Date at) throws CertificateExpiredException,
			CertificateNotYetValidException {
		if (at.before(created))
			throw new CertificateNotYetValidException();

		if (!expires.equals(FOREVER))
			if (at.after(expires))
				throw new CertificateExpiredException();

		if (hasParentCertificate()) {
			// parent certificate had to be valid when this one was created
			Certificate parent = getParentCertificate();
			if (parent instanceof BeeCertificate)
				((BeeCertificate) parent).checkValidity(created);
		}
	}

	public Date getNotAfter() {
		return expires;
	}

	public Date getNotBefore() {
		return created;
	}

	public byte[] getSignature() {
		return signature;
	}

	public String getSigAlgName() {
		return signatureAlgorithm;
	}

	public boolean hasPublicKey() {
		for (int i = 0; i < fields.size(); i++) {
			Object obj = fields.get(i);
			if (obj instanceof PublicKeyField)
				return true;
		}
		return false;
	}

	public boolean hasParentCertificate() {
		for (int i = 0; i < fields.size(); i++) {
			Object obj = fields.get(i);
			if (obj instanceof ParentCertificateField)
				return true;
		}
		return false;
	}

	public boolean isSelfSignedCertificate() {
		if (hasParentCertificate())
			return false;

		if (!hasPublicKey())
			return false;

		try {
			verify(getPublicKey());
			return true;
		} catch (Exception e) {
			return false;
		}
	}

	public static BeeCertificate self(PublicKey publicKey, PrivateKey signKey,
			String signatureAlgorithm) throws InvalidKeyException,
			SignatureException, CertificateEncodingException,
			NoSuchAlgorithmException {
		if (publicKey.getEncoded() == null)
			throw new InvalidKeyException("PublicKey doesn't have an encoding");

		Signature sig = Signature.getInstance(signatureAlgorithm);

		sig.initSign(signKey);

		BeeCertificate cert = new BeeCertificate();

		cert.subject = "Public Key Certificate";
		cert.expires = FOREVER;
		cert.signatureAlgorithm = signatureAlgorithm;
		cert.fields.add(new PublicKeyField(publicKey));

		byte[] tmp = cert.encodeTBS();

		sig.update(tmp);

		cert.signature = sig.sign();

		return cert;
	}

	public BeeCertificate make(PublicKey publicKey, PrivateKey signKey,
			String signatureAlgorithm, Certificate parent)
			throws InvalidKeyException, CertificateEncodingException,
			SignatureException, NoSuchAlgorithmException {
		if (publicKey.getEncoded() == null)
			throw new InvalidKeyException("PublicKey doesn't have an encoding");

		Signature sig = Signature.getInstance(signatureAlgorithm);

		sig.initSign(signKey);

		BeeCertificate cert = new BeeCertificate();

		cert.subject = "Public Key Certificate";
		cert.expires = FOREVER;
		cert.signatureAlgorithm = signatureAlgorithm;
		cert.fields.add(new PublicKeyField(publicKey));
		cert.fields.add(new ParentCertificateField(parent));

		byte[] tmp = cert.encodeTBS();

		sig.update(tmp);

		cert.signature = sig.sign();

		return cert;
	}
}
