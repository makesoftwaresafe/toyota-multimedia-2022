package beecrypt.security;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.io.*;

public class DSAPrivateKeyImpl implements DSAPrivateKey, Cloneable {
	private DSAParameterSpec _params;
	private BigInteger _x;
	private byte[] _enc;

	public DSAPrivateKeyImpl(BigInteger p, BigInteger q, BigInteger g,
			BigInteger x) {
		_params = new DSAParameterSpec(p, q, g);
		_x = x;
	}

	public DSAPrivateKeyImpl(DSAParameterSpec params, BigInteger x) {
		_params = params;
		_x = x;
	}

	public DSAPrivateKeyImpl(DSAPrivateKeySpec spec) {
		_params = new DSAParameterSpec(spec.getP(), spec.getQ(), spec.getG());
		_x = spec.getX();
	}

	public DSAPrivateKeyImpl(DSAPrivateKey key) {
		_params = new DSAParameterSpec(key.getParams().getP(), key.getParams()
				.getQ(), key.getParams().getG());
		_x = key.getX();
	}

	public Object clone() {
		return new DSAPrivateKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof DSAPrivateKey) {
			DSAPrivateKey pri = (DSAPrivateKey) obj;

			if (!pri.getParams().getP().equals(_params.getP()))
				return false;
			if (!pri.getParams().getQ().equals(_params.getQ()))
				return false;
			if (!pri.getParams().getG().equals(_params.getG()))
				return false;
			if (!pri.getX().equals(_x))
				return false;

			return true;
		}
		return false;
	}

	public DSAParams getParams() {
		return _params;
	}

	public BigInteger getX() {
		return _x;
	}

	public byte[] getEncoded() {
		if (_enc == null) {
			try {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				BeeOutputStream bee = new BeeOutputStream(bos);

				bee.writeBigInteger(_params.getP());
				bee.writeBigInteger(_params.getQ());
				bee.writeBigInteger(_params.getG());
				bee.writeBigInteger(_x);
				bee.close();

				_enc = bos.toByteArray();
			} catch (IOException e) {
				// shouldn't occur
			}
		}

		return _enc;
	}

	public String getAlgorithm() {
		return "DSA";
	}

	public String getFormat() {
		return "BEE";
	}
}
