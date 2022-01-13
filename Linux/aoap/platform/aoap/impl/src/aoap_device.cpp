/*
 * aoap_device.cpp
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_device.h"
#include "aoap_accessory.h"
#include "aoap.h"
#include "aoap_tools.h"
#include "aoap_usbmonitor.h"
#include "aoap_deviceinfo.h"
#include "aoap_logging.h"
#include <cstring>

using namespace AOAP::Logging;

const int AOAP_MAJOR_VERSION_INDEX = 0;
const int AOAP_MINOR_VERSION_INDEX = 1;
const int AOAP_PROTOCOL_VERSION_SIZE = 2;

/*static*/unsigned int Device::gCounter = 0;

/*static*/bool Device::gPerformanceMeasurements = false;

Device::Device(unsigned int vendorId, unsigned int productId, const std::string &serial)
: AOAP::Time::Timer()
, mVendorId(vendorId)
, mProductId(productId)
, mSerial(serial)
, mDevNum(0)
, mPrevDevNum(0)
, mDevNumSet(false)
, mId(++gCounter)
, mpDeviceHandle(NULL)
, mpAccessory(NULL)
, mState(AOAP_DEVICE_STATE_IDLE)
, mAccessoryEndpointIn()
, mAccessoryEndpointOut()
, mEnableAudio(false)
, mAudioEnabled(false)
, mpConnectCb(NULL)
, mpToken(NULL)
, mInterfaceNumber(-1)
, mAoapSupportQueried(false)
, mMajorAoapVersion(0)
, mMinorAoapVersion(0)
, mClaimedInterfaceNumber(-1)
, mMaxPrintBufferSize(128)
, mpUsbContext(NULL)
, mIOErrCounter(0)
, mReadIOErrCounter(0)
{
    init();
}

Device::Device(unsigned int vendorId, unsigned int productId, const std::string &serial,
        unsigned int devNum)
: AOAP::Time::Timer()
, mVendorId(vendorId)
, mProductId(productId)
, mSerial(serial)
, mDevNum(devNum)
, mPrevDevNum(0)
, mDevNumSet(true)
, mId(++gCounter)
, mpDeviceHandle(NULL)
, mpAccessory(NULL)
, mState(AOAP_DEVICE_STATE_IDLE)
, mAccessoryEndpointIn()
, mAccessoryEndpointOut()
, mEnableAudio(false)
, mAudioEnabled(false)
, mpConnectCb(NULL)
, mpToken(NULL)
, mInterfaceNumber(-1)
, mAoapSupportQueried(false)
, mMajorAoapVersion(0)
, mMinorAoapVersion(0)
, mClaimedInterfaceNumber(-1)
, mMaxPrintBufferSize(128)
, mpUsbContext(NULL)
, mIOErrCounter(0)
, mReadIOErrCounter(0)
{
    init();
}

Device::~Device()
{
    dbgPrintLine(eLogDebug, "%s() %.4x:%.4x, s/n='%s', gCounter=%d, id=%d",
            __FUNCTION__, mVendorId, mProductId, mSerial.c_str(), gCounter, getId());

    if ((mIOErrCounter > 0) || (mReadIOErrCounter > 0))
    {
        dbgPrintLine(eLogWarn, "%s() WARNING: Total I/O Errors during read = %d / write = %d",
                    __FUNCTION__, mReadIOErrCounter, mIOErrCounter);
    }

    /* stop timer before releasing the memory of Device::timeoutHandler */
    AOAP::Time::Timer::stop();

    if (isOpen())
    {
        int result = libusb_release_interface(mpDeviceHandle, mInterfaceNumber);
        dbgPrintLine(eLogDebug, "%s() %.4x:%.4x, s/n='%s', id=%d => interface (%d) released with result=%d",
                __FUNCTION__, mVendorId, mProductId, mSerial.c_str(), getId(), mInterfaceNumber, result);
        close(true);
    }

    if (mpUsbContext)
    {
        libusb_exit(mpUsbContext);
        mpUsbContext = NULL;
    }
    else
    {
        dbgPrintLine(eLogWarn, "%s() WARNING: libusb already deinitialized (USB context invalid)", __FUNCTION__);
    }

    /* Do not decrement gCounter.
     * Otherwise the same mId could be assigned to different Device */
}


void Device::retrieveDevNum(void)
{
    int devNum = DeviceInfo::getInstance()->getDevNum(getVendorId(), getProductId(), getSerial());
    if (devNum < 0)
    {
        dbgPrintLine(eLogError,
                "%s() WARNING: Couldn't retrieve devNum with result %s (%d) -> has to be updated later",
                __FUNCTION__, aoap_get_result_as_string(devNum), devNum);
        mDevNumSet = false;
        mDevNum = 0;
    }
    else
    {
        if (mDevNumSet) {
            dbgPrintLine(eLogDebug, "%s() update devNum=%d from %d", __FUNCTION__, devNum, mDevNum);
            mPrevDevNum = mDevNum;
        }
        else {
            dbgPrintLine(eLogDebug, "%s() set devNum=%d", __FUNCTION__, devNum);
        }

        mDevNumSet = true;
        mDevNum = static_cast<unsigned int>(devNum);
    }
    /* Note:
     * Can be called here as long as retrieveDevNum() is called
     * while it is protected by DeviceList::getLock().
     */
    DeviceInfo::deleteInstance();
}

void Device::setAccessory(Accessory *pAccessory)
{
    mpAccessory = pAccessory;
}

int Device::getAccessoryId(void) const
{
    if (mpAccessory)
    {
        return mpAccessory->getId();
    }
    else
    {
        return AOAP_ERROR_ACCESSORY_NOT_SET;
    }
}

