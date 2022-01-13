/************************************************************************
 * @file: DeviceConfig.cpp
 *
 * @version: 1.0
 *
 * @description: DeviceConfig module provides the wrapper for constructing
 * the iAP2InitParam_t structure for establishing the connection with Apple
 * device. Packing & Unpacking of init paramters are part of the interface
 * library.
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
 * @see iAP2/interface/src/iap2_service_messages.cpp
 *
 * @history
 *
 ***********************************************************************/
#include <adit_logging.h>
#include <iap2_service_init.h>

#include "iap2_commands.h"
#include "iap2_parameter_free.h"
#include "iap2_service_messages.h"
#include "Callbacks.h"

#include "DeviceConfig.h"
#include "FileTransfer.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

std::shared_ptr<iAP2Device_st> DeviceConfig::initializeDevice()
{
    int32_t rc = initCallbacks(&mInitParam);

    iAP2Device_t* iap2device = NULL;
    if(rc == IAP2_OK)
    {
        iap2device = iAP2InitDeviceStructureService(&mInitParam);

        if(iap2device != NULL)
        {
            rc = iAP2InitDeviceConnection(iap2device);
            if(rc != IAP2_OK)
            {
                iAP2DeInitDeviceStructure(iap2device);
                iap2device = NULL;
                LOG_ERROR((iap2, "iAP2InitDeviceConnection failed! rc = %d ", rc));
            }
        }
        else
        {
            LOG_ERROR((iap2, "iAP2InitDeviceStructure iap2device = %p ", iap2device));
        }
    }

    return std::shared_ptr<iAP2Device_st>((iAP2Device_st*)iap2device,
            [=](iAP2Device_st* device)
            {
                DeviceConfig::deinitializeDevice(device);
            });
}

void DeviceConfig::deinitializeDevice(iAP2Device_t* iap2Device)
{
    /* clean-up and release all settings and allocations */
    LOG_INFO((iap2, "DeviceConfig::deinitializeDevice"));
    iAP2DisconnectDevice(iap2Device);
    iAP2DeInitDeviceStructure(iap2Device);
}

int32_t DeviceConfig::initCallbacks(iAP2InitParam_t* iAP2InitParam)
{
    int32_t rc = IAP2_OK ;
    if(NULL != iAP2InitParam)
    {
        mInitParam.p_iAP2CSCallbacks    = (iAP2SessionCallbacks_t*)calloc(1, sizeof(iAP2SessionCallbacks_t));
        mInitParam.p_iAP2StackCallbacks = (iAP2StackCallbacks_t*)calloc(1, sizeof(iAP2StackCallbacks_t));
        mInitParam.p_iAP2FileTransferCallbacks = (iAP2FileTransferCallbacks_t*)calloc(1, sizeof(iAP2FileTransferCallbacks_t));
        mInitParam.p_iAP2EAPSessionCallbacks = (iAP2EAPSessionCallbacks_t*)calloc(1, sizeof(iAP2EAPSessionCallbacks_t));
        mInitParam.p_iAP2MultiEAPSessionCallbacks = (iAP2MultiEAPSessionCallbacks_t*)calloc(1, sizeof(iAP2MultiEAPSessionCallbacks_t));
        mInitParam.p_iAP2EANativeTransportCallbacks = (iAP2EANativeTransportCallbacks_t*)calloc(1, sizeof(iAP2EANativeTransportCallbacks_t));

        if (mInitParam.p_iAP2CSCallbacks
                && mInitParam.p_iAP2StackCallbacks
                && mInitParam.p_iAP2FileTransferCallbacks
                && mInitParam.p_iAP2EAPSessionCallbacks
                && mInitParam.p_iAP2MultiEAPSessionCallbacks
                && mInitParam.p_iAP2EANativeTransportCallbacks)
        {
            initCSCallbacks(iAP2InitParam->p_iAP2CSCallbacks);
            initStackCallbacks(iAP2InitParam->p_iAP2StackCallbacks);
            initEAPSessionCallbacks(iAP2InitParam->p_iAP2EAPSessionCallbacks);
            initMultiEAPSessionCallbacks(iAP2InitParam->p_iAP2MultiEAPSessionCallbacks);
            initFileTransferCallbacks(iAP2InitParam->p_iAP2FileTransferCallbacks);
            initEANativeTransportCallbacks(iAP2InitParam->p_iAP2EANativeTransportCallbacks);
        }
        else
        {
            LOG_ERROR((iap2, "Failed to create memory for Callbacks"));
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER ;
    }

    return rc;
}

void DeviceConfig::initStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
    iap2StackCb->p_iapSend2Application_cb = &iap2Send2Application_CB;
    iap2StackCb->p_iap2SendEAP2Application_cb = &iap2SendEAP2Application_CB;
    iap2StackCb->p_iap2SendFileTransfer2Application_cb = &iap2SendFileTransferApplication_CB;
    iap2StackCb->p_iap2StartEAPsession_cb = &iap2StartEAPsession_CB;
    iap2StackCb->p_iap2StopEAPsession_cb = &iap2StopEAPsession_CB;
}

