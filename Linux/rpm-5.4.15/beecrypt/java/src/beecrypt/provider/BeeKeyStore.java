package beecrypt.provider;

import java.io.*;
import java.security.*;
import java.security.cert.*;
import java.security.cert.Certificate;
import java.util.*;
import javax.crypto.*;

import beecrypt.beeyond.*;
import beecrypt.io.*;

public class BeeKeyStore extends KeyStoreSpi {
	private static final char[] EMPTY_PASSWORD = new char[0];

	private static final int BKS_MAGIC = 0xbeecceec;
	private static final int BKS_VERSION_1 = 1;
	private static final int BKS_PRIVATEKEY_ENTRY = 1;
	private static final int BKS_CERTIFICATE_ENTRY = 2;

	static class Entry {
		protected Date date = new Date();
	}

	static class KeyEntry extends Entry {
		byte[] encryptedKey;
		Certificate[] chain;

		KeyEntry() {
			this.encryptedKey = null;
			this.chain = null;
		}

		KeyEntry(byte[] key, Certificate[] chain) {
			this.encryptedKey = key;
			this.chain = (Certificate[]) chain.clone();
		}
	}

	static class CertEntry extends Entry {
		Certificate cert;

		CertEntry() {
			this.cert = null;
		}

		CertEntry(Certificate cert) {
			this.cert = cert;
		}
	}

	private byte[] _bmac;
	private byte[] _salt;
	private int _iter;

	private Hashtable _entries = new Hashtable();

	public Enumeration engineAliases() {
		return _entries.keys();
	}

	public boolean engineContainsAlias(String alias) {
		return _entries.containsKey(alias);
	}

	public void engineDeleteEntry(String alias) {
		_entries.remove(alias);
	}

	public Date engineGetCreationDate(String alias) {
		Object o = _entries.get(alias);
		if (o instanceof Entry)
			return ((Entry) o).date;
		return null;
	}

	public Certificate engineGetCertificate(String alias) {
		Object o = _entries.get(alias);
		if (o instanceof CertEntry)
			return ((CertEntry) o).cert;
		if (o instanceof KeyEntry)
			return ((KeyEntry) o).chain[0];
		return null;
	}

	public String engineGetCertificateAlias(Certificate cert) {
		Iterator it = _entries.entrySet().iterator();

		while (it.hasNext()) {
			Map.Entry me = (Map.Entry) it.next();
			Entry e = (Entry) me.getValue();
			if (e instanceof CertEntry)
				if (cert.equals(((CertEntry) e).cert))
					return (String) me.getKey();
		}
		return null;
	}

	public Certificate[] engineGetCertificateChain(String alias) {
		Object o = _entries.get(alias);
		if (o instanceof KeyEntry)
			return ((KeyEntry) o).chain;
		return null;
	}

	public boolean engineIsCertificateEntry(String alias) {
		return (_entries.get(alias) instanceof CertEntry);
	}

	public void engineSetCertificateEntry(String alias, Certificate cert)
			throws KeyStoreException {
		_entries.put(alias, new CertEntry(cert));
	}

	public Key engineGetKey(String alias, char[] password)
			throws NoSuchAlgorithmException, UnrecoverableKeyException {
		Object o = _entries.get(alias);
		if (o instanceof KeyEntry) {
			KeyEntry ke = (KeyEntry) o;
			PKCS12PBEKey pbekey = new PKCS12PBEKey(password, _salt, _iter);
			try {
				KeyProtector p = new KeyProtector(pbekey);
				return p.recover(ke.encryptedKey);
			} catch (NoSuchPaddingException e) {
				throw new NoSuchAlgorithmException(e.getMessage());
			} catch (InvalidKeyException e) {
				throw new UnrecoverableKeyException(e.getMessage());
			}
		}
		return null;
	}

	public boolean engineIsKeyEntry(String alias) {
		return (_entries.get(alias) instanceof KeyEntry);
	}

	public void engineSetKeyEntry(String alias, byte[] encryptedKey,
			Certificate[] chain) throws KeyStoreException {
		_entries.put(alias, new KeyEntry(encryptedKey, chain));
	}

	public void engineSetKeyEntry(String alias, Key key, char[] password,
			Certificate[] chain) throws KeyStoreException {
		if (key instanceof PrivateKey) {
			PKCS12PBEKey pbekey = new PKCS12PBEKey(password, _salt, _iter);
			try {
				KeyProtector p = new KeyProtector(pbekey);

				byte[] tmp = p.protect((PrivateKey) key);
				if (tmp != null)
					engineSetKeyEntry(alias, tmp, chain);
				else
					throw new KeyStoreException("Failed to protect key");
			} catch (NoSuchAlgorithmException e) {
				throw new KeyStoreException(e.getMessage());
			} catch (InvalidKeyException e) {
				throw new KeyStoreException(e.getMessage());
			}
		} else
			throw new KeyStoreException(
					"BeeKeyStore only supports storing of PrivateKey objects");
	}

	public int engineSize() {
		return _entries.size();
	}

