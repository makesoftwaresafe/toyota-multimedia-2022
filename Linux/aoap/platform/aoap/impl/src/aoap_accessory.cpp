/*
 * aoap_accessory.cpp
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_accessory.h"
#include "aoap_device.h"
#include "aoap_logging.h"
#include "aoap.h"
#include "aoap_tools.h"
#include "aoap_devicelist.h"
#include <stdio.h>

using namespace AOAP::Logging;

#define AOAP_DEVICE_CONNECT_TIMEOUT 10
#define AOAP_DEVICE_CTRL_REQ_TIMEOUT_MS 1000

/*static*/unsigned int Accessory::gCounter = 0;

Accessory::Accessory(const std::string &manufacturer,
        const std::string &modelName, const std::string &description,
        const std::string &version, const std::string &uri,
        const std::string &serial, bool enableAudio)
: mId(++gCounter)
, mManufacturer(manufacturer)
, mModelName(modelName)
, mDescription(description)
, mVersion(version)
, mUri(uri)
, mSerial(serial)
, mAudio(enableAudio)
, mConnectTimeout(AOAP_DEVICE_CONNECT_TIMEOUT)
, mControlRequestTimeout(AOAP_DEVICE_CTRL_REQ_TIMEOUT_MS)
, mReferenceCounter(0)
{
    dbgPrintLine(eLogInfo, "%s created w/id=%d", __FUNCTION__, mId);
    dbgPrintLine(eLogDebug, "  manufacturer=\"%s\"", manufacturer.c_str());
    dbgPrintLine(eLogDebug, "  modelName=\"%s\"", modelName.c_str());
    dbgPrintLine(eLogDebug, "  description=\"%s\"", description.c_str());
    dbgPrintLine(eLogDebug, "  version=\"%s\"", version.c_str());
    dbgPrintLine(eLogDebug, "  uri=\"%s\"", uri.c_str());
    dbgPrintLine(eLogDebug, "  serial=\"%s\"", serial.c_str());
    dbgPrintLine(eLogDebug, "  audio %s", enableAudio ? "enabled" : "disabled");
}

Accessory::~Accessory()
{
    dbgPrintLine(eLogDebug, "Delete %s w/id=%d", __FUNCTION__, mId);
    if (gCounter > 0) {
        gCounter--;
    }
}

bool Accessory::getAccessory(void)
{
    if (getManufacturer().empty() || (0 == getManufacturer().size()))
    {
        dbgPrintLine(eLogInfo, "%s() manufacturer is empty", __FUNCTION__);
        return false;
    }

    if (getModelName().empty() || (0 == getModelName().size()))
    {
        dbgPrintLine(eLogInfo, "%s() model name is empty", __FUNCTION__);
        return false;
    }

    if (getDescription().empty() || (0 == getDescription().size()))
    {
        dbgPrintLine(eLogInfo, "%s() description is empty", __FUNCTION__);
        return false;
    }

    if (getVersion().empty() || (0 == getVersion().size()))
    {
        dbgPrintLine(eLogInfo, "%s() version is empty", __FUNCTION__);
        return false;
    }

    // HUIG 2.4: The URI and Serial Number fields are required for the AOAP handshake,
    // but the HU can set them to any value, including the empty string.
    if (getUri().empty() || (0 == getUri().size()))
    {
        dbgPrintLine(eLogInfo, "%s() URI is empty", __FUNCTION__);
    }

    if (getSerial().empty() || (0 == getSerial().size()))
    {
        dbgPrintLine(eLogInfo, "%s() serial is empty", __FUNCTION__);
    }

    return true;
}

unsigned int Accessory::getNumDevices()
{
    // Acquire lock
    if (!DeviceList::getInstance()->getLock()) {
        return AOAP_ERROR_GENERAL;
    }
    // return the number of devices which are still associated to the accessory
    int result = DeviceList::getInstance()->getNumDevices();

    if (!DeviceList::getInstance()->releaseLock()) {
        result = AOAP_ERROR_GENERAL;
    }
    return result;
}

int Accessory::disconnectDevice(unsigned int deviceId)
{
    // Acquire lock
    if (!DeviceList::getInstance()->getLock()) {
        return AOAP_ERROR_GENERAL;
    }

    int result = AOAP_SUCCESS;

    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(deviceId);
    if (pDevice) {
        // removeDevice() has void as return value. be sure result = AOAP_SUCCESS
        DeviceList::getInstance()->removeDevice(pDevice);
        pDevice = nullptr;
    } else {
        dbgPrintLine(eLogDebug, "disconnectDevice() Device w/id=%d not found", deviceId);
        result = AOAP_ERROR_NO_DEVICE;
    }

    if (!DeviceList::getInstance()->releaseLock()) {
        result = AOAP_ERROR_GENERAL;
    }
    return result;
}

