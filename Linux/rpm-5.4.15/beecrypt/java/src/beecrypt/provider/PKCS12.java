package beecrypt.provider;

import java.io.*;
import java.util.*;
import java.security.*;
import javax.crypto.*;
import javax.crypto.interfaces.*;
import javax.crypto.spec.*;

import beecrypt.beeyond.*;

public class PKCS12 {
	public static final byte ID_CIPHER = 0x1;
	public static final byte ID_IV     = 0x2;
	public static final byte ID_MAC    = 0x3;

	public static final byte[] deriveKey(MessageDigest md, int blockSize, int keyLength, byte id, byte[] password, byte[] salt, int iterations) {
		int remain;

		md.reset();
		md.update(id);

		// hash a whole number of blocks filled with salt data
		if (salt.length > 0) {
			remain = ((salt.length / blockSize) + (salt.length % blockSize)) * blockSize;
			while (remain > 0) {
				int tmp = (remain > salt.length) ? salt.length : remain;
				md.update(salt, 0, tmp);
				remain -= tmp;
			}
		}
		// hash a whole number of blocks filled with password data
		if (password.length > 0) {
			remain = ((password.length / blockSize) + (password.length % blockSize)) * blockSize;
			while (remain > 0) {
				int tmp = (remain > password.length) ? password.length : remain;
				md.update(password, 0, tmp);
				remain -= tmp;
			}
		}

		while (iterations-- > 0)
			md.update(md.digest());

		// compute the final digest
		byte[] digest = md.digest();

		// allocate a key of the requested size
		byte[] key = new byte[keyLength];

		if (keyLength > 0) {
			// fill the key with the result
			int offset = 0;
			remain = keyLength;
			while (remain > 0) {
				int tmp = (remain > digest.length) ? digest.length : remain;
				System.arraycopy(digest, 0, key, offset, tmp);
				offset += tmp;
				remain -= tmp;
			}
		}

		return key;
	}

	public static final byte[] deriveKey(MessageDigest md, int blockSize, int keyLength, byte id, PBEKey key) {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		DataOutputStream dos = new DataOutputStream(bos);

		try {
			char[] tmp = key.getPassword();
			if (tmp != null)
				for (int i = 0; i < tmp.length; i++)
					dos.writeChar(tmp[i]);

			dos.writeChar((char) 0);
			dos.close();
		}
		catch (IOException e)
		{
			throw new RuntimeException(e.getMessage());
		}

		return deriveKey(md, blockSize, keyLength, id, bos.toByteArray(), key.getSalt(), key.getIterationCount());
	}
}
