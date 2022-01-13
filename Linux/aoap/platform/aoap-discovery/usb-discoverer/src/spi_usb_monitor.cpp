
/************************************************************************
 *
 * \file: spi_usb_monitor.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * <brief description>.
 * <detailed description>
 * \component: SPI Discovery
 *
 * \author: D. Girnus / ADIT/SW2 / dgirnus@de.adit-jv.com
 *
 * \copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/


/* *************  includes  ************* */

#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <errno.h>
#include <inttypes.h>

#include <adit_logging.h>
#include "spi_usb_monitor.h"
#include "spi_usb_discoverer_eventlist.h"

/* *************  defines  ************* */

LOG_IMPORT_CONTEXT(spi_usb_discoverer)


/*static*/UsbDiscovererUdevMonitor* UsbDiscovererUdevMonitor::pmUsbMonitor = NULL;


/* *************  function  ************* */

/*static*/UsbDiscovererUdevMonitor* UsbDiscovererUdevMonitor::getInstance(void) {
    if (!pmUsbMonitor) {
        pmUsbMonitor = new UsbDiscovererUdevMonitor();
    }
    return pmUsbMonitor;
}

/*static*/void UsbDiscovererUdevMonitor::deleteInstance(void) {
    if (pmUsbMonitor) {
        delete (pmUsbMonitor);
        pmUsbMonitor = NULL;
    }
}

UsbDiscovererUdevMonitor::UsbDiscovererUdevMonitor(void) {
    memset(&mCallbacks, 0, sizeof(mCallbacks));

    mContext = nullptr;

    mUsbMonitorThreadId = 0;

    mpUdev = nullptr;
    mpMonitor = nullptr;
    mUdevMonitorFd = -1;

    mEventFd = -1;

    mShutdown = false;
}

UsbDiscovererUdevMonitor::~UsbDiscovererUdevMonitor() {

    /* Call of stopMonitoring() is guaranteed by destructor of class UsbDiscoverer. */
}

int UsbDiscovererUdevMonitor::startMonitoring(void* context, SpiUsbDiscovererCallbacks *pCallbacks) {
    int res = SPI_USB_DISCOVERY_SUCCESS;

    /* If already monitoring, then do nothing */
    if (mpMonitor) {
        LOG_INFO((spi_usb_discoverer, "already monitoring"));
        return SPI_USB_DISCOVERY_SUCCESS;
    }

    memcpy(&mCallbacks, pCallbacks, sizeof(mCallbacks));
    mContext = context;

    /* Initialize udev if necessary */
    if (!mpUdev) {
        mpUdev = udev_new();
        if (!mpUdev) {
            LOG_ERROR((spi_usb_discoverer, "create new udev failed"));
            return SPI_USB_DISCOVERY_UDEV_ERROR;
        }
    }

    /* Initialize the udev monitor */
    if (true != startUdevMonitor()) {
        LOG_ERROR((spi_usb_discoverer, "startUdevMonitor() failed"));
        udev_monitor_unref(mpMonitor);
        mpMonitor = nullptr;
        return SPI_USB_DISCOVERY_UDEV_ERROR;
    }

    /* create eventFd to send stop event */
    if (0 > (mEventFd = eventfd(0, 0))) {
        LOG_ERROR((spi_usb_discoverer, "create EventFd failed=%d, errno=%d", mEventFd, errno));
        return SPI_USB_DISCOVERY_ERROR;
    }

    /* Set flag to false before creating usbMonitorThread */
    mShutdown = false;

    /* create monitorThread for SPI USB discovery */
    if (SPI_USB_DISCOVERY_SUCCESS != pthread_create(&mUsbMonitorThreadId, nullptr, &usbMonitorThread, this)) {
        LOG_ERROR((spi_usb_discoverer, "create monitorThread failed"));
        udev_monitor_unref(mpMonitor);
        mpMonitor = NULL;
        return SPI_USB_DISCOVERY_ERROR;
    } else {
        /* created monitorThread */
        LOGD_DEBUG((spi_usb_discoverer, "monitorThread created"));
    }

    return res;
}

