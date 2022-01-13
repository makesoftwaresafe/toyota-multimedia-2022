/**
 * Copyright (C) 2006 International Business Machines
 * Copyright (C) 2011 Gazzang, Inc
 * Author(s): Michael C. Thompson <mcthomps@us.ibm.com>
 *            Dustin Kirkland <dustin.kirkland@gazzang.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <errno.h>
#ifdef ENABLE_NSS
#include <nss.h>
#include <pk11func.h>
#else
#include <gcrypt.h>
#endif /* #ifdef ENABLE_NSS */
#include <keyutils.h>
#ifndef S_SPLINT_S
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <inttypes.h>
#include "../include/ecryptfs.h"

#ifndef ENOKEY
#warning ENOKEY is not defined in your errno.h; setting it to 126
#define ENOKEY 126
#endif

/**
 * @auth_tok: (out) This function will allocate; callee must free
 * @auth_tok_sig: (out) Allocated memory this function fills in:
                        (ECRYPTFS_SIG_SIZE_HEX + 1)
 * @fekek: (out) Allocated memory this function fills in: ECRYPTFS_MAX_KEY_BYTES
 * @salt: (in) salt: ECRYPTFS_SALT_SIZE
 * @passphrase: (in) passphrase: ECRYPTFS_MAX_PASSPHRASE_BYTES
 */
int ecryptfs_generate_passphrase_auth_tok(struct ecryptfs_auth_tok **auth_tok,
					  char *auth_tok_sig, char *fekek,
					  char *salt, char *passphrase)
{
	int rc;

	*auth_tok = NULL;
	rc = generate_passphrase_sig(auth_tok_sig, fekek, salt, passphrase);
	if (rc) {
		syslog(LOG_ERR, "Error generating passphrase signature; "
		       "rc = [%d]\n", rc);
		rc = (rc < 0) ? rc : rc * -1;
		goto out;
	}
	*auth_tok = malloc(sizeof(struct ecryptfs_auth_tok));
	if (!*auth_tok) {
		syslog(LOG_ERR, "Unable to allocate memory for auth_tok\n");
		rc = -ENOMEM;
		goto out;
	}
	rc = generate_payload(*auth_tok, auth_tok_sig, salt, fekek);
	if (rc) {
		syslog(LOG_ERR, "Error generating payload for auth tok key; "
		       "rc = [%d]\n", rc);
		memset(*auth_tok, 0, sizeof(struct ecryptfs_auth_tok));
		free(*auth_tok);
		*auth_tok = NULL;
		rc = (rc < 0) ? rc : rc * -1;
		goto out;
	}
out:
	return rc;
}

/**
 * ecryptfs_passphrase_sig_from_blob
 * @blob: Byte array of struct ecryptfs_auth_tok
 *
 * SWIG support function.
 */
binary_data ecryptfs_passphrase_sig_from_blob(char *blob)
{
	struct ecryptfs_auth_tok *auth_tok;
	binary_data bd;

	auth_tok = (struct ecryptfs_auth_tok *)blob;
	bd.size = (ECRYPTFS_PASSWORD_SIG_SIZE + 1);
	bd.data = auth_tok->token.password.signature;
	return bd;
}

/**
 * ecryptfs_passphrase_blob
 * @salt: Hexadecimal representation of the salt value
 * @passphrase: Passphrase
 *
 * SWIG support function.
 */
binary_data ecryptfs_passphrase_blob(char *salt, char *passphrase)
{
	unsigned char *blob;
	struct ecryptfs_auth_tok *auth_tok;
	char auth_tok_sig[ECRYPTFS_SIG_SIZE_HEX + 1];
	char fekek[ECRYPTFS_MAX_KEY_BYTES];
	binary_data bd;
	int rc;

	memset(&bd, 0, sizeof(bd));
	rc = ecryptfs_generate_passphrase_auth_tok(&auth_tok, auth_tok_sig,
						   fekek, salt, passphrase);
	if (rc) {
		syslog(LOG_ERR, "%s: Error attempting to generate passphrase "
		       "authentication token blob; rc = [%d]\n", __FUNCTION__,
		       rc);
		blob = NULL;
		goto out;
	}
	blob = (unsigned char *)auth_tok;
	bd.size = sizeof(struct ecryptfs_auth_tok);
	bd.data = blob;
out:
	return bd;
}


int ecryptfs_remove_auth_tok_from_keyring(char *auth_tok_sig)
{
	int rc;

	rc = (int)keyctl_search(KEY_SPEC_USER_KEYRING, "user", auth_tok_sig, 0);
	if (rc < 0) {
		rc = errno;
		syslog(LOG_ERR, "Failed to find key with sig [%s]: %m\n",
		       auth_tok_sig);
		goto out;
	}
	rc = keyctl_unlink(rc, KEY_SPEC_USER_KEYRING);
	if (rc < 0) {
		rc = errno;
		syslog(LOG_ERR, "Failed to unlink key with sig [%s]: %s\n",
		       auth_tok_sig, strerror(rc));
		goto out;
	}
	rc = 0;
out:
	return rc;
}
int ecryptfs_add_auth_tok_to_keyring(struct ecryptfs_auth_tok *auth_tok,
				     char *auth_tok_sig)
{
	int rc;

	rc = (int)keyctl_search(KEY_SPEC_USER_KEYRING, "user", auth_tok_sig, 0);
	if (rc != -1) { /* we already have this key in keyring; we're done */
		rc = 1;
		goto out;
	} else if ((rc == -1) && (errno != ENOKEY)) {
		int errnum = errno;

		syslog(LOG_ERR, "keyctl_search failed: %m errno=[%d]\n",
		       errnum);
		rc = (errnum < 0) ? errnum : errnum * -1;
		goto out;
	}
	rc = add_key("user", auth_tok_sig, (void *)auth_tok,
		     sizeof(struct ecryptfs_auth_tok), KEY_SPEC_USER_KEYRING);
	if (rc == -1) {
		rc = -errno;
		syslog(LOG_ERR, "Error adding key with sig [%s]; rc = [%d] "
		       "\"%m\"\n", auth_tok_sig, rc);
		if (rc == -EDQUOT)
			syslog(LOG_WARNING, "Error adding key to keyring - keyring is full\n");
		goto out;
	}
	rc = 0;
out:
	return rc;
}

