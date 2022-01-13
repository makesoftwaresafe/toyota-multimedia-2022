/*
 * aoap_controller.cpp
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_controller.h"
#include "aoap_accessory.h"
#include "aoap_device.h"
#include "aoap_usbmonitor.h"
#include "aoap_devicelist.h"
#include "aoap_timer.h"
#include "aoap_logging.h"
#include <libusb-1.0/libusb.h>
#include <cstdlib> //to have NULL defined
#include <cstring>
#include <assert.h>

using namespace AOAP::Logging;

namespace AOAP { namespace Control {

/*static*/Controller& Controller::getInstance(void)
{
    static Controller instance;
    return instance;
}

int Controller::createAccessory(t_aoap_accessory_param *pAccessoryParam)
{
    if (!pAccessoryParam)
    {
        dbgPrintLine(eLogError, "createAccessory() ERROR: accessory parameter struct is NULL");
        return AOAP_ERROR_INVALID_PARAM;
    }

    UsbMonitor::getInstance()->initMonitoring(); //make sure monitoring gets started when first accessory gets created

    /*PRQA: Lint Message 429: pAccessory will be released when deleting object from vector mAccessories */
    /*lint -save -e429*/
    Accessory *pAccessory = findAccessory(pAccessoryParam);
    if (!pAccessory)
    {
        pAccessory = new Accessory(pAccessoryParam->manufacturer,
                pAccessoryParam->modelName,
                pAccessoryParam->description,
                pAccessoryParam->version,
                pAccessoryParam->uri,
                pAccessoryParam->serial,
                pAccessoryParam->enableAudio);
        if (pAccessory)
        {
            mAccessories.push_back(pAccessory);
            pAccessory->refCntUp();
            return pAccessory->getId();
        }
        else
        {
            dbgPrintLine(eLogFatal, "createAccessory() FATAL ERROR: Failed to create accessory");
            return AOAP_ERROR_MEMORY_FAULT;
        }
    }
    else
    {
        pAccessory->refCntUp();
        dbgPrintLine(eLogInfo, "createAccessory() Accessory w/same identifiers already exists (refCnt=%d)",
                pAccessory->getRefCnt());

        return pAccessory->getId();
    }
    /*lint -restore*/
}

void Controller::deinitAccessory(unsigned int accessory_id)
{
    dbgPrintLine(eLogInfo, "deinitAccessory(acc_id=%d)", accessory_id);
    for (std::vector<Accessory*>::iterator p = mAccessories.begin(); p != mAccessories.end(); ++p)
    {
        if (*p && (*p)->getId() == accessory_id)
        {
            (*p)->refCntDown();
            if (0 == (*p)->getRefCnt()) {
                delete *p;
                mAccessories.erase(p);
            } else {
                dbgPrintLine(eLogInfo, "deinitAccessory(acc_id=%d) still some connections(refCnt=%d) associated to the accessory ",
                        accessory_id, (*p)->getRefCnt());
            }
            return;
        }
    }
}

int Controller::disconnectDevice(unsigned int accessory_id, unsigned int deviceId)
{
    Accessory *pAccessory = findAccessory(accessory_id);
    if (pAccessory) {
        return pAccessory->disconnectDevice(deviceId);
    } else {
        dbgPrintLine(eLogError, "disconnectDevice() ERROR: No accessory with id=%d found",
                accessory_id);
        return AOAP_ERROR_ACCESSORY_NOT_FOUND;
    }
}

unsigned int Controller::getNumDevices(unsigned int accessoryId)
{
    Accessory *pAccessory = findAccessory(accessoryId);
    if (pAccessory) {
        return pAccessory->getNumDevices();
    } else {
        dbgPrintLine(eLogError, "getNumDevices() ERROR: No accessory with id=%d found",
                accessoryId);
        return AOAP_ERROR_ACCESSORY_NOT_FOUND;
    }
}

int Controller::connectDevice(unsigned int accessoryId, unsigned int vendorId,
        unsigned int productId, const std::string &serial,
        aoap_connectCB callback, bool audioSupport, void *token)
{
    Accessory *pAccessory = findAccessory(accessoryId);
    if (pAccessory)
    {
        return pAccessory->connectDevice(vendorId, productId, serial,
                callback, audioSupport, token);
    }
    else
    {
        dbgPrintLine(eLogError, "connectDevice() ERROR: No accessory with id=%d found",
                accessoryId);
        return AOAP_ERROR_ACCESSORY_NOT_FOUND;
    }
}

void Controller::setConnectTimeout(unsigned int accessoryId, unsigned int seconds)
{
    Accessory *pAccessory = findAccessory(accessoryId);
    if (pAccessory)
    {
        pAccessory->setConnectTimeout(seconds);
    }
    else
    {
        dbgPrintLine(eLogError, "setConnectTimeout() ERROR: No accessory with id=%d found",
                        accessoryId);
    }
}

void Controller::setControlRequestTimeout(unsigned int accessoryId, unsigned int inTimeoutMs)
{
    Accessory *pAccessory = findAccessory(accessoryId);
    if (pAccessory)
    {
        pAccessory->setControlRequestTmo(inTimeoutMs);
    }
    else
    {
        dbgPrintLine(eLogError, "setControlRequestTimeout() ERROR: No accessory with id=%d found",
                     accessoryId);
    }
}

int Controller::read(unsigned int /*accessoryId*/, unsigned int deviceId,
        unsigned char *pBuffer, unsigned int bufferSize, unsigned int timeout)
{
    //Do not check if accessory is present

    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(deviceId);
    if (pDevice)
    {
        return pDevice->read(pBuffer, bufferSize, timeout);
    }
    else
    {
        dbgPrintLine(eLogError, "read() ERROR: Device w/id=%d not found", deviceId);
        return AOAP_ERROR_NO_DEVICE;
    }
}

