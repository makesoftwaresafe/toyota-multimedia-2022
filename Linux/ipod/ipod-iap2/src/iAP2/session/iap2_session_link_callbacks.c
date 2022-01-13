/*
 * iap2_session_callbacks.c
 *
 *  Created on: Jul 8, 2013
 *      Author: ajaykumar.s
 */
#include "iap2_init_private.h"
#include "iAP2LinkRunLoop.h"
#include "iap2_session_link_callbacks.h"
#include "iap2_utility.h"

#include "iap2_dlt_log.h"

IMPORT S32 iAP2ParseDeviceMessage(iAP2Device_st* iap2Device, U8* buf, U32 bufSize);
IMPORT S32 iAP2ParseFileTransferMessage(iAP2Device_st* this_iAP2Device, U8* Sourcebuf, U32 SourceBufSize);
IMPORT S32 iAP2ParseEAMessage(iAP2Device_st* iap2Device, U8* buf, U32 bufSize);
IMPORT S32 iAP2ParseiAP2StartExternalAccessoryProtocolSessionParameter(iAP2StartExternalAccessoryProtocolSessionParameter* theiAP2StartExternalAccessoryProtocolSessionParameter, U8* p_SourceBuffer, U16 msgLength, const U8* iAP2BufferPool);
IMPORT S32 iAP2ParseiAP2StopExternalAccessoryProtocolSessionParameter(iAP2StopExternalAccessoryProtocolSessionParameter* theiAP2StopExternalAccessoryProtocolSessionParameter, U8* p_SourceBuffer, U16 msgLength, const U8* iAP2BufferPool);
IMPORT S32 iAP2CheckStartOfMsg(U8* p_SourceBuff);

BOOL iAP2LinkDataReady_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session)
{
    dataLen = dataLen;
    session = session;

    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2Device_st*      iap2Device  = NULL;


    IAP2LINKDLTLOG(DLT_LOG_DEBUG, "iAP2LinkDataReady_CB");

    if(NULL != iap2Link && NULL != data)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)iap2Link->context;
        if (NULL != linkRunLoop)
        {
            iap2Device = (iAP2Device_st*)linkRunLoop->context;

            IAP2LINKDLTLOG(DLT_LOG_DEBUG, "received %d bytes DevID:%p", dataLen, iap2Device);

            if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeControl))
            {
                iAP2ParseDeviceMessage(iap2Device, data , dataLen);
            }
            else if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer))
            {
                iAP2ParseFileTransferMessage(iap2Device, data, dataLen);
            }
            else if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeEA))
            {
                iAP2ParseEAMessage(iap2Device, data , dataLen);
            }
            else
            {
                /* Unknown message */
                IAP2LINKDLTLOG(DLT_LOG_ERROR, "Unknown message DevID:%p", iap2Device);
            }
        }
        else
        {
            IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid linkRunLoop DevID:%p", iap2Device);
        }
    }
    else
    {
        IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid iap2Link DevID:%p", iap2Device);
    }

    return TRUE;
}
/*
 * Callback function to call when link connection is UP/DOWN.
 */
void iAP2LinkConnected_CB (iAP2Link_t* iap2Link, BOOL bConnected)
{
    iAP2Device_st*      iap2Device  = NULL;
    iAP2LinkRunLoop_t*  linkRunLoop = NULL;


    if (NULL != iap2Link)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)iap2Link->context;
        if(NULL != linkRunLoop)
        {
            iap2Device = (iAP2Device_st*)linkRunLoop->context;
        }
        else
        {
            IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid linkRunLoop. DevID:%p",iap2Device);
        }
        if(NULL != iap2Device)
        {
            if(TRUE == bConnected)
            {
                iap2Device->iAP2DeviceState = iAP2LinkConnected;
                (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2LinkConnected, iap2Device->iAP2ContextCallback);

                IAP2LINKDLTLOG(DLT_LOG_DEBUG, "bConnected = TRUE, DevID:%p", iap2Device);
            }
            else
            {
                /* send device state callback only if device state is not iAP2LinkiAP1DeviceDetected */
                if(iap2Device->iAP2DeviceState != iAP2LinkiAP1DeviceDetected)
                {
                    iap2Device->iAP2DeviceState = iAP2NotConnected;
                    (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2NotConnected, iap2Device->iAP2ContextCallback);
                }

                IAP2LINKDLTLOG(DLT_LOG_WARN, "bConnected = FALSE, DevID:%p", iap2Device);
            }
        }
        else
        {
            IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid iap2Device. DevID:%p", iap2Device);
        }
    }
    else
    {
        IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid iap2Link. DevID:%p", iap2Device);
    }
}

