/********************************************************************************/
/*			     	TPM SHA Digest Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: sha.c 4089 2010-06-09 00:50:31Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/* Start a SHA1 Digest					                    */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_SHA1Start(uint32_t * maxNumBytes)
{
    uint32_t ordinal_no;
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)

    /* move Network byte order data to variable for hmac calculation */
    ordinal_no = htonl(TPM_ORD_SHA1Start);

    TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "SHA1Start");
    if (ret)
	return ret;

    ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, maxNumBytes);

    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Do a SHA1 Update					                    */
/*                                                                          */
/* The arguments are ...                                                    */
/*                                                                          */
/* data       : A block of data whose size must be a multiple of 64         */
/* datalen    : The length of the data block                                */
/****************************************************************************/
uint32_t TPM_SHA1Update(void *data, uint32_t datalen)
{
    uint32_t ordinal_no;
    uint32_t ret;
    struct tpm_buffer *tpmdata = TSS_AllocTPMBuffer(datalen + 20);
    /* move Network byte order data to variable for hmac calculation */
    ordinal_no = htonl(TPM_ORD_SHA1Update);

    if (tpmdata == NULL)
	return ERR_BAD_SIZE;

    ret = TSS_buildbuff("00 c1 T l @", tpmdata, ordinal_no, datalen, data);
    if (ret & ERR_MASK)
	goto err_exit;

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(tpmdata, "SHA1Update");

  err_exit:
    TSS_FreeTPMBuffer(tpmdata);

    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Do a SHA1 Complete					                    */
/*                                                                          */
/* The arguments are ...                                                    */
/*                                                                          */
/* data       : A block of data whose size must be 64 or less               */
/* datalen    : The length of the data block                                */
/* hash       : A block of size TPM_HASH_SIZE (=20) to hold the SHA1 hash   */
/****************************************************************************/
uint32_t TPM_SHA1Complete(void *data, uint32_t datalen,
			  unsigned char *hash)
{
    uint32_t ordinal_no;
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)

    /* move Network byte order data to variable for hmac calculation */
    ordinal_no = htonl(TPM_ORD_SHA1Complete);

    TSS_buildbuff("00 c1 T l @", &tpmdata, ordinal_no, datalen, data);

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "SHA1Complete");

    if (ret)
	return ret;

    memcpy(hash, &tpmdata.buffer[TPM_DATA_OFFSET], TPM_HASH_SIZE);
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Do a SHA1 Complete Extend 				                    */
/*                                                                          */
/* The arguments are ...                                                    */
/*                                                                          */
/* data       : A block of data whose size must be 64 or less               */
/* datalen    : The length of the data block                                */
/* pcrNum     : The index of the CPR to be modified                         */
/* hash       : A block of size TPM_HASH_SIZE (=20) to hold the SHA1 hash   */
/* pcrValue   : A block of size TPM_HASH_SIZE (=20) to hold the PCR value   */
/****************************************************************************/
uint32_t TPM_SHA1CompleteExtend(void *data, uint32_t datalen,
				uint32_t pcrNum,
				unsigned char *hash,
				unsigned char *pcrValue)
{
    uint32_t ordinal_no;
    uint32_t pcrNum_no = htonl(pcrNum);
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)

	/* move Network byte order data to variable for hmac calculation */
	ordinal_no = htonl(TPM_ORD_SHA1CompleteExtend);

    TSS_buildbuff("00 c1 T l l @", &tpmdata,
		  ordinal_no, pcrNum_no, datalen, data);

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "SHA1CompleteExtend");
    if (ret)
	return ret;

    memcpy(hash, &tpmdata.buffer[TPM_DATA_OFFSET], TPM_HASH_SIZE);

    memcpy(pcrValue, &tpmdata.buffer[TPM_DATA_OFFSET + TPM_HASH_SIZE],
	   TPM_HASH_SIZE);

    return ret;
}