/**
 * ecryptfs_add_blob_to_keyring
 * @blob: Byte array containing struct ecryptfs_auth_tok
 * @sig: Hexadecimal representation of the auth tok signature
 *
 * SWIG support function.
 */
int ecryptfs_add_blob_to_keyring(char *blob, char *sig)
{
	int rc;

	rc = ecryptfs_add_auth_tok_to_keyring((struct ecryptfs_auth_tok *)blob,
					      sig);
	return rc;
}

/**
 * This is the common functionality used to put a password generated key into
 * the keyring, shared by both non-interactive and interactive signature
 * generation code.
 *
 * Returns 0 on add, 1 on pre-existed, negative on failure.
 */
int ecryptfs_add_passphrase_key_to_keyring(char *auth_tok_sig, char *passphrase,
					   char *salt)
{
	int rc;
	char fekek[ECRYPTFS_MAX_KEY_BYTES];
	struct ecryptfs_auth_tok *auth_tok = NULL;

	rc = ecryptfs_generate_passphrase_auth_tok(&auth_tok, auth_tok_sig,
						   fekek, salt, passphrase);
	if (rc) {
		syslog(LOG_ERR, "%s: Error attempting to generate the "
		       "passphrase auth tok payload; rc = [%d]\n",
		       __FUNCTION__, rc);
		goto out;
	}
	rc = ecryptfs_add_auth_tok_to_keyring(auth_tok, auth_tok_sig);
	if (rc < 0) {
		syslog(LOG_ERR, "%s: Error adding auth tok with sig [%s] to "
		       "the keyring; rc = [%d]\n", __FUNCTION__, auth_tok_sig,
		       rc);
		goto out;
	}
out:
	if (auth_tok) {
		memset(auth_tok, 0, sizeof(*auth_tok));
		free(auth_tok);
	}
	return rc;
}

/**
 * A wrapper around write(2) that handles short and interrupted writes.
 *
 * Returns the number of bytes written or -1 with errno set on failure.
 */
static ssize_t do_write(int fd, const void *buf, size_t count)
{
	ssize_t rc = 0;

	do {
		ssize_t bytes = write(fd, buf + rc, count - rc);

		if (bytes == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		rc += bytes;
	} while (rc < count);

	return rc;
}

/**
 * A wrapper around read(2) that handles short and interrupted reads.
 *
 * Returns the number of bytes read or -1 with errno set on failure.
 */
static ssize_t do_read(int fd, void *buf, size_t count)
{
	ssize_t rc = 0;

	do {
		ssize_t bytes = read(fd, buf + rc, count - rc);

		if (bytes == 0) {
			break;
		} else if (bytes == -1) {
			if (errno == EINTR)
				continue;

			return -1;
		}

		rc += bytes;
	} while (rc < count);

	return rc;
}

/**
 * read_wrapped_passphrase_file_version
 * @fd: A file descriptor, opened for reading, of a wrapped passphrase file
 * @version: On success, *version is set to the detected file version
 *
 * Sets the fd offset to 0 and attempts to determine the version number of the
 * opened wrapped-passphrase file. If a versioned wrapped-passphrase file is
 * not found and the first 16 bytes of the file are hex encoded values, then
 * the version is assumed to be '1'.
 *
 * Returns 0 on success, sets *version to the determined wrapped-passphrase
 * file version, and ensures that the fd offset is appropriately set for
 * reading the next field in the wrapped passphrase file. Returns negative on
 * error (*version and the fd offset is undefined upon error).
 */
static int read_wrapped_passphrase_file_version(int fd, uint8_t *version)
{
	char buf[ECRYPTFS_SIG_SIZE_HEX];
	int bytes_read, i;

	memset(buf, 0, sizeof(buf));

	if (lseek(fd, 0, SEEK_SET) != 0)
		return -errno;

	bytes_read = do_read(fd, buf, sizeof(buf));
	if (bytes_read < 0)
		return -errno;
	else if (bytes_read != sizeof(buf))
		return -EINVAL;

	if (buf[0] == ':') {
		/* A leading ':' character means that this is a properly
		 * versioned wrapped passphrase file. The second octet contains
		 * the version number.
		 */
		uint8_t v = buf[1];

		/* Only version 2 files are currently supported */
		if (v != 2)
			return -ENOTSUP;

		/* Set the offset to the beginning of the wrapping salt field */
		if (lseek(fd, 2, SEEK_SET) != 2)
			return -errno;

		*version = v;
	} else {
		/* This wrapped passphrase file isn't versioned. We can assume
		 * that it is a "version 1" file if the first 16 bytes are hex
		 * encoded values.
		 */
		for (i = 0; i < bytes_read; i++) {
			if (!isxdigit(buf[i]))
				return -EINVAL;
		}

		/* Reset the offset to 0 since there is no actual version field
		 * in version 1 files
		 */
		if (lseek(fd, 0, SEEK_SET) != 0)
			return -errno;

		*version = 1;
	}

	return 0;
}

/**
 * __ecryptfs_detect_wrapped_passphrase_file_version
 * @filename: The path of a wrapped passphrase file
 * @version: On success, *version is set to the detected file version
 *
 * THIS FUNCTION IS NOT PART OF THE LIBECRYPTFS PUBLIC API. Code external to
 * ecryptfs-utils should not use it.
 *
 * Detects the wrapped passphrase file version of @filename.
 *
 * Returns 0 on success, sets *version to the determined wrapped-passphrase
 * file version. Returns negative on error (*version is undefined upon error).
 */
int __ecryptfs_detect_wrapped_passphrase_file_version(const char *filename,
						      uint8_t *version)
{
	int fd = -1;
	int rc;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		rc = -errno;
		goto out;
	}

	rc = read_wrapped_passphrase_file_version(fd, version);
	if (rc != 0)
		goto out;

	rc = 0;
out:
	if (fd != -1)
		close(fd);

	return rc;
}

