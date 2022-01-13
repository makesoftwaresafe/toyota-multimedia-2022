/********************************************************************************/
/*			     	TPM Startup Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: startup.c 4073 2010-04-30 14:44:14Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>
#include <hmac.h>

#include "debug.h"

uint32_t TPM_Startup(uint16_t type)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_Startup);
    STACK_TPM_BUFFER(tpmdata)
    uint16_t type_no = htons(type);

    ret = TSS_buildbuff("00 c1 T l s", &tpmdata, ordinal_no, type_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "Startup");
    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_SaveState()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_SaveState);
    STACK_TPM_BUFFER(tpmdata)

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "SaveState");
    if (ret == 0 && tpmdata.used != 10)
	ret = ERR_BAD_RESP;

    return ret;
}

uint32_t TPM_Init()
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_Init);
    STACK_TPM_BUFFER(tpmdata);

    ret = TSS_buildbuff("00 c1 T l", &tpmdata, ordinal_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "Init");

    return ret;
}
