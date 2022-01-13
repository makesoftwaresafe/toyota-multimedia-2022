#ifndef PERSISTENCE_FILE_CACHE_CONFIG_READER_H_
#define PERSISTENCE_FILE_CACHE_CONFIG_READER_H_

/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_file_cache_config_reader.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner (XSe)
 * @brief          Header of the persistence file cache configuration reader.
 *
 */



int getConfiguration(const char* appID, unsigned int* cacheSize);



#endif /* PERSISTENCE_FILE_CACHE_CONFIG_READER_H_ */
