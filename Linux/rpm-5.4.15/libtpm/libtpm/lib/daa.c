/********************************************************************************/
/*			     	TPM DAA Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: daa.c 4089 2010-06-09 00:50:31Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

uint32_t TPM_DAA_Join(uint32_t sesshandle, unsigned char *ownerauth,	// HMAC key
		      unsigned char stage,
		      unsigned char *inputData0, uint32_t inputData0Size,
		      unsigned char *inputData1, uint32_t inputData1Size,
		      unsigned char *outputData, uint32_t * outputDataSize)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_DAA_Join);
    uint32_t sesshandle_no = htonl(sesshandle);
    uint32_t inputData0Size_no = htonl(inputData0Size);
    uint32_t inputData1Size_no = htonl(inputData1Size);
    uint32_t ret;
    uint32_t len;
    session sess;

    /* check input arguments */
    if (inputData0 == NULL)
	inputData0Size = 0;
    if (inputData1 == NULL)
	inputData1Size = 0;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerauth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* move Network byte order data to variable for HMAC calculation */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, sizeof(BYTE), &stage,
		     TPM_U32_SIZE, &inputData0Size_no, inputData0Size,
		     inputData0, TPM_U32_SIZE, &inputData1Size_no,
		     inputData1Size, inputData1, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l o @ @ L % o %", &tpmdata,
			ordinal_no,
			sesshandle_no,
			stage,
			inputData0Size, inputData0,
			inputData1Size, inputData1,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "TPM_DAA_Join - AUTH1");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       TPM_U32_SIZE + len, TPM_DATA_OFFSET, 0, 0);
    if (ret)
	return ret;

    if (outputData != NULL) {
	*outputDataSize = MIN(*outputDataSize, len);
	memcpy(outputData,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *outputDataSize);
    }

    return ret;
}

uint32_t TPM_DAA_Sign(uint32_t sesshandle, unsigned char *ownerauth,	// HMAC key
		      unsigned char stage,
		      unsigned char *inputData0, uint32_t inputData0Size,
		      unsigned char *inputData1, uint32_t inputData1Size,
		      unsigned char *outputData, uint32_t * outputDataSize)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_DAA_Sign);
    uint32_t sesshandle_no = htonl(sesshandle);
    uint32_t inputData0Size_no = htonl(inputData0Size);
    uint32_t inputData1Size_no = htonl(inputData1Size);
    uint32_t ret;
    uint32_t len;
    session sess;

    /* check input arguments */
    if (inputData0 == NULL)
	inputData0Size = 0;
    if (inputData1 == NULL)
	inputData1Size = 0;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerauth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* move Network byte order data to variable for HMAC calculation */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, sizeof(BYTE), &stage,
		     TPM_U32_SIZE, &inputData0Size_no, inputData0Size,
		     inputData0, TPM_U32_SIZE, &inputData1Size_no,
		     inputData1Size, inputData1, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l o @ @ L % o %", &tpmdata,
			ordinal_no,
			sesshandle_no,
			stage,
			inputData0Size, inputData0,
			inputData1Size, inputData1,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "TPM_DAA_Join");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       TPM_U32_SIZE + len, TPM_DATA_OFFSET, 0, 0);

    if (ret)
	return ret;

    if (outputData != NULL) {
	*outputDataSize = MIN(*outputDataSize, len);
	memcpy(outputData,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *outputDataSize);
    }

    return ret;
}
