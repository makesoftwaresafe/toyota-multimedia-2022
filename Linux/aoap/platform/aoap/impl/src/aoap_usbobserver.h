/*
 * aoap_usbobserver.h
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_USBOBSERVER_H_
#define AOAP_USBOBSERVER_H_

#include <string>

/**
 * @class UsbObserver aoap_usbobserver.h "aoap_usbobserver.h"
 * @brief Abstract USB Observer class for getting notifications when device gets attached/detached.
 *
 * Follows observer design pattern
 */
class UsbObserver
{
public:
    /**
     * @brief Constructor of UsbObserver (empty)
     */
    UsbObserver() { };

    /**
     * @brief Destructor of UsbObserver
     */
    virtual ~UsbObserver() { };

    /**
     * @brief This function gets called when an add or remove event for the specified USB device occurs.
     *
     * Function is pure virtual (=must be implemented by derived classes)
     *
     * @param attach true when the specified USB device gets attached otherwise false
     * @param vendorId The USB vendor id
     * @param productId The USB product id
     * @param serial The USB serial
     * @param devNum The device number
     */
    virtual void update(bool attach, unsigned int vendorId,
            unsigned int productId, const std::string &serial, unsigned int devNum) = 0;

private:
};

#endif /* AOAP_USBOBSERVER_H_ */
