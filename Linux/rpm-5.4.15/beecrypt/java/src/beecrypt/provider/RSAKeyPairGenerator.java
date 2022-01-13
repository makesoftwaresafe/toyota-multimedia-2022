package beecrypt.provider;

import java.math.BigInteger;
import java.security.*;
import java.security.spec.*;

import beecrypt.security.*;

public final class RSAKeyPairGenerator extends KeyPairGeneratorSpi {
	private int _size = 1024;
	private byte[] _n = null;
	private byte[] _e = RSAKeyGenParameterSpec.F4.toByteArray();
	private byte[] _d = null;
	private byte[] _p = null;
	private byte[] _q = null;
	private byte[] _dp = null;
	private byte[] _dq = null;
	private byte[] _qi = null;

	private SecureRandom _srng;

	private native void generate();

	public KeyPair generateKeyPair() {
		generate();

		BigInteger n = new BigInteger(_n);
		BigInteger e = new BigInteger(_e);

		return new KeyPair(new RSAPublicKeyImpl(n, e),
				new RSAPrivateCrtKeyImpl(n, e, new BigInteger(_d),
						new BigInteger(_p), new BigInteger(_q), new BigInteger(
								_dp), new BigInteger(_dq), new BigInteger(_qi)));
	}

	public void initialize(int keysize, SecureRandom random)
			throws InvalidParameterException {
		if (keysize < 768)
			throw new InvalidParameterException("minimum size is 768");

		_size = keysize;
		_e = RSAKeyGenParameterSpec.F4.toByteArray();
		_srng = random;
	}

	public void initialize(AlgorithmParameterSpec spec, SecureRandom random)
			throws InvalidAlgorithmParameterException {
		if (spec instanceof RSAKeyGenParameterSpec) {
			RSAKeyGenParameterSpec rs = (RSAKeyGenParameterSpec) spec;

			_size = rs.getKeysize();
			_e = rs.getPublicExponent().toByteArray();
			_srng = random;
		} else
			throw new InvalidAlgorithmParameterException(
					"not an RSAKeyGenParameterSpec");
	}
}
