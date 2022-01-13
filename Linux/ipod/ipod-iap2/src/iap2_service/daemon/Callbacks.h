/************************************************************************
 * @file: Callbacks.h
 *
 * @version: 1.0
 *
 * @description: This module declares the callback functions for
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


#ifndef IAP2_SERVICE_CALLBACKS_H_
#define IAP2_SERVICE_CALLBACKS_H_

#include <iap2_service_init.h>

namespace adit { namespace iap2service {

extern "C"
{
S32 iap2MessageFilter_CB(iAP2Device_t* iap2Device, uint16_t msgId, void* context);
S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context);

S32 iap2StartEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context);
S32 iap2StopEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context);

S32 iap2Send2Application_CB(iAP2Device_t* iap2Device, uint16_t msgId, uint8_t* data, uint16_t length, void* context);
S32 iap2SendEAP2Application_CB(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint16_t length);
S32 iap2SendFileTransferApplication_CB(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint32_t length);

S32 iap2StartEAPsession_CB(iAP2Device_t* iap2Device, uint8_t protocolIdentifier, uint16_t sessionIdentifier);
S32 iap2StopEAPsession_CB(iAP2Device_t* iap2Device, uint16_t sessionIdentifier);

} //extern "C"

} } //namespace adit { namespace iap2service {

#endif /* IAP2_SERVICE_CALLBACKS_H_ */