int ecryptfs_wrap_passphrase_file(char *dest, char *wrapping_passphrase,
				  char *salt, char *src)
{
	int rc = 0;
	ssize_t size;
	int fd;
	char *p = NULL;
	char decrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES + 1];

	memset(decrypted_passphrase, 0, sizeof(decrypted_passphrase));
	if ((fd = open(src, O_RDONLY)) == -1) {
		syslog(LOG_ERR, "Error attempting to open [%s] for reading\n",
		       src);
		rc = -EIO;
		goto out;
	}
	if ((size = do_read(fd, decrypted_passphrase,
			    ECRYPTFS_MAX_PASSPHRASE_BYTES)) <= 0) {
		syslog(LOG_ERR, "Error attempting to read encrypted "
		       "passphrase from file [%s]; size = [%zd]\n",
		       src, size);
                p = strrchr(decrypted_passphrase, '\n');
                if (p) *p = '\0';
		rc = -EIO;
		close(fd);
		goto out;
	}
	close(fd);
	if (ecryptfs_wrap_passphrase(dest, wrapping_passphrase, salt,
	    decrypted_passphrase) == 0) {
		unlink(src);
	} else {
		syslog(LOG_ERR, "Error attempting to wrap passphrase file "
			"[%s]-> [%s]\n", src, dest);
		rc = -EIO;
		goto out;
	}
out:
	return rc;
}

static int read_urandom(void *buf, size_t count)
{
	ssize_t bytes;
	int fd = -1;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1)
		return -1;

	bytes = do_read(fd, buf, count);
	close(fd);

	return bytes;
}

/**
 * write_v2_wrapped_passphrase_file
 * @filename: Path to the wrapped passphrase file
 * @wrapping_salt: The salt to be used with the wrapping passphrase
 * @wrapping_key_sig: The signature of the wrapping key
 * @encrypted_passphrase: The encrypted passphrase
 * @encrypted_passphrase_bytes: The size of the encrypted passphrase
 *
 * Writes a version 2 wrapped passphrase file containing the following format
 * described in the read_v2_wrapped_passphrase_file() function.
 *
 * Returns 0 upon success. Negative upon error.
 */
static int write_v2_wrapped_passphrase_file(const char *filename,
			const char wrapping_salt[ECRYPTFS_SALT_SIZE],
			const char wrapping_key_sig[ECRYPTFS_SIG_SIZE_HEX],
			const char *encrypted_passphrase,
			int encrypted_passphrase_bytes)
{
	ssize_t size;
	uint8_t version = 2;
	mode_t old_umask;
	char *temp = NULL;
	int fd = -1;
	int rc;

	if (asprintf(&temp, "%s-XXXXXX", filename) < 0) {
		rc = -errno;
		temp = NULL;
		goto out;
	}

	old_umask = umask(S_IRWXG | S_IRWXO);
	fd = mkstemp(temp);
	umask(old_umask);
	if (fd == -1) {
		rc = -errno;
		goto out;
	}

	size = do_write(fd, ":", 1);
	if (size != 1) {
		rc = size == -1 ? -errno : -EIO;
		goto out;
	}

	size = do_write(fd, &version, 1);
	if (size != 1) {
		rc = size == -1 ? -errno : -EIO;
		goto out;
	}

	size = do_write(fd, wrapping_salt, ECRYPTFS_SALT_SIZE);
	if (size != ECRYPTFS_SALT_SIZE) {
		rc = size == -1 ? -errno : -EIO;
		goto out;
	}

	size = do_write(fd, wrapping_key_sig, ECRYPTFS_SIG_SIZE_HEX);
	if (size != ECRYPTFS_SIG_SIZE_HEX) {
		rc = size == -1 ? -errno : -EIO;
		goto out;
	}

	size = do_write(fd, encrypted_passphrase, encrypted_passphrase_bytes);
	if (size != encrypted_passphrase_bytes) {
		rc = size == -1 ? -errno : -EIO;
		goto out;
	}

	if (fsync(fd) == -1) {
		rc = -errno;
		goto out;
	}

	close(fd);
	fd = -1;

	if (rename(temp, filename) == -1) {
		rc = -errno;
		goto out;
	}

	rc = 0;
out:
	if (fd != -1)
		close(fd);
	free(temp);

	return rc;
}

/**
 * ecryptfs_wrap_passphrase
 * @filename: Path to the wrapped passphrase file
 * @wrapping_passphrase: The passphrase used for wrapping the @decrypted_passphrase
 * @unused: Previously used for specifying a wrapping salt. It is now randomly
 *  generated so @unused is no longer used.
 * @decrypted_passphrase: The passphrase to be wrapped
 *
 * Returns 0 upon success. Negative upon error.
 */
int ecryptfs_wrap_passphrase(char *filename, char *wrapping_passphrase,
			     char *unused, char *decrypted_passphrase)
{
	char wrapping_salt[ECRYPTFS_SALT_SIZE];
	char wrapping_auth_tok_sig[ECRYPTFS_SIG_SIZE_HEX + 1];
	char wrapping_key[ECRYPTFS_MAX_KEY_BYTES];
	char padded_decrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES +
		ECRYPTFS_AES_BLOCK_SIZE + 1];
	char encrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES +
		ECRYPTFS_AES_BLOCK_SIZE + 1];
	int encrypted_passphrase_pos = 0;
	int decrypted_passphrase_pos = 0;
#ifdef ENABLE_NSS
	int tmp1_outlen = 0;
	int tmp2_outlen = 0;
	SECStatus err;
	SECItem key_item;
	PK11SymKey *sym_key = NULL;
	PK11SlotInfo *slot = NULL;
	PK11Context *enc_ctx = NULL;
	SECItem *sec_param = NULL;
#else
#warning Building against gcrypt instead of nss
       gcry_cipher_hd_t gcry_handle;
       gcry_error_t gcry_err;
