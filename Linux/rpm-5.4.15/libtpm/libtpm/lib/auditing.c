/********************************************************************************/
/*			     	TPM Auditing Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: auditing.c 4637 2011-10-11 01:02:07Z stefanb $		*/
/********************************************************************************/
#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <tpm_error.h>

#include <hmac.h>

#include "debug.h"

struct auditing {
    uint8_t ordinalbitmap[256 / 8];
    TPM_DIGEST inDigest;
    TPM_DIGEST auditDigest;
    TPM_DIGEST calcAuditDigest;
    TPM_COUNTER_VALUE auditCounter;
};

static const uint32_t def_audit_ord[] = {
    TPM_ORD_ActivateIdentity,
    TPM_ORD_AuthorizeMigrationKey,
    TPM_ORD_ChangeAuthOwner,
    TPM_ORD_ConvertMigrationBlob,
    TPM_ORD_CreateMaintenanceArchive,
    TPM_ORD_CreateMigrationBlob,
    TPM_ORD_CreateWrapKey,
    TPM_ORD_DisableForceClear,
    TPM_ORD_DisableOwnerClear,
    TPM_ORD_DisablePubekRead,
    TPM_ORD_ForceClear,
    TPM_ORD_KillMaintenanceFeature,
    TPM_ORD_LoadMaintenanceArchive,
    TPM_ORD_LoadManuMaintPub,
    TPM_ORD_MakeIdentity,
    TPM_ORD_OwnerClear,
    TPM_ORD_OwnerReadPubek,
    TPM_ORD_OwnerSetDisable,
    TPM_ORD_PhysicalDisable,
    TPM_ORD_PhysicalEnable,
    TPM_ORD_PhysicalSetDeactivated,
    TPM_ORD_ReadManuMaintPub,
    TPM_ORD_ReadPubek,
    TPM_ORD_SetOrdinalAuditStatus,
    TPM_ORD_SetOwnerInstall,
    TPM_ORD_SetRedirection,
    TPM_ORD_SetTempDeactivated,
    TPM_ORD_TakeOwnership,
    0
};

static const uint32_t never_audit_ord[] = {
    TPM_ORD_GetAuditDigest,
    TPM_ORD_GetAuditDigestSigned,
    TPM_ORD_GetAuditEvent,
    TPM_ORD_GetAuditEventSigned,
    TPM_ORD_GetOrdinalAuditStatus,
    0
};

static char *getAuditingFile(void)
{
    char *filename;
    char *inst = getenv("TPM_INSTANCE");
    if (inst == NULL)
	inst = "0";

    filename = malloc(strlen(inst) + strlen(".auditing-") + 2);
    sprintf(filename, ".auditing-%s", inst);
    return filename;
}

static uint32_t _TPM_WriteAuditingData(struct auditing *au)
{
    uint32_t ret = 0;
    char *filename = getAuditingFile();
    FILE *fp = fopen(filename, "w");

//printf("fp=%p\n",fp);
    if (fp == NULL) {
	ret = ERR_BAD_FILE;
	goto exit;
    }

    if (fwrite(au, sizeof(*au), 1, fp) != 1)
	ret = ERR_BAD_FILE_WRITE;

exit:
    if (fp && fclose(fp) != 0 && ret == 0)
	ret = ERR_BAD_FILE_CLOSE;
    if (filename)
	free(filename);
    return ret;
}

static uint32_t _TPM_InitAuditing(void)
{
    struct auditing au;
    uint32_t ctr = 0;

//printf("*** Initializing auditing file.\n");
    memset(&au, 0x0, sizeof(au));
    while (def_audit_ord[ctr]) {
	uint32_t ord = def_audit_ord[ctr];
	uint8_t idx = ord >> 3;
	uint8_t mask = 1 << (ord & 0x7);
	au.ordinalbitmap[idx] |= mask;
	ctr++;
    }
    return _TPM_WriteAuditingData(&au);
}