int Controller::read(unsigned int /*accessoryId*/, unsigned int deviceId,
        unsigned char *pBuffer, unsigned int bufferSize, unsigned int *pTransferred, unsigned int timeout)
{
    //Do not check if accessory is present

    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(deviceId);
    if (pDevice)
    {
        return pDevice->read(pBuffer, bufferSize, pTransferred, timeout);
    }
    else
    {
        dbgPrintLine(eLogError, "read() ERROR: Device w/id=%d not found", deviceId);
        return AOAP_ERROR_NO_DEVICE;
    }
}

int Controller::write(unsigned int /*accessoryId*/, unsigned int deviceId,
        const unsigned char *pBuffer, unsigned int bufferSize, unsigned int timeout)
{
    //Do not check if accessory is present

    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(deviceId);
    if (pDevice)
    {
        return pDevice->write(pBuffer, bufferSize, timeout);
    }
    else
    {
        dbgPrintLine(eLogError, "write() ERROR: Device w/id=%d not found", deviceId);
        return AOAP_ERROR_NO_DEVICE;
    }
}

int Controller::write(unsigned int /*accessoryId*/, unsigned int deviceId,
        const unsigned char *pBuffer, unsigned int bufferSize, unsigned int *pTransferred, unsigned int timeout)
{
    //Do not check if accessory is present

    std::shared_ptr<Device> pDevice = DeviceList::getInstance()->findDevice(deviceId);
    if (pDevice)
    {
        return pDevice->write(pBuffer, bufferSize, pTransferred, timeout);
    }
    else
    {
        dbgPrintLine(eLogError, "write() ERROR: Device w/id=%d not found", deviceId);
        return AOAP_ERROR_NO_DEVICE;
    }
}

int Controller::checkAoapSupport(unsigned int accessoryId,
        unsigned int vendorId, unsigned int productId,
        const std::string &serial, unsigned int &majorVersion, unsigned int &minorVersion)
{
    Accessory *pAccessory = findAccessory(accessoryId);
    if (pAccessory)
    {
        return pAccessory->checkAoapSupport(vendorId, productId, serial,
                majorVersion, minorVersion);
    }
    else
    {
        dbgPrintLine(eLogError, "checkAoapSupport() ERROR: Accessory with id=%d not found",
                accessoryId);
        return AOAP_ERROR_ACCESSORY_NOT_FOUND;
    }
}

/*static*/void Controller::enablePerformanceMeasurements(void)
{
    Device::enablePerformanceMeasurements();
}

Controller::Controller()
{
    registerWithDlt();
    /* initialize the mutex referenced by mMutex */
    if (0 != pthread_mutex_init(&mMutex , NULL)) {
        dbgPrintLine(eLogError, "pthread_mutex_init() failed.");
        assert(&mMutex);
    }
    dbgPrintLine(eLogDebug, "Construct Controller");
}

Controller::~Controller()
{
    dbgPrintLine(eLogDebug, "Delete Controller");
    clearAccessories();

    unlock();
    /* destroy the mutex object referenced by mMutex */
    pthread_mutex_destroy(&mMutex);

    unregisterWithDlt();
}

void Controller::clearAccessories(void)
{
    while (mAccessories.size() > 0)
    {
        delete mAccessories.back();
        mAccessories.pop_back();
    }
}

Accessory* Controller::findAccessory(unsigned int accessory_id)
{
    for (unsigned int i = 0; i < mAccessories.size(); i++)
    {
        if (mAccessories[i] && (accessory_id == mAccessories[i]->getId()))
        {
            return mAccessories[i];
        }
    }

    return NULL;
}

Accessory* Controller::findAccessory(t_aoap_accessory_param *pAccessoryParam)
{
    if (!pAccessoryParam)
    {
        dbgPrintLine(eLogError, "findAccessory() ERROR: accessory parameters are NULL");
        return NULL;
    }
    if ((pAccessoryParam->manufacturer == NULL)
            || (pAccessoryParam->modelName == NULL)
            || (pAccessoryParam->description == NULL)
            || (pAccessoryParam->version == NULL)
            || (pAccessoryParam->uri == NULL)
            || (pAccessoryParam->serial == NULL))
    {
        for (unsigned int i = 0; i < mAccessories.size(); i++)
        {
            if (mAccessories[i]
                    && pAccessoryParam->enableAudio
                    && (pAccessoryParam->enableAudio == mAccessories[i]->getAudio())
                    && mAccessories[i]->getManufacturer().empty()
                    && mAccessories[i]->getModelName().empty()
                    && mAccessories[i]->getDescription().empty()
                    && mAccessories[i]->getVersion().empty()
                    && mAccessories[i]->getUri().empty()
                    && mAccessories[i]->getSerial().empty())
            {
                return mAccessories[i];
            }
        }
    }

    for (unsigned int i = 0; i < mAccessories.size(); i++)
    {
        if (mAccessories[i]
                && (0 == strcmp(mAccessories[i]->getManufacturer().c_str(), pAccessoryParam->manufacturer))
                && (0 == strcmp(mAccessories[i]->getModelName().c_str(), pAccessoryParam->modelName))
                && (0 == strcmp(mAccessories[i]->getDescription().c_str(), pAccessoryParam->description))
                && (0 == strcmp(mAccessories[i]->getVersion().c_str(), pAccessoryParam->version))
                && (0 == strcmp(mAccessories[i]->getUri().c_str(), pAccessoryParam->uri))
                && (0 == strcmp(mAccessories[i]->getSerial().c_str(), pAccessoryParam->serial)))
        {
            return mAccessories[i];
        }
    }

    return NULL;
}

} }/* namespace AOAP { namespace Controller { */
