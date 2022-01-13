/********************************************************************************/
/*			     	TPM HMAC routines				*/
/*			     Written by J. Kravitz				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: hmac.c 4073 2010-04-30 14:44:14Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmutil.h>
#include <tpm_error.h>

#include <hmac.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "debug.h"

#define TPM_TAG_RSP_COMMAND       0x00C4
#define TPM_TAG_RSP_AUTH1_COMMAND 0x00C5
#define TPM_TAG_RSP_AUTH2_COMMAND 0x00C6

/****************************************************************************/
/*                                                                          */
/* Validate the HMAC in an AUTH1 response                                   */
/*                                                                          */
/* This function validates the Authorization Digest for all AUTH1           */
/* responses.                                                               */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* buffer - a pointer to response buffer                                    */
/* command - the command code from the original request                     */
/* ononce - a pointer to a 20 byte array containing the oddNonce            */
/* key    - a pointer to the key used in the request HMAC                   */
/* keylen - the size of the key                                             */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is an offset in the buffer to the data     */
/*   to be included in the hash for the paramdigest                         */
/* There should NOT be pairs for the TPM_RESULT or TPM_COMMAND_CODE         */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/
uint32_t TSS_checkhmac1(const struct tpm_buffer *tb, uint32_t command,
			unsigned char *ononce, unsigned char *key,
			unsigned int keylen, ...)
{
    uint32_t bufsize;
    uint16_t tag;
    uint32_t ordinal;
    uint32_t result;
    const unsigned char *enonce;
    const unsigned char *continueflag;
    const unsigned char *authdata;
    unsigned char testhmac[20];
    unsigned char paramdigest[20];
    SHA_CTX sha;
    unsigned int dlen;
    unsigned int dpos;
    va_list argp;
    const unsigned char *buffer = tb->buffer;
    uint32_t ret;

    va_start(argp, keylen);
    ret = tpm_buffer_load32(tb, TPM_U16_SIZE, &bufsize);
    if (ret & ERR_MASK)
	goto exit;

    tag = LOAD16(buffer, 0);
    ordinal = command;
    ret = tpm_buffer_load32N(tb, TPM_RETURN_OFFSET, &result);
    if (ret & ERR_MASK)
	goto exit;

    if (tag == TPM_TAG_RSP_COMMAND) {
	ret = 0;
	goto exit;
    }
    if (tag != TPM_TAG_RSP_AUTH1_COMMAND) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    authdata = buffer + bufsize - TPM_HASH_SIZE;
    continueflag = authdata - 1;
    enonce = continueflag - TPM_NONCE_SIZE;
    ret = (SHA1_Init(&sha) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &result, 4) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &ordinal, 4) ? 0 : TPM_SHA_ERROR);
    for (;;) {
	dlen = (unsigned int) va_arg(argp, unsigned int);
	if (dlen == 0)
	    break;
	dpos = (unsigned int) va_arg(argp, unsigned int);
	if (dpos + dlen > tb->used) {
	    ret = ERR_BUFFER;
	    goto exit;
	}
	ret = (SHA1_Update(&sha, buffer + dpos, dlen) ? 0 : TPM_SHA_ERROR);
    }
    ret = (SHA1_Final(paramdigest, &sha) ? 0 : TPM_SHA_ERROR);
    TSS_rawhmac(testhmac, key, keylen, TPM_HASH_SIZE, paramdigest,
		TPM_NONCE_SIZE, enonce,
		TPM_NONCE_SIZE, ononce, 1, continueflag, 0, 0);
    if (memcmp(testhmac, authdata, TPM_HASH_SIZE) != 0) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    ret = 0;

exit:
    va_end(argp);
    return ret;
}

