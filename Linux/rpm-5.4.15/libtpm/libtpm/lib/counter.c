/********************************************************************************/
/*			     	TPM Counter Routines				*/
/*			     Written by S. Berger 				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: counter.c 4168 2010-11-08 19:15:20Z stefanb $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/* Create a monotonic counter                                               */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the keyhandle to be used to establish the OSAP session      */
/* ownauth   is the SHA'ed TPM owner password                               */
/* label     is a label given to the counter by the user                    */
/* counterauth is the SHA'ed password for this counter, may be NULL         */
/*                                                                          */
/****************************************************************************/

uint32_t TPM_CreateCounter(uint32_t keyhandle, unsigned char *ownauth,	// HMAC key
			   uint32_t label,	// label for counter
			   unsigned char *counterauth,	// authdata for counter
			   uint32_t * counterId,
			   unsigned char *counterValue)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char encauth[TPM_HASH_SIZE];
    unsigned char dummy[TPM_HASH_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_CreateCounter);
    uint32_t label_no = htonl(label);
    uint32_t ret;
    uint16_t keytype;
    unsigned char *passptr1;
    session sess;

    memset(dummy, 0x0, sizeof(dummy));

    /* check input arguments */
    if (ownauth == NULL || counterauth == NULL)
	return ERR_NULL_ARG;

    if (keyhandle == 0x40000000)
	keytype = TPM_ET_SRK;
    else
	keytype = TPM_ET_OWNER;
    if (counterauth != NULL)
	passptr1 = counterauth;
    else
	passptr1 = dummy;

    if (keyhandle != 0) {
	ret = needKeysRoom(keyhandle, 0, 0, 0);
	if (ret)
	    return ret;
    }

    /* Open OSAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP,
			  &sess, ownauth, keytype, keyhandle);
    if (ret)
	return ret;

    /* calculate encrypted authorization value */
    TPM_CreateEncAuth(&sess, passptr1, encauth, 0);

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* move Network byte order data to variable for HMAC calculation */

    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_HASH_SIZE, encauth,
		     TPM_U32_SIZE, &label_no, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l % % L % o %", &tpmdata,
			ordinal_no,
			TPM_HASH_SIZE, encauth,
			4, &label_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "CreateCounter");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       TPM_U32_SIZE + TPM_COUNTER_VALUE_SIZE,
		       TPM_DATA_OFFSET, 0, 0);
    if (ret)
	return ret;

    if (counterId != NULL) {
	ret = tpm_buffer_load32(&tpmdata, TPM_DATA_OFFSET, counterId);
	if (ret & ERR_MASK)
	    return ret;
    }

    if (counterValue != NULL)
	memcpy(counterValue,
	       &tpmdata.buffer[TPM_DATA_OFFSET + TPM_U32_SIZE],
	       TPM_COUNTER_VALUE_SIZE);
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Increment a counter                                                      */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the keyhandle to be used to establish the OSAP session      */
/* counterid is the ID of the counter the user wants to have incremented    */
/* counterauth is the SHA'ed password for the counter                       */
/* counterbuffer is a pointer to a buffer that will receive the current     */
/*               state of the counter                                       */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_IncrementCounter(uint32_t countid,	// id of the counter
			      unsigned char *counterauth,	// authdata for counter
			      unsigned char *counterbuffer	// buffer to return the counter in
    )
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_IncrementCounter);
    uint32_t countid_no = htonl(countid);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (counterauth == NULL || counterbuffer == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_OIAP | SESSION_OSAP,
			  &sess, counterauth, TPM_ET_COUNTER, countid);
    if (ret)
	return ret;

    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE, &countid_no,
		     0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l L % o %", &tpmdata,
			ordinal_no,
			countid_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    ret = TPM_Transmit(&tpmdata, "IncrementCounter - AUTH1");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       TPM_COUNTER_VALUE_SIZE, TPM_DATA_OFFSET, 0, 0);

    memcpy(counterbuffer,
	   &tpmdata.buffer[TPM_DATA_OFFSET], TPM_COUNTER_VALUE_SIZE);

    if (ret)
	return ret;

    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Read a counter                                                           */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* countid   is the ID of the counter to read                               */
