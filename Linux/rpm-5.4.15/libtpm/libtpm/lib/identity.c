/********************************************************************************/
/*			     	TPM Identity Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: identity.c 4645 2011-10-18 21:12:01Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/* Generate a new Attestation Identity Key                                  */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle     is the handle of the key                                   */
/* identityauth  is the encrypted usage authorization data for the new     */
/*               identity                                                   */
/* identitylabel is a digest of the identity label for the new TPM identity */
/* keyparms      is a pointer to a key that contains information for the    */
/*               new key                                                    */
/* key           is a pointer to an area that will receive the newly        */
/*               created identity key                                       */
/* srkauth       is the authorization data for the inputs and the SRK       */
/* ownerAuth     is the authorization data of the owner                     */
/* idbinding     is a pointer to an area that will receive the signature of */
/*               TPM_IDENTITY_CONTENTS                                      */
/* idbindingsize must indicate the size of the idbinding area on input and  */
/*               will hold the used size in the idbinding area on output    */
/****************************************************************************/
uint32_t TPM_MakeIdentity(unsigned char *identityauth,
			  unsigned char *identitylabel,
			  keydata * keyparms,
			  keydata * key,
			  unsigned char *keyblob,
			  unsigned int *keybloblen,
			  unsigned char *srkAuth,
			  unsigned char *ownerAuth,
			  unsigned char *idbinding, uint32_t * idbsize)
{
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_MakeIdentity);
    unsigned char c = 0;
    STACK_TPM_BUFFER(tpmdata)
	STACK_TPM_BUFFER(ser_key)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char nonceodd2[TPM_NONCE_SIZE];
    unsigned char authdata1[TPM_NONCE_SIZE];
    unsigned char authdata2[TPM_NONCE_SIZE];
    unsigned char encauth1[TPM_NONCE_SIZE];
    unsigned char dummy[TPM_HASH_SIZE];
    session sess;
    int serkeysize;
    int keylen;

    (void) idbinding;

    if (keyparms == NULL || key == NULL || identitylabel == NULL)
	return ERR_NULL_ARG;

    memset(dummy, 0x0, sizeof(dummy));
    if (identityauth == NULL)
	identityauth = dummy;

    /*
     * Serialize the key
     */
    serkeysize = TPM_WriteKey(&ser_key, keyparms);

    TSS_gennonce(nonceodd);

    if (srkAuth != NULL) {
	session sess0;
	TSS_gennonce(nonceodd2);

	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP | SESSION_DSAP,
			      &sess0, srkAuth, TPM_ET_SRK, 0);
	if (ret)
	    return ret;

	/*
	 * Open OSAP session
	 */
	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_DSAP,
			      &sess, ownerAuth, TPM_ET_OWNER, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    return ret;
	}

	/* Generate the encrypted usage authorization */
	TPM_CreateEncAuth(&sess, identityauth, encauth1, 0);

	ret = TSS_authhmac(authdata1, TSS_Session_GetAuth(&sess0),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess0),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_HASH_SIZE, encauth1, TPM_HASH_SIZE,
			 identitylabel, serkeysize, ser_key.buffer, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess);
	    return ret;
	}
	ret = TSS_authhmac(authdata2, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd2, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_HASH_SIZE, encauth1, TPM_HASH_SIZE,
			 identitylabel, serkeysize, ser_key.buffer, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess);
	    return ret;
	}

	ret = TSS_buildbuff("00 c3 T l % % % L % o % L % o %", &tpmdata,
			    ordinal_no,
			    TPM_HASH_SIZE, encauth1,
			    TPM_HASH_SIZE, identitylabel,
			    serkeysize, ser_key.buffer,
			    TSS_Session_GetHandle(&sess0),
			    TPM_NONCE_SIZE, nonceodd,
			    c,
			    TPM_HASH_SIZE, authdata1,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd2,
			    c, TPM_HASH_SIZE, authdata2);

	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess);
	    return ret;
	}

	ret = TPM_Transmit(&tpmdata, "MakeIdentity - AUTH2");
	TSS_SessionClose(&sess0);
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	/*
	 * Have to deserialize the key
	 */
	keylen = TSS_KeyExtract(&tpmdata, TPM_DATA_OFFSET, key);
	ret = tpm_buffer_load32(&tpmdata,
				TPM_DATA_OFFSET + keylen, idbsize);
	if (ret & ERR_MASK)
	    return ret;

	ret = TSS_checkhmac2(&tpmdata, ordinal_no, nonceodd,
			     TSS_Session_GetAuth(&sess0), TPM_HASH_SIZE,
			     nonceodd2,
			     TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			     keylen, TPM_DATA_OFFSET,
			     TPM_U32_SIZE, TPM_DATA_OFFSET + keylen,
			     *idbsize,
			     TPM_DATA_OFFSET + keylen + TPM_U32_SIZE, 0,
			     0);

    } else {
	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_DSAP, &sess, ownerAuth,
			    TPM_ET_OWNER, 0);
	if (ret)
	    return ret;

	/* Generate the encrypted usage authorization */
	TPM_CreateEncAuth(&sess, identityauth, encauth1, 0);

	ret = TSS_authhmac(authdata1, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_HASH_SIZE, encauth1, TPM_HASH_SIZE,
			 identitylabel, serkeysize, ser_key.buffer, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}


	ret = TSS_buildbuff("00 c2 T l % % % L % o %", &tpmdata,
			    ordinal_no,
			    TPM_HASH_SIZE, encauth1,
			    TPM_HASH_SIZE, identitylabel,
			    serkeysize, ser_key.buffer,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata1);

	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	ret = TPM_Transmit(&tpmdata, "MakeIdentity - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	/*
	 * Have to deserialize the key
	 */
	keylen = TSS_KeyExtract(&tpmdata, TPM_DATA_OFFSET, key);
	ret = tpm_buffer_load32(&tpmdata,
				TPM_DATA_OFFSET + keylen, idbsize);
	if (ret & ERR_MASK)
	    return ret;

	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   keylen, TPM_DATA_OFFSET, TPM_U32_SIZE,
			   TPM_DATA_OFFSET + keylen, *idbsize,
			   TPM_DATA_OFFSET + keylen + TPM_U32_SIZE, 0, 0);
    }
    /* extract the identity key blob, return to caller */
    if (ret == 0) {
	int len = TSS_KeySize(&tpmdata, TPM_DATA_OFFSET);
	if (keyblob != NULL) {
	    memcpy(keyblob, &tpmdata.buffer[TPM_DATA_OFFSET], len);
	    if (keybloblen != NULL)
		*keybloblen = len;
	}
    }
    return ret;
}

