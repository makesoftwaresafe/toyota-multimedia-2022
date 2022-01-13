/*
 * aoap_tools.h
 *
 *  Created on: Jul 25, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_TOOLS_H_
#define AOAP_TOOLS_H_

namespace AOAP
{
namespace Tools
{
/**
 * @brief Checks if the specified vendor and product IDs match to Google's accessory IDs
 *
 * @param vendorId The USB vendor ID to be checked
 * @param productId The USB product ID to be checked
 * @return 0 means the IDs do not match. In case they match 1 is returned
 */
bool isAccessory(unsigned int vendorId, unsigned int productId);

/**
 * @brief Checks if the specified product ID matches to one of Google's accessory product IDs
 *
 * @param productId The USB product ID to be checked
 * @return true when it matches, otherwise false
 */
bool isAccessoryproductId(unsigned int productId);
}
}

#endif /* AOAP_TOOLS_H_ */