static uint32_t _TPM_ReadAuditingData(struct auditing *au)
{
    char *filename = getAuditingFile();
    FILE *fp = fopen(filename, "r");
    uint32_t ret = 0;

    if (fp == NULL) {
	ret = _TPM_InitAuditing();
	goto exit;
    }

    memset(au, 0x0, sizeof(*au));
    if (fread(au, sizeof(*au), 1, fp) != 1)
	ret = ERR_BAD_FILE_READ;

exit:
    if (fp && fclose(fp) != 0 && ret == 0)
	ret = ERR_BAD_FILE_CLOSE;
    if (filename)
	free(filename);
    return ret;
}

static uint32_t _TPM_SetAuditingCounterValue(TPM_COUNTER_VALUE * cv)
{
    struct auditing au;
    uint32_t ret = 0;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;
    au.auditCounter = *cv;
    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    return ret;
}

static uint32_t _TPM_SetAuditDigest(TPM_DIGEST * digest)
{
    uint32_t ret = 0;
    struct auditing au;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	ret = _TPM_InitAuditing();
    if (ret)
	goto exit;

    memcpy(au.calcAuditDigest, au.auditDigest, TPM_DIGEST_SIZE);
//printf("COPIED!\n");
#if 0
    print_array("my digest: ", (unsigned char *) au.auditDigest,
		    TPM_DIGEST_SIZE);
#endif
    memcpy(au.auditDigest, digest, TPM_DIGEST_SIZE);
    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    return ret;
}

uint32_t TPM_GetAuditDigest(uint32_t startOrdinal,
			    TPM_COUNTER_VALUE * countervalue,
			    unsigned char *digest,
			    TPM_BOOL * more,
			    uint32_t ** ord, uint32_t * ordSize)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_GetAuditDigest);
    uint32_t startOrdinal_no = htonl(startOrdinal);
    ALLOC_TPM_BUFFER(tpmdata, 0)
    uint32_t size;
    TPM_COUNTER_VALUE cv;

    if (tpmdata == NULL) {
	ret = ERR_MEM_ERR;
	goto exit;
    }

    ret = TSS_buildbuff("00 c1 T l l", tpmdata,
			ordinal_no, startOrdinal_no);
    if (ret & ERR_MASK)
	goto exit;

    ret = TPM_Transmit(tpmdata, "GetAuditDigest");
    if (ret)
	goto exit;

    if (countervalue) {
	ret = TPM_ReadCounterValue(&tpmdata->buffer[TPM_DATA_OFFSET],
				 countervalue);
	if (ret & ERR_MASK)
	    goto exit;
    }

    ret = TPM_ReadCounterValue(&tpmdata->buffer[TPM_DATA_OFFSET], &cv);
    if (ret & ERR_MASK)
	goto exit;

    ret = _TPM_SetAuditingCounterValue(&cv);
    if (ret)
	goto exit;

    if (digest)
	memcpy(digest, &tpmdata->buffer[TPM_DATA_OFFSET + 10], TPM_DIGEST_SIZE);

    ret = _TPM_SetAuditDigest((TPM_DIGEST *)
			&tpmdata->buffer[TPM_DATA_OFFSET + 10]);
//printf("Refreshed audit digest. ret=%d\n",ret);
    if (ret)
	goto exit;

    if (more)
	*more = *(TPM_BOOL *)
		(&tpmdata->buffer[TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE]);

    ret = tpm_buffer_load32(tpmdata,
		TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE + sizeof(TPM_BOOL), &size);
    if (ret & ERR_MASK)
	goto exit;

    // !!! need to check upper limit of size
    if (size > 0 && NULL != ord) {
	*ord = malloc(size);
	*ordSize = size;
	memcpy(*ord,
	       &tpmdata->buffer[TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE +
				sizeof(TPM_BOOL) + TPM_U32_SIZE], size);
    }

exit:
    if (tpmdata)
	FREE_TPM_BUFFER(tpmdata);
    return ret;
}


