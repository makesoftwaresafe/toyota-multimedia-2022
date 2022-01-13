/********************************************************************************/
/*			     	TPM Signature Routines				*/
/*			     Written by J. Kravitz, S. Berger			*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: signature.c 4089 2010-06-09 00:50:31Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/* Sign some data                                                           */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the handle of the key to sign with                          */
/* keyauth   is the authorization data (password) for the parent key        */
/*           if null, it is assumed that the key has no authorization req   */
/* data      is a pointer to the data to be signed                          */
/* datalen   is the length of the data being signed                         */
/* sig       is a pointer to an area to receive the signature (<=256 bytes) */
/* siglen    is a pointer to an integer to receive the signature length     */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_Sign(uint32_t keyhandle, unsigned char *keyauth,
		  unsigned char *data, uint32_t datalen,
		  unsigned char *sig, uint32_t * siglen)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char pubauth[TPM_HASH_SIZE];
    unsigned char c = 0;
    uint32_t ordinal = htonl(TPM_ORD_Sign);
    uint32_t keyhndl = htonl(keyhandle);
    uint32_t datasize = htonl(datalen);
    uint32_t sigsize;

    /* check input arguments */
    if (data == NULL || sig == NULL)
	return ERR_NULL_ARG;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    if (keyauth) {	/* key requires authorization */
	session sess;
	/* 
	   generate odd nonce from data. This is
	   good, but for a test suite it should
	   be OK. I need to do this to be able to later on
	   verify the INFO type of signature.
	 */
	TSS_sha1(data, datalen, nonceodd);

	/* Open OIAP Session */
	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP | SESSION_DSAP,
			      &sess, keyauth, TPM_ET_KEYHANDLE, keyhandle);
	if (ret)
	    return ret;

	/* calculate authorization HMAC value */
	ret = TSS_authhmac(pubauth, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal, TPM_U32_SIZE,
			 &datasize, datalen, data, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l @ L % o %", &tpmdata,
			    ordinal,
			    keyhndl,
			    datalen, data,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, pubauth);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "Sign");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &sigsize);
	if (ret & ERR_MASK)
	    return ret;

	/* check the HMAC in the response */
	ret = TSS_checkhmac1(&tpmdata, ordinal, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   TPM_U32_SIZE, TPM_DATA_OFFSET, sigsize,
			   TPM_DATA_OFFSET + TPM_U32_SIZE, 0, 0);
	if (ret)
	    return ret;

	memcpy(sig, &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE], sigsize);
	*siglen = sigsize;
    } else {			/* key requires NO authorization */
	/* move Network byte order data to variables for hmac calculation */
	/* build the request buffer */
	ret = TSS_buildbuff("00 c1 T l l @", &tpmdata,
			    ordinal, keyhndl, datalen, data);
	if (ret & ERR_MASK)
	    return ret;

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "Sign");
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &sigsize);
	if (ret & ERR_MASK)
	    return ret;

	memcpy(sig, &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE], sigsize);
	*siglen = sigsize;
    }
    return 0;
}
