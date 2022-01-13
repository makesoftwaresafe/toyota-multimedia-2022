package beecrypt.security;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.io.*;

public class RSAPublicKeyImpl implements RSAPublicKey, Cloneable {
	private BigInteger _n, _e;
	private byte[] _enc;

	public RSAPublicKeyImpl(BigInteger modulus, BigInteger publicExponent) {
		_n = modulus;
		_e = publicExponent;
	}

	public RSAPublicKeyImpl(RSAPublicKeySpec spec) {
		_n = spec.getModulus();
		_e = spec.getPublicExponent();
	}

	public RSAPublicKeyImpl(RSAPublicKey key) {
		_n = key.getModulus();
		_e = key.getPublicExponent();
	}

	public Object clone() {
		return new RSAPublicKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof RSAPublicKey) {
			RSAPublicKey pri = (RSAPublicKey) obj;

			if (!pri.getModulus().equals(_n))
				return false;
			if (!pri.getPublicExponent().equals(_e))
				return false;

			return true;
		}
		return false;
	}

	public BigInteger getModulus() {
		return _n;
	}

	public BigInteger getPublicExponent() {
		return _e;
	}

	public byte[] getEncoded() {
		if (_enc == null) {
			try {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				BeeOutputStream bee = new BeeOutputStream(bos);

				bee.writeBigInteger(_n);
				bee.writeBigInteger(_e);
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