int Accessory::connectDevice(unsigned int vendorId, unsigned int productId,
        const std::string &serial, aoap_connectCB callback, bool audioSupport, void *pToken)
{
    dbgPrintLine(eLogInfo, "%s(%.4x:%.4x, s/n='%s', audio=%s, token=%p) called",
            __FUNCTION__, vendorId, productId, serial.c_str(), audioSupport ? "enabled" : "disable", pToken);

    // Acquire lock
    if (!DeviceList::getInstance()->getLock())
    {
        return AOAP_ERROR_GENERAL;
    }

    int result = AOAP_SUCCESS;
    /* find device by vendorId & productId and cross check with serial */
    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(vendorId, productId, serial);
    if (!pDevice)
    {
        /* device not found by vendorId & productId,
         * but is may be in different mode -> find by serial number */
        pDevice = DeviceList::getInstance()->findDeviceBySerial(serial);
        if (!pDevice)
        {
            /* new device */
            pDevice = std::make_shared<Device> (vendorId, productId, serial);
            if (pDevice)
            {
                pDevice->setAccessory(this);
                pDevice->retrieveDevNum();

                DeviceList::getInstance()->addDevice(pDevice);
                dbgPrintLine(eLogDebug,
                        "%s(%.4x:%.4x, serial='%s', audio=%s) Device id=%d created and inserted",
                        __FUNCTION__, pDevice->getVendorId(), pDevice->getProductId(), pDevice->getSerial().c_str(),
                        audioSupport ? "enabled" : "disabled", pDevice->getId());
            }
            else
            {
                dbgPrintLine(eLogFatal, "%s() FATAL ERROR: Couldn't create device", __FUNCTION__);
                result = AOAP_ERROR_MEMORY_FAULT;
            }
        }
        else
        {
            /* Check if device is in accessory mode */
            if (AOAP::Tools::isAccessory(pDevice->getVendorId(), pDevice->getProductId()))
            {
                /* assume provided vendorId & productId are incorrect and that upper layer
                 * does not know that the device is already or still in accessory mode */
                dbgPrintLine(eLogWarn, "%s(%.4x:%.4x, s/n='%s) WARNING input parameter are incorrect. Device with id=%d has %.4x:%.4x",
                        __FUNCTION__, vendorId, productId, serial.c_str(), pDevice->getId(), pDevice->getVendorId(), pDevice->getProductId());
                result = AOAP_SUCCESS;
            }
            else
            {
                /* shall not be possible to come here:
                 * device not found by vendorId & productId
                 * but device found by serial and device not in AOAP mode ?!? */
                dbgPrintLine(eLogError, "%s() ERROR: Device w/id=%d found by serial='%s'," \
                                        "but vendorId & productId does not match to a known state",
                        __FUNCTION__, pDevice->getId(), serial.c_str());
                result = AOAP_ERROR_GENERAL;
            }
        }
    }

    /* Check the current state since we check if device is in accessory mode
     * at device creation with "new Device(...)". */
    if (pDevice && (AOAP_SUCCESS == result))
    {
        Device::tState deviceState = pDevice->getState();
        switch (deviceState)
        {
            case Device::AOAP_DEVICE_STATE_IDLE:
            case Device::AOAP_DEVICE_STATE_UNKNOWN:
            {
                dbgPrintLine(eLogInfo,
                        "%s(%.4x:%.4x, serial='%s', audio=%s) Device w/id=%d already exists -> continue",
                        __FUNCTION__, pDevice->getVendorId(), pDevice->getProductId(), pDevice->getSerial().c_str(),
                        audioSupport ? "enabled" : "disabled", pDevice->getId());
                pDevice->setAccessory(this);
                result = AOAP_SUCCESS;
                break;
            }
            case Device::AOAP_DEVICE_STATE_ACCESSORY:
            {
                result = AOAP_ERROR_ALREADY_DONE;
                dbgPrintLine(eLogInfo, "%s() Device w/id=%d is already in accessory mode '%s' (%d)",
                        __FUNCTION__, pDevice->getId(), aoap_get_result_as_string(result), result);
                break;
            }
            case Device::AOAP_DEVICE_STATE_SWITCHING:
            {
                result = AOAP_ERROR_BUSY;
                dbgPrintLine(eLogError, "%s() ERROR: Device w/id=%d is already switching '%s' (%d)",
                        __FUNCTION__, pDevice->getId(), aoap_get_result_as_string(result), result);
                break;
            }
            default:
            {
                result = AOAP_ERROR_GENERAL;
                dbgPrintLine(eLogError, "%s() ERROR: Device w/id=%d in wrong state '%s' (%d)",
                        __FUNCTION__, pDevice->getId(), aoap_get_result_as_string(result), result);
                break;
            }
        }
    }

    if (pDevice && (AOAP_SUCCESS == result))
    {
        result = pDevice->open();
        if (AOAP_SUCCESS == result)
        {
            dbgPrintLine(eLogDebug, "%s(%.4x:%.4x, serial='%s', audio=%s) opened",
                    __FUNCTION__, pDevice->getVendorId(), pDevice->getProductId(),
                    pDevice->getSerial().c_str(), audioSupport ? "enabled" : "disabled");
        }
        else
        {
            dbgPrintLine(eLogError, "%s() ERROR: Failed to open device with '%s' (%d) -> remove device w/id=%d",
                    __FUNCTION__, aoap_get_result_as_string(result), result, pDevice->getId());
            DeviceList::getInstance()->removeDevice(pDevice);
            pDevice = nullptr;
        }
    }

    //If everything was successful so far, connect device now
    if (pDevice && (AOAP_SUCCESS == result))
    {
        result = pDevice->connect(callback, audioSupport, pToken, mConnectTimeout, mControlRequestTimeout);
        if (AOAP_SUCCESS == result)
        {
            result = static_cast<int>(pDevice->getId()); //return device ID
        }
        else
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Failed to switch to accessory mode with '%s' (%d)",
                    __FUNCTION__, aoap_get_result_as_string(result), result);
        }
    }
    else if (pDevice && (AOAP_ERROR_ALREADY_DONE == result))
    {
        /* Assume that AOAP_ERROR_ALREADY_DONE will be set in case:
         * - aoap_usbmonitor creates the device before application call connectDevice()
         * - application calls connectDevice() with original vendorId, productId,
         *   but device already in accessory mode */

        if (callback)
        {
            /* trigger connection callback to inform upper layer about device in accessory mode */
            callback(getId(), pDevice->getId(), result, pToken, pDevice->getAudioStatus());
        }

        result = static_cast<int>(pDevice->getId()); //return device ID
    }
    else
    {
    }

    if (!DeviceList::getInstance()->releaseLock())
    {
        result = AOAP_ERROR_GENERAL;
    }
    return result;
}

