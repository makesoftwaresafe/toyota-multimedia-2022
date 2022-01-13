#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <adit_typedef.h>
#include "iap2_dlt_log.h"
#include "adit_dlt.h"

#include "ipodauth.h"
#include "authentication.h"
#include "cinemo_apple_auth_interface.h"


int Create_AppleAuth_Custom(const struct cali_create_params * params,struct cali_context * ctx)
{
    int ret = CALI_NO_ERROR;

    /* APP registration will be done in Application. Below is for test purpose only*/
    /*IAP2REGISTERAPPWITHDLT("CIAP", "iAP2 Logging");*/
    DLT_REGISTER_CONTEXT_LL_TS(iAP2AuthCtxt, "CAUT", "CINEMO Auth logging" , DLT_LOG_DEBUG, DLT_TRACE_STATUS_OFF);

    if(ctx == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR," cali_context is NULL");
        return CALI_ERROR_RESOURCES;
    }

    ctx->protocol_version = CINEMO_AUTHENTICATION_LIBRARY_PROTOCOL_VERSION;

    ctx->delete_authentication = &Delete_AppleAuth_Custom;
    ctx->get_certificate = &Get_Certificate_Custom;
    ctx->get_challenge_response = &Get_Challenge_Response_Custom;
    ctx->get_serial_number = &Get_Serial_Number_Custom;
    ctx->auth_handle = "ADITAppleAuth"; //This handle is not used. If Auth Handle is NULL, Cinemo Engine complains that the authentication library uses incomplete parameters (CinemoErrorAuthDevice)

    IAP2AUTHDLTLOG(DLT_LOG_VERBOSE, "Protocol version %d",ctx->protocol_version);

    return ret;
}

int Delete_AppleAuth_Custom(cali_handle auth_handle)
{
    /*DLT_UNREGISTER_CONTEXT(iAP2AuthCtxt);
    IAP2DEREGISTERAPPWITHDLT();*/
    // Deregister will be done in iAP2USBHostCinemoDelete() since DLT logs are used in all the ADIT libraries for cinemo SDK

    if(auth_handle != NULL)
    {
        auth_handle = NULL;
    }
    DLT_UNREGISTER_CONTEXT(iAP2AuthCtxt);

    return CALI_NO_ERROR;
}

int Get_Serial_Number_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest)
{
    int ret = CALI_NO_ERROR;
    IAP2AUTHDLTLOG(DLT_LOG_VERBOSE,"Fetching Authentication Serial Number : Size of input buffer %d",npdest);

    if(pdest == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Address of Serial Number data buffer is invalid");
        return CALI_ERROR_RESOURCES;
    }

    ret = AuthenticationGetSerialNumber(pdest);

    if(ret == CALI_NO_ERROR)
    {
        ret = strlen(pdest);
        IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "Serial Number size %d",ret);
    }
    else if (ret > CALI_NO_ERROR)
    {
        IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "Serial Number is not available for this copro version %d",ret);
        ret = CALI_ERROR_NOTIMPLEMENTED;
    }
    else
    {
        ret = CALI_ERROR_IO;
    }

    IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "AuthenGetSerialNumber returns number of bytes read %d ",ret);
    return ret;
}

int Get_Certificate_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest)
{
    U16 ret = CALI_NO_ERROR;
    IAP2AUTHDLTLOG(DLT_LOG_VERBOSE, " Fetching Authentication Certificate Data : Size of input buffer %d",npdest);

    if(pdest == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Address of Certificate data buffer is invalid ");
        return CALI_ERROR_RESOURCES;
    }

    AuthenticationGetCertificate(&ret,pdest);

    IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "AuthenticationGetCertificate returns number of bytes read %d ",ret);
    return (int)ret;
}

int Get_Challenge_Response_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest,const void * pchallenge,unsigned int nbytes)
{
    int ret = CALI_NO_ERROR;
    U16 length = CALI_NO_ERROR;
    IAP2AUTHDLTLOG(DLT_LOG_VERBOSE, "Fetching authentication challenge response data : Size of input buffer %d", npdest);

    if(pdest == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Address of Challenge response data buffer is invalid ");
        return CALI_ERROR_RESOURCES;
    }

    ret = AuthenticationGetSignatureData(pchallenge,nbytes,&length,pdest);
    IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "AuthenticationGetSignatureData returns %d, read %d number of bytes",ret, length );

    if(ret == CALI_NO_ERROR)
    {
        return (int)length;
    }
    else
    {
        return CALI_ERROR_IO;
    }
}