int UsbDiscovererUdevMonitor::stopMonitoring(void) {
    int res = SPI_USB_DISCOVERY_SUCCESS;
    uint64_t event = EVENTFD_EVENT_CHECK_LIST;

    bool wasShutdown = mShutdown;
    /* Check if stopMonitoring was already called. */
    if (true == wasShutdown) {
        /* Inform about the incorrect usage, but continue to stop monitoring. */
        LOG_WARN((spi_usb_discoverer, "stopMonitoring() was already called"));
        return res;
    }
    mShutdown = true;

    if (mEventFd >= 0) {
        /* Ownership is hand over to event list */
        t_monitorEventListItem* stopEventItem = new t_monitorEventListItem(EventStop, nullptr);
        /* add stop event to event list */
        if (SPI_USB_DISCOVERY_SUCCESS == (res = EventList::getInstance()->addEventItem(stopEventItem))) {
            /* trigger eventfd */
            if (eventfd_write(mEventFd, event) != 0){
                LOG_ERROR((spi_usb_discoverer, "stopMonitoring() eventfd_write(fd=%d) failed", mEventFd));
            } else {
                LOGD_VERBOSE((spi_usb_discoverer, "stopMonitoring() eventfd_write(fd=%d) to stop spiUsbMonitor", mEventFd));
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "stopMonitoring() Add EventStop failed=%d", res));
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "stopMonitoring() mEventFd=%d is invalid", mEventFd));
    }

    if (mUsbMonitorThreadId > 0) {
        pthread_join(mUsbMonitorThreadId, nullptr);
        mUsbMonitorThreadId = 0;
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

    return res;
}

int UsbDiscovererUdevMonitor::resetMonitoredDevice(t_usbDeviceInformation* inUsbDevParam)
{
    int res = SPI_USB_DISCOVERY_SUCCESS;

    if (UsbDeviceList::getInstance()->getLock()) {
        UsbDevice *pUsbDevice = UsbDeviceList::getInstance()->findDevice(inUsbDevParam->vendorId, inUsbDevParam->productId, inUsbDevParam->serial);
        if (pUsbDevice) {
            res = pUsbDevice->reset(inUsbDevParam);
            if (SPI_USB_DISCOVERY_SUCCESS == res) {
                LOGD_DEBUG((spi_usb_discoverer, "resetDevice() Reset UsbDevice done"));
            }
        } else {
            LOG_WARN((spi_usb_discoverer, "resetDevice() Cannot find UsbDevice with serial=%s.", inUsbDevParam->serial.c_str()));
            res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
        }

        UsbDeviceList::getInstance()->releaseLock();
    }
    return res;
}

struct udev_enumerate* UsbDiscovererUdevMonitor::enumerateUdevDeviceList(struct udev_list_entry** outUdevDeviceList)
{
    struct udev_enumerate*  pEnumerate = nullptr;

    /* Create a list of the devices in the 'usb' subsystem. */
    pEnumerate = udev_enumerate_new(mpUdev);
    if (nullptr != pEnumerate) {
        if (SPI_USB_DISCOVERY_SUCCESS == udev_enumerate_add_match_subsystem(pEnumerate, STR_USB)) {
            udev_enumerate_add_match_sysattr(pEnumerate, STR_IDVENDOR, nullptr);
            udev_enumerate_add_match_sysattr(pEnumerate, STR_IDPRODUCT, nullptr);
            udev_enumerate_add_match_sysattr(pEnumerate, STR_SERIAL, nullptr);
            udev_enumerate_add_match_sysattr(pEnumerate, STR_DEVIVE_CLASS, nullptr);
            udev_enumerate_add_match_sysattr(pEnumerate, STR_DEVIVE_SUB_CLASS, nullptr);
            udev_enumerate_add_match_sysattr(pEnumerate, STR_DEVIVE_PROTOCOL, nullptr);

            if (SPI_USB_DISCOVERY_SUCCESS == udev_enumerate_scan_devices(pEnumerate)) {
                *outUdevDeviceList = udev_enumerate_get_list_entry(pEnumerate);
            } else {
                LOG_ERROR((spi_usb_discoverer, "scan devices failed"));
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "add match subsystem() failed"));
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "udev_enumerate_new() failed"));
    }

    if (*outUdevDeviceList == nullptr) {
        LOG_ERROR((spi_usb_discoverer, "scan found no devices"));
    }

    return pEnumerate;
}