int Device::open(void)
{
    int result = AOAP_SUCCESS;
    /* TODO:    How often should we retry? 3 or 5 times?
     *          5 times means we are sleeping in worst case 500ms. Too much? */
    bool bFound = false;
    bool bBreak = false;
    int i = 0;

    libusb_device **pDevs = NULL;
    libusb_device *usb_dev = NULL;
    int num_devs = 0;

    if (!mpUsbContext)
    {
        dbgPrintLine(eLogFatal, "%s() FATAL ERROR: USB context is not retrieved", __FUNCTION__);
        return AOAP_ERROR_GENERAL;
    }

    if (isOpen())
    {
        dbgPrintLine(eLogDebug, "%s() device handle is already there -> device already open", __FUNCTION__);
        return AOAP_SUCCESS;
    }

    num_devs = (int)libusb_get_device_list(mpUsbContext, &pDevs);
    dbgPrintLine(eLogDebug, "%s() num usb devices = %d. Find and open device %.4x:%.4x,s/n='%s'",
            __FUNCTION__, num_devs, mVendorId, mProductId, mSerial.c_str());

    // iterate through all available devices to find the respective device
    while ((i < num_devs) && ((usb_dev = pDevs[i++]) != nullptr) && (true != bFound))
    {
        struct libusb_device_descriptor desc;
        result = libusb_get_device_descriptor(usb_dev, &desc);
        // open device only if vendorId and productId are matching
        if ( (LIBUSB_SUCCESS > result) || ((mVendorId != desc.idVendor) || (mProductId != desc.idProduct)) )
        {
            if (LIBUSB_SUCCESS > result)
                dbgPrintLine(eLogError, "%s() ERROR: Failed to get device descriptor with code=%s",
                             __FUNCTION__, aoap_get_result_as_string(result));
            else
                dbgPrintLine(eLogDebug, "%s() Ignoring device %.4x:%.4x because vendor/product id do not match",
                             __FUNCTION__, desc.idVendor, desc.idProduct);

            // start next iteration of while()
            continue;
        }

        // (re)try to get serial number of the device
        unsigned int retries = 5;
        do
        {
            dbgPrintLine(eLogVerbose, "%s() before open", __FUNCTION__);
            // open the device and get libusb device handle
            int openResult = libusb_open(usb_dev, &mpDeviceHandle);
            if ((LIBUSB_SUCCESS == openResult) && mpDeviceHandle)
            {
                if (desc.iSerialNumber > 0)
                {
                    // sleep 0.1 sec before retrieving serial number
                    usleep(100000);

                    char retrievedSerial[64];
                    // retrieve the serial number from the libusb device
                    int len = libusb_get_string_descriptor_ascii(mpDeviceHandle,
                                                                 desc.iSerialNumber,
                                                                 (unsigned char*) retrievedSerial,
                                                                 sizeof(retrievedSerial));
                    // compare the serial numbers to verify if we found the correct device
                    if ((len > 0) && (0 == std::strcmp(mSerial.c_str(), retrievedSerial)))
                    {
                        dbgPrintLine(eLogInfo,
                                     "%s() Device %.4x:%.4x s/n='%s',len=%d found -> gets opened %p",
                                     __FUNCTION__, desc.idVendor, desc.idProduct, retrievedSerial, len, mpDeviceHandle);
                        result = AOAP_SUCCESS;
                        bFound = true;
                    }
                    else
                    {
                        if (0 >= len)
                        {
                            // could not retrieve serial number - retry
                            retries--;
                            dbgPrintLine(eLogError,
                                         "%s() ERROR: Device %.4x:%.4x Failed to retrieve serial with rc=%d. retries left =%d",
                                         __FUNCTION__, desc.idVendor, desc.idProduct, len, retries);
                        }
                        else
                        {
                            dbgPrintLine(eLogInfo,
                                         "%s() Device %.4x:%.4x s/n='%s',len=%d found but serial does not match to s/n='%s'",
                                         __FUNCTION__, desc.idVendor, desc.idProduct, retrievedSerial, len, mSerial.c_str());
                            // leave do-while loop to start next iteration
                            bBreak = true;
                        }
                    }
                }
                else
                {
                    dbgPrintLine(eLogError, "%s() ERROR: USB device has no serial", __FUNCTION__);
                    // leave do-while loop to start next iteration
                    bBreak = true;
                }

                // did not found our device but libusb was opened, close it
                if (true != bFound)
                    close();
            }
            else
            {
                dbgPrintLine(eLogError, "%s() ERROR: Couldn't open device (error code=%d)",
                             __FUNCTION__, openResult);
                // leave do-while loop to start next iteration
                bBreak = true;
            }
        } while ((true != bFound) && (true != bBreak) && (retries > 0));
    } // while-loop

    //free and unreference all devices
    libusb_free_device_list(pDevs, 1);

    if (!mpDeviceHandle)
    {
        dbgPrintLine(eLogError,
                "%s() ERROR: Couldn't find device with %.4x:%.4x,s/n='%s'",
                __FUNCTION__, mVendorId, mProductId, mSerial.c_str());
        result = AOAP_ERROR_NO_DEVICE;
    }
    else
    {
        dbgPrintLine(eLogDebug, "%s() device handle is %p", __FUNCTION__, mpDeviceHandle);
    }

    return result;
}

void Device::close(bool usbClose /*= true*/)
{
    if (isOpen())
    {
        dbgPrintLine(eLogInfo, "%s() device %.4x:%.4x handle=%p",
                __FUNCTION__, mVendorId, mProductId, mpDeviceHandle);

        //This is a workaround because libusb_close is blocking in some cases
        //  when the device is already disconnected. In this case there will be a warning given when calling libusb_exit that
        //  some resources are not released correctly. This has to be accepted
        if (usbClose)
        {
            libusb_close(mpDeviceHandle);
        }
        mpDeviceHandle = NULL;

        dbgPrintLine(eLogDebug, "%s() device %.4x:%.4x closed",
                __FUNCTION__, mVendorId, mProductId);
    }
    else
    {
        dbgPrintLine(eLogWarn, "%s() WARNING: device %.4x:%.4x isn't open",
                __FUNCTION__, mVendorId, mProductId);
    }
}

int Device::connect(aoap_connectCB pConnectCb, bool audioSupport, void *pToken, unsigned int seconds, unsigned int ctrlReqTmoMs)
{
    mEnableAudio = audioSupport;
    mpConnectCb = pConnectCb;
    mpToken = pToken;

    return switchToAccessory(seconds, ctrlReqTmoMs);
}

int Device::read(unsigned char *pBuffer, int bufferSize, unsigned int timeout)
{
    int result = AOAP_SUCCESS;
    int readSize = bufferSize;

    struct timeval startTime = {0, 0};
    struct timeval endTime = {0, 0};

    if (gPerformanceMeasurements)
    {
        gettimeofday(&startTime, NULL);
    }

    //Open device first (if not yet opened)
    if (!isOpen())
    {
        result = open();
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError,
                    "%s(size=%d, timeout=%d) ERROR: Failed to open device with code=%s (%d)",
                    __FUNCTION__, bufferSize, timeout, aoap_get_result_as_string(result), result);
            return result;
        }
    }

    //Claim interface if not yet done in order to get ownership of the interface
    if (mClaimedInterfaceNumber < 0)
    {
        result = claimInterface(mInterfaceNumber);
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Claiming interface failed",
                    __FUNCTION__, bufferSize, timeout);
            return result;
        }
    }

    //To avoid buffer overflows reduce the buffer size to the next smaller multiple of
    // endpoint's max packet size
    if (bufferSize > mAccessoryEndpointIn.maxPacketSize)
    {
        readSize = bufferSize - (bufferSize % mAccessoryEndpointIn.maxPacketSize);
        //dbgPrintLine(eLogVerbose, "read() reduce buffer size from %d to %d", bufferSize, readSize);
    }

    // workaround: on simu reading more than 16k can lead to loosing data
    // see http://jira.adit-jv.com/browse/SWGIII-3142
