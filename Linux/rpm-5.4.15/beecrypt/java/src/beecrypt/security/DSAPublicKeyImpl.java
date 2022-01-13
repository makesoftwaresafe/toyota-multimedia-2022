package beecrypt.security;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.io.*;

public class DSAPublicKeyImpl implements DSAPublicKey, Cloneable {
	private DSAParameterSpec _params;
	private BigInteger _y;
	private byte[] _enc;

	public DSAPublicKeyImpl(BigInteger p, BigInteger q, BigInteger g,
			BigInteger y) {
		_params = new DSAParameterSpec(p, q, g);
		_y = y;
	}

	public DSAPublicKeyImpl(DSAParameterSpec params, BigInteger y) {
		_params = params;
		_y = y;
	}

	public DSAPublicKeyImpl(DSAPublicKeySpec spec) {
		_params = new DSAParameterSpec(spec.getP(), spec.getQ(), spec.getG());
		_y = spec.getY();
	}

	public DSAPublicKeyImpl(DSAPublicKey key) {
		_params = new DSAParameterSpec(key.getParams().getP(), key.getParams()
				.getQ(), key.getParams().getG());
		_y = key.getY();
	}

	public Object clone() {
		return new DSAPublicKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof DSAPublicKey) {
			DSAPublicKey pri = (DSAPublicKey) obj;

			if (!pri.getParams().getP().equals(_params.getP()))
				return false;
			if (!pri.getParams().getQ().equals(_params.getQ()))
				return false;
			if (!pri.getParams().getG().equals(_params.getG()))
				return false;
			if (!pri.getY().equals(_y))
				return false;

			return true;
		}
		return false;
	}

	public DSAParams getParams() {
		return _params;
	}

	public BigInteger getY() {
		return _y;
	}

	public byte[] getEncoded() {
		if (_enc == null) {
			try {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				BeeOutputStream bee = new BeeOutputStream(bos);

				bee.writeBigInteger(_params.getP());
				bee.writeBigInteger(_params.getQ());
				bee.writeBigInteger(_params.getG());
				bee.writeBigInteger(_y);
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
