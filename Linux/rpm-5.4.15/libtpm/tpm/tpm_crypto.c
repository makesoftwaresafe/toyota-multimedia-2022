/********************************************************************************/
/*                                                                              */
/*                      Platform Dependent Crypto                               */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_crypto.c 4603 2011-08-16 20:40:26Z kgoldman $            */
/*                                                                              */
/* (c) Copyright IBM Corporation 2006, 2010.					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

/* This is the openSSL implementation */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/engine.h>

#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_key.h"
#include "tpm_io.h"
#include "tpm_load.h"
#include "tpm_memory.h"
#include "tpm_process.h"
#include "tpm_types.h"

#include "tpm_crypto.h"


/* The TPM OAEP encoding parameter */
static const unsigned char tpm_oaep_pad_str[] = { 'T', 'C', 'P', 'A' };


/* TPM_OpenSSL_PrintError() prints a detailed openSSL error trace.  */
static void TPM_OpenSSL_PrintError()
{
    /* openssl error printing */
    unsigned long       error;
    const char          *file;
    int                 line;
    const char          *data;
    int                 flags;

    error = ERR_get_error_line_data(&file, &line, &data, &flags);
    printf("\terror %08lx file %s line %d data %s flags %08x\n",
           error, file, line, data, flags);
    return;
}

/* TPM_bin2bn() wraps the openSSL function in a TPM error handler

   Converts a char array to bignum

   bn must be freed by the caller.
*/

