/*
 * aoap_usbmonitor.cpp
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_usbmonitor.h"
#include "aoap_logging.h"
#include "aoap_devicelist.h"
#include <cstdlib> //to have NULL defined
#include <sstream>
#include <iostream>
#include "string.h"
#include <sys/prctl.h>

using namespace AOAP::Logging;

/*static*/UsbMonitor* UsbMonitor::gpSelf = NULL;
/*static*/bool UsbMonitor::gQuit = false;

/*static*/UsbMonitor* UsbMonitor::getInstance(void)
{
    if (!gpSelf)
    {
        gpSelf = new UsbMonitor();
    }

    return gpSelf;
}

/*static*/void UsbMonitor::deleteInstance(void)
{
    if (gpSelf)
    {
        delete gpSelf;
        gpSelf = NULL;
    }
}

void UsbMonitor::initMonitoring(void)
{
    if (!mpUdev)
    {
        mpUdev = udev_new();
        if (!mpUdev)
        {
            dbgPrintLine(eLogFatal, "%s() FATAL ERROR: Couldn't create udev", __FUNCTION__);
            return;
        }
    }

    if (!mpMonitor)
    {
        dbgPrintLine(eLogInfo, "%s() Init udev monitoring", __FUNCTION__);

        mpMonitor = udev_monitor_new_from_netlink(mpUdev, mpUdevEventNameSource);
        if (mpMonitor)
        {
            int result = udev_monitor_filter_add_match_subsystem_devtype(mpMonitor, "usb",
                    "usb_device"); //filter for usb_devices only
            if (0 == result)
            {
                result = udev_monitor_enable_receiving(mpMonitor);
                if (0 == result)
                {
                    mMonitorFd = udev_monitor_get_fd(mpMonitor);

                    startMonitoringThread();
                }
                else
                {
                    dbgPrintLine(eLogError,
                            "%s() ERROR: Failed to bind socket to event source with code=%d",
                            __FUNCTION__, result);
                }
            }
            else
            {
                dbgPrintLine(eLogError, "%s() ERROR: Failed to add filter with code=%d",
                        __FUNCTION__, result);
            }
        }
        else
        {
            dbgPrintLine(eLogError,
                    "%s() ERROR: Failed to create udev monitor and connect to event source=%s",
                    __FUNCTION__, mpUdevEventNameSource);
        }
    }
    else
    {
        dbgPrintLine(eLogDebug,
                "%s() Monitoring is already running => nothing to do", __FUNCTION__);
    }
}

/*static*/ unsigned int UsbMonitor::convertAsciiToDecimal(const char *pValue)
{
    if (pValue)
    {
        //DbgPrintLine(eLogVerbose, "convertAsciiToDecimal('%s')", p_value);
        return static_cast<unsigned int>(strtol(pValue, NULL, 16));
    }
    else
    {
        dbgPrintLine(eLogDebug, "%s() string is NULL", __FUNCTION__);
        return 0;
    }
}

UsbMonitor::UsbMonitor(void)
: mpUdev(NULL)
, mpMonitor(NULL)
, mMonitorThread(0)
, mMonitorFd(-1)
, mMonitorEventFd(-1)
, mSelectTimeout(3) //3 seconds timeout
, mpUdevEventNameSource("udev")
, mpGlobalObserver(DeviceList::getInstance())
{
    dbgPrintLine(eLogInfo, "%s() Create UsbMonitor", __FUNCTION__);
    registerObserver(mpGlobalObserver);
}

UsbMonitor::~UsbMonitor(void)
{
    dbgPrintLine(eLogDebug, "%s() Delete UsbMonitor", __FUNCTION__);
    stopMonitoring();
    unregisterObserver(mpGlobalObserver);
    DeviceList::deleteInstance();
}

void UsbMonitor::stopMonitoring(void)
{
    /* event to tear shutdown */
    uint64_t event = 100;

    gQuit = true;
    dbgPrintLine(eLogInfo, "%s()", __FUNCTION__);

    if (mpMonitor)
    {
        if (mMonitorEventFd >= 0)
        {
            if (eventfd_write(mMonitorEventFd, event) != 0){
                dbgPrintLine(eLogError, "%s() ERROR: eventfd_write(fd=%d) failed", __FUNCTION__, mMonitorEventFd);
            }
        }
        else
        {
            dbgPrintLine(eLogError, "%s() ERROR: mMonitorEventFd invalid =%d", __FUNCTION__, mMonitorEventFd);
        }
        pthread_join(mMonitorThread, NULL); //joining takes as long as 'select'/'udev_monitor_receive_device' is blocking

        (void)udev_monitor_unref(mpMonitor);
        mpMonitor = NULL;
    }

    if (mpUdev)
    {
        (void)udev_unref(mpUdev); //causes segmentation fault
        mpUdev = NULL;
    }

    if (mMonitorEventFd >= 0)
    {
        close(mMonitorEventFd);
        mMonitorEventFd = -1;
    }
    dbgPrintLine(eLogInfo, "%s() done", __FUNCTION__);
}

