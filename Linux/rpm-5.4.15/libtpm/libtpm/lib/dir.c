/********************************************************************************/
/*			     	TPM Dir Routines				*/
/*			     Written by S. Berger 				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: dir.c 4089 2010-06-09 00:50:31Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

uint32_t TPM_DirWriteAuth(uint32_t dirIndex,
			  unsigned char *newValue,
			  unsigned char *ownerAuth)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_DirWriteAuth);
    uint32_t ret;
    uint32_t dirIndex_no = htonl(dirIndex);
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    session sess;
    int c = 0;

    /* check input arguments */
    if (ownerAuth == NULL || newValue == NULL)
	return ERR_NULL_ARG;

    /* Open OSAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);

    if (ret)
	return ret;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* move Network byte order data to variable for HMAC calculation */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE, &dirIndex_no,
		     TPM_HASH_SIZE, newValue, 0, 0);

    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l % L % o %", &tpmdata,
			ordinal_no,
			dirIndex_no,
			TPM_HASH_SIZE, newValue,
			TSS_Session_GetHandle(&sess),
			TPM_HASH_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "DirWriteAuth");
    TSS_SessionClose(&sess);

    if (ret)
	return ret;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    return ret;
}

uint32_t TPM_DirRead(uint32_t dirIndex, unsigned char *dirValueBuffer)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_DirRead);
    STACK_TPM_BUFFER(tpmdata)
    uint32_t dirIndex_no = htonl(dirIndex);

    ret = TSS_buildbuff("00 c1 T l l", &tpmdata, ordinal_no, dirIndex_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "DirRead");
    if (ret)
	return ret;

    if (tpmdata.used != 30)
	ret = ERR_BAD_RESP;

    if (dirValueBuffer != NULL)
	memcpy(dirValueBuffer, &tpmdata.buffer[TPM_DATA_OFFSET], 20);

    return ret;
}
