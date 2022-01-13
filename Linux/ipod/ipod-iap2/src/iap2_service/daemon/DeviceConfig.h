/************************************************************************
 * @file: DeviceConfig.h
 *
 * @version: 1.0
 *
 * @description: IAP2DeviceConfig module provides the wrapper for constructing
 * the iAP2InitParam_t structure for establishing the connection with Apple
 * device. Packing & Unpacking of init parameters are part of the interface
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
 * @see iAP2/interface/include/iap2_service_messages.h
 *
 * @history
 *
 ***********************************************************************/
#ifndef DEVICE_CONFIG_H_
#define DEVICE_CONFIG_H_

#include <iap2_service_init.h>
#include <iap2_service_messages.h>
#include <iap2_init_private.h>
#include <Events.h>

#include <memory>
#include <map>
#include <list>
#include <mutex>
#include <atomic>
#include <vector>

namespace adit { namespace iap2service {

class DeviceConfig
{
public:
    DeviceConfig() : mLanguages(nullptr), mAppInfo(nullptr), mAudioRates(nullptr), mVehicle(nullptr)
    {
        memset(&mInitParam, 0, sizeof(iAP2InitParam_t));
        memset(&mAccConfig, 0, sizeof(mAccConfig));
        memset(&mAccInfo, 0, sizeof(mAccInfo));
    }
    virtual ~DeviceConfig() {}

public:
    static void deinitializeDevice(iAP2Device_t* iap2Device);
    std::shared_ptr<iAP2Device_st> initializeDevice();

    int32_t setAccessoryConfiguration(struct AccessoryConfiguration* accConfig);
    int32_t setAccessoryIdentification(struct AccessoryIdentficiation* accInfo);
    int32_t setSupportediOSAppInfo(struct AccessorySupportediOSApps* appInfo);
    int32_t setSupportedLanguages(struct AccessorySupportedLanguages* languages);
    int32_t setSupportedAudioRates(struct USBDeviceAudioSampleRates* audioRates);
    int32_t setVehicleInformation(struct VehicleInformation* vehicle);
    int32_t setVehicleStatus(struct VehicleStatus* vehicle);
    int32_t setLocationInformation(struct LocationInformation* locationData);
    int32_t setMessageSentByApplication(struct MessagsSentByApplication* messages);
    int32_t setCallbacksExpectedFromDevice(struct CallbacksExpectedFromDevice* messages);

    int32_t setUSBHIDInformation(struct USBDeviceHID* message);
    int32_t setUSBHostHIDInformation(struct USBHostHID* message);
    int32_t setBluetoothHIDInformation(struct BluetoothHID* message);
    int32_t setBluetoothTransport(struct BluetoothTransport* message);
    int32_t setWirelessCarPlayTransport(struct WirelessCarPlayTransport* message);
    int32_t setRouteGuidanceDisplay(struct RouteGuidanceDisplay* message);
protected: //deleted
     DeviceConfig(const DeviceConfig&) = delete;
     DeviceConfig operator=(const DeviceConfig&) = delete;

protected:
    virtual void initStackCallbacks(iAP2StackCallbacks_t* iap2StackCb);
    virtual void initFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks);
    virtual void initEANativeTransportCallbacks(iAP2EANativeTransportCallbacks_t* iAP2EANativeTransportCallbacks);
    virtual void initEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks);
    virtual void initMultiEAPSessionCallbacks(iAP2MultiEAPSessionCallbacks_t* iAP2MultiEAPSessionCallbacks);
    virtual void initCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks);

private:
    int32_t initCallbacks(iAP2InitParam_t* iAP2InitParam);

private:
    struct AccessoryConfiguration mAccConfig;
    struct AccessoryIdentficiation mAccInfo;
    struct AccessorySupportedLanguages* mLanguages;
    struct AccessorySupportediOSApps* mAppInfo;
    struct USBDeviceAudioSampleRates* mAudioRates;
    struct VehicleInformation* mVehicle;

    iAP2InitParam_t mInitParam;
};

typedef std::map<uint16_t, bool> MsgTable;
struct DeviceInfo
{
    DeviceInfo() {lock = 0;}

    std::shared_ptr<iAP2Device_st> device;
    std::atomic<uint32_t> lock; //acquire to executing work in iAP2 stack
    char name[256];
    uint32_t id;

    std::vector<int32_t> clients; //TODO: connected clients FDs. Remove when File Transfer routing implemented!
    std::mutex clientsMutex;
    std::list<std::shared_ptr<Event>> events;

    std::map<uint16_t, bool> identifiedMessageMap; //iAP2 Messages sent during identification of device

    std::map<uint16_t, std::vector<int32_t>> mMessageMap; //<Message Id, ClientFds>
    std::mutex mMessageMapMutex;

    std::map<uint8_t, std::vector<uint32_t>> mEapIdentifier; //<iOSAppIdentifier, ClientIds> - possible options(static)
    std::map<int16_t, std::vector<int32_t>> mEapSessionIdentifiers; //<eapSessionId, ClientFds>
    std::mutex mEapMutex;


    bool operator==(const DeviceInfo& devInfo)
    {
        return (devInfo.id == id) ? true : false;
    }
    bool operator!=(const DeviceInfo& devInfo)
    {
        return !operator==(devInfo);
    }
};

} } //namespace adit { namespace iap2service {

#endif /* DEVICE_CONFIG_H_ */