#endif /* #ifdef ENABLE_NSS */
	int encrypted_passphrase_bytes;
	int decrypted_passphrase_bytes;
	int fd;
	ssize_t size;
	int rc;

	decrypted_passphrase_bytes = strlen(decrypted_passphrase);
	if (decrypted_passphrase_bytes > ECRYPTFS_MAX_PASSPHRASE_BYTES) {
		syslog(LOG_ERR, "Decrypted passphrase is [%d] bytes long; "
		       "[%d] is the max\n", decrypted_passphrase_bytes,
		       ECRYPTFS_MAX_PASSPHRASE_BYTES);
		rc = -EIO;
		goto out;
	}
	rc = read_urandom(wrapping_salt, ECRYPTFS_SALT_SIZE);
	if (rc != ECRYPTFS_SALT_SIZE) {
		rc = rc == -1 ? -errno : -EIO;
		syslog(LOG_ERR, "Error generating random salt: %s\n",
		       strerror(-rc));
		goto out;
	}
	rc = generate_passphrase_sig(wrapping_auth_tok_sig, wrapping_key,
				     wrapping_salt, wrapping_passphrase);
	if (rc) {
		syslog(LOG_ERR, "Error generating passphrase signature; "
		       "rc = [%d]\n", rc);
		rc = (rc < 0) ? rc : rc * -1;
		goto out;
	}
	memset(padded_decrypted_passphrase, 0,
	       (ECRYPTFS_MAX_PASSPHRASE_BYTES + 1));
	memcpy(padded_decrypted_passphrase, decrypted_passphrase,
	       decrypted_passphrase_bytes);
	if ((decrypted_passphrase_bytes % ECRYPTFS_AES_BLOCK_SIZE) != 0)
		decrypted_passphrase_bytes += (ECRYPTFS_AES_BLOCK_SIZE
					       - (decrypted_passphrase_bytes
						  % ECRYPTFS_AES_BLOCK_SIZE));
	encrypted_passphrase_bytes = decrypted_passphrase_bytes;
#ifdef ENABLE_NSS
	NSS_NoDB_Init(NULL);
	slot = PK11_GetBestSlot(CKM_AES_ECB, NULL);
	key_item.data = (unsigned char *)wrapping_key;
	key_item.len = ECRYPTFS_AES_KEY_BYTES;
	sym_key = PK11_ImportSymKey(slot, CKM_AES_ECB, PK11_OriginUnwrap,
				    CKA_ENCRYPT, &key_item, NULL);
	if (!sym_key) {
		syslog(LOG_ERR, "%s: PK11_ImportSymKey() returned NULL\n",
		       __FUNCTION__);
		rc = -EIO;
		goto out;
	}
	sec_param = PK11_ParamFromIV(CKM_AES_ECB, NULL);
	enc_ctx = PK11_CreateContextBySymKey(CKM_AES_ECB, CKA_ENCRYPT,
					     sym_key, sec_param);
	err = PK11_CipherOp(
		enc_ctx, (unsigned char *) encrypted_passphrase,
		&tmp1_outlen, ECRYPTFS_MAX_PASSPHRASE_BYTES +
		  ECRYPTFS_AES_BLOCK_SIZE,
		(unsigned char *) padded_decrypted_passphrase,
		decrypted_passphrase_bytes);//ECRYPTFS_MAX_PASSPHRASE_BYTES);
	if (err == SECFailure) {
		syslog(LOG_ERR, "%s: PK11_CipherOp() error; "
			"SECFailure = [%d]; PORT_GetError() = [%d]\n",
			__FUNCTION__, SECFailure, PORT_GetError());
		rc = - EIO;
		goto nss_finish;
	}
	err = PK11_DigestFinal(
		enc_ctx, 
		(unsigned char *) encrypted_passphrase + tmp1_outlen,
		(unsigned int *) &tmp2_outlen,
		(ECRYPTFS_MAX_PASSPHRASE_BYTES + 
		  ECRYPTFS_AES_BLOCK_SIZE - tmp1_outlen));
	if (err == SECFailure) {
		syslog(LOG_ERR, "%s: PK11 error on digest final; "
			"SECFailure = [%d]; PORT_GetError() = [%d]\n",
			__FUNCTION__, SECFailure, PORT_GetError());
		rc = - EIO;
	}

nss_finish:
	if (enc_ctx)
		PK11_DestroyContext(enc_ctx, PR_TRUE);
	if (sym_key)
		PK11_FreeSymKey(sym_key);
	if (sec_param)
		SECITEM_FreeItem(sec_param, PR_TRUE);
	if (slot)
		PK11_FreeSlot(slot);
	if (rc)
		goto out;
	encrypted_passphrase_pos += tmp1_outlen + tmp2_outlen;
	decrypted_passphrase_pos += tmp1_outlen + tmp2_outlen;
	decrypted_passphrase_bytes -= tmp1_outlen + tmp2_outlen;
	if (decrypted_passphrase_bytes != 0) {
		syslog(LOG_ERR, "Wrong size of wrapped passphrase\n");
		rc = - EIO;
		goto out;
	}
#else
       if ((gcry_err = gcry_cipher_open(&gcry_handle, GCRY_CIPHER_AES,
                                        GCRY_CIPHER_MODE_ECB, 0))) {
               syslog(LOG_ERR, "Error attempting to initialize AES cipher; "
                      "gcry_error_t = [%d]\n", gcry_err);
               rc = -EIO;
               goto out;
       }
       if ((gcry_err = gcry_cipher_setkey(gcry_handle, wrapping_key,
                                          ECRYPTFS_AES_KEY_BYTES))) {
               syslog(LOG_ERR, "Error attempting to set AES key; "
                      "gcry_error_t = [%d]\n", gcry_err);
               rc = -EIO;
               gcry_cipher_close(gcry_handle);
               goto out;
       }
       while (decrypted_passphrase_bytes > 0) {
               if ((gcry_err = gcry_cipher_encrypt(
                            gcry_handle,
                            &encrypted_passphrase[encrypted_passphrase_pos],
                            ECRYPTFS_AES_BLOCK_SIZE,
                            &decrypted_passphrase[decrypted_passphrase_pos],
                            ECRYPTFS_AES_BLOCK_SIZE))) {
                       syslog(LOG_ERR, "Error attempting to encrypt block; "
                              "gcry_error = [%d]\n", gcry_err);
                       rc = -EIO;
                       gcry_cipher_close(gcry_handle);
                       goto out;
               }
               encrypted_passphrase_pos += ECRYPTFS_AES_BLOCK_SIZE;
               decrypted_passphrase_pos += ECRYPTFS_AES_BLOCK_SIZE;
               decrypted_passphrase_bytes -= ECRYPTFS_AES_BLOCK_SIZE;
       }
       gcry_cipher_close(gcry_handle);
