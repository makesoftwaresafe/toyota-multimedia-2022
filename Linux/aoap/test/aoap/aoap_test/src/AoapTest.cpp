/**
* \file: AoapTest.cpp
*
* \version: 0.1
*
* \release: $Name:$
*
* Implementation to test aoap-test.
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

/* AoapTest header */
#include "AoapTest.h"

/* header of component to test */
#include <aoap_types.h>
#include <aoap.h>


/* logging header (DLT) */
#include <adit_logging.h>
#include <adit_dlt.h>

/* header for test synchronization, etc. */
#include <condition_variable>
#include <mutex>
#include <string.h>

/* read arguments */
#include <getopt.h>

#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <assert.h>

#ifndef DLT_TEST_AOAP_TAOA
#define DLT_TEST_AOAP_TAOA "TAOA"
#endif

LOG_DECLARE_CONTEXT(aoap_test);


/* TODO:    Really necessary */
int32_t gQuit = 0;


/* test synchronization */
std::condition_variable gConditonVar;
std::mutex gMutex;


void AoapTest::setTestResult(AoapTestResults inTestResult)
{
    mTestResult = (int32_t)inTestResult;
}

int32_t AoapTest::getTestResult(void)
{
    return mTestResult;
}

std::string AoapTest::getErrString()
{
    std::string str = "";
    if (mTestResult == (int32_t)AoapTestResults::IDLE) {
        str.assign("IDLE");
    } else if (mTestResult == (int32_t)AoapTestResults::PASS) {
        str.assign("PASS");
    } else if (mTestResult == (int32_t)AoapTestResults::SETUP_ERR) {
        str.assign("SETUP_ERR");
    } else if (mTestResult == (int32_t)AoapTestResults::TIMER_CREATE_ERR) {
        str.assign("TIMER_CREATE_ERR");
    } else if (mTestResult == (int32_t)AoapTestResults::TIMER_START_ERR) {
        str.assign("TIMER_START_ERR");
    } else if (mTestResult == (int32_t)AoapTestResults::SWITCH_TIMEOUT) {
        str.assign("SWITCH_TIMEOUT");
    } else if (mTestResult == (int32_t)AoapTestResults::STATE_ERR) {
        str.assign("STATE_ERR");
    } else if (mTestResult == (int32_t)AoapTestResults::AOAP_NOT_SUPPORTED) {
        str.assign("AOAP_NOT_SUPPORTED");
    } else if (mTestResult == (int32_t)AoapTestResults::NO_TEST_DEVICE) {
        str.assign("NO_TEST_DEVICE");
    } else if (mTestResult == (int32_t)AoapTestResults::FAIL) {
        str.assign("FAIL");
    } else {
        LOG_ERROR((aoap_test, "%s()  Unknown test result '%d'", __func__, mTestResult));
        str.assign("UNKOWN");
    }


    return str;
}


int32_t AoapTest::start()
{
    int32_t ret = 0;

    if (0 > (ret = pthread_create(&mMonitorThreadId, nullptr, &aoapMonitorThread, this))) {
        LOG_ERROR((aoap_test, "%s()  create aoapMonitorThread failed=%d, errno=%d", __func__, ret, errno));
        return ret;
    } else {
        LOG_INFO((aoap_test, "%s()  USB monitor registered", __func__));
    }

    /* create eventFd to send stop event */
    if (0 > (mEventFd = eventfd(0, 0))) {
        LOG_ERROR((aoap_test, "%s()  create EventFd failed=%d, errno=%d", __func__, mEventFd, errno));
        ret = mEventFd;
        return ret;
    }

    LOG_INFO((aoap_test, "%s()  create timer", __func__));
    if (0 != mAoapTestTimer.createSignalTimer())
        setTestResult(AoapTestResults::TIMER_CREATE_ERR);

    LOG_INFO((aoap_test, "%s()  start timer", __func__));
    if (0 != mAoapTestTimer.startSignalTimer(mTimeout))
        setTestResult(AoapTestResults::TIMER_START_ERR);

    return ret;
}

