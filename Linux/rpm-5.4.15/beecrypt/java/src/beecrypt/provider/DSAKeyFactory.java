package beecrypt.provider;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.beeyond.*;
import beecrypt.io.*;
import beecrypt.security.*;

public final class DSAKeyFactory extends KeyFactorySpi {
	private static DSAPrivateKey generatePrivate(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger p, q, g, x;

			p = bee.readBigInteger();
			q = bee.readBigInteger();
			g = bee.readBigInteger();
			x = bee.readBigInteger();

			return new DSAPrivateKeyImpl(p, q, g, x);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	private static DSAPublicKey generatePublic(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger p, q, g, y;

			p = bee.readBigInteger();
			q = bee.readBigInteger();
			g = bee.readBigInteger();
			y = bee.readBigInteger();

			return new DSAPublicKeyImpl(p, q, g, y);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	protected PrivateKey engineGeneratePrivate(KeySpec spec)
			throws InvalidKeySpecException {
		if (spec instanceof DSAPrivateKeySpec) {
			return new DSAPrivateKeyImpl((DSAPrivateKeySpec) spec);
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
		if (spec instanceof DSAPublicKeySpec) {
			return new DSAPublicKeyImpl((DSAPublicKeySpec) spec);
		}

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
		if (key instanceof DSAPublicKey) {
			DSAPublicKey pub = (DSAPublicKey) key;

			if (keySpec.equals(KeySpec.class)
					|| keySpec.equals(DSAPublicKeySpec.class)) {
				return new DSAPublicKeySpec(pub.getY(), pub.getParams().getP(),
						pub.getParams().getQ(), pub.getParams().getG());
			}
			if (keySpec.equals(EncodedKeySpec.class)) {
				String format = pub.getFormat();
				if (format != null) {
					byte[] enc = pub.getEncoded();
					if (enc != null)
						return new AnyEncodedKeySpec(format, enc);
				}
			}
		} else if (key instanceof DSAPrivateKey) {
			DSAPrivateKey pri = (DSAPrivateKey) key;

			if (keySpec.equals(KeySpec.class)
					|| keySpec.equals(DSAPrivateKeySpec.class)) {
				return new DSAPrivateKeySpec(pri.getX(),
						pri.getParams().getP(), pri.getParams().getQ(), pri
								.getParams().getG());
			}
			if (keySpec.equals(EncodedKeySpec.class)) {
				String format = pri.getFormat();
				if (format != null) {
					byte[] enc = pri.getEncoded();
					if (enc != null)
						return new AnyEncodedKeySpec(format, enc);
				}
			}
		}
		throw new InvalidKeySpecException("Unsupported Key type");
	}

	protected Key engineTranslateKey(Key key) throws InvalidKeyException {
		if (key instanceof DSAPublicKey) {
			return new DSAPublicKeyImpl((DSAPublicKey) key);
		} else if (key instanceof DSAPrivateKey) {
			return new DSAPrivateKeyImpl((DSAPrivateKey) key);
		}
		throw new InvalidKeyException("Unsupported Key type");
	}

	public DSAKeyFactory() {
	}
}