int UsbDiscovererUdevMonitor::getDevPath(struct udev_device* inUdevDevice) {
    int devPath = -1;

    const char* strDevPath = udev_device_get_sysattr_value(inUdevDevice, STR_DEVPATH);
    if (NULL != strDevPath) {
        sscanf(strDevPath, "%x", &devPath);
    } else {
        LOG_WARN((spi_usb_discoverer, "Could not get udev device sysattr '%s'.", STR_DEVPATH));
    }

    /* The devpath of an USB controller is = 0.
     * The first possible device can be on devpath = 1.
     * Check the devices which has a devpath = 1 and the vendorId. */
    return devPath;
}

std::shared_ptr<t_usbDeviceInformation> UsbDiscovererUdevMonitor::createDevInfo(struct udev_device* inUdevDevice) {
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
        const char* strDevClassString      = udev_device_get_sysattr_value(inUdevDevice, STR_DEVIVE_CLASS);
        const char* strDevSubClassString   = udev_device_get_sysattr_value(inUdevDevice, STR_DEVIVE_SUB_CLASS);
        const char* strDevProtocolString   = udev_device_get_sysattr_value(inUdevDevice, STR_DEVIVE_PROTOCOL);
        /* Base Class 09h (Hub)  is defined for devices that are USB hubs
         * and conform to the definition in the USB specification. */
        if ( (NULL != strDevClassString) && (NULL != strDevSubClassString) && (NULL != strDevProtocolString) ) {
            uint32_t devClass = 0, devSubClass = 0, devProt = 0;
            sscanf(strDevClassString,    "%x", &devClass);
            sscanf(strDevSubClassString, "%x", &devSubClass);
            sscanf(strDevProtocolString, "%x", &devProt);
            /* If the device is an USB hub, do not add the device
             * to the internal device list and avoid check for AOAP support */
            if ((devClass == USB_HUB_BASE_CLASS) && (devSubClass == USB_HUB_SUB_CLASS)) {
                LOG_INFO((spi_usb_discoverer, "createDevInfo() Found a USB Hub (DevClass=%d, DevSubClass=%d, DevProtocol=%d)",
                        devClass, devSubClass, devProt));
            } else {
                /* Device is not a USB Hub, but it could be some Unwired (Hub) Technology.
                 * Continue to get the USB device sysattr_values */

                const char* strVendorIdString      = udev_device_get_sysattr_value(inUdevDevice, STR_IDVENDOR);
                const char* strProductIdString     = udev_device_get_sysattr_value(inUdevDevice, STR_IDPRODUCT);
                const char* strSerialString        = udev_device_get_sysattr_value(inUdevDevice, STR_SERIAL);
                const char* strProductString       = udev_device_get_sysattr_value(inUdevDevice, STR_PRODUCT);
                const char* strManufacturerString  = udev_device_get_sysattr_value(inUdevDevice, STR_MANUFACTURER);
                const char* strSysPathString       = udev_device_get_syspath(inUdevDevice);
                if ( (NULL != strVendorIdString) && (NULL != strProductIdString)
                     && (NULL != strSerialString) && (NULL != strProductString)
                     && (NULL != strManufacturerString) /*&& (NULL != strDevNumString)*/
                     && (NULL != strSysPathString) ) {

                    /* Check if the device is part of the Unwired Technology
                     * and do not add such device to the internal device list.
                     * (should avoid check for AOAP support) */
                    if (0 == strncmp(strVendorIdString, UNWIRED_TECHNOLOGY_VENDOR_ID, strlen(strVendorIdString))) {
                        LOG_INFO((spi_usb_discoverer, "createDevInfo() Found Unwired (Hub) Technology (%s, %s, %s)",
                                strVendorIdString, strProductIdString, strProductString));
                    } else {
                        std::shared_ptr<t_usbDeviceInformation> outParam(new t_usbDeviceInformation());

                        sscanf(strVendorIdString,  "%x", &outParam->vendorId);
                        sscanf(strProductIdString, "%x", &outParam->productId);

                        outParam->serial.assign(strSerialString);
                        outParam->product.assign(strProductString);
                        outParam->manufacturer.assign(strManufacturerString);
                        outParam->sysPath.assign(strSysPathString);
                        outParam->devNum= udev_device_get_devnum(inUdevDevice);

                        LOG_INFO((spi_usb_discoverer, "createDevInfo() vendorId:%s, productId=%s, sn=%s, product=%s, devNum=%u",
                                strVendorIdString, strProductIdString, strSerialString, strProductString, outParam->devNum));

                        return outParam;
                    }
                } else {
                    LOG_WARN((spi_usb_discoverer, "createDevInfo() device_get_sysattr_value() is NULL"));
                }
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "createDevInfo() device_get_sysattr_value() of USB Base Class, SubClass, Protocol are NULL"));
        }
    } else if (devPath == 0) {
        LOGD_DEBUG((spi_usb_discoverer, "createDevInfo() Found USB controller"));
    } else {
        /* could not get devPath from udev device sysattr */
        LOG_INFO((spi_usb_discoverer, "createDevInfo() Could not get devPath. devPath=%d", devPath));
    }

    return NULL;
}

