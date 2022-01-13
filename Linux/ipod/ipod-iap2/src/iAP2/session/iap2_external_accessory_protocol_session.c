/*****************************************************************************
-  \file : iap2_external_accessory_protocol_session.c
-  \version : $Id: iap2_external_accessory_protocol_session.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for forming the EAP Session messages that has to be sent to the Apple Device
-  \component :
-  \author : Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/
#include "iap2_external_accessory_protocol_session.h"

#include "iap2_dlt_log.h"
#include "iAP2LinkRunLoop.h"
#include "iap2_utility.h"
#include "iap2_session_link_callbacks.h"

EXPORT S32 iAP2AppendEAPSessionIdentifier(iAP2Device_t* device, U16 iAP2ExternalAccessoryProtocolSessionIdentifier, U8 iAP2ExternalAccesoryProtocolIdentifier);
EXPORT S32 iAP2DeleteEAPSessionIdentifier(iAP2Device_t* device, U16 iAP2ExternalAccessoryProtocolSessionIdentifier);
EXPORT S32 iAP2ParseEAMessage(iAP2Device_t* device, U8* buf, U32 bufSize);

/**
 * \addtogroup EAPSessionAPIs
 * @{
 */

/***************************************************************************//**
 * Verifies whether the EAPIdentifier (i.e., iOSIdentifier), sent by Application
 * is valid or not.
 *
 * \param thisDevice  Structure having information about the device connected to
 *                    the target.
 * \param iAP2EAPIdentifier EAPIdentifier of an iOS APP provided by Apple Device.
 *
 * \return IAP2_OK                       When the message has been sent to Link
 *                                       Layer successfully.
 * \return IAP2_INVALID_EAP_IDENTIFIER   When the EAP Identifier provided by the
 *                                       Application is not present.
 * \see
 * \note
 *******************************************************************************/
static S32 iAP2CheckForEAPIdentifier(iAP2Device_st* thisDevice, U8 iAP2EAPIdentifier)
{
    S32 rc = IAP2_INVALID_EAP_IDENTIFIER;
    U16 i;

    for(i = 0; i < thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount; i++)
    {
        if( (iAP2EAPIdentifier == thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppIdentifier) &&
            (thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier != NULL) )
        {
            thisDevice->iAP2MatchingEAPSessionIdentifier = *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier);
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "Matching Identifier Found, iAP2iOSAppInfo[%d].iAP2iOSAppSessionIdentifier:%d DevID:%p", i, *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier), thisDevice);
            rc = IAP2_OK;
            break;
        }
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc = %d, DevID:%p", rc, thisDevice);

    return rc;
}

/***************************************************************************//**
 * Sends the EAP Session Message to the Link Layer.
 *
 * \param device     Structure having information about the device connected to
 *                   the target.
 * \param iAP2iOSAppDataToSend  Buffer having information about the data's that
 *                              has to be sent to iOS App of the Apple Device.
 * \param iAP2iOSAppDataLength  Length of the Buffer having information about the
 *                              data's that has to be sent to iOS App of the Apple
 *                              Device.
 * \param iAP2iOSAppIdentifier  Identifier of the iOS App to which the data has
 *                              to be sent.
 *
 * \return IAP2_OK                            When the message has been sent to
 *                                            Link Layer successfully.
 * \return IAP2_BAD_PARAMETER                 When the Link Run Loop is NULL.
 * \return IAP2_INVALID_EAP_IDENTIFIER        When the EAP Identifier provided by
 *                                            the Application is not present.
 * \return IAP2_ERR_NO_MEM                    While unable to allocate memory for
 *                                            the EASession Datagram.
 * \return IAP2_SIZE_EXCEED_MAX_PAYLOAD_SIZE  if the AppData to send is greater
 *                                            than the payload size.
 *
 * If callback iAP2iOSAppDataReceived_cb is registered, then MC sends the raw EAP data alone, session ID will be added here based on the
 * Protocol ID provided.
 *
 * If callback iAP2iOSMultiAppDataReceived_cb is registered, then MC should send the whole EAP data along with session ID.
 * iAP2iOSAppIdentifier parameter is of no use here since session ID is directly sent by the MC.
 *
 * \see
 * \note
 *******************************************************************************/