#endif /* #ifdef ENABLE_NSS */
	rc = write_v2_wrapped_passphrase_file(filename, wrapping_salt,
					      wrapping_auth_tok_sig,
					      encrypted_passphrase,
					      encrypted_passphrase_bytes);
	if (rc)
		goto out;
	rc = 0;
out:
	return rc;
}

/**
 * read_v1_wrapped_passphrase_file - Reads a v1 wrapped passphrase file
 * @filename: Path to the wrapped passphrase file
 * @wrapping_key_sig: Will contain the parsed wrapping key sig upon success.
 *  MUST be zeroed prior to calling this function.
 * @encrypted_passphrase: Will contain the parsed encrypted passphrase upon
 *  success. MUST be zeroed prior to calling this function.
 * @encrypted_passphrase_bytes: Will contain the size of the parsed encrypted
 *  passphrase upon success
 *
 * Reads a version 1 wrapped passphrase file containing the following format:
 *
 *   Octets 0-15:   Signature of wrapping key
 *   Octets 16-79:  Variable length field containing the encrypted
 *                  passphrase.
 *
 * Returns 0 upon success with the size of the encrypted passphrase returned in
 * *encrypted_passphrase_bytes. Returns negative upon failure.
 */
static int read_v1_wrapped_passphrase_file(const char *filename,
				char wrapping_key_sig[ECRYPTFS_SIG_SIZE_HEX],
				char encrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES],
				int *encrypted_passphrase_bytes)
{
	ssize_t size;
	int fd;
	int rc;

	*encrypted_passphrase_bytes = 0;

	if ((fd = open(filename, O_RDONLY)) == -1) {
		syslog(LOG_ERR, "Error attempting to open [%s] for reading\n",
		       filename);
		rc = -EIO;
		goto out;
	}

	if ((size = do_read(fd, wrapping_key_sig,
			    ECRYPTFS_SIG_SIZE_HEX)) < ECRYPTFS_SIG_SIZE_HEX) {
		syslog(LOG_ERR,
		       "Error attempting to read encrypted passphrase from file [%s]; size = [%zu]\n",
		       filename, size);
		rc = -EIO;
		goto out;
	}

	if ((size = do_read(fd, encrypted_passphrase,
			    ECRYPTFS_MAX_PASSPHRASE_BYTES)) <= 0) {
		syslog(LOG_ERR,
		       "Error attempting to read encrypted passphrase from file [%s]; size = [%zu]\n",
		       filename, size);
		rc = -EIO;
		goto out;
	}

	*encrypted_passphrase_bytes = size;
	rc = 0;
out:
	if (fd != -1)
		close(fd);

	return 0;
}

/**
 * read_v2_wrapped_passphrase_file - Reads a v2 wrapped passphrase file
 * @filename: Path to the wrapped passphrase file
 * @wrapping_salt: Will contain the parsed wrapping salt upon success. MUST be
 *  zeroed prior to calling this function.
 * @wrapping_key_sig: Will contain the parsed wrapping key sig upon success.
 *  MUST be zeroed prior to calling this function.
 * @encrypted_passphrase: Will contain the parsed encrypted passphrase upon
 *  success. MUST be zeroed prior to calling this function.
 * @encrypted_passphrase_bytes: Will contain the size of the parsed encrypted
 *  passphrase upon success
 *
 * Reads a version 2 wrapped passphrase file containing the following format:
 *
 *   Octet  0:      A ':' character
 *   Octet  1:      uint8_t value indicating file version (MUST be 0x02)
 *   Octets 2-9:    Wrapping salt
 *   Octets 10-25:  Signature of wrapping key (16 octets)
 *   Octets 26-N1:  Variable length field containing the encrypted
 *                  passphrase. (Up to 64 octets. Must be non-empty.)
 *
 * Returns 0 upon success with the size of the encrypted passphrase returned in
 * *encrypted_passphrase_bytes. Returns negative upon failure.
 */
static int read_v2_wrapped_passphrase_file(const char *filename,
				char wrapping_salt[ECRYPTFS_SALT_SIZE],
				char wrapping_key_sig[ECRYPTFS_SIG_SIZE_HEX],
				char encrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES],
				int *encrypted_passphrase_bytes)
{
	uint8_t version = 0;
	uint8_t salt_len = 0;
	ssize_t size;
	int fd = -1;
	int rc;

	*encrypted_passphrase_bytes = 0;

	if ((fd = open(filename, O_RDONLY)) == -1) {
		rc = -errno;
		goto out;
	}

	/* Parse file version (must be 2) */
	rc = read_wrapped_passphrase_file_version(fd, &version);
	if (rc != 0) {
		goto out;
	} else if (version != 2) {
		rc = -EINVAL;
		goto out;
	}

	/* Parse the wrapping salt */
	size = do_read(fd, wrapping_salt, ECRYPTFS_SALT_SIZE);
	if (size != ECRYPTFS_SALT_SIZE) {
		rc = size == -1 ? errno : -EINVAL;
		goto out;
	}

	/* Parse the wrapping key signature */
	size = do_read(fd, wrapping_key_sig, ECRYPTFS_SIG_SIZE_HEX);
	if (size != ECRYPTFS_SIG_SIZE_HEX) {
		rc = size == -1 ? errno : -EINVAL;
		goto out;
	}

	/* Parse the encrypted passphrase */
	size = do_read(fd, encrypted_passphrase, ECRYPTFS_MAX_PASSPHRASE_BYTES);
	if (size < 0) {
		rc = size;
		goto out;
	} else if(size == 0) {
		rc = -EINVAL;
		goto out;
	}

	*encrypted_passphrase_bytes = size;
	rc = 0;
out:
	if (fd != -1)
		close(fd);

	return rc;
}

/**
 * decryptfs_passphrase must be able to hold
 * ECRYPTFS_MAX_PASSPHRASE_BYTES + 1 bytes
 */
int ecryptfs_unwrap_passphrase(char *decrypted_passphrase, char *filename,
			       char *wrapping_passphrase, char *wrapping_salt)
{
	char v2_wrapping_salt[ECRYPTFS_SALT_SIZE];
	char wrapping_auth_tok_sig[ECRYPTFS_SIG_SIZE_HEX + 1];
	char wrapping_auth_tok_sig_from_file[ECRYPTFS_SIG_SIZE_HEX + 1];
	char wrapping_key[ECRYPTFS_MAX_KEY_BYTES];
	char encrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES + 1];
	int encrypted_passphrase_pos = 0;
	int decrypted_passphrase_pos = 0;