bool UsbDiscovererUdevMonitor::verifyAttach(struct udev_device* inUdevDevice)
{
    bool result = false;
    std::shared_ptr<t_usbDeviceInformation> usbDevInfo(nullptr);
    std::shared_ptr<t_usbDeviceInformation> usbDevParam(nullptr);

    /* creates the t_usbDeviceInformation pointer by parsing the udev attributes */
    usbDevInfo = createDevInfo(inUdevDevice);
    if (nullptr != usbDevInfo) {
        /* lock access to device list */
        if (UsbDeviceList::getInstance()->getLock()) {
            /* search by devNum - which is unique for each connected device */
            UsbDevice *pUsbDevice = UsbDeviceList::getInstance()->findDeviceById(usbDevInfo->devNum);
            if (!pUsbDevice) {
                /* UsbDevice with devNum not found -> cross check */
                pUsbDevice = UsbDeviceList::getInstance()->findDeviceById(usbDevInfo->sysPath.c_str());

                if (pUsbDevice) {
                    /* UsbDevice not found by devNum but by sysPath -> compare devNum */
                    if (usbDevInfo->devNum != pUsbDevice->getDevNum()) {
                        /* current devNum is newer than that from internal list
                         * -> assume udev events are out of sequence */
                        usbDevParam = pUsbDevice->getUsbDeviceInformation();

                        LOG_WARN((spi_usb_discoverer, "verifyAttach() UsbDevice %s changed devNum=%u to new devNum=%u",
                                to_string(usbDevParam).c_str(), pUsbDevice->getDevNum(), usbDevInfo->devNum));

                        /* close libusb device handle */
                        pUsbDevice->close(true);
                        /* erase UsbDevice from internal list */
                        UsbDeviceList::getInstance()->removeDevice(pUsbDevice);

                        LOG_INFO((spi_usb_discoverer, "verifyAttach() Removed UsbDevice %s (devNum=%u) from internal list",
                                to_string(usbDevParam).c_str(), usbDevParam->devNum));

                        if (NULL != mCallbacks.notifyDeviceRemoved) {
                            /* close libusb device handle */
                            mCallbacks.notifyDeviceRemoved(mContext, usbDevParam.get(), SPI_USB_DISCOVERY_SUCCESS);
                        }

                        delete pUsbDevice;
                        pUsbDevice = nullptr;
                    }
                }

                /* device not known, create new UsbDevice to add to device list */
                UsbDevice * pNewUsbDevice = new UsbDevice(usbDevInfo);
                if (pNewUsbDevice) {

                    bool checkAoapSupport = true;
                    if (NULL != mCallbacks.notifyGrantAoapSupportCheck) {
                        mCallbacks.notifyGrantAoapSupportCheck(mContext, usbDevInfo.get(), &checkAoapSupport);
                    }
                    if (true == checkAoapSupport) {
                        /* Add the UsbDevice to the device list.
                         * addDevice() creates a thread to check support for AOAP. */
                        UsbDeviceList::getInstance()->addDevice(pNewUsbDevice, mEventFd);
                        result = true;
                    } else {
                        LOG_INFO((spi_usb_discoverer, "%s Application does not allow to check device for AOAP support. UsbDevice %s will be discarded",
                                __FUNCTION__, to_string(usbDevInfo).c_str()));
                        result = false;
                    }
                } else {
                    LOG_ERROR((spi_usb_discoverer, "verifyAttach() Create UsbDevice for %s failed", to_string(usbDevInfo).c_str()));
                }
            } else {
                LOG_INFO((spi_usb_discoverer, "verifyAttach() UsbDevice %s (devNum=%u) already known",
                        to_string(usbDevInfo).c_str(), usbDevInfo->devNum));
                result = true;
            }

            UsbDeviceList::getInstance()->releaseLock();
        } else {
            LOG_ERROR((spi_usb_discoverer, "verifyAttach() Acquire lock failed"));
        }
    } else {
        /* we get into this else-condition if:
         *  - the udev_device is a USB controller
         *  - the udev_device is a USB hub
         *  - the udev_sysattr_value could not be retrieve */
//        LOG_ERROR((spi_usb_discoverer, "getUdevAttr() failed"));
    }
    return result;
}