S32 iAP2SendEAPSessionMessage(iAP2Device_t* device, const U8* iAP2iOSAppDataToSend, U32 iAP2iOSAppDataLength, U8 iAP2iOSAppIdentifier)
{
    S32 rc = IAP2_BAD_PARAMETER;
    U8* EASessionDatagram  = NULL;
    U32 BufferLengthToSend = 0;
    iAP2Device_st*     thisDevice  = (iAP2Device_st*)device;
    iAP2LinkRunLoop_t* linkRunLoop = (iAP2LinkRunLoop_t*)thisDevice->p_iAP2AccessoryLink;

    if(iAP2iOSAppDataToSend == NULL)
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Input buffer is NULL. DevID:%p", device);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    if(thisDevice->iAP2EAPSessionCallbacks.iAP2iOSAppDataReceived_cb != NULL)
    {
        BufferLengthToSend = iAP2iOSAppDataLength + IAP2_EA_SESSION_IDENTFIER_LENGTH;
        rc = iAP2CheckForEAPIdentifier(thisDevice, iAP2iOSAppIdentifier);
    }
    else if(thisDevice->iAP2MultiEAPSessionCallbacks.iAP2iOSMultiAppDataReceived_cb != NULL)
    {
        BufferLengthToSend = iAP2iOSAppDataLength;
        if(IAP2_EA_SESSION_IDENTFIER_LENGTH > BufferLengthToSend)
        {
            rc = IAP2_ERROR_INVALID_MESSAGE;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iOS App data is invalid. DevID:%p", device);
        }
        else
        {
            rc = IAP2_OK;
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "No callback registered");
    }

    if(linkRunLoop != NULL)
    {
        if(iAP2LinkGetMaxPayloadSize(linkRunLoop->link) < BufferLengthToSend)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iOS App data length Exceeds Maximum Payload Size. DevID:%p", device);
            rc = IAP2_SIZE_EXCEED_MAX_PAYLOAD_SIZE;
        }
    }

    if(rc == IAP2_OK)
    {
        EASessionDatagram = calloc(1, BufferLengthToSend);
        if(EASessionDatagram != NULL)
        {
            if(thisDevice->iAP2EAPSessionCallbacks.iAP2iOSAppDataReceived_cb != NULL)
            {
                U16 iAP2MatchingEAPSessionIdentifier = thisDevice->iAP2MatchingEAPSessionIdentifier;

                /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                iAP2MatchingEAPSessionIdentifier = IAP2_ADHERE_TO_APPLE_ENDIANESS_16(iAP2MatchingEAPSessionIdentifier);    /*lint !e160 !e644 */
                memcpy(EASessionDatagram, &iAP2MatchingEAPSessionIdentifier, sizeof(U16));
                memcpy(&EASessionDatagram[IAP2_EA_SESSION_IDENTFIER_LENGTH], iAP2iOSAppDataToSend, iAP2iOSAppDataLength);
            }
            else if(thisDevice->iAP2MultiEAPSessionCallbacks.iAP2iOSMultiAppDataReceived_cb != NULL)
            {
                memcpy(EASessionDatagram, iAP2iOSAppDataToSend, iAP2iOSAppDataLength);
            }

            rc = thisDevice->iAP2Link->send(thisDevice, EASessionDatagram, BufferLengthToSend, EAP);

            free(EASessionDatagram);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Unable to allocate memory for EASessionDatagram. DevID:%p", device);
        }
    }

    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc:%d DevID:%p", rc, device);

    return rc;
}

/***************************************************************************//**
 * Evaluates whether the External Accessory Protocol Session Identifier sent by
 * Apple Device is correct or not.
 *
 * \param device      Structure having information about the device connected to
 *                    the target.
 * \param p_SourceBuf Buffer holding the message from Apple Device.
 *
 * \return IAP2_OK    On Successful completion of validating the External Accessory
 *                    Protocol Session Identifier sent by Apple Device.
 * \return IAP2_INVALID_INPUT_PARAMETER        When the input pointer is NULL.
 * \return IAP2_INVALID_EAP_SESSION_IDENTIFIER When the Session Identifier Sent
 *                    by Apple Device does not match with the Session Identifier
 *                    held by Accessory.
 * \see
 * \note
 ********************************************************************************/
static S32 iAP2CheckForEASessionIdentifier(iAP2Device_t* device, const U8* p_SourceBuff)
{
    S32 rc = IAP2_INVALID_INPUT_PARAMETER;
    U16 EASessionIdentifier = 0;
    U16 i;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;

    if(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo != NULL)
    {
        rc = IAP2_INVALID_EAP_SESSION_IDENTIFIER;
        EASessionIdentifier = ( ( ((U16)p_SourceBuff[0]) << IPOD_SHIFT_8) | (U16)p_SourceBuff[1]);
        for(i = 0; i < thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount; i++)
        {
            if(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier != NULL)
            {
                if(EASessionIdentifier == *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier) )
                {
                    thisDevice->iAP2CurrentiOSAppIdentifier = thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                    rc = IAP2_OK;
                    break;
                }
            }
        }
        if(rc == IAP2_INVALID_EAP_SESSION_IDENTIFIER)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid EASessionIdentifier Bytes Received, p_SourceBuff[0] = 0x%X, p_SourceBuff[1] = 0x%X DevID:%p", p_SourceBuff[0], p_SourceBuff[1], device);
        }
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc:%d DevID:%p",rc, device);

    return rc;
}

