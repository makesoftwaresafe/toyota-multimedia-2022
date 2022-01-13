/********************************************************************************/
/*			     	TPM Testing Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: testing.c 4404 2011-02-08 18:29:35Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

uint32_t TPM_SelfTestFull()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SelfTestFull);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SelfTestFull");
    if (tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_ContinueSelfTest()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_ContinueSelfTest);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "ContinueSelfTest");
    if (tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}


uint32_t TPM_GetTestResult(char *buffer, uint32_t * bufferlen)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_GetTestResult);
    STACK_TPM_BUFFER(tpmdata)
    uint32_t len;

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "GetTestResult");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    if (tpmdata.used != 10 + 4 + len)
	ret = ERR_BAD_RESP;

    if (buffer && ret == 0) {
	*bufferlen = MIN(len, *bufferlen);
	memcpy(buffer,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *bufferlen);
    }

    return ret;
}

uint32_t TPM_CertifySelfTest(uint32_t keyhandle, unsigned char *usageAuth,	// HMAC key
			     unsigned char *antiReplay,
			     struct tpm_buffer * signature)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_CertifySelfTest);
    uint32_t ret;
    uint32_t keyhandle_no = htonl(keyhandle);
    uint32_t _sigSize;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    /* check input arguments */
    if (usageAuth) {
	unsigned char nonceodd[TPM_NONCE_SIZE];
	unsigned char authdata[TPM_NONCE_SIZE];
	TPM_BOOL c = 0;
	session sess;

	/* Open OSAP Session */
	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			      &sess,
			      usageAuth, TPM_ET_KEYHANDLE, keyhandle);
	if (ret)
	    return ret;

	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	/* move Network byte order data to variable for hmac calculation */
	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_HASH_SIZE, antiReplay, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l % L % o %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    TPM_HASH_SIZE, antiReplay,
			    TSS_Session_GetHandle(&sess),
			    TPM_HASH_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "CertifySelfTest - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	/* check the HMAC in the response */
	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &_sigSize);
	if (ret & ERR_MASK)
	    return ret;

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   TPM_U32_SIZE + _sigSize, TPM_DATA_OFFSET, 0, 0);
	if (ret)
	    return ret;
    } else {
	ret = TSS_buildbuff("00 c1 T l l %", &tpmdata,
			    ordinal_no,
			    keyhandle_no, TPM_HASH_SIZE, antiReplay);
	if (ret & ERR_MASK)
	    return ret;

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "CertifySelfTest");
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &_sigSize);
	if (ret & ERR_MASK)
	    return ret;
    }

    if (signature)
	SET_TPM_BUFFER(signature,
		       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		       _sigSize);

    return ret;
}
