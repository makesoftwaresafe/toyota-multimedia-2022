/*
 * aoap_usbmonitor_subject.h
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_USBMONITOR_SUBJECT_H_
#define AOAP_USBMONITOR_SUBJECT_H_

#include <string>
#include <vector>

class UsbObserver;

/**
 * @class UsbMonitorSubject aoap_usbmonitor_subject.h "aoap_usbmonitor_subject.h"
 * The UsbMonitor subject used for implementing the observer design pattern
 */
class UsbMonitorSubject
{
public:
    /**
     * @brief Constructor of UsbMonitorSubject
     */
    UsbMonitorSubject(void) { };

    /**
     * @brief Destructor of UsbMonitorSubject
     */
    virtual ~UsbMonitorSubject();

    /**
     * @brief Register the specified observer
     *
     * @param pObserver The observer to be registered
     */
    void registerObserver(UsbObserver *pObserver);

    /**
     * @brief Unregister the specified observer
     *
     * @param pObserver The pointer to the observer to be unregistered
     */
    void unregisterObserver(UsbObserver *pObserver);

    /**
     * @brief Notify all registered observers about an attach/detach event of an USB device
     *
     * @param attach The type of the event (true when it is an attach otherwise detach)
     * @param vendorId The vendor ID of the USB device
     * @param productId The product ID of the USB device
     * @param serial The serial of the USB device
     * @param devNum The device number
     */
    virtual void notify(bool attach,
                        unsigned int vendorId,
                        unsigned int productId,
                        const std::string &serial,
                        unsigned int devNum);


private:
    std::vector<UsbObserver*> mObservers;
};

#endif /* AOAP_USBMONITOR_SUBJECT_H_ */