bool UsbDiscovererUdevMonitor::verifyDetach(struct udev_device* inUdevDevice)
{
    bool result = false;
    std::shared_ptr<t_usbDeviceInformation> usbDevParam(nullptr);

    /* lock access to device list */
    if (UsbDeviceList::getInstance()->getLock()) {
        /* find device by sysPath and devNum
         * because all other udev attributes could lead to an seg-fault */
        const char* sysPath = udev_device_get_syspath(inUdevDevice);
        uint32_t    devNum  = udev_device_get_devnum(inUdevDevice);

        UsbDevice *pUsbDevice = UsbDeviceList::getInstance()->findDeviceById(sysPath, devNum);
        if (pUsbDevice) {
            /* get USB device information to send notifyDeviceRemoved callback */
            usbDevParam = pUsbDevice->getUsbDeviceInformation();
            /* close libusb device handle */
            pUsbDevice->close(true);
            /* erase UsbDevice from internal list */
            UsbDeviceList::getInstance()->removeDevice(pUsbDevice);

            LOG_INFO((spi_usb_discoverer, "verifyDetach() Removed UsbDevice %s (devNum=%u) from internal list",
                    to_string(usbDevParam).c_str(), devNum));
            result = true;
        } else {
            /* this can cause e.g. if udev events out of sequence and outdated UsbDevice was already removed */

            usbDevParam = std::shared_ptr<t_usbDeviceInformation>(new t_usbDeviceInformation(sysPath));

            LOG_INFO((spi_usb_discoverer, "verifyDetach() Could not find UsbDevice (sysPath=%s, devNum=%u) in internal list",
                    sysPath, devNum));
        }
        UsbDeviceList::getInstance()->releaseLock();

        if (true == result) {
            if (NULL != mCallbacks.notifyDeviceRemoved) {
                /* notify upper layer about detach*/
                mCallbacks.notifyDeviceRemoved(mContext, usbDevParam.get(), SPI_USB_DISCOVERY_SUCCESS);
            }
        }

        if (nullptr != pUsbDevice) {
            /* delete UsbDevice which was created at verifyAttach() */
            delete pUsbDevice;
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "verifyDetach() Acquire lock failed"));
    }

    return result;
}

bool UsbDiscovererUdevMonitor::verifyDevice(struct udev_device* inUdevDevice, bool attach) {
    if (true == attach) {
        /* device attached.
         * don't check result because false returned in case of USB Hub. */
        return verifyAttach(inUdevDevice);
    } else {
        /* device detached
         * don't check result because false returned in case of USB Hub. */
        return verifyDetach(inUdevDevice);
    }

    return false;
}

