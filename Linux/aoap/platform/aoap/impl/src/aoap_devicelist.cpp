/*
 * aoap_devicelist.cpp
 *
 *  Created on: Sep 11, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_devicelist.h"
#include "aoap_device.h"
#include "aoap_tools.h"
#include "aoap_logging.h"
#include <cstring>
#include <cstdlib> //to have NULL defined

using namespace AOAP::Logging;

/*static*/DeviceList* DeviceList::gpSelf = NULL;

/*static*/ DeviceList* DeviceList::getInstance(void)
{
    if (!gpSelf)
    {
        gpSelf = new DeviceList();
    }

    return gpSelf;
}

/*static*/void DeviceList::deleteInstance(void)
{
    if (gpSelf)
    {
        dbgPrintLine(eLogDebug, "%s() Deleting DeviceList singleton", __FUNCTION__);
        delete gpSelf;
        gpSelf = NULL;
    }
}

bool DeviceList::getLock()
{
    dbgPrintLine(eLogDebug, "%s() --> Waiting for lock", __FUNCTION__);

    /*PRQA: Lint Message 454: This is intention. Mutex will be unlocked in releaseLock() */
    /*lint -save -e454*/
    int result = pthread_mutex_lock(&mMutex);
    if (0 != result)
    {
        dbgPrintLine(eLogError, "%s() Failed to lock mutex with code=%d", __FUNCTION__, result);
        return false;
    }

    dbgPrintLine(eLogDebug, "%s() <-- Got lock", __FUNCTION__);
    return true;
    /*lint -restore*/
}

bool DeviceList::releaseLock()
{
    dbgPrintLine(eLogDebug, "%s()", __FUNCTION__);

    /*PRQA: Lint Message 455: This is intention. Mutex will be locked in getLock() */
    /*lint -save -e455*/
    int result = pthread_mutex_unlock(&mMutex);
    if (0 != result)
    {
        dbgPrintLine(eLogError, "%s() Failed to unlock mutex with code=%d", __FUNCTION__, result);
        return false;
    }
    return true;
    /*lint -restore*/
}

void DeviceList::removeDevice(std::shared_ptr<Device> pDevice)
{
    if (pDevice)
    {
        for (std::vector<std::shared_ptr<Device>>::iterator p = mDevices.begin(); p != mDevices.end(); p++)
        {
            if ((pDevice == *p) && (pDevice->getId() == (*p)->getId()))
            {
                dbgPrintLine(eLogDebug, "%s() device %d found => remove it now",
                        __FUNCTION__, (*p)->getId());
                mDevices.erase(p);
                break;
            }
        }
    }
}

std::shared_ptr<Device> DeviceList::findDevice(unsigned int vendorId, unsigned int productId,
        const std::string& serial)
{
    std::shared_ptr<Device> retDevice (nullptr);

    for (unsigned int i = 0; i < mDevices.size(); i++)
    {
        if (mDevices[i] && (mDevices[i]->getProductId()
                == productId) && (mDevices[i]->getVendorId()
                == vendorId) && (0 == std::strcmp(serial.c_str(),
                        mDevices[i]->getSerial().c_str())))
        {
            retDevice = mDevices[i];
            break;
        }
    }

    return retDevice;
}

std::shared_ptr<Device> DeviceList::findDeviceByDeviceNumber(unsigned int devNum)
{
    std::shared_ptr<Device> retDevice (nullptr);

    for (unsigned int i = 0; i < mDevices.size(); i++)
    {
        if (mDevices[i] && (mDevices[i]->getDevNum() == devNum))
        {
            retDevice = mDevices[i];
            break;
        }
    }

    return retDevice;
}

std::shared_ptr<Device> DeviceList::findDeviceBySerial(const std::string& serial)
{
    std::shared_ptr<Device> retDevice (nullptr);

    for (unsigned int i = 0; i < mDevices.size(); i++)
    {
        if (mDevices[i] && (0 == strcmp(mDevices[i]->getSerial().c_str(), serial.c_str())))
        {
            retDevice = mDevices[i];
            break;
        }
    }

    return retDevice;
}

std::shared_ptr<Device> DeviceList::findDevice(unsigned int device_id)
{
    std::shared_ptr<Device> retDevice (nullptr);

    for (unsigned int i = 0; i < mDevices.size(); i++)
    {
        if (mDevices[i] && (mDevices[i]->getId() == device_id))
        {
            retDevice = mDevices[i];
            break;
        }
    }
    return retDevice;
}