int32_t AoapTest::stop()
{
    int32_t ret = 0;
    uint64_t event = 1;

    LOG_INFO((aoap_test, "%s()  stop timer", __func__));
    mAoapTestTimer.deleteSignalTimer();

    /* X. Test Step - delete accessory. */
    if (mAccId >= 0) {
        aoap_delete_accessory(mAccId); //cleanup AOAP
        mAccId = -1;
    }

    mShutdown = true;

    if (mEventFd >= 0) {
        /* trigger eventfd */
        if (eventfd_write(mEventFd, event) != 0){
            LOG_ERROR((aoap_test, "%s() eventfd_write(fd=%d) failed", __func__, mEventFd));
        } else {
            LOG_INFO((aoap_test, "%s() eventfd_write(fd=%d) to stop spiUsbMonitor", __func__, mEventFd));
        }
    } else {
        LOG_ERROR((aoap_test, "%s() mEventFd=%d is invalid", __func__, mEventFd));
    }

    if (mMonitorThreadId != 0) {
        pthread_join(mMonitorThreadId, nullptr);
        mMonitorThreadId = 0;
    }

    /* Close the eventfd after usbMonitorThread has joined.
     * Otherwise, usbMonitorThread may could not read the stop event. */
    if (mEventFd >= 0) {
        close(mEventFd);
        mEventFd = -1;
    }

    if (mpMonitor) {
        udev_monitor_unref(mpMonitor);
        mpMonitor = nullptr;
    }
    if (mpUdev) {
        udev_unref(mpUdev);
        mpUdev = nullptr;
    }

    return ret;
}

bool AoapTest::startUdevMonitor() {
    /* create udev monitor */
    mpMonitor = udev_monitor_new_from_netlink(mpUdev, "udev");
    if (nullptr != mpMonitor) {
        /* filter for usb_devices only */
        if (0 == udev_monitor_filter_add_match_subsystem_devtype(mpMonitor, "usb", "usb_device")) {
            if (0 == udev_monitor_enable_receiving(mpMonitor)) {
                /* get file descriptor of udev monitor */
                mUdevMonitorFd = udev_monitor_get_fd(mpMonitor);
                if (mUdevMonitorFd >= 0) {
                    return true;
                } else {
                    LOG_ERROR((aoap_test, "%s()  udev_monitor_get_fd() failed", __func__));
                }
            } else {
                LOG_ERROR((aoap_test, "%s()  udev_monitor_enable_receiving() failed", __func__));
            }
        } else {
            LOG_ERROR((aoap_test, "%s()  udev_monitor_filter_add_match_subsystem_devtype() failed", __func__));
        }
    } else {
        LOG_ERROR((aoap_test, "%s()  udev_monitor_new_from_netlink() failed", __func__));
    }

    return false;
}

void AoapTest::connectCallback(int32_t accessoryId, int32_t deviceId, int32_t result, void *pToken, uint32_t audioSupport)
{
    LOG_INFO((aoap_test, "%s()  called for accId:%d, devId:%d with result:%d, audioSupport=%u",
            __func__, accessoryId, deviceId, result, audioSupport));

    if (nullptr != pToken) {
        AoapTest* me = (AoapTest*)pToken;

        if (AOAP_SUCCESS == result) {
            LOG_INFO((aoap_test, "%s()  switch to accessory mode success=%d", __func__, result));
        } else if (AOAP_ERROR_ALREADY_DONE == result) {
            LOG_INFO((aoap_test, "%s()  switch to accessory mode already done=%d", __func__, result));
        } else {
            LOG_ERROR((aoap_test, "%s()  switch to accessory mode failed=%d", __func__, result));

            me->setTestResult(AoapTestResults::SWITCH_TIMEOUT);

            gQuit = true;
            gConditonVar.notify_one();
        }
        me->mSwitchResult = result;
        if (0 > sem_post(&me->mSwitchSyncSem)) {
            LOG_ERROR((aoap_test, "%s()  sem_post() failed", __func__));
        } else {
            LOG_INFO((aoap_test, "%s()  sem_post() done", __func__));
        }
    } else {
        LOG_FATAL((aoap_test, "%s()  pToken is NULL. Cannot signal main thread", __func__));
        assert(pToken);
    }
}