BOOL iAP2LinkDataReadyService_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session)
{
    (void)session;

    S32 rc = IAP2_OK;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2Device_st*      iap2Device  = NULL;


    IAP2LINKDLTLOG(DLT_LOG_DEBUG, "iAP2LinkDataReady_CB");

    if(NULL != iap2Link && NULL != data)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)iap2Link->context;
        if (NULL != linkRunLoop)
        {
            iap2Device = (iAP2Device_st*)linkRunLoop->context;

            IAP2LINKDLTLOG(DLT_LOG_DEBUG, "received %d bytes DevID:%p", dataLen, iap2Device);

            if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeControl))
            {
                U16 msgID = 0x0000;
                U16 msgLength = 0x0000;
                if(dataLen < IAP2_MSG_HEADER_SIZE)
                {
                    rc = IAP2_ERROR_INVALID_MESSAGE;
                }
                if (rc == IAP2_OK)
                {
                    rc = iAP2CheckStartOfMsg(data);
                }
                if (rc == IAP2_OK)
                {
                    msgLength = iAP2GetIDorLength(&data[IAP2_MSG_LENGTH_OFFSET]);
                    msgID     = iAP2GetIDorLength(&data[IAP2_MSG_LENGTH_OFFSET + IAP2_ID_OFFSET]);

                    /* StartExternalAccessoryProtocol & StopExternalAccessoryProtocol messages are parsed and
                     * ProtocolIdentifier and Session Identifiers are retrieved and send to CSM/Dispatcher.
                     * Using the identifiers EAP messages are sent only to the intended Application by CSM.
                     * These Control session messages are also routed to Applications for handling.
                     * */
                    if(msgID == IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION)
                    {
                        iAP2StartExternalAccessoryProtocolSessionParameter theiAP2StartExternalAccessoryProtocolSessionParameter;
                        memset(&theiAP2StartExternalAccessoryProtocolSessionParameter, 0, sizeof(iAP2StartExternalAccessoryProtocolSessionParameter) );
                        iAP2InitializeBufferPool(iap2Device->iAP2BufferPool);
                        rc = iAP2ParseiAP2StartExternalAccessoryProtocolSessionParameter(&theiAP2StartExternalAccessoryProtocolSessionParameter, data, msgLength, iap2Device->iAP2BufferPool);
                        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "Received iAP2StartExternalAccessoryProtocolSession Message from Device, Msg ID:%X, Msg Length:0x%X DevID:%p", msgID, msgLength, iap2Device);

                        /*call StackCallback to notify CSM*/
                        if((rc == IAP2_OK) && (iap2Device->iAP2StackCallbacks.p_iap2StartEAPsession_cb != NULL))
                            rc = iap2Device->iAP2StackCallbacks.p_iap2StartEAPsession_cb(iap2Device, *theiAP2StartExternalAccessoryProtocolSessionParameter.iAP2ExternalAccesoryProtocolIdentifier, *theiAP2StartExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionIdentifier);
                        else
                            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Failed to send! rc:%d callback:%p DevID:%p", rc, iap2Device->iAP2StackCallbacks.p_iap2StartEAPsession_cb,iap2Device);
                    }
                    if(msgID == IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION)
                    {
                        iAP2StopExternalAccessoryProtocolSessionParameter theiAP2StopExternalAccessoryProtocolSessionParameter;

                        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "Received iAP2StopExternalAccessoryProtocolSession Message from Device, Msg ID:%X, Msg Length:0x%X DevID:%p", msgID, msgLength, iap2Device);
                        memset(&theiAP2StopExternalAccessoryProtocolSessionParameter, 0, sizeof(iAP2StopExternalAccessoryProtocolSessionParameter) );
                        iAP2InitializeBufferPool(iap2Device->iAP2BufferPool);
                        rc = iAP2ParseiAP2StopExternalAccessoryProtocolSessionParameter(&theiAP2StopExternalAccessoryProtocolSessionParameter, data, msgLength, iap2Device->iAP2BufferPool);

                        /*call StackCallback to notify CSM*/
                        if((rc == IAP2_OK) && (iap2Device->iAP2StackCallbacks.p_iap2StopEAPsession_cb != NULL))
                            rc = iap2Device->iAP2StackCallbacks.p_iap2StopEAPsession_cb(iap2Device, *theiAP2StopExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionIdentifier);
                        else
                            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Failed to send! rc:%d callback:%p DevID:%p", rc, iap2Device->iAP2StackCallbacks.p_iap2StopEAPsession_cb, iap2Device);
                    }