void UsbDiscovererUdevMonitor::checkEventList(const uint64_t inEvent, bool* outStopMonitoring) {
    int res = SPI_USB_DISCOVERY_ERROR;
    t_monitorEventListItem* checkEventItem = nullptr;

    /* check if we have an event */
    if (inEvent >= EVENTFD_EVENT_CHECK_LIST) {
        /* get the first event from the event list */
        checkEventItem = EventList::getInstance()->getEventItem();
        if (checkEventItem) {
            /* check if the event is known */
            switch(checkEventItem->eventToHandle)
            {
                case EventStop:
                {
                    /* stop monitoring was triggered. */
                    *outStopMonitoring = true;
                    LOG_INFO((spi_usb_discoverer, "checkEventList(EventStop) stop monitoring"));
                    break;
                }
                case EventAttach:
                {
                    /* got usbDeviceInformation direct from usbDevice object
                     * and provided to Application */
                    std::shared_ptr<t_usbDeviceInformation> usbDevParam(nullptr);

                    if (checkEventItem->pUsbDevParam) {
                        if (UsbDeviceList::getInstance()->getLock()) {
                            LOG_INFO((spi_usb_discoverer, "checkEventList(EventAttach) vendorId=0x%X, productId=0x%X, sn=%s",
                                checkEventItem->pUsbDevParam->vendorId, checkEventItem->pUsbDevParam->productId, \
                                checkEventItem->pUsbDevParam->serial.c_str()));

                            /* get UsbDevice object from device list */
                            UsbDevice *pUsbDevice = UsbDeviceList::getInstance()->findDevice(checkEventItem->pUsbDevParam->vendorId, \
                                                                                             checkEventItem->pUsbDevParam->productId, \
                                                                                             checkEventItem->pUsbDevParam->serial);
                            if (pUsbDevice) {
                                /* found device in internal list, assume it is still available */

                                /* get usbDeviceInformation from UsbDevice object */
                                usbDevParam = pUsbDevice->getUsbDeviceInformation();
                                if (nullptr != usbDevParam) {
                                    res = SPI_USB_DISCOVERY_SUCCESS;
                                } else {
                                    LOG_ERROR((spi_usb_discoverer, "checkEventList() Could not get usbDeviceInformation from UsbDevice"));
                                    res = SPI_USB_DISCOVERY_BAD_PARAMETER;
                                }
                            } else {
                                res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;

                                LOG_ERROR((spi_usb_discoverer, "checkEventList() Could not find UsbDevice in internal list"));
                            }
                            UsbDeviceList::getInstance()->releaseLock();
                        }
                        *outStopMonitoring = false;
                    } else {
                        LOG_WARN((spi_usb_discoverer, "checkEventList() usbDeviceInformation in EventItem is NULL"));
                        res = SPI_USB_DISCOVERY_BAD_PARAMETER;
                    }
                    if (SPI_USB_DISCOVERY_SUCCESS == res) {
                        /* https://source.android.com/accessories/aoa.html
                         * The vendor ID should match Google's ID (0x18D1)
                         * and the product ID should be 0x2D00 or 0x2D01 (AOAP 2.0 0x2D00 - 0x2D05)
                         * if the device is already in accessory mode. */
                        if ((usbDevParam->vendorId == 0x18d1) &&
                            ((usbDevParam->productId >= 0x2d00) && (usbDevParam->productId <= 0x2d05))) {
                            /* device connected and already in accessory mode */
                            if (NULL != mCallbacks.notifyDeviceFoundAccessoryMode) {
                                mCallbacks.notifyDeviceFoundAccessoryMode(mContext, usbDevParam.get(), res);
                            }
                        } else {
                            if (NULL != mCallbacks.notifyDeviceFound) {
                                mCallbacks.notifyDeviceFound(mContext, usbDevParam.get(), res);
                            }
                        }
                    }
                    break;
                }
                default:
                {
                    LOG_ERROR((spi_usb_discoverer, "checkEventList() Unknown event=%d", checkEventItem->eventToHandle));
                    break;
                }
            } // switch

            /* delete event list item which was added by addEventItem() */
            delete checkEventItem;
            checkEventItem = nullptr;
        } else {
            LOG_ERROR((spi_usb_discoverer, "checkEventList() Failed to get event checkEventItem=%p", checkEventItem));
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "checkEventList() inEvent=%" PRIu64 " is invalid", inEvent));
    }

    return;
}

bool UsbDiscovererUdevMonitor::startUdevMonitor() {
    /* create udev monitor */
    mpMonitor = udev_monitor_new_from_netlink(mpUdev, STR_UDEV);
    if (nullptr != mpMonitor) {
        /* filter for usb_devices only */
        if (SPI_USB_DISCOVERY_SUCCESS == udev_monitor_filter_add_match_subsystem_devtype(mpMonitor, STR_USB, STR_USB_DEVICE)) {
            if (SPI_USB_DISCOVERY_SUCCESS == udev_monitor_enable_receiving(mpMonitor)) {
                /* get file descriptor of udev monitor */
                mUdevMonitorFd = udev_monitor_get_fd(mpMonitor);
                if (mUdevMonitorFd >= 0) {
                    return true;
                } else {
                    LOG_ERROR((spi_usb_discoverer, "udev_monitor_get_fd() failed"));
                }
            } else {
                LOG_ERROR((spi_usb_discoverer, "udev_monitor_enable_receiving() failed"));
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "udev_monitor_filter_add_match_subsystem_devtype() failed"));
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "udev_monitor_new_from_netlink() failed"));
    }

    return false;
}

