/************************************************************************
 * @file: Callbacks.cpp
 *
 * @version: 1.0
 *
 * @description: This module implements the callback functions for
 * receiving iAP2 messages from iAP2Device.
 *
 * @component: platform/ipod
 *
 * @author: Dhanasekaran Devarasu, Dhanasekaran.D@in.bosch.com 2017
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

#include <adit_logging.h>
#include "Callbacks.h"
#include "Core.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

extern "C" {

S32 iap2Send2Application_CB(iAP2Device_t* iap2Device, uint16_t msgId, uint8_t* data, uint16_t length, void* context)
{
    return Core::instance().sendControlSessionMessageToApplication(iap2Device, msgId, data, length, context);
}

S32 iap2SendEAP2Application_CB(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint16_t length)
{
    return Core::instance().sendEAPSessionMessageToApplication(iap2Device, sessionId, data, length);
}

S32 iap2SendFileTransferApplication_CB(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint32_t length)
{
    return Core::instance().sendFileTransferSessionMessageToApplication(iap2Device, sessionId, data, length);
}

S32 iap2StartEAPsession_CB(iAP2Device_t* iap2Device, uint8_t protocolIdentifier, uint16_t sessionIdentifier)
{
    return Core::instance().setEAPSessionId(iap2Device, protocolIdentifier, sessionIdentifier);
}

S32 iap2StopEAPsession_CB(iAP2Device_t* iap2Device, uint16_t sessionIdentifier)
{
    return Core::instance().resetEAPSessionId(iap2Device, sessionIdentifier);
}

S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    (void)context;
    int32_t rc = IAP2_OK;
    LOGD_DEBUG((iap2, "iap2DeviceState_CB: %d", dState));

    rc = Core::instance().sendDeviceStateMessage(iap2Device, dState);

    if(dState == iAP2DeviceReady)
    {
        rc = Core::instance().sendConnectedDeviceList(iap2Device);
    }

    return rc;
}

static S32 sendEANativeAction(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, enum EANativeAction action)
{
    struct EANativeTransport message;
    std::vector<uint32_t> clientIds;
    std::vector<int32_t> clientFds;
    message.header.deviceId = ((iAP2Device_st*)iap2Device)->iAP2DeviceId;
    message.header.type = MessageType::EANativeTransport;
    message.header.length = sizeof(message);

    message.eapIdentifier = iAP2iOSAppIdentifier;
    message.action = action;
    message.sinkEndPoint = sinkEndpoint;
    message.sourceEndPoint = sourceEndpoint;

    Core::instance().getMessageRouter().getClientIdsForEANativeApp(message.header.deviceId, iAP2iOSAppIdentifier, clientIds);
    for(auto Id : clientIds)
    {
        auto fd = Core::instance().getMessageRouter().getSocketFd(Id);
        clientFds.push_back(fd);
    }
    return Core::instance().getMessageRouter().sendMessage(clientFds, &message, message.header.length);
}

S32 iap2StartEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    (void)context;
    int32_t rc = IAP2_OK;

    rc = sendEANativeAction(iap2Device, iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint, EAN_Start);

    LOGD_DEBUG((iap2, "iap2StartEANativeTransport_CB called"));
    LOGD_DEBUG((iap2, "Identifier:  %d | SinkEndpoint:  %d | SourceEndpoint:  %d",
            iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint));

    return rc;
}

S32 iap2StopEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    (void)context;
    int32_t rc = IAP2_OK;

    rc = sendEANativeAction(iap2Device, iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint, EAN_Stop);

    LOGD_DEBUG((iap2, "iap2StopEANativeTransport_CB called"));
    LOGD_DEBUG((iap2, "Identifier:  %d | SinkEndpoint:  %d | SourceEndpoint:  %d",
            iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint));

    return rc;
}

} //extern "C"

} } //namespace adit { namespace iap2service {