/****************************************************************************/
/*                                                                          */
/* Activate a TPM Identity                                                  */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle  is the handle of the identity key that is to be activated     */
/* blob       is the encrypted ASYM_CA_CONTENTS or TPM_EK_BLOB              */
/* blobsize   is the size of the blob parameter                             */
/* usageAuth  is the authorization data for the input and ID key            */
/* ownerAuth  is the authorization data of  the owner                       */
/* symkey     is a pointer to an area to receive the symmetric key          */
/* symkeysize indicates the size of the symkey area on input and receives   */
/*            the used size of symkey upon output.                          */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_ActivateIdentity(uint32_t keyhandle,
			      unsigned char *blob, uint32_t blobsize,
			      unsigned char *usageAuth,
			      unsigned char *ownerAuth,
			      struct tpm_buffer * symkey)
{
    uint32_t ret = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_ActivateIdentity);
    uint32_t blobsize_no = htonl(blobsize);
    uint32_t keyhandle_no = htonl(keyhandle);
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char c = 0;

    ALLOC_TPM_BUFFER(tpmdata, 0)

    uint32_t keylen;

    if (ownerAuth == NULL || symkey == NULL || (blob == NULL && blobsize != 0))
	return ERR_NULL_ARG;

    if (tpmdata == NULL)
	return ERR_MEM_ERR;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    TSS_gennonce(nonceodd);

    if (usageAuth) {
	unsigned char nonceodd2[TPM_NONCE_SIZE];
	unsigned char authdata1[TPM_NONCE_SIZE];
	unsigned char authdata2[TPM_NONCE_SIZE];
	session sess0;
	session sess1;

	TSS_gennonce(nonceodd2);

	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP | SESSION_DSAP,
			      &sess0,
			      usageAuth, TPM_ET_KEYHANDLE, keyhandle);
	if (ret)
	    goto exit;

	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP,
			      &sess1, ownerAuth, TPM_ET_OWNER, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    goto exit;
	}

	ret = TSS_authhmac(authdata1, TSS_Session_GetAuth(&sess0),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess0),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &blobsize_no, blobsize, blob, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess1);
	    goto exit;
	}

	ret = TSS_authhmac(authdata2, TSS_Session_GetAuth(&sess1),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess1),
			 nonceodd2, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &blobsize_no, blobsize, blob, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess1);
	    goto exit;
	}

	ret = TSS_buildbuff("00 c3 T l l @ L % o % L % o %", tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    blobsize, blob,
			    TSS_Session_GetHandle(&sess0),
			    TPM_NONCE_SIZE, nonceodd,
			    c,
			    TPM_HASH_SIZE, authdata1,
			    TSS_Session_GetHandle(&sess1),
			    TPM_NONCE_SIZE, nonceodd2,
			    c, TPM_HASH_SIZE, authdata2);

	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess0);
	    TSS_SessionClose(&sess1);
	    goto exit;
	}

	ret = TPM_Transmit(tpmdata, "ActivateIdentity - AUTH2");
	TSS_SessionClose(&sess0);
	TSS_SessionClose(&sess1);
	if (ret)
	    goto exit;

	keylen = TSS_SymKeySize(&tpmdata->buffer[TPM_DATA_OFFSET]);
	ret = TSS_checkhmac2(tpmdata, ordinal_no, nonceodd,
			     TSS_Session_GetAuth(&sess0), TPM_HASH_SIZE,
			     nonceodd2,
			     TSS_Session_GetAuth(&sess1), TPM_HASH_SIZE,
			     keylen, TPM_DATA_OFFSET, 0, 0);
	if (ret)
	    goto exit;

	if (symkey)
	    TSS_SetTPMBuffer(symkey,
			     &tpmdata->buffer[TPM_DATA_OFFSET], keylen);
    } else {
	unsigned char authdata[TPM_NONCE_SIZE];
	session sess;

	ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			      &sess, ownerAuth, TPM_ET_OWNER, 0);
	if (ret)
	    goto exit;

	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no,
			 TPM_U32_SIZE, &blobsize_no, blobsize, blob, 0, 0);
	if (ret) {
	    TSS_SessionClose(&sess);
	    goto exit;
	}

	ret = TSS_buildbuff("00 c2 T l l @ L % o %", tpmdata,
			    ordinal_no,
			    keyhandle_no,
			    blobsize, blob,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    goto exit;
	}

	ret = TPM_Transmit(tpmdata, "ActivateIdentity - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    goto exit;

	keylen = TSS_AsymKeySize(&tpmdata->buffer[TPM_DATA_OFFSET]);
	ret = TSS_checkhmac1(tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
			   keylen, TPM_DATA_OFFSET, 0, 0);

	if (ret)
	    goto exit;

	if (symkey)
	    TSS_SetTPMBuffer(symkey,
			     &tpmdata->buffer[TPM_DATA_OFFSET], keylen);
    }

exit:
    FREE_TPM_BUFFER(tpmdata);
    return ret;
}
