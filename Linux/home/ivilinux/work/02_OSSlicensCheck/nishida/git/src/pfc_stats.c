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
 * @file           persistence_file_cache_stats.c
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence file cache statistics
 * @see
 */

#include "pfc_def.h"
#include "pfc_stats.h"

#include <sys/time.h>



// define for the used clock: "CLOCK_MONOTONIC" or "CLOCK_REALTIME"
#define CLOCK_ID  CLOCK_MONOTONIC

// used for conversion
#define SECONDS2NANO 1000000000L
#define NANO2MIL        1000000L
#define MIL2SEC            1000L


// default cache size is 10 MBytes size
#define DEFAULT_CACHE_SIZE (10 * 1024 * 1024)

/// structure used to store statistics about the cached file
typedef struct _PfcCacheStatisitcs_s
{
   unsigned int CacheFileSize;               /// the file size in bytes
   unsigned int EntryEmpty;                  /// indicator to see if the entry is empty
   unsigned int CacheFactor;                 /// cache factor used to determin the cachen entry to be removed
                                             /// when the cache is full
   PfcCacheStatus_e   CacheStatus;           /// the RAM status: in ram, not in ram, suspended
   PfcCacheAffinity   CachePrio;             /// the cache priority

   struct timespec CacheLastAccess;          /// last access
}  PfcCacheStatisitcs_s;




pthread_mutex_t gCacheSizeMtx = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gRemoveMtx = PTHREAD_MUTEX_INITIALIZER;



/// the current cache size
static unsigned int gCurrentCacheSize = 0;

/// size of the cache in bytes (initialized with default size)
static int unsigned gMaxCacheSize = DEFAULT_CACHE_SIZE;

/// persistence file handle array
static PfcCacheStatisitcs_s gCacheStatsArray[MaxPersHandle];


long long getNsDuration(struct timespec* start, struct timespec* end)
{
   return ((end->tv_sec * SECONDS2NANO) + end->tv_nsec) - ((start->tv_sec * SECONDS2NANO) + start->tv_nsec);
}


void statsSingleInitCacheStats(int handle, PfcCacheAffinity affinity)
{
   clock_gettime(CLOCK_ID, &gCacheStatsArray[handle].CacheLastAccess);

   gCacheStatsArray[handle].EntryEmpty = 1;
   gCacheStatsArray[handle].CacheFileSize = 0;
   gCacheStatsArray[handle].CachePrio = affinity;
   gCacheStatsArray[handle].CacheStatus = CacheStatusNotInCache;
   gCacheStatsArray[handle].CacheFactor = -3;
}



void statsSingleClearCacheStats(int handle)
{
   clock_gettime(CLOCK_ID, &gCacheStatsArray[handle].CacheLastAccess);

   gCacheStatsArray[handle].EntryEmpty = 0;
   gCacheStatsArray[handle].CacheFileSize = 0;
   gCacheStatsArray[handle].CachePrio = CacheAffMedium;
   gCacheStatsArray[handle].CacheStatus = CacheStatusNotInCache;
   gCacheStatsArray[handle].CacheFactor = -2;
}



void statsInitCacheStats(void)
{
   int i = 0;

   for(i= 0; i< MaxPersHandle; i++)
   {
      statsSingleClearCacheStats(i);
   }
}


unsigned int statsGetCacheUsage(void)
{
   return  gCurrentCacheSize;
}


void statsGetHandleToRemove(unsigned int startValue, int* handle)
{
   unsigned int min = startValue;
   int i = 0;

   pthread_mutex_lock(&gRemoveMtx);

   *handle = -1;  // init with -1

   for(i = 0; i < MaxPersHandle; i++)
   {
      if(gCacheStatsArray[i].EntryEmpty != 0)
      {
         //printf("    statsGetHandleToRemove[%d] ==> Not empty\n", i);
         if(   (gCacheStatsArray[i].CacheFactor <= min)
            && (gCacheStatsArray[i].CacheFactor >0))
         {
            min  = gCacheStatsArray[i].CacheFactor;
            (*handle) = i;
            //printf("      statsGetHandleToRemove[%d] ==> Y E S: %d \n", i, gCacheStatsArray[i].CacheFactor);
         }
         /*
         else
         {
            printf("      statsGetHandleToRemove[%d] ==> N O  : %d \n", i, gCacheStatsArray[i].CacheFactor);
         }
         */
      }
   }
   pthread_mutex_unlock(&gRemoveMtx);
}


//(priority+1)*((sizeBytes/10)+1)*(1/lastAccesTimeSec) = cache factor

void statsUpdateCacheFactor(int handle)
{
   long long lastAccessNs = 0;
   struct timespec curTime;

   clock_gettime(CLOCK_ID, &curTime);  // get the current time

   // calculate the duration when the file has been accessed the last time
   lastAccessNs = getNsDuration(&(gCacheStatsArray[handle].CacheLastAccess), &curTime);

   // TODO improve calculation
   // calculate the cache factor
   gCacheStatsArray[handle].CacheFactor = ((double)(gCacheStatsArray[handle]).CachePrio + 1.0 )
                                           * (double)((gCacheStatsArray[handle].CacheFileSize / 10.0) + 1.0)
                                           * ((1.0/lastAccessNs)*NANO2MIL);

   // remember the time the file has been accessed
   clock_gettime(CLOCK_ID, &gCacheStatsArray[handle].CacheLastAccess);
   //printf("* * statsUpdateCacheFactor CacheFactor[%d]: %u \n", handle, gCacheStatsArray[handle].CacheFactor);
}


void statsAddCacheUsage(unsigned int size)
{
   pthread_mutex_lock(&gCacheSizeMtx);

   //printf("* * statsAddCacheUsage => CacheSize: %d | toAdd: %d\n", gCurrentCacheSize, size);
   gCurrentCacheSize += size;
   //printf("* * statsAddCacheUsage <= CacheSize: %d\n\n", gCurrentCacheSize);

   pthread_mutex_unlock(&gCacheSizeMtx);
}


void statsSubCacheUsage(unsigned int size)
{
   pthread_mutex_lock(&gCacheSizeMtx);

   //printf("* * statsSubCacheUsage => CacheSize: %d | toRemove: %d\n", gCurrentCacheSize, size);
   gCurrentCacheSize -= size;
   //printf("* * statsSubCacheUsage <= CacheSize: %d\n\n", gCurrentCacheSize);

   pthread_mutex_unlock(&gCacheSizeMtx);
}


unsigned int statsGetMaxCacheSize(void)
{
   return gMaxCacheSize;
}


void statsSetMaxCacheSize(unsigned int size)
{
   gMaxCacheSize = size;
}

void statsSetCacheSize(int handle, unsigned int size)
{
   gCacheStatsArray[handle].CacheFileSize = size;
}


unsigned int statsGetCacheSize(int handle)
{
   return gCacheStatsArray[handle].CacheFileSize;
}

void statsSetCacheStatus(int handle, PfcCacheStatus_e status)
{
   gCacheStatsArray[handle].CacheStatus = status;
}


PfcCacheStatus_e statsGetCacheStatus(int handle)
{
   return gCacheStatsArray[handle].CacheStatus;
}

void statsSetInvalidateEntry(int handle)
{
   gCacheStatsArray[handle].EntryEmpty = 0;
}

