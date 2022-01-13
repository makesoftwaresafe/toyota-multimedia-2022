package beecrypt.provider;

import java.security.*;

/**
 * This class specifies the set of algorithms defined by the BeeCrypt JCE 1.4
 * Cryptography Provider.
 * <p>
 * 
 * @author Bob Deblier &lt;bob.deblier@telenet.be&gt;
 */
public final class BaseProvider extends Provider {
	private static final String NAME = "BeeCrypt",
			INFO = "BeeCrypt JCE 1.4 Cryptography Provider";

	private static final double VERSION = 4.2;

	static {
		System.loadLibrary("beecrypt");
		System.loadLibrary("beecrypt_java");
	}

	public BaseProvider() {
		super(NAME, VERSION, INFO);

		AccessController.doPrivileged(new java.security.PrivilegedAction() {
			public Object run() {
				// setProperty("AlgorithmParameterGenerator.DH",
				// "beecrypt.provider.DHParameterGenerator");
				setProperty("AlgorithmParameters.DH",
						"beecrypt.provider.DHParameters");
				setProperty("AlgorithmParameters.DHIES",
						"beecrypt.provider.DHIESParameters");
				// setProperty("AlgorithmParameters.DSA",
				// "beecrypt.provider.DSAParameters");
				setProperty("CertificateFactory.BEE",
						"beecrypt.provider.BeeCertificateFactory");
				setProperty("CertPathValidator.BEE",
						"beecrypt.provider.BeeCertPathValidator");
				setProperty("Cipher.AES", "beecrypt.provider.AES");
				setProperty("Cipher.AES SupportKeyFormats", "RAW");
				setProperty("Cipher.DHIES", "beecrypt.provider.DHIES");
				setProperty("KeyFactory.DH", "beecrypt.provider.DHKeyFactory");
				setProperty("KeyFactory.DSA", "beecrypt.provider.DSAKeyFactory");
				setProperty("KeyFactory.RSA", "beecrypt.provider.RSAKeyFactory");
				setProperty("KeyPairGenerator.DH",
						"beecrypt.provider.DHKeyPairGenerator");
				setProperty("KeyPairGenerator.RSA",
						"beecrypt.provider.RSAKeyPairGenerator");
				setProperty("KeyStore.BEE", "beecrypt.provider.BeeKeyStore");
				setProperty("Mac.HmacMD5", "beecrypt.provider.HMACMD5");
				setProperty("Mac.HmacSHA1", "beecrypt.provider.HMACSHA1");
				setProperty("Mac.HmacSHA256", "beecrypt.provider.HMACSHA256");
				setProperty("Mac.HmacSHA384", "beecrypt.provider.HMACSHA384");
				setProperty("Mac.HmacSHA512", "beecrypt.provider.HMACSHA512");
				setProperty("MessageDigest.MD4", "beecrypt.provider.MD4");
				setProperty("MessageDigest.MD5", "beecrypt.provider.MD5");
				setProperty("MessageDigest.SHA-1", "beecrypt.provider.SHA1");
				setProperty("MessageDigest.SHA-224", "beecrypt.provider.SHA224");
				setProperty("MessageDigest.SHA-256", "beecrypt.provider.SHA256");
				setProperty("MessageDigest.SHA-384", "beecrypt.provider.SHA384");
				setProperty("MessageDigest.SHA-512", "beecrypt.provider.SHA512");
				setProperty(
						"Als.Alias.AlgorithmParameterGenerator.DiffieHellman",
						"AlgorithmParameterGenerator.DH");
				setProperty("Als.Alias.AlgorithmParameters.DiffieHellman",
						"AlgorithmParameters.DH");
				setProperty("Als.Alias.AlgorithmParameters.DHAES",
						"AlgorithmParameters.DHIES");
				setProperty("Als.Alias.AlgorithmParameters.DHES",
						"AlgorithmParameters.DHIES");
				setProperty("Als.Alias.Cipher.DHAES", "Cipher.DHIES");
				setProperty("Als.Alias.Cipher.DHES", "Cipher.DHIES");
				setProperty("Als.Alias.KeyFactory.DiffieHellman",
						"KeyFactory.DH");
				setProperty("Als.Alias.KeyPairGenerator.DiffieHellman",
						"KeyPairGenerator.DH");
				setProperty("Als.Alias.Mac.HMAC-MD5", "Mac.HmacMD5");
				setProperty("Als.Alias.Mac.HMAC-SHA-1", "Mac.HmacSHA1");
				setProperty("Als.Alias.Mac.HMAC-SHA-256", "Mac.HmacSHA256");
				setProperty("Als.Alias.Mac.HMAC-SHA-384", "Mac.HmacSHA384");
				setProperty("Als.Alias.Mac.HMAC-SHA-512", "Mac.HmacSHA512");
				setProperty("Als.Alias.MessageDigest.SHA",
						"MessageDigest.SHA-1");

				return null;
			}
		});
	}
}