#ifdef ENABLE_NSS
	int tmp1_outlen = 0;
	int tmp2_outlen = 0;
	SECStatus err;
	SECItem key_item;
	PK11SymKey *sym_key = NULL;
	PK11SlotInfo *slot = NULL;
	PK11Context *enc_ctx = NULL;
	SECItem *sec_param = NULL;
#else
       gcry_cipher_hd_t gcry_handle;
       gcry_error_t gcry_err;
#endif /* #ifdef ENABLE_NSS */
	uint8_t version = 0;
	int encrypted_passphrase_bytes;
	int rc;

	memset(wrapping_auth_tok_sig_from_file, 0,
	       sizeof(wrapping_auth_tok_sig_from_file));
	memset(encrypted_passphrase, 0, sizeof(encrypted_passphrase));

	rc = __ecryptfs_detect_wrapped_passphrase_file_version(filename,
							       &version);
	if (rc) {
		syslog(LOG_ERR,
		       "Failed to detect wrapped passphrase version: %s\n",
		       strerror(-rc));
		goto out;
	}

	if (version == 1) {
		rc = read_v1_wrapped_passphrase_file(filename,
						wrapping_auth_tok_sig_from_file,
						encrypted_passphrase,
						&encrypted_passphrase_bytes);
		if (rc)
			goto out;
	} else if (version == 2) {
		rc = read_v2_wrapped_passphrase_file(filename,
						v2_wrapping_salt,
						wrapping_auth_tok_sig_from_file,
						encrypted_passphrase,
						&encrypted_passphrase_bytes);
		if (rc)
			goto out;

		/**
		 * Version 2 wrapped passphrase self-contains the wrapping salt.
		 * The passed in @wrapping_salt buffer is ignored and the
		 * parsed wrapping salt is used instead.
		 */
		wrapping_salt = v2_wrapping_salt;
	} else {
		syslog(LOG_ERR,
		       "Unsupported wrapped passphrase file version: %u\n",
		       version);
		rc = -ENOTSUP;
		goto out;
	}

	rc = generate_passphrase_sig(wrapping_auth_tok_sig, wrapping_key,
				     wrapping_salt, wrapping_passphrase);
	if (rc) {
		syslog(LOG_ERR, "Error generating passphrase signature; "
		       "rc = [%d]\n", rc);
		rc = (rc < 0) ? rc : rc * -1;
		goto out;
	}

	if (memcmp(wrapping_auth_tok_sig_from_file, wrapping_auth_tok_sig,
		   ECRYPTFS_SIG_SIZE_HEX) != 0) {
		syslog(LOG_ERR, "Incorrect wrapping key for file [%s]\n",
		       filename);
		rc = -EIO;
		goto out;
	}
#ifdef ENABLE_NSS
	NSS_NoDB_Init(NULL);
	slot = PK11_GetBestSlot(CKM_AES_ECB, NULL);
	key_item.data = (unsigned char *)wrapping_key;
	key_item.len = ECRYPTFS_AES_KEY_BYTES;
	sym_key = PK11_ImportSymKey(slot, CKM_AES_ECB, PK11_OriginUnwrap,
				    CKA_ENCRYPT, &key_item, NULL);
	if (!sym_key) {
		syslog(LOG_ERR, "%s: PK11_ImportSymKey() returned NULL\n",
		       __FUNCTION__);
		rc = -EIO;
		goto out;
	}
	sec_param = PK11_ParamFromIV(CKM_AES_ECB, NULL);
	enc_ctx = PK11_CreateContextBySymKey(CKM_AES_ECB, CKA_DECRYPT,
					     sym_key, sec_param);
	memset(decrypted_passphrase, 0, ECRYPTFS_MAX_PASSPHRASE_BYTES + 1);
	err = PK11_CipherOp(
		enc_ctx, (unsigned char *) decrypted_passphrase,
		&tmp1_outlen, ECRYPTFS_MAX_PASSPHRASE_BYTES,
		(unsigned char *) encrypted_passphrase,
		encrypted_passphrase_bytes);//ECRYPTFS_MAX_PASSPHRASE_BYTES);
	if (err == SECFailure) {
		syslog(LOG_ERR, "%s: PK11_CipherOp() error; "
			"SECFailure = [%d]; PORT_GetError() = [%d]\n",
			__FUNCTION__, SECFailure, PORT_GetError());
		rc = - EIO;
		goto nss_finish;
	}
	err = PK11_DigestFinal(
		enc_ctx, 
		(unsigned char *) decrypted_passphrase + tmp1_outlen,
		(unsigned int *) &tmp2_outlen,
		(ECRYPTFS_MAX_PASSPHRASE_BYTES - tmp1_outlen));
	if (err == SECFailure) {
		syslog(LOG_ERR, "%s: PK11 error on digest final; "
			"SECFailure = [%d]; PORT_GetError() = [%d]\n",
			__FUNCTION__, SECFailure, PORT_GetError());
		rc = - EIO;
	}

nss_finish:
	if (enc_ctx)
		PK11_DestroyContext(enc_ctx, PR_TRUE);
	if (sym_key)
		PK11_FreeSymKey(sym_key);
	if (sec_param)
		SECITEM_FreeItem(sec_param, PR_TRUE);
	if (slot)
		PK11_FreeSlot(slot);
	if (rc)
		goto out;
	encrypted_passphrase_pos += tmp1_outlen + tmp2_outlen;
	decrypted_passphrase_pos += tmp1_outlen + tmp2_outlen;
	encrypted_passphrase_bytes -= tmp1_outlen + tmp2_outlen;
	if (encrypted_passphrase_bytes != 0) {
		syslog(LOG_ERR, "Wrong size of unwrapped passphrase\n");
		rc = - EIO;
		goto out;
	}