uint32_t TPM_GetAuditDigestSigned(uint32_t keyhandle, TPM_BOOL closeAudit, unsigned char *usageAuth,	// HMAC key
				  unsigned char *antiReplay,
				  TPM_COUNTER_VALUE * countervalue,
				  unsigned char *auditDigest,
				  unsigned char *ordinalDigest,
				  struct tpm_buffer * signature)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_GetAuditDigestSigned);
    uint32_t ret;
    uint32_t keyhandle_no = htonl(keyhandle);
    uint32_t _sigSize;
    TPM_COUNTER_VALUE cv;
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    TPM_BOOL c = 0;
    session sess;

    /* check input arguments */

    ret = TSS_gennonce(antiReplay);

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	goto exit;

    if (usageAuth) {

	/* Open OSAP Session */
	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			      &sess,
			      usageAuth, TPM_ET_KEYHANDLE, keyhandle);

	if (ret)
	    goto exit;

	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0) {
	    ret = ERR_CRYPT_ERR;
	    goto exit;
	}

	/* move Network byte order data to variable for HMAC calculation */
	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 sizeof(TPM_BOOL), &closeAudit, TPM_HASH_SIZE,
			 antiReplay, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    goto exit;
	}
	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l o % L % o %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    closeAudit,
			    TPM_HASH_SIZE, antiReplay,
			    TSS_Session_GetHandle(&sess),
			    TPM_HASH_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);

	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    goto exit;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "GetAuditDigestSigned - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    goto exit;

	/* check the HMAC in the response */
	ret = tpm_buffer_load32(&tpmdata,
			      TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE +
			      TPM_HASH_SIZE, &_sigSize);
	if (ret & ERR_MASK)
	    goto exit;

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   10 + TPM_HASH_SIZE + TPM_HASH_SIZE + 4 +
			   _sigSize, TPM_DATA_OFFSET, 0, 0);
	if (ret)
	    goto exit;
    } else {
	ret = TSS_buildbuff("00 c1 T l l o %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    closeAudit, TPM_HASH_SIZE, antiReplay);
	if (ret & ERR_MASK)
	    goto exit;

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "GetAuditDigestSigned");
	if (ret)
	    goto exit;

	ret = tpm_buffer_load32(&tpmdata,
			      TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE +
			      TPM_HASH_SIZE, &_sigSize);
	if (ret & ERR_MASK)
	    goto exit;
    }

    if (countervalue) {
	ret = TPM_ReadCounterValue(&tpmdata.buffer[TPM_DATA_OFFSET],
				 countervalue);
	if (ret & ERR_MASK)
	    goto exit;
	ret = 0;
    }

    if (closeAudit == TRUE) {
	_TPM_InitAuditing();
	ret = TPM_ReadCounterValue(&tpmdata.buffer[TPM_DATA_OFFSET], &cv);
	if (ret & ERR_MASK)
	    goto exit;
	ret = _TPM_SetAuditingCounterValue(&cv);
	if (ret != TPM_SUCCESS)
	    goto exit;
    }

    if (auditDigest != NULL)
	memcpy(auditDigest,
	       &tpmdata.buffer[TPM_DATA_OFFSET + 10], TPM_HASH_SIZE);

    if (closeAudit == FALSE) {
	ret = _TPM_SetAuditDigest((TPM_DIGEST *) & tpmdata.
				buffer[TPM_DATA_OFFSET + 10]);
	if (ret)
	    goto exit;
    }

    if (ordinalDigest)
	memcpy(ordinalDigest,
	       &tpmdata.buffer[TPM_DATA_OFFSET + 10 + TPM_HASH_SIZE],
	       TPM_HASH_SIZE);

    if (signature)
	SET_TPM_BUFFER(signature,
		       &tpmdata.buffer[TPM_DATA_OFFSET + 10 +
				       TPM_HASH_SIZE + TPM_HASH_SIZE +
				       TPM_U32_SIZE], _sigSize);

exit:
    return ret;
}