void DeviceConfig::initCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks)
{
    iap2CSCallbacks->iAP2AuthenticationFailed_cb                    = NULL;
    iap2CSCallbacks->iAP2AuthenticationSucceeded_cb                 = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationCertificate_cb        = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationChallengeResponse_cb  = NULL;
    iap2CSCallbacks->iAP2StartIdentification_cb                     = NULL;
    iap2CSCallbacks->iAP2IdentificationAccepted_cb                  = NULL;
    iap2CSCallbacks->iAP2IdentificationRejected_cb                  = NULL;
    iap2CSCallbacks->iAP2BluetoothConnectionUpdate_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationCertificate_cb         = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationResponse_cb            = NULL;
    iap2CSCallbacks->iAP2DeviceInformationUpdate_cb                 = NULL;
    iap2CSCallbacks->iAP2DeviceLanguageUpdate_cb                    = NULL;
    iap2CSCallbacks->iAP2TelephonyCallStateInformation_cb           = NULL;
    iap2CSCallbacks->iAP2TelephonyUpdate_cb                         = NULL;
    iap2CSCallbacks->iAP2StartVehicleStatusUpdates_cb               = NULL;
    iap2CSCallbacks->iAP2StopVehicleStatusUpdates_cb                = NULL;
    iap2CSCallbacks->iAP2AssistiveTouchInformation_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceHIDReport_cb                         = NULL;
    iap2CSCallbacks->iAP2StartLocationInformation_cb                = NULL;
    iap2CSCallbacks->iAP2StopLocationInformation_cb                 = NULL;
    iap2CSCallbacks->iAP2MediaLibraryInformation_cb                 = NULL;
    iap2CSCallbacks->iAP2MediaLibraryUpdate_cb                      = NULL;
    iap2CSCallbacks->iAP2NowPlayingUpdateParameter_cb               = NULL;
    iap2CSCallbacks->iAP2PowerUpdate_cb                             = NULL;
    iap2CSCallbacks->iAP2USBDeviceModeAudioInformation_cb           = NULL;
    iap2CSCallbacks->iAP2VoiceOverUpdate_cb                         = NULL;
    iap2CSCallbacks->iAP2WiFiInformation_cb                         = NULL;
}

void DeviceConfig::initFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks)
{
    iAP2FileTransferCallbacks->iAP2FileTransferSuccess_cb           = iap2FTSuccess_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferFailure_cb           = iap2FTfailure_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferCancel_cb            = iap2FTCancel_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferPause_cb             = iap2FTPause_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferResume_cb            = iap2FTResume_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferSetup_cb             = iap2FTSetup_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferDataRcvd_cb          = iap2FTDataRcvd_CB;
}

void DeviceConfig::initEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks)
{
    (void)iAP2EAPSessionCallbacks;
}

