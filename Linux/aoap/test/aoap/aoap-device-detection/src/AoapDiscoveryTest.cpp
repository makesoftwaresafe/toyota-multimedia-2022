/**
* \file: AoapDiscoveryTest.cpp
*
* \version: 0.1
*
* \release: $Name:$
*
* Implementation to test aoap-disovery.
*
* \component: AOAP
*
* \author: D. Girnus / ADIT/ESM / dgirnus@de.adit-jv.com
*
* \copyright (c) 2017 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* \see <related items>
*
* \history
*
***********************************************************************/


#include "AoapDiscoveryTest.h"
#include "Timer.h"

#include <adit_logging.h>
#include <adit_dlt.h>

#include <memory>

#include <condition_variable>
#include <mutex>


#ifndef DLT_TEST_AOAP_TADD
#define DLT_TEST_AOAP_TADD "TADD"
#endif

LOG_DECLARE_CONTEXT(aoap_dev_test);

std::condition_variable gConditonVar;
std::mutex gMutex;
int gNotifications = 0;


void Tdd::notifyDeviceFound_cb(void *pContext, t_usbDeviceInformation* pDevice, int result) {
    auto me = static_cast<Tdd*>(pContext);

    if (0 > result) {
        LOG_INFO((aoap_dev_test, "%s()  device attached. result=%d",
                __FUNCTION__, result));
        return;
    }

    LOG_INFO((aoap_dev_test, "%s()  device found vendorId=%X, productId=%X, serial=%s, AOAP supported=%s",
            __FUNCTION__, pDevice->vendorId, pDevice->productId, pDevice->serial.c_str(),
            pDevice->aoapSupported ? "true" : "false"));

    if (me->mRunning && pDevice->aoapSupported) {
        me->onDeviceFound(*pDevice);
    }
}

void Tdd::notifyDeviceRemoved_cb(void *pContext, t_usbDeviceInformation* pDevice, int result) {
    auto me = static_cast<Tdd*>(pContext);

    if (0 > result) {
        LOG_INFO((aoap_dev_test, "%s()  device detached. result=%d", __FUNCTION__, result));
        return;
    }

    LOG_INFO((aoap_dev_test, "%s()  device removed (serial=%s)", __FUNCTION__, pDevice->serial.c_str()));

    if (me->mRunning) {
        me->onDeviceLost(*pDevice);
    }
}

void Tdd::notifyDeviceFoundAccessoryMode_cb(void *pContext, t_usbDeviceInformation* pDevice, int result) {
    auto me = static_cast<Tdd*>(pContext);

    if (0 > result) {
        LOG_INFO((aoap_dev_test, "%s()  device attached accessory mode. result=%d", __FUNCTION__, result));
        return;
    }

    if ((pDevice->productId >= 0x2d00) && (pDevice->productId <= 0x2d05)) {
        LOG_INFO((aoap_dev_test, "%s()  device in accessory mode connected %s",
                __FUNCTION__, pDevice->serial.c_str()));

        if (me->mRunning) {
            me->onDeviceSwitched(*pDevice);
        }
    } else {
        /* ERROR:
         * Shall never the case that spi_usb_discoverer
         * informs about a device in AOAP mode
         * and the productId is not within the range of 0x2d00 - 0x2d05 */
        LOG_ERROR((aoap_dev_test, "%s()  AOAP device with invalid productId=0x%X found",
                __FUNCTION__, pDevice->productId));
        // TODO error handling required?
    }
}

void Tdd::notifyDeviceCheckAoapSupport_cb(void *pContext, t_usbDeviceInformation* pDevice, bool* outAllowCheck)
{
    auto me = static_cast<Tdd*>(pContext);
    uint32_t blackListedVendorId = 0x0781; // SanDisk Corp.

    LOG_INFO((aoap_dev_test, "%s()  device attached. Consider to allow AOAP support check", __FUNCTION__));


    if (me->mRunning) {
        if (blackListedVendorId == pDevice->vendorId) {
            LOG_INFO((aoap_dev_test, "%s()  device is black listed. Deny AOAP support check", __FUNCTION__));
            *outAllowCheck = false;
        } else {
            LOG_INFO((aoap_dev_test, "%s()  device is not black listed. Allow AOAP support check", __FUNCTION__));
            *outAllowCheck = true;
        }
    }
}