#ifdef AOAP_X86SIM
    if (readSize > 16*1024)
    {
        readSize = 16*1024;
    }
#endif

    //Read once data
    int transferred = 0;
    result = libusb_bulk_transfer(mpDeviceHandle, mAccessoryEndpointIn.address, pBuffer,
            readSize, &transferred, timeout);
    if (LIBUSB_SUCCESS == result)
    {
        if (gPerformanceMeasurements)
        {
            gettimeofday(&endTime, NULL);
            dbgPrintLine(eLogInfo, "%s(size=%d, timeout=%d) %d bytes received in %ldus",
                    __FUNCTION__, bufferSize, timeout, transferred,
                    ((endTime.tv_sec * 1000000 + endTime.tv_usec)
                            - (startTime.tv_sec *1000000 + startTime.tv_usec)));
        }
        else
        {
            dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) %d bytes received",
                    __FUNCTION__, bufferSize, timeout, transferred);
        }
        printBytes(pBuffer, transferred);

        return transferred;
    }
    else if (LIBUSB_ERROR_TIMEOUT == result)
    {
        dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) Time out, only %d bytes received",
                __FUNCTION__, bufferSize, timeout, transferred);
        if (transferred > 0)
        {
            printBytes(pBuffer, transferred);
        }
        return transferred;
    }
    else if (LIBUSB_ERROR_IO == result)
    {
        mReadIOErrCounter++;
        /* sleep 10 ms to avoid high CPU load in case error occur to often */
        usleep(10000);
        return transferred;
    }
    else
    {
        dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: libusb(%p) returned '%s' (%d)",
                __FUNCTION__, bufferSize, timeout, mpDeviceHandle, aoap_get_result_as_string(result), result);
        return result;
    }
}

int Device::read(unsigned char *pBuffer, int bufferSize, unsigned int *pTransferred, unsigned int timeout)
{
    int result = AOAP_SUCCESS;
    int readSize = bufferSize;

    struct timeval startTime = {0, 0};
    struct timeval endTime = {0, 0};

    if (gPerformanceMeasurements)
    {
        gettimeofday(&startTime, NULL);
    }

    //Open device first (if not yet opened)
    if (!isOpen())
    {
        result = open();
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError,
                    "%s(size=%d, timeout=%d) ERROR: Failed to open device with code=%s (%d)",
                    __FUNCTION__, bufferSize, timeout, aoap_get_result_as_string(result), result);
            return result;
        }
    }

    //Claim interface if not yet done in order to get ownership of the interface
    if (mClaimedInterfaceNumber < 0)
    {
        result = claimInterface(mInterfaceNumber);
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Claiming interface failed",
                    __FUNCTION__, bufferSize, timeout);
            return result;
        }
    }

    //To avoid buffer overflows reduce the buffer size to the next smaller multiple of
    // endpoint's max packet size
    if (bufferSize > mAccessoryEndpointIn.maxPacketSize)
    {
        readSize = bufferSize - (bufferSize % mAccessoryEndpointIn.maxPacketSize);
        //dbgPrintLine(eLogVerbose, "read() reduce buffer size from %d to %d", bufferSize, readSize);
    }

    // workaround: on simu reading more than 16k can lead to loosing data
    // see http://jira.adit-jv.com/browse/SWGIII-3142
#ifdef AOAP_X86SIM
    if (readSize > 16*1024)
    {
        readSize = 16*1024;
    }
#endif

    //Read once data
    int transferred = 0;
    result = libusb_bulk_transfer(mpDeviceHandle, mAccessoryEndpointIn.address, pBuffer,
            readSize, &transferred, timeout);
    if (LIBUSB_SUCCESS == result)
    {
        if (gPerformanceMeasurements)
        {
            gettimeofday(&endTime, NULL);
            dbgPrintLine(eLogInfo, "%s(size=%d, timeout=%d) %d bytes received in %ldus",
                    __FUNCTION__, bufferSize, timeout, transferred,
                    ((endTime.tv_sec * 1000000 + endTime.tv_usec)
                            - (startTime.tv_sec *1000000 + startTime.tv_usec)));
        }
        else
        {
            dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) %d bytes received",
                    __FUNCTION__, bufferSize, timeout, transferred);
        }
        printBytes(pBuffer, transferred);

    }
    else if (LIBUSB_ERROR_TIMEOUT == result)
    {
        dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) Time out, only %d bytes received",
                __FUNCTION__, bufferSize, timeout, transferred);
        if (transferred > 0)
        {
            printBytes(pBuffer, transferred);
        }
    }
    else if (LIBUSB_ERROR_IO == result)
    {
        mReadIOErrCounter++;
        /* sleep 10 ms to avoid high CPU load in case error occur to often */
        usleep(10000);
    }
    else
    {
        dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: libusb(%p) returned '%s' (%d)",
                __FUNCTION__, bufferSize, timeout, mpDeviceHandle, aoap_get_result_as_string(result), result);
    }

    *pTransferred = transferred;

    return result;
}

int Device::write(const unsigned char *pBuffer, int bufferSize,
            unsigned int timeout)
{
    int result = AOAP_SUCCESS;
    struct timeval startTime = {0, 0};
    struct timeval endTime = {0, 0};

    if (gPerformanceMeasurements)
    {
        gettimeofday(&startTime, NULL);
    }

    //Open device first (if not yet opened)
    if (!isOpen())
    {
        result = open();
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Failed to open device with code=%s (%d)",
                    __FUNCTION__, bufferSize, timeout, aoap_get_result_as_string(result), result);
            return result;
        }
    }

    //Claim interface if not yet done in order to get ownership of the interface
    if (mClaimedInterfaceNumber < 0)
    {
        result = claimInterface(mInterfaceNumber);
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Claiming interface failed",
                    __FUNCTION__, bufferSize, timeout);
            return result;
        }
    }

    dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) waiting for lock",
            __FUNCTION__, bufferSize, timeout);

    //Logging all
    //printBytes(pBuffer, bufferSize);

    //Acquire lock. lock_guard is destructed and the mutex is released when we leave the function
    std::lock_guard<std::mutex> lockGuard(mWriteMutex);

    //Write date till all data is written
    int transferredUntilNow = 0;
    int nextChunkSize = 0;
    int lastTransfer = 0; //bytes transferred in the last bulk transfer
    while (bufferSize > transferredUntilNow)
    {
        //Determine next chunk size
        if (bufferSize - transferredUntilNow <= mAccessoryEndpointOut.maxPacketSize)
        {
            nextChunkSize = bufferSize - transferredUntilNow;
        }
        else
        {
            //Make sure that the chunk size is a multiple of max packet size
            nextChunkSize = bufferSize - (bufferSize % mAccessoryEndpointOut.maxPacketSize);
        }

        //Check again if device is still open because getting mutex might take several seconds
        if (!isOpen())
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Device isn't open anymore -> abort write",
                    __FUNCTION__, bufferSize, timeout);
            result = AOAP_ERROR_NO_DEVICE;
            break;
        }

        dbgPrintLine(eLogVerbose, "%s() writing %d/%d bytes, already transferred=%d",
                __FUNCTION__, nextChunkSize, bufferSize, transferredUntilNow);

        printBytes(pBuffer+transferredUntilNow, nextChunkSize);

        lastTransfer = 0;
        result = libusb_bulk_transfer(mpDeviceHandle, mAccessoryEndpointOut.address,
                const_cast<unsigned char*>(pBuffer)+transferredUntilNow,
                nextChunkSize, &lastTransfer, timeout);

        if (LIBUSB_SUCCESS == result)
        {
            transferredUntilNow += lastTransfer;
            result = transferredUntilNow;
        }
        else if (LIBUSB_ERROR_TIMEOUT == result)
        {
            dbgPrintLine(eLogWarn,
                         "%s() Warning: Time out, %d bytes send", __FUNCTION__, lastTransfer);
            transferredUntilNow += lastTransfer;

            return transferredUntilNow;
            break;
        }
        else if (LIBUSB_ERROR_IO == result)
        {
            mIOErrCounter++;

            transferredUntilNow += lastTransfer;
            result = transferredUntilNow;
            /* sleep 10 ms to avoid high CPU load in case error occur to often */
            usleep(10000);
            break;
        }
        else
        {
            dbgPrintLine(eLogError, "%s() ERROR:: libusb returned '%s' (%d)",
                    __FUNCTION__, aoap_get_result_as_string(result), result);
            break;
        }
    }

    if (gPerformanceMeasurements)
    {
        gettimeofday(&endTime, NULL);
        dbgPrintLine(eLogInfo, "%s(size=%d, timeout=%d) %d bytes written in %ldus",
                __FUNCTION__, bufferSize, timeout, transferredUntilNow,
                ((endTime.tv_sec * 1000000 + endTime.tv_usec)
                        - (startTime.tv_sec *1000000 + startTime.tv_usec)));
    }
    else
    {
        dbgPrintLine(eLogVerbose, "%s() all %d bytes send", __FUNCTION__, transferredUntilNow);
    }

    if (result >= LIBUSB_SUCCESS)
    {
        result = transferredUntilNow;
    }

    return result;
}


int Device::write(const unsigned char *pBuffer, int bufferSize,
                  unsigned int *pTransferred, unsigned int timeout)
{
    int result = AOAP_SUCCESS;
    struct timeval startTime = {0, 0};
    struct timeval endTime = {0, 0};

    if (gPerformanceMeasurements)
    {
        gettimeofday(&startTime, NULL);
    }

    //Open device first (if not yet opened)
    if (!isOpen())
    {
        result = open();
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Failed to open device with code=%s (%d)",
                    __FUNCTION__, bufferSize, timeout, aoap_get_result_as_string(result), result);
            return result;
        }
    }

    //Claim interface if not yet done in order to get ownership of the interface
    if (mClaimedInterfaceNumber < 0)
    {
        result = claimInterface(mInterfaceNumber);
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Claiming interface failed",
                    __FUNCTION__, bufferSize, timeout);
            return result;
        }
    }

    dbgPrintLine(eLogVerbose, "%s(size=%d, timeout=%d) waiting for lock",
            __FUNCTION__, bufferSize, timeout);

    //Logging all
    //printBytes(pBuffer, bufferSize);

    //Acquire lock. lock_guard is destructed and the mutex is released when we leave the function
    std::lock_guard<std::mutex> lockGuard(mWriteMutex);

    //Write date till all data is written
    int transferredUntilNow = 0;
    int nextChunkSize = 0;
    int lastTransfer = 0; //bytes transferred in the last bulk transfer
    while (bufferSize > transferredUntilNow)
    {
        //Determine next chunk size
        if (bufferSize - transferredUntilNow <= mAccessoryEndpointOut.maxPacketSize)
        {
            nextChunkSize = bufferSize - transferredUntilNow;
        }
        else
        {
            //Make sure that the chunk size is a multiple of max packet size
            nextChunkSize = bufferSize - (bufferSize % mAccessoryEndpointOut.maxPacketSize);
        }

        //Check again if device is still open because getting mutex might take several seconds
        if (!isOpen())
        {
            dbgPrintLine(eLogError, "%s(size=%d, timeout=%d) ERROR: Device isn't open anymore -> abort write",
                    __FUNCTION__, bufferSize, timeout);
            result = AOAP_ERROR_NO_DEVICE;
            break;
        }

        dbgPrintLine(eLogVerbose, "%s() writing %d/%d bytes, already transferred=%d",
                __FUNCTION__, nextChunkSize, bufferSize, transferredUntilNow);

        printBytes(pBuffer+transferredUntilNow, nextChunkSize);

        lastTransfer = 0;
        result = libusb_bulk_transfer(mpDeviceHandle, mAccessoryEndpointOut.address,
                const_cast<unsigned char*>(pBuffer)+transferredUntilNow,
                nextChunkSize, &lastTransfer, timeout);

        if (LIBUSB_SUCCESS == result)
        {
            transferredUntilNow += lastTransfer;
        }
        else if (LIBUSB_ERROR_TIMEOUT == result)
        {
            dbgPrintLine(eLogWarn,
                         "%s() Warning: Time out, %d bytes send", __FUNCTION__, lastTransfer);
            transferredUntilNow += lastTransfer;

            break;
        }
        else if (LIBUSB_ERROR_IO == result)
        {
            mIOErrCounter++;

            transferredUntilNow += lastTransfer;
            /* sleep 10 ms to avoid high CPU load in case error occur to often */
            usleep(10000);
            break;
        }
        else
        {
            dbgPrintLine(eLogError, "%s() ERROR:: libusb returned '%s' (%d)",
                    __FUNCTION__, aoap_get_result_as_string(result), result);
            break;
        }
    }

    if (gPerformanceMeasurements)
    {
        gettimeofday(&endTime, NULL);
        dbgPrintLine(eLogInfo, "%s(size=%d, timeout=%d) %d bytes written in %ldus",
                __FUNCTION__, bufferSize, timeout, transferredUntilNow,
                ((endTime.tv_sec * 1000000 + endTime.tv_usec)
                        - (startTime.tv_sec *1000000 + startTime.tv_usec)));
    }
    else
    {
        dbgPrintLine(eLogVerbose, "%s() all %d bytes send", __FUNCTION__, transferredUntilNow);
    }

    *pTransferred = transferredUntilNow;

    return result;
}

