/********************************************************************************/
/*			     	TPM Key Maintenance Routines			*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: maintenance.c 4073 2010-04-30 14:44:14Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

/****************************************************************************/
uint32_t TPM_CreateMaintenanceArchive(TPM_BOOL generateRandom,
				      unsigned char *ownerAuth,
				      unsigned char *random,
				      uint32_t * randomSize,
				      unsigned char *archive,
				      uint32_t * archiveSize)
{
    ALLOC_TPM_BUFFER(tpmdata, 0)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_CreateMaintenanceArchive);
    uint32_t ret;
    session sess;
    uint32_t size1, size2;

    if (tpmdata == NULL)
	return ERR_MEM_ERR;

    /* check input arguments */
    if (ownerAuth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OSAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* calculate encrypted authorization value */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, 0, 0);

    if (ret) {
	TSS_SessionClose(&sess);
	goto exit;
    }
    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l o L % o %", tpmdata,
			ordinal_no,
			generateRandom,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	goto exit;
    }

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(tpmdata, "CreateMaintenanceArchive - AUTH1");
    TSS_SessionClose(&sess);
    if (ret)
	goto exit;

    /* check the HMAC in the response */
    ret = tpm_buffer_load32(tpmdata, TPM_DATA_OFFSET, &size1);
    if (ret & ERR_MASK)
	return ret;

    ret = tpm_buffer_load32(tpmdata, TPM_DATA_OFFSET + TPM_U32_SIZE + size1,
			  &size2);
    if (ret & ERR_MASK)
	return ret;

    ret = TSS_checkhmac1(tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       TPM_U32_SIZE + size1 + TPM_U32_SIZE + size2,
		       TPM_DATA_OFFSET, 0, 0);

    if (random) {
	*randomSize = MIN(*randomSize, size1);
	memcpy(random, &tpmdata->buffer[TPM_DATA_OFFSET], *randomSize);
    }

    if (archive) {
	*archiveSize = MIN(*archiveSize, size1);
	memcpy(random,
	       &tpmdata->buffer[TPM_DATA_OFFSET + TPM_U32_SIZE + size1],
	       *archiveSize);
    }
  exit:
    FREE_TPM_BUFFER(tpmdata);
    return ret;
}

uint32_t TPM_LoadMaintenanceArchive(unsigned char *ownerAuth)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_LoadMaintenanceArchive);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (ownerAuth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OSAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* calculate encrypted authorization value */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }
    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l L % o %", &tpmdata,
			ordinal_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }
    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "LoadMaintenanceArchive");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    return ret;
}

uint32_t TPM_KillMaintenanceFeature(unsigned char *ownerAuth)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_KillMaintenanceFeature);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (ownerAuth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OSAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* calculate encrypted authorization value */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }
    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l L % o %", &tpmdata,
			ordinal_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }
    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "KillMaintenanceFeature");
    TSS_SessionClose(&sess);

    if (ret)
	return ret;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    return ret;
}

uint32_t TPM_LoadManuMaintPub(unsigned char *nonce,
			      keydata * pubKey, unsigned char *digest)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_LoadManuMaintPub);
    STACK_TPM_BUFFER(serPubKey)
    uint32_t serPubKeySize;

    /* check input arguments */
    if (pubKey == NULL)
	return ERR_NULL_ARG;

    ret = TPM_WriteKeyPub(&serPubKey, pubKey);
    if (ret & ERR_MASK)
	return ret;
    serPubKeySize = ret;

    /* build the request buffer */
    ret = TSS_buildbuff("00 c1 T l % %", &tpmdata,
			ordinal_no,
			TPM_NONCE_SIZE, nonce,
			serPubKeySize, serPubKey.buffer);
    if (ret & ERR_MASK)
	return ret;

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "LoadManuMaintPub");
    if (ret)
	return ret;

    if (digest)
	memcpy(digest, &tpmdata.buffer[TPM_DATA_OFFSET], TPM_DIGEST_SIZE);

    return 0;
}

uint32_t TPM_ReadManuMaintPub(unsigned char *nonce, unsigned char *digest)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_ReadManuMaintPub);

    /* build the request buffer */
    ret = TSS_buildbuff("00 c1 T l %", &tpmdata,
			ordinal_no, TPM_NONCE_SIZE, nonce);
    if (ret & ERR_MASK)
	return ret;

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "ReadManuMaintPub");
    if (ret)
	return ret;

    if (digest)
	memcpy(digest, &tpmdata.buffer[TPM_DATA_OFFSET], TPM_DIGEST_SIZE);

    return 0;
}
