package beecrypt.io;

import java.io.*;
import java.math.BigInteger;

public class BeeOutputStream extends DataOutputStream {
	public BeeOutputStream(OutputStream out) {
		super(out);
	}

	public void writeBigInteger(BigInteger b) throws IOException {
		byte[] data = b.toByteArray();

		writeInt(data.length);
		write(data);
	}
}
