#ifndef PERSISTENCE_FILE_CACHE_STATS_H
#define PERSISTENCE_FILE_CACHE_STATS_H

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
 * @file           persistence_file_cache_stats.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner (XSe)
 * @brief          Header of the persistence file cache data.
 *
 */


/**
 * C A C H E  remove algorythm:
 *
 * (priority+1)*((sizeBytes/10)+1)*(1/lastAccesTimeSec) = cache factor
 *
 * The entry with the lowest cache factor number will be removed matching the
 * required size best will be removed.
 * If the cache factor and the size is the same, remove the lower prio
 *
 * Improvements:
 * - Consider to remove 2 or more entries with a lower prio instead
 *   one item with the appropriate size
 * -
 *
 * Examples:
 * prio  Size[bytes]   last access[sec]  Cache Factor
 * 4     10.000         10                501
 * 4     10.000         5                 1001
 * 2     10.000         4                 751
 * 0     10.000         2                 501
 * 3     10.000         1                 4004
 * 3     5.000          5                 401
 * 3     100            5                 9
 * 3     10             5                 2
 * 3     1              5                 1
 *
 */

#include <pthread.h>


/** specify the type of the resource */
typedef enum _PfcCacheStatus_e
{
   CacheStatusNotInCache   = 0,
   CacheStatusInCache      = 1,
   CacheStatusSuspended    = 2

} PfcCacheStatus_e;

extern pthread_mutex_t gCacheSizeMtx;

extern pthread_mutex_t gRemoveMtx;

/**
 * @brief adds a number to the current cache usage
 *
 * @param the size in bytes to add
 */
void statsAddCacheUsage(unsigned int size);

/**
 * @brief subtracts a number from the current cache usage
 *
 * @param the size in bytes to subtract
 */
void statsSubCacheUsage(unsigned int size);


/**
 * @brief query the maximum cache size in bytes
 *
 * @return the max cach size in bytes
 */
unsigned int statsGetMaxCacheSize(void);

/**
 * @brief set the maximum cache size in bytes
 *
 * @param the max cach size in bytes
 */
void statsSetMaxCacheSize(unsigned int size);


/**
 * @brief returns the number of bytes currently in cache
 *
 * @param cache usage in bytes
 */
unsigned int statsGetCacheUsage(void);





/**
 * @brief returns the size in bytes in the cache
 *
 * @param handle for getting the cache size
 *
 * @return the cache size
 */
unsigned int statsGetCacheSize(int handle);

void statsSetCacheSize(int handle, unsigned int size);






void statsSetCacheStatus(int handle, PfcCacheStatus_e status);

PfcCacheStatus_e statsGetCacheStatus(int handle);

void statsSetInvalidateEntry(int handle);

void statsUpdateCacheFactor(int handle);


/**
 * @brief initialize cache statistics
 *
 * @param handle to init the cache statistics for
 * @param prio the cache priority
 */
void statsSingleInitCacheStats(int handle, PfcCacheAffinity affinity);

void statsSingleClearCacheStats(int handle);

void statsInitCacheStats(void);

void statsGetHandleToRemove(unsigned int startValue, int* handle);





#endif /* PERSISTENCE_FILE_CACHE_STATS_H */