void* UsbDiscovererUdevMonitor::usbMonitorThread(void* p) {
    struct udev_device *pUdevDevice = nullptr;
    fd_set fds;
    int nfds = 0;   /* highest-numbered file descriptor in any of the three sets, plus 1 */
    int selectResult = 0;
    int j = 0;
    int cntfds = 2; /* Currently, we have two fds, mUdevMonitorFd + mEventFd */
    uint64_t event = 0;
    bool bStopMonitoring = false;

    prctl(PR_SET_NAME, "spiUsbMonitor", 0, 0, 0);

    auto me = static_cast<UsbDiscovererUdevMonitor*>(p);
    if (me == nullptr) {
        LOG_ERROR((spi_usb_discoverer, "usbMonitorThread() could not cast input pointer to UsbDiscovererUdevMonitor"));
        return nullptr;
    }

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
                        if (SPI_USB_DISCOVERY_SUCCESS == strcmp(udev_device_get_action(pUdevDevice), STR_ADD)) {
                            LOG_INFO((spi_usb_discoverer, "usbMonitorThread() Device was added"));

                            me->verifyDevice(pUdevDevice, true);
                        } else if (SPI_USB_DISCOVERY_SUCCESS == strcmp(udev_device_get_action(pUdevDevice), STR_REMOVE)) {
                            LOG_INFO((spi_usb_discoverer, "usbMonitorThread() Device was removed"));

                            me->verifyDevice(pUdevDevice, false);
                        } else if (SPI_USB_DISCOVERY_SUCCESS == strcmp(udev_device_get_action(pUdevDevice), STR_CHANGE)) {
                            LOG_INFO((spi_usb_discoverer, "usbMonitorThread() Device was changed"));
                            LOG_INFO((spi_usb_discoverer, "usbMonitorThread() Udev event 'change' received for device %s:%s",
                                        udev_device_get_sysattr_value(pUdevDevice, STR_IDVENDOR),
                                        udev_device_get_sysattr_value(pUdevDevice, STR_IDPRODUCT)));
                        } else {
                            const char* sysPath = udev_device_get_syspath(pUdevDevice);
                            LOG_INFO((spi_usb_discoverer, "usbMonitorThread() unknown udev action=%s received for device (devNum=%u) on sysPath=%s",
                                      udev_device_get_action(pUdevDevice), (uint32_t)udev_device_get_devnum(pUdevDevice),
                                      (NULL != sysPath) ? sysPath : ""));
                        }
                        udev_device_unref(pUdevDevice);
                    } else {
                        LOG_ERROR((spi_usb_discoverer, "usbMonitorThread() pUdevDevice is NULL"));
                    }
                    selectResult--;
                }
                /* eventfd triggered at file descriptor mEventFd */
                if ((j < cntfds) && (FD_ISSET(me->mEventFd, &fds))) {
                    /* read eventfd to get current number of occurred events */
                    if (eventfd_read(me->mEventFd, &event) != 0) {
                        LOG_ERROR((spi_usb_discoverer, "usbMonitorThread() eventfd_read(fd=%d) failed", me->mEventFd));
                    } else {
                        /* check event list for all current events */
                        do {
                            LOGD_DEBUG((spi_usb_discoverer, "usbMonitorThread() eventfd triggered event(%" PRIu64 ") to check list", event));
                            me->checkEventList(event, &bStopMonitoring);
                            event -= EVENTFD_EVENT_CHECK_LIST;
                        } while ((event >= EVENTFD_EVENT_CHECK_LIST) && (true != bStopMonitoring));
                    }
                    selectResult--;
                }
            } // for-loop
        } else {
            /* selectResult = 0 : select() timed out
             * selectResult < 0 : indicates an error (check errno)
             */
            LOG_ERROR((spi_usb_discoverer, "usbMonitorThread() select failed=%d", selectResult));
        }
    } /* while */

    EventList::deleteInstance();
    UsbDeviceList::deleteInstance();

    LOG_INFO((spi_usb_discoverer, "usbMonitorThread() exit"));
    return nullptr;
}


