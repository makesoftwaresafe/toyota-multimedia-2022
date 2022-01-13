/********************************************************************************/
/*			     	TPM Delegation Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: delegation.c 4641 2011-10-11 01:46:05Z stefanb $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <tpm_error.h>

#include <hmac.h>

#include "debug.h"

uint32_t TPM_Delegate_Manage(uint32_t familyID,
			     uint32_t opFlag,
			     unsigned char *opData, uint32_t opDataLen,
			     unsigned char *ownerAuth,
			     unsigned char *retData, uint32_t * retDataLen)
{
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_Manage);
    unsigned char c = 0;
    uint32_t len;
    uint32_t familyId_no = htonl(familyID);
    uint32_t opFlag_no = htonl(opFlag);
    uint32_t opDataLen_no = htonl(opDataLen);

    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata1[TPM_NONCE_SIZE];

    if (opData == NULL && TPM_FAMILY_INVALIDATE != opFlag)
	return ERR_NULL_ARG;

    if (ownerAuth != NULL) {
	session sess;
	TSS_gennonce(nonceodd);

	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			      &sess, ownerAuth, TPM_ET_OWNER, 0);
	if (ret)
	    return ret;

	ret = TSS_authhmac(authdata1, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &familyId_no, TPM_U32_SIZE,
			 &opFlag_no, TPM_U32_SIZE, &opDataLen_no,
			 opDataLen, opData, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	ret = TSS_buildbuff("00 c2 T l l l @ L % o %", &tpmdata,
			    ordinal_no,
			    familyId_no,
			    opFlag_no,
			    opDataLen, opData,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata1);

	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	ret = TPM_Transmit(&tpmdata, "Delegate_Manage - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;
//#warning need to check HMAC here!

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
	if (ret & ERR_MASK)
	    return ret;
	if (retData != NULL) {
	    *retDataLen = MIN(*retDataLen, len);
	    memcpy(retData,
		   &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		   *retDataLen);
	}
    } else {
	ret = TSS_buildbuff("00 c1 T l l l @", &tpmdata,
			    ordinal_no,
			    familyId_no, opFlag_no, opDataLen, opData);

	if (ret & ERR_MASK)
	    return ret;

	ret = TPM_Transmit(&tpmdata, "Delegate_Manage - AUTH0");
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
	if (ret & ERR_MASK)
	    return ret;
	if (retData != NULL) {
	    *retDataLen = MIN(*retDataLen, len);
	    memcpy(retData,
		   &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		   *retDataLen);
	}
    }

    return ret;
}

uint32_t TPM_Delegate_CreateKeyDelegation(uint32_t keyhandle,
					  TPM_DELEGATE_PUBLIC * tdp,
					  unsigned char *blobAuth,
					  unsigned char *usageAuth,
					  unsigned char *blob,
					  uint32_t * blobSize)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_CreateKeyDelegation);
    unsigned char c = 0;
    uint32_t len;
    uint32_t keyhandle_no = htonl(keyhandle);
    STACK_TPM_BUFFER(serTdp);

    if (tdp == NULL || blobAuth == NULL || usageAuth == NULL)
	return ERR_NULL_ARG;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    ret = TPM_WriteDelegatePublic(&serTdp, tdp);
    if (ret & ERR_MASK)
	return ret;

    if (usageAuth != NULL) {
	session sess;
	unsigned char authdata[TPM_NONCE_SIZE];
	unsigned char nonceodd[TPM_NONCE_SIZE];
	unsigned char encauth[TPM_HASH_SIZE];
	uint16_t keytype;

	if (keyhandle == 0x40000000)
	    keytype = TPM_ET_SRK;
	else
	    keytype = TPM_ET_KEYHANDLE;

	/* Open OSAP Session */
	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_DSAP, &sess, usageAuth,
			    keytype, keyhandle);
	if (ret)
	    return ret;

	/* calculate encrypted authorization value */
	TPM_CreateEncAuth(&sess, blobAuth, encauth, 0);

	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	/* move Network byte order data to variable for HMAC calculation */
	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 serTdp.used, serTdp.buffer, TPM_HASH_SIZE,
			 encauth, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}
	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l % % L % o %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    serTdp.used, serTdp.buffer,
			    TPM_HASH_SIZE, encauth,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}
	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "Delegate_CreateKeyDelegation - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
	if (ret & ERR_MASK)
	    return ret;

	/* check the HMAC in the response */
	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   TPM_U32_SIZE + len, TPM_DATA_OFFSET, 0, 0);

	if (ret)
	    return ret;

	if (blob != NULL) {
	    *blobSize = MIN(*blobSize, len);
	    memcpy(blob,
		   &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		   *blobSize);
	}

	/*
	 * Keep track of the active delegation blobs.
	 */
	TPM_AddDelegationBlob(TPM_ET_DEL_KEY_BLOB,
			      keyhandle,
			      usageAuth,
			      encauth,
			      &tpmdata.buffer[TPM_DATA_OFFSET +
					      TPM_U32_SIZE], len);
    }
    return ret;
}

