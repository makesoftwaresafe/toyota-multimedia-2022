package beecrypt.security;

import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;

import beecrypt.io.*;

public class RSAPrivateCrtKeyImpl implements RSAPrivateCrtKey, Cloneable {
	private BigInteger _n, _e, _d, _p, _q, _dp, _dq, _qi;
	private byte[] _enc;

	public RSAPrivateCrtKeyImpl(BigInteger modulus, BigInteger publicExponent,
			BigInteger privateExponent, BigInteger primeP, BigInteger primeQ,
			BigInteger primeExponentP, BigInteger primeExponentQ,
			BigInteger crtCoefficient) {
		_n = modulus;
		_e = publicExponent;
		_d = privateExponent;
		_p = primeP;
		_q = primeQ;
		_dp = primeExponentP;
		_dq = primeExponentQ;
		_qi = crtCoefficient;
	}

	public RSAPrivateCrtKeyImpl(RSAPrivateCrtKeySpec spec) {
		_n = spec.getModulus();
		_e = spec.getPublicExponent();
		_d = spec.getPrivateExponent();
		_p = spec.getPrimeP();
		_q = spec.getPrimeQ();
		_dp = spec.getPrimeExponentP();
		_dq = spec.getPrimeExponentQ();
		_qi = spec.getCrtCoefficient();
	}

	public RSAPrivateCrtKeyImpl(RSAPrivateCrtKey key) {
		_n = key.getModulus();
		_e = key.getPublicExponent();
		_d = key.getPrivateExponent();
		_p = key.getPrimeP();
		_q = key.getPrimeQ();
		_dp = key.getPrimeExponentP();
		_dq = key.getPrimeExponentQ();
		_qi = key.getCrtCoefficient();
	}

	public Object clone() {
		return new RSAPrivateCrtKeyImpl(this);
	}

	public boolean equals(Object obj) {
		if (this == obj)
			return true;

		if (obj instanceof RSAPrivateCrtKey) {
			RSAPrivateCrtKey pri = (RSAPrivateCrtKey) obj;

			if (!pri.getModulus().equals(_n))
				return false;
			if (!pri.getPublicExponent().equals(_e))
				return false;
			if (!pri.getPrivateExponent().equals(_d))
				return false;
			if (!pri.getPrimeP().equals(_p))
				return false;
			if (!pri.getPrimeQ().equals(_q))
				return false;
			if (!pri.getPrimeExponentP().equals(_dp))
				return false;
			if (!pri.getPrimeExponentQ().equals(_dq))
				return false;
			if (!pri.getCrtCoefficient().equals(_qi))
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

	public BigInteger getPrivateExponent() {
		return _d;
	}

	public BigInteger getPrimeP() {
		return _p;
	}

	public BigInteger getPrimeQ() {
		return _q;
	}

	public BigInteger getPrimeExponentP() {
		return _dp;
	}

	public BigInteger getPrimeExponentQ() {
		return _dq;
	}

	public BigInteger getCrtCoefficient() {
		return _qi;
	}

	public byte[] getEncoded() {
		if (_enc == null) {
			try {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				BeeOutputStream bee = new BeeOutputStream(bos);

				bee.writeBigInteger(_n);
				bee.writeBigInteger(_d);
				bee.writeBigInteger(_e);
				bee.writeBigInteger(_p);
				bee.writeBigInteger(_q);
				bee.writeBigInteger(_dp);
				bee.writeBigInteger(_dq);
				bee.writeBigInteger(_qi);
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