int32_t AoapTest::switchDevice(myDevice& inDevice)
{
    int32_t result = 0;
    uint32_t majorVersion = 0;
    uint32_t minorVersion = 0;

    /* 1. Test Step - create accessory. */
    mAccId = aoap_create_accessory((t_aoap_accessory_param*)&mAccessory);
    if (mAccId >= 0) {
        LOG_INFO((aoap_test, "%s()  Accessory %d initialized", __func__, mAccId));

        /* 2. Test Step - set timeout for AOAP switch. */
        aoap_set_connect_timeout(mAccId, 8); //change connect timeout


        /* 3. Test Step - check AOAP support */
        result = aoap_check_support(mAccId, inDevice.mVendorId, inDevice.mProductId,
                                    inDevice.mSerial.c_str(), &majorVersion, &minorVersion);
        if ((result >= AOAP_SUCCESS) && (majorVersion > 0)) {
            LOG_INFO((aoap_test, "%s()  device 0x%X:0x%X:%s supports AOAP version %u.%u",
                    __func__, inDevice.mVendorId, inDevice.mProductId, inDevice.mSerial.c_str(), majorVersion, minorVersion));

            /* 4. Test Step - switch device to AOAP mode */
            mDevId = aoap_connect(mAccId, inDevice.mVendorId, inDevice.mProductId,
                    inDevice.mSerial.c_str(), &connectCallback, mAccessory.enableAudio, this);
            if (0 > mDevId) {
                LOG_WARN((aoap_test, "%s()  aoap_connect() returned with %d", __func__, mDevId));
                result = mDevId;
            }
        } else {
            LOG_INFO((aoap_test, "%s()  device 0x%X:0x%X:%s does not support AOAP (result=%d, %u.%u)",
                    __func__, inDevice.mVendorId, inDevice.mProductId, inDevice.mSerial.c_str(), result, majorVersion, minorVersion));

            setTestResult(AoapTestResults::AOAP_NOT_SUPPORTED);
            if (mAccId >= 0) {
                aoap_delete_accessory(mAccId); //cleanup AOAP
                mAccId = -1;
            }
            if (0 == result)
                result = -1;
        }
    } else {
        LOG_ERROR((aoap_test, "%s()  Accessory not initialized (%d)", __func__, mAccId));
        result = mAccId;
    }

    return result;
}

int32_t AoapTest::releaseDevice(uint32_t inVendorId, uint32_t inProductId, std::string inSerial)
{
    int32_t result = 0;
    int i = 0;
    libusb_context* UsbContext = NULL;
    libusb_device_handle *UsbDeviceHandle = NULL;

    libusb_init(&UsbContext);
    if (UsbContext) {
        if ((inProductId < 0x2d00) || (inProductId > 0x2d05)) {
            LOG_WARN((aoap_test, "%s()  USB port reset of device %s which is not in accessory mode", __func__, inSerial.c_str()));
        }

        UsbDeviceHandle = libusb_open_device_with_vid_pid(UsbContext, inVendorId, inProductId);
        if (UsbDeviceHandle) {
            libusb_device *pDevs = NULL;
            pDevs = libusb_get_device(UsbDeviceHandle);
            if (pDevs) {
                struct libusb_device_descriptor desc;
                result = libusb_get_device_descriptor(pDevs, &desc);
                if ((result >= 0) && (desc.iSerialNumber > 0)) {
                    char retrievedSerial[64];
                    int len = libusb_get_string_descriptor_ascii(
                            UsbDeviceHandle, desc.iSerialNumber,
                            (unsigned char*) retrievedSerial,
                            sizeof(retrievedSerial));

                    if ((len >= 0) && (0 == inSerial.compare(retrievedSerial))) {
                        LOG_INFO((aoap_test, "%s()  perform USB port reset \n", __func__));

                        int i = 0;
                        int maxRetries = 10;
                        int waitTimeSec = 1;
                        do
                        {
                            result = libusb_reset_device(UsbDeviceHandle);
                            LOG_INFO((aoap_test, "%s()  libusb_reset_device() = %d", __func__, result));
                            if ((result != LIBUSB_ERROR_NOT_FOUND) && (result != LIBUSB_ERROR_NO_DEVICE)) {
                                sleep(waitTimeSec);
                            }
                            i++;
                        } while ( ((result != LIBUSB_ERROR_NOT_FOUND) && (result != LIBUSB_ERROR_NO_DEVICE)) && (maxRetries > i) );
                    } else {
                        LOG_INFO((aoap_test, "%s()  serial %s does not match to %s ",
                                __func__, &retrievedSerial[0], inSerial.c_str()));
                    }
                } else {
                    LOG_WARN((aoap_test, "%s()  Could not get serial", __func__));
                }
            } else {
                LOG_ERROR((aoap_test, "%s()  Device %d is NULL \n", __func__, i));
            }

            libusb_close(UsbDeviceHandle);
        } else {
            LOG_WARN((aoap_test, "%s()  Could not open device(0x%X:0x%X)",
                    __func__, inVendorId, inProductId));
        }

        libusb_exit(UsbContext);
    } else {
        LOG_ERROR((aoap_test, "%s() USB context is not retrieved \n", __func__));
    }

    return result;
}