#else
       if ((gcry_err = gcry_cipher_open(&gcry_handle, GCRY_CIPHER_AES,
                                        GCRY_CIPHER_MODE_ECB, 0))) {
               syslog(LOG_ERR, "Error attempting to initialize AES cipher; "
                      "gcry_error_t = [%d]\n", gcry_err);
               rc = -EIO;
               goto out;
       }
       if ((gcry_err = gcry_cipher_setkey(gcry_handle, wrapping_key,
                                          ECRYPTFS_AES_KEY_BYTES))) {
               syslog(LOG_ERR, "Error attempting to set AES key; "
                      "gcry_error_t = [%d]\n", gcry_err);
               rc = -EIO;
               gcry_cipher_close(gcry_handle);
               goto out;
       }
       memset(decrypted_passphrase, 0, ECRYPTFS_MAX_PASSPHRASE_BYTES + 1);
       while (encrypted_passphrase_bytes > 0) {
               if ((gcry_err = gcry_cipher_decrypt(
                            gcry_handle,
                            &decrypted_passphrase[encrypted_passphrase_pos],
                            ECRYPTFS_AES_BLOCK_SIZE,
                            &encrypted_passphrase[decrypted_passphrase_pos],
                            ECRYPTFS_AES_BLOCK_SIZE))) {
                       syslog(LOG_ERR, "Error attempting to decrypt block; "
                              "gcry_error = [%d]\n", gcry_err);
                       rc = -EIO;
                       gcry_cipher_close(gcry_handle);
                       goto out;
               }
               encrypted_passphrase_pos += ECRYPTFS_AES_BLOCK_SIZE;
               decrypted_passphrase_pos += ECRYPTFS_AES_BLOCK_SIZE;
               encrypted_passphrase_bytes -= ECRYPTFS_AES_BLOCK_SIZE;
       }
#endif /* #ifdef ENABLE_NSS */
out:
	return rc;
}

/**
 * ecryptfs_insert_wrapped_passphrase_into_keyring()
 *
 * Inserts two auth_tok objects into the user session keyring: a
 * wrapping passphrase auth_tok and the unwrapped passphrase auth_tok.
 *
 * Returns the signature of the wrapped passphrase that is inserted
 * into the user session keyring.
 */
int ecryptfs_insert_wrapped_passphrase_into_keyring(
	char *auth_tok_sig, char *filename, char *wrapping_passphrase,
	char *salt)
{
	char decrypted_passphrase[ECRYPTFS_MAX_PASSPHRASE_BYTES + 1] ;
	int rc = 0;

	if ((rc = ecryptfs_unwrap_passphrase(decrypted_passphrase, filename,
					     wrapping_passphrase, salt))) {
		syslog(LOG_ERR, "Error attempting to unwrap passphrase from "
		       "file [%s]; rc = [%d]\n", filename, rc);
		rc = -EIO;
		goto out;
	}
	if ((rc = ecryptfs_add_passphrase_key_to_keyring(auth_tok_sig,
					decrypted_passphrase,
					ECRYPTFS_DEFAULT_SALT_FNEK_HEX)) < 0) {
		syslog(LOG_ERR, "Error attempting to add filename encryption "
		       "key to user session keyring; rc = [%d]\n", rc);
		goto out;
	}
	if ((rc = ecryptfs_add_passphrase_key_to_keyring(auth_tok_sig,
							 decrypted_passphrase,
							 salt)) < 0) {
		syslog(LOG_ERR, "Error attempting to add passphrase key to "
		       "user session keyring; rc = [%d]\n", rc);
	}
out:
	return rc;
}

/**
 * ecryptfs_add_key_module_key_to_keyring
 * @auth_tok_sig: (ECRYPTFS_SIG_SIZE_HEX + 1) bytes of allocated
 *                memory into which this function will write the
 *                expanded-hex key signature for the given key
 *                module
 * @key_mod: Key module handle
 *
 * Inserts a key module key blob into the keyring, using the
 * auth_tok_sig as the key signature.
 *
 * Returns =0 on successful addition, =1 if the key is already in the
 * keyring, and <0 on failure.
 */
int
ecryptfs_add_key_module_key_to_keyring(char *auth_tok_sig,
				       struct ecryptfs_key_mod *key_mod)
{
	size_t blob_size;
	struct ecryptfs_auth_tok *auth_tok = NULL;
	int rc;

	if (key_mod->blob == NULL) {
		if ((rc = (key_mod->ops->get_blob)(NULL, &blob_size,
						   key_mod->param_vals,
						   key_mod->num_param_vals))) {
			syslog(LOG_ERR, "Error attempting to get blob from "
			       "key module; rc = [%d]\n", rc);
			goto out;
		}
	} else {
		blob_size = key_mod->blob_size;
	}
	if ((auth_tok = malloc(sizeof(struct ecryptfs_auth_tok) + blob_size))
	    == NULL) {
		rc = -ENOMEM;
		goto out;
	}
	if ((rc = ecryptfs_generate_key_payload(auth_tok, key_mod, auth_tok_sig,
						blob_size))) {
		syslog(LOG_ERR, "Error initializing key from module; "
		       "rc = [%d]\n", rc);
		goto out;
	}
	rc = (int)keyctl_search(KEY_SPEC_USER_KEYRING, "user", auth_tok_sig, 0);
	if (rc != -1) { /* we already have this key in keyring; we're done */
		rc = 1;
		goto out;
	}
	rc = add_key("user", auth_tok_sig, (void *)auth_tok,
		     (sizeof(struct ecryptfs_auth_tok) + blob_size),
		     KEY_SPEC_USER_KEYRING);
	if (rc < 0) {
		rc = -errno;
		syslog(LOG_ERR, "Error adding key with sig [%s]; rc ="
		       " [%d]\n", auth_tok_sig, rc);
		if (rc == -EDQUOT)
			syslog(LOG_WARNING, "Error adding key to keyring - keyring is full\n");
	} else rc = 0;
out:
	if (auth_tok != NULL) {
		memset(auth_tok, 0, (sizeof(struct ecryptfs_auth_tok) + blob_size));
		free(auth_tok);
	}
	return rc;
}