void DeviceConfig::initMultiEAPSessionCallbacks(iAP2MultiEAPSessionCallbacks_t* iAP2MultiEAPSessionCallbacks)
{
    (void)iAP2MultiEAPSessionCallbacks;
}

void DeviceConfig::initEANativeTransportCallbacks(iAP2EANativeTransportCallbacks_t* iAP2EANativeTransportCallbacks)
{
    iAP2EANativeTransportCallbacks->p_iAP2StartEANativeTransport_cb = &iap2StartEANativeTransport_CB;
    iAP2EANativeTransportCallbacks->p_iAP2StopEANativeTransport_cb = &iap2StopEANativeTransport_CB;
}

int32_t DeviceConfig::setAccessoryConfiguration(struct AccessoryConfiguration* msg)
{
    int32_t rc = IAP2_ERR_NO_MEM;
    mInitParam.p_iAP2AccessoryConfig = (iAP2AccessoryConfig_t*)calloc(1, sizeof(iAP2AccessoryConfig_t));
    if(mInitParam.p_iAP2AccessoryConfig != NULL)
    {
        memcpy(mInitParam.iAP2DeviceId, msg->deviceSerial, sizeof(mInitParam.iAP2DeviceId));
        rc = recvAccessoryConfiguration(msg, mInitParam.p_iAP2AccessoryConfig);
    }
    else
    {
        LOG_ERROR((iap2, "setAccessoryConfiguration memory allocation failed!"));
    }
    return rc;
}

int32_t DeviceConfig::setAccessoryIdentification(struct AccessoryIdentficiation* msg)
{
    int32_t rc = IAP2_ERR_NO_MEM;
    mInitParam.p_iAP2AccessoryInfo = (iAP2AccessoryInfo_t*)calloc(1, sizeof(iAP2AccessoryInfo_t));
    if(mInitParam.p_iAP2AccessoryInfo != NULL)
    {
        rc = recvAccessoryIdentification(msg, mInitParam.p_iAP2AccessoryInfo);
    }
    else
    {
        LOG_ERROR((iap2, "setAccessoryIdentification memory allocation failed!"));
    }
    return rc;
}

int32_t DeviceConfig::setSupportediOSAppInfo(struct AccessorySupportediOSApps* msg)
{
    return recvSupportediOSAppInfo(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setSupportedLanguages(struct AccessorySupportedLanguages* msg)
{
    return recvSupportedLanguages(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setSupportedAudioRates(struct USBDeviceAudioSampleRates* msg)
{
    return recvSupportedAudioRates(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setVehicleInformation(struct VehicleInformation* msg)
{
    return recvVehicleInformation(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setVehicleStatus(struct VehicleStatus* msg)
{
    return recvVehicleStatus(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setLocationInformation(struct LocationInformation *msg)
{
    return recvLocationInformation(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setMessageSentByApplication(struct MessagsSentByApplication* msg)
{
    return recvMessageSentByApplication(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setCallbacksExpectedFromDevice(struct CallbacksExpectedFromDevice* msg)
{
    return recvCallbacksExpectedFromDevice(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setUSBHIDInformation(struct USBDeviceHID* msg)
{
    return recvUSBHIDInformation(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setUSBHostHIDInformation(struct USBHostHID* msg)
{
    return recvUSBHostHIDInformation(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setBluetoothHIDInformation(struct BluetoothHID* msg)
{
    return recvBluetoothHIDInformation(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setBluetoothTransport(struct BluetoothTransport* msg)
{
    return recvBluetoothTransport(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setWirelessCarPlayTransport(struct WirelessCarPlayTransport* msg)
{
    return recvWirelessCarPlayTransport(msg, mInitParam.p_iAP2AccessoryInfo);
}

int32_t DeviceConfig::setRouteGuidanceDisplay(struct RouteGuidanceDisplay* msg)
{
    return recvRouteGuidanceDisplay(msg, mInitParam.p_iAP2AccessoryInfo);
}

} } //namespace adit { namespace iap2service {