uint32_t TSS_checkhmac1New(const struct tpm_buffer * tb, uint32_t command,
			   session * sess, unsigned char *ononce,
			   unsigned char *key, unsigned int keylen, ...)
{
    uint32_t bufsize;
    uint16_t tag;
    uint32_t ordinal;
    uint32_t result;
    const unsigned char *enonce;
    const unsigned char *continueflag;
    const unsigned char *authdata;
    unsigned char testhmac[20];
    unsigned char paramdigest[20];
    SHA_CTX sha;
    unsigned int dlen;
    unsigned int dpos;
    va_list argp;
    const unsigned char *buffer = tb->buffer;
    uint32_t ret;

    va_start(argp, keylen);
    ret = tpm_buffer_load32(tb, TPM_U16_SIZE, &bufsize);
    if (ret & ERR_MASK)
	goto exit;

    tag = LOAD16(buffer, 0);
    ordinal = command;
    ret = tpm_buffer_load32N(tb, TPM_RETURN_OFFSET, &result);
    if (ret & ERR_MASK)
	goto exit;

    if (tag == TPM_TAG_RSP_COMMAND) {
	ret = 0;
	goto exit;
    }
    if (tag != TPM_TAG_RSP_AUTH1_COMMAND) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    authdata = buffer + bufsize - TPM_HASH_SIZE;
    continueflag = authdata - 1;
    enonce = continueflag - TPM_NONCE_SIZE;
    ret = (SHA1_Init(&sha) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &result, 4) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &ordinal, 4) ? 0 : TPM_SHA_ERROR);
    for (;;) {
	dlen = (unsigned int) va_arg(argp, unsigned int);
	if (dlen == 0)
	    break;
	dpos = (unsigned int) va_arg(argp, unsigned int);
	ret = (SHA1_Update(&sha, buffer + dpos, dlen) ? 0 : TPM_SHA_ERROR);
    }
    ret = (SHA1_Final(paramdigest, &sha) ? 0 : TPM_SHA_ERROR);
    TSS_rawhmac(testhmac, key, keylen, TPM_HASH_SIZE, paramdigest,
		TPM_NONCE_SIZE, enonce,
		TPM_NONCE_SIZE, ononce, 1, continueflag, 0, 0);
    if (memcmp(testhmac, authdata, TPM_HASH_SIZE) != 0) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    TSS_Session_SetENonce(sess, enonce);
    ret = 0;

exit:
    va_end(argp);
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Validate the HMACS in an AUTH2 response                                  */
/*                                                                          */
/* This function validates the Authorization Digests for all AUTH2          */
/* responses.                                                               */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* buffer - a pointer to response buffer                                    */
/* command - the command code from the original request                     */
/* ononce - a pointer to a 20 byte array containing the oddNonce            */
/* key1    - a pointer to the 1st   key used in the request HMAC            */
/* keylen1 - the size of the key                                            */
/* key2    - a pointer to the 2nd   key used in the request HMAC            */
/* keylen2 - the size of the key                                            */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is an offset in the buffer to the data     */
/*   to be included in the hash for the paramdigest                         */
/* There should NOT be pairs for the TPM_RESULT or TPM_COMMAND_CODE         */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/
uint32_t TSS_checkhmac2(const struct tpm_buffer * tb, uint32_t command,
			unsigned char *ononce1,
			unsigned char *key1, unsigned int keylen1,
			unsigned char *ononce2,
			unsigned char *key2, unsigned int keylen2, ...)
{
    uint32_t bufsize;
    uint16_t tag;
    uint32_t ordinal;
    uint32_t result;
    const unsigned char *enonce1;
    const unsigned char *continueflag1;
    const unsigned char *authdata1;
    const unsigned char *enonce2;
    const unsigned char *continueflag2;
    const unsigned char *authdata2;
    unsigned char testhmac1[20];
    unsigned char paramdigest[20];
    unsigned char testhmac2[20];
    SHA_CTX sha;
    unsigned int dlen;
    unsigned int dpos;
    va_list argp;
    const unsigned char *buffer = tb->buffer;
    uint32_t ret;

    va_start(argp, keylen2);
    ret = tpm_buffer_load32(tb, TPM_U16_SIZE, &bufsize);
    if (ret & ERR_MASK)
	goto exit;

    tag = LOAD16(buffer, 0);
    ordinal = command;
    ret = tpm_buffer_load32N(tb, TPM_RETURN_OFFSET, &result);
    if (ret & ERR_MASK)
	goto exit;

    if (tag == TPM_TAG_RSP_COMMAND) {
	ret = 0;
	goto exit;
    }
    if (tag != TPM_TAG_RSP_AUTH2_COMMAND) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    authdata1 =
	buffer + bufsize - (TPM_HASH_SIZE + 1 + TPM_HASH_SIZE + TPM_HASH_SIZE);
    authdata2 = buffer + bufsize - (TPM_HASH_SIZE);
    continueflag1 = authdata1 - 1;
    continueflag2 = authdata2 - 1;
    enonce1 = continueflag1 - TPM_NONCE_SIZE;
    enonce2 = continueflag2 - TPM_NONCE_SIZE;
    ret = (SHA1_Init(&sha) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &result, 4) ? 0 : TPM_SHA_ERROR);
    ret = (SHA1_Update(&sha, &ordinal, 4) ? 0 : TPM_SHA_ERROR);
    for (;;) {
	dlen = (unsigned int) va_arg(argp, unsigned int);
	if (dlen == 0)
	    break;
	dpos = (unsigned int) va_arg(argp, unsigned int);
	if (dpos + dlen > tb->used) {
	    ret = ERR_BUFFER;
	    goto exit;
	}
	ret = (SHA1_Update(&sha, buffer + dpos, dlen) ? 0 : TPM_SHA_ERROR);
    }
    ret = (SHA1_Final(paramdigest, &sha) ? 0 : TPM_SHA_ERROR);
    TSS_rawhmac(testhmac1, key1, keylen1, TPM_HASH_SIZE, paramdigest,
		TPM_NONCE_SIZE, enonce1,
		TPM_NONCE_SIZE, ononce1, 1, continueflag1, 0, 0);
    TSS_rawhmac(testhmac2, key2, keylen2, TPM_HASH_SIZE, paramdigest,
		TPM_NONCE_SIZE, enonce2,
		TPM_NONCE_SIZE, ononce2, 1, continueflag2, 0, 0);
    if (memcmp(testhmac1, authdata1, TPM_HASH_SIZE) != 0) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    if (memcmp(testhmac2, authdata2, TPM_HASH_SIZE) != 0) {
	ret = ERR_HMAC_FAIL;
	goto exit;
    }
    ret = 0;

