#ifndef CRC32_H
#define CRC32_H

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * Copyright
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           crc32.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Header of crc32 checksum generation
 * @see            
 */

#ifdef __cplusplus
extern "C" {
#endif


#include <string.h>

const unsigned int checksumCrc32(unsigned int crc, const unsigned char *buf, size_t theSize);


#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */
