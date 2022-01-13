/*
 * aoap.cpp
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap.h"
#include "aoap_controller.h"
#include "aoap_usbmonitor.h"
#include "aoap_logging.h"
#include <stdio.h>
#include <cstring>

using namespace AOAP::Logging;

using namespace AOAP::Control;

int aoap_create_accessory(t_aoap_accessory_param* pAccessory)
{
    int ret = AOAP_SUCCESS;
    Controller::getInstance(); //in constructor of 'Controller' DLT will be registered. Therefore do not print anything before
#ifdef COMP_GIT_VERSION
    dbgPrintLine(eLogInfo, "%s() AOAP version %s", __FUNCTION__, COMP_GIT_VERSION); //print AOAP version info
#else
    dbgPrintLine(eLogWarn, "%s() AOAP version '%s'", __FUNCTION__, "n/a");
#endif //#ifdef COMP_GIT_VERSION

    if (!pAccessory)
    {
        dbgPrintLine(eLogError, "%s() ERROR: pAccessory is NULL", __FUNCTION__);
        return AOAP_ERROR_INVALID_PARAM;
    }

    Controller::getInstance().lock();
    ret = Controller::getInstance().createAccessory(pAccessory);
    Controller::getInstance().unlock();
    return ret;
}

void aoap_delete_accessory(unsigned int accessoryId)
{
    Controller::getInstance().lock();
    Controller::getInstance().deinitAccessory(accessoryId);
    if (Controller::getInstance().getAccessoryNumber() == 0)
    {
        UsbMonitor::deleteInstance();
    }
    else
    {
        dbgPrintLine(eLogInfo,
                "%s(acc_id=%d) There are still some accessories left",
                __FUNCTION__, accessoryId);
    }
    Controller::getInstance().unlock();
}

int aoap_defer_delete_accessory(unsigned int accessoryId, unsigned int deviceId)
{
    int ret = AOAP_SUCCESS;
    Controller::getInstance().lock();
    // Delete the device specified by the deviceId
    if (AOAP_SUCCESS != (ret = Controller::getInstance().disconnectDevice(accessoryId, deviceId))) {
        dbgPrintLine(eLogInfo, "%s() delete device failed, ret=%d", __FUNCTION__, ret);
    }
    // get the number of devices which are associated to the accessory
    if (0 > (ret = Controller::getInstance().getNumDevices(accessoryId))) {
        dbgPrintLine(eLogError, "%s() ERROR:  getDeviceNumber() failed ret=%d",
                __FUNCTION__, ret);
    }
    Controller::getInstance().deinitAccessory(accessoryId);

    if (Controller::getInstance().getAccessoryNumber() == 0) {
        UsbMonitor::deleteInstance();
    } else {
        dbgPrintLine(eLogInfo, "%s(acc_id=%d) There are still some accessories left",
                __FUNCTION__, accessoryId);
    }

    Controller::getInstance().unlock();
    ret = AOAP_SUCCESS;

    return ret;
}

int aoap_connect(unsigned int accessoryId, unsigned int vendorId,
        unsigned int productId, const char *pSerial, aoap_connectCB callback,
        unsigned int audioSupport, void *pToken)
{
    Controller::getInstance().lock();
    int ret = Controller::getInstance().connectDevice(accessoryId, vendorId, productId,
            pSerial, callback, (0 == audioSupport) ? false : true, pToken);
    Controller::getInstance().unlock();
    return ret;
}

void aoap_set_connect_timeout(unsigned int accessoryId, unsigned int seconds)
{
    Controller::getInstance().lock();
    Controller::getInstance().setConnectTimeout(accessoryId, seconds);
    Controller::getInstance().unlock();
}

void aoap_set_control_request_timeout(unsigned int accessoryId, unsigned int inTimeoutMs)
{
    Controller::getInstance().lock();
    Controller::getInstance().setControlRequestTimeout(accessoryId, inTimeoutMs);
    Controller::getInstance().unlock();
}

int aoap_read(unsigned int accessoryId, unsigned int deviceId,
        unsigned char *pBuffer, unsigned int bufferSize, unsigned int timeout)
{
    return Controller::getInstance().read(accessoryId, deviceId, pBuffer,
            bufferSize, timeout);
}

int aoap_read1(unsigned int accessoryId, unsigned int deviceId,
        unsigned char *pBuffer, unsigned int bufferSize, unsigned int *pTransferred, unsigned int timeout)
{
    return Controller::getInstance().read(accessoryId, deviceId, pBuffer,
            bufferSize, pTransferred, timeout);
}

int aoap_write(unsigned int accessoryId, unsigned int deviceId,
        const unsigned char *pBuffer, unsigned int bufferSize,
        unsigned int timeout)
{
    return Controller::getInstance().write(accessoryId, deviceId, pBuffer,
            bufferSize, timeout);
}

int aoap_write1(unsigned int accessoryId, unsigned int deviceId,
        const unsigned char *pBuffer, unsigned int bufferSize,
        unsigned int *pTransferred, unsigned int timeout)
{
    return Controller::getInstance().write(accessoryId, deviceId, pBuffer,
                                           bufferSize, pTransferred, timeout);
}

int aoap_check_support(unsigned int accessoryId, unsigned int vendorId,
        unsigned int productId, const char *pSerial, unsigned int *pMajor,
        unsigned int *pMinor)
{
    Controller::getInstance().lock();
    int ret = Controller::getInstance().checkAoapSupport(accessoryId, vendorId, productId,
                                                         pSerial, *pMajor, *pMinor);
    Controller::getInstance().unlock();
    return ret;
}

const char* aoap_get_result_as_string(int result)
{
    switch (result)
    {
        case AOAP_SUCCESS:
        {
            return "Success";
            //break;
        }
        case AOAP_ERROR_IO:
        {
            return "I/O error";
            //break;
        }
        case AOAP_ERROR_INVALID_PARAM:
        {
            return "Invalid Parameter";
            //break;
        }
        case AOAP_ERROR_ACCESS:
        {
            return "Access error";
            //break;
        }
        case AOAP_ERROR_NO_DEVICE:
        {
            return "No device";
            //break;
        }
        case AOAP_ERROR_NOT_FOUND:
        {
            return "Not found";
            //break;
        }
        case AOAP_ERROR_BUSY:
        {
            return "Busy";
            //break;
        }
        case AOAP_ERROR_TIMEOUT:
        {
            return "Timeout";
            //break;
        }
        case AOAP_ERROR_OVERFLOW:
        {
            return "Overflow";
            //break;
        }
        case AOAP_ERROR_PIPE:
        {
            return "Pipe Error";
            //break;
        }
        case AOAP_ERROR_INTERRUPTED:
        {
            return "Interrupted";
            //break;
        }
        case AOAP_ERROR_NO_MEM:
        {
            return "No Memory";
            //break;
        }
        case AOAP_ERROR_NOT_SUPPORTED:
        {
            return "Not supported";
            //break;
        }
        case AOAP_ERROR_OTHER:
        {
            return "Other libusb Error";
            //break;
        }
        case AOAP_ERROR_GENERAL:
        {
            return "General error";
            //break;
        }
        case AOAP_ERROR_MEMORY_FAULT:
        {
            return "Memory Fault";
            //break;
        }
        case AOAP_ERROR_CLAIMING_USB_INTERFACE_FAILED:
        {
            return "Claiming USB interface failed";
            //break;
        }
        case AOAP_ERROR_DEVICE_MISMATCH:
        {
            return "Device mismatch";
            //break;
        }
        case AOAP_ERROR_ALREADY_DONE:
        {
            return "Already done";
            //break;
        }
        case AOAP_ERROR_ACCESSORY_NOT_FOUND:
        {
            return "Accessory not found";
            //break;
        }
        case AOAP_ERROR_ERROR_NO_ENDPOINTS:
        {
            return "No USB endpoints found";
            //break;
        }
        case AOAP_ERROR_CREATING_UDEV:
        {
            return "Couldn't create udev";
            //break;
        }
        case AOAP_ERROR_ACCESSORY_NOT_SET:
        {
            return "Accessory not set";
            //break;
        }
        case AOAP_ERROR_CONNECT_TIMEOUT:
        {
            return "Connect timed out";
            //break;
        }
        default:
        {
            return "unknown";
            //break;
        }
    }
}

void aoap_set_log_level(int logLevel, bool prependTime)
{
    setLogLevel(logLevel, prependTime);
}

void aoap_set_log_destination(t_aoap_logging_destination destination, bool enable, const char* filename)
{
    setLogDestination(destination, enable, filename);
}

void aoap_enable_performance_measurement(void)
{
    Controller::getInstance().lock();
    Controller::getInstance().enablePerformanceMeasurements();
    Controller::getInstance().unlock();
}