uint32_t TPM_SetOrdinalAuditStatus(uint32_t ordinalToAudit, TPM_BOOL auditState, unsigned char *ownerAuth	// HMAC key
    )
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_SetOrdinalAuditStatus);
    uint32_t ret;
    uint32_t ordinalToAudit_no = htonl(ordinalToAudit);

    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    TPM_BOOL c = 0;
    session sess;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0) {
	ret = ERR_CRYPT_ERR;
	goto exit;
    }

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerAuth, TPM_ET_OWNER, 0);

    if (ret)
	goto exit;

    /* move Network byte order data to variable for HMAC calculation */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE,
		     &ordinalToAudit_no, sizeof(TPM_BOOL), &auditState, 0,
		     0);
    if (ret) {
	TSS_SessionClose(&sess);
	goto exit;
    }
    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l o L % o %", &tpmdata,
			ordinal_no,
			ordinalToAudit_no,
			auditState,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	goto exit;
    }

    _TPM_SetAuditStatus(ordinalToAudit, auditState);

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "SetOrdinalAuditStatus - AUTH1");
    TSS_SessionClose(&sess);
    if (ret)
	goto exit;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);
    if (ret)
	goto exit;

exit:
    return ret;
}

static
uint32_t _TPM_SHA1_NoAuthParms(const struct tpm_buffer *buf,
			       uint32_t handles,
			       uint32_t size,
			       TPM_DIGEST * digest,
			       int is_enc, uint32_t dsap_gap)
{
    uint32_t ret = 0;
    uint32_t offset = handles * 4 + dsap_gap;
    unsigned char *buffer = NULL;
    unsigned char mybuf[4];
    uint32_t sz;

    if (is_enc) {
	memcpy(&mybuf[0], &buf->buffer[6], sizeof(mybuf));
//print_array("buffer to hash:",buffer,4);
	TSS_sha1(mybuf, sizeof(mybuf), (unsigned char *) digest);
//print_array("inparam digest (ENC):", (unsigned char *)digest, 20);
	goto exit;
    }

    if (size == 0)
	goto exit;

    buffer = malloc(size);
    if (buffer == NULL) {
	ret = ERR_MEM_ERR;
	goto exit;
    }

    sz = (size < 4) ? size : 4;
    memcpy(&buffer[0], &buf->buffer[6], sz);
    if (size > 4)
	memcpy(&buffer[sz], &buf->buffer[10 + offset], size - 4);
//
//print_array("buffer to hash:",buffer,size);
    TSS_sha1(buffer, size, (unsigned char *) digest);
//print_array("inparam digest:", (unsigned char *)digest, 20);

exit:
    if (buffer)
	free(buffer);
    return ret;
}

static
uint32_t _TPM_SHA1_NoAuthParmsResp(const struct tpm_buffer *buf,
				   uint32_t handles,
				   uint32_t size,
				   TPM_DIGEST * digest,
				   uint32_t ordinal, int is_enc)
{
    uint32_t ret = 0;
    uint32_t offset = handles * 4;
    unsigned char *buffer = NULL;
    uint32_t ord_no;
    uint32_t sz;

//printf("size=%d\n",size);
    if (is_enc) {
	if (size >= 4)
	    size = 4;
    }
    if (size == 0)
	goto exit;

    buffer = malloc(size + sizeof(ordinal));
    if (buffer == NULL) {
	ret = ERR_MEM_ERR;
	goto exit;
    }

    ord_no = htonl(ordinal);
    sz = (size < 4) ? size : 4;
    memcpy(&buffer[0], &buf->buffer[6], sz);
    memcpy(&buffer[sz], &ord_no, sizeof(ord_no));
//printf("handles=%d,size=%d\n",handles,size);
    if (size > 4) {
	size -= (4 * handles);
	memcpy(&buffer[sz + 4], &buf->buffer[10 + offset], size - 4);
    }
    TSS_sha1(buffer, size + sizeof(ordinal), (unsigned char *) digest);
//print_array("buffer: ",buffer,size+4);
//print_array("outparam digest:", (unsigned char *)digest, 20);

exit:
    if (buffer)
	free(buffer);
    return ret;
}