TPM_RESULT TPM_bin2bn(TPM_BIGNUM *bn_in, const unsigned char *bin, unsigned int bytes)
{
    TPM_RESULT  rc = 0;
    BIGNUM	**bn = (BIGNUM **)bn_in;

    /* BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret);
    
       BN_bin2bn() converts the positive integer in big-endian form of length len at s into a BIGNUM
       and places it in ret. If ret is NULL, a new BIGNUM is created.

       BN_bin2bn() returns the BIGNUM, NULL on error.
    */
    *bn = BN_bin2bn(bin, bytes, *bn);
    if (*bn == NULL) {
	printf("TPM_bin2bn: Error in BN_bin2bn\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_SIZE;
    }
    return rc;
}

/* TPM_RSAGeneratePublicToken() generates an RSA key token from n and e */
static TPM_RESULT TPM_RSAGeneratePublicToken(RSA **rsa_pub_key,		/* freed by caller */
					     unsigned char *narr,      	/* public modulus */
					     uint32_t nbytes,
					     unsigned char *earr,      	/* public exponent */
					     uint32_t ebytes)
{
    TPM_RESULT  rc = 0;
    BIGNUM *    n = NULL;
    BIGNUM *    e = NULL;

    /* sanity check for the free */
    if (*rsa_pub_key != NULL) {
	printf("TPM_RSAGeneratePublicToken: Error (fatal), token %p should be NULL\n",
		   *rsa_pub_key );
	rc = TPM_FAIL;
	goto exit;
    }

    /* construct the OpenSSL private key object */
    *rsa_pub_key = RSA_new();                        	/* freed by caller */
    if (*rsa_pub_key == NULL) {
	printf("TPM_RSAGeneratePublicToken: Error in RSA_new()\n");
	rc = TPM_SIZE;
	goto exit;
    }

    rc = TPM_bin2bn((TPM_BIGNUM *)&n, narr, nbytes);	/* freed by caller */
    if (rc)
	goto exit;

    (*rsa_pub_key)->n = n;
    rc = TPM_bin2bn((TPM_BIGNUM *)&e, earr, ebytes);	/* freed by caller */
    if (rc)
	goto exit;

    (*rsa_pub_key)->e = e;
    (*rsa_pub_key)->d = NULL;

exit:
    return rc;
}

/* TPM_RSAGeneratePrivateToken() generates an RSA key token from n,e,d */
static TPM_RESULT TPM_RSAGeneratePrivateToken(RSA **rsa_pri_key,	/* freed by caller */
					      unsigned char *narr,      /* public modulus */
					      uint32_t nbytes,
					      unsigned char *earr,      /* public exponent */
					      uint32_t ebytes,
					      unsigned char *darr,	/* private exponent */
					      uint32_t dbytes)
{
    TPM_RESULT  rc = 0;
    BIGNUM *    n = NULL;
    BIGNUM *    e = NULL;
    BIGNUM *    d = NULL;

    /* sanity check for the free */
    if (*rsa_pri_key != NULL) {
	printf("TPM_RSAGeneratePrivateToken: Error (fatal), token %p should be NULL\n",
			*rsa_pri_key );
	rc = TPM_FAIL;
	goto exit;
    }

    /* construct the OpenSSL private key object */
    *rsa_pri_key = RSA_new();                        /* freed by caller */
    if (*rsa_pri_key == NULL) {
	printf("TPM_RSAGeneratePrivateToken: Error in RSA_new()\n");
	rc = TPM_SIZE;
	goto exit;
    }

    rc = TPM_bin2bn((TPM_BIGNUM *)&n, narr, nbytes);	/* freed by caller */
    if (rc)
	goto exit;

    (*rsa_pri_key)->n = n;
    rc = TPM_bin2bn((TPM_BIGNUM *)&e, earr, ebytes);	/* freed by caller */
    if (rc)
	goto exit;

    (*rsa_pri_key)->e = e;
    rc = TPM_bin2bn((TPM_BIGNUM *)&d, darr, dbytes);	/* freed by caller */
    if (rc)
	goto exit;

    (*rsa_pri_key)->d = d;

exit:
    return rc;
}

/* TPM_RSASignSHA1() performs the following:
        prepend a DER encoded algorithm ID
        prepend a type 1 pad
        encrypt with the private key
*/
static TPM_RESULT TPM_RSASignSHA1(unsigned char *signature,             /* output */
                                  unsigned int *signature_length, /* output, size of signature */
                                  const unsigned char *message,         /* input */
                                  size_t message_size,                  /* input */
                                  RSA *rsa_pri_key)                     /* signing private key */
{
    TPM_RESULT  rc = 0;
    int         irc;

    printf(" TPM_RSASignSHA1:\n");
    /* sanity check, SHA1 messages must be 20 bytes */
    if (message_size != TPM_DIGEST_SIZE) {
	printf("TPM_RSASignSHA1: Error, message size %lu not TPM_DIGEST_SIZE\n",
		(unsigned long)message_size );
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    } 

    /* type NID_sha1, adds the algorithm identifier and type 1 pad */
    irc = RSA_sign(NID_sha1, message, message_size,
                       signature, signature_length,
                       rsa_pri_key);

    /* RSA_sign() returns 1 on success, 0 otherwise. */
    if (irc != 1) {
	printf("TPM_RSASignSHA1: Error in RSA_sign()\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

exit:
    return rc;
}

/* TPM_RSASignDER() performs the following:
        prepend a type 1 pad
        encrypt with the private key
   The caller must check that the signature buffer is >= the key size.
*/
static TPM_RESULT TPM_RSASignDER(unsigned char *signature,              /* output */
                                 unsigned int *signature_length, /* output, size of signature */
                                 const unsigned char *message,          /* input */
                                 size_t message_size,                   /* input */
                                 RSA *rsa_pri_key)                      /* signing private key */
{
    TPM_RESULT  rc = 0;
    int         irc;
    int         key_size;
    unsigned char * message_pad = NULL;			/* freed @1 */
    int         int_sig_len;    /* openSSL overloads RSA_private_decrypt return code */
    
    printf(" TPM_RSASignDER:\n");
    /* the padded message size is the same as the key size */
    key_size = RSA_size(rsa_pri_key);
    if (key_size < 0) {
	printf(" TPM_RSASignDER: Error (fatal), negative key size %d\n", key_size);
	rc = TPM_FAIL;      /* should never occur */
	goto exit;
    }

    /* allocate memory for the padded message */
    printf(" TPM_RSASignDER: key size %d\n", key_size);
    rc = TPM_Malloc(&message_pad, key_size);		/* freed @1 */
    if (rc)
	goto exit;

    /* PKCS1 type 1 pad the message */
    printf("  TPM_RSASignDER: Applying PKCS1 type 1 padding, size from %lu to %u\n",
		(unsigned long)message_size, key_size);
    TPM_PrintFour("  TPM_RSASignDER: Input message", message);
    /* This call checks that the message will fit with the padding */
    irc = RSA_padding_add_PKCS1_type_1(message_pad,
		key_size, message, message_size);
    if (irc != 1) {
	printf("TPM_RSASignDER: Error padding message, size %lu key size %u\n",
		(unsigned long)message_size, key_size);
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    /* raw sign with private key */
    printf("  TPM_RSASignDER: Encrypting with private key, message size %d\n", key_size);
    TPM_PrintFour("  TPM_RSASignDER: Padded message", message_pad);
    /* returns the size of the encrypted data.  On error, -1 is returned */
    int_sig_len = RSA_private_encrypt(key_size,         /* int flen */
			message_pad,      /* unsigned char *from, */
			signature,        /* unsigned char *to, */
			rsa_pri_key,      /* RSA *rsa, */
			RSA_NO_PADDING);  /* int padding); */
    if (int_sig_len < 0) {
	printf("TPM_RSASignDER: Error in RSA_private_encrypt()\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }
    *signature_length = (unsigned int)int_sig_len;

    TPM_PrintFour("  TPM_RSASignDER: signature", signature);

exit:
    if (message_pad)
	free(message_pad);          /* @1 */
    return rc;
}

/* TPM_BN_CTX_new() wraps the openSSL function in a TPM error handler */
static TPM_RESULT TPM_BN_CTX_new(BN_CTX **ctx)
{
    TPM_RESULT  rc = 0;

    if (*ctx != NULL) {
	printf("TPM_BN_CTX_new: Error (fatal), *ctx %p should be NULL before BN_CTX_new \n",
		*ctx);
	rc = TPM_FAIL;
	goto exit;
    }

    *ctx = BN_CTX_new();
    if (*ctx == NULL) {
	printf("TPM_BN_CTX_new: Error, context is NULL\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_SIZE;
	goto exit;

    }

exit:
    return rc;
}

/* TPM_SYMMETRIC_KEY_DATA is a crypto library platform dependent symmetric key structure
 */
#ifdef TPM_DES

/* local prototype and structure for DES */

#include <openssl/des.h>

/* DES requires data lengths that are a multiple of the block size */
#define TPM_DES_BLOCK_SIZE 8

typedef struct tdTPM_SYMMETRIC_KEY_DATA {
    TPM_TAG tag;
    TPM_BOOL valid;
    BYTE fill;
    DES_cblock des_cblock1;
    DES_cblock des_cblock2;
    DES_cblock des_cblock3;
} TPM_SYMMETRIC_KEY_DATA;

/* TPM_SymmetricKeyData_Crypt() is DES common code for openSSL, since encrypt and decrypt use the
   same function with an 'enc' flag.

   'data_in' and 'data_out' must be preallocated arrays of 'length' bytes.  'length' must be a
   multiple of TPM_DES_BLOCK_SIZE.

   Returns 'error' on error.
*/

/* openSSL prototype

   void DES_ede3_cbc_encrypt(const unsigned char *input,
                             unsigned char *output, long length, DES_key_schedule *ks1,
                             DES_key_schedule *ks2, DES_key_schedule *ks3, DES_cblock *ivec,
                             int enc);
*/
static TPM_RESULT TPM_SymmetricKeyData_Crypt(unsigned char *data_out,           /* output */
                                             const unsigned char *data_in,      /* input */
                                             uint32_t length,			/* input */
                                             TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data, /*in*/
                                             int enc,                           /* input */
                                             TPM_RESULT error)                  /* input */
{
    TPM_RESULT  rc = 0;
    int         irc;
    DES_key_schedule des_key_schedule1;
    DES_key_schedule des_key_schedule2;
    DES_key_schedule des_key_schedule3;
    DES_cblock ivec;    /* initial chaining vector */

    if ((length % TPM_DES_BLOCK_SIZE) != 0) {
	printf("TPM_SymmetricKeyData_Crypt: Error, illegal length %u\n", length);
	rc = error;	/* should never occur */
	goto exit;
    }

    TPM_PrintFour("  TPM_SymmetricKeyData_Crypt: des1", tpm_symmetric_key_data->des_cblock1);
    TPM_PrintFour("  TPM_SymmetricKeyData_Crypt: des2", tpm_symmetric_key_data->des_cblock2);
    TPM_PrintFour("  TPM_SymmetricKeyData_Crypt: des3", tpm_symmetric_key_data->des_cblock3);

    /* Before a DES key can be used, it must be converted into the architecture dependent
       DES_key_schedule via the DES_set_key_checked() or DES_set_key_unchecked() function. */
    irc = DES_set_key_checked(&(tpm_symmetric_key_data->des_cblock1), &des_key_schedule1);
    if (irc != 0) {
        printf("TPM_SymmetricKeyData_Crypt: Error, DES_set_key_checked rc %d\n", irc);
        rc = error;
	goto exit;
    }

    irc = DES_set_key_checked(&(tpm_symmetric_key_data->des_cblock2), &des_key_schedule2);
    if (irc != 0) {
        printf("TPM_SymmetricKeyData_Crypt: Error, DES_set_key_checked rc %d\n", irc);
        rc = error;
	goto exit;
    }

    irc = DES_set_key_checked(&(tpm_symmetric_key_data->des_cblock3), &des_key_schedule3);
    if (irc != 0) {
        printf("TPM_SymmetricKeyData_Crypt: Error, DES_set_key_checked rc %d\n", irc);
        rc = error;
	goto exit;
    }

    /* initialize initial chaining vector */
    TPM_PrintFour("  TPM_SymmetricKeyData_Crypt: Input", data_in);
    /* encrypt operation */
    memset(&ivec, 0, sizeof(DES_cblock));
    DES_ede3_cbc_encrypt(data_in,
                             data_out,
                             length,
                             &des_key_schedule1,
                             &des_key_schedule2,
                             &des_key_schedule3,
                             &ivec,
                             enc);
    TPM_PrintFour("  TPM_SymmetricKeyData_Crypt: Output", data_out);

exit:
    return rc;
}
#endif

#ifdef TPM_AES

/* local prototype and structure for AES */

#include <openssl/aes.h>

/* AES requires data lengths that are a multiple of the block size */
#define TPM_AES_BITS 128
/* The AES block size is always 16 bytes */
#define TPM_AES_BLOCK_SIZE 16

/* Since the AES key is often derived by truncating the session shared secret, test that it's not
   too large
*/

#if (TPM_AES_BLOCK_SIZE > TPM_SECRET_SIZE)
#error TPM_AES_BLOCK_SIZE larger than TPM_SECRET_SIZE
#endif

/* The AES initial CTR value is derived from a nonce. */

#if (TPM_AES_BLOCK_SIZE > TPM_NONCE_SIZE)
#error TPM_AES_BLOCK_SIZE larger than TPM_NONCE_SIZE
#endif

typedef struct tdTPM_SYMMETRIC_KEY_DATA {
    TPM_TAG tag;
    TPM_BOOL valid;
    TPM_BOOL fill;
    unsigned char userKey[TPM_AES_BLOCK_SIZE];
    /* For performance, generate these once from userKey */
    AES_KEY aes_enc_key;
    AES_KEY aes_dec_key;
} TPM_SYMMETRIC_KEY_DATA;

/* TPM_SymmetricKeyData_SetKeys() is AES non-portable code to construct the internal AES keys from
   the userKey

   tpm_symmetric_key_data should be initialized before and after use
*/
static TPM_RESULT TPM_SymmetricKeyData_SetKeys(TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data)
{
    TPM_RESULT rc = 0;
    int irc;

    printf(" TPM_SymmetricKeyData_SetKeys:\n");
    TPM_PrintFour("  TPM_SymmetricKeyData_SetKeys: userKey", tpm_symmetric_key_data->userKey);
    irc = AES_set_encrypt_key(tpm_symmetric_key_data->userKey,
                                  TPM_AES_BITS,
                                  &(tpm_symmetric_key_data->aes_enc_key));
    if (irc != 0) {
	printf("TPM_SymmetricKeyData_SetKeys: Error (fatal) generating enc key\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_FAIL;      /* should never occur, null pointers or bad bit size */
	goto exit;
    }

    irc = AES_set_decrypt_key(tpm_symmetric_key_data->userKey,
                                  TPM_AES_BITS,
                                  &(tpm_symmetric_key_data->aes_dec_key));
    if (irc != 0) {
	printf("TPM_SymmetricKeyData_SetKeys: Error (fatal) generating dec key\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_FAIL;      /* should never occur, null pointers or bad bit size */
	goto exit;
    }

exit:
    return rc;
}

/* TPM_SymmetricKeyData_SetKey() is AES non-portable code to set a symmetric key from input data

   tpm_symmetric_key_data should be initialized before and after use
*/
static TPM_RESULT TPM_SymmetricKeyData_SetKey(TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data,
                                       const unsigned char *key_data,
                                       uint32_t key_data_size)
{
    TPM_RESULT rc = 0;
    
    printf(" TPM_SymmetricKeyData_SetKey:\n");
    /* check the input data size, it can be truncated, but cannot be smaller than the AES key */
    if (sizeof(tpm_symmetric_key_data->userKey) > key_data_size) {
        printf("TPM_SymmetricKeyData_SetKey: Error (fatal), need %lu bytes, received %u\n",
		(unsigned long)sizeof(tpm_symmetric_key_data->userKey), key_data_size);
        rc = TPM_FAIL;              /* should never occur */
	goto exit;
    }

    /* copy the input data into the AES key structure */
    memcpy(tpm_symmetric_key_data->userKey, key_data, sizeof(tpm_symmetric_key_data->userKey));
    /* construct the internal AES keys */
    rc = TPM_SymmetricKeyData_SetKeys(tpm_symmetric_key_data);
    if (rc)
	goto exit;

    tpm_symmetric_key_data->valid = TRUE;

exit:
    return rc;
}

/* TPM_AES_ctr128_encrypt() is a TPM variant of the openSSL AES_ctr128_encrypt() function that
   increments only the low 4 bytes of the counter.

   openSSL increments the entire CTR array.  The TPM does not follow that convention.
*/
static TPM_RESULT TPM_AES_ctr128_encrypt(unsigned char *data_out,
					 const unsigned char *data_in,
					 uint32_t data_size,
					 const AES_KEY *aes_enc_key,
					 unsigned char ctr[TPM_AES_BLOCK_SIZE])
{
    TPM_RESULT  rc = 0;
    uint32_t cint;
    unsigned char pad_buffer[TPM_AES_BLOCK_SIZE];       /* the XOR pad */

    printf("  TPM_AES_Ctr128_encrypt:\n");
    while (data_size != 0) {
        printf("   TPM_AES_Ctr128_encrypt: data_size %lu\n", (unsigned long)data_size);
        /* get an XOR pad array by encrypting the CTR with the AES key */
        AES_encrypt(ctr, pad_buffer, aes_enc_key);
        /* partial or full last data block */
        if (data_size <= TPM_AES_BLOCK_SIZE) {
            TPM_XOR(data_out, data_in, pad_buffer, data_size);
            data_size = 0;
        }
        /* full block, not the last block */
        else {
            TPM_XOR(data_out, data_in, pad_buffer, TPM_AES_BLOCK_SIZE);
            data_in += TPM_AES_BLOCK_SIZE;
            data_out += TPM_AES_BLOCK_SIZE;
            data_size -= TPM_AES_BLOCK_SIZE;
        }
        /* if not the last block, increment CTR, only the low 4 bytes */
        if (data_size != 0) {
            /* CTR is a big endian array, so the low 4 bytes are 12-15 */
            cint = LOAD32(ctr, 12);     /* byte array to uint32_t */
            cint++;                     /* increment */
            STORE32(ctr, 12, cint);     /* uint32_t to byte array */
        }
    }
    return rc;
}

#endif

/* Initialization function */
TPM_RESULT TPM_Crypto_Init(void)
{
    TPM_RESULT rc = 0;

    printf("TPM_Crypto_Init: OpenSSL library %08lx\n", (unsigned long)OPENSSL_VERSION_NUMBER);
    OpenSSL_add_all_algorithms();
    /* sanity check that the SHA1 context handling remains portable */
    if ((sizeof(SHA_LONG) != sizeof(uint32_t))
     || (sizeof(unsigned int) != sizeof(uint32_t))
     || (sizeof(SHA_CTX) != (sizeof(uint32_t) * (8 + SHA_LBLOCK))))
    {
	printf("TPM_Crypto_Init: Error(fatal), SHA_CTX has unexpected structure\n");
	rc = TPM_FAIL;
	goto exit;
    }

exit:
    return rc;
}

/* TPM_Crypto_TestSpecific() performs any library specific tests For OpenSSL */
TPM_RESULT TPM_Crypto_TestSpecific(void)
{
    TPM_RESULT          rc = 0;
   
    /* Saving the SHA-1 context is fragile code, so test at startup */
    void * context1 = NULL;			/* freed @1 */
    void * context2 = NULL;			/* freed @2 */
    unsigned char buffer1[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    unsigned char expect1[] = {0x84,0x98,0x3E,0x44,0x1C,
			       0x3B,0xD2,0x6E,0xBA,0xAE,
			       0x4A,0xA1,0xF9,0x51,0x29,
			       0xE5,0xE5,0x46,0x70,0xF1};
    TPM_DIGEST	actual;
    int		not_equal;
    TPM_STORE_BUFFER sbuffer;
    const unsigned char *stream;
    uint32_t stream_size;
    
    printf(" TPM_Crypto_TestSpecific: Test 1 - SHA1 two parts\n");
    TPM_Sbuffer_Init(&sbuffer);		/* freed @3 */
    
    rc = TPM_Malloc((unsigned char **)&context1, sizeof(SHA_CTX));	/* freed @1 */
    if (rc)
	goto exit;

    /* digest the first part of the array */
    rc = (SHA1_Init(context1) ? 0 : TPM_SHA_ERROR);
    if (rc)
	goto exit;
    rc = (SHA1_Update(context1, buffer1, 16) ? 0 : TPM_SHA_ERROR);
    if (rc)
	goto exit;

    /* store the SHA1 context */
    rc = TPM_Sha1Context_Store(&sbuffer, context1);
    if (rc)
	goto exit;

    /* load the SHA1 context */
    TPM_Sbuffer_Get(&sbuffer, &stream, &stream_size);
    rc = TPM_Sha1Context_Load(&context2,
		(unsigned char **)&stream, &stream_size);	/* freed @2 */
    if (rc)
	goto exit;

    /* digest the rest of the array */
    rc = (SHA1_Update(context2, buffer1 + 16, sizeof(buffer1) - 17)
		? 0 : TPM_SHA_ERROR);
    if (rc)
	goto exit;

    rc = (SHA1_Final(actual, context2) ? 0 : TPM_SHA_ERROR);
    if (rc)
	goto exit;

    not_equal = memcmp(expect1, actual, TPM_DIGEST_SIZE);
    if (not_equal) {
	printf("TPM_Crypto_TestSpecific: Error in test 1\n");
	TPM_PrintFour("\texpect", expect1);
	TPM_PrintFour("\tactual", actual);
	rc = TPM_FAILEDSELFTEST;
    }

    TPM_Sbuffer_Delete(&sbuffer);	/* @3 */

exit:
    if (context2)
	free(context2);			/* @2 */
    if (context1)
	free(context1);			/* @1 */
    return rc;
}

/* Random Number Functions */

/* TPM_Random() fills 'buffer' with 'bytes' bytes.  */
TPM_RESULT TPM_Random(BYTE *buffer, size_t bytes)
{
    TPM_RESULT rc = 0;

    printf(" TPM_Random: Requesting %lu bytes\n", (unsigned long)bytes);

    /* openSSL call */
    rc = (RAND_bytes(buffer, bytes) ? 0 : TPM_FAIL);
    if (rc)
	printf("TPM_Random: Error (fatal) calling RAND_bytes()\n");

    return rc;
}

TPM_RESULT TPM_StirRandomCmd(TPM_SIZED_BUFFER *inData)
{
    TPM_RESULT rc = 0;

    printf(" TPM_StirRandomCmd:\n");

    /* NOTE: The TPM command does not give an entropy estimate.  This assumes the best case */
    /* openSSL call */
    RAND_add(inData->buffer,	/* buf mixed into PRNG state*/
		 inData->size,		/* number of bytes */
		 inData->size);	/* entropy, the lower bound of an estimate of how much randomness is
				   contained in buf */

    return rc;
}

/* RSA Functions */

/* Generate an RSA key pair.

   'n', 'p', 'q', 'd' must be freed by the caller
*/

TPM_RESULT TPM_RSAGenerateKeyPair(unsigned char **n,            /* public key - modulus */
                                  unsigned char **p,            /* private key prime */
                                  unsigned char **q,            /* private key prime */
                                  unsigned char **d,            /* private key (private exponent) */
                                  int num_bits,                 /* key size in bits */
                                  const unsigned char *earr,    /* public exponent as an array */
                                  uint32_t e_size)
{
    TPM_RESULT rc = 0;
    RSA *rsa = NULL;
    uint32_t nbytes;
    uint32_t pbytes;
    uint32_t qbytes;
    uint32_t dbytes;

    unsigned long e;

    /* initialize in case of error */
    printf(" TPM_RSAGenerateKeyPair:\n");
    *n = NULL;
    *p = NULL;
    *q = NULL;
    *d = NULL;

    /* check that num_bits is a multiple of 16.  If not, the primes p and q will not be a multiple of
       8 and will not fit well in a byte */
    if ((num_bits % 16) != 0) {
	printf("TPM_RSAGenerateKeyPair: Error, num_bits %d is not a multiple of 16\n",
		   num_bits);
	rc = TPM_BAD_KEY_PROPERTY;
	goto exit;
    }

    /* convert the e array to an unsigned long */
    rc = TPM_LoadLong(&e, earr, e_size);
    if (rc)
	goto exit;

    /* validate the public exponent against a list of legal values.  Some values (e.g. even numbers)
       will hang the key generator. */
    rc = TPM_RSA_exponent_verify(e);
    if (rc)
	goto exit;

    printf("  TPM_RSAGenerateKeyPair: num_bits %d exponent %08lx\n", num_bits, e);
    rsa = RSA_generate_key(num_bits, e, NULL, NULL);                /* freed @1 */
    if (rsa == NULL) {
	printf("TPM_RSAGenerateKeyPair: Error calling RSA_generate_key()\n");
	rc = TPM_BAD_KEY_PROPERTY;
	goto exit;
    }

    /* load n */
    rc = TPM_bn2binMalloc(n, &nbytes, (TPM_BIGNUM)rsa->n, num_bits/8); /* freed by caller */
    if (rc)
	goto exit;

    /* load p */
    rc = TPM_bn2binMalloc(p, &pbytes, (TPM_BIGNUM)rsa->p, num_bits/16); /* freed by caller */
    if (rc)
	goto exit;

    /* load q */
    rc = TPM_bn2binMalloc(q, &qbytes, (TPM_BIGNUM)rsa->q, num_bits/16); /* freed by caller */
    if (rc)
	goto exit;

    /* load d */
    rc = TPM_bn2binMalloc(d, &dbytes, (TPM_BIGNUM)rsa->d, num_bits/8); /* freed by caller */
    if (rc)
	goto exit;

    printf("  TPM_RSAGenerateKeyPair: length of n,p,q,d = %d / %d / %d / %d\n",
		nbytes, pbytes, qbytes, dbytes);

exit:
    if (rc != 0) {
	if (n)
	    free(*n);
	if (p)
	    free(*p);
	if (q)
	    free(*q);
	if (d)
	    free(*d);
    }
    if (rsa)
        RSA_free(rsa);  /* @1 */
    return rc;
}

/* TPM_RSAPrivateDecrypt() decrypts 'encrypt_data' using the private key 'n, e, d'.  The OAEP
   padding is removed and 'decrypt_data_length' bytes are moved to 'decrypt_data'.

   'decrypt_data_length' is at most 'decrypt_data_size'.
*/

TPM_RESULT TPM_RSAPrivateDecrypt(unsigned char *decrypt_data,   /* decrypted data */
                                 uint32_t *decrypt_data_length,	/* length of data put into
                                                                   decrypt_data */
                                 size_t decrypt_data_size,      /* size of decrypt_data buffer */
                                 TPM_ENC_SCHEME encScheme,      /* encryption scheme */
                                 unsigned char *encrypt_data,   /* encrypted data */
                                 uint32_t encrypt_data_size,
                                 unsigned char *narr,           /* public modulus */
                                 uint32_t nbytes,
                                 unsigned char *earr,           /* public exponent */
                                 uint32_t ebytes,
                                 unsigned char *darr,           /* private exponent */
                                 uint32_t dbytes)
{
    TPM_RESULT  rc = 0;
    int         irc;
    RSA *       rsa_pri_key = NULL;	/* freed @1 */

    unsigned char       *padded_data = NULL;
    int                 padded_data_size = 0;
    
    printf(" TPM_RSAPrivateDecrypt:\n");
    /* construct the OpenSSL private key object */
    if (rc == 0) {
	rc = TPM_RSAGeneratePrivateToken(&rsa_pri_key,	/* freed @1 */
					 narr,      	/* public modulus */
					 nbytes,
					 earr,      	/* public exponent */
					 ebytes,
					 darr,		/* private exponent */
					 dbytes);
    }
    /* intermediate buffer for the decrypted but still padded data */
    /* the size of the decrypted data is guaranteed to be less than this */
    padded_data_size = RSA_size(rsa_pri_key);
    rc = TPM_Malloc(&padded_data, padded_data_size);
    if (rc)
	goto exit;

    /* decrypt with private key.  Must decrypt first and then remove padding because the decrypt
       call cannot specify an encoding parameter */
    /* returns the size of the encrypted data.  On error, -1 is returned */
    irc = RSA_private_decrypt(encrypt_data_size,        /* length */
                                      encrypt_data,             /* from - the encrypted data */
                                      padded_data,      /* to - the decrypted but padded data */
                                      rsa_pri_key,              /* key */
                                      RSA_NO_PADDING);          /* padding */
    if (irc < 0) {
	printf("TPM_RSAPrivateDecrypt: Error in RSA_private_decrypt()\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    printf("  TPM_RSAPrivateDecrypt: RSA_private_decrypt() success\n");
    printf("  TPM_RSAPrivateDecrypt: Padded data size %u\n", padded_data_size);
    TPM_PrintFour("  TPM_RSAPrivateDecrypt: Decrypt padded data", padded_data);
    if (encScheme == TPM_ES_RSAESOAEP_SHA1_MGF1) {
	/* openSSL expects the padded data to skip the first 0x00 byte, since it expects the
               padded data to come from a bignum via bn2bin. */
	irc = RSA_padding_check_PKCS1_OAEP(decrypt_data,            /* to */
                                               decrypt_data_size,       /* to length */
                                               padded_data + 1,         /* from */
                                               padded_data_size - 1,    /* from length */
                                               encrypt_data_size,       /* rsa_len */
                                               tpm_oaep_pad_str,        /* encoding parameter */
                                               sizeof(tpm_oaep_pad_str) /* encoding parameter length
                                                                           */
                                               );
	if (irc < 0) {
	    printf("TPM_RSAPrivateDecrypt: Error in RSA_padding_check_PKCS1_OAEP()\n");
	    rc = TPM_DECRYPT_ERROR;
	    goto exit;
	}
    } else
    if (encScheme == TPM_ES_RSAESPKCSv15) {
	irc = RSA_padding_check_PKCS1_type_2(decrypt_data,          /* to */
                                                 decrypt_data_size,     /* to length */
                                                 padded_data + 1,       /* from */
                                                 padded_data_size - 1,  /* from length */
                                                 encrypt_data_size      /* rsa_len */
                                                 );
	if (irc < 0) {
	    printf("TPM_RSAPrivateDecrypt: Error in RSA_padding_check_PKCS1_type_2()\n");
	    rc = TPM_DECRYPT_ERROR;
	    goto exit;
	}
    } else {
	printf("TPM_RSAPrivateDecrypt: Error, unknown encryption scheme %04x\n", encScheme);
	rc = TPM_INAPPROPRIATE_ENC;
	goto exit;
    }

    *decrypt_data_length = irc;
    printf("  TPM_RSAPrivateDecrypt: RSA_padding_check_PKCS1_OAEP() recovered %d bytes\n", irc);
    TPM_PrintFour("  TPM_RSAPrivateDecrypt: Decrypt data", decrypt_data);

exit:
    if (rsa_pri_key)
        RSA_free(rsa_pri_key);		/* @1 */
    if (padded_data)
	free(padded_data);		/* @2 */
    return rc;
}

/* TPM_RSAPublicEncrypt() pads 'decrypt_data' to 'encrypt_data_size' and encrypts using the public
   key 'n, e'.
*/
TPM_RESULT TPM_RSAPublicEncrypt(unsigned char* encrypt_data,    /* encrypted data */
                                size_t encrypt_data_size,       /* size of encrypted data buffer */
                                TPM_ENC_SCHEME encScheme,
                                const unsigned char *decrypt_data,      /* decrypted data */
                                size_t decrypt_data_size,
                                unsigned char *narr,           /* public modulus */
                                uint32_t nbytes,
                                unsigned char *earr,           /* public exponent */
                                uint32_t ebytes)
{
    TPM_RESULT  rc = 0;
    int         irc;
    RSA         *rsa_pub_key = NULL;
    unsigned char *padded_data = NULL;
    
    printf(" TPM_RSAPublicEncrypt: Input data size %lu\n", (unsigned long)decrypt_data_size);
    /* intermediate buffer for the decrypted by still padded data */
    rc = TPM_Malloc(&padded_data, encrypt_data_size);               /* freed @2 */
    if (rc)
	goto exit;

    /* construct the OpenSSL public key object */
    rc = TPM_RSAGeneratePublicToken(&rsa_pub_key,	/* freed @1 */
					narr,      	/* public modulus */
					nbytes,
					earr,      	/* public exponent */
					ebytes);
    if (rc)
	goto exit;

    if (encScheme == TPM_ES_RSAESOAEP_SHA1_MGF1) {
	irc = RSA_padding_add_PKCS1_OAEP(padded_data,               /* to */
                                             encrypt_data_size,         /* to length */
                                             decrypt_data,              /* from */
                                             decrypt_data_size,         /* from length */
                                             tpm_oaep_pad_str,          /* encoding parameter */
                                             sizeof(tpm_oaep_pad_str)   /* encoding parameter length
                                                                           */
                                             );
	if (irc != 1) {
	    printf("TPM_RSAPublicEncrypt: Error in RSA_padding_add_PKCS1_OAEP()\n");
	    rc = TPM_ENCRYPT_ERROR;
	    goto exit;
	}
        printf("  TPM_RSAPublicEncrypt: RSA_padding_add_PKCS1_OAEP() success\n");
    } else
    if (encScheme == TPM_ES_RSAESPKCSv15) {
	irc = RSA_padding_add_PKCS1_type_2(padded_data,             /* to */
                                               encrypt_data_size,       /* to length */
                                               decrypt_data,            /* from */
                                               decrypt_data_size);      /* from length */
	if (irc != 1) {
	    printf("TPM_RSAPublicEncrypt: Error in RSA_padding_add_PKCS1_type_2()\n");
	    rc = TPM_ENCRYPT_ERROR;
	    goto exit;
	}
	printf("  TPM_RSAPublicEncrypt: RSA_padding_add_PKCS1_type_2() success\n");
    } else {
	printf("TPM_RSAPublicEncrypt: Error, unknown encryption scheme %04x\n", encScheme);
	rc = TPM_INAPPROPRIATE_ENC;
	goto exit;
    }

    printf("  TPM_RSAPublicEncrypt: Padded data size %lu\n", (unsigned long)encrypt_data_size);
    TPM_PrintFour("  TPM_RSAPublicEncrypt: Padded data", padded_data);
    /* encrypt with public key.  Must pad first and then encrypt because the encrypt
           call cannot specify an encoding parameter */
    /* returns the size of the encrypted data.  On error, -1 is returned */
    irc = RSA_public_encrypt(encrypt_data_size,         /* from length */
                                     padded_data,               /* from - the clear text data */
                                     encrypt_data,              /* the padded and encrypted data */
                                     rsa_pub_key,               /* key */
                                     RSA_NO_PADDING);           /* padding */
    if (irc < 0) {
	printf("TPM_RSAPublicEncrypt: Error in RSA_public_encrypt()\n");
	rc = TPM_ENCRYPT_ERROR;
	goto exit;
    }

    printf("  TPM_RSAPublicEncrypt: RSA_public_encrypt() success\n");

exit:
    if (rsa_pub_key)
        RSA_free(rsa_pub_key);		/* @1 */
    if (padded_data)
	free(padded_data);		/* @2 */
    return rc;
}

/* TPM_RSAPublicEncryptRaw() does a raw public key operation without any padding.

*/
TPM_RESULT TPM_RSAPublicEncryptRaw(unsigned char *encrypt_data,	/* output */
				   uint32_t encrypt_data_size,	/* input, size of message buffer */
				   unsigned char *decrypt_data,	/* input */
				   uint32_t decrypt_data_size,	/* input, size of sig buffer */
				   unsigned char *narr,		/* public modulus */
				   uint32_t nbytes,
				   unsigned char *earr,		/* public exponent */
				   uint32_t ebytes)
{
    TPM_RESULT          rc = 0;
    int                 irc;
    RSA                 *rsa_pub_key = NULL;

    printf("   TPM_RSAPublicEncryptRaw:\n");
    /* the input data size must equal the public key size */
    if (decrypt_data_size != nbytes) {
	printf("TPM_RSAPublicEncryptRaw: Error, decrypt data size is %u not %u\n",
		   decrypt_data_size, nbytes);
	rc = TPM_ENCRYPT_ERROR;
	goto exit;
    }

    /* the output data size must equal the public key size */
    if (encrypt_data_size != nbytes) {
	printf("TPM_RSAPublicEncryptRaw: Error, Encrypted data size is %u not %u\n",
		   decrypt_data_size, nbytes);
	rc = TPM_ENCRYPT_ERROR;
	goto exit;
    }

    /* construct the OpenSSL public key object */
    rc = TPM_RSAGeneratePublicToken(&rsa_pub_key,	/* freed @1 */
					narr,      	/* public modulus */
					nbytes,
					earr,      	/* public exponent */
					ebytes);
    if (rc)
	goto exit;

    TPM_PrintFour("  TPM_RSAPublicEncryptRaw: Public modulus", narr);
    TPM_PrintAll("  TPM_RSAPublicEncryptRaw: Public exponent", earr, ebytes);
    TPM_PrintFour("  TPM_RSAPublicEncryptRaw: Decrypt data", decrypt_data);
    /* encrypt the decrypt_data */
    irc = RSA_public_encrypt(decrypt_data_size,      	/* from length */
                                 decrypt_data,     	/* from - the clear text data */
                                 encrypt_data,         	/* to - the padded and encrypted data */
                                 rsa_pub_key,           /* key */
                                 RSA_NO_PADDING);       /* padding */
    if (irc < 0) {
	printf("TPM_RSAPublicEncryptRaw: Error in RSA_public_encrypt()\n");
	rc = TPM_ENCRYPT_ERROR;
	goto exit;
    }

    TPM_PrintFour("  TPM_RSAPublicEncryptRaw: Encrypt data", encrypt_data);
#if 0	/* NOTE: Uncomment as a debug aid for signature verification */
    TPM_PrintAll("  TPM_RSAPublicEncryptRaw: Padded signed data",
		     encrypt_data, encrypt_data_size);
#endif

exit:
    if (rsa_pub_key)
        RSA_free(rsa_pub_key);          /* @1 */
    return rc;
}

/* TPM_RSASign() signs 'message' of size 'message_size' using the private key n,e,d and the
   signature scheme 'sigScheme' as specified in PKCS #1 v2.0.

   'signature_length' bytes are moved to 'signature'.  'signature_length' is at most
   'signature_size'.  signature must point to RSA_size(rsa) bytes of memory.
*/
TPM_RESULT TPM_RSASign(unsigned char *signature,        /* output */
                       unsigned int *signature_length,  /* output, size of signature */
                       unsigned int signature_size,     /* input, size of signature buffer */
                       TPM_SIG_SCHEME sigScheme,        /* input, type of signature */
                       const unsigned char *message,    /* input */
                       size_t message_size,             /* input */
                       unsigned char *narr,             /* public modulus */
                       uint32_t nbytes,
                       unsigned char *earr,             /* public exponent */
                       uint32_t ebytes,
                       unsigned char *darr,             /* private exponent */
                       uint32_t dbytes)
{
    TPM_RESULT          rc = 0;
    RSA *               rsa_pri_key = NULL;	/* freed @1 */
    unsigned int        key_size;

    printf(" TPM_RSASign:\n");
    /* construct the OpenSSL private key object */
    rc = TPM_RSAGeneratePrivateToken(&rsa_pri_key,	/* freed @1 */
					 narr,      	/* public modulus */
					 nbytes,
					 earr,      	/* public exponent */
					 ebytes,
					 darr,		/* private exponent */
					 dbytes);
    if (rc)
	goto exit;

    /* check the size of the output signature buffer */
    key_size = (unsigned int)RSA_size(rsa_pri_key); /* openSSL returns an int, but never
                                                           negative */
    if (signature_size < key_size) {
	printf("TPM_RSASign: Error (fatal), buffer %u too small for signature %u\n",
                   signature_size, key_size);
	rc = TPM_FAIL;      /* internal error, should never occur */
    }
    if (rc)
	goto exit;

    /* determine the signature scheme for the key */
    switch(sigScheme) {
    case TPM_SS_NONE:
	printf("TPM_RSASign: Error, sigScheme TPM_SS_NONE\n");
	rc = TPM_INVALID_KEYUSAGE;
	break;
    case TPM_SS_RSASSAPKCS1v15_SHA1:
    case TPM_SS_RSASSAPKCS1v15_INFO:
	rc = TPM_RSASignSHA1(signature,
                                 signature_length,
                                 message,
                                 message_size,
                                 rsa_pri_key);
	break;
    case TPM_SS_RSASSAPKCS1v15_DER:
	rc = TPM_RSASignDER(signature,
                                signature_length,
                                message,
                                message_size,
                                rsa_pri_key);
	break;
    default:
	printf("TPM_RSASign: Error, sigScheme %04hx unknown\n", sigScheme);
	rc = TPM_INVALID_KEYUSAGE;
	break;
    }
    if (rc)
	goto exit;

exit:
    if (rsa_pri_key)
	RSA_free(rsa_pri_key);		/* @1 */
    return rc;
}

/* TPM_RSAVerifySHA1() performs the following:
        decrypt the signature
        verify and remove type 1 pad
        verify and remove DER encoded algorithm ID
        verify the signature on the message
*/

TPM_RESULT TPM_RSAVerifySHA1(unsigned char *signature,		/* input */
			     unsigned int signature_size, 	/* input, size of signature
								   buffer */
			     const unsigned char *message,	/* input */
			     uint32_t message_size,             /* input */
			     unsigned char *narr,		/* public modulus */
			     uint32_t nbytes,
			     unsigned char *earr,		/* public exponent */
			     uint32_t ebytes)
{
    TPM_RESULT  rc = 0;
    TPM_BOOL 	valid;
    RSA *       rsa_pub_key = NULL;
    
    printf(" TPM_RSAVerifySHA1:\n");
    /* construct the openSSL public key object from n and e */
    rc = TPM_RSAGeneratePublicToken(&rsa_pub_key,	/* freed @1 */
					narr,      	/* public modulus */
					nbytes,
					earr,      	/* public exponent */
					ebytes);
    if (rc)
	goto exit;

    /* RSA_verify() returns 1 on successful verification, 0 otherwise. */
    valid = RSA_verify(NID_sha1, message, message_size,
			   signature, signature_size, rsa_pub_key);
    if (valid != 1) {
	printf("TPM_RSAVerifySHA1: Error, bad signature\n");
	rc = TPM_BAD_SIGNATURE;
	goto exit;
    }

exit:
    if (rsa_pub_key)
	RSA_free(rsa_pub_key);		/* @1 */
    return rc;
}

/* TPM_RSAGetPrivateKey recalculates q (2nd prime factor) and d (private key) from n (public key), e
   (public exponent), and p (1st prime factor)

   The private key is validated by dividing the RSA product n by the RSA prime p and verifying that
   the remainder is 0.

   'qarr', darr' must be freed by the caller.
*/
TPM_RESULT TPM_RSAGetPrivateKey(uint32_t *qbytes, unsigned char **qarr,
                                uint32_t *dbytes, unsigned char **darr,
                                uint32_t nbytes, unsigned char *narr,
                                uint32_t ebytes, unsigned char *earr,
                                uint32_t pbytes, unsigned char *parr)
{
    TPM_RESULT          rc = 0;         /* TPM return code */
    int                 irc;            /* openSSL return code */
    BIGNUM              *brc;           /* BIGNUM return code */

    BIGNUM *n = NULL;           	/* public modulus */
    BIGNUM *e = NULL;           	/* public exponent */
    BIGNUM *d = NULL;           	/* private exponent */
    BIGNUM *p = NULL;           	/* secret prime factor */
    BIGNUM *q = NULL;           	/* secret prime factor */
    /* temporary variables */
    BN_CTX *ctx = NULL;			/* freed @5, @6 */
    BIGNUM *r0 = NULL;          	/* n/p remainder */
    BIGNUM *r1 = NULL;
    BIGNUM *r2 = NULL;

    /* set to NULL so caller can free after failure */
    printf(" TPM_RSAGetPrivateKey:\n");
    *qarr = NULL;
    *darr = NULL;

    /* check input parameters */
    if ((narr == NULL) || (nbytes == 0)) {
	printf("TPM_RSAGetPrivateKey: Error, missing n\n");
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* check input parameters */
    if ((earr == NULL) || (ebytes == 0)) {
	printf("TPM_RSAGetPrivateKey: Error, missing e\n");
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* check input parameters */
    if ((parr == NULL) || (pbytes == 0)) {
	printf("TPM_RSAGetPrivateKey: Error, missing p\n");
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* get some temporary BIGNUM's for use in the calculations */
    rc = TPM_BN_CTX_new(&ctx);
    if (rc)
	goto exit;

    BN_CTX_start(ctx);      /* no return code */
    r0 = BN_CTX_get(ctx);   /* sufficient to test return of last 'get' call */
    r1 = BN_CTX_get(ctx);
    r2 = BN_CTX_get(ctx);
    if (r2 == 0) {
	printf("TPM_RSAGetPrivateKey: Error in BN_CTX_get()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_SIZE;
	goto exit;
    }
    /* allocate BIGNUM's for q, d */
    rc = TPM_BN_new((TPM_BIGNUM *)&q);
    if (rc)
	goto exit;
    rc = TPM_BN_new((TPM_BIGNUM *)&d);
    if (rc)
	goto exit;
    /* convert n, e, p to BIGNUM's */
    rc = TPM_bin2bn((TPM_BIGNUM *)&n, narr, nbytes);	/* freed @1 */
    if (rc)
	goto exit;
    rc = TPM_bin2bn((TPM_BIGNUM *)&e, earr, ebytes);	/* freed @2 */
    if (rc)
	goto exit;
    rc = TPM_bin2bn((TPM_BIGNUM *)&p, parr, pbytes);	/* freed @3 */
    if (rc)
	goto exit;

    /* calculate q = n/p */
    irc = BN_div(q, r0, n, p, ctx);         /* q = n/p freed @4 */
    if (irc != 1) {         /* 1 is success */
	printf("TPM_RSAGetPrivateKey: Error in BN_div()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* remainder should be zero */
    irc = BN_is_zero(r0);
    if (irc != 1) {         /* 1 is success */
	printf("TPM_RSAGetPrivateKey: Error in BN_is_zero()\n");
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* calculate r0 = p-1 */
    irc = BN_sub(r0, p, BN_value_one());    /* r0 = p-1 freed @6 */
    if (irc != 1) {         /* 1 is success */
	printf("TPM_RSAGetPrivateKey: Error in BN_sub()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* calculate r1 = q-1 */
    irc = BN_sub(r1, q, BN_value_one());    /* freed @6 */
    if (irc != 1) {         /* 1 is success */
	printf("TPM_RSAGetPrivateKey: Error in BN_sub()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* calculate r2 = (p-1)(q-1) */
    irc = BN_mul(r2, r0, r1, ctx);          /* freed @6 */
    if (irc != 1) {         /* 1 is success */
	printf("TPM_RSAGetPrivateKey: Error in BN_mul()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* calculate d  = multiplicative inverse e mod r0 */
    brc = BN_mod_inverse(d, e, r2, ctx);    /* feed @5 */
    if (brc == NULL) {
	printf("TPM_RSAGetPrivateKey: Error in BN_mod_inverse()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_BAD_PARAMETER;
	goto exit;
    }

    /* get q as an array */
    rc = TPM_bn2binMalloc(qarr, qbytes, (TPM_BIGNUM)q, pbytes);	/* freed by caller */
    if (rc)
	goto exit;

    /* get d as an array */
    TPM_PrintFour("  TPM_RSAGetPrivateKey: Calculated q",  *qarr);
    rc = TPM_bn2binMalloc(darr, dbytes, (TPM_BIGNUM)d, nbytes);	/* freed by caller */
    if (rc)
	goto exit;

    TPM_PrintFour("  TPM_RSAGetPrivateKey: Calculated d",  *darr);
    printf("  TPM_RSAGetPrivateKey: length of n,p,q,d = %u / %u / %u / %u\n",
               nbytes, pbytes, *qbytes, *dbytes);

exit:
    if (n)
	BN_free(n);		/* @1 */
    if (e)
	BN_free(e);		/* @2 */
    if (p)
	BN_free(p);		/* @3 */
    if (q)
	BN_free(q);		/* @4 */
    if (d)
	BN_free(d);		/* @3 */
    if (ctx) {
	BN_CTX_end(ctx);	/* @5 */
	BN_CTX_free(ctx);	/* @6 */
    }
    return rc;
}

/*
  openSSL wrappers do error logging and transformation of openSSL errors to TPM type errors
*/

/* TPM_BN_num_bytes() wraps the openSSL function in a TPM error handler
 
   Returns number of bytes in the input
*/
TPM_RESULT TPM_BN_num_bytes(unsigned int *numBytes, TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    int         i;
    BIGNUM	*bn = (BIGNUM *)bn_in;
    
    i = BN_num_bytes(bn);
    if (i >= 0) {
        *numBytes = (unsigned int)i;
    } else {
        printf("TPM_BN_num_bytes: Error (fatal), bytes in BIGNUM is negative\n");
        TPM_OpenSSL_PrintError();
        rc = TPM_FAIL;
    }
    return rc;
}

/* TPM_BN_is_one() wraps the openSSL function in a TPM error handler

   Returns success if input is 1
 */

TPM_RESULT TPM_BN_is_one(TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM	*bn = (BIGNUM *)bn_in;

    /* int BN_is_one(BIGNUM *a);
       BN_is_one() tests if a equals 0, 1,
       BN_is_one() returns 1 if the condition is true, 0 otherwise. */
    irc = BN_is_one(bn);
    if (irc != 1) {
        printf("TPM_BN_is_one: Error, result is not 1\n");
        rc = TPM_DAA_WRONG_W;
    }
    return rc;
}

/* TPM_BN_mod() wraps the openSSL function in a TPM error handler

   r = a mod m
 */

TPM_RESULT TPM_BN_mod(TPM_BIGNUM rem_in,
		      const TPM_BIGNUM a_in,
		      const TPM_BIGNUM m_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM	*rem = (BIGNUM *)rem_in;
    BIGNUM	*a = (BIGNUM *)a_in;
    BIGNUM	*m = (BIGNUM *)m_in;
    BN_CTX	*ctx = NULL;			/* freed @1 */

    rc = TPM_BN_CTX_new(&ctx);		/* freed @1 */
    if (rc)
	goto exit;

    /*int BN_mod(BIGNUM *rem, const BIGNUM *a, const BIGNUM *m, BN_CTX *ctx);
      BN_mod() corresponds to BN_div() with dv set to NULL.

      int BN_div(BIGNUM *dv, BIGNUM *rem, const BIGNUM *a, const BIGNUM *d, BN_CTX *ctx);

      BN_div() divides a by d and places the result in dv and the remainder in rem (dv=a/d,
      rem=a%d). Either of dv and rem may be NULL, in which case the respective value is not
      returned. The result is rounded towards zero; thus if a is negative, the remainder will be
      zero or negative. For division by powers of 2, use BN_rshift(3).

      For all functions, 1 is returned for success, 0 on error. The return value should always be
      checked
    */
    irc = BN_mod(rem, a, m, ctx);
    if (irc != 1) {
        printf("TPM_BN_mod: Error performing BN_mod()\n");
        TPM_OpenSSL_PrintError();
        rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    if (ctx)
	BN_CTX_free(ctx);	/* @1 */
    return rc;
}

/* TPM_BN_mask_bits() wraps the openSSL function in a TPM error handler

   erase all but the lowest n bits of bn
   bn  = bn mod 2^^n
*/

TPM_RESULT TPM_BN_mask_bits(TPM_BIGNUM bn_in, unsigned int n)
{
    TPM_RESULT          rc = 0;
    int                 irc;
    unsigned int        numBytes;
    BIGNUM		*bn = (BIGNUM *)bn_in;

    rc = TPM_BN_num_bytes(&numBytes, bn_in);
    if (rc)
	goto exit;

    /* if the BIGNUM is already the correct number of bytes, no need to mask, and BN_mask_bits()
       will fail. */

    if (numBytes > (n / 8)) {
	/* BN_mask_bits() truncates a to an n bit number (a&=~((~0)>>;n)).
	   An error occurs if a already is shorter than n bits.

           int BN_mask_bits(BIGNUM *a, int n);
           return 1 for success, 0 on error.
	*/
	irc = BN_mask_bits(bn, n);
	if (irc != 1) {
	    printf("TPM_BN_mask_bits: Error performing BN_mask_bits()\n");
	    TPM_OpenSSL_PrintError();
	    rc = TPM_DAA_WRONG_W;
	    goto exit;
	}
    }

exit:
    return rc;
}

/* TPM_BN_rshift() wraps the openSSL function in a TPM error handler

   Shift a right by n bits (discard the lowest n bits) and label the result r
*/
TPM_RESULT TPM_BN_rshift(TPM_BIGNUM *rBignum_in,              /* freed by caller */
                         TPM_BIGNUM aBignum_in,
                         int n)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM	**rBignum = (BIGNUM **)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    
    printf(" TPM_BN_rshift: n %d\n", n);
    rc = TPM_BN_new(rBignum_in);
    if (rc)
	goto exit;

    /* BN_rshift() shifts a right by n bits and places result in r (r=a/2^n).
       int BN_rshift(BIGNUM *r, BIGNUM *a, int n);
       return 1 for success, 0 on error.
    */
    irc = BN_rshift(*rBignum, aBignum, n);
    if (irc != 1) {
	printf("TPM_BN_rshift: Error performing BN_rshift()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    return rc;
}

/* TPM_BN_lshift() wraps the openSSL function in a TPM error handler

   Shift a left by n bits and label the result r
*/
TPM_RESULT TPM_BN_lshift(TPM_BIGNUM *rBignum_in,              /* freed by caller */
                         TPM_BIGNUM aBignum_in,
                         int n)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM	**rBignum = (BIGNUM **)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    
    printf(" TPM_BN_lshift: n %d\n", n);
    rc = TPM_BN_new(rBignum_in);
    if (rc)
	goto exit;

    /* BN_lshift() shifts a left by n bits and places the result in r (r=a*2^n).
       int BN_lshift(BIGNUM *r, const BIGNUM *a, int n);
       return 1 for success, 0 on error.
    */
    irc = BN_lshift(*rBignum, aBignum, n);
    if (irc != 1) {
	printf("TPM_lshift: Error performing BN_lshift()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    return rc;
}

/* TPM_BN_add() wraps the openSSL function in a TPM error handler

   Performs R = A + B

   R may be the same as A or B
*/
TPM_RESULT TPM_BN_add(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BIGNUM	*rBignum = (BIGNUM *)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    BIGNUM	*bBignum = (BIGNUM *)bBignum_in;

    printf(" TPM_BN_add:\n");
    /* int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);
       BN_add() adds a and b and places the result in r (r=a+b). r may be the same BIGNUM as a or b.
       1 is returned for success, 0 on error.
    */
    irc = BN_add(rBignum, aBignum, bBignum); 
    if (irc != 1) {
        printf("TPM_BN_add: Error performing BN_add()\n");
        TPM_OpenSSL_PrintError();
        rc = TPM_DAA_WRONG_W;
    }
    return rc;
}

/* TPM_BN_mul() wraps the openSSL function in a TPM error handler

   r = a * b
*/
TPM_RESULT TPM_BN_mul(TPM_BIGNUM rBignum_in,
                      TPM_BIGNUM aBignum_in,
                      TPM_BIGNUM bBignum_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BN_CTX	* ctx = NULL;		/* freed @1 */
    BIGNUM	*rBignum = (BIGNUM *)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    BIGNUM	*bBignum = (BIGNUM *)bBignum_in;

    printf(" TPM_BN_mul:\n");
    rc = TPM_BN_CTX_new(&ctx);	/* freed @1 */
    if (rc)
	goto exit;

    /* int BN_mul(BIGNUM *r, BIGNUM *a, BIGNUM *b, BN_CTX *ctx);
       BN_mul() multiplies a and b and places the result in r (r=a*b). r may be the same BIGNUM as a
       or b.
       1 is returned for success, 0 on error.
    */
    irc = BN_mul(rBignum, aBignum, bBignum, ctx); 
    if (irc != 1) {
	printf("TPM_BN_add: Error performing BN_mul()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    if (ctx)
	BN_CTX_free(ctx);	/* @1 */
    return rc;
}

/* TPM_BN_mod_exp() wraps the openSSL function in a TPM error handler

   computes a to the p-th power modulo m (r=a^p % n)
*/
TPM_RESULT TPM_BN_mod_exp(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM pBignum_in,
                          TPM_BIGNUM nBignum_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BN_CTX	* ctx = NULL;			/* freed @1 */
    BIGNUM	*rBignum = (BIGNUM *)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    BIGNUM	*pBignum = (BIGNUM *)pBignum_in;
    BIGNUM	*nBignum = (BIGNUM *)nBignum_in;
    
    printf(" TPM_BN_mod_exp:\n");
    rc = TPM_BN_CTX_new(&ctx);
    if (rc)
	goto exit;

    /* BIGNUM calculation */
    /* int BN_mod_exp(BIGNUM *r, BIGNUM *a, const BIGNUM *p, const BIGNUM *m, BN_CTX *ctx);

    BN_mod_exp() computes a to the p-th power modulo m (r=a^p % m). This function uses less time
    and space than BN_exp().

    1 is returned for success, 0 on error.
    */
    printf("  TPM_BN_mod_exp: Calculate mod_exp\n");
    irc = BN_mod_exp(rBignum, aBignum, pBignum, nBignum, ctx);
    if (irc != 1) {
	printf("TPM_BN_mod_exp: Error performing BN_mod_exp()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    if (ctx)
	BN_CTX_free(ctx);		/* @1 */
    return rc;
}

/* TPM_BN_Mod_add() wraps the openSSL function in a TPM error handler

   adds a to b modulo m
*/
TPM_RESULT TPM_BN_mod_add(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BN_CTX      * ctx = NULL;			/* freed @1 */
    BIGNUM	*rBignum = (BIGNUM *)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    BIGNUM	*bBignum = (BIGNUM *)bBignum_in;
    BIGNUM	*mBignum = (BIGNUM *)mBignum_in;

    printf(" TPM_BN_mod_add:\n");
    rc = TPM_BN_CTX_new(&ctx);
    if (rc)
	goto exit;

    /* int BN_mod_add(BIGNUM *r, BIGNUM *a, BIGNUM *b, const BIGNUM *m, BN_CTX *ctx);
       BN_mod_add() adds a to b modulo m and places the non-negative result in r.
       1 is returned for success, 0 on error.
    */
    irc = BN_mod_add(rBignum, aBignum, bBignum, mBignum, ctx); 
    if (irc != 1) {
	printf("TPM_BN_mod_add: Error performing BN_mod_add()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    if (ctx)
	BN_CTX_free(ctx);		/* @1 */
    return rc;
}

/* TPM_BN_mod_mul() wraps the openSSL function in a TPM error handler

   r = (a * b) mod m
 */
TPM_RESULT TPM_BN_mod_mul(TPM_BIGNUM rBignum_in,
                          TPM_BIGNUM aBignum_in,
                          TPM_BIGNUM bBignum_in,
                          TPM_BIGNUM mBignum_in)
{
    TPM_RESULT  rc = 0;
    int         irc;
    BN_CTX      * ctx = NULL;			/* freed @1 */
    BIGNUM	*rBignum = (BIGNUM *)rBignum_in;
    BIGNUM	*aBignum = (BIGNUM *)aBignum_in;
    BIGNUM	*bBignum = (BIGNUM *)bBignum_in;
    BIGNUM	*mBignum = (BIGNUM *)mBignum_in;

    printf(" TPM_BN_mod_mul:\n");
    rc = TPM_BN_CTX_new(&ctx);
    if (rc)
	goto exit;

    /*  int BN_mod_mul(BIGNUM *r, BIGNUM *a, BIGNUM *b, const BIGNUM *m, BN_CTX *ctx);
        BN_mod_mul() multiplies a by b and finds the non-negative remainder respective to modulus m
        (r=(a*b) mod m). r may be the same BIGNUM as a or b.
        1 is returned for success, 0 on error.
    */
    irc = BN_mod_mul(rBignum, aBignum, bBignum, mBignum, ctx);
    if (irc != 1) {
	printf("TPM_BN_mod_mul: Error performing BN_mod_mul()\n");
	TPM_OpenSSL_PrintError();
	rc = TPM_DAA_WRONG_W;
	goto exit;
    }

exit:
    if (ctx)
	BN_CTX_free(ctx);		/* @1 */
    return rc;
}
     
/* TPM_BN_new() wraps the openSSL function in a TPM error handler
   Allocates a new bignum
*/
TPM_RESULT TPM_BN_new(TPM_BIGNUM *bn_in) 
{
    TPM_RESULT  rc = 0;
    BIGNUM	**bn = (BIGNUM **)bn_in;

    *bn  = BN_new();
    if (*bn == NULL) {
        printf("TPM_BN_new: Error, bn is NULL\n");
        TPM_OpenSSL_PrintError();
        rc = TPM_SIZE;
    }
    return rc;
}

/* TPM_BN_free() wraps the openSSL function
   Frees the bignum
*/
void TPM_BN_free(TPM_BIGNUM bn_in)
{
    BIGNUM	*bn = (BIGNUM *)bn_in;
    
    BN_free(bn);
    return;
}

/* TPM_bn2bin wraps the openSSL function in a TPM error handler.

   Converts a bignum to char array

   'bin' must already be checked for sufficient size.

   int BN_bn2bin(const BIGNUM *a, unsigned char *to);
   BN_bn2bin() returns the length of the big-endian number placed at to
*/
TPM_RESULT TPM_bn2bin(unsigned char *bin, TPM_BIGNUM bn_in)
{
    TPM_RESULT  rc = 0;
    BN_bn2bin((BIGNUM *)bn_in, bin);
    return rc;
}


/* Hash Functions */

/* for the openSSL version, TPM_SHA1Context is a SHA_CTX structure */

/* TPM_SHA1InitCmd() initializes a platform dependent TPM_SHA1Context structure.

   The structure must be freed using TPM_SHA1Delete()
*/
TPM_RESULT TPM_SHA1InitCmd(void **context)
{
    TPM_RESULT  rc = 0;

    printf(" TPM_SHA1InitCmd:\n");
    rc = TPM_Malloc((unsigned char **)context, sizeof(SHA_CTX));
    if (rc)
	goto exit;

    rc = (SHA1_Init(*context) ? 0 : TPM_SHA_ERROR);

exit:
    return rc;
}

/* TPM_SHA1UpdateCmd() adds 'data' of 'length' to the SHA-1 context */
TPM_RESULT TPM_SHA1UpdateCmd(void *context, const unsigned char *data, uint32_t length)
{
    TPM_RESULT  rc = 0;
    
    printf(" TPM_SHA1Update: length %u\n", length);
    if (context) {
        rc = (SHA1_Update(context, data, length) ? 0 : TPM_SHA_ERROR);
    } else {
        printf("TPM_SHA1Update: Error, no existing SHA1 thread\n");
        rc = TPM_SHA_THREAD;
    }
    return rc;
}

/* TPM_SHA1FinalCmd() extracts the SHA-1 digest 'md' from the context */
TPM_RESULT TPM_SHA1FinalCmd(unsigned char *md, void *context)
{
    TPM_RESULT  rc = 0;
    
    printf(" TPM_SHA1FinalCmd:\n");
    if (context) {
        rc = (SHA1_Final(md, context) ? 0 : TPM_SHA_ERROR);
    } else {
        printf("TPM_SHA1FinalCmd: Error, no existing SHA1 thread\n");
        rc = TPM_SHA_THREAD;
    }
    return rc;
}

/* TPM_SHA1Delete() zeros and frees the SHA1 context */
void TPM_SHA1Delete(void **context)
{
    if (*context != NULL) {
        printf(" TPM_SHA1Delete:\n");
	/* zero because the SHA1 context might have data left from an HMAC */
        memset(*context, 0, sizeof(SHA_CTX));
        free(*context);
        *context = NULL;
    }
    return;
}

/* TPM_Sha1Context_Load() is non-portable code to deserialize the OpenSSL SHA1 context.

   If the contextPresent prepended by TPM_Sha1Context_Store() is FALSE, context remains NULL.  If
   TRUE, context is allocated and loaded.
*/
TPM_RESULT TPM_Sha1Context_Load(void **context,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT 	rc = 0;
    size_t 	i;
    SHA_CTX 	*sha_ctx = NULL;	/* initialize to silence hopefully bogus gcc 4.4.4
					   warning */
    TPM_BOOL	contextPresent;		/* is there a context to be loaded */

    printf(" TPM_Sha1Context_Load: OpenSSL\n");
    /* TPM_Sha1Context_Store() stored a flag to indicate whether a context should be stored */
    rc = TPM_LoadBool(&contextPresent, stream, stream_size);
    printf(" TPM_Sha1Context_Load: contextPresent %u\n", contextPresent);
    if (contextPresent == FALSE)
	goto exit;

    /* check format tag */
    /* In the future, if multiple formats are supported, this check will be replaced by a 'switch'
       on the tag */
    rc = TPM_CheckTag(TPM_TAG_SHA1CONTEXT_OSSL_V1, stream, stream_size);
    if (rc)
	goto exit;
    rc = TPM_Malloc((unsigned char **)context, sizeof(SHA_CTX));
    if (rc)
	goto exit;
    sha_ctx = (SHA_CTX *)*context;
    /* load h0 */
    rc = TPM_Load32(&(sha_ctx->h0), stream, stream_size);
    if (rc)
	goto exit;
    /* load h1 */
    rc = TPM_Load32(&(sha_ctx->h1), stream, stream_size);
    if (rc)
	goto exit;
    /* load h2 */
    rc = TPM_Load32(&(sha_ctx->h2), stream, stream_size);
    if (rc)
	goto exit;
    /* load h3 */
    rc = TPM_Load32(&(sha_ctx->h3), stream, stream_size);
    if (rc)
	goto exit;
    /* load h4 */
    rc = TPM_Load32(&(sha_ctx->h4), stream, stream_size);
    if (rc)
	goto exit;
    /* load Nl */
    rc = TPM_Load32(&(sha_ctx->Nl), stream, stream_size);
    if (rc)
	goto exit;
    /* load Nh */
    rc = TPM_Load32(&(sha_ctx->Nh), stream, stream_size);
    if (rc)
	goto exit;
    /* load data */
    for (i = 0 ; i < SHA_LBLOCK ; i++) {
	rc = TPM_Load32(&(sha_ctx->data[i]), stream, stream_size);
	if (rc)
	    goto exit;
    }
    /* load num */
    rc = TPM_Load32(&(sha_ctx->num), stream, stream_size);
    if (rc)
	goto exit;

exit:
    return rc;
}

/* TPM_Sha1Context_Store() is non-portable code to serialize the OpenSSL SHA1 context.  context is
   not altered.

   It prepends a contextPresent flag to the stream, FALSE if context is NULL, TRUE if not.
*/
TPM_RESULT TPM_Sha1Context_Store(TPM_STORE_BUFFER *sbuffer,
				 void *context)
{
    TPM_RESULT 	rc = 0;
    size_t 	i;
    SHA_CTX 	*sha_ctx = (SHA_CTX *)context;
    TPM_BOOL	contextPresent;		/* is there a context to be stored */

    printf(" TPM_Sha1Context_Store: OpenSSL\n");
    /* store contextPresent */
    if (sha_ctx == NULL) {
	printf("  TPM_Sha1Context_Store: No context to store\n");
	goto exit;
    }

    printf("  TPM_Sha1Context_Store: Storing context\n");
    rc = TPM_Sbuffer_Append(sbuffer, &contextPresent, sizeof(TPM_BOOL));
    if (rc || contextPresent == FALSE)
	goto exit;

    /* overall format tag */
    rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_SHA1CONTEXT_OSSL_V1);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->h0);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->h1);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->h2);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->h3);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->h4);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->Nl);
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->Nh);
    if (rc)
	goto exit;
    for (i = 0 ; i < SHA_LBLOCK ; i++) {
	rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->data[i]);
	if (rc)
	    goto exit;
    }
    rc = TPM_Sbuffer_Append32(sbuffer, sha_ctx->num);
    if (rc)
	goto exit;

exit:
    return rc;
}

/*
  TPM_SYMMETRIC_KEY_DATA
*/

/* TPM_SymmetricKeyData_New() allocates memory for and initializes a TPM_SYMMETRIC_KEY_DATA token.
 */

TPM_RESULT TPM_SymmetricKeyData_New(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_SymmetricKeyData_New:\n");
    rc = TPM_Malloc(tpm_symmetric_key_data, sizeof(TPM_SYMMETRIC_KEY_DATA));
    if (rc)
	goto exit;

    TPM_SymmetricKeyData_Init(*tpm_symmetric_key_data);

exit:
    return rc;
}

/* TPM_SymmetricKeyData_Free() initializes the key token to wipe secrets.  It then frees the
   TPM_SYMMETRIC_KEY_DATA token and sets it to NULL.
*/

void TPM_SymmetricKeyData_Free(TPM_SYMMETRIC_KEY_TOKEN *tpm_symmetric_key_data)
{
    printf(" TPM_SymmetricKeyData_Free:\n");
    if (*tpm_symmetric_key_data != NULL) {
        TPM_SymmetricKeyData_Init(*tpm_symmetric_key_data);
	free(*tpm_symmetric_key_data);
	*tpm_symmetric_key_data = NULL;
    }
    return;
}

#ifdef TPM_DES

/* TPM_SymmetricKeyData_Init() is DES non-portable code to initialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the TPM_SYMMETRIC_KEY_DATA declaration.
*/

void TPM_SymmetricKeyData_Init(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Init:\n");
    tpm_symmetric_key_data->tag = TPM_TAG_KEY;
    tpm_symmetric_key_data->valid = FALSE;
    tpm_symmetric_key_data->fill = 0;
    memset(tpm_symmetric_key_data->des_cblock1, 0, sizeof(DES_cblock));
    memset(tpm_symmetric_key_data->des_cblock2, 0, sizeof(DES_cblock));
    memset(tpm_symmetric_key_data->des_cblock3, 0, sizeof(DES_cblock));
    return;
}

/* TPM_SymmetricKeyData_Load() is DES non-portable code to deserialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the TPM_SYMMETRIC_KEY_DATA declaration.
*/

TPM_RESULT TPM_SymmetricKeyData_Load(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token,
                                     unsigned char **stream,
                                     uint32_t *stream_size)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
   
    printf(" TPM_SymmetricKeyData_Load:\n");
    /* check tag */
    rc = TPM_CheckTag(TPM_TAG_KEY, stream, stream_size);
    if (rc)
	goto exit;
    /* load valid */
    rc = TPM_LoadBool(&(tpm_symmetric_key_data->valid), stream, stream_size);
    if (rc)
	goto exit;
    /* load fill */
    rc = TPM_Load8(&(tpm_symmetric_key_data->fill), stream, stream_size);
    if (rc)
	goto exit;
    /* this assumes that DES_cblock is a consistently packed structure.  It is in fact an array of 8
       bytes for openSSL. */
    rc = TPM_Loadn(tpm_symmetric_key_data->des_cblock1, sizeof(DES_cblock),
                       stream, stream_size);
    if (rc)
	goto exit;
    rc = TPM_Loadn(tpm_symmetric_key_data->des_cblock2, sizeof(DES_cblock),
                       stream, stream_size);
    if (rc)
	goto exit;

    rc = TPM_Loadn(tpm_symmetric_key_data->des_cblock3, sizeof(DES_cblock),
                       stream, stream_size);
    if (rc)
	goto exit;

    TPM_PrintFour("  TPM_SymmetricKeyData_Load: des1", tpm_symmetric_key_data->des_cblock1);
    TPM_PrintFour("  TPM_SymmetricKeyData_Load: des2", tpm_symmetric_key_data->des_cblock2);
    TPM_PrintFour("  TPM_SymmetricKeyData_Load: des3", tpm_symmetric_key_data->des_cblock3);

exit:
    return rc;
}

/* TPM_SymmetricKeyData_Store() DES is non-portable code to serialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the TPM_SYMMETRIC_KEY_DATA declaration.
*/
TPM_RESULT TPM_SymmetricKeyData_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Store:\n");
    TPM_PrintFour("  TPM_SymmetricKeyData_Store: des1", tpm_symmetric_key_data->des_cblock1);
    TPM_PrintFour("  TPM_SymmetricKeyData_Store: des2", tpm_symmetric_key_data->des_cblock2);
    TPM_PrintFour("  TPM_SymmetricKeyData_Store: des3", tpm_symmetric_key_data->des_cblock3);

    /* store tag */
    rc = TPM_Sbuffer_Append16(sbuffer, tpm_symmetric_key_data->tag);
    if (rc)
	goto exit;
    /* store valid */s
    rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->valid), sizeof(TPM_BOOL));
    if (rc)
	goto exit;
    /* store fill */
    rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->fill), sizeof(TPM_BOOL));
    if (rc)
	goto exit;
    /* store DES key */
    rc = TPM_Sbuffer_Append(sbuffer, tpm_symmetric_key_data->des_cblock1, sizeof(DES_cblock));
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append(sbuffer, tpm_symmetric_key_data->des_cblock2, sizeof(DES_cblock));
    if (rc)
	goto exit;
    rc = TPM_Sbuffer_Append(sbuffer, tpm_symmetric_key_data->des_cblock3, sizeof(DES_cblock));
    if (rc)
	goto exit;

exit:
    return rc;
}

/* TPM_SymmetricKeyData_GenerateKey() is DES non-portable code to generate a symmetric key

   vsymmetric_key must be freed by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_GenerateKey(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_GenerateKey:\n");
    /* generate a random key */
    DES_random_key(&(tpm_symmetric_key_data->des_cblock1));
    DES_random_key(&(tpm_symmetric_key_data->des_cblock2));
    DES_random_key(&(tpm_symmetric_key_data->des_cblock3));
    /* sets the parity of the passed key to odd. */
    DES_set_odd_parity(&(tpm_symmetric_key_data->des_cblock1));
    DES_set_odd_parity(&(tpm_symmetric_key_data->des_cblock2));
    DES_set_odd_parity(&(tpm_symmetric_key_data->des_cblock3));
    TPM_PrintFour("  TPM_SymmetricKeyData_GenerateKey: des1",
                      tpm_symmetric_key_data->des_cblock1);
    TPM_PrintFour("  TPM_SymmetricKeyData_GenerateKey: des2",
                      tpm_symmetric_key_data->des_cblock2);
    TPM_PrintFour("  TPM_SymmetricKeyData_GenerateKey: des3",
                      tpm_symmetric_key_data->des_cblock3);
    tpm_symmetric_key_data->valid = TRUE;

exit:
    return rc;
}

/* TPM_SymmetricKeyData_Encrypt() is DES non-portable code to encrypt 'decrypt_data' to
   'encrypt_data'

   The stream is padded as per PKCS#7 / RFC2630

   'encrypt_data' must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_Encrypt(unsigned char **encrypt_data,   /* output, caller frees */
                                        uint32_t *encrypt_length,		/* output */
                                        const unsigned char *decrypt_data,      /* input */
                                        uint32_t decrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_token) 		/* input */
{
    TPM_RESULT          rc = 0;
    uint32_t		pad_length;
    unsigned char       * decrypt_data_pad = NULL;	/* freed @1 */
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Encrypt: Length %u\n", decrypt_length);
    /* calculate the pad length and padded data length */
    pad_length = TPM_DES_BLOCK_SIZE - (decrypt_length % TPM_DES_BLOCK_SIZE);
    *encrypt_length = decrypt_length + pad_length;
    printf("  TPM_SymmetricKeyData_Encrypt: Padded length %u pad length %u\n",
               *encrypt_length, pad_length);
    /* allocate memory for the encrypted response */
    rc = TPM_Malloc(encrypt_data, *encrypt_length);
    if (rc)
	goto exit;

    /* allocate memory for the padded decrypted data */
    rc = TPM_Malloc(&decrypt_data_pad, *encrypt_length);
    if (rc)
	goto exit;

    /* pad the decrypted clear text data */
    /* unpadded original data */
    memcpy(decrypt_data_pad, decrypt_data, decrypt_length);
    /* last gets pad = pad length */
    memset(decrypt_data_pad + decrypt_length, pad_length, pad_length);
    /* encrypt the padded input to the output */
    rc = TPM_SymmetricKeyData_Crypt(*encrypt_data,
                                        decrypt_data_pad,
                                        *encrypt_length,
                                        tpm_symmetric_key_data,
                                        DES_ENCRYPT,
                                        TPM_ENCRYPT_ERROR);
    if (rc)
	goto exit;

exit:
    if (decrypt_data_pad)
	free(decrypt_data_pad);		/* @1 */
    return rc;
}

/* TPM_SymmetricKeyData_Decrypt() is DES non-portable code to decrypt 'encrypt_data' to
   'decrypt_data'

   The stream must be padded as per PKCS#7 / RFC2630

   decrypt_data must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_Decrypt(unsigned char **decrypt_data,   /* output, caller frees */
                                        uint32_t *decrypt_length,	/* output */
                                        const unsigned char *encrypt_data,      /* input */
                                        uint32_t encrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_data) 		/* input */
{
    TPM_RESULT  rc = 0;
    uint32_t      pad_length;
    uint32_t      i;
    unsigned char *pad_data;
    
    printf(" TPM_SymmetricKeyData_Decrypt: Length %u\n", encrypt_length);
    /* sanity check encrypted length */
    if (encrypt_length < TPM_DES_BLOCK_SIZE) {
	printf("TPM_SymmetricKeyData_Decrypt: Error, bad length\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    /* allocate memory for the padded decrypted data */
    rc = TPM_Malloc(decrypt_data, encrypt_length);
    if (rc)
	goto exit;

    /* decrypt the input to the padded output */
    rc = TPM_SymmetricKeyData_Crypt(*decrypt_data,
                                        encrypt_data,
                                        encrypt_length,
                                        tpm_symmetric_key_data,
                                        DES_DECRYPT,
                                        TPM_DECRYPT_ERROR);
    if (rc)
	goto exit;

    /* get the pad length */
    /* get the pad length from the last byte */
    pad_length = (uint32_t)*(*decrypt_data + encrypt_length - 1);
    /* sanity check the pad length */
    printf(" TPM_SymmetricKeyData_Decrypt: Pad length %u\n", pad_length);
    if ((pad_length == 0) || (pad_length > TPM_DES_BLOCK_SIZE)) {
	printf("TPM_SymmetricKeyData_Decrypt: Error, illegal pad length\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    /* get the unpadded length */
    *decrypt_length = encrypt_length - pad_length;
    /* pad starting point */
    pad_data = *decrypt_data + *decrypt_length;
    /* sanity check the pad */
    for (i = 0 ; i < pad_length ; i++, pad_data++) {
	if (*pad_data != pad_length) {
	    printf("TPM_SymmetricKeyData_Decrypt: Error, bad pad %02x at index %u\n",
                       *pad_data, i);
	    rc = TPM_DECRYPT_ERROR;
	    goto exit;
	}
    }

exit:
    return rc;
}

#endif

#ifdef TPM_AES

/* TPM_SymmetricKeyData_Init() is AES non-portable code to initialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the TPM_SYMMETRIC_KEY_DATA declaration.
*/

void TPM_SymmetricKeyData_Init(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Init:\n");
    tpm_symmetric_key_data->tag = TPM_TAG_KEY;
    tpm_symmetric_key_data->valid = FALSE;
    tpm_symmetric_key_data->fill = 0;
    memset(tpm_symmetric_key_data->userKey, 0, sizeof(tpm_symmetric_key_data->userKey));
    memset(&(tpm_symmetric_key_data->aes_enc_key), 0, sizeof(tpm_symmetric_key_data->aes_enc_key));
    memset(&(tpm_symmetric_key_data->aes_dec_key), 0, sizeof(tpm_symmetric_key_data->aes_dec_key));
    return;
}

/* TPM_SymmetricKeyData_Load() is AES non-portable code to deserialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the above TPM_SYMMETRIC_KEY_DATA declaration.
*/
TPM_RESULT TPM_SymmetricKeyData_Load(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token,
                                     unsigned char **stream,
                                     uint32_t *stream_size)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Load:\n");
    /* check tag */
    rc = TPM_CheckTag(TPM_TAG_KEY, stream, stream_size);
    if (rc)
	goto exit;

    /* load valid */
    rc = TPM_LoadBool(&(tpm_symmetric_key_data->valid), stream, stream_size);
    if (rc)
	goto exit;

    /* load fill */
    rc = TPM_Load8(&(tpm_symmetric_key_data->fill), stream, stream_size);
    if (rc)
	goto exit;

    /* The AES key is a simple array. */
    rc = TPM_Loadn(tpm_symmetric_key_data->userKey, sizeof(tpm_symmetric_key_data->userKey),
                       stream, stream_size);
    if (rc)
	goto exit;

    /* reconstruct the internal AES keys */
    rc = TPM_SymmetricKeyData_SetKeys(tpm_symmetric_key_data);
    if (rc)
	goto exit;

exit:
    return rc;
}

/* TPM_SymmetricKeyData_Store() is AES non-portable code to serialize the TPM_SYMMETRIC_KEY_DATA

   It depends on the above TPM_SYMMETRIC_KEY_DATA declaration.
*/
TPM_RESULT TPM_SymmetricKeyData_Store(TPM_STORE_BUFFER *sbuffer,
                                      const TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Store:\n");
    /* store tag */
    rc = TPM_Sbuffer_Append16(sbuffer, tpm_symmetric_key_data->tag);
    if (rc)
	goto exit;

    /* store valid */
    rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->valid), sizeof(TPM_BOOL));
    if (rc)
	goto exit;

    /* store fill */
    rc = TPM_Sbuffer_Append(sbuffer, &(tpm_symmetric_key_data->fill), sizeof(TPM_BOOL));
    if (rc)
	goto exit;

    /* store AES key */
    rc = TPM_Sbuffer_Append(sbuffer,
                                tpm_symmetric_key_data->userKey,
                                sizeof(tpm_symmetric_key_data->userKey));
    if (rc)
	goto exit;
    /* No need to store the internal AES keys.  They are reconstructed on load */

exit:
    return rc;
}

/* TPM_SymmetricKeyData_GenerateKey() is AES non-portable code to generate a random symmetric key

   tpm_symmetric_key_data should be initialized before and after use
*/

TPM_RESULT TPM_SymmetricKeyData_GenerateKey(TPM_SYMMETRIC_KEY_TOKEN tpm_symmetric_key_token)
{
    TPM_RESULT rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_GenerateKey:\n");
    /* generate a random key */
    rc = TPM_Random(tpm_symmetric_key_data->userKey, sizeof(tpm_symmetric_key_data->userKey));
    if (rc)
	goto exit;
    /* construct the internal AES keys */
    rc = TPM_SymmetricKeyData_SetKeys(tpm_symmetric_key_data);
    if (rc)
	goto exit;

    tpm_symmetric_key_data->valid = TRUE;

exit:
    return rc;
}

/* TPM_SymmetricKeyData_Encrypt() is AES non-portable code to encrypt 'decrypt_data' to
   'encrypt_data'

   The stream is padded as per PKCS#7 / RFC2630

   'encrypt_data' must be free by the caller
*/
TPM_RESULT TPM_SymmetricKeyData_Encrypt(unsigned char **encrypt_data,   /* output, caller frees */
                                        uint32_t *encrypt_length,		/* output */
                                        const unsigned char *decrypt_data,	/* input */
                                        uint32_t decrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_token) 		/* input */
{
    TPM_RESULT          rc = 0;
    uint32_t              pad_length;
    unsigned char       * decrypt_data_pad = NULL;	/* freed @1 */
    unsigned char       ivec[TPM_AES_BLOCK_SIZE];	/* initial chaining vector */
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;

    printf(" TPM_SymmetricKeyData_Encrypt: Length %u\n", decrypt_length);
    /* calculate the pad length and padded data length */
    pad_length = TPM_AES_BLOCK_SIZE - (decrypt_length % TPM_AES_BLOCK_SIZE);
    *encrypt_length = decrypt_length + pad_length;
    printf("  TPM_SymmetricKeyData_Encrypt: Padded length %u pad length %u\n",
               *encrypt_length, pad_length);
    /* allocate memory for the encrypted response */
    rc = TPM_Malloc(encrypt_data, *encrypt_length);
    if (rc)
	goto exit;

    /* allocate memory for the padded decrypted data */
    rc = TPM_Malloc(&decrypt_data_pad, *encrypt_length);
    if (rc)
	goto exit;

    /* pad the decrypted clear text data */
    /* unpadded original data */
    memcpy(decrypt_data_pad, decrypt_data, decrypt_length);
    /* last gets pad = pad length */
    memset(decrypt_data_pad + decrypt_length, pad_length, pad_length);
    /* set the IV */
    memset(ivec, 0, sizeof(ivec));
    /* encrypt the padded input to the output */
    TPM_PrintFour("  TPM_SymmetricKeyData_Encrypt: Input", decrypt_data_pad);
    AES_cbc_encrypt(decrypt_data_pad,
                        *encrypt_data,
                        *encrypt_length,
                        &(tpm_symmetric_key_data->aes_enc_key),
                        ivec,
                        AES_ENCRYPT);
    TPM_PrintFour("  TPM_SymmetricKeyData_Encrypt: Output", *encrypt_data);

exit:
    if (decrypt_data_pad)
	free(decrypt_data_pad);		/* @1 */
    return rc;
}

/* TPM_SymmetricKeyData_Decrypt() is AES non-portable code to decrypt 'encrypt_data' to
   'decrypt_data'

   The stream must be padded as per PKCS#7 / RFC2630

   decrypt_data must be free by the caller
*/

TPM_RESULT TPM_SymmetricKeyData_Decrypt(unsigned char **decrypt_data,   /* output, caller frees */
                                        uint32_t *decrypt_length,		/* output */
                                        const unsigned char *encrypt_data,	/* input */
                                        uint32_t encrypt_length,		/* input */
                                        const TPM_SYMMETRIC_KEY_TOKEN
					tpm_symmetric_key_token) 		/* input */
{
    TPM_RESULT          rc = 0;
    uint32_t		pad_length;
    uint32_t		i;
    unsigned char       *pad_data;
    unsigned char       ivec[TPM_AES_BLOCK_SIZE];       /* initial chaining vector */
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data =
	(TPM_SYMMETRIC_KEY_DATA *)tpm_symmetric_key_token;
    
    printf(" TPM_SymmetricKeyData_Decrypt: Length %u\n", encrypt_length);
    /* sanity check encrypted length */
    if (encrypt_length < TPM_AES_BLOCK_SIZE) {
	printf("TPM_SymmetricKeyData_Decrypt: Error, bad length\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    /* allocate memory for the padded decrypted data */
    rc = TPM_Malloc(decrypt_data, encrypt_length);
    if (rc)
	goto exit;

    /* decrypt the input to the padded output */
    /* set the IV */
    memset(ivec, 0, sizeof(ivec));
    /* decrypt the padded input to the output */
    TPM_PrintFour("  TPM_SymmetricKeyData_Decrypt: Input", encrypt_data);
    AES_cbc_encrypt(encrypt_data,
                        *decrypt_data,
                        encrypt_length,
                        &(tpm_symmetric_key_data->aes_dec_key),
                        ivec,
                        AES_DECRYPT);
    TPM_PrintFour("  TPM_SymmetricKeyData_Decrypt: Output", *decrypt_data);

    /* get the pad length */
    /* get the pad length from the last byte */
    pad_length = (uint32_t)*(*decrypt_data + encrypt_length - 1);
    /* sanity check the pad length */
    printf(" TPM_SymmetricKeyData_Decrypt: Pad length %u\n", pad_length);
    if ((pad_length == 0) || (pad_length > TPM_AES_BLOCK_SIZE)) {
	printf("TPM_SymmetricKeyData_Decrypt: Error, illegal pad length\n");
	rc = TPM_DECRYPT_ERROR;
	goto exit;
    }

    /* get the unpadded length */
    *decrypt_length = encrypt_length - pad_length;
    /* pad starting point */
    pad_data = *decrypt_data + *decrypt_length;
    /* sanity check the pad */
    for (i = 0 ; i < pad_length ; i++, pad_data++) {
	if (*pad_data != pad_length) {
	    printf("TPM_SymmetricKeyData_Decrypt: Error, bad pad %02x at index %u\n",
			*pad_data, i);
	    rc = TPM_DECRYPT_ERROR;
	    goto exit;
	}
    }

exit:
    return rc;
}

/* TPM_SymmetricKeyData_CtrCrypt() does an encrypt or decrypt (they are the same XOR operation with
   a CTR mode pad) of 'data_in' to 'data_out'.

   NOTE: This function looks general, but is currently hard coded to AES128.

   'symmetric key' is the raw key, not converted to a non-portable form
   'ctr_in' is the initial CTR value before possible truncation
*/

TPM_RESULT TPM_SymmetricKeyData_CtrCrypt(unsigned char *data_out,               /* output */
                                         const unsigned char *data_in,          /* input */
                                         uint32_t data_size,			/* input */
                                         const unsigned char *symmetric_key,    /* input */
                                         uint32_t symmetric_key_size,		/* input */
                                         const unsigned char *ctr_in,		/* input */
                                         uint32_t ctr_in_size)			/* input */
{
    TPM_RESULT  rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data = NULL;	/* freed @1 */
    unsigned char ctr[TPM_AES_BLOCK_SIZE];

    printf(" TPM_SymmetricKeyData_CtrCrypt: data_size %u\n", data_size);
    /* allocate memory for the key token.  The token is opaque in the API, but at this low level,
       the code understands the TPM_SYMMETRIC_KEY_DATA structure */
    rc = TPM_SymmetricKeyData_New((TPM_SYMMETRIC_KEY_TOKEN *)&tpm_symmetric_key_data);
    if (rc)
	goto exit;

    /* convert the raw key to the AES key, truncating as required */
    rc = TPM_SymmetricKeyData_SetKey(tpm_symmetric_key_data,
                                         symmetric_key,
                                         symmetric_key_size);
    if (rc)
	goto exit;

    /* check the input CTR size, it can be truncated, but cannot be smaller than the AES key */
    if (ctr_in_size < sizeof(ctr)) {
	printf("  TPM_SymmetricKeyData_CtrCrypt: Error (fatal)"
                   ", CTR size %u too small for AES key\n", ctr_in_size);
	rc = TPM_FAIL;              /* should never occur */
	goto exit;
    }

    /* make a truncated copy of CTR, since AES_ctr128_encrypt alters the value */
    memcpy(ctr, ctr_in, sizeof(ctr));
    printf("  TPM_SymmetricKeyData_CtrCrypt: Calling AES in CTR mode\n");
    TPM_PrintFour("  TPM_SymmetricKeyData_CtrCrypt: CTR", ctr);
    rc = TPM_AES_ctr128_encrypt(data_out,
				    data_in,
				    data_size,
				    &(tpm_symmetric_key_data->aes_enc_key),
				    ctr);
    if (rc)
	goto exit;

exit:
    if (tpm_symmetric_key_data)
	TPM_SymmetricKeyData_Free((TPM_SYMMETRIC_KEY_TOKEN *)&tpm_symmetric_key_data);	/* @1 */
    return rc;
}

/* TPM_SymmetricKeyData_OfbCrypt() does an encrypt or decrypt (they are the same XOR operation with
   a OFB mode pad) of 'data_in' to 'data_out'

   NOTE: This function looks general, but is currently hard coded to AES128.

   'symmetric key' is the raw key, not converted to a non-portable form
   'ivec_in' is the initial IV value before possible truncation
*/

/* openSSL prototype

   void AES_ofb128_encrypt(const unsigned char *in,
                           unsigned char *out,
                           const unsigned long length,
                           const AES_KEY *key,
                           unsigned char *ivec,
                           int *num);
*/
TPM_RESULT TPM_SymmetricKeyData_OfbCrypt(unsigned char *data_out,       /* output */
                                         const unsigned char *data_in,  /* input */
                                         uint32_t data_size,		/* input */
                                         const unsigned char *symmetric_key,    /* in */
                                         uint32_t symmetric_key_size,		/* in */
                                         unsigned char *ivec_in,        /* input */
                                         uint32_t ivec_in_size)		/* input */
{
    TPM_RESULT  rc = 0;
    TPM_SYMMETRIC_KEY_DATA *tpm_symmetric_key_data = NULL;	/* freed @1 */
    unsigned char ivec[TPM_AES_BLOCK_SIZE];
    int num;

    printf(" TPM_SymmetricKeyData_OfbCrypt: data_size %u\n", data_size);
    /* allocate memory for the key token.  The token is opaque in the API, but at this low level,
       the code understands the TPM_SYMMETRIC_KEY_DATA structure */
    rc = TPM_SymmetricKeyData_New((TPM_SYMMETRIC_KEY_TOKEN *)&tpm_symmetric_key_data);
    if (rc)
	goto exit;

    /* convert the raw key to the AES key, truncating as required */
    rc = TPM_SymmetricKeyData_SetKey(tpm_symmetric_key_data,
                                         symmetric_key,
                                         symmetric_key_size);
    if (rc)
	goto exit;

    /* check the input OFB size, it can be truncated, but cannot be smaller than the AES key */
    if (ivec_in_size < sizeof(ivec)) {
	printf("  TPM_SymmetricKeyData_OfbCrypt: Error (fatal),"
                   "IV size %u too small for AES key\n", ivec_in_size);
	rc = TPM_FAIL;              /* should never occur */
	goto exit;
    }

    /* make a truncated copy of IV, since AES_ofb128_encrypt alters the value */
    memcpy(ivec, ivec_in, sizeof(ivec));
    num = 0;
    printf("  TPM_SymmetricKeyData_OfbCrypt: Calling AES in OFB mode\n");
    TPM_PrintFour("  TPM_SymmetricKeyData_OfbCrypt: IV", ivec);
    AES_ofb128_encrypt(data_in,
                           data_out,
                           data_size,
                           &(tpm_symmetric_key_data->aes_enc_key),
                           ivec,
                           &num);

exit:
    if (tpm_symmetric_key_data)
	TPM_SymmetricKeyData_Free((TPM_SYMMETRIC_KEY_TOKEN *)&tpm_symmetric_key_data);
    return rc;
}

#endif  /* TPM_AES */
