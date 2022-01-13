package beecrypt.io;

import java.io.*;
import java.math.BigInteger;

public class BeeInputStream extends DataInputStream {
	public BeeInputStream(InputStream in) {
		super(in);
	}

	public BigInteger readBigInteger() throws IOException {
		int size = readInt();
		if (size <= 0)
			throw new IOException("Invalid BigInteger size");

		byte[] data = new byte[size];

		readFully(data);

		return new BigInteger(data);
	}
}
