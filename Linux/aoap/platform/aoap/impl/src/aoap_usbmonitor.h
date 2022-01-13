/*
 * aoap_usbmonitor.h
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_USBMONITOR_H_
#define AOAP_USBMONITOR_H_

#include "aoap_usbmonitor_subject.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <libudev.h>

/**
 * @class UsbMonitor aoap_usbmonitor.h "aoap_usbmonitor.h"
 * The USB monitoring instance. It is implemented as singleton.
 */
class UsbMonitor: public UsbMonitorSubject
{
public:

    /**
     * @typedef typedef enum UsbAction tUsbAction
     * @enum UsbAction
     * The USB device actions
     */
    typedef enum UsbAction
    {
        USB_ADD,    //!< USB_ADD
        USB_REMOVE, //!< USB_REMOVE
        USB_UNKNOWN//!< USB_UNKNOWN
    } tUsbAction;

    /**
     * @brief Get the singleton instance of the USB monitor.
     *
     * If not yet created, it will create it
     *
     * @return A pointer to the instance. This should be always not NULL.
     */
    static UsbMonitor* getInstance(void);

    /**
     * @brief Deletes the singleton instance of the USB monitor
     */
    static void deleteInstance(void);

    /**
     * @brief Initializes USB monitoring using 'libudev'
     *
     * Monitoring will be started only once.
     */
    void initMonitoring(void);

    /**
     * @brief Helper function to convert an ASCII value to its decimal representation
     *
     * @param value The ASCII value to be converted
     * @return The resulting decimal value
     */
    static unsigned int convertAsciiToDecimal(const char *value);

private:
    /**
     * @brief Constructor of UsbMonitor
     */
    UsbMonitor(void); //lint !e1704

    /**
     * @brief Destructor of UsbMonitor
     */
    virtual ~UsbMonitor(void);

    /**
     * @brief Stops USB monitoring
     */
    void stopMonitoring(void);

    /**
     * @brief Start the monitoring thread
     */
    void startMonitoringThread(void);

    /**
     * @brief Static monitoring function started by 'startMonitoringThread'
     *
     * @param pToken The pointer to the UsbMonitor
     */
    static void* monitor(void *pToken);

    /**
     * @brief The monitoring function which runs in the context of the UsbMonitor instance
     */
    void monitor(void);

    /**
     * @brief Get the USB device action from a string
     *
     * @param pAction The action value to be converted
     * @return The resulting action
     */
    tUsbAction getAction(const char *pAction);

    /** The static instance of the UsbMonitor*/
    static UsbMonitor *gpSelf;

    /** Flag to indicate to stop the monitoring thread */
    static bool gQuit;

    /** A pointer to the 'libudev' */
    struct udev *mpUdev;

    /** A pointer to the 'libudev' monitor */
    struct udev_monitor *mpMonitor;

    /** The monitoring thread */
    pthread_t mMonitorThread;

    /** The udev monitoring file descriptor */
    int mMonitorFd;

    /* event file descriptor to abort the select in UsbMonitor */
    int mMonitorEventFd;

    /** The timeout for select in seconds */
    const int mSelectTimeout;

    /** The udev event name source used for 'udev_monitor_new_from_netlink'. Valid sources are "udev" and "kernel" */
    const char *mpUdevEventNameSource;

    UsbObserver *mpGlobalObserver;
};

#endif /* AOAP_USBMONITOR_H_ */
