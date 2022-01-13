
/************************************************************************
 *
 * \file: spi_usb_discoverer_utility.cpp
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
 * \copyright (c) 2015 Advanced Driver Information Technology.
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
#include "spi_usb_discoverer_utility.h"

/* *************  defines  ************* */

/* *************  function  ************* */

string to_string(std::shared_ptr<t_usbDeviceInformation> inVal)
{
    if (inVal != nullptr) {
        std::stringstream ss;
        ss << "0x" << std::hex << inVal->vendorId <<
             ":0x" << std::hex << inVal->productId <<
             " sn=" << inVal->serial;
        return ss.str();
    } else {
        return "to_string() ERROR: input parameter is NULL";
    }
}

