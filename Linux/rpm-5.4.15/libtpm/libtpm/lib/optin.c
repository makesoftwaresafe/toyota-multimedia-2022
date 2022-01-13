/********************************************************************************/
/*			     	TPM Testing Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: optin.c 4089 2010-06-09 00:50:31Z kgoldman $			*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

uint32_t TPM_SetOwnerInstall(TPM_BOOL state)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SetOwnerInstall);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l o", &tpmdata, ordinal_no, state);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SetOwnerInstall");
    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}


uint32_t TPM_OwnerSetDisable(unsigned char *ownerauth, TPM_BOOL state)
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_OwnerSetDisable);
    uint32_t ret;
    session sess;

    /* check input arguments */
    if (ownerauth == NULL)
	return ERR_NULL_ARG;

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ERR_CRYPT_ERR;

    /* Open Session */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownerauth, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* move Network byte order data to variable for hmac calculation */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, c,
		     TPM_U32_SIZE, &ordinal_no, sizeof(TPM_BOOL), &state,
		     0, 0);

    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* build the request buffer */
    ret = TSS_buildbuff("00 c2 T l o L % o %", &tpmdata,
			ordinal_no,
			state,
			TSS_Session_GetHandle(&sess),
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "OwnerSetDisable");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    /* check the HMAC in the response */
    ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0, 0);

    return ret;
}

uint32_t TPM_SetTempDeactivated(unsigned char *operatorauth	// HMAC key
    )
{
    STACK_TPM_BUFFER(tpmdata)
    unsigned char nonceodd[TPM_NONCE_SIZE];
    unsigned char authdata[TPM_NONCE_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = htonl(TPM_ORD_SetTempDeactivated);
    uint32_t ret;

    /* check input arguments */

    if (operatorauth) {
	/* Open OIAP Session */
	session sess;
	ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP,
			      &sess, operatorauth, TPM_ET_OWNER, 0);
	if (ret)
	    return ret;

	/* calculate encrypted authorization value */
	/* generate odd nonce */
	ret = TSS_gennonce(nonceodd);
	if (ret == 0)
	    return ERR_CRYPT_ERR;

	ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess),
			 TPM_HASH_SIZE, TSS_Session_GetENonce(&sess),
			 nonceodd, c, TPM_U32_SIZE, &ordinal_no, 0, 0);

	if (ret) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* build the request buffer */
	ret = TSS_buildbuff("00 c1 T l L % o %", &tpmdata,
			    ordinal_no,
			    TSS_Session_GetHandle(&sess),
			    TPM_NONCE_SIZE, nonceodd,
			    c, TPM_HASH_SIZE, authdata);
	if (ret & ERR_MASK) {
	    TSS_SessionClose(&sess);
	    return ret;
	}

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "SetTempDeactivated - AUTH1");
	TSS_SessionClose(&sess);
	if (ret)
	    return ret;

	/* check the HMAC in the response */
	ret = TSS_checkhmac1(&tpmdata, ordinal_no, nonceodd,
			   TSS_Session_GetAuth(&sess), TPM_HASH_SIZE, 0,
			   0);
    } else {
	/* build the request buffer */
	ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);

	/* transmit the request buffer to the TPM device and read the reply */
	ret = TPM_Transmit(&tpmdata, "SetTempDeactivated");
    }

    return ret;
}

uint32_t TPM_PhysicalEnable()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_PhysicalEnable);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "PhysicalEnable");
    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_PhysicalDisable()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_PhysicalDisable);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "PhysicalDisable");
    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_PhysicalSetDeactivated(TPM_BOOL state)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_PhysicalSetDeactivated);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l o", &tpmdata, ordinal_no, state);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "PhysicalSetDeactivated");

    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_SetOperatorAuth(unsigned char *operatorAuth)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SetOperatorAuth);
    STACK_TPM_BUFFER(tpmdata)

    if (operatorAuth == NULL)
	return ERR_NULL_ARG;

    ret = TSS_buildbuff("00 c1 T l %", &tpmdata,
			ordinal_no, TPM_HASH_SIZE, operatorAuth);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SetOperatorAuth");

    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}
