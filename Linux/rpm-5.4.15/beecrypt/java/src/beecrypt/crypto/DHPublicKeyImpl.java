package beecrypt.crypto;

import java.io.*;
import java.math.BigInteger;
import javax.crypto.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

import beecrypt.io.*;

public class DHPublicKeyImpl implements DHPublicKey, Cloneable {
	private DHParameterSpec _params;
	private BigInteger _y;
	private byte[] _enc;

	public DHPublicKeyImpl(BigInteger p, BigInteger g, BigInteger y) {
		_params = new DHParameterSpec(p, g);
		_y = y;
	}

	public DHPublicKeyImpl(DHParameterSpec params, BigInteger y) {
		_params = params;
		_y = y;
	}

	public DHPublicKeyImpl(DHPublicKeySpec spec) {
		_params = new DHParameterSpec(spec.getP(), spec.getG());
		_y = spec.getY();
	}

	public DHPublicKeyImpl(DHPublicKey key) {
		_params = key.getParams();
		_y = key.getY();
	}

	public Object clone() {
		return new DHPublicKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof DHPublicKey) {
			DHPublicKey pri = (DHPublicKey) obj;

			if (!pri.getParams().getP().equals(_params.getP()))
				return false;
			if (!pri.getParams().getG().equals(_params.getG()))
				return false;
			if (!pri.getY().equals(_y))
				return false;

			return true;
		}
		return false;
	}

	public DHParameterSpec getParams() {
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
		return "DH";
	}

	public String getFormat() {
		return "BEE";
	}
}