exit:
    va_end(argp);
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Calculate HMAC value for an AUTH1 command                                */
/*                                                                          */
/* This function calculates the Authorization Digest for all OIAP           */
/* commands.                                                                */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* digest - a pointer to a 20 byte array that will receive the result       */
/* key    - a pointer to the key to be used in the HMAC calculation         */
/* keylen - the size of the key in bytes                                    */
/* h1     - a pointer to a 20 byte array containing the evenNonce           */
/* h2     - a pointer to a 20 byte array containing the oddNonce            */
/* h3     - an unsigned character containing the continueAuthSession value  */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is a pointer to the data to be hashed      */
/*   into the paramdigest.                                                  */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/
uint32_t TSS_authhmac(unsigned char *digest, unsigned char *key,
		      unsigned int keylen, unsigned char *h1,
		      unsigned char *h2, unsigned char h3, ...)
{
    unsigned char paramdigest[TPM_HASH_SIZE];
    SHA_CTX sha;
    unsigned int dlen;
    unsigned char *data;
    unsigned char c;
    uint32_t ret;

    va_list argp;

    va_start(argp, h3);
    ret = (SHA1_Init(&sha) ? 0 : TPM_SHA_ERROR);
    if (h1 == NULL || h2 == NULL) {
	ret = ERR_NULL_ARG;
	goto exit;
    }
    c = h3;
    for (;;) {
	dlen = (unsigned int) va_arg(argp, unsigned int);
	if (dlen == 0)
	    break;
	data = (unsigned char *) va_arg(argp, unsigned char *);
	if (data == NULL) {
	    ret = ERR_NULL_ARG;
	    goto exit;
	}
	ret = (SHA1_Update(&sha, data, dlen) ? 0 : TPM_SHA_ERROR);
    }
    ret = (SHA1_Final(paramdigest, &sha) ? 0 : TPM_SHA_ERROR);

    TSS_rawhmac(digest, key, keylen, TPM_HASH_SIZE, paramdigest,
		TPM_NONCE_SIZE, h1, TPM_NONCE_SIZE, h2, 1, &c, 0, 0);
    ret = 0;

exit:
    va_end(argp);
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Calculate Raw HMAC value                                                 */
/*                                                                          */
/* This function calculates an HMAC digest                                  */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* digest - a pointer to a 20 byte array that will receive the result       */
/* key    - a pointer to the key to be used in the HMAC calculation         */
/* keylen - the size of the key in bytes                                    */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is a pointer to the data to be hashed      */
/*   into the paramdigest.                                                  */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/
uint32_t TSS_rawhmac(unsigned char *digest, const unsigned char *key,
		     unsigned int keylen, ...)
{
    HMAC_CTX hmac;
    unsigned int dlen;
    unsigned char *data;
    va_list argp;
    uint32_t ret;

#ifdef HAVE_HMAC_CTX_CLEANUP
    HMAC_CTX_init(&hmac);
#endif
    HMAC_Init(&hmac, key, keylen, EVP_sha1());

    va_start(argp, keylen);
    for (;;) {
	dlen = (unsigned int) va_arg(argp, unsigned int);
	if (dlen == 0)
	    break;
	data = (unsigned char *) va_arg(argp, unsigned char *);
	if (data == NULL) {
	    ret = ERR_NULL_ARG;
	    goto exit;
	}
	HMAC_Update(&hmac, data, dlen);
    }
    HMAC_Final(&hmac, digest, &dlen);

#ifdef HAVE_HMAC_CTX_CLEANUP
    HMAC_CTX_cleanup(&hmac);
#else
    HMAC_cleanup(&hmac);
#endif
    ret = 0;

exit:
    va_end(argp);
    return 0;
}