uint32_t TPM_Delegate_CreateOwnerDelegation(TPM_BOOL increment,
					    TPM_DELEGATE_PUBLIC * tdp,
					    unsigned char *blobAuth,
					    unsigned char *ownerAuth,
					    unsigned char *blob,
					    uint32_t * blobSize)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_CreateOwnerDelegation);
    unsigned char c = 0;
    uint32_t len;
    STACK_TPM_BUFFER(serTdp);

    if (tdp == NULL || blobAuth == NULL || ownerAuth == NULL)
	return ERR_NULL_ARG;

    ret = TPM_WriteDelegatePublic(&serTdp, tdp);
    if (ret & ERR_MASK) {
	return ret;
    }

    if (ownerAuth != NULL) {
	session sess;
	unsigned char authdata[TPM_NONCE_SIZE];
	unsigned char nonceodd[TPM_NONCE_SIZE];
	unsigned char encauth[TPM_HASH_SIZE];

	/* Open OSAP Session */
	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP, &sess,
			      ownerAuth, TPM_ET_OWNER, 0);
	if (ret)
	    return ret;

	/* calculate encrypted authorization value */
	TPM_CreateEncAuth(&sess, blobAuth, encauth, 0);

	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	/* move Network byte order data to variable for HMAC calculation */
	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 sizeof(TPM_BOOL), &increment, serTdp.used,
			 serTdp.buffer, TPM_HASH_SIZE, encauth, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}
	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l o % % L % o %", &tpmdata,
			    ordinal_no,
			    increment,
			    serTdp.used, serTdp.buffer,
			    TPM_HASH_SIZE, encauth,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata,
			 "Delegate_CreateOwnerDelegation - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
	if (ret & ERR_MASK)
	    return ret;

	/* check the HMAC in the response */
	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   TPM_U32_SIZE + len, TPM_DATA_OFFSET, 0, 0);

	if (ret)
	    return ret;

	if (blob != NULL) {
	    *blobSize = MIN(*blobSize, len);
	    memcpy(blob,
		   &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		   *blobSize);
	}

	/*
	 * Keep track of the active delegation blobs.
	 */
	TPM_AddDelegationBlob(TPM_ET_DEL_OWNER_BLOB,
			      0,
			      ownerAuth,
			      blobAuth,
			      &tpmdata.buffer[TPM_DATA_OFFSET +
					      TPM_U32_SIZE], len);
    }
    return ret;
}

uint32_t TPM_Delegate_LoadOwnerDelegation(uint32_t index,
					  unsigned char *ownerAuth,
					  unsigned char *blob,
					  uint32_t blobSize)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_LoadOwnerDelegation);
    unsigned char c = 0;
    uint32_t index_no = htonl(index);
    uint32_t blobSize_no = htonl(blobSize);

    if (blob == NULL)
	return ERR_NULL_ARG;

    if (ownerAuth != NULL) {
	session sess;
	unsigned char authdata[TPM_NONCE_SIZE];
	unsigned char nonceodd[TPM_NONCE_SIZE];
	unsigned char encauth[TPM_HASH_SIZE];

	/* Open OSAP Session */
	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP, &sess,
			      ownerAuth, TPM_ET_OWNER, 0);
	if (ret)
	    return ret;

	/* calculate encrypted authorization value */
	TPM_CreateEncAuth(&sess, ownerAuth, encauth, 0);

	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	/* move Network byte order data to variable for HMAC calculation */
	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &index_no, TPM_U32_SIZE,
			 &blobSize_no, blobSize, blob, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}
	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l @ L % o %", &tpmdata,
			    ordinal_no,
			    index_no,
			    blobSize, blob,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "Delegate_LoadOwnerDelegation - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0,
			   0);

    } else {
	/* build the request buffer */
	ret = TSS_buildbuff("00 c1 T l l @", &tpmdata,
			    ordinal_no, index_no, blobSize, blob);
	if (ret & ERR_MASK)
	    return ret;

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "Delegate_LoadOwnerDelegation");
    }
    return ret;
}