void* AoapTest::aoapMonitorThread(void *pArg)
{
    struct udev_device *pUdevDevice = nullptr;
    bool bStopMonitoring = false;

    uint64_t event = 0;

    int32_t result = 0;

    prctl(PR_SET_NAME, "aoapMonitorThread", 0, 0, 0);

    auto me = static_cast<AoapTest*>(pArg);
    if (me == nullptr) {
        LOG_ERROR((aoap_test, "%s()  could not cast input pointer to AoapTest", __func__));
        return nullptr;
    }

    me->mpUdev = udev_new();
    if (nullptr == me->mpUdev) {
        LOG_FATAL((aoap_test, "%s()  could not create udev\n", __func__));
        result = -1;
    }
    if (0 == result) {
        if (true != me->startUdevMonitor()) {
            LOG_ERROR((aoap_test, "%s()  could not start UdevMonitoring", __func__));
            result = -1;
        }
    }

    if (0 == result) {
        int32_t j = 0;
        int32_t selectResult = 0;
        int32_t nfds = 0;   /* highest-numbered file descriptor in any of the three sets, plus 1 */
        int32_t cntfds = 2; /* Currently, we have two fds, mUdevMonitorFd + mEventFd */
        fd_set fds;

        LOG_INFO((aoap_test, "%s()  start monitoring", __func__));

        /* start device detection monitor */
        while ((true != me->mShutdown) && (true != bStopMonitoring))
        {
            FD_ZERO(&fds);
            FD_SET(me->mUdevMonitorFd, &fds);
            FD_SET(me->mEventFd, &fds);

            if (me->mUdevMonitorFd > me->mEventFd) {
                nfds = me->mUdevMonitorFd;
            } else {
                nfds = me->mEventFd;
            }

            selectResult = select(nfds+1, &fds, nullptr, nullptr, NULL);
            /* select return the number of triggered file descriptors */
            if (selectResult > 0) {
                /* go through all file descriptors and handle if necessary */
                for (j = 0; (j < cntfds) && (selectResult > 0); j++)
                {
                    /* udev event triggered at file descriptor mUdevMonitorFd */
                    if ((j < cntfds) && (FD_ISSET(me->mUdevMonitorFd, &fds))) {
                        /* get udev device */
                        pUdevDevice = udev_monitor_receive_device(me->mpMonitor);
                        if (nullptr != pUdevDevice) {
                            if (0 == strcmp(udev_device_get_action(pUdevDevice), "add")) {
                                LOG_INFO((aoap_test, "%s()  Device was added", __func__));

                                myDevice* _device = new myDevice;
                                result = me->verifyUdevDevice(pUdevDevice, *_device);
                                if (0 == result) {
                                    LOG_INFO((aoap_test, "%s()  verified device 0x%X:0x%X:%s (in list: %zu)",
                                            __func__, _device->mVendorId, _device->mProductId, _device->mSerial.c_str(), me->mDeviceList.size()));

                                    if ((_device->mProductId >= 0x2d00) && (_device->mProductId <= 0x2d05)) {
                                        LOG_INFO((aoap_test, "%s()  device 0x%X:0x%X:%s in AOAP mode (in list: %zu)",
                                                __func__, _device->mVendorId, _device->mProductId, _device->mSerial.c_str(), me->mDeviceList.size()));

                                        bool knownDevice = false;
                                        uint32_t i = 0;
                                        if (true != me->mDeviceList.empty()) {
                                            for (i = 0; i < me->mDeviceList.size(); i++)
                                            {
                                                if ( (0 == me->mDeviceList[i]->mSysPath.compare(_device->mSysPath))
                                                     && (0 == me->mDeviceList[i]->mSerial.compare(_device->mSerial)) ) {

                                                    knownDevice = true;

                                                    LOG_INFO((aoap_test, "%s()  before sem_wait() - mSwitchResult = %d", __func__, me->mSwitchResult));
                                                    if (0 > sem_wait(&me->mSwitchSyncSem)) {
                                                        LOG_ERROR((aoap_test, "%s()  sem_wait() failed", __func__));
                                                    } else {
                                                        LOG_INFO((aoap_test, "%s()  after mSwitchResult = %d", __func__, me->mSwitchResult));
                                                    }
                                                }
                                            }
                                        }
                                        if (true != knownDevice) {
                                            LOG_WARN((aoap_test, "%s()  unkown device already in AOAP mode (in list: %zu)", __func__, me->mDeviceList.size()));
                                        } else {
                                            LOG_INFO((aoap_test, "%s()  reset known device an exit (in list: %zu)", __func__, me->mDeviceList.size()));

                                            me->setTestResult(AoapTestResults::PASS);

                                            gQuit = true;
                                            gConditonVar.notify_one();
                                        }

                                        result = me->releaseDevice(_device->mVendorId, _device->mProductId, _device->mSerial);

                                        /* remove device from internal list */
                                        std::deque<myDevice*>::iterator iter;
                                        for (i = 0; i < me->mDeviceList.size(); i++)
                                        {
                                            for (iter = me->mDeviceList.begin(); iter < me->mDeviceList.end(); iter++)
                                            {
                                                LOG_INFO((aoap_test, "%s()  erase item from internal list", __func__));
                                                me->mDeviceList.erase(iter);
                                            } /* for */
                                        } /* for */

                                        LOG_INFO((aoap_test, "%s()  in list: %zu", __func__, me->mDeviceList.size()));
                                    } else {

                                        if (!me->mDeviceList.empty()) {
                                            LOG_INFO((aoap_test, "%s()  Test already in progress (in list: %zu)", __func__, me->mDeviceList.size()));
                                            break;
                                        }

                                        result = me->switchDevice(*_device);
                                        LOG_INFO((aoap_test, "%s()  switchDevice() = %d ", __func__, result));
                                        if (0 == result) {
                                            /* insert device to internal list */
                                            me->mDeviceList.push_back(_device);
                                            LOG_INFO((aoap_test, "%s()  device added to list (size: %zu)", __func__, me->mDeviceList.size()));
                                        }
                                    }
                                } else {
                                    LOG_INFO((aoap_test, "%s()  not a device to test", __func__));
                                    if (me->mDeviceList.empty()) {
                                        /* setTestResult in case we are not already testing */
                                        me->setTestResult(AoapTestResults::NO_TEST_DEVICE);
                                    }
                                }
                            } else if (0 == strcmp(udev_device_get_action(pUdevDevice), "remove")) {
                                LOG_INFO((aoap_test, "%s()  Device was removed", __func__));
                                LOG_INFO((aoap_test, "%s()    sysPath: %s", __func__, udev_device_get_syspath(pUdevDevice)));

                            } else if (0 == strcmp(udev_device_get_action(pUdevDevice), "change")) {
                                LOG_INFO((aoap_test, "%s()  Device was changed", __func__));
                                LOG_INFO((aoap_test, "%s()  Udev event 'change' received for device %s:%s",
                                            __func__, udev_device_get_sysattr_value(pUdevDevice, "idVendor"),
                                            udev_device_get_sysattr_value(pUdevDevice, "idProduct")));
                            } else {
                                LOG_INFO((aoap_test, "%s()  unknown udev action=%s",
                                        __func__, udev_device_get_action(pUdevDevice)));
                            }
                            udev_device_unref(pUdevDevice);
                        } else {
                            LOG_ERROR((aoap_test, "%s()  pUdevDevice is NULL", __func__));
                        }
                        selectResult--;
                    }
                    /* eventfd triggered at file descriptor mEventFd */
                    if ((j < cntfds) && (FD_ISSET(me->mEventFd, &fds))) {
                        /* read eventfd to get current number of occurred events */
                        if (eventfd_read(me->mEventFd, &event) != 0) {
                            LOG_ERROR((aoap_test, "%s()  eventfd_read(fd=%d) failed", __func__, me->mEventFd));
                        } else {
                            LOG_INFO((aoap_test, "%s()  received stop event", __func__));
                        }
                        bStopMonitoring = true;
                        selectResult--;
                    }
                } // for-loop
            } else {
                /* selectResult = 0 : select() timed out
                 * selectResult < 0 : indicates an error (check errno)
                 */
                LOG_ERROR((aoap_test, "%s()  select failed=%d", __func__, selectResult));
            }
        } /* while */

    }

    LOG_INFO((aoap_test, "%s()  exit", __func__));
    return nullptr;
}


