package beecrypt.io;

import java.io.*;
import javax.crypto.*;

public class MacInputStream extends FilterInputStream {
	private boolean _on;

	protected Mac mac;

	public MacInputStream(InputStream in, Mac mac) {
		super(in);
		this.mac = mac;
	}

	public Mac getMac() {
		return mac;
	}

	public void on(boolean on) {
		_on = on;
	}

	public int read() throws IOException {
		int rc = in.read();
		if (rc >= 0 && _on)
			mac.update((byte) rc);
		return rc;
	}

	public int read(byte[] b, int off, int len) throws IOException {
		int rc = in.read(b, off, len);
		if (rc >= 0 && _on)
			mac.update(b, off, rc);
		return rc;
	}

	public void setMac(Mac m) {
		mac = m;
	}
}
