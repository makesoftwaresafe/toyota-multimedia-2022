/********************************************************************************/
/*			     	TPM Eviction Routines				*/
/*			     Written by S. Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: eviction.c 4073 2010-04-30 14:44:14Z kgoldman $		*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

#include <tpmfunc.h>

#include <hmac.h>

#include "debug.h"

uint32_t TPM_FlushSpecific(uint32_t handle, uint32_t resourceType)
{
    uint32_t ret;
    uint32_t ordinal_no = htonl(TPM_ORD_FlushSpecific);
    uint32_t handle_no = htonl(handle);
    uint32_t resourceType_no = htonl(resourceType);
    STACK_TPM_BUFFER(tpmdata)

#if 0
    if (resourceType == TPM_RT_KEY) {
	ret = needKeysRoom(handle, 0, 0, 0);
	if (ret)
	    return ret;
    }
#endif

    ret = TSS_buildbuff("00 c1 T l l l", &tpmdata,
			ordinal_no, handle_no, resourceType_no);
    if (ret & ERR_MASK)
	return ret;

    ret = TPM_Transmit(&tpmdata, "FlushSpecific");

    return ret;
}