int ecryptfs_read_salt_hex_from_rc(char *salt_hex)
{
	struct ecryptfs_name_val_pair nvp_list_head;
	struct ecryptfs_name_val_pair *nvp;
	int rc;

	memset(&nvp_list_head, 0, sizeof(struct ecryptfs_name_val_pair));
	rc = ecryptfs_parse_rc_file(&nvp_list_head);
	if (rc) {
		if (rc != -ENOENT && rc != -EACCES) {
			syslog(LOG_WARNING,
				"Error attempting to parse .ecryptfsrc file; "
				"rc = [%d]", rc);
		}
		goto out;
	}
	nvp = nvp_list_head.next;
	while (nvp) {
		if (strcmp(nvp->name, "salt") == 0) {
			int valsize;

			if (!nvp->value)
				goto next_iteration;
			valsize = strlen(nvp->value);
			if (valsize != ECRYPTFS_SALT_SIZE_HEX)
				goto next_iteration;
			memcpy(salt_hex, nvp->value, ECRYPTFS_SALT_SIZE_HEX);
			goto out_free;
		}
next_iteration:
		nvp = nvp->next;
	}
	rc = -EINVAL;
out_free:
	free_name_val_pairs(nvp_list_head.next);
out:
	return rc;
}

int ecryptfs_check_sig(char *auth_tok_sig, char *sig_cache_filename,
		       int *flags)
{
	int fd;
	char tmp[ECRYPTFS_SIG_SIZE_HEX + 1];
	ssize_t size;
	int rc = 0;

	memset(tmp, 0, sizeof(tmp));
	(*flags) &= ~ECRYPTFS_SIG_FLAG_NOENT;
	fd = open(sig_cache_filename, O_RDONLY);
	if (fd == -1) {
		(*flags) |= ECRYPTFS_SIG_FLAG_NOENT;
		goto out;
	}
	while ((size = read(fd, tmp, (ECRYPTFS_SIG_SIZE_HEX + 1)))
	       == (ECRYPTFS_SIG_SIZE_HEX + 1)) {
		if (memcmp(auth_tok_sig, tmp, ECRYPTFS_SIG_SIZE_HEX)
		    == 0) {
			close(fd);
			goto out;
		}
		memset(tmp, 0, sizeof(tmp));
	}
	close(fd);
	(*flags) |= ECRYPTFS_SIG_FLAG_NOENT;
out:
	return rc;
}

int ecryptfs_append_sig(char *auth_tok_sig, char *sig_cache_filename)
{
	int fd;
	ssize_t size;
	char tmp[ECRYPTFS_SIG_SIZE_HEX + 1];
	int rc = 0;

	fd = open(sig_cache_filename, (O_WRONLY | O_CREAT),
		  (S_IRUSR | S_IWUSR));
	if (fd == -1) {
		syslog(LOG_ERR, "Open resulted in [%d]; [%m]\n", errno);
		rc = -EIO;
		goto out;
	}
	if (fchown(fd, getuid(), getgid()) == -1) {
		syslog(LOG_WARNING, "Can't change ownership of sig file; "
				    "errno = [%d]; [%m]\n", errno);
	}
	lseek(fd, 0, SEEK_END);
	memcpy(tmp, auth_tok_sig, ECRYPTFS_SIG_SIZE_HEX);
	tmp[ECRYPTFS_SIG_SIZE_HEX] = '\n';
	if ((size = write(fd, tmp, (ECRYPTFS_SIG_SIZE_HEX + 1))) !=
	    (ECRYPTFS_SIG_SIZE_HEX + 1)) {
		syslog(LOG_ERR, "Write of sig resulted in [%zu]; errno = [%d]; "
		       "[%m]\n", size, errno);
		rc = -EIO;
		close(fd);
		goto out;
	}
	close(fd);
out:
	return rc;
}

int ecryptfs_validate_keyring(void)
{
	long rc_long;
	int rc = 0;

	if ((rc_long = keyctl(KEYCTL_LINK, KEY_SPEC_USER_KEYRING,
			      KEY_SPEC_SESSION_KEYRING))) {
		syslog(LOG_ERR, "Error attempting to link the user session "
		       "keyring into the session keyring\n");
		rc = -EIO;
		goto out;
	}
out:
	return rc;
}

int ecryptfs_disable_echo(struct termios *saved_settings)
{
	struct termios current_settings;
	int rc = 0;

	rc = tcgetattr(0, &current_settings);
	if (rc)
		return rc;
	*saved_settings = current_settings;
	current_settings.c_lflag &= ~ECHO;
	rc = tcsetattr(0, TCSANOW, &current_settings);
	return rc;
}

int ecryptfs_enable_echo(struct termios *saved_settings)
{
	return tcsetattr(0, TCSANOW, saved_settings);
}

char *ecryptfs_get_passphrase(char *prompt) {
	char *passphrase = NULL;
	char *p;
	struct termios current_settings;

	if ((passphrase =
	    (char *)malloc(ECRYPTFS_MAX_PASSWORD_LENGTH+2)) == NULL) {
		perror("malloc");
		printf("\n");
		return NULL;
	}
	if (prompt != NULL) {
		printf("%s: ", prompt);
	}
	ecryptfs_disable_echo(&current_settings);
	if (fgets(passphrase,
		  ECRYPTFS_MAX_PASSWORD_LENGTH+2, stdin) == NULL) {
		ecryptfs_enable_echo(&current_settings);
		printf("\n");
		free(passphrase);
		return NULL;
	}
	ecryptfs_enable_echo(&current_settings);
	p = strrchr(passphrase, '\n');
	if (p) *p = '\0';
	if (prompt != NULL)
		printf("\n");
	if (strlen(passphrase) > ECRYPTFS_MAX_PASSWORD_LENGTH) {
		fprintf(stderr,"Passphrase is too long. Use at most %u "
			       "characters long passphrase.\n",
			ECRYPTFS_MAX_PASSWORD_LENGTH);
		free(passphrase);
		return NULL;
	}
	return passphrase;
}

char *ecryptfs_get_wrapped_passphrase_filename() {
	struct passwd *pwd = NULL;
	struct stat s;
	char *filename = NULL;
	if ((pwd = getpwuid(getuid())) == NULL) {
		perror("getpwuid");
		return NULL;
	}
	if ((asprintf(&filename,
	    "%s/.ecryptfs/wrapped-passphrase", pwd->pw_dir) < 0)) {
		perror("asprintf");
		return NULL;
	}
	if (stat(filename, &s) != 0) {
		perror("stat");
		return NULL;
	}
	return filename;
}