void UsbMonitor::startMonitoringThread(void)
{
    gQuit = false;
    /* create event file descriptor */
    mMonitorEventFd = eventfd(0, 0);
    /* create & start UsbMonitor */
    if (0 != pthread_create(&mMonitorThread, NULL, &(UsbMonitor::monitor), this))
    {
        dbgPrintLine(eLogError, "%s() ERROR: Failed to start USB monitoring", __FUNCTION__);
    }
}

/*static*/void* UsbMonitor::monitor(void* pToken)
{
    if (static_cast<UsbMonitor*>(pToken))
    {
        static_cast<UsbMonitor*>(pToken)->monitor();
    }
    return NULL;
}

void UsbMonitor::monitor(void)
{
    if (!mpMonitor)
    {
        dbgPrintLine(eLogError, "%s() ERROR: mpMonitor is NULL!", __FUNCTION__);
        return;
    }

    prctl(PR_SET_NAME, "aoapMonitor", 0, 0, 0);

    int selectResult = 0;
    fd_set fds;
    int nfds = 0;   /* highest-numbered file descriptor in any of the three sets, plus 1 */
    struct timeval tv;
    struct udev_device *pDevice;

    while (!gQuit)
    {
        FD_ZERO(&fds);
        FD_SET(mMonitorFd, &fds);
        FD_SET(mMonitorEventFd, &fds);
        tv.tv_sec = mSelectTimeout; //let select block the defined time
        tv.tv_usec = 0;

        if (mMonitorFd > mMonitorEventFd) {
            nfds = mMonitorFd;
        } else {
            nfds = mMonitorEventFd;
        }

        selectResult = select(nfds+1, &fds, NULL, NULL, &tv);
        if ((selectResult > 0) && FD_ISSET(mMonitorFd, &fds))
        {
            pDevice = udev_monitor_receive_device(mpMonitor);
            if (pDevice)
            {
                dbgPrintLine(eLogInfo, "%s() Action: %s, notified at Node: %s",
                             __FUNCTION__, udev_device_get_action(pDevice),
                             udev_device_get_devnode(pDevice));
                dbgPrintLine(eLogInfo, "   ID: %s:%s Serial: %s ",
                             udev_device_get_sysattr_value(pDevice, "idVendor"),
                             udev_device_get_sysattr_value(pDevice, "idProduct"),
                             udev_device_get_sysattr_value(pDevice, "serial"));
                dbgPrintLine(eLogDebug, "   Manufacturer: %s, Product: %s, DevNum: %d",
                             udev_device_get_sysattr_value(pDevice, "manufacturer"),
                             udev_device_get_sysattr_value(pDevice, "product"),
                             (int) udev_device_get_devnum(pDevice));

                std::string serialString = "";
                const char *serial = udev_device_get_sysattr_value(pDevice, "serial");
                UsbMonitor::tUsbAction action = getAction( udev_device_get_action(pDevice));
                if (serial)
                {
                    serialString = serial;
                }
                if (serial || (action == USB_REMOVE)) // handle if with serial or if remove
                {
                    if (action != USB_UNKNOWN)
                    {
                        notify((action == USB_ADD) ? true : false,
                                convertAsciiToDecimal(udev_device_get_sysattr_value(pDevice, "idVendor")),
                                convertAsciiToDecimal(udev_device_get_sysattr_value(pDevice, "idProduct")),
                                serialString,
                                udev_device_get_devnum(pDevice));
                    }
                    else
                    {
                        dbgPrintLine(eLogInfo,
                                "%s() Unknown action %s for device DevNum: %d",
                                __FUNCTION__, udev_device_get_action(pDevice),
                                (int)udev_device_get_devnum(pDevice));
                    }
                }
                else
                {
                    dbgPrintLine(eLogInfo,
                            "%s() Ignoring device %s:%s without serial",
                            __FUNCTION__,
                            udev_device_get_sysattr_value(pDevice, "idVendor"),
                            udev_device_get_sysattr_value(pDevice,
                                    "idProduct"));
                }
                udev_device_unref(pDevice);
                pDevice = NULL;
            }
            else
            {
                dbgPrintLine(eLogError,
                        "%s() ERROR: No device from receive_device -> leave monitoring",
                        __FUNCTION__);
                break;
            }
        }
        else if ((selectResult > 0) && (FD_ISSET(mMonitorEventFd, &fds))) {
            uint64_t event = 0;
            if (eventfd_read(mMonitorEventFd, &event) != 0) {
                dbgPrintLine(eLogError, "%s() ERROR: eventfd_read(fd=%d) failed", __FUNCTION__, mMonitorEventFd);
            }
            dbgPrintLine(eLogDebug, "%s() eventfd triggered to exit UsbMonitor", __FUNCTION__);
        }
        else if (selectResult < 0)
        {
            dbgPrintLine(eLogError, "%s() ERROR: select returned error with code=%d",
                    __FUNCTION__, selectResult);
        }
        else
        {
            //Timeout, nothing to do
        }
    }
    dbgPrintLine(eLogDebug, "%s() exit monitor thread", __FUNCTION__);
}

UsbMonitor::tUsbAction UsbMonitor::getAction(const char *pAction)
{
    if (0 == strcasecmp(pAction, "add"))
    {
        return USB_ADD;
    }
    else if (0 == strcasecmp(pAction, "remove"))
    {
        return USB_REMOVE;
    }
    else
    {
        return USB_UNKNOWN;
    }
}
