package beecrypt.beeyond;

import java.security.spec.*;

public class AnyEncodedKeySpec extends EncodedKeySpec {
	String format;

	public AnyEncodedKeySpec(String format, byte[] encodedKey) {
		super(encodedKey);

		this.format = format;
	}

	public String getFormat() {
		return format;
	}
}