int32_t AoapTest::getDevPath(struct udev_device* inUdevDevice) {
    int32_t devPath = -1;

    const char* strDevPath = udev_device_get_sysattr_value(inUdevDevice, "devpath");
    if (NULL != strDevPath) {
        sscanf(strDevPath, "%x", &devPath);
    } else {
        LOG_WARN((aoap_test, "%s()  Could not get udev device sysattr '%s'.", __func__, "devpath"));
    }

    /* The devpath of an USB controller is = 0.
     * The first possible device can be on devpath = 1.
     * Check the devices which has a devpath = 1 and the vendorId. */
    return devPath;
}

int32_t AoapTest::verifyUdevDevice(struct udev_device* inUdevDevice, myDevice& outDevice)
{
    int32_t ret = -1;
    int devPath = -1;

    /* The devpath of an USB controller is = 0.
     * The first possible device can be on devpath = 1.
     * Check the devices which has a devpath = 1 and the vendorId. */
    devPath = getDevPath(inUdevDevice);
    if (devPath > 0) {
        /* From here, we can call get_sysattr_value() for each file
           in the device's /sys entry. The strings passed into these
           functions (idProduct, idVendor, serial, etc.) correspond
           directly to the files in the directory which represents
           the USB device. Note that USB strings are Unicode, UCS2
           encoded, but the strings returned from
           udev_device_get_sysattr_value() are UTF-8 encoded. */

        /* Check the USB base class to identify USB Hub(s). */
        const char* strDevClassString      = udev_device_get_sysattr_value(inUdevDevice, "bDeviceClass");
        const char* strDevSubClassString   = udev_device_get_sysattr_value(inUdevDevice, "bDeviceSubClass");
        const char* strDevProtocolString   = udev_device_get_sysattr_value(inUdevDevice, "bDeviceProtocol");
        /* Base Class 09h (Hub)  is defined for devices that are USB hubs
         * and conform to the definition in the USB specification. */
        if ( (NULL != strDevClassString) && (NULL != strDevSubClassString) && (NULL != strDevProtocolString) ) {
            uint32_t devClass = 0, devSubClass = 0, devProt = 0;
            sscanf(strDevClassString,    "%x", &devClass);
            sscanf(strDevSubClassString, "%x", &devSubClass);
            sscanf(strDevProtocolString, "%x", &devProt);
            /* If the device is an USB hub, do not add the device
             * to the internal device list and avoid check for AOAP support */
            if ((devClass == 9) && (devSubClass == 0)) {
                LOG_INFO((aoap_test, "%s() found a USB Hub (DevClass=%d, DevSubClass=%d, DevProtocol=%d)",
                        __func__, devClass, devSubClass, devProt));
            } else {
                /* Device is not a USB Hub, but it could be some Unwired (Hub) Technology.
                 * Continue to get the USB device sysattr_values */

                const char* strVendorIdString      = udev_device_get_sysattr_value(inUdevDevice, "idVendor");
                const char* strProductIdString     = udev_device_get_sysattr_value(inUdevDevice, "idProduct");
                const char* strSerialString        = udev_device_get_sysattr_value(inUdevDevice, "serial");
                const char* strProductString       = udev_device_get_sysattr_value(inUdevDevice, "product");
                const char* strManufacturerString  = udev_device_get_sysattr_value(inUdevDevice, "manufacturer");
                const char* strSysPathString       = udev_device_get_syspath(inUdevDevice);
                if ( (NULL != strVendorIdString) && (NULL != strProductIdString)
                     && (NULL != strSerialString) && (NULL != strProductString)
                     && (NULL != strManufacturerString) /*&& (NULL != strDevNumString)*/
                     && (NULL != strSysPathString) ) {

                    uint32_t _vendorId = 0;
                    uint32_t _productId = 0;
                    sscanf(strVendorIdString,  "%x", &_vendorId);
                    sscanf(strProductIdString, "%x", &_productId);

                    uint32_t j = 0;
                    for(j = 0; j < mBlackList.size(); j++)
                    {
                        if (_vendorId == mBlackList[j]) {
                            break;
                        }
                    }

                    if (_vendorId == mBlackList[j]) {
                        LOG_WARN((aoap_test, "%s() found  device of vendorId:0x%X which is black listed",
                                __func__, mBlackList[j]));
                    } else if (_vendorId == 0x2996) {
                        /* Check if the device is part of the Unwired Technology
                         * and do not add such device to the internal device list.
                         * (should avoid check for AOAP support) */
                        LOG_INFO((aoap_test, "%s() found Unwired (Hub) Technology (%s, %s, %s)",
                                __func__, strVendorIdString, strProductIdString, strProductString));
                    } else if (_vendorId == 0x05ac) {
                        LOG_INFO((aoap_test, "%s() found an Apple device (%s, %s, %s)",
                                __func__, strVendorIdString, strProductIdString, strProductString));
                    } else {
                        outDevice.mVendorId = _vendorId;
                        outDevice.mProductId = _productId;

                        outDevice.mSerial.assign(strSerialString);
                        outDevice.mProduct.assign(strProductString);
                        outDevice.mManufacturer.assign(strManufacturerString);
                        outDevice.mSysPath.assign(strSysPathString);
                        outDevice.mDevNum= udev_device_get_devnum(inUdevDevice);

                        LOG_INFO((aoap_test, "%s() found  device vendorId:%s, productId=%s, sn=%s, product=%s, devNum=%u",
                                __func__, strVendorIdString, strProductIdString, strSerialString, strProductString, outDevice.mDevNum));
                        ret = 0;
                    }
                } else {
                    LOG_WARN((aoap_test, "%s() device_get_sysattr_value() is NULL", __func__));
                }
            }
        } else {
            LOG_WARN((aoap_test, "%s() device_get_sysattr_value() of USB Base Class, SubClass, Protocol are NULL", __func__));
        }
    } else if (devPath == 0) {
        LOG_INFO((aoap_test, "%s() found USB controller", __func__));
    } else {
        /* could not get devPath from udev device sysattr */
        LOG_INFO((aoap_test, "%s() could not get devPath. devPath=%d", __func__, devPath));
    }

    return ret;
}

