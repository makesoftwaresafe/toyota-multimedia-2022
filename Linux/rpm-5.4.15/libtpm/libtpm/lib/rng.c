/********************************************************************************/
/*			     	TPM Random Number Generator Routines		*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: rng.c 4073 2010-04-30 14:44:14Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/*  Get Random Number                                                       */
/*                                                                          */
/*  The parameters are...                                                   */
/*                                                                          */
/*  numbytes : The number of bytes requested                                */
/*  buffer   : a buffer to hold the amount of requested bytes               */
/*  bytesret : The actual number of bytes that were returned                */
/****************************************************************************/
uint32_t TPM_GetRandom(uint32_t bytesreq,
		       unsigned char *buffer, uint32_t * bytesret)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)

    uint32_t ordinal_no = htonl(TPM_ORD_GetRandom);
    uint32_t numbytes_no = htonl(bytesreq);

    TSS_buildbuff("00 c1 T l l", &tpmdata, ordinal_no, numbytes_no);

    ret = TPM_Transmit(&tpmdata, "GetRandom");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, bytesret);
    if (ret & ERR_MASK)
	return ret;
    memcpy(buffer, &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE], *bytesret);

    return ret;
}

/****************************************************************************/
/*                                                                          */
/*  Stir Random Number Generator                                            */
/*                                                                          */
/*  The parameters are...                                                   */
/*                                                                          */
/*  data    : Data to add entropy to the random number generator's state    */
/*  datalen : The number of bytes; must be < 256                            */
/****************************************************************************/
uint32_t TPM_StirRandom(unsigned char *data, uint32_t datalen)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_StirRandom);

    TSS_buildbuff("00 c1 T l @", &tpmdata,
		  ordinal_no, (datalen & 0xff), data);

    ret = TPM_Transmit(&tpmdata, "StirRandom");
    return ret;
}
