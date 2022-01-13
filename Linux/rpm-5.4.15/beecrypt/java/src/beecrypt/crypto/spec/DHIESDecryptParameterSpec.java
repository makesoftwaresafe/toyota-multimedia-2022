package beecrypt.crypto.spec;

import java.math.BigInteger;
import java.security.spec.*;

public class DHIESDecryptParameterSpec extends DHIESParameterSpec {
	private BigInteger pub;
	private byte[] mac;

	public DHIESDecryptParameterSpec(DHIESParameterSpec copy, BigInteger pub,
			byte[] mac) {
		super(copy);

		this.pub = pub;
		this.mac = (byte[]) mac.clone();
	}

	public DHIESDecryptParameterSpec(DHIESDecryptParameterSpec copy) {
		super(copy);

		this.pub = copy.pub;
		this.mac = (byte[]) copy.mac.clone();
	}

	public BigInteger getEphemeralPublicKey() {
		return pub;
	}

	public byte[] getMac() {
		return mac;
	}
}
