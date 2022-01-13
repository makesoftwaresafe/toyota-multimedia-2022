/************************************************************************
 * @file: FileTransfer.h
 *
 * @version: 1.0
 *
 * @description: This module declares callback functions for
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
#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <iap2_service_init.h>
#include <iap2_init_private.h>
#include <iap2_service_messages.h>

extern "C"
{
S32 iAP2FileXferStartOk(iAP2Device_st* this_iAP2Device,U8 ftid,BOOL startOK );
S32 handleFTClientMsg(iAP2Device_st* this_iAP2Device,iAP2FTMessage_t *iap2msg);
S32 iap2FTSetup_CB(iAP2Device_t *this_iAP2Device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTDataRcvd_CB(iAP2Device_t *iap2Device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTfailure_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTSuccess_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTResume_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTCancel_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);
S32 iap2FTPause_CB(iAP2Device_t *device,iAP2FileTransferSession_t *iAP2FileXferSession,void* context);

}

#endif /* FILETRANSFER_H_ */