int32_t AoapTest::setConfig(int argc, char *argv[])
{
    int32_t ret = 0;
    int32_t opt_c;

    const char* b1 = 0;
    uint32_t b2 = 0;

    while((opt_c = getopt(argc, argv, "a:b:d:h:m:o:s:t:u:v:")) != -1)
    {
        switch(opt_c)
        {
            case 'a':
            {
                mAccessory.enableAudio = atoi(optarg);
                break;
            }
            case 'b':
            {
                b1 = optarg;
                sscanf(b1,  "%x", &b2);
                mBlackList.push_back(b2);
                fprintf(stdout, "%s()  vendorId:0x%X set to backlist\n", __func__, b2);
                break;
            }
            case 'd':
            {
                mAccessory.description = optarg;
                break;
            }
            case 'h':
            {
                fprintf(stdout, "USAGE: aoap_test [-a 0|1] [-d <description>] [-m <manufacturer>] [-o <model>] [-s <serial>] [-u <uri>] [-v <version>] [-t <timeout>] \n");
                fprintf(stdout, "USAGE: aoap_test -h\n\n");
                fprintf(stdout, "Options:\n");
                fprintf(stdout, "  --audio,        -a <enable audio>    : enable audio of the accessory (0 = disable, 1 = enable)\n");
                fprintf(stdout, "  --blacklist     -b <blacklist>       : vendorID which is black listed and shall not used for test (e.g. 5ac)\n");
                fprintf(stdout, "  --description,  -d <description>     : description of the accessory\n");
                fprintf(stdout, "  --manufacturer, -m <manufacturer>    : manufacturer of the accessory\n");
                fprintf(stdout, "  --model,        -o <model>           : model name of the accessory\n");
                fprintf(stdout, "  --serial,       -s <serial>          : serial of the accessory\n");
                fprintf(stdout, "  --uri,          -u <uri>             : URI of the accessory\n");
                fprintf(stdout, "  --version,      -v <version>         : version of the accessory\n");
                fprintf(stdout, "  --timeout,      -t                   : timeout of automated Smoketest\n");
                fprintf(stdout, "  --help,         -h                   : prints this help\n");
                return(EXIT_SUCCESS);
                //break;
            }
            case 'm':
            {
                mAccessory.manufacturer = optarg;
                break;
            }
            case 'o':
            {
                mAccessory.modelName = optarg;
                break;
            }
            case 's':
            {
                mAccessory.serial = optarg;
                break;
            }
            case 't':
            {
                mTimeout = atoi(optarg);
                if (mTimeout > 0) {
                    fprintf(stdout, "%s()  Timeout configured to %d [sec] \n", __func__, mTimeout);
                } else {
                    fprintf(stdout, "%s()  ERROR:  Timeout %d is not valid \n", __func__, mTimeout);
                    setTestResult(AoapTestResults::SETUP_ERR);
                }
                break;
            }
            case 'u':
            {
                mAccessory.uri = optarg;
                break;
            }
            case 'v':
            {
                mAccessory.version = optarg;
                break;
            }
            default:
            {
                fprintf(stdout, "%s()  ERROR:  Invalid option %c\n",
                        __func__, optopt);
                break;
            }
        }
    }

    return ret;
}

