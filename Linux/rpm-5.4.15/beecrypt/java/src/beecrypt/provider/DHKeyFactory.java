package beecrypt.provider;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

// import beecrypt.asn1.*;
// import beecrypt.asn1.x509.SubjectPublicKeyInfo;
import beecrypt.beeyond.*;
import beecrypt.crypto.*;
import beecrypt.io.*;

public final class DHKeyFactory extends KeyFactorySpi {
	private static DHPrivateKey generatePrivate(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger p, g, x;

			p = bee.readBigInteger();
			g = bee.readBigInteger();
			x = bee.readBigInteger();

			return new DHPrivateKeyImpl(p, g, x);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	private static DHPublicKey generatePublic(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger p, g, y;

			p = bee.readBigInteger();
			g = bee.readBigInteger();
			y = bee.readBigInteger();

			return new DHPublicKeyImpl(p, g, y);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	/*
	 * private static DHPublicKey generatePublicX509(byte[] enc) throws
	 * InvalidKeySpecException { try { DERDecoder d = new DERDecoder();
	 * 
	 * Any tmp = d.decode(enc, SubjectPublicKeyInfo.NAME);
	 * 
	 * if (tmp instanceof SubjectPublicKeyInfo) { SubjectPublicKeyInfo spki =
	 * (SubjectPublicKeyInfo) tmp;
	 * 
	 * AlgorithmIdentifier aid = spki.getAlgorithmIdentifier();
	 * 
	 * ObjectIdentifier oid = aid.getAlgorithm(); } } catch (IOException e) {
	 * throw new InvalidKeySpecException("invalid X.509 SubjectPublicKeyInfo");
	 * } catch (NoSuchTypeException e) { throw new
	 * RuntimeException("ASN.1 X.509 module required"); } }
	 */

	protected PrivateKey engineGeneratePrivate(KeySpec spec)
			throws InvalidKeySpecException {
		if (spec instanceof DHPrivateKeySpec) {
			return new DHPrivateKeyImpl((DHPrivateKeySpec) spec);
		}

		if (spec instanceof EncodedKeySpec) {
			EncodedKeySpec enc = (EncodedKeySpec) spec;

			if (enc.getFormat().equals("BEE")) {
				return generatePrivate(enc.getEncoded());
			}
			throw new InvalidKeySpecException("Unsupported KeySpec format");
		}

		throw new InvalidKeySpecException("Unsupported KeySpec type");
	}

	protected PublicKey engineGeneratePublic(KeySpec spec)
			throws InvalidKeySpecException {
		if (spec instanceof DHPublicKeySpec) {
			return new DHPublicKeyImpl((DHPublicKeySpec) spec);
		}

		/*
		 * if (spec instanceof X509EncodedKeySpec) { X509EncodedKeySpec xspec =
		 * (X509EncodedKeySpec) spec;
		 * 
		 * return generatePublicX509(xspec.getEncoded()); }
		 */

		if (spec instanceof EncodedKeySpec) {
			EncodedKeySpec enc = (EncodedKeySpec) spec;

			if (enc.getFormat().equals("BEE")) {
				return generatePublic(enc.getEncoded());
			}
			throw new InvalidKeySpecException("Unsupported KeySpec format");
		}

		throw new InvalidKeySpecException("Unsupported KeySpec type");
	}

	protected KeySpec engineGetKeySpec(Key key, Class keySpec)
			throws InvalidKeySpecException {
		if (keySpec.isAssignableFrom(DHPrivateKeySpec.class)
				&& (key instanceof DHPrivateKey)) {
			DHPrivateKey pri = (DHPrivateKey) key;

			return new DHPrivateKeySpec(pri.getX(), pri.getParams().getP(), pri
					.getParams().getG());
		}

		if (keySpec.isAssignableFrom(DHPublicKeySpec.class)
				&& (key instanceof DHPublicKey)) {
			DHPublicKey pub = (DHPublicKey) key;

			return new DHPublicKeySpec(pub.getY(), pub.getParams().getP(), pub
					.getParams().getG());
		}

		if (keySpec.isAssignableFrom(PKCS8EncodedKeySpec.class)
				&& key.getFormat().equalsIgnoreCase("PKCS#8")) {
			return new PKCS8EncodedKeySpec(key.getEncoded());
		}

		if (keySpec.isAssignableFrom(X509EncodedKeySpec.class)
				&& key.getFormat().equalsIgnoreCase("X.509")) {
			return new PKCS8EncodedKeySpec(key.getEncoded());
		}

		if (keySpec.equals(EncodedKeySpec.class)) {
			String format = key.getFormat();
			if (format != null) {
				byte[] enc = key.getEncoded();
				if (enc != null)
					return new AnyEncodedKeySpec(format, enc);
			}
		}

		throw new InvalidKeySpecException(
				"Unsupported KeySpec class for this key");
	}

	protected Key engineTranslateKey(Key key) throws InvalidKeyException {
		if (key instanceof DHPublicKey) {
			return new DHPublicKeyImpl((DHPublicKey) key);
		} else if (key instanceof DHPrivateKey) {
			return new DHPrivateKeyImpl((DHPrivateKey) key);
		}
		throw new InvalidKeyException("Unsupported Key type");
	}

	public DHKeyFactory() {
	}
}
