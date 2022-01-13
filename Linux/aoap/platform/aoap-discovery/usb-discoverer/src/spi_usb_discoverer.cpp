
/************************************************************************
 *
 * \file: spi_usb_discoverer.cpp
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

#include <adit_logging.h>
#include <adit_dlt.h>

#include "spi_usb_discoverer.h"
#include "spi_usb_discoverer_device_list.h"
#include "spi_usb_monitor.h"

#ifndef DLT_SUD
#define DLT_SUD "SUD"
#endif

LOG_DECLARE_CONTEXT(spi_usb_discoverer)


std::atomic<uint32_t> UsbDeviceInformation::_count(0);

namespace AOAP {

/* *************  defines  ************* */


/* *************  function  ************* */

UsbDiscoverer::UsbDiscoverer(void* context, SpiUsbDiscovererCallbacks *pCallbacks) {
    mCallbacks = *pCallbacks;

    mContext = context;
    mMonitoringActive = false;

    LOG_REGISTER_CONTEXT(spi_usb_discoverer, DLT_SUD, "SPI USB Discoverer");
}

UsbDiscoverer::~UsbDiscoverer() {
    
    if (mMonitoringActive) {
        LOG_WARN((spi_usb_discoverer, "~UsbDiscoverer() mMonitoringActive=%d was not stopped", mMonitoringActive));
        stopMonitoring();
    }
    UsbDiscovererUdevMonitor::deleteInstance();

    memset(&mCallbacks, 0, sizeof(mCallbacks));
    LOG_UNREGISTER_CONTEXT(spi_usb_discoverer);
}

int UsbDiscoverer::startMonitoring() {
    int res = SPI_USB_DISCOVERY_SUCCESS;

    res = UsbDiscovererUdevMonitor::getInstance()->startMonitoring(mContext, &mCallbacks);
    mMonitoringActive = true;

    return res;
}

int UsbDiscoverer::stopMonitoring() {
    int res = SPI_USB_DISCOVERY_SUCCESS;

    res = UsbDiscovererUdevMonitor::getInstance()->stopMonitoring();
    mMonitoringActive = false;

    return res;
}

int UsbDiscoverer::resetDevice(t_usbDeviceInformation* inUsbDevParam) {

    int res = SPI_USB_DISCOVERY_SUCCESS;

    if (NULL != inUsbDevParam) {
        res = UsbDiscovererUdevMonitor::getInstance()->resetMonitoredDevice(inUsbDevParam);
    } else {
        res = SPI_USB_DISCOVERY_BAD_PARAMETER;
    }

    return res;
}

} // namespace AOAP {

