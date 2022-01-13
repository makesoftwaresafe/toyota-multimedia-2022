package beecrypt.provider;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

import beecrypt.beeyond.*;
import beecrypt.io.*;
import beecrypt.security.*;

public final class RSAKeyFactory extends KeyFactorySpi {
	private static RSAPrivateKey generatePrivate(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger n, d;

			n = bee.readBigInteger();
			d = bee.readBigInteger();

			if (bee.available() > 0) {
				BigInteger e, p, q, dp, dq, qi;

				e = bee.readBigInteger();
				p = bee.readBigInteger();
				q = bee.readBigInteger();
				dp = bee.readBigInteger();
				dq = bee.readBigInteger();
				qi = bee.readBigInteger();

				return new RSAPrivateCrtKeyImpl(n, e, d, p, q, dp, dq, qi);
			}

			return new RSAPrivateKeyImpl(n, d);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	private static RSAPublicKey generatePublic(byte[] enc)
			throws InvalidKeySpecException {
		try {
			ByteArrayInputStream bis = new ByteArrayInputStream(enc);
			BeeInputStream bee = new BeeInputStream(bis);

			BigInteger n, e;

			n = bee.readBigInteger();
			e = bee.readBigInteger();

			return new RSAPublicKeyImpl(n, e);
		} catch (IOException e) {
			throw new InvalidKeySpecException("Invalid KeySpec encoding");
		}
	}

	protected PrivateKey engineGeneratePrivate(KeySpec spec)
			throws InvalidKeySpecException {
		if (spec instanceof RSAPrivateKeySpec) {
			if (spec instanceof RSAPrivateCrtKeySpec)
				return new RSAPrivateCrtKeyImpl((RSAPrivateCrtKeySpec) spec);

			return new RSAPrivateKeyImpl((RSAPrivateKeySpec) spec);
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
		if (spec instanceof RSAPublicKeySpec) {
			return new RSAPublicKeyImpl((RSAPublicKeySpec) spec);
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
		if (keySpec.isAssignableFrom(RSAPrivateCrtKeySpec.class)
				&& (key instanceof RSAPrivateCrtKey)) {
			RSAPrivateCrtKey pri = (RSAPrivateCrtKey) key;

			return new RSAPrivateCrtKeySpec(pri.getModulus(), pri
					.getPublicExponent(), pri.getPrivateExponent(), pri
					.getPrimeP(), pri.getPrimeQ(), pri.getPrimeExponentP(), pri
					.getPrimeExponentQ(), pri.getCrtCoefficient());
		}

		if (keySpec.isAssignableFrom(RSAPrivateKeySpec.class)
				&& (key instanceof RSAPrivateKey)) {
			RSAPrivateKey pri = (RSAPrivateKey) key;

			return new RSAPrivateKeySpec(pri.getModulus(), pri
					.getPrivateExponent());
		}

		if (keySpec.isAssignableFrom(RSAPublicKeySpec.class)
				&& (key instanceof RSAPublicKey)) {
			RSAPublicKey pub = (RSAPublicKey) key;

			return new RSAPublicKeySpec(pub.getModulus(), pub
					.getPublicExponent());
		}

		if (keySpec.isAssignableFrom(PKCS8EncodedKeySpec.class)
				&& key.getFormat().equalsIgnoreCase("PKCS#8")) {
			return new PKCS8EncodedKeySpec(key.getEncoded());
		}

		if (keySpec.isAssignableFrom(X509EncodedKeySpec.class)
				&& key.getFormat().equalsIgnoreCase("X.509")) {
			return new X509EncodedKeySpec(key.getEncoded());
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
		if (key instanceof RSAPublicKey) {
			return new RSAPublicKeyImpl((RSAPublicKey) key);
		} else if (key instanceof RSAPrivateCrtKey) {
			return new RSAPrivateCrtKeyImpl((RSAPrivateCrtKey) key);
		} else if (key instanceof RSAPrivateKey) {
			return new RSAPrivateKeyImpl((RSAPrivateKey) key);
		}
		throw new InvalidKeyException("Unsupported Key type");
	}

	public RSAKeyFactory() {
	}
}
