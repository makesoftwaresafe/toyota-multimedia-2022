/************************************************************************
 * @file: FileTransfer.cpp
 *
 * @version: 1.0
 *
 * @description: This module implements callback functions for
 * handling iAP2 file transfer message from iAP2Link and
 * acknowledgement for the initiated file transfer from Application(s).
 *
 * @component: platform/ipod
 *
 * @author: Sundhar Asokan, Sundhar.Asokan@in.bosch.com 2017
 *
 * @copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include "iap2_init_private.h"
#include <adit_logging.h>
#include "Core.h"
#include "FileTransfer.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

extern "C" {

S32 iap2FTSetup_CB(iAP2Device_t *this_iAP2Device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTDataRcvd_CB(iAP2Device_t *iap2Device,    iAP2FileTransferSession_t *iAP2FileXferSession,    void* context);

S32 handleFTClientMsg(iAP2Device_st* this_iAP2Device,iAP2FTMessage_t *iap2msg)
{
    int rc=IAP2_CTL_ERROR;
    switch(iap2msg->ftcmd)
    {
        case FileTransferStartOK:
        {
            LOGD_DEBUG((iap2, "StartOK"));
            rc = iAP2FileXferStartOk(this_iAP2Device,iap2msg->ftid,TRUE);//start cmd sent to Apple Device
            break;
        }
        case FileTransferStartNOK:
        {
            LOGD_DEBUG((iap2, "StartNOK"));
            rc = iAP2FileXferStartOk(this_iAP2Device,iap2msg->ftid,FALSE);//Cancel cmd sent to Apple Device
            break;
        }
        default:
        {
            LOGD_DEBUG((iap2, " Default setup response from Application"));
        }
        break;
    }
    return rc;
}

S32 handleFTServerMsg(iAP2Device_t *device, iAP2FileTransferSession_t *iAP2FileXferSession,void* context,IAP2FTCmd_t ftcmd)
{
    iAP2Device_st* this_iAP2Device=(iAP2Device_st*)device;
    iAP2FTMessage_t msg;
    context =context;
    int rc=IAP2_CTL_ERROR;
    LOGD_DEBUG((iap2, "Handle ftcmd %d", ftcmd));
    msg.deviceid = (U32)this_iAP2Device->iAP2DeviceId;
    msg.ftid =  iAP2FileXferSession->iAP2FileTransferID;
    msg.ftcmd = ftcmd;
    msg.len= iAP2FileXferSession->iAP2FileXferRxLen;
    if(this_iAP2Device->iAP2StackCallbacks.p_iap2SendFileTransfer2Application_cb != NULL)
    {
        if(ftcmd==FileTransferDataRcvd)
        {
            memcpy(msg.buff, iAP2FileXferSession->iAP2FileXferRxBuf,iAP2FileXferSession->iAP2FileXferRxLen);
            rc = this_iAP2Device->iAP2StackCallbacks.p_iap2SendFileTransfer2Application_cb(this_iAP2Device,msg.ftid, (U8*) &msg, IAP2FTMESSAGE_HDRSIZE + msg.len);
        }
        else
        {
            rc = this_iAP2Device->iAP2StackCallbacks.p_iap2SendFileTransfer2Application_cb(this_iAP2Device,msg.ftid, (U8*) &msg, IAP2FTMESSAGE_HDRSIZE);
        }
    }
    return rc;
}

S32 iap2FTSetup_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferSetup);
}

S32 iap2FTDataRcvd_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferDataRcvd);
}

S32 iap2FTfailure_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferFailure);
}


S32 iap2FTSuccess_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferSuccess);
}

S32 iap2FTCancel_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferCancel);
}


S32 iap2FTPause_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferPause);
}


S32 iap2FTResume_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context)
{
    return handleFTServerMsg(device,iAP2FileXferSession,context,FileTransferResume);
}

} //extern "C"

} } //namespace adit { namespace iap2service {
