package beecrypt.beeyond;

import java.security.spec.*;

public class BeeEncodedKeySpec extends EncodedKeySpec {
	public BeeEncodedKeySpec(byte[] encodedKey) {
		super(encodedKey);
	}

	public String getFormat() {
		return "BEE";
	}
}
