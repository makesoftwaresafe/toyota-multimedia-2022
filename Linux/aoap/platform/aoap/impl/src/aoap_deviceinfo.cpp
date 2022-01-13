/*
 * aoap_deviceinfo.cpp
 *
 *  Created on: Aug 14, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_deviceinfo.h"
#include "aoap_types.h"
#include <cstdlib> //to have NULL defined
#include "aoap_logging.h"
#include "aoap_usbmonitor.h"
#include <cstring>

using namespace AOAP::Logging;

/*static*/ DeviceInfo *DeviceInfo::mpSelf = NULL;

/*static*/ DeviceInfo* DeviceInfo::getInstance(void)
{
    if (!mpSelf)
    {
        mpSelf = new DeviceInfo();
    }
    return mpSelf;
}

/*static*/ void DeviceInfo::deleteInstance(void)
{
    if (mpSelf)
    {
        delete mpSelf;
        mpSelf = NULL;
    }
}

int DeviceInfo::getDevNum(unsigned int vendorId, unsigned int productId, const std::string& serial)
{
    dbgPrintLine(eLogDebug, "getDevNum(id=%.4x:%.4x, s/n='%s')", vendorId, productId, serial.c_str());
    if (!createUdev())
    {
        return AOAP_ERROR_CREATING_UDEV;
    }

    int devNum = -1;
    unsigned int currentIdVendor = 0;
    unsigned int currentIdProduct = 0;
    char *pCurrentSerial = NULL;
    udev_list_entry *pDevListEntry;

    udev_enumerate *pEnumerate = udev_enumerate_new(mpUdev);
    udev_enumerate_add_match_subsystem(pEnumerate, "usb");
    udev_enumerate_scan_devices(pEnumerate);
    udev_list_entry *pDevices = udev_enumerate_get_list_entry(pEnumerate);
    udev_list_entry_foreach(pDevListEntry, pDevices)
    {
        const char *pPath = udev_list_entry_get_name(pDevListEntry);
        struct udev_device *org;
        udev_device *pDevice;
        org = pDevice = udev_device_new_from_syspath(mpUdev, pPath);
        pDevice = udev_device_get_parent_with_subsystem_devtype(pDevice, "usb", "usb_device");

        currentIdVendor = UsbMonitor::convertAsciiToDecimal(udev_device_get_sysattr_value(pDevice, "idVendor"));
        currentIdProduct = UsbMonitor::convertAsciiToDecimal(udev_device_get_sysattr_value(pDevice, "idProduct"));
        pCurrentSerial = const_cast<char*>(udev_device_get_sysattr_value(pDevice, "serial"));

        dbgPrintLine(eLogDebug, "getDevNum(id=%.4x:%.4x, s/n='%s') checking id=%.4x:%.4x s/n='%s'",
                vendorId, productId, serial.c_str(),
                currentIdVendor, currentIdProduct, pCurrentSerial);

        if ((vendorId == currentIdVendor) && (productId == currentIdProduct)
                && pCurrentSerial && (0 == strcmp(pCurrentSerial, serial.c_str())))
        {
            devNum = (unsigned int)udev_device_get_devnum(pDevice);
        }

        udev_device_unref(org);

        if (devNum > -1)
        {
            dbgPrintLine(eLogDebug, "getDevNum(id=%.4x:%.4x, s/n='%s') device found with num=%d",
                    vendorId, productId, serial.c_str(), devNum);
            break; //one found devNum is ok
        }
    }

    udev_enumerate_unref(pEnumerate);

    if (devNum > -1)
    {
        return devNum;
    }
    else
    {
        return AOAP_ERROR_NO_DEVICE;
    }
}

DeviceInfo::DeviceInfo(void)
: mpUdev(NULL)
{
    //Nothing to do
}

DeviceInfo::~DeviceInfo(void)
{
    if (mpUdev)
    {
        udev_unref(mpUdev);
        mpUdev = NULL;
    }
}

bool DeviceInfo::createUdev(void)
{
    if (!mpUdev)
    {
        mpUdev = udev_new();
    }

    if (!mpUdev)
    {
        dbgPrintLine(eLogFatal, "createUdev() FATAL ERROR: Couldn't create udev");
        return false;
    }

    return true;
}
