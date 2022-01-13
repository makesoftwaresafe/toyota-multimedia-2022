#ifndef PERSISTENCY_FILE_CACHE_H
#define PERSISTENCY_FILE_CACHE_H

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
 * @file           persistence_file_cache.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner (XSe)
 * @brief          Header of the persistence file cache.
 *                 Library provides an API to handle cached files
 *
 * @par change history
 * Date     Author          Version
 * 04/11/13 Ingo Hürner     0.0.1 Initial creation of this file
 *
 */

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum _PfcCacheAffinity_e
{
   CacheAffLow          = 0,
   CacheAffBelowMedium  = 1,
   CacheAffMedium       = 2,
   CacheAffAboveMedium  = 3,
   CacheAffHighest      = 4
} PfcCacheAffinity;



typedef enum _PfcFileCreateMode_e
{
   DontCreateFile    = 0,
   CreateFile  		= 1
} PfcFileCreateMode;

/**
 * @brief Initialize the cache
 *
 * @param appID theApplicationID
 *        There is a configuration file where the cache limit for the app is set
 * @return positive value (bigger than zero):
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcInitCache(const char* appID);


/**
 * @brief deinitialize the cache
 *
 * @return positive value (bigger than zero):
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcDeinitCache(void);

/**
 * @brief open a file.
 *        This function opens a file and returns a handle.
 *
 * @param path to the file
 *
 * @return positive value (bigger than zero): the handle
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcOpenFile(const char* path, PfcFileCreateMode mode);

/**
 * @brief open a file.
 *        This function opens a file and returns a handle.
 *
 * @param path to the file
 * @param cacheaffinity the cache affinity
 *        the cache affinity will be used to determin if a file could be removed
 *
 * @return positive value (bigger than zero): the handle
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcOpenFileAffinity(const char* path, PfcFileCreateMode mode, PfcCacheAffinity affinity);



/**
 * @brief close a file.
 *        This function closes the file descriptor
 *        Writes back to flash device
 *        Removes from cache
 *
 * @param handle to the file
 *
 * @return positive value (bigger than zero): successfully closed
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcCloseFile(int handle);



/**
 * @brief read data from a file.
 *        This function reads data from a file given by the handle
 *
 * @param handle to the file
 * @param buffer to store the data
 * @param count the size of the buffer
 *
 * @return positive value the number of data read
 *   On error a negative value will be returned with th following error codes:
 *   
 */
int pfcReadFile(int handle, void *buf, size_t count);



/**
 * @brief read data from a file.
 *        This function reads data from a file given by the handle
 *
 * @param handle to the file
 * @param buffer the data to write
 * @param count the data size to write
 *
 * @return positive value the number of data written 
 *   On error a negative value will be returned with th following error codes:
 *   
 */
int pfcWriteFile(int handle, const void *buf, size_t nbyte);




/**
 * @brief reposition the file descriptor
 *
 * @param fd the POSIX file descriptor
 * @param offset the reposition offset
 * @param whence the direction to reposition
                 SEEK_SET
                      The offset is set to offset bytes.
                 SEEK_CUR
                      The offset is set to its current location plus offset bytes.
                 SEEK_END
                      The offset is set to the size of the file plus offset bytes.
 *
 * @return positive value (0 or greater): resulting offset location;
 * On error, the value -1 is returned and errno is set to indicate the error.
 *
 */
int pfcFileSeek(int handle, long int offset, int whence);



/**
 * @brief get thsi size of a file.
 *
 * @param handle to the file
 *
 * @return positive value (bigger than zero): the size
 *   On error a negative value will be returned with th following error codes:
 *
 */
int pfcFileGetSize(int handle);


#ifdef __cplusplus
}
#endif


#endif /* PERSISTENCY_FILE_CACHE_H */
