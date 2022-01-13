#ifndef _H_RPMUUID_
#define	_H_RPMUUID_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate a Universally Unique Identifier (UUID).
 * @param version	UUID version 1, 3, 4 or 5
 * @param ns	namespace UUID in string representation or symbolic name (UUID version 3 and 5 only)
 * @param data	data to digest (UUID version 3 and 5 only)
 * @retval buf_str	buffer of at least 37 bytes (UUID_LEN_STR+1) to store UUID in string representation
 * @retval buf_bin	buffer of at least 16 bytes (UUID_LEN_BIN) to store UUID in binary representation
 * @return		0 on success, 1 on failure
 */
int rpmuuidMake(int version, const char *ns,
		/*@null@*/ const char *data,
		/*@out@*/ char *buf_str,
		/*@out@*/ /*@null@*/ unsigned char *buf_bin)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* _H_ RPMUUID_ */

