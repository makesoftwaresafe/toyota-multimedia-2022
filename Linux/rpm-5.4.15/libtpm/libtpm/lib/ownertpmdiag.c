/********************************************************************************/
/*			     	TPM TakeOwnerShip Routine			*/
/*			     Written by J. Kravitz 				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: ownertpmdiag.c 4073 2010-04-30 14:44:14Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "debug.h"

#define TPM_SRK_PARAM_BUFF_SIZE 256
#define RSA_MODULUS_BYTE_SIZE 256
#define RSA_MODULUS_BIT_SIZE  ( RSA_MODULUS_BYTE_SIZE * 8 )

/****************************************************************************/
/*                                                                          */
/*  Take Ownership of the TPM                                               */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* ownpass   is the authorization data (password) for the new owner         */
/* srkpass   is the authorization data (password) for the new root key      */
/*           if NULL, authorization required flag is turned off             */
/*           both authorization values must be 20 bytes long                */
/* key       a pointer to a keydata structure to receive the SRK public key */
/*           or NULL if this information is not required                    */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_TakeOwnership12(unsigned char *ownpass,
			     unsigned char *srkpass, keydata * key)
{
    /* required OAEP padding P parameter */
    unsigned char tpm_oaep_pad_str[] = { 'T', 'C', 'P', 'A' };
    uint32_t ret;
    int iret;
    STACK_TPM_BUFFER(tpmdata)
    pubkeydata tpmpubkey;	/* public endorsement key data */
    uint32_t srkparamsize;	/* SRK parameter buffer size */
    RSA *pubkey;		/* PubEK converted to OpenSSL format */
    unsigned char padded[RSA_MODULUS_BYTE_SIZE];	/* area for OAEP padding */
    keydata srk;		/* key info for SRK */
    unsigned char dummypass[TPM_HASH_SIZE];	/* dummy srk password */
    unsigned char *spass;	/* pointer to srkpass or dummy */
    STACK_TPM_BUFFER(response);

    /* data to be inserted into Take Owner Request Buffer (in Network Byte Order) */
    /* the uint32_t and uint16_t values are stored in network byte order so they
     ** are in the correct format when being hashed by the HMAC calculation */
    uint32_t command;		/* command ordinal */
    uint16_t protocol;		/* protocol ID */
    uint32_t oencdatasize;	/* owner auth data encrypted size */
    unsigned char ownerencr[RSA_MODULUS_BYTE_SIZE];	/* owner auth data encrypted */
    uint32_t sencdatasize;	/* srk auth data encrypted size */
    unsigned char srkencr[RSA_MODULUS_BYTE_SIZE];	/* srk auth data encrypted */
    STACK_TPM_BUFFER(srk_param_buff)
    unsigned char nonceodd[TPM_HASH_SIZE];	/* odd nonce */
    unsigned char authdata[TPM_HASH_SIZE];	/* auth data */
    session sess;

    /* check that parameters are valid */
    if (ownpass == NULL)
	return ERR_NULL_ARG;

    if (srkpass == NULL) {
	memset(dummypass, 0, sizeof dummypass);
	spass = dummypass;
    } else
	spass = srkpass;
    /* set up command and protocol values for TakeOwnership function */
    command = htonl(0x0d);
    protocol = htons(0x05);
    /* get the TPM Endorsement Public Key */
    ret = TPM_ReadPubek(&tpmpubkey);
    if (ret)
	return ret;

    /* convert the public key to OpenSSL format */
    pubkey = TSS_convpubkey(&tpmpubkey);
    if (pubkey == NULL)
	return ERR_CRYPT_ERR;
    memset(ownerencr, 0, sizeof ownerencr);
    memset(srkencr, 0, sizeof srkencr);

    /* Pad and then encrypt the owner data using the RSA public key */
    iret = RSA_padding_add_PKCS1_OAEP(padded, RSA_MODULUS_BYTE_SIZE,
				      ownpass, TPM_HASH_SIZE,
				      tpm_oaep_pad_str,
				      sizeof tpm_oaep_pad_str);
    if (iret == 0)
	return ERR_CRYPT_ERR;
    iret = RSA_public_encrypt(RSA_MODULUS_BYTE_SIZE, padded, ownerencr,
			   pubkey, RSA_NO_PADDING);
    if (iret < 0)
	return ERR_CRYPT_ERR;
    oencdatasize = htonl(iret);

    /* Pad and then encrypt the SRK data using the RSA public key */
    iret = RSA_padding_add_PKCS1_OAEP(padded, RSA_MODULUS_BYTE_SIZE,
				      spass, TPM_HASH_SIZE,
				      tpm_oaep_pad_str,
				      sizeof tpm_oaep_pad_str);
    if (iret == 0)
	return ERR_CRYPT_ERR;
    iret =
	RSA_public_encrypt(RSA_MODULUS_BYTE_SIZE, padded, srkencr, pubkey,
			   RSA_NO_PADDING);
    if (iret < 0)
	return ERR_CRYPT_ERR;
    sencdatasize = htonl(iret);
    RSA_free(pubkey);
    if ((int) ntohl(oencdatasize) < 0)
	return ERR_CRYPT_ERR;
    if ((int) ntohl(sencdatasize) < 0)
	return ERR_CRYPT_ERR;
    /* fill the SRK-params key structure */
    /* get tpm version */
    ret = TPM_GetCapability(TPM_CAP_VERSION, NULL, &response);
    if (ret)
	return ret;

    memcpy(&(srk.v.ver), response.buffer, 4);
    srk.keyUsage = 0x0011;	/* Storage Key */
    srk.keyFlags = 0;

    /* changed for 1.2 to TPM_AUTH_PRIV_USE_ONLY */
    if (srkpass)
	srk.authDataUsage = 0x03;
    else
	srk.authDataUsage = 0x00;

    srk.encData.size = 0;	/* private key not specified here */
    srk.pub.algorithmParms.algorithmID = 0x00000001;	/* RSA */
    srk.pub.algorithmParms.encScheme = 0x0003;	/* RSA OAEP SHA1 MGF1 */
    srk.pub.algorithmParms.sigScheme = 0x0001;	/* NONE */
    srk.pub.algorithmParms.u.rsaKeyParms.keyLength = RSA_MODULUS_BIT_SIZE;
    srk.pub.algorithmParms.u.rsaKeyParms.numPrimes = 2;
    srk.pub.algorithmParms.u.rsaKeyParms.exponentSize = 0;	/* defaults to 0x010001 */
    srk.pub.pubKey.keyLength = 0;	/* not used here */
    srk.pub.pcrInfo.size = 0;	/* not used here */
    /* convert to a memory buffer */
    srkparamsize = TPM_WriteKey(&srk_param_buff, &srk);

    /* generate the odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ret;

    /* initiate the OIAP protocol */
    ret = TSS_SessionOpen(SESSION_OSAP | SESSION_OIAP,
			  &sess, ownpass, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    /* calculate the Authorization Data */
    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, 0,
		     TPM_U32_SIZE, &command, TPM_U16_SIZE, &protocol,
		     TPM_U32_SIZE, &oencdatasize, ntohl(oencdatasize),
		     ownerencr, TPM_U32_SIZE, &sencdatasize,
		     ntohl(sencdatasize), srkencr, srkparamsize,
		     srk_param_buff.buffer, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* insert all the calculated fields into the request buffer */
    ret = TSS_buildbuff("00 c2 T l s @ @ % L % 00 %", &tpmdata,
			command,
			protocol,
			ntohl(oencdatasize),
			ownerencr,
			ntohl(sencdatasize),
			srkencr,
			srkparamsize,
			srk_param_buff.buffer,
			TSS_Session_GetHandle(&sess),
			TPM_HASH_SIZE, nonceodd, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    /* transmit the request buffer to the TPM device and read the reply */
    ret = TPM_Transmit(&tpmdata, "Take Ownership");
    TSS_SessionClose(&sess);
    if (ret)
	return ret;

    /* check the response HMAC */
    srkparamsize = TSS_KeySize(&tpmdata, TPM_DATA_OFFSET);
    if (srkparamsize & ERR_MASK)
	return srkparamsize;

    ret = TSS_checkhmac1(&tpmdata, command, nonceodd,
		       TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		       srkparamsize, TPM_DATA_OFFSET, 0, 0);
    if (ret)
	return ret;
    /* convert the returned key to a structure */
    if (key == NULL)
	return 0;
    TSS_KeyExtract(&tpmdata, TPM_DATA_OFFSET, key);
    return 0;
}

/****************************************************************************/
/*                                                                          */
/*  Clear the TPM                                                           */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* ownpass   is the authorization data (password) for the owner             */
/*           the authorization value must be 20 bytes long                  */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_OwnerClear12(unsigned char *ownpass)
{
    uint32_t ret;
    STACK_TPM_BUFFER(tpmdata)
    session sess;

    /* fields to be inserted into Owner Clear Request Buffer */
    uint32_t command;
    unsigned char nonceodd[TPM_HASH_SIZE];
    unsigned char authdata[TPM_HASH_SIZE];

    /* check that parameters are valid */
    if (ownpass == NULL)
	return ERR_NULL_ARG;
    command = htonl(91);

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0)
	return ret;

    /* start OIAP Protocol */
    ret = TSS_SessionOpen(SESSION_DSAP | SESSION_OSAP | SESSION_OIAP,
			  &sess, ownpass, TPM_ET_OWNER, 0);
    if (ret)
	return ret;

    ret = TSS_authhmac(authdata, TSS_Session_GetAuth(&sess), TPM_HASH_SIZE,
		     TSS_Session_GetENonce(&sess), nonceodd, 0,
		     TPM_U32_SIZE, &command, 0, 0);
    if (ret) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TSS_buildbuff("00 c2 T l L % 00 %", &tpmdata,
			command,
			TSS_Session_GetHandle(&sess),
			TPM_HASH_SIZE, nonceodd, TPM_HASH_SIZE, authdata);
    if (ret & ERR_MASK) {
	TSS_SessionClose(&sess);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "Owner Clear");
    TSS_SessionClose(&sess);
    return ret;
}