int Device::checkAoapSupport(unsigned int &majorVersion, unsigned int &minorVersion)
{
    //Get protocol version
    if (!mAoapSupportQueried)
    {
        dbgPrintLine(eLogDebug, "%s() not yet queried -> query now", __FUNCTION__);
        uint8_t protocol_buffer[AOAP_PROTOCOL_VERSION_SIZE];
        int result = getAoapProtocolVersion(protocol_buffer,
                sizeof(protocol_buffer), 5000); //5 second timeout

        if (result >= AOAP_SUCCESS)
        {
            mAoapSupportQueried = true;
            mMajorAoapVersion = protocol_buffer[AOAP_MAJOR_VERSION_INDEX];
            mMinorAoapVersion = protocol_buffer[AOAP_MINOR_VERSION_INDEX];
            majorVersion = mMajorAoapVersion;
            minorVersion = mMinorAoapVersion;

            dbgPrintLine(eLogInfo, "%s() device supports %d.%d",
                    __FUNCTION__, mMajorAoapVersion, mMinorAoapVersion);

            return AOAP_SUCCESS;
        }
        else if (AOAP_ERROR_PIPE == result)
        {
            //A pipe error means that the USB device does not support AOAP (refer
            // to documentation of libusb_control_transfer) => return AOAP_SUCCESS
            mAoapSupportQueried = true;
            mMajorAoapVersion = 0;
            mMinorAoapVersion = 0;
            majorVersion = mMajorAoapVersion;
            minorVersion = mMinorAoapVersion;

            dbgPrintLine(eLogInfo, "%s() device does not support AOAP (pipe error)",
                    __FUNCTION__, mMajorAoapVersion, mMinorAoapVersion);

            return AOAP_SUCCESS;
        }
        else
        {
            mAoapSupportQueried = true;
            mMajorAoapVersion = 0;
            mMinorAoapVersion = 0;
            majorVersion = mMajorAoapVersion;
            minorVersion = mMinorAoapVersion;

            dbgPrintLine(eLogInfo, "%s() device does not support AOAP",
                    __FUNCTION__, mMajorAoapVersion, mMinorAoapVersion);

            return result;
        }
    }
    else
    {
        majorVersion = mMajorAoapVersion;
        minorVersion = mMinorAoapVersion;
        dbgPrintLine(eLogDebug, "%s() AOAP support already queried with result %d.%d",
                __FUNCTION__, mMajorAoapVersion, mMinorAoapVersion);

        return AOAP_SUCCESS;
    }
}

void Device::finishSwitching(void)
{
    int result = AOAP_SUCCESS;
    /* set max retries to open the libusb device to 5 */
    unsigned int retries = 5;
    /* Check if accessory is set */
    if (!mpAccessory)
    {
        dbgPrintLine(eLogError, "%s() ERROR: Accessory not set", __FUNCTION__);
        return;
    }

    if (AOAP::Tools::isAccessory(getVendorId(), getProductId()))
    {
        dbgPrintLine(eLogDebug, "%s() Device is in accessory mode -> find endpoints", __FUNCTION__);
        do
        {
            result = findEndpoints();
            if (AOAP_SUCCESS == result)
            {
                setState(AOAP_DEVICE_STATE_ACCESSORY);
            }
            else if (AOAP_ERROR_NO_DEVICE == result)
            {
                /* Assuming we are triggered by udev with an attach event, the device must be available.
                 * However, we know that the libusb 1.0.19 has deficiency to enumerate newly devices.
                 * Retry mechanism is recommend. */
                retries--;
                dbgPrintLine(eLogWarn,
                             "%s() Could not find device, retries left =%d", __FUNCTION__, retries);
                usleep(200000);
            }
            else
            {
                dbgPrintLine(eLogError,
                        "%s() ERROR: Failed to find endpoints in accessory mode", __FUNCTION__);

                setState(AOAP_DEVICE_STATE_UNKNOWN);
                result = AOAP_ERROR_ERROR_NO_ENDPOINTS;
            }
        } while ((retries > 0) && (result == AOAP_ERROR_NO_DEVICE));

        /* Cancel connect timeout */
        stop();
        /* trigger callback */
        if (mpConnectCb)
        {
            mpConnectCb(mpAccessory->getId(), getId(), result, mpToken, mAudioEnabled ? 1 : 0);
        }
    }
    else
    {
        /* Cancel connect timeout */
        stop();
        if (mpConnectCb)
        {
            dbgPrintLine(eLogError, "%s() ERROR: Device got removed by user", __FUNCTION__);
            mpConnectCb(mpAccessory->getId(), getId(), AOAP_ERROR_INTERRUPTED, mpToken,
                    mAudioEnabled ? 1 : 0);
            setState(AOAP_DEVICE_STATE_IDLE);
        }
    }
}

void Device::timeoutHandler(void)
{
    struct timeval end;
    gettimeofday(&end, NULL);

    if (AOAP_DEVICE_STATE_SWITCHING == getState())
    {
        if (mpConnectCb)
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Timer elapsed=%1d us and connection is not established",
                    __FUNCTION__,
                                ((end.tv_sec * 1000000 + end.tv_usec) - (mStartTime.tv_sec * 1000000 + mStartTime.tv_usec)));

            mpConnectCb(mpAccessory->getId(), getId(), AOAP_ERROR_CONNECT_TIMEOUT,
                    mpToken, mAudioEnabled ? 1 : 0);
            setState(AOAP_DEVICE_STATE_UNKNOWN);
        }
        else
        {
            dbgPrintLine(eLogWarn,
                    "%s() WARNING: Connect CB is NULL. Maybe not set?", __FUNCTION__);
        }
    }
    else
    {
        dbgPrintLine(eLogWarn, "%s() WARNING: Called in wrong state -> ignored", __FUNCTION__);
    }
}

/*static*/ void Device::enablePerformanceMeasurements(void)
{
    gPerformanceMeasurements = true;
}

void Device::init(void)
{
    if (!mpUsbContext)
    {
        int result = libusb_init(&mpUsbContext);
        if (LIBUSB_SUCCESS == result)
        {
            if (!AOAP::Logging::getDltLogging()
                && (AOAP::Logging::getLogLevel() > static_cast<int>(AOAP::Logging::eLogInfo)))
            {
                libusb_set_debug(mpUsbContext, 3);
            }
        }
        else
        {
            dbgPrintLine(eLogError,
                        "%s() Failed to initialize libusb with result=%d",
                        __FUNCTION__, result);
            // make sure we are not using the libusb context
            mpUsbContext = NULL;
        }
    }

    if (getDevNumSet())
    {
        dbgPrintLine(eLogDebug, "%s() %.4x:%.4x, s/n='%s', devNum=%d, id=%d",
                __FUNCTION__, mVendorId, mProductId, mSerial.c_str(), mDevNum, mId);
    }
    else
    {
        dbgPrintLine(eLogDebug, "%s() %.4x:%.4x, s/n='%s', devNum=n/a, id=%d",
                __FUNCTION__, mVendorId, mProductId, mSerial.c_str(), mId);
    }

    mAccessoryEndpointIn.address = 0;
    mAccessoryEndpointIn.maxPacketSize = -1;

    mAccessoryEndpointOut.address = 0;
    mAccessoryEndpointOut.maxPacketSize = -1;

    if (AOAP::Tools::isAccessory(getVendorId(), getProductId()))
    {
        if (AOAP_SUCCESS != findEndpoints())
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: in accessory mode failed because couldn't find endpoints",
                    __FUNCTION__);
            setState(AOAP_DEVICE_STATE_UNKNOWN);
        }
        else
        {
            dbgPrintLine(eLogInfo,
                    "%s() Already in accessory mode and endpoints found -> continue",
                    __FUNCTION__);
            setState(AOAP_DEVICE_STATE_ACCESSORY);
        }
    }
}

