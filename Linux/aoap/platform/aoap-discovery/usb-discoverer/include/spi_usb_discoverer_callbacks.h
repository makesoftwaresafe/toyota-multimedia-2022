/************************************************************************
 *
 * \file: spi_usb_discoverer_callbacks.h
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

#ifndef SPI_USB_DISCOVERER_CALLBACKS_H_
#define SPI_USB_DISCOVERER_CALLBACKS_H_

/* *************  includes  ************* */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include "spi_usb_discoverer_types.h"


/* *************  defines  ************* */

/**
 * \brief Notification that a device has been found which supports AOAP.
 *
 * \param pContext The context
 * \param pDevice The device information of the device found. SpiUsbDiscoverer owns and deletes the memory.
 * \param result indicates the result of the device detection
 *               (e.g. if all device information could be received)
 */
typedef void SpiUsbDiscovererNotifyDeviceFound(void *pContext, t_usbDeviceInformation* pDevice, int result);

/**
 * \brief Notification that a device has been removed.
 * Only triggered if detached device supported AOAP. Otherwise not triggered.
 *
 * \param pContext The context
 * \param pDevice Pointer to the device information to the removed device. SpiUsbDiscoverer owns and deletes the memory.
 * \param result indicates the result of the device detection
 *               (e.g. if all device information could be received)
 * \note: There is no guaranty that all device information can be provided
 * in case of disconnecting the MD because the system (udev, Kernel) releases
 * the information immediately.
 *
 */
typedef void SpiUsbDiscovererNotifyDeviceRemoved(void *pContext, t_usbDeviceInformation* pDevice, int result);

/**
 * \brief Notification that a device has been found which is in accessory mode (AOAP).
 *
 * \param pContext The context
 * \param pDevice The device information of the device found. SpiUsbDiscoverer owns and deletes the memory.
 * \param result indicates the result of the device detection
 *               (e.g. if all device information could be received)
 */
typedef void SpiUsbDiscovererNotifyDeviceFoundAccessoryMode(void *pContext, t_usbDeviceInformation* pDevice, int result);

/**
 * \brief Notification that a device has been found and upper layer should agree to check for AOAP support.
 *
 * \param pContext The context
 * \param pDevice The device information of the device found. SpiUsbDiscoverer owns and deletes the memory.
 * \param checkAoapSupport true indicates that the device should be checked for AOAP support
 */
typedef void SpiUsbDiscovererNotifyGrantAoapSupportCheck(void *pContext, t_usbDeviceInformation* pDevice, bool* checkAoapSupport);


/**
 * \brief The Interface of the Callbacks API.
 */
struct SpiUsbDiscovererCallbacks
{
    /**
    * Notification that a device has been found.
    */
    SpiUsbDiscovererNotifyDeviceFound *notifyDeviceFound;

    /**
    * Notification that a device has been removed.
    */
    SpiUsbDiscovererNotifyDeviceRemoved *notifyDeviceRemoved;

    /**
    * Notification that a device in accessory mode has been found.
    */
    SpiUsbDiscovererNotifyDeviceFoundAccessoryMode *notifyDeviceFoundAccessoryMode;

    /**
    * Notification that a device has been found and we need agreement to continue with check for AOAP support.
    */
    SpiUsbDiscovererNotifyGrantAoapSupportCheck *notifyGrantAoapSupportCheck;

    SpiUsbDiscovererCallbacks()
    {
        notifyDeviceFound = nullptr;
        notifyDeviceRemoved = nullptr;
        notifyDeviceFoundAccessoryMode = nullptr;
        notifyGrantAoapSupportCheck = nullptr;
    };
};


#endif /* SPI_USB_DISCOVERER_CALLBACKS_H_ */