#define MSG_ID_UPPERBYTE_MASK            (0xFF00)
#define IAP2_MSG_ID_AUTHENTICATION_MASK  (0xAA00)
#define IAP2_MSG_ID_IDENTIFICATION_MASK  (0x1D00)
                    if(((MSG_ID_UPPERBYTE_MASK & msgID) == IAP2_MSG_ID_AUTHENTICATION_MASK) || ((MSG_ID_UPPERBYTE_MASK & msgID) == IAP2_MSG_ID_IDENTIFICATION_MASK))
                    {
                        IAP2SESSIONDLTLOG(DLT_LOG_INFO, "calling ParseDeviceMessage for message(%u)", msgID);
                        /*Process message in iAP2Service itself*/
                        rc = iAP2ParseDeviceMessage(iap2Device, data , dataLen);
                    }
                    else
                    {
                        /*Application(s) will process the message*/
                        if (iap2Device->iAP2StackCallbacks.p_iapSend2Application_cb != NULL)
                        {
                            rc = iap2Device->iAP2StackCallbacks.p_iapSend2Application_cb(iap2Device, msgID, data, dataLen, NULL);
                        }
                        else
                        {
                            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iapSend2Application_cb NOT registered for DevID:%p", iap2Device);
                        }
                    }
                }
            }
            else if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer))
            {
                //if(iap2Device->iAP2StackCallbacks.p_iap2SendFileTransfer2Application_cb != NULL)
                //    rc = iap2Device->iAP2StackCallbacks.p_iap2SendFileTransfer2Application_cb(iap2Device, 0/*TODO*/, data, dataLen);
            	iAP2ParseFileTransferMessage(iap2Device, data, dataLen);
            }
            else if(session == iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeEA))
            {
                uint16_t sessionIdentifier = ( ( ((U16)data[0]) << IPOD_SHIFT_8) | (U16)data[1]);
                if(iap2Device->iAP2StackCallbacks.p_iap2SendEAP2Application_cb != NULL)
                    rc = iap2Device->iAP2StackCallbacks.p_iap2SendEAP2Application_cb(iap2Device, sessionIdentifier, data, dataLen);
            }
            else
            {
                /* Unknown message */
                IAP2LINKDLTLOG(DLT_LOG_ERROR, "Unknown message DevID:%p", iap2Device);
            }
        }
        else
        {
            IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid linkRunLoop DevID:%p", iap2Device);
        }
    }
    else
    {
        IAP2LINKDLTLOG(DLT_LOG_ERROR, "Invalid iap2Link DevID:%p", iap2Device);
    }

    return TRUE;
}


/*!Callback function to call when data is received on Application(iAP2Library) from iAP2Service*/
BOOL iAP2ServiceLinkDataReady_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session)
{
    /*WARNING: Only context to be used in iap2Link structure. This is used for interface model*/
    iAP2Device_st* device = iap2Link->context;
    S32 rc = IAP2_OK;
    switch(session)
    {
    case kIAP2PacketServiceTypeControl:
    {
        rc = iAP2ParseDeviceMessage(device, data, dataLen);
        break;
    }
    case kIAP2PacketServiceTypeEA:
    {
        rc = iAP2ParseEAMessage(device, data, dataLen);
        break;
    }
    case kIAP2PacketServiceTypeBuffer:
    {
        rc = iAP2ServiceParseFileTransferMessage(device, data, dataLen);
        break;
    }
    default: /* Unknown message */
        IAP2LINKDLTLOG(DLT_LOG_ERROR, "Unknown message DevID:%p", device);
        break;
    }
    (void)rc;
    return TRUE;
}
