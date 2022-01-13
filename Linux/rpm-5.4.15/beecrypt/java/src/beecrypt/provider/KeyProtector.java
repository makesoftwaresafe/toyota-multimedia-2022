package beecrypt.provider;

import java.io.*;
import java.util.*;
import java.security.*;
import javax.crypto.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

import beecrypt.beeyond.*;

public class KeyProtector {
	private SecretKeySpec _cipher_key;
	private SecretKeySpec _mac_key;
	private IvParameterSpec _iv;

	public KeyProtector(PBEKey key) throws InvalidKeyException,
			NoSuchAlgorithmException {
		byte[] rawKey = key.getEncoded();
		if (rawKey == null)
			throw new InvalidKeyException("PBEKey must have an encoding");

		byte[] salt = key.getSalt();

		int iter = key.getIterationCount();

		MessageDigest sha256 = MessageDigest.getInstance("SHA-256");

		_cipher_key = new SecretKeySpec(PKCS12.deriveKey(sha256, 64, 32,
				PKCS12.ID_CIPHER, rawKey, salt, iter), "RAW");
		_mac_key = new SecretKeySpec(PKCS12.deriveKey(sha256, 64, 32,
				PKCS12.ID_MAC, rawKey, salt, iter), "RAW");
		_iv = new IvParameterSpec(PKCS12.deriveKey(sha256, 64, 16,
				PKCS12.ID_IV, rawKey, salt, iter));
	}

	byte[] protect(PrivateKey key) {
		if (key.getEncoded() == null)
			return null;
		if (key.getFormat() == null)
			return null;

		try {
			byte[] enc = key.getEncoded();

			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			DataOutputStream dos = new DataOutputStream(bos);

			dos.writeUTF(key.getAlgorithm());
			dos.writeUTF(key.getFormat());
			dos.writeInt(enc.length);
			dos.write(enc);
			dos.close();

			byte[] clearText = bos.toByteArray();

			Mac m = Mac.getInstance("HmacSHA256");

			m.init(_mac_key);

			byte[] mac = m.doFinal(clearText);

			Cipher c = Cipher.getInstance("AES/CBC/PKCS5Padding");

			c.init(Cipher.ENCRYPT_MODE, _cipher_key, _iv);

			return c.doFinal(clearText);
		} catch (Exception e) {
			return null;
		}
	}

	PrivateKey recover(byte[] encryptedKey) throws NoSuchAlgorithmException,
			NoSuchPaddingException, UnrecoverableKeyException {
		Cipher c = Cipher.getInstance("AES/CBC/PKCS5Padding");
		Mac m = Mac.getInstance("HmacSHA256");

		final int macLength = m.getMacLength();

		final int cipherTextLength = encryptedKey.length - macLength;
		if (cipherTextLength <= 0)
			throw new UnrecoverableKeyException("encrypted data way too short");

		try {
			c.init(Cipher.DECRYPT_MODE, _cipher_key, _iv);

			byte[] clearText = c.doFinal(encryptedKey, 0, cipherTextLength);

			m.init(_mac_key);

			byte[] computedMac = m.doFinal(clearText);

			byte[] originalMac = new byte[m.getMacLength()];

			System.arraycopy(encryptedKey, cipherTextLength, originalMac, 0,
					macLength);

			if (!Arrays.equals(computedMac, originalMac))
				throw new UnrecoverableKeyException(
						"mac of decrypted key didn't match");

			ByteArrayInputStream bis = new ByteArrayInputStream(clearText);
			DataInputStream dis = new DataInputStream(bis);

			String algorithm = dis.readUTF();
			String format = dis.readUTF();

			int encSize = dis.readInt();
			if (encSize <= 0)
				throw new IOException("key size < 0");

			byte[] enc = new byte[encSize];

			dis.readFully(enc);

			KeyFactory kf = KeyFactory.getInstance(algorithm);

			return kf.generatePrivate(new AnyEncodedKeySpec(format, enc));
		} catch (Exception e) {
		}
		throw new UnrecoverableKeyException("unable to recover key");
	}
}