	public void engineLoad(InputStream in, char[] password) throws IOException,
			CertificateException, NoSuchAlgorithmException {
		if (in == null) {
			_salt = SecureRandom.getSeed(64);
			_iter = 1024;
		} else {
			Mac m = Mac.getInstance("HmacSHA256");
			MacInputStream mis = new MacInputStream(in, m);
			DataInputStream dis = new DataInputStream(mis);

			mis.on(false);

			int magic = dis.readInt();
			int version = dis.readInt();

			if (magic != BKS_MAGIC || version != BKS_VERSION_1)
				throw new IOException("Invalid BeeKeyStore format");

			_entries.clear();

			int saltSize = dis.readInt();
			if (saltSize <= 0)
				throw new IOException("Invalid BeeKeyStore salt size");

			_salt = new byte[saltSize];
			dis.readFully(_salt);

			_iter = dis.readInt();
			if (_iter <= 0)
				throw new IOException("Invalid BeeKeyStore iteration count");

			PKCS12PBEKey pbekey = new PKCS12PBEKey(
					password == null ? EMPTY_PASSWORD : password, _salt, _iter);

			try {
				m.init(pbekey);
			} catch (InvalidKeyException e) {
				throw new ProviderException(e.getMessage());
			}

			mis.on(true);

			int entryCount = dis.readInt();
			if (entryCount <= 0)
				throw new IOException("Invalid BeeKeyStore entry count");

			for (int i = 0; i < entryCount; i++) {
				String alias;

				switch (dis.readInt()) {
				case BKS_PRIVATEKEY_ENTRY: {
					alias = dis.readUTF();

					KeyEntry e = new KeyEntry();
					e.date.setTime(dis.readLong());

					int keySize = dis.readInt();
					if (keySize <= 0)
						throw new IOException(
								"Invalid BeeKeyStore encoded key size");

					e.encryptedKey = new byte[keySize];
					dis.readFully(e.encryptedKey);

					int certCount = dis.readInt();
					if (certCount <= 0)
						throw new IOException(
								"Invalid BeeKeyStore certificate count");

					e.chain = new Certificate[certCount];

					for (int j = 0; j < certCount; j++) {
						String type = dis.readUTF();

						CertificateFactory cf = CertificateFactory
								.getInstance(type);

						int certSize = dis.readInt();
						if (certSize <= 0)
							throw new IOException(
									"Invalid BeeKeyStore encoded certificate size");

						byte[] cert = new byte[certSize];
						dis.readFully(cert);

						ByteArrayInputStream bis = new ByteArrayInputStream(
								cert);
						e.chain[j] = cf.generateCertificate(bis);
					}

					_entries.put(alias, e);
				}
					break;

				case BKS_CERTIFICATE_ENTRY: {
					alias = dis.readUTF();

					CertEntry e = new CertEntry();
					e.date.setTime(dis.readLong());

					String type = dis.readUTF();

					CertificateFactory cf = CertificateFactory
							.getInstance(type);

					int certSize = dis.readInt();
					if (certSize <= 0)
						throw new IOException(
								"Invalid BeeKeyStore encoded certificate size");

					byte[] cert = new byte[certSize];
					dis.readFully(cert);

					ByteArrayInputStream bis = new ByteArrayInputStream(cert);

					e.cert = cf.generateCertificate(bis);

					_entries.put(alias, e);
				}
					break;

				default:
					throw new IOException("Invalid BeeKeyStore entry tag");
				}
			}

			byte[] computedMac, originalMac;

			mis.on(false);

			int macSize = dis.available();
			if (macSize <= 0)
				throw new IOException("Invalid BeeKeyStore MAC size");

			computedMac = m.doFinal();
			if (macSize != computedMac.length)
				throw new IOException(
						"BeeKeyStore has been tampered with, or password was incorrect (incorrect mac size)");

			originalMac = new byte[macSize];
			dis.readFully(originalMac);

			if (!Arrays.equals(computedMac, originalMac))
				throw new IOException(
						"BeeKeyStore has been tampered with, or password was incorrect (incorrect mac)");
		}

	}

	public void engineStore(OutputStream out, char[] password)
			throws IOException, CertificateException, NoSuchAlgorithmException {
		Mac m = Mac.getInstance("HmacSHA256");
		PKCS12PBEKey pbekey = new PKCS12PBEKey(
				password == null ? EMPTY_PASSWORD : password, _salt, _iter);

		try {
			m.init(pbekey);
		} catch (InvalidKeyException e) {
			throw new ProviderException(e.getMessage());
		}

		MacOutputStream mos = new MacOutputStream(out, m);
		DataOutputStream dos = new DataOutputStream(mos);

		mos.on(false);
		dos.writeInt(BKS_MAGIC);
		dos.writeInt(BKS_VERSION_1);
		dos.writeInt(_salt.length);
		dos.write(_salt);
		dos.writeInt(_iter);
		mos.on(true);
		dos.writeInt(_entries.size());
		Iterator it = _entries.entrySet().iterator();
		while (it.hasNext()) {
			Map.Entry me = (Map.Entry) it.next();
			String alias = (String) me.getKey();
			Object o = me.getValue();
			if (o instanceof KeyEntry) {
				KeyEntry ke = (KeyEntry) o;

				dos.writeInt(BKS_PRIVATEKEY_ENTRY);
				dos.writeUTF(alias);
				dos.writeLong(ke.date.getTime());
				dos.writeInt(ke.encryptedKey.length);
				dos.write(ke.encryptedKey);
				dos.writeInt(ke.chain.length);
				for (int i = 0; i < ke.chain.length; i++) {
					Certificate cert = ke.chain[i];

					dos.writeUTF(cert.getType());
					byte[] enc = cert.getEncoded();
					dos.writeInt(enc.length);
					dos.write(enc);
				}
				continue;
			}
			if (o instanceof CertEntry) {
				CertEntry ce = (CertEntry) o;

				dos.writeInt(BKS_CERTIFICATE_ENTRY);
				dos.writeUTF(alias);
				dos.writeLong(ce.date.getTime());
				dos.writeUTF(ce.cert.getType());
				byte[] enc = ce.cert.getEncoded();
				dos.writeInt(enc.length);
				dos.write(enc);
				continue;
			}
			throw new ProviderException(
					"entry is neither KeyEntry nor CertEntry");
		}

		dos.flush();
		mos.flush();

		out.write(m.doFinal());
		out.close();
	}
}
