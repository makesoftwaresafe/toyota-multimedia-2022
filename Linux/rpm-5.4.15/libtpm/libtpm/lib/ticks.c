/********************************************************************************/
/*			     	TPM Tick Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: ticks.c 4073 2010-04-30 14:44:14Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

/****************************************************************************/
uint32_t TPM_GetTicks(unsigned char *tickbuffer)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_GetTicks);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "GetTicks");
    if (ret)
	return ret;

    memcpy(tickbuffer, &tpmdata.buffer[TPM_DATA_OFFSET],
	   sizeof(TPM_CURRENT_TICKS) - 2 * TPM_U32_SIZE);

    return ret;
}

/****************************************************************************/
/*                                                                          */
/*Apply a timestamp to a passed blob                                        */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the handle of the key used to seal the data                 */
/*           0x40000000 for the SRK                                         */
/* usageauth The sha'ed usage password                                      */
/* tickbuff  A pointer to an area that will hold the current ticks of the   */
/*           TPM upon return; may be NULL                                   */
/* sigbuf    a pointer to an area containing the signature upon returns     */
/* sifbuflen an integer that indicates the size of the sigbuf on input and  */
/*           the size of the valid data in sigbuf upon return               */
/****************************************************************************/
uint32_t TPM_TickStampBlob(uint32_t keyhandle,
			   unsigned char *digestToStamp,
			   unsigned char *usageauth,
			   unsigned char *antireplay,
			   unsigned char *tickbuff,
			   struct tpm_buffer * signature)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_TickStampBlob);
    uint32_t ret;
    uint32_t keyhandle_no = htonl(keyhandle);
    uint32_t len;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    if (usageauth) {
	uint32_t size;
	session sess;

	TSS_gennonce(nonceodd);

	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP,
			      &sess,
			      usageauth, TPM_ET_KEYHANDLE, keyhandle);
	if (ret)
	    return ret;

	ret = TSS_authhmac(authdata, /*usageauth */
			 TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			 TSS_Session_GetENonce(&sess), nonceodd, c,
			 TPM_U32_SIZE, &ordinal_no, TPM_HASH_SIZE,
			 antireplay, TPM_HASH_SIZE, digestToStamp, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l % % L % o %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    TPM_HASH_SIZE, antireplay,
			    TPM_HASH_SIZE, digestToStamp,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);

	if (ret & ERR_MASK)
	    return ret;

	ret = TPM_Transmit(&tpmdata, "TickStampBlob - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	ret = tpm_buffer_load32(&tpmdata,
				TPM_DATA_OFFSET + TPM_CURRENT_TICKS_SIZE,
				&size);
	if (ret & ERR_MASK)
	    return ret;

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   TPM_CURRENT_TICKS_SIZE + TPM_U32_SIZE + size,
			   TPM_DATA_OFFSET, 0, 0);
	if (ret)
	    return ret;

	if (tickbuff)
	    memcpy(tickbuff,
		   &tpmdata.buffer[TPM_DATA_OFFSET],
		   TPM_CURRENT_TICKS_SIZE);

	if (signature) {
	    ret = tpm_buffer_load32(&tpmdata,
				    TPM_DATA_OFFSET +
				    TPM_CURRENT_TICKS_SIZE, &len);
	    if (ret & ERR_MASK)
		return ret;

	    SET_TPM_BUFFER(signature,
			   &tpmdata.buffer[TPM_DATA_OFFSET +
					   TPM_CURRENT_TICKS_SIZE +
					   TPM_U32_SIZE], len);
	}
    } else {
	ret = TSS_buildbuff("00 c1 T l l % %", &tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    TPM_HASH_SIZE, antireplay,
			    TPM_HASH_SIZE, digestToStamp);
	if (ret & ERR_MASK)
	    return ret;

	ret = TPM_Transmit(&tpmdata, "TickStampBlob");
	if (ret)
	    return ret;

	if (tickbuff)
	    memcpy(tickbuff,
		   &tpmdata.buffer[TPM_DATA_OFFSET],
		   TPM_CURRENT_TICKS_SIZE);

	if (signature) {
	    ret = tpm_buffer_load32(&tpmdata,
				    TPM_DATA_OFFSET +
				    TPM_CURRENT_TICKS_SIZE, &len);
	    if (ret & ERR_MASK)
		return ret;
	    SET_TPM_BUFFER(signature,
			   &tpmdata.buffer[TPM_DATA_OFFSET +
					   TPM_CURRENT_TICKS_SIZE +
					   TPM_U32_SIZE], len);
	}
    }
    return ret;
}
