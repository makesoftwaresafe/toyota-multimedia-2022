package beecrypt.io;

import java.io.*;
import javax.crypto.*;

public class MacOutputStream extends FilterOutputStream {
	private boolean _on;

	protected Mac mac;

	public MacOutputStream(OutputStream out, Mac mac) {
		super(out);
		this.mac = mac;
	}

	public Mac getMac() {
		return mac;
	}

	public void on(boolean on) {
		_on = on;
	}

	public void write(byte[] b, int off, int len) throws IOException {
		if (len > 0) {
			out.write(b, off, len);
			if (_on)
				;
			mac.update(b, off, len);
		}
	}

	public void setMac(Mac m) {
		mac = m;
	}
}