uint32_t TPM_Delegate_ReadTable(unsigned char *familyTable,
				uint32_t * familyTableSize,
				unsigned char *delegateTable,
				uint32_t * delegateTableSize)
{
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_ReadTable);
    STACK_TPM_BUFFER(tpmdata)
    uint32_t len1, len2;

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "Delegate_ReadTable");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len1);
    if (ret & ERR_MASK)
	return ret;
    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET + TPM_U32_SIZE + len1,
			  &len2);
    if (ret & ERR_MASK)
	return ret;

    if (familyTable != NULL) {
	*familyTableSize = MIN(*familyTableSize, len1);
	memcpy(familyTable,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *familyTableSize);
    }

    if (delegateTable != NULL) {
	*delegateTableSize = MIN(*delegateTableSize, len2);
	memcpy(delegateTable,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE + len1 +
			       TPM_U32_SIZE], *delegateTableSize);
    }

    return ret;
}

uint32_t TPM_Delegate_UpdateVerification(unsigned char *inputData,
					 uint32_t inputDataSize,
					 unsigned char *ownerAuth,
					 unsigned char *outputData,
					 uint32_t * outputDataSize)
{

    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_UpdateVerification);
    unsigned char c = 0;
    uint32_t len;
    uint32_t inputDataSize_no = htonl(inputDataSize);

    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata1[TPM_NONCE_SIZE];
    session sess;

    if (inputData == NULL)
	return ERR_NULL_ARG;

    TSS_gennonce(nonceodd);

    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    ret = TSS_authhmac(authdata1, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE,
		     &inputDataSize_no, inputDataSize, inputData, 0, 0);

    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TSS_buildbuff("00 c2 T l @ L % o %", &tpmdata,
			ordinal_no,
			inputDataSize, inputData,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata1);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "Delegate_UpdateVerification");
    TSS_SessionClose(&sess);

    if (ret)
	return ret;
//#warning Must check HMAC here!

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    if (outputData != NULL) {
	*outputDataSize = MIN(*outputDataSize, len);
	memcpy(outputData,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *outputDataSize);
    }
    return ret;
}


uint32_t TPM_Delegate_VerifyDelegation(unsigned char *delegation,
				       uint32_t delegationLen)
{
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_Delegate_VerifyDelegation);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l @", &tpmdata,
			    ordinal_no, delegationLen, delegation);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "Delegate_VerifyDelegation");

    return ret;
}

struct delegation_blob {
    uint32_t etype;
    uint32_t keyhandle;
    unsigned char keyDigest[TPM_DIGEST_SIZE];
    uint32_t blobSize;
    unsigned char passHash[TPM_HASH_SIZE];
    unsigned char oldPassHash[TPM_HASH_SIZE];
};

static
char *getDelegationFile(void)
{
    char *filename;
    char *inst = getenv("TPM_INSTANCE");
    if (inst == NULL)
	inst = "0";
    filename = malloc(strlen(inst) + strlen("/tmp/.delegation-") + 2);
    sprintf(filename, "/tmp/.delegation-%s", inst);
    return filename;
}

static
TPM_BOOL TPM_FindDelegationBlob(uint32_t etype,
				uint32_t keyhandle, int *fdnop)
{
    unsigned int entry = 0;
    TPM_BOOL rc = FALSE;
    char *filename = getDelegationFile();
    *fdnop = open(filename, O_RDWR | O_CREAT, S_IRWXU);
    free(filename);
    if (*fdnop > 0) {
	while (1) {
	    struct delegation_blob db;
	    int n;
	    if (sizeof(db) == read(*fdnop, &db, sizeof(db))) {
		if (db.etype == etype && db.keyhandle == keyhandle) {
		    /* found it */
		    n = lseek(*fdnop, -sizeof(db), SEEK_CUR);
#ifdef __CYGWIN__
		    lseek(*fdnop, n, SEEK_SET);
		    rc = TRUE;
#else
		    if (n >= 0)
			rc = TRUE;
#endif
		    break;
		}
		/* Not the right one. */
		/* Jump over the blob */
		n = lseek(*fdnop, db.blobSize, SEEK_CUR);
#ifdef __CYGWIN__
		lseek(*fdnop, n, SEEK_SET);
#endif
		entry++;
	    } else {
		/* Read failed. Assuming I am at the end of the
		   file
		 */
		rc = FALSE;
		break;
	    }
	}
    }
#if 0
    if (TRUE == rc)
	printf("found entry : entry = %d\n", entry);
    else
	printf("NO entry found!\n");
#endif
    return rc;
}