AoapTest::AoapTest()
{
    mRunning = false;
    /* set timeout to 10 sec by default */
    mTimeout = 10;

    mAccId = -1;
    mDevId = -1;

    setTestResult(AoapTestResults::IDLE);

    mMonitorThreadId = 0;
    mpUdev = nullptr;
    mpMonitor = nullptr;
    mUdevMonitorFd = -1;
    mShutdown = false;
    mSwitchResult = -1;

    if (sem_init(&mSwitchSyncSem, 0, 0) == -1) {
        assert(&mSwitchSyncSem);
    }

    /* set default values */
    mAccessory.manufacturer = "Android";
    mAccessory.modelName    = "Android Auto";
    mAccessory.description  = "Android Auto";
    mAccessory.version      = "1.0";
    mAccessory.uri          = "http://www.android.com/auto";
    mAccessory.serial       = "000000001234567";
    mAccessory.enableAudio  = 0;
}

AoapTest::~AoapTest()
{
    sem_post(&mSwitchSyncSem);
    sem_destroy(&mSwitchSyncSem);
}

void AoapTest::signaling(void)
{
    fprintf(stdout, "\n %s()  Signal occurred. \n\n", __func__);
    int32_t currRes = getTestResult();
    if ( ((int32_t)AoapTestResults::IDLE == currRes) || ((int32_t)AoapTestResults::PASS == currRes) ) {
        fprintf(stdout, " %s()  current result:  %d \n", __func__, currRes);
        setTestResult(AoapTestResults::FAIL);
    }

    gQuit = true;
    gConditonVar.notify_one();
}


