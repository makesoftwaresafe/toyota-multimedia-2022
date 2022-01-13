/**
* \file: AoapDiscoveryTest.h
*
* \version: 0.1
*
* \release: $Name:$
*
* Includes the necessary header file to test the aoap-discovery.
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


#ifndef TEST_AOAP_DISCOVERY_TEST_H_
#define TEST_AOAP_DISCOVERY_TEST_H_


/* *************  includes  ************* */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <signal.h>
#include <deque>

#include <aoap_types.h>
#include <aoap.h>

#include <spi_usb_discoverer.h>
#include <spi_usb_discoverer_types.h>

#include "SignalHandler.h"


/* *************  defines  ************* */

#define ACCESSORY_MANUFACTURER_NAME     "Android"
#define ACCESSORY_MODEL_NAME            "Android Auto"
#define ACCESSORY_DESCRIPTION           "Android Auto"
#define ACCESSORY_VERSION               "1.0"
#define ACCESSORY_URI                   "http://www.android.com/auto"
#define ACCESSORY_SERIAL_NUMBER         "000000001234567"

typedef enum TddDeviceState
{
    DEVICE_STATE_IDLE,        //!< DEVICE_STATE_IDLE
    DEVICE_STATE_SWITCHING,   //!< DEVICE_STATE_SWITCHING
    DEVICE_STATE_ACCESSORY,   //!< DEVICE_STATE_ACCESSORY
    DEVICE_STATE_SWITCH_FAIL, //!< DEVICE_STATE_SWITCH_FAIL
    DEVICE_STATE_UNKNOWN      //!< DEVICE_STATE_UNKNOWN
//!< DEVICE_STATE_UNKNOWN
} tTddDeviceState;

enum class TddTestState : int32_t
{
    IDLE            = 1 << 0,
    DEV_FOUND       = 1 << 1,
    DEV_SWITCHING   = 1 << 2,
    DEV_LOST        = 1 << 3,
    DEV_ACC_MODE    = 1 << 4,
    DEV_RESET       = 1 << 5,
    DONE            = (IDLE | DEV_FOUND | DEV_LOST | DEV_SWITCHING | DEV_ACC_MODE | DEV_RESET),
    END             = 0xfff
};

enum class TddTestError : int32_t
{
    ERR_PASS            = 1 << 0,
    ERR_SETUP           = 1 << 1,
    ERR_TIMER_CREATE    = 1 << 2,
    ERR_TIMER_START     = 1 << 3,
    ERR_TIMER_TMO       = 1 << 4,
    ERR_STATE           = 1 << 5,
    ERR_FAIL            = 1 << 6,
    ERR_SIGNAL          = 1 << 7
//!< ERR
};


/* *************  functions  ************* */

using std::string;


class TddDevice {
public:
    TddDevice(const t_usbDeviceInformation& inDevInfo);
    virtual ~TddDevice();

    void setDeviceState(tTddDeviceState inState) { mCurrState = inState; };
    tTddDeviceState getDeviceState(void) { return mCurrState; };

    int switchDevice();
    int disconnectDevice();
    static void aoap_device_detection_connectCB(int accessoryId, int deviceId, int result, void *pToken, unsigned int audioSupport);

    t_usbDeviceInformation mDevInfo;
private:
    int mAccId;
    int mDevId;

    tTddDeviceState mCurrState;
}; // class TddDevice

class Tdd : public SignalHandler {
public:
    Tdd();
    virtual ~Tdd();

    bool start();
    void stop();

    void onDeviceFound(t_usbDeviceInformation& inDevice);
    void onDeviceLost(t_usbDeviceInformation& inDevice);
    void onDeviceSwitched(t_usbDeviceInformation& inDevice);

    void addDevice(TddDevice* inTddDevice) { dqTddDevices.push_back(inTddDevice); };
    void rmDevice(TddDevice* inTddDevice);
    void switchDevice(TddDevice* inTddDevice);
    void resetDevice(TddDevice* inTddDevice);
    TddDevice* findDevice(t_usbDeviceInformation& inDevice);

    static void notifyDeviceFound_cb(void *pContext, t_usbDeviceInformation* pDevice, int result);
    static void notifyDeviceRemoved_cb(void *pContext, t_usbDeviceInformation* pDevice, int result);
    static void notifyDeviceFoundAccessoryMode_cb(void *pContext, t_usbDeviceInformation* pDevice, int result);
    static void notifyDeviceCheckAoapSupport_cb(void *pContext, t_usbDeviceInformation* pDevice, bool* outAllowCheck);

    virtual void signaling(void);

    void setTestState(TddTestState inTestState);
    int32_t getTestState(void);
    std::string strTestState(void);

    void setTestError(TddTestError inTestError);
    int32_t getTestError(void);
    std::string strTestError(void);
private:
    int32_t mTestState;
    int32_t mTestError;

    AOAP::UsbDiscoverer* mDiscoverer;

    std::deque<TddDevice*> dqTddDevices;

    bool mRunning;
    size_t mMaxDevices;
}; // class Tdd


#endif /* TEST_AOAP_DISCOVERY_TEST_H_ */