int Device::switchToAccessory(unsigned int seconds, unsigned int inTimeoutMs)
{
    int result = AOAP_SUCCESS;
    //Check if accessory is set
    if (!mpAccessory)
    {
        dbgPrintLine(eLogError, "%s() ERROR: Accessory not set", __FUNCTION__);
        return AOAP_ERROR_ACCESSORY_NOT_SET;
    }

    //Check if we are not in accessory mode
    if (!AOAP::Tools::isAccessory(getVendorId(), getProductId()))
    {
        if ((AOAP_DEVICE_STATE_IDLE == getState())
                || (AOAP_DEVICE_STATE_UNKNOWN == getState()))
        {
            dbgPrintLine(eLogInfo,
                    "%s() Device is not in accessory mode -> switch to accessory mode",
                    __FUNCTION__);
            result = bringToAccessoryMode(inTimeoutMs);
            if (result != AOAP_SUCCESS)
            {
                dbgPrintLine(eLogError,
                        "%s() ERROR: Failed to switch to accessory mode with result=%d", __FUNCTION__, result);
                setState(AOAP_DEVICE_STATE_IDLE);
            }
            else
            {
                start(seconds); //start connect timer
                dbgPrintLine(eLogDebug,
                            "%s() Switch sent and timer started. result = %d",
                             __FUNCTION__, result);
            }
            return result;
        }
        else if (AOAP_DEVICE_STATE_SWITCHING == getState())
        {
            dbgPrintLine(eLogWarn,
                    "%s() WARNING: Device is in switching state but has not (yet) Google's accessory IDs. Did AOAP receive the attach event after user has switched to accessory mode?",
                    __FUNCTION__);
            return AOAP_SUCCESS; //treat this as OK
        }
        else
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Device has not Google's IDs and isn't in idle or switching state => do nothing",
                    __FUNCTION__);
            return AOAP_ERROR_GENERAL;
        }
    }
    else
    {
        dbgPrintLine(eLogInfo, "%s() WARNING: Already in accessory mode. Bring back to given accessory", __FUNCTION__);

        /* Android Open Accessory Protocol 1.0
         * If the device is already in accessory mode, the product ID should be 0x2D00 or 0x2D01
         * and the accessory can establish communication with the device through
         * bulk transfer endpoints using its own communication protocol
         * (the device does not need to be started in accessory mode).
         *
         * If the accessory detects an Android-powered device in accessory mode,
         * the accessory can query the device interface and endpoint descriptors
         * to obtain the bulk endpoints for communicating with the device.
         */
        /* Endpoints query at Device::init(). */

        /* set current state AOAP_DEVICE_STATE_ACCESSORY */
        setState(AOAP_DEVICE_STATE_ACCESSORY);

        return AOAP_SUCCESS;
    }
}

int Device::bringToAccessoryMode(unsigned int inTimeoutMs)
{
    int result = AOAP_SUCCESS;
    setState(AOAP_DEVICE_STATE_SWITCHING);

    //Get protocol version
    uint8_t protocol_buffer[AOAP_PROTOCOL_VERSION_SIZE];
    result = getAoapProtocolVersion(protocol_buffer, sizeof(protocol_buffer), inTimeoutMs);
    if (result < LIBUSB_SUCCESS)
    {
        return result;
    }

    if (!mpAccessory)
    {
        dbgPrintLine(eLogError, "%s() ERROR: Accessory not set", __FUNCTION__);
        return AOAP_ERROR_ACCESSORY_NOT_SET;
    }

    result = sendStringInformation(inTimeoutMs);
    if (result < LIBUSB_SUCCESS)
    {
        dbgPrintLine(eLogError, "%s() ERROR: sendStringInformation() failed=%d", __FUNCTION__, result);
        return result;
    }

    //Switch to accessory mode
    result = sendControlTransfer(LIBUSB_REQUEST_TYPE_VENDOR,
            static_cast<uint8_t> (AOAP_ACCESSORY_START_MODE), 0, 0, NULL, 0, inTimeoutMs);
    close(true);
    if (result < LIBUSB_SUCCESS)
    {
        if (result == LIBUSB_ERROR_NO_DEVICE)
        {
            /* libusb returns a no device error in some cases, most probably due to USB reset while
             * switching to accessory mode. Ignore it. If the device is really lost, wait for timeout to
             * handle it */
            dbgPrintLine(eLogWarn, "%s() WARNING: sendControlTransfer() returned no device error: "
                    "ignore and wait for device", __FUNCTION__, result);
            result = LIBUSB_SUCCESS;
        } else {
            dbgPrintLine(eLogError, "%s() ERROR: sendControlTransfer() failed=%d", __FUNCTION__, result);
            return result;
        }
    }
    return result;
}