void Accessory::setConnectTimeout(unsigned int seconds)
{
    dbgPrintLine(eLogInfo, "%s(%d seconds) for acc id=%d", __FUNCTION__, seconds, getId());

    mConnectTimeout = seconds;
}

void Accessory::setControlRequestTmo(unsigned int inTimeoutMs)
{
    dbgPrintLine(eLogInfo, "%s(%d miliseconds) for acc id=%d", __FUNCTION__, inTimeoutMs, getId());

    mControlRequestTimeout = inTimeoutMs;
}

int Accessory::checkAoapSupport(unsigned int vendorId,
        unsigned int productId, const std::string &serial,
        unsigned int &majorVersion, unsigned int &minorVersion)
{
    dbgPrintLine(eLogInfo, "%s(%.4x:%.4x s/n='%s') called",
            __FUNCTION__, vendorId, productId, serial.c_str());

    if (!DeviceList::getInstance()->getLock())
    {
        return AOAP_ERROR_GENERAL; //if we couldn't acquire lock, leave
    }

    int result = AOAP_SUCCESS;
    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDeviceBySerial(serial);

    if (!pDevice)
    {
        pDevice = std::make_shared<Device> (vendorId, productId, serial);
        if (pDevice)
        {
            pDevice->retrieveDevNum();

            DeviceList::getInstance()->addDevice(pDevice);
            dbgPrintLine(eLogInfo, "%s() Device id=%d created and inserted",
                    __FUNCTION__, pDevice->getId());
        }
        else
        {
            dbgPrintLine(eLogFatal,
                    "%s() FATAL ERROR: Couldn't create device %.4x:%.4x w/serial='%s'",
                    __FUNCTION__, vendorId, productId, serial.c_str());
            result = AOAP_ERROR_MEMORY_FAULT;
        }
    }
    else
    {
        if (AOAP::Tools::isAccessory(vendorId, productId)) //device is already in accessory mode
        {
            int accId = pDevice->getAccessoryId();
            if ((accId >= 0) && (static_cast<unsigned int>(accId) != getId()))
            {
                dbgPrintLine(eLogError,
                        "%s() ERROR: Device is already in use by another accessory",
                        __FUNCTION__);
                result = AOAP_ERROR_BUSY;
            }
        }
    }

    //Open device if everything was successful so far
    if (pDevice && (AOAP_SUCCESS == result))
    {
        result = pDevice->open();
        if (AOAP_SUCCESS != result)
        {
            dbgPrintLine(eLogError, "%s() ERROR: Opening device failed -> remove device w/id=%d",
                    __FUNCTION__, pDevice->getId());
            DeviceList::getInstance()->removeDevice(pDevice);
            pDevice = nullptr;
        }
    }

    if (!DeviceList::getInstance()->releaseLock())
    {
        dbgPrintLine(eLogError, "%s() ERROR: Failed to release lock", __FUNCTION__);
        result = AOAP_ERROR_GENERAL;
    }

    //Finally check AOAP support if everything was successful so far
    if (pDevice && (AOAP_SUCCESS == result))
    {
        result = pDevice->checkAoapSupport(majorVersion, minorVersion);
    }

    return result;
}
