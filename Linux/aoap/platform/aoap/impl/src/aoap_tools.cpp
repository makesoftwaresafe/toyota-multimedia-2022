/*
 * aoap_tools.cpp
 *
 *  Created on: Jul 25, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_tools.h"
#include "aoap_types.h"

bool AOAP::Tools::isAccessory(unsigned int vendorId, unsigned int productId)
{
    if ((vendorId == AOAP_GOOGLE_VENDOR_ID)
            && AOAP::Tools::isAccessoryproductId(productId))
    {
        return true;
    }
    return false;
}


bool AOAP::Tools::isAccessoryproductId(unsigned int productId)
{
    if ((productId >= AOAP_GOOGLE_PRODUCT_ID_ACCESSORY) && (productId
            <= AOAP_GOOGLE_PRODUCT_ID_LATEST))
    {
        return true;
    }
    return false;
}
