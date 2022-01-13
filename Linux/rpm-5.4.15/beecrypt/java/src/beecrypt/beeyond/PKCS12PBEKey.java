package beecrypt.beeyond;

import javax.crypto.interfaces.*;

public class PKCS12PBEKey implements PBEKey {
	private char[] _pwd;
	private byte[] _salt;
	private int _iter;
	private byte[] _enc;

	public static byte[] encode(char[] password) {
		int i;
		byte[] result = new byte[(password.length + 1) * 2];
		for (i = 0; i < password.length; i++) {
			char c = password[i];
			result[2 * i] = (byte) (c >> 8);
			result[2 * i + 1] = (byte) c;
		}
		result[2 * i] = 0;
		result[2 * i + 1] = 0;
		return result;
	}

	public PKCS12PBEKey(char[] password, byte[] salt, int iterationCount) {
		_pwd = password;
		_salt = salt;
		_iter = iterationCount;
	}

	public String getAlgorithm() {
		return "PKCS#12/PBE";
	}

	public byte[] getEncoded() {
		if (_enc == null)
			_enc = encode(_pwd);

		return _enc;
	}

	public String getFormat() {
		return "RAW";
	}

	public int getIterationCount() {
		return _iter;
	}

	public char[] getPassword() {
		return _pwd;
	}

	public byte[] getSalt() {
		return _salt;
	}

}