void DeviceList::update(bool attach, unsigned int vendorId,
            unsigned int productId, const std::string &serial,
            unsigned int devNum)
{
    bool checkAccessoryMode = false;

    //Acquire lock to update device
    if (!getLock())
    {
        dbgPrintLine(eLogError, "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) ERROR: Acquiring lock",
                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                serial.c_str(), devNum);
        return;
    }

    //First find device by devNum
    std::shared_ptr<Device> pDevice = findDeviceByDeviceNumber(devNum);
    if (pDevice)
    {
        if (attach)
        {
            //Device re-attached
            dbgPrintLine(eLogError,
                    "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d ERROR: existing device re-attached => do nothing",
                    __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                    serial.c_str(), devNum, pDevice->getId());
        }
        else //removed
        {
            if (pDevice->getState() != Device::AOAP_DEVICE_STATE_SWITCHING)
            {
                //Device not switching to accessory mode then we can simply remove it
                dbgPrintLine(eLogInfo,
                        "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d => remove device",
                        __FUNCTION__, attach ? "attach" : "detach",
                        pDevice->getVendorId(), pDevice->getProductId(), pDevice->getSerial().c_str(), devNum, pDevice->getId());
                removeDevice(pDevice);
                pDevice = nullptr;
            }
            else
            {
                //Device is switching to accessory, simply wait for the attach signal
                dbgPrintLine(eLogInfo,
                        "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d switching device removed => nothing to do",
                        __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                        serial.c_str(), devNum, pDevice->getId());
            }
        }
    }
    else //device not found by devNum
    {
        //Do we have this device already?
        pDevice = findDeviceBySerial(serial);
        if (pDevice)
        {
            //Device is already present
            if (attach)
            {
                bool devNumUpdated = false; //only set to true when changing. Not when setting the first time

                //Check if devNum is correct
                if (!pDevice->getDevNumSet() || (pDevice->getDevNum() != devNum))
                {
                    if (!pDevice->getDevNumSet())
                    {
                        dbgPrintLine(eLogDebug, "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d Set devNum first time",
                                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                                serial.c_str(), devNum, pDevice->getId());
                    }
                    else
                    {
                        dbgPrintLine(eLogDebug, "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d devNum changed -> update and store prev devNum=%d",
                                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                                serial.c_str(), devNum, pDevice->getId(), pDevice->getDevNum());
                        devNumUpdated = true;

                        pDevice->setPreviousDevNum(pDevice->getDevNum()); //store here the current device number as the previous
                    }

                    pDevice->setDevNum(devNum);
                }

                //Has vendor ID changed?
                if (pDevice->getVendorId() != vendorId)
                {
                    dbgPrintLine(eLogDebug,
                         "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d Update vendor id from 0x%x",
                         __FUNCTION__, attach ? "attach" : "detach", vendorId, productId, serial.c_str(),
                         devNum, pDevice->getId(), pDevice->getVendorId());
                    pDevice->setVendorId(vendorId);
                    pDevice->resetInterface();
                }

                //Has product ID changed?
                if (pDevice->getProductId() != productId)
                {
                    dbgPrintLine(eLogDebug,
                         "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d Update product id from 0x%x",
                         __FUNCTION__, attach ? "attach" : "detach", vendorId, productId, serial.c_str(),
                         devNum, pDevice->getId(), pDevice->getProductId());
                    pDevice->setProductId(productId);
                    pDevice->resetInterface();
                }

                //Switched to accessory mode?
                if (pDevice->getState() == Device::AOAP_DEVICE_STATE_SWITCHING)
                {
                    if (devNumUpdated && !AOAP::Tools::isAccessory(vendorId, productId))
                    {
                        //Check if we have switched to accessory mode
                        dbgPrintLine(eLogError,
                                "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) ERROR: id=%d switching device re-attached with changed devNum but not in accessory mode => abort",
                                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                                serial.c_str(), devNum, pDevice->getId());
                        checkAccessoryMode = true;
                    }
                    else if (devNumUpdated)
                    {
                        dbgPrintLine(eLogInfo,
                                "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d device in accessory mode found => finish switching",
                                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                                serial.c_str(), devNum, pDevice->getId());
                        checkAccessoryMode = true;
                    }
                    else
                    {
                        dbgPrintLine(eLogInfo,
                                "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d device notification received for already present device (device notification after connect?)",
                                __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                                serial.c_str(), devNum, pDevice->getId());
                        //checkAccessoryMode is already false, nothing to do
                    }
                }
                else
                {
                    dbgPrintLine(eLogInfo,
                        "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d is already present but not switching -> nothing to do",
                        __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                        serial.c_str(), devNum, pDevice->getId());
                }
            }
            else //remove
            {
                if (pDevice->getDevNumSet())
                {
                    //When the devNum matches to the previous device number, we can assume that the udev events are out of order => ignore removal
                    if ((devNum < pDevice->getDevNum()) && (pDevice->getPreviousDevNum() == devNum))
                    {
                        dbgPrintLine(eLogError,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d ERROR: device found by s/n but not by devNum, devNum matches to previous devNum=%d => assume udev events are in wrong order and ignore",
                            __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                            serial.c_str(), devNum, pDevice->getId(), pDevice->getPreviousDevNum());
                    }
                    else
                    {
                        dbgPrintLine(eLogError,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d ERROR: device found by s/n but not by devNum => remove device",
                            __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                            serial.c_str(), devNum, pDevice->getId());
                        removeDevice(pDevice);
                        pDevice = nullptr;
                    }
                }
                else if (Device::AOAP_DEVICE_STATE_SWITCHING == pDevice->getState())
                {
                    dbgPrintLine(eLogWarn,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d WARNING: Switching device has not set devNum => update and do not remove",
                            __FUNCTION__);
                    pDevice->setDevNum(devNum);
                }
                else
                {
                    dbgPrintLine(eLogError,
                        "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) id=%d ERROR: device found by s/n but devNum not set => remove device",
                        __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                        serial.c_str(), devNum, pDevice->getId());
                    removeDevice(pDevice);
                    pDevice = nullptr;
                }
            }
        }
        else //no device
        {
            if (attach)
            {
                //New device
                if (AOAP::Tools::isAccessory(vendorId, productId))
                {
                    dbgPrintLine(eLogInfo,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) New device in accessory mode found -> create it",
                            __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                            serial.c_str(), devNum);
                }
                else
                {
                    dbgPrintLine(eLogDebug,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) => create new device",
                            __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                            serial.c_str(), devNum);
                }

                pDevice = std::make_shared<Device>(vendorId, productId, serial, devNum);
                if (!pDevice)
                {
                    dbgPrintLine(eLogFatal,
                            "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) FATAL ERROR: Failed to create new device",
                            __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                            serial.c_str(), devNum);
                }
                else
                {
                    addDevice(pDevice);
                }
            }
            else
            {
                dbgPrintLine(eLogWarn,
                        "%s(%s, %.4x:%.4x, s/n='%s', devNum=%d) WARNING: non-listed device removed => nothing to do",
                        __FUNCTION__, attach ? "attach" : "detach", vendorId, productId,
                        serial.c_str(), devNum);
            }
        }
    }

    //Device got updated, lock isn't required anymore
    releaseLock();

    //Finally switch to accessory mode
    if (checkAccessoryMode && pDevice)
    {
        /* checkAccessoryMode was set to true if we were notified by udev about attach.
         * If checkAccessoryMode is true, pDevice was found by findDeviceBySerial() internally.
         * Both things indicate that the device must be available. */
        pDevice->finishSwitching();
    }
}

DeviceList::DeviceList(void)
{
    dbgPrintLine(eLogDebug, "Construct %s creating this=%p", __FUNCTION__, this);
    if (0 != pthread_mutex_init(&mMutex, NULL))
    {
        dbgPrintLine(eLogError, "%s() ERROR: Failed to initialize mutex", __FUNCTION__);
    }
}

DeviceList::~DeviceList(void)
{
    dbgPrintLine(eLogDebug, "%s() deleting this=%p", __FUNCTION__, this);
    getLock();
    clearDevices();
    /* unlock mutex and destroy mutex */
    releaseLock();
    pthread_mutex_destroy(&mMutex);
}

void DeviceList::clearDevices(void)
{
    while (mDevices.size() > 0)
    {
        mDevices.pop_back();
    }
}