int32_t main(int32_t argc, char *argv[])
{
    int32_t result = 0;

    AoapTest _aoapTest;

    fprintf(stdout, "%s()  start signalhandler \n", __func__);
    result = _aoapTest.startSignalHandler();


    LOG_REGISTER_APP(DLT_TEST_AOAP_TAOA, "AOAP test");
    LOG_REGISTER_CONTEXT(aoap_test, "TAOA", "AOAP Smoketest");

    /* sleep to give DLT time to be ready */
    sleep(2);

    result = _aoapTest.setConfig(argc, argv);

    result = _aoapTest.start();

    LOG_INFO((aoap_test, "%s()  Use udevadm to trigger add event(s)", __func__));
    fprintf(stdout, "%s()  Use udevadm to trigger add event(s)", __func__);
    int32_t err = system("udevadm trigger --type=devices --subsystem-match=usb --action=add");
    if (0 != err)
    {
        LOG_ERROR((aoap_test, "%s()  Failed to trigger add event(s). system() returned with err = %d", __func__, err));
    }


    std::unique_lock<std::mutex> guard(gMutex);
    fprintf(stdout, "%s()  wait until test execution ends or timer expired. \n", __func__);
    gConditonVar.wait(guard);
    guard.unlock();

    LOG_INFO((aoap_test, "%s()  test execution done. Stop testing.", __func__));
    fprintf(stdout, "%s()  test execution done. Stop testing. \n", __func__);
    result = _aoapTest.stop();


    result = _aoapTest.getTestResult();
   /* Test result output */
   if (result == (int32_t)AoapTestResults::PASS) {
       LOG_INFO((aoap_test, "AOAP SMOKETEST PASSED !!!"));
       /* for Console */
       result = 0;
       fprintf(stdout,"\n");
       fprintf(stdout, "AOAP SMOKETEST PASSED !!! \n");
       fprintf(stdout,"result: %d \n\n", result);
   } else {
       LOG_ERROR((aoap_test, "AOAP SMOKETEST FAILED !!!"));
       LOG_ERROR((aoap_test, "result: %s", _aoapTest.getErrString().c_str() ));
       /* for Console */
       fprintf(stdout,"\n");
       fprintf(stdout,"AOAP SMOKETEST FAILED !!! \n");
       fprintf(stdout,"result: %s \n\n", _aoapTest.getErrString().c_str());
   }


    LOG_UNREGISTER_CONTEXT(aoap_test);
    LOG_UNREGISTER_APP();


    fprintf(stdout, "%s()  stop signalhandler \n", __func__);
    _aoapTest.stopSignalHandler();

    return result;
}