void Tdd::onDeviceFound(t_usbDeviceInformation& inDevice) {
    /* any AOAP supporting device */
    LOG_INFO((aoap_dev_test, "%s()  AOAP device found: aoap supported=%d product=%s serial=%s",
            __FUNCTION__, inDevice.aoapSupported, inDevice.product.c_str(), inDevice.serial.c_str()));

    if (dqTddDevices.size() >= mMaxDevices) {
        LOG_INFO((aoap_dev_test, "%s()  Reached maximum number of devices (%zu) to test (in test: %zu)", __FUNCTION__, mMaxDevices, dqTddDevices.size()));
        return;
    }

    if (true == inDevice.aoapSupported) {
        /* does the MD reappear and we are not aware of the disappear? */
        TddDevice* foundTddDevice = findDevice(inDevice);
        if (foundTddDevice) {
            /* the result of comparing serial number is,
             * that the MD is still/already in our internal device list */
            LOG_WARN((aoap_dev_test, "%s()  device product=%s serial=%s still/already in internal list. last state was = %d",
                        __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str(), foundTddDevice->getDeviceState()));

            rmDevice(foundTddDevice);
        }

        /* switch device and retrieve info */
        TddDevice* tddDevice = new TddDevice(inDevice);
        if (tddDevice) {
            /* add device in list */
            addDevice(tddDevice);
            /* set current state */
            tddDevice->setDeviceState(DEVICE_STATE_IDLE);

            setTestState(TddTestState::DEV_FOUND);

            /* switch device */
            switchDevice(tddDevice);
        } else {
            LOG_ERROR((aoap_dev_test, "%s()  create TddDevice object failed", __FUNCTION__));
        }
    } else {
        LOG_INFO((aoap_dev_test, "%s()  Connected mobile device does not support AOAP", __FUNCTION__));
    }
}