static
uint32_t TPM_GetDelegationBlobFromFile(uint32_t etype,
				       unsigned char *buffer,
				       uint32_t * bufferSize)
{
    uint32_t ret = 0;
    char *name = NULL;
    int fdno = 0;
    struct stat sb;

    switch (etype) {
    case TPM_ET_DEL_KEY_BLOB:
	name = getenv("TPM_DSAP_KEYBLOB");
	break;
    case TPM_ET_DEL_OWNER_BLOB:
	name = getenv("TPM_DSAP_OWNERBLOB");
	break;
    }

    if (name == NULL) {
	ret = ERR_ENV_VARIABLE;
	goto exit;
    }

    fdno = open(name, O_RDONLY);
    if (fdno <= 0) {
	ret = ERR_BAD_FILE;
	goto exit;
    }
    if (fstat(fdno, &sb) < 0) {
	ret = ERR_BAD_FILE;
	goto exit;
    }
    if ((off_t) *bufferSize > sb.st_size) {
	ret = ERR_BUFFER;
	goto exit;
    }

    *bufferSize = read(fdno, buffer, sb.st_size);

exit:
    if (fdno > 0)
	close(fdno);
    return ret;
}

uint32_t TPM_GetDelegationBlob(uint32_t etype,
			       uint32_t keyhandle,
			       unsigned char *newPassHash,
			       unsigned char *buffer,
			       uint32_t * bufferSize)
{
    uint32_t ret = 0;
    int fdno = 0;
    TPM_BOOL found;
    struct delegation_blob db;
    (void) newPassHash;

    if (TPM_GetDelegationBlobFromFile(etype, buffer, bufferSize) == 0)
	goto exit;

    found = TPM_FindDelegationBlob(etype, keyhandle, &fdno);
    if (found != TRUE) {
	ret = ERR_NOT_FOUND;
	goto exit;
    }
    if (fdno <= 0) {
	ret = ERR_BAD_FILE;
	goto exit;
    }

    if (read(fdno, &db, sizeof(db)) != sizeof(db)) {
	ret = ERR_BAD_FILE;
	goto exit;
    }
    if (*bufferSize < db.blobSize) {
	ret = ERR_BUFFER;
	goto exit;
    }
    if (read(fdno, buffer, db.blobSize) != (int)db.blobSize) {
	ret = ERR_BAD_FILE;
	goto exit;
    }
    *bufferSize = db.blobSize;

exit:
    if (fdno > 0)
	close(fdno);
    return ret;
}

uint32_t TPM_AddDelegationBlob(uint32_t etype,
			       uint32_t keyhandle,
			       unsigned char *oldPassHash,
			       unsigned char *newPassHash,
			       unsigned char *buffer, uint32_t bufferSize)
{
    TPM_BOOL found;
    uint32_t ret = 0;
    struct delegation_blob db;
    int fdno = 0;
    ssize_t wrc;

    memset(&db, 0x0, sizeof(db));

    db.etype = etype;

    if (newPassHash)
	memcpy(db.passHash, newPassHash, TPM_HASH_SIZE);
    if (oldPassHash)
	memcpy(db.oldPassHash, oldPassHash, TPM_HASH_SIZE);

    db.keyhandle = keyhandle;
    db.blobSize = bufferSize;

    found = TPM_FindDelegationBlob(etype, keyhandle, &fdno);
    /* whether an existing one was found is not so important 
       but that we have a spot where to write the new one to is */
    (void) found;
    
    if (fdno <= 0) {
	ret = ERR_BAD_FILE;
	goto exit;
    }

    wrc = write(fdno, &db, sizeof(db));
    if (wrc < 0 || (size_t) wrc != sizeof(db)) {
	ret = ERR_BAD_FILE_WRITE;
	goto exit;
    }
    wrc = write(fdno, buffer, bufferSize);
    if (wrc < 0 || (size_t) wrc != bufferSize) {
	ret = ERR_BAD_FILE_WRITE;
	goto exit;
    }
    if (close(fdno) != 0) {
	fdno = 0;
	ret = ERR_BAD_FILE_CLOSE;
	goto exit;
    }
    fdno = 0;

exit:
    if (fdno > 0)
	close(fdno);
    return ret;
}

uint32_t TPM_ResetDelegation(void)
{
    uint32_t ret = 0;
    char *filename = getDelegationFile();
    FILE *fp = fopen(filename, "w");
    if (fp) {
	if (fclose(fp) != 0)
	    ret = ERR_BAD_FILE_CLOSE;
    }
    free(filename);
    return ret;
}