/***************************************************************************//**
 * Evaluates whether the External Accessory Protocol Identifier sent by Apple
 * Device is correct or not, if correct then it stores the corresponding session
 * identifier to the structure.
 *
 * \param device  Structure having information about the device connected to the
 *                target.
 * \param iAP2ExternalAccessoryProtocolSessionIdentifier  EAP Session Identifier
 *                Sent by the Apple Device.
 * \param iAP2ExternalAccesoryProtocolIdentifier    EAP Identifier for which the
 *                Session Identifier is received.
 *
 * \return IAP2_OK  On Successful completion appending the External Accessory
 *                  Protocol Session Identifier sent by Apple Device.
 * \return IAP2_INVALID_INPUT_PARAMETER        When the input pointer is NULL.
 * \return IAP2_INVALID_EAP_SESSION_IDENTIFIER When the Identifier Sent by Apple
 *                  Device does not match with the Identifier held by Accessory.
 * \see
 * \note
 *******************************************************************************/
S32 iAP2AppendEAPSessionIdentifier(iAP2Device_t* device, U16 iAP2ExternalAccessoryProtocolSessionIdentifier, U8 iAP2ExternalAccesoryProtocolIdentifier)
{
    S32 rc = IAP2_INVALID_INPUT_PARAMETER;
    U32 i;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;

    if( (thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo != NULL) && (thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount > 0) )
    {
        rc = IAP2_INVALID_EAP_SESSION_IDENTIFIER;
        for(i = 0; ( (i < thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount) && (rc != IAP2_ERR_NO_MEM) ); i++)
        {
            if(iAP2ExternalAccesoryProtocolIdentifier == thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppIdentifier)
            {
                if(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier == NULL)
                {
                    thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier = calloc(1, sizeof(U16) );
                    if(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier != NULL)
                    {
                        *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier) = iAP2ExternalAccessoryProtocolSessionIdentifier;
                        rc = IAP2_OK;
                        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2iOSAppSessionIdentifier = 0x%.2X  DevID:%p", *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier), device);
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                }
                else
                {
                    /* TBD: Error case to be handled */
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2iOSAppInfo[%d].iAP2iOSAppSessionIdentifier = %p is not NULL DevID:%p", i, thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier, device);
                }
                break;
            }
        }
        if(rc == IAP2_INVALID_EAP_SESSION_IDENTIFIER)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid iAP2ExternalAccesoryProtocolIdentifier:%d DevID:%p", iAP2ExternalAccesoryProtocolIdentifier, device);
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2iOSAppInfo= 0x%p, iAP2SupportediOSAppCount = %d, DevID:%p", thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo, thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount, device);
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc = %d DevID:%p", rc, device);

    return rc;
}

/***************************************************************************//**
 * Evaluates whether the External Accessory Protocol Session Identifier sent by
 * Application is correct or not, if correct then it deletes the corresponding
 * session identifier from the structure.
 *
 * \param device   Structure having information about the device connected to the
 *                 target.
 * \param iAP2ExternalAccessoryProtocolSessionIdentifier  EAP Session Identifier
 *                                                        that has to be deleted.
 *
 * \return IAP2_OK                             On Successful completion appending
 *                                             the External Accessory Protocol Session
 *                                             Identifier sent by Apple Device.
 * \return IAP2_INVALID_INPUT_PARAMETER        When the input pointer is NULL.
 * \return IAP2_INVALID_EAP_SESSION_IDENTIFIER When the SessionIdentifier that has
 *                                             to be deleted was not present with
 *                                             Accessory.
 * \see
 * \note
 *******************************************************************************/
S32 iAP2DeleteEAPSessionIdentifier(iAP2Device_t* device, U16 iAP2ExternalAccessoryProtocolSessionIdentifier)
{
    S32 rc = IAP2_INVALID_INPUT_PARAMETER;
    U32 i;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;

    if( (thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo != NULL) && (thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount > 0) )
    {
        rc = IAP2_INVALID_EAP_SESSION_IDENTIFIER;
        for(i = 0; i < thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount; i++)
        {
            if(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier != NULL)
            {
                if(iAP2ExternalAccessoryProtocolSessionIdentifier == *(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier) )
                {
                    free(thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier);
                    thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppSessionIdentifier = 0;
                    rc = IAP2_OK;
                    break;
                }
            }
        }
        if(rc == IAP2_INVALID_EAP_SESSION_IDENTIFIER)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid iAP2ExternalAccessoryProtocolSessionIdentifier:%d DevID:%p", iAP2ExternalAccessoryProtocolSessionIdentifier, device);
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2iOSAppInfo= 0x%p, iAP2SupportediOSAppCount = %d DevID:%p", thisDevice->iAP2AccessoryInfo.iAP2iOSAppInfo, thisDevice->iAP2AccessoryInfo.iAP2SupportediOSAppCount, device);
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc = %d DevID:%p", rc, device);

    return rc;
}

/***************************************************************************//**
 * Parse the messages from Apple Device & Appropriately responds
 * with iAP2 Messages or Intimates to Mother Company regarding the status.
 *
 * \param device        Structure having information about the device connected
 *                      to the target.
 * \param p_SourceBuf   Source Buffer containing the message from Apple Device which
 *                      has to be parsed.
 * \param SourceBufSize Size of the Source Buffer.
 *
 * \return IAP2_OK                      On Successful completion of parsing the
 *                                      message that has been sent by Apple Device.
 * \return IAP2_INVALID_INPUT_PARAMETER When the input pointer is NULL.
 * \return IAP2_INVALID_START_OF_BYTES  When the Start of Bytes are invalid.
 * \return IAP2_ERROR_INVALID_MESSAGE   When the message length is less than the basic
 *                                      message header length.
 * \return IAP2_ERR_NO_MEM              While unable to allocate memory.
 * \return IAP2_INVALID_PARAMETER_COUNT When the Parameter Count does not meet the
 *                                      criteria as mentioned in the Specification.
 * \return IAP2_UNKNOWN_MSG_ID          When the Message ID does not match with
 *                                      MessageID's mentioned in the Specification.
 *
 * If callback iAP2iOSAppDataReceived_cb is registered, then MC will not get the session ID value from the buffer p_iOSDataRxd.
 * In addition it will get the protocol ID.
 *
 * If callback iAP2iOSMultiAppDataReceived_cb is registered, then EAP message received from the device will be sent without any modification,
 * so buffer p_iOSDataRxd will contain the session ID value.
 * But Protocol ID will not be provided for multiple EA session mode.
 *
 * \see
 * \note
 *******************************************************************************/
S32 iAP2ParseEAMessage(iAP2Device_t* device, U8* p_SourceBuf, U32 SourceBufSize)
{
    S32 rc = IAP2_OK;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;
    U8* p_iOSDataRxd = NULL;

    if( (thisDevice != NULL) && (p_SourceBuf != NULL) )
    {
        if(SourceBufSize < IAP2_EA_SESSION_IDENTFIER_LENGTH)
        {
            rc = IAP2_ERROR_INVALID_MESSAGE;
        }

        if( (rc == IAP2_OK) && (thisDevice->iAP2EAPSessionCallbacks.iAP2iOSAppDataReceived_cb != NULL) )
        {
            rc = iAP2CheckForEASessionIdentifier(thisDevice, p_SourceBuf);
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc = %d DevID:%p", rc, device);

            if(rc == IAP2_OK)
            {
                p_iOSDataRxd = calloc(1, (SourceBufSize - IAP2_EA_SESSION_IDENTFIER_LENGTH) );
                if(p_iOSDataRxd != NULL)
                {
                    memcpy(p_iOSDataRxd, &p_SourceBuf[IAP2_EA_SESSION_IDENTFIER_LENGTH], (SourceBufSize-IAP2_EA_SESSION_IDENTFIER_LENGTH) );
                    thisDevice->iAP2EAPSessionCallbacks.iAP2iOSAppDataReceived_cb(thisDevice,
                                                                              thisDevice->iAP2CurrentiOSAppIdentifier,
                                                                              p_iOSDataRxd,
                                                                              SourceBufSize-IAP2_EA_SESSION_IDENTFIER_LENGTH,
                                                                              thisDevice->iAP2ContextCallback);
                    free(p_iOSDataRxd);
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
        }
        else if( (rc == IAP2_OK) && (thisDevice->iAP2MultiEAPSessionCallbacks.iAP2iOSMultiAppDataReceived_cb != NULL) )
        {
            p_iOSDataRxd = calloc(1,SourceBufSize );
            if(p_iOSDataRxd != NULL)
            {
                memcpy(p_iOSDataRxd, p_SourceBuf, SourceBufSize);
                thisDevice->iAP2MultiEAPSessionCallbacks.iAP2iOSMultiAppDataReceived_cb(thisDevice,
                                                                                      p_iOSDataRxd,
                                                                                      SourceBufSize,
                                                                                      thisDevice->iAP2ContextCallback);
                free(p_iOSDataRxd);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            rc = IAP2_BAD_PARAMETER;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "No callback registered");
        }
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "rc = %d DevID:%p", rc, device);

    return rc;
}
/** @} */
