package beecrypt.crypto.spec;

import java.security.spec.*;
import java.util.regex.*;

public class DHIESParameterSpec implements AlgorithmParameterSpec {
	private static final Pattern DHIESPAT = Pattern
			.compile("DHIES\\((\\w(?:\\w|\\d)*(?:-(?:\\w|\\d)*)*),(\\w(?:\\w|\\d)*(?:-(?:\\w|\\d)*)*),(\\w(?:\\w|\\d)*(?:-(?:\\w|\\d)*)*)(?:,(\\d+))?(?:,(\\d+))?\\)");

	private String messageDigestAlgorithm;
	private String cipherAlgorithm;
	private String macAlgorithm;

	private int cipherKeyLength;
	private int macKeyLength;

	public DHIESParameterSpec(String description)
			throws IllegalArgumentException {
		Matcher m = DHIESPAT.matcher(description);
		if (!m.matches())
			throw new IllegalArgumentException(
					"couldn't parse descriptor into DHIES(<digest>,<cipher>,<mac>[,<cipherkeylen>[,<mackeylen>]])");

		messageDigestAlgorithm = m.group(1);
		cipherAlgorithm = m.group(2);
		macAlgorithm = m.group(3);

		String tmp = m.group(4);
		if (tmp != null && tmp.length() > 0)
			cipherKeyLength = Integer.parseInt(tmp);
		else
			cipherKeyLength = 0;

		tmp = m.group(5);
		if (tmp != null && tmp.length() > 0)
			macKeyLength = Integer.parseInt(tmp);
		else
			macKeyLength = 0;
	}

	public DHIESParameterSpec(String messageDigestAlgorithm,
			String cipherAlgorithm, String macAlgorithm) {
		this(messageDigestAlgorithm, cipherAlgorithm, macAlgorithm, 0, 0);
	}

	public DHIESParameterSpec(String messageDigestAlgorithm,
			String cipherAlgorithm, String macAlgorithm, int cipherKeyLength,
			int macKeyLength) {
		if (cipherKeyLength < 0 || macKeyLength < 0)
			throw new IllegalArgumentException();

		this.messageDigestAlgorithm = messageDigestAlgorithm;
		this.cipherAlgorithm = cipherAlgorithm;
		this.macAlgorithm = macAlgorithm;

		this.cipherKeyLength = cipherKeyLength;
		this.macKeyLength = macKeyLength;
	}

	public DHIESParameterSpec(DHIESParameterSpec copy) {
		this.messageDigestAlgorithm = copy.messageDigestAlgorithm;
		this.cipherAlgorithm = copy.cipherAlgorithm;
		this.macAlgorithm = copy.macAlgorithm;

		this.cipherKeyLength = copy.cipherKeyLength;
		this.macKeyLength = copy.macKeyLength;
	}

	public String getCipherAlgorithm() {
		return cipherAlgorithm;
	}

	public String getMacAlgorithm() {
		return macAlgorithm;
	}

	public String getMessageDigestAlgorithm() {
		return messageDigestAlgorithm;
	}

	public int getCipherKeyLength() {
		return cipherKeyLength;
	}

	public int getMacKeyLength() {
		return macKeyLength;
	}

	public String toString() {
		StringBuffer tmp = new StringBuffer();

		tmp.append("DHIES(").append(messageDigestAlgorithm).append(',').append(
				cipherAlgorithm).append(',').append(macAlgorithm);

		if (macKeyLength > 0) {
			tmp.append(',').append(Integer.toString(cipherKeyLength));
			tmp.append(',').append(Integer.toString(macKeyLength));
		} else if (cipherKeyLength > 0) {
			tmp.append(',').append(Integer.toString(cipherKeyLength));
		}

		return tmp.append(')').toString();
	}
}