/* counterauth is the SHA'ed password for the counter if it requires a      */
/*             password                                                     */
/* counterbuffer is a pointer to an area that will hold the current value   */
/*               of the counter                                             */
/****************************************************************************/
uint32_t TPM_ReadCounter(uint32_t countid,	// id of the counter
			 unsigned char *counterauth,	// hashed authdata for counter (unused...)
			 unsigned char *counterbuffer	// buffer to return the counter in
    )
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = htonl(TPM_ORD_ReadCounter);
    uint32_t countid_no = htonl(countid);
    uint32_t ret;

    (void) counterauth;
    /* check input arguments */
    if (counterbuffer == NULL)
	return ERR_NULL_ARG;

#if 0
    if (counterauth != NULL) {
	unsigned char c = 0;
	unsigned char nonceodd[TPM_NONCE_SIZE];
	unsigned char evennonce[TPM_NONCE_SIZE];
	unsigned char authdata[TPM_NONCE_SIZE];
	uint32_t authhandle;
	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	/* Open OIAP Session */
	ret = TSS_OIAPopen(&authhandle, evennonce);
	if (ret)
	    return ret;
	/* move Network byte order data to variable for HMAC calculation */

	ret = TSS_authhmac(authdata, counterauth, TPM_HASH_SIZE, evennonce,
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &countid_no, 0, 0);
	if (ret) {
	    TSS_OIAPclose(authhandle);
	    return ret;
	}

	/* build the request buffer */
	ret = TSS_buildbuff("00 c2 T l l l % o %", &tpmdata,
			    ordinal_no,
			    countid_no,
			    authhandle,
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);


	ret = TPM_Transmit(&tpmdata, "ReadCounter - AUTH 1");

	if (ret) {
	    TSS_OIAPclose(authhandle);
	    return ret;
	}

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd, counterauth,
			   TPM_HASH_SIZE, TPM_COUNTER_VALUE_SIZE,
			   TPM_DATA_OFFSET, 0, 0);

	if (ret)
	    return ret;

	memcpy(counterbuffer,
	       &tpmdata.buffer[TPM_DATA_OFFSET], TPM_COUNTER_VALUE_SIZE);

    } else {
#endif
	/* build the request buffer */
	ret = TSS_buildbuff("00 c1 T l l", &tpmdata,
			    ordinal_no, countid_no);
	if (ret & ERR_MASK)
	    return ret;

	ret = TPM_Transmit(&tpmdata, "TPM_ReadCounter");
	if (ret)
	    return ret;

	memcpy(counterbuffer,
	       &tpmdata.buffer[TPM_DATA_OFFSET], TPM_COUNTER_VALUE_SIZE);
#if 0
    }
#endif

    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Release a counter                                                        */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* countid   is the id of the counter to release                            */
/* counterauth is the SHA'ed password for this counter, maybe be NULL if    */
/*             this counter does not require a password                     */
/****************************************************************************/
uint32_t TPM_ReleaseCounter(uint32_t countid,	// id of the counter
			    unsigned char *counterauth	// hashed authdata for counter
    )
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_ReleaseCounter);
    uint32_t countid_no = htonl(countid);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (counterauth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_OIAP | SESSION_OSAP,
			  &sess, counterauth, TPM_ET_COUNTER, countid);
    if (ret)
	return ret;

    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE, &countid_no,
		     0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l L % o %", &tpmdata,
			ordinal_no,
			countid_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "ReleaseCounter - AUTH1");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    if (ret)
	return ret;

    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Release a counter using owner credentials                                */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* countid   is the id of the counter to release                            */
/* ownerauth is the SHA'ed password for the TPM                             */
/****************************************************************************/
uint32_t TPM_ReleaseCounterOwner(uint32_t countid,	// id of the counter
				 unsigned char *ownerauth	// authdata for counter
    )
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_ReleaseCounterOwner);
    uint32_t countid_no = htonl(countid);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (ownerauth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open OIAP Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerauth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, TPM_U32_SIZE, &countid_no,
		     0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l l L % o %", &tpmdata,
			ordinal_no,
			countid_no,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "ReleaseCounterOwner");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    return ret;
}
