package beecrypt.crypto;

import java.io.*;
import java.math.BigInteger;
import javax.crypto.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

import beecrypt.io.*;

public class DHPrivateKeyImpl implements DHPrivateKey, Cloneable {
	private DHParameterSpec _params;
	private BigInteger _x;
	private byte[] _enc;

	public DHPrivateKeyImpl(BigInteger p, BigInteger g, BigInteger x) {
		_params = new DHParameterSpec(p, g);
		_x = x;
	}

	public DHPrivateKeyImpl(DHParameterSpec params, BigInteger x) {
		_params = params;
		_x = x;
	}

	public DHPrivateKeyImpl(DHPrivateKeySpec spec) {
		_params = new DHParameterSpec(spec.getP(), spec.getG());
		_x = spec.getX();
	}

	public DHPrivateKeyImpl(DHPrivateKey key) {
		_params = key.getParams();
		_x = key.getX();
	}

	public Object clone() {
		return new DHPrivateKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof DHPrivateKey) {
			DHPrivateKey pri = (DHPrivateKey) obj;

			if (!pri.getParams().getP().equals(_params.getP()))
				return false;
			if (!pri.getParams().getG().equals(_params.getG()))
				return false;
			if (!pri.getX().equals(_x))
				return false;

			return true;
		}
		return false;
	}

	public DHParameterSpec getParams() {
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
		return "DH";
	}

	public String getFormat() {
		return "BEE";
	}
}
