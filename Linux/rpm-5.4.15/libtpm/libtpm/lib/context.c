/********************************************************************************/
/*			     	TPM Context Management Routines			*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: context.c 4073 2010-04-30 14:44:14Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

uint32_t TPM_SaveKeyContext(uint32_t keyhandle, struct tpm_buffer *context)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SaveKeyContext);
    STACK_TPM_BUFFER(tpmdata)
    uint32_t keyhandle_no = htonl(keyhandle);
    uint32_t len;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    ret = TSS_buildbuff("00 c1 T l l", &tpmdata, ordinal_no, keyhandle_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SaveKeyContext");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    if (context != NULL)
	SET_TPM_BUFFER(context,
		       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
		       len);

    return ret;
}

uint32_t TPM_LoadKeyContext(struct tpm_buffer * context,
			    uint32_t * keyhandle)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_LoadKeyContext);
    STACK_TPM_BUFFER(tpmdata);

    ret = TSS_buildbuff("00 c1 T l @", &tpmdata,
			ordinal_no, context->used, context->buffer);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "LoadKeyContext");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, keyhandle);
    if (ret & ERR_MASK)
	return ret;

    return ret;
}

uint32_t TPM_SaveAuthContext(uint32_t authhandle,
			     unsigned char *authContextBlob,
			     uint32_t * authContextSize)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SaveAuthContext);
    STACK_TPM_BUFFER(tpmdata)
    uint32_t authhandle_no = htonl(authhandle);
    uint32_t len;

    ret = TSS_buildbuff("00 c1 T l l", &tpmdata,
			ordinal_no, authhandle_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SaveAuthContext");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, &len);
    if (ret & ERR_MASK)
	return ret;

    if (authContextBlob != NULL) {
	*authContextSize = MIN(*authContextSize, len);
	memcpy(authContextBlob,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       *authContextSize);
    }

    return ret;
}

uint32_t TPM_LoadAuthContext(unsigned char *authContextBlob,
			     uint32_t authContextSize,
			     uint32_t * authhandle)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_LoadAuthContext);
    STACK_TPM_BUFFER(tpmdata);

    ret = TSS_buildbuff("00 c1 T l @", &tpmdata,
			ordinal_no, authContextSize, authContextBlob);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "LoadAuthContext");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, authhandle);
    if (ret & ERR_MASK)
	return ret;

    return ret;
}