void Tdd::onDeviceLost(t_usbDeviceInformation& inDevice) {
    LOG_INFO((aoap_dev_test, "%s()  AOAP device lost: product=%s serial=%s",
            __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));

    /* check if we are aware of the device */
    TddDevice* foundTddDevice = findDevice(inDevice);
    if (foundTddDevice) {
        LOG_INFO((aoap_dev_test, "%s()  found device product=%s serial=%s in internal list.",
                __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
        /* check if the device was switched. in that case -> do not remove from internal list */
        if (DEVICE_STATE_SWITCHING == foundTddDevice->getDeviceState()) {
            LOG_INFO((aoap_dev_test, "%s()  device product=%s serial=%s is switching.",
                    __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));

            setTestState(TddTestState::DEV_LOST);
        } else {
            LOG_INFO((aoap_dev_test, "%s()  found device product=%s serial=%s. remove from in internal list",
                    __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
            rmDevice(foundTddDevice);
        }
    } else {
        LOG_WARN((aoap_dev_test, "%s()  could not find device product=%s serial=%s in internal list",
                __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
    }

}

void Tdd::onDeviceSwitched(t_usbDeviceInformation& inDevice) {
    LOG_INFO((aoap_dev_test, "%s()  AOAP device switched: product=%s serial=%s",
            __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
    /* check if we are aware of the device */
    TddDevice* foundTddDevice = findDevice(inDevice);
    if (foundTddDevice) {
        LOG_INFO((aoap_dev_test, "%s()  found device product=%s serial=%s in internal list.",
                __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
        /* check if the device was switched. in that case -> do not remove from internal list */
        if (DEVICE_STATE_SWITCHING == foundTddDevice->getDeviceState()) {
            LOG_INFO((aoap_dev_test, "%s()  device product=%s serial=%s was switched into AOAP mode",
                    __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));

            foundTddDevice->mDevInfo.vendorId = inDevice.vendorId;
            foundTddDevice->mDevInfo.productId = inDevice.productId;
            foundTddDevice->mDevInfo.devNum = inDevice.devNum;

            foundTddDevice->setDeviceState(DEVICE_STATE_ACCESSORY);

            bool notify = false;
            unsigned int i = 0;
            std::deque<TddDevice*>::iterator itrTddDevice;
            /* go through all queued devices to find the request one */
            for (i=0; i<dqTddDevices.size(); i++)
            {
                itrTddDevice = dqTddDevices.begin()+i;

                if ( DEVICE_STATE_ACCESSORY == itrTddDevice.operator *()->getDeviceState() )
                {
                    LOG_INFO((aoap_dev_test, "%s()  device with serial=%s in accessory mode",
                            __func__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));
                    notify = true;
                }
                else
                {
                    LOG_INFO((aoap_dev_test, "%s()  device with serial=%s not yet in accessory mode",
                            __func__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));
                    notify = false;
                    break;
                }
            }

            setTestState(TddTestState::DEV_ACC_MODE);
            if (true == notify) {
                std::unique_lock<std::mutex> guard(gMutex);
                gNotifications++;
                gConditonVar.notify_one();
            }
        } else {
            LOG_INFO((aoap_dev_test, "%s()  found device product=%s serial=%s. remove from in internal list",
                    __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
            rmDevice(foundTddDevice);
        }
    } else {
        LOG_INFO((aoap_dev_test, "%s()  device product=%s serial=%s in AOAP mode without previous switch",
                __FUNCTION__, inDevice.product.c_str(), inDevice.serial.c_str()));
        /* device is unknown. create object an set state */
        TddDevice* tddDevice = new TddDevice(inDevice);
        if (tddDevice) {
            /* add device in list */
            addDevice(tddDevice);
            /* set current state */
            tddDevice->setDeviceState(DEVICE_STATE_UNKNOWN);
            /* try to reset device into normal mode */
            resetDevice(tddDevice);
        } else {
            LOG_ERROR((aoap_dev_test, "%s()  create TddDevice object failed", __FUNCTION__));
        }
    }
}

bool Tdd::start() {

    // register spi_usb_discoverer callbacks
    SpiUsbDiscovererCallbacks callbacks;
    callbacks.notifyDeviceFound = &notifyDeviceFound_cb;
    callbacks.notifyDeviceRemoved = &notifyDeviceRemoved_cb;
    callbacks.notifyDeviceFoundAccessoryMode = &notifyDeviceFoundAccessoryMode_cb;
    callbacks.notifyGrantAoapSupportCheck = &notifyDeviceCheckAoapSupport_cb;

    mDiscoverer = new AOAP::UsbDiscoverer(this, &callbacks);
    /* Start UsbDiscoverer
     * Start monitoring of mobile device which supporting AOA protocl (AOAP) */
    if (0 != mDiscoverer->startMonitoring()) {
        LOG_ERROR((aoap_dev_test, "%s()  failed to start UsbDiscover", __FUNCTION__));
        delete mDiscoverer;
        mDiscoverer = nullptr;
        return false;
    } else {
        mRunning = true;
        setTestState(TddTestState::IDLE);

        LOG_INFO((aoap_dev_test, "%s()  Use udevadm to trigger add event(s)", __FUNCTION__));
        int32_t err = system("udevadm trigger --type=devices --subsystem-match=usb --action=add");
        if (0 != err)
        {
            LOG_ERROR((aoap_dev_test, "%s()  Failed to trigger add event(s). system() returned with err = %d", __func__, err));
        }
    }

    return true;
}

void Tdd::stop() {
    mRunning = false;

    while (true != dqTddDevices.empty()) {
        std::deque<TddDevice*>::iterator itrTddDevice = dqTddDevices.begin();

        if (DEVICE_STATE_SWITCH_FAIL == itrTddDevice.operator *()->getDeviceState()) {
            LOG_WARN((aoap_dev_test, "%s()  Connect Callback notified about timeout during switch to AOAP.", __func__));
            setTestError(TddTestError::ERR_STATE);
        }

        LOG_INFO((aoap_dev_test, "%s()  reset device with serial=%s",
                __func__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));
        resetDevice(itrTddDevice.operator *());

        if (0 != itrTddDevice.operator *()->disconnectDevice()) {
            LOG_ERROR((aoap_dev_test, "%s()  disconnectDevice() failed", __FUNCTION__));
        }

        LOG_INFO((aoap_dev_test, "%s()  erase device with serial=%s",
                __func__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));

        dqTddDevices.erase(itrTddDevice);
        delete *itrTddDevice;
        *itrTddDevice = nullptr;
    }

    if (mDiscoverer != nullptr) {
        if (0 != mDiscoverer->stopMonitoring()) {
            LOG_ERROR((aoap_dev_test, "%s()  failed to properly stop UsbDiscoverer", __FUNCTION__));
            // just continue though
        }
    }

    if (mDiscoverer != nullptr) {
        LOG_INFO((aoap_dev_test, "%s()  destroy UsbDiscover TID=%u", __FUNCTION__, (unsigned int)pthread_self()));
        delete mDiscoverer;
        mDiscoverer = nullptr;
    }
}

void Tdd::rmDevice(TddDevice* inTddDevice) {
    uint32_t i = 0;
    if ( (inTddDevice) && (true != dqTddDevices.empty()) ) {
        std::deque<TddDevice*>::iterator itrTddDevice;
        for (i=0; i<dqTddDevices.size(); i++)
        {
            itrTddDevice = dqTddDevices.begin()+i;
            LOG_INFO((aoap_dev_test, "%s()  element=%d / %zu, check serial=%s",
                    __FUNCTION__, i, dqTddDevices.size(), itrTddDevice.operator *()->mDevInfo.serial.c_str()));

            if (0 == inTddDevice->mDevInfo.serial.compare((itrTddDevice.operator *()->mDevInfo.serial.c_str()))) {
                if (0 != itrTddDevice.operator *()->disconnectDevice()) {
                    LOG_ERROR((aoap_dev_test, "%s()  disconnectDevice() failed", __FUNCTION__));
                }
                LOG_INFO((aoap_dev_test, "%s()  erase device with serial=%s",
                        __FUNCTION__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));

                dqTddDevices.erase(itrTddDevice);
                delete inTddDevice;
                inTddDevice = nullptr;
            }
        }
    }

    LOG_INFO((aoap_dev_test, "%s()  num of available devices = %zu",
                        __FUNCTION__, dqTddDevices.size()));
}

void Tdd::switchDevice(TddDevice* inTddDevice) {

    if (0 != inTddDevice->switchDevice()) {
        LOG_ERROR((aoap_dev_test, "%s()  switchDevice() 0x%X:0x%X failed",
                __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId));
        inTddDevice->setDeviceState(DEVICE_STATE_UNKNOWN);
    } else {
        LOG_INFO((aoap_dev_test, "%s()  switchDevice() 0x%X:0x%X  in progress",
                __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId));
        inTddDevice->setDeviceState(DEVICE_STATE_SWITCHING);

        setTestState(TddTestState::DEV_SWITCHING);
    }
}

void Tdd::resetDevice(TddDevice* inTddDevice) {
    int ret = 0;
    unsigned int retries = 10;
    if (mDiscoverer != nullptr) {
        do {
            ret = mDiscoverer->resetDevice(&inTddDevice->mDevInfo);
            if (0 > ret) {
                if (SPI_USB_DISCOVERY_LIBUSB_RESET_FAILED == ret) {
                    LOG_WARN((aoap_dev_test, "%s()  reset device 0x%X:0x%X:%s failed. Retry.",
                            __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId, inTddDevice->mDevInfo.serial.c_str()));
                    sleep(1);
                    retries--;
                } else if (SPI_USB_DISCOVERY_NO_DEVICE_FOUND == ret) {
                    LOG_INFO((aoap_dev_test, "%s()  device 0x%X:0x%X:%s to reset could not be found - assume it resets",
                            __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId, inTddDevice->mDevInfo.serial.c_str()));
                } else {
                    LOG_ERROR((aoap_dev_test, "%s()  reset device 0x%X:0x%X:%s failed",
                            __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId, inTddDevice->mDevInfo.serial.c_str()));
                    break;
                }
            } else {
                LOG_INFO((aoap_dev_test, "%s()  reset device 0x%X:0x%X:%s done",
                        __FUNCTION__, inTddDevice->mDevInfo.vendorId, inTddDevice->mDevInfo.productId, inTddDevice->mDevInfo.serial.c_str()));

                setTestState(TddTestState::DEV_RESET);
            }
        } while ((SPI_USB_DISCOVERY_LIBUSB_RESET_FAILED == ret) && (retries > 0));
    }
}

TddDevice* Tdd::findDevice(t_usbDeviceInformation& inDevice) {
    uint32_t i = 0;
    /* check if we queued any device */
    if (true != dqTddDevices.empty()) {
        std::deque<TddDevice*>::iterator itrTddDevice;
        /* go through all queued devices to find the request one */
        for (i=0; i<dqTddDevices.size(); i++)
        {
            itrTddDevice = dqTddDevices.begin()+i;
            LOG_INFO((aoap_dev_test, "%s()  check serial=%s",
                    __FUNCTION__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));

            if (0 == inDevice.serial.compare((itrTddDevice.operator *()->mDevInfo.serial.c_str()))) {
                LOG_INFO((aoap_dev_test, "%s()  found device with serial=%s",
                        __FUNCTION__, itrTddDevice.operator *()->mDevInfo.serial.c_str()));
                /* requested device found. return TddDevice */
                return itrTddDevice.operator *();
            }
        }
    } else {
        LOG_INFO((aoap_dev_test, "%s()  No device in list available", __FUNCTION__));
    }

    return nullptr;
}


TddDevice::TddDevice(const t_usbDeviceInformation& inDevInfo) {
    mDevInfo = inDevInfo;
    mCurrState = DEVICE_STATE_IDLE;
    mAccId = -1;
    mDevId = -1;
}

TddDevice::~TddDevice() {
}

struct myAoap
{
    const char* manufacturer;
    const char* modelName;
    const char* description;
    const char* version;
    const char* uri;
    const char* serial;
    uint32_t enableAudio;
};

int32_t TddDevice::switchDevice() {

    int32_t ret = 0;

    struct myAoap accInfo;
    accInfo.manufacturer = ACCESSORY_MANUFACTURER_NAME;
    accInfo.modelName = ACCESSORY_MODEL_NAME;
    accInfo.description = ACCESSORY_DESCRIPTION;
    accInfo.version = ACCESSORY_VERSION;
    accInfo.uri = ACCESSORY_URI;
    accInfo.serial = ACCESSORY_SERIAL_NUMBER;
    accInfo.enableAudio = 0;

    mAccId = aoap_create_accessory((t_aoap_accessory_param*)&accInfo);
    if (mAccId < 0) {
        LOG_ERROR((aoap_dev_test, "%s()  Create libaoap accessory session failed=%d", __FUNCTION__, mAccId));
        ret = mAccId;
    } else {
        /* change connect timeout based on the Application setting */
        aoap_set_connect_timeout(mAccId, 8);

        mDevId = aoap_connect(mAccId, \
                              mDevInfo.vendorId, mDevInfo.productId, mDevInfo.serial.c_str(), \
                              &aoap_device_detection_connectCB, 1, this);
        if (mDevId < 0) {
            LOG_ERROR((aoap_dev_test, "%s()  aoap_connect failed=%d", __FUNCTION__, mDevId));
            /* de-initialize AOAP library in case switch failed */
            LOG_INFO((aoap_dev_test, "%s()  Cleanup AOAP library accessory session due to switch device failed.",
                    __FUNCTION__));
            disconnectDevice();
            ret = mDevId;
        } else {
            LOG_INFO((aoap_dev_test, "%s()  Wait for Aoap library device (AccId=%d, DevId=%d) will be switched",
                    __FUNCTION__, mAccId, mDevId));
        }
    }

    return ret;
}

int32_t TddDevice::disconnectDevice() {
    int32_t ret = 0;
    if (mAccId > 0) {
        LOG_INFO((aoap_dev_test, "%s()  cleanup AOAP library accessory session", __FUNCTION__));
        /* cleanup AOAP library accessory session
         * if no devices associated to the accessory session. */
        ret = aoap_defer_delete_accessory(mAccId, mDevId);
        mAccId = -1;
        mDevId = -1;
    }

    return ret;
}

void TddDevice::aoap_device_detection_connectCB(int accessoryId, int deviceId, int result, void *pToken, unsigned int audioSupport)
{
    (void)audioSupport;

    TddDevice* me = (TddDevice*)pToken;
    if (me == nullptr) {
        LOG_ERROR((aoap_dev_test, "Could not cast pToken to TddDevice (AccId=%d, DevId=%d, switchResult=%d)",
                accessoryId, deviceId, result));
        return;
    }

    if (result >= 0) {
        LOG_INFO((aoap_dev_test, "Aoap Library notify device (AccId=%d, DevId=%d) switched to accessory mode",
                accessoryId, deviceId));
    } else {
        LOG_WARN((aoap_dev_test, "Aoap Library notify device (AccId=%d, DevId=%d) switched to accessory mode failed=%d",
                accessoryId, deviceId, result));
        me->setDeviceState(TddDeviceState::DEVICE_STATE_SWITCH_FAIL);

        std::unique_lock<std::mutex> guard(gMutex);
        gNotifications++;
        gConditonVar.notify_one();
    }

    std::unique_lock<std::mutex> guard(gMutex);
    gNotifications++;
    gConditonVar.notify_one();
}


void Tdd::setTestState(TddTestState inTestState)
{
    mTestState |= (int32_t)inTestState;
    LOG_INFO((aoap_dev_test, "%s()  %d", __func__, mTestState));
}
int32_t Tdd::getTestState(void)
{
    LOG_INFO((aoap_dev_test, "%s()  %d", __func__, mTestState));
    return mTestState;
}

std::string Tdd::strTestState(void)
{
    TddTestState currTestState = (TddTestState)mTestState;
    std::string result = "";

    switch (currTestState)
    {
        case TddTestState::IDLE:
        {
            result.assign("IDLE");
            break;
        }
        case TddTestState::DEV_FOUND:
        {
            result.assign("DEV_FOUND");
            break;
        }
        case TddTestState::DEV_SWITCHING:
        {
            result.assign("DEV_SWITCHING");
            break;
        }
        case TddTestState::DEV_LOST:
        {
            result.assign("DEV_LOST");
            break;
        }
        case TddTestState::DEV_ACC_MODE:
        {
            result.assign("DEV_ACC_MODE");
            break;
        }
        case TddTestState::DEV_RESET:
        {
            result.assign("DEV_RESET");
            break;
        }
        case TddTestState::DONE:
        {
            result.assign("DONE");
            break;
        }
        case TddTestState::END:
        {
            result.assign("END");
            break;
        }
        default:
        {
            result.assign("Unknown");
            LOG_WARN((aoap_dev_test, "%s()  Unknown TddTestState:  %d", __func__, (int32_t)currTestState));
            break;
        }
    }

    return result;
}

void Tdd::setTestError(TddTestError inTestError)
{
    mTestError = (int32_t)inTestError;
    LOG_INFO((aoap_dev_test, "%s()  %d", __func__, mTestError));
}
int32_t Tdd::getTestError(void)
{
    LOG_INFO((aoap_dev_test, "%s()  %d - %s", __func__, mTestError, strTestError().c_str()));
    return mTestError;
}

std::string Tdd::strTestError(void)
{
    TddTestError currTestError = (TddTestError)mTestError;
    std::string result = "";

    switch(currTestError)
    {
        case TddTestError::ERR_PASS:
        {
            result.assign("ERR_PASS");
            break;
        }
        case TddTestError::ERR_SETUP:
        {
            result.assign("ERR_SETUP");
            break;
        }
        case TddTestError::ERR_TIMER_CREATE:
        {
            result.assign("ERR_TIMER_CREATE");
            break;
        }
        case TddTestError::ERR_TIMER_START:
        {
            result.assign("ERR_TIMER_START");
            break;
        }
        case TddTestError::ERR_TIMER_TMO:
        {
            result.assign("ERR_TIMER_TMO");
            break;
        }
        case TddTestError::ERR_STATE:
        {
            result.assign("ERR_STATE");
            break;
        }
        case TddTestError::ERR_FAIL:
        {
            result.assign("ERR_FAIL");
            break;
        }
        case TddTestError::ERR_SIGNAL:
        {
            result.assign("ERR_SIGNAL");
            break;
        }
        default:
        {
            result.assign("Unknown");
            LOG_WARN((aoap_dev_test, "%s()  Unknown TddTestError:  %d", __func__, (int32_t)currTestError));
            break;
        }
    }
    return result;
}

void Tdd::signaling(void)
{
    LOG_INFO((aoap_dev_test, "%s()  Signal occurred.", __func__));
    setTestError(TddTestError::ERR_SIGNAL);

    std::unique_lock<std::mutex> guard(gMutex);
    gNotifications = 3;
    gConditonVar.notify_one();
}


Tdd::Tdd()
{
    mTestError = (int32_t)TddTestError::ERR_PASS;
    mTestState = (int32_t)TddTestState::IDLE;
    mRunning = false;
    mDiscoverer = nullptr;
    mMaxDevices = 1;
}

Tdd::~Tdd()
{
}

int main(int argc, char *argv[])
{
    int32_t result = 0;

    int32_t testTime = 0;
    int32_t opt_c;

    Tdd test;
    TestTimer testTimer;

    while((opt_c = getopt(argc, argv, "h:s:")) != -1)
    {
        switch(opt_c)
        {
            case 'h':
                fprintf(stderr, "Usage: aauto-demo [-l <city name>] [-h] \n");
                fprintf(stderr, " -h             : describe option for this command.\n");
                fprintf(stderr, " -s <timeout>     : Automatic smoketest mode with timeout \n");
                return 0;
                break;

            case 's':
                testTime = atoi(optarg);
                if (testTime > 0) {
                    fprintf(stderr, "Timeout in %d Seconds \n", testTime);
                    result = testTimer.createSignalTimer(); //timerid is now valid
                    if (0 != result) {
                        fprintf(stderr, "Failed to create timer \n");
                        test.setTestError(TddTestError::ERR_TIMER_CREATE);
                    }
                } else {
                    fprintf(stderr, "Timeout %d is not valid \n", testTime);
                    test.setTestError(TddTestError::ERR_SETUP);
                }
                break;

            case ':':
                fprintf(stderr, "Option %c requires an operand\n", optopt);
                test.setTestError(TddTestError::ERR_SETUP);
                break;

            default:
                fprintf(stderr, "Unknown argument %c\n", optopt);
                test.setTestError(TddTestError::ERR_SETUP);
                break;
        }
    }

    fprintf(stderr, "%s()  start signalhandler \n", __func__);
    test.startSignalHandler();

    LOG_REGISTER_APP(DLT_TEST_AOAP_TADD, "AOAP DevDetetion test");
    LOG_REGISTER_CONTEXT(aoap_dev_test, "TADD", "AOAP DevDetetion test");

    /* sleep to start DLT */
    std::unique_lock<std::mutex> guard(gMutex);
    gConditonVar.wait_for(guard, std::chrono::milliseconds(1000));
    guard.unlock();

    if ( (int32_t)TddTestError::ERR_PASS == test.getTestError() ) {

        LOG_INFO((aoap_dev_test, "%s()  start timer", __func__));
        result = testTimer.startSignalTimer(testTime);
        if (0 != result) {
            LOG_WARN((aoap_dev_test, "%s()  Failed to start timer", __func__));
            test.setTestError(TddTestError::ERR_TIMER_START);
        } else {

            LOG_INFO((aoap_dev_test, "%s()  start testing", __func__));
            if (true != test.start()) {
                LOG_WARN((aoap_dev_test, "%s()  Failed to start test", __func__));
                test.setTestError(TddTestError::ERR_FAIL);
            } else {
                std::unique_lock<std::mutex> guard(gMutex);
                LOG_INFO((aoap_dev_test, "%s()  wait until test execution ends", __func__));
                gConditonVar.wait(guard, []{return gNotifications >= 2;});
                LOG_INFO((aoap_dev_test, "%s()  wait returned - %s",
                        __func__, (gNotifications == 3) ? "signal received" : "test executed"));
                guard.unlock();
            }
        }
    } else {
        LOG_INFO((aoap_dev_test, "%s()  test not started due to initial error", __func__));
    }

    LOG_INFO((aoap_dev_test, "%s()  stop timer", __func__));
    testTimer.deleteSignalTimer();

    LOG_INFO((aoap_dev_test, "%s()  stop testing", __func__));
    test.stop();

    /* Check if test execution was done */
    if ( (int32_t)TddTestState::DONE == test.getTestState() ) {
        result = (int32_t)TddTestError::ERR_PASS;
    } else {
        /* get test result */
        result = test.getTestError();
    }

   /* Test result output */
   if (result == (int32_t)TddTestError::ERR_PASS) {
       LOG_INFO((aoap_dev_test, "AOAP DISCOVERY SMOKETEST PASSED"));
       /* for Console */
       fprintf(stderr,"\n");
       fprintf(stderr, "AOAP DISCOVERY PASSED\n");
       result = 0;
   } else {
       LOG_ERROR((aoap_dev_test, "AOAP DISCOVERY FAILED"));
       LOG_ERROR((aoap_dev_test, "test device state: %d | test result: %d (%s)",
               test.getTestState(), result, test.strTestError().c_str()));
       /* for Console */
       fprintf(stderr,"\n");
       fprintf(stderr,"AOAP DISCOVERY FAILED\n");
       fprintf(stderr,"test device state: %d (%s) | test result: %d (%s) \n",
               test.getTestState(), test.strTestState().c_str(), result, test.strTestError().c_str());
   }

   LOG_UNREGISTER_CONTEXT(aoap_dev_test);
   LOG_UNREGISTER_APP();

   fprintf(stderr, "%s()  stop signalhandler \n", __func__);
   test.stopSignalHandler();

    return result;
}