uint32_t _TPM_AuditInputstream(const struct tpm_buffer * req, int is_enc)
{
    uint32_t ret = TPM_SUCCESS;
    uint16_t type;
    int32_t size;
    uint32_t handles = 0;
    uint32_t ord;
    TPM_AUDIT_EVENT_IN aei;
    struct auditing au;
    uint32_t dsap_gap = 0;
    uint32_t rc = 0;
    uint32_t aei_len = 0;
    STACK_TPM_BUFFER(audit_event_ser);
    char * buffer = NULL;

    ret = tpm_buffer_load16(req, 0, &type);
    if (ret)
	goto exit;
    ret = tpm_buffer_load32(req, 2, (uint32_t *) & size);
    if (ret)
	goto exit;
    ret = tpm_buffer_load32(req, 6, &ord);
    if (ret)
	goto exit;

    _TPM_IsAuditedOrdinal(ord, &rc);
    if (rc == 0) {
	ret = TPM_SUCCESS;
	goto exit;
    }

    handles = getNumHandles(ord);

    if (type == TPM_TAG_RQU_COMMAND) {
    } else if (type == TPM_TAG_RQU_AUTH1_COMMAND) {
	size -= 45;
    } else if (type == TPM_TAG_RQU_AUTH2_COMMAND) {
	size -= 90;
    } else {
	//ret = ERR_BADREQTAG
    }

    if (ord == TPM_ORD_OSAP) {
	size -= (6 + 2 + 4 + 20);
    } else if (ord == TPM_ORD_DSAP) {
	dsap_gap = 2 + 4 + 20 + 4;
	size -= (6 + dsap_gap);
    } else {
	size -= (6 + 4 * handles);
    }
    if (size < 0) {
	ret = ERR_BUFFER;
	goto exit;
    }

    aei.tag = TPM_TAG_AUDIT_EVENT_IN;
    ret = _TPM_SHA1_NoAuthParms(req, handles, size,
				    &aei.inputParms, is_enc, dsap_gap);
    if (ret)
	goto exit;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;

    memcpy(&aei.auditCount, &au.auditCounter, sizeof(aei.auditCount));
    ret = TPM_WriteAuditEventIn(&audit_event_ser, &aei);
    if (ret & ERR_MASK)
	goto exit;
    aei_len = ret;

    buffer = malloc(aei_len + TPM_DIGEST_SIZE);
    if (buffer == NULL) {
	ret = ERR_MEM_ERR;
	goto exit;
    }

    memcpy(&buffer[0], au.auditDigest, TPM_DIGEST_SIZE);
    memcpy(&buffer[TPM_DIGEST_SIZE], audit_event_ser.buffer, aei_len);

#if 0
    if (ord == 11)
	printf("hashing a total of %d bytes.\n", aei_len + 20);
    print_array("IN DATA: ", buffer, aei_len + TPM_DIGEST_SIZE);
#endif
    TSS_sha1(buffer, aei_len+TPM_DIGEST_SIZE, (unsigned char *) au.auditDigest);
#if 0
    {
	int j = 0;
	printf("IN auditDigest: ");
	while (j < 4) {
	    printf("%02X ", au.auditDigest[j]);
	    j++;
	}
	printf("\n");
    }
#endif

    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    if (buffer)
	free(buffer);
    return ret;
};