int Device::sendStringInformation(unsigned int sendStringTmoMs)
{
    int result = AOAP_SUCCESS;

    dbgPrintLine(eLogDebug, "%s() %s, %s, %s, %s, %s, %s, %s", \
            __FUNCTION__, mpAccessory->getManufacturer().c_str(), mpAccessory->getModelName().c_str(), mpAccessory->getDescription().c_str(), \
            mpAccessory->getDescription().c_str(), mpAccessory->getVersion().c_str(), mpAccessory->getUri().c_str(), mpAccessory->getSerial().c_str());

    if (mpAccessory->getAccessory())
    {
        //Manufacturer
        result = sendControlString(mpAccessory->getManufacturer(),
                MANUFACURER_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }

        //Model name
        result = sendControlString(mpAccessory->getModelName(),
                MODEL_NAME_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }

        //Description
        result = sendControlString(mpAccessory->getDescription(),
                DESCRIPTION_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }

        //Version
        result = sendControlString(mpAccessory->getVersion(),
                VERSION_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }

        //URI
        result = sendControlString(mpAccessory->getUri(),
                URI_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }

        //Serial
        result = sendControlString(mpAccessory->getSerial(),
                SERIAL_CONTROL_STRING, sendStringTmoMs);
        if (result < LIBUSB_SUCCESS)
        {
            return result;
        }
    }
    else
    {
        if (!mpAccessory->getAccessory() && !mpAccessory->getAudio())
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Neither audio nor accessory mode selected -> nothing to do",
                    __FUNCTION__);
            return AOAP_ERROR_INVALID_PARAM;
        }
    }

    if (mpAccessory->getAudio() && mEnableAudio)
    {
        result = sendControlTransfer(LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
                static_cast<uint8_t> (AOAP_ACCESSORY_ENABLE_AUDIO), 1, //enable audio (2 channel, 16-bit PCM at 44100 Hz)
                0, 0, 0, 0); //no timeout
        if (result < LIBUSB_SUCCESS)
        {
            if (LIBUSB_ERROR_PIPE == result)
            {
                dbgPrintLine(eLogWarn,
                        "%s() WARNING: Device does not support audio", __FUNCTION__);
                mAudioEnabled = false;
                //simply continue w/o audio
            }
            else
            {
                dbgPrintLine(eLogError,
                    "%s() ERROR: Failed to enable audio with code=%d", __FUNCTION__, result);
                return result;
            }
        }
        else
        {
            mAudioEnabled = true;
        }
    }
    else if (mpAccessory->getAudio() && !mEnableAudio)
    {
        dbgPrintLine(eLogWarn,
                "%s() WARNING: Audio support for this device is switched off",
                __FUNCTION__);
    }
    else
    {
    }

    return result;
}

int Device::findEndpoints(void)
{
    int result = AOAP_SUCCESS;
    bool bulkInEndpointFound = false;
    bool bulkOutEndpointFound = false;

    if (!isOpen())
    {
        result = open();
        if (!isOpen())
        {
            dbgPrintLine(eLogWarn, "%s() Couldn't open device with '%s' (%d)",
                    __FUNCTION__, aoap_get_result_as_string(result), result);
            return result;
        }
    }

    struct libusb_device *device = libusb_get_device(mpDeviceHandle);
    struct libusb_config_descriptor *config = NULL;
    result = libusb_get_active_config_descriptor(device, &config);
    if (result < LIBUSB_SUCCESS)
    {
        dbgPrintLine(eLogError,
                "%s() ERROR: Failed to get config descriptor with code='%s'(%d)",
                __FUNCTION__, aoap_get_result_as_string(result), result);
        return result;
    }

    for (int i = 0; i < config->bNumInterfaces; i++)
    {
        const struct libusb_interface interface = config->interface[i];
        for (int j = 0; j < interface.num_altsetting; j++)
        {
            const struct libusb_interface_descriptor *interfaceDescriptor = &(interface.altsetting[j]);
            for (int k = 0; k < interfaceDescriptor->bNumEndpoints; k++)
            {
                //Use first bulk endpoints found
                const struct libusb_endpoint_descriptor
                        * endpointDescriptor =
                                &(interfaceDescriptor->endpoint[k]);
                if (LIBUSB_TRANSFER_TYPE_BULK == (0x03
                        & endpointDescriptor->bmAttributes))
                {
                    if (0 != (LIBUSB_ENDPOINT_IN
                            & endpointDescriptor->bEndpointAddress))
                    {
                        mAccessoryEndpointIn.address = endpointDescriptor->bEndpointAddress;
                        mAccessoryEndpointIn.maxPacketSize = libusb_get_max_packet_size(device, endpointDescriptor->bEndpointAddress);
                        bulkInEndpointFound = true;

                        dbgPrintLine(eLogDebug,
                                "%s() Bulk IN endpoint found with address=0x%x and maxPacketSize=%d",
                                __FUNCTION__, mAccessoryEndpointIn.address,
                                mAccessoryEndpointIn.maxPacketSize);
                    }
                    else
                    {
                        mAccessoryEndpointOut.address = endpointDescriptor->bEndpointAddress;
                        mAccessoryEndpointOut.maxPacketSize = libusb_get_max_packet_size(device, endpointDescriptor->bEndpointAddress);
                        bulkOutEndpointFound = true;

                        dbgPrintLine(eLogDebug,
                                "%s() Bulk OUT endpoint found with address=0x%x and maxPacketSize=%d",
                                __FUNCTION__, mAccessoryEndpointOut.address,
                                mAccessoryEndpointOut.maxPacketSize);
                    }

                    //Both found?
                    if (bulkInEndpointFound && bulkOutEndpointFound)
                    {
                        setState(AOAP_DEVICE_STATE_ACCESSORY);
                        dbgPrintLine(eLogInfo, "%s() Found Endpoint In:%d/Out:%d at Interface=%d",
                                __FUNCTION__, mAccessoryEndpointIn.address, \
                                mAccessoryEndpointOut.address, interfaceDescriptor->bInterfaceNumber);
                        mInterfaceNumber = interfaceDescriptor->bInterfaceNumber;

                        libusb_free_config_descriptor(config);
                        return AOAP_SUCCESS;
                    }
                }
                else
                {
                    dbgPrintLine(eLogInfo,
                            "%s() Non-bulk endpoint found with address0x%x\n",
                            __FUNCTION__, endpointDescriptor->bEndpointAddress);
                }
            }
        }
    }

    libusb_free_config_descriptor(config);

    return AOAP_ERROR_ERROR_NO_ENDPOINTS;
}

int Device::sendControlString(const std::string& message,
        tControlString type, unsigned int timeout /*= 0*/)
{
    int result = AOAP_SUCCESS;
    if (isOpen())
    {
        result = sendControlTransfer(LIBUSB_REQUEST_TYPE_VENDOR,
                                     AOAP_ACCESSORY_SEND_IDENTIFICATION, 0,
                                     static_cast<uint16_t> (type),
                                     reinterpret_cast<const unsigned char*>(message.c_str()),
                                     message.size(), timeout);
        if (result < 0)
        {
            dbgPrintLine(eLogError, "%s() ERROR: Sending %s string failed",
                         __FUNCTION__, convertControlStringType(type).c_str());
        }
        else
        {
            dbgPrintLine(eLogDebug, "%s() %s string '%s' sent with bytes=%d",
                    __FUNCTION__, convertControlStringType(type).c_str(), message.c_str(),
                    result);
        }
    }
    else
    {
        dbgPrintLine(eLogError, "%s() ERROR: No Device handle", __FUNCTION__);
        result = AOAP_ERROR_NO_DEVICE;
    }
    return result;
}

