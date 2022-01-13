/********************************************************************************/
/*			     	TPM Test Routines to detect bugs		*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: raw.c 4073 2010-04-30 14:44:14Z kgoldman $			*/
/********************************************************************************/

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

uint32_t TPM_RawDataRaw(uint32_t ordinal,
			unsigned char *data, uint32_t datalen)
{
    STACK_TPM_BUFFER(tpmdata)
    uint32_t ordinal_no = ntohl(ordinal);
    uint32_t ret;

    ret = TSS_buildbuff("00 c1 T l %", &tpmdata,
			ordinal_no, datalen, data);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "* RawData - Raw *");

    return ret;
}

uint32_t TPM_RawDataOIAP(uint32_t ordinal,
			 unsigned char *ownerauth,
			 unsigned char *data, uint32_t datalen)
{
    unsigned char enonce[TPM_HASH_SIZE];
    unsigned char nonceodd[TPM_HASH_SIZE];
    unsigned char authdata[TPM_HASH_SIZE];
    STACK_TPM_BUFFER(tpmdata)
    unsigned char c = 0;
    uint32_t ordinal_no = ntohl(ordinal);
    uint32_t ret;
    uint32_t authhandle;

    ret = TSS_OIAPopen(&authhandle, enonce);
    if (ret) {
	printf("Could not open OIAP session!\n");
	return ret;
    }

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0) {
	TSS_OIAPclose(authhandle);
	return ERR_CRYPT_ERR;
    }

    ret = TSS_authhmac(authdata, ownerauth, TPM_HASH_SIZE, enonce, nonceodd,
		     c, TPM_U32_SIZE, &ordinal_no, datalen, data, 0, 0);

    if (ret) {
	printf("Error calculating MAC.\n");
	TSS_OIAPclose(authhandle);
	return ret;
    }

    ret = TSS_buildbuff("00 c1 T l % l % o %", &tpmdata,
			ordinal_no,
			datalen, data,
			authhandle,
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_OIAPclose(authhandle);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "* RawData - OIAP*");

    TSS_OIAPclose(authhandle);

    return ret;
}

uint32_t TPM_RawDataOSAP(uint32_t keyhandle,
			 uint32_t ordinal,
			 unsigned char *ownerauth,
			 unsigned char *data, uint32_t datalen)
{
    unsigned char nonceodd[TPM_HASH_SIZE];
    unsigned char authdata[TPM_HASH_SIZE];
    STACK_TPM_BUFFER(tpmdata)
    unsigned char encauth[TPM_HASH_SIZE];
    unsigned char dummy[TPM_HASH_SIZE];
    unsigned char c = 0;
    uint32_t ordinal_no = ntohl(ordinal);
    uint32_t ret;
    session sess;
    osapsess *osap = &sess.type.osap;
    uint16_t keytype;
    unsigned char *passptr1;

    if (keyhandle == 0x40000000)
	keytype = TPM_ET_SRK;
    else
	keytype = TPM_ET_OWNER;

    ret = needKeysRoom(keyhandle, 0, 0, 0);
    if (ret)
	return ret;

    memset(dummy, 0x0, sizeof(dummy));

    if (ownerauth)
	passptr1 = ownerauth;
    else
	passptr1 = dummy;

    sess.sess_type = SESSION_OSAP;
    ret = TSS_OSAPopen(osap, ownerauth, keytype, keyhandle);
    if (ret) {
	printf("Could not open OIAP session!\n");
	return ret;
    }

    /* calculate encrypted authorization value */
    TPM_CreateEncAuth(&sess, passptr1, encauth, 0);

    /* generate odd nonce */
    ret = TSS_gennonce(nonceodd);
    if (ret == 0) {
	TSS_OSAPclose(osap);
	return ERR_CRYPT_ERR;
    }

    ret = TSS_authhmac(authdata, osap->ssecret, TPM_HASH_SIZE, osap->enonce,
		     nonceodd, c, TPM_U32_SIZE, &ordinal_no, datalen, data,
		     0, 0);

    if (ret) {
	printf("Error calculating MAC.\n");
	TSS_OSAPclose(osap);
	return ret;
    }

    ret = TSS_buildbuff("00 c1 T l % l % o %", &tpmdata,
			ordinal_no,
			datalen, data,
			osap->handle,
			TPM_NONCE_SIZE, nonceodd,
			c, TPM_HASH_SIZE, authdata);

    if (ret & ERR_MASK) {
	TSS_OSAPclose(osap);
	return ret;
    }

    ret = TPM_Transmit(&tpmdata, "* RawData - OIAP*");

    TSS_OSAPclose(osap);

    return ret;
}