uint32_t _TPM_AuditOutputstream(const struct tpm_buffer * res,
				uint32_t ord, int is_enc)
{
    uint32_t ret = TPM_SUCCESS;
    uint16_t type;
    int32_t size;
    TPM_AUDIT_EVENT_OUT aeo;
    struct auditing au;
    uint32_t handles = 0;
    uint32_t result = 0;
    uint32_t rc = 0;
    uint32_t aeo_len = 0;
    STACK_TPM_BUFFER(audit_event_ser);
    char *buffer = NULL;

    ret = tpm_buffer_load16(res, 0, &type);
    if (ret)
	goto exit;
    ret = tpm_buffer_load32(res, 2, (uint32_t *) &size);
    if (ret)
	goto exit;
    ret = tpm_buffer_load32(res, 6, (uint32_t *) &result);
    if (ret)
	goto exit;

    _TPM_IsAuditedOrdinal(ord, &rc);
    if (rc == 0) {
	ret = TPM_SUCCESS;
	goto exit;
    }
#if 0
    printf("ord %x audited.\n", ord);
#endif

    handles = getNumRespHandles(ord);

    if (result == 0) {
	if (type == TPM_TAG_RSP_COMMAND) {
	} else if (type == TPM_TAG_RSP_AUTH1_COMMAND) {
	    size -= 41;
	} else if (type == TPM_TAG_RSP_AUTH2_COMMAND) {
	    size -= 82;
	}

	if (ord == TPM_ORD_OSAP || ord == TPM_ORD_DSAP) {
	    size -= (6 + 4 + 20 + 20);
	} else if (ord == TPM_ORD_OIAP) {
	    size -= (6 + 4 + 20);
	} else {
	    size -= 6;
	}
    } else {
	size -= 6;
    }

    if (size < 0) {
	ret = ERR_BUFFER;
	goto exit;
    }

    aeo.tag = TPM_TAG_AUDIT_EVENT_OUT;
    ret = _TPM_SHA1_NoAuthParmsResp(res, handles, size,
					&aeo.outputParms, ord, is_enc);
    if (ret)
	goto exit;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;

    memcpy(&aeo.auditCount, &au.auditCounter, sizeof(aeo.auditCount));
    ret = TPM_WriteAuditEventOut(&audit_event_ser, &aeo);

    if (ret & ERR_MASK)
	goto exit;
    aeo_len = ret;
    ret = 0;

    buffer = malloc(aeo_len + TPM_DIGEST_SIZE);
    if (buffer == NULL) {
	ret = ERR_MEM_ERR;
	goto exit;
    }

    memcpy(&buffer[00], au.auditDigest, TPM_DIGEST_SIZE);
    memcpy(&buffer[TPM_DIGEST_SIZE],
		       audit_event_ser.buffer, audit_event_ser.used);
    TSS_sha1(buffer, aeo_len+TPM_DIGEST_SIZE, (unsigned char *)&au.auditDigest);
#if 0
    {
	print_array("OUT auditDigest: ", au.auditDigest, 4);
    }
#endif

    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    if (buffer)
	free(buffer);
    return ret;
};

uint32_t TPM_SetAuditedOrdinal(uint32_t ord)
{
    struct auditing au;
    uint32_t ret = 0;
    uint32_t ordinal;
    uint32_t ctr = 0;
    uint8_t mask;
    uint8_t idx;

    for (ctr = 0; (ordinal = never_audit_ord[ctr]) != 0; ctr++)
	if (ordinal == ord)
	    goto exit;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;
    mask = 1 << (ord & 0x7);
    idx = (ord & 0xff) >> 3;
    au.ordinalbitmap[idx] |= mask;
    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    return ret;
}

uint32_t TPM_ClearAuditedOrdinal(uint32_t ord)
{
    struct auditing au;
    uint32_t ret = 0;
    uint8_t mask;
    uint8_t idx;

    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;

    mask = (1 << (ord & 0x7)) ^ 0xff;
    idx = (ord & 0xff) >> 3;
    au.ordinalbitmap[idx] &= mask;
    ret = _TPM_WriteAuditingData(&au);
    if (ret)
	goto exit;

exit:
    return ret;
}

uint32_t _TPM_SetAuditStatus(uint32_t ord, TPM_BOOL enable)
{
    return (enable
	? TPM_SetAuditedOrdinal(ord)
	: TPM_ClearAuditedOrdinal(ord)
	);
}


uint32_t _TPM_IsAuditedOrdinal(uint32_t ord, uint32_t * rc)
{
    uint32_t ret = 0;
    struct auditing au;
    uint8_t mask;
    uint8_t idx;

    *rc = 0;
    ret = _TPM_ReadAuditingData(&au);
    if (ret)
	goto exit;

    if (ord >= 256) {
	*rc = 0;
    } else {
	mask = 1 << (ord & 0x7);
	idx = (ord & 0xff) >> 3;
	if (mask & au.ordinalbitmap[idx])
	    *rc = 1;
    }
#if 0
    if (*rc)
	printf("Ordinal %d (%x) is audited\n", ord, ord);
    else
	printf("Ordinal %d (%x) is NOT audited\n", ord, ord);
#endif

exit:
    return ret;
}

uint32_t _TPM_GetCalculatedAuditDigest(TPM_DIGEST * digest)
{
    struct auditing au;
    uint32_t ret = _TPM_ReadAuditingData(&au);
    if (ret == 0)
	memcpy(digest, au.calcAuditDigest, sizeof(*digest));
    return ret;
}