int Device::getAoapProtocolVersion(uint8_t *pBuffer, int bufferSize,
        unsigned int timeout /*= 0*/)
{
    int result = AOAP_ERROR_GENERAL;
    if (nullptr != pBuffer)
    {
        result = sendControlTransfer(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
                                     static_cast<uint8_t> (AOAP_ACCESSORY_PROTOCOL_REQUEST), 0, 0,
                                     pBuffer, bufferSize, timeout);

        if (result >= LIBUSB_SUCCESS)
        {
            if (AOAP_PROTOCOL_VERSION_SIZE == result)
            {
                if ((pBuffer[AOAP_MAJOR_VERSION_INDEX] == 0) && (pBuffer[AOAP_MINOR_VERSION_INDEX] == 0))
                {
                    dbgPrintLine(eLogError, "%s() ERROR: Invalid protocol version (major=0,minor=0)", __FUNCTION__);
                    result = AOAP_ERROR_INVALID_PROTOCOL;
                }
                else
                {
                    dbgPrintLine(eLogInfo, "%s() AOAP protocol version is: %d.%d",
                            __FUNCTION__, pBuffer[AOAP_MAJOR_VERSION_INDEX], pBuffer[AOAP_MINOR_VERSION_INDEX]);
                }
            }
            else
            {
                dbgPrintLine(eLogWarn, "%s() Warning: Expected %d bytes, but received %d bytes", __FUNCTION__, AOAP_PROTOCOL_VERSION_SIZE, result);
            }
        }
        else if (result == LIBUSB_ERROR_PIPE)
        {
            dbgPrintLine(eLogWarn,
                    "%s() Warning: Device does not support AOAP (code='%s')",
                    __FUNCTION__, aoap_get_result_as_string(result));
        }
        else
        {
            dbgPrintLine(eLogWarn,
                    "%s() Warning: Failed to get protocol with code=%s (%d)",
                    __FUNCTION__, aoap_get_result_as_string(result), result);
        }
    }
    else
    {
        dbgPrintLine(eLogError, "%s() ERROR: pBuffer is NULL", __FUNCTION__);
    }

    return result;
}

void Device::setState(tState state)
{
    if (getState() != state)
    {
        dbgPrintLine(eLogDebug, "%s() '%s' -> '%s'",
                __FUNCTION__, getStateString(getState()).c_str(),
                getStateString(state).c_str());
        mState = state;
    }
}

int Device::claimInterface(int interfaceNumber)
{
    if (!isOpen())
    {
        dbgPrintLine(eLogError, "%s() ERROR: Device not opened", __FUNCTION__);
        return AOAP_ERROR_NO_DEVICE;
    }
    if (0 > interfaceNumber)
    {
        dbgPrintLine(eLogError, "%s() ERROR: interfaceNumber=%d is not valid",
                __FUNCTION__, interfaceNumber);
        return AOAP_ERROR_INVALID_PARAM;
    }

    if (mClaimedInterfaceNumber != mInterfaceNumber)
    {
        int result = libusb_claim_interface(mpDeviceHandle, interfaceNumber);
        if (result < LIBUSB_SUCCESS)
        {
            dbgPrintLine(eLogError, "%s() ERROR: Failed to claim interface(%d) with code=%d",
                    __FUNCTION__, interfaceNumber, result);
            return result;
        }
        else
        {
            dbgPrintLine(eLogDebug, "%s() SUCCESS: claim interface =%d",
                    __FUNCTION__, interfaceNumber);
            mClaimedInterfaceNumber = interfaceNumber;
            return AOAP_SUCCESS;
        }
    }
    else
    {
        /* Interface already claimed */
        return AOAP_SUCCESS;
    }
}

int Device::releaseInterface(void)
{
    if (!isOpen())
    {
        dbgPrintLine(eLogError, "%s() ERROR: Device not opened", __FUNCTION__);
        return AOAP_ERROR_NO_DEVICE;
    }

    if (mClaimedInterfaceNumber >= 0)
    {
        int result = libusb_release_interface(mpDeviceHandle, mClaimedInterfaceNumber);
        if (result < LIBUSB_SUCCESS)
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Failed to release interface(%d) with code=%d",
                    __FUNCTION__, mClaimedInterfaceNumber, result);
            //return libusb result
        }
        else
        {
            result = AOAP_SUCCESS;
        }
        mClaimedInterfaceNumber = -1;
        return result;
    }
    else
    {
        dbgPrintLine(eLogWarn, "%s() WARNING: interface(%d) isn't claimed",
                    __FUNCTION__, mClaimedInterfaceNumber);
        return AOAP_SUCCESS;
    }
}

/*state*/std::string Device::getStateString(tState state)
{
    switch (state)
    {
        case AOAP_DEVICE_STATE_IDLE:
        {
            return "idle";
            //break;
        }
        case AOAP_DEVICE_STATE_SWITCHING:
        {
            return "switchingToAccessory";
            //break;
        }
        case AOAP_DEVICE_STATE_ACCESSORY:
        {
            return "accessoryMode";
            //break;
        }
        case AOAP_DEVICE_STATE_UNKNOWN:
        {
            return "unknownState";
            //break;
        }
        default:
        {
            return "unknown";
            //break;
        }
    }
}

std::string Device::convertControlStringType(tControlString type) const
{
    switch (type)
    {
        case MANUFACURER_CONTROL_STRING:
        {
            return "manufacturer";
            //break;
        }
        case MODEL_NAME_CONTROL_STRING:
        {
            return "model name";
            //break;
        }
        case DESCRIPTION_CONTROL_STRING:
        {
            return "description";
            //break;
        }
        case VERSION_CONTROL_STRING:
        {
            return "version";
            //break;
        }
        case URI_CONTROL_STRING:
        {
            return "URI";
            //break;
        }
        case SERIAL_CONTROL_STRING:
        {
            return "serial";
            //break;
        }
        default:
        {
            return "unknown";
            //break;
        }
    }
}

void Device::printBytes(const unsigned char *pBuffer, int bufferSize) const
{
    //This code is only active when not using DLT
    if (!getDltLogging() && getLogLevel() > eLogDebug)
    {
        int printSize = bufferSize;
        if (bufferSize > mMaxPrintBufferSize)
        {
            if (getLogLevel() <= eLogVerbose) //then limit
            {
                printSize = mMaxPrintBufferSize;
            }
        }

        for (int i = 0; i < printSize; i++)
        {
            if (!(i % 16))
            {
                dbgPrint(eLogVerbose, "  ");
            }

            dbgPrint(eLogVerbose, "%2.2x ", pBuffer[i]);

            //Append a line break after 16 bytes or at the end of the buffer
            if ((15 == (i % 16)) || (i+1 == printSize))
            {
                if (15 == (i % 16)) //at the end of the buffer
                {
                    dbgPrint(eLogVerbose, "\n");
                    if ((i+1 == printSize) && (printSize != bufferSize))
                    {
                        dbgPrint(eLogVerbose, "  ...\n");
                    }
                }
                else //not at the end of a line but at the end of the buffer
                {
                    if (printSize != bufferSize)
                    {
                        dbgPrint(eLogVerbose, "...\n");
                    }
                    else
                    {
                        dbgPrint(eLogVerbose, "\n");
                    }
                }
            }
        }
    }
}
