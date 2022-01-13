/**
* \file: AoapTest.h
*
* \version: 0.1
*
* \release: $Name:$
*
* Includes the necessary header file to test the aoap-test.
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

#ifndef TEST_AOAP_TEST_H_
#define TEST_AOAP_TEST_H_

/* *************  includes  ************* */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>

#include <signal.h>
#include <deque>
#include <semaphore.h>

/* device detection and handling */
#include <libudev.h>
#include <libusb-1.0/libusb.h>

#include "SignalHandler.h"
#include "Timer.h"


enum class AoapTestResults : int32_t
{
    IDLE                = 0,
    PASS                = 1 << 0,
    SETUP_ERR           = 1 << 1,
    TIMER_CREATE_ERR    = 1 << 2,
    TIMER_START_ERR     = 1 << 3,
    SWITCH_TIMEOUT      = 1 << 4,
    STATE_ERR           = 1 << 5,
    AOAP_NOT_SUPPORTED  = 1 << 6,
    NO_TEST_DEVICE      = 1 << 7,
    FAIL                = 1 << 8
//!< ERR
};


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

struct myDevice {
    uint32_t    mVendorId;
    uint32_t    mProductId;
    std::string mSerial;
    std::string mProduct;
    std::string mManufacturer;
    std::string mSysPath;
    uint32_t    mDevNum;
};


class AoapTest : public SignalHandler {
public:
    AoapTest();
    virtual ~AoapTest();

    virtual void signaling(void);

    int32_t start();
    int32_t stop();

    int32_t setConfig(int argc, char *argv[]);

    void setTestResult(AoapTestResults inResult);
    int32_t getTestResult(void);
    std::string getErrString(void);
private:

    bool mRunning;

    int32_t mTestResult; // of type AoapTestResults

    int32_t mAccId;
    int32_t mDevId;

    myAoap mAccessory;

    int32_t mTimeout;

    AoapTestTimer mAoapTestTimer;

    /* eventfd added in usbMonitorThread to polling. */
    int32_t mEventFd;

    bool startUdevMonitor();

    int32_t getDevPath(struct udev_device* inUdevDevice);
    int32_t verifyUdevDevice(struct udev_device* inUdevDevice, myDevice& outDevice);
    static void* aoapMonitorThread(void* p);
    pthread_t mMonitorThreadId;

    struct udev *mpUdev;
    struct udev_monitor *mpMonitor;
    int32_t mUdevMonitorFd;

    bool mShutdown;

    std::deque<myDevice*> mDeviceList;

    int32_t switchDevice(myDevice& inDevice);
    int32_t releaseDevice(uint32_t inVendorId, uint32_t inProductId, std::string inSerial);

    static void connectCallback(int32_t accessoryId, int32_t deviceId, int32_t result, void *pToken,
                                uint32_t audioSupport);
    int32_t mSwitchResult;
    sem_t   mSwitchSyncSem;


    std::deque<uint32_t> mBlackList;
};


#endif /* TEST_AOAP_TEST_H_ */
