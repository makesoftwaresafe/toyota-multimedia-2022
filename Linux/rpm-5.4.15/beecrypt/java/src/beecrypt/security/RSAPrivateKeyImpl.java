package beecrypt.security;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.io.*;

public class RSAPrivateKeyImpl implements RSAPrivateKey, Cloneable {
	private BigInteger _n, _d;
	private byte[] _enc;

	public RSAPrivateKeyImpl(BigInteger modulus, BigInteger privateExponent) {
		_n = modulus;
		_d = privateExponent;
	}

	public RSAPrivateKeyImpl(RSAPrivateKeySpec spec) {
		_n = spec.getModulus();
		_d = spec.getPrivateExponent();
	}

	public RSAPrivateKeyImpl(RSAPrivateKey key) {
		_n = key.getModulus();
		_d = key.getPrivateExponent();
	}

	public Object clone() {
		return new RSAPrivateKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof RSAPrivateKey) {
			RSAPrivateKey pri = (RSAPrivateKey) obj;

			if (!pri.getModulus().equals(_n))
				return false;
			if (!pri.getPrivateExponent().equals(_d))
				return false;

			return true;
		}
		return false;
	}

	public BigInteger getModulus() {
		return _n;
	}

	public BigInteger getPrivateExponent() {
		return _d;
	}

	public byte[] getEncoded() {
		if (_enc == null) {
			try {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				BeeOutputStream bee = new BeeOutputStream(bos);

				bee.writeBigInteger(_n);
				bee.writeBigInteger(_d);
				bee.close();

				_enc = bos.toByteArray();
			} catch (IOException e) {
				// shouldn't occur
			}
		}

		return _enc;
	}

	public String getAlgorithm() {
		return "RSA";
	}

	public String getFormat() {
		return "BEE";
	}
}
