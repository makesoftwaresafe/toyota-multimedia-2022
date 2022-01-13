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
 * @file           persistence_file_cache.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence file cache library.
 *                 Library provides an API to cached files
 * @see            
 */


#include "persistence_file_cache.h"
#include "pfc_def.h"
#include "pfc_stats.h"
#include "pfc_handle.h"
#include "pfc_file_helper.h"
#include "pfc_config_reader.h"

#include "../include/persistence_file_cache_error_def.h"

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>

#include <sys/stat.h>


// TBD
/// provide also flag parameter to the open function????
typedef enum _PfcOpenFlag_e
{
   Cache_ReadOnly = 0,
   Cache_ReadWrite

} _PfcOpenFlag_e;



//-------------------------------------------------------------------
// function prototypes of internal helper functions
// ------------------------------------------------------------------
int pfcHelperRemoveFromCache(int handle, unsigned int fileSize);
int pfcHelperDelFilesRecursive(const char* dir_name);
int pfcHelperWriteBack(int srcFd, int dstFd);
void pfcHelperRemoveComplete(int handle);
int pfcHelperDoBackup(int handle);
// ------------------------------------------------------------------


/// write back from cache to non volatile memory device
/// ATTENTION:
/// THIS FUNCTION IS NOT INTENDED TO BE USED BY A NORMAL APPLICATION.
/// ONLY SPECIAL APPLICATION ARE ALLOWED TO USING USE THIS FUNCTION
int pfcWriteBackAndSync(int handle);


/**
 * initialize library
 */

int pfcInitCache(const char* appID)
{
   int ravl = 1;

   if(gPfcInitialized == PFCnotInitialized)
   {
      unsigned int cacheLimit = DefaultCacheLimit;  		// init with default cache

      DLT_REGISTER_CONTEXT(gDLTContext,"PFC","Context for persistence file cache logging");

      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pfcInitCache => app: "), DLT_STRING(appID));

      if(getConfiguration(appID, &cacheLimit) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("pfcInitCache ==> failed to read configuration, using defaults"));
      }
      else
      {
      	DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pfcInitCache ==> to read configuration, cacheLimit: "), DLT_INT(cacheLimit));
      }

      statsSetMaxCacheSize(cacheLimit);

      gPfcInitialized++;

      // init
      statsInitCacheStats();
   }
   else if(gPfcInitialized >= PFCinitialized)
   {
      gPfcInitialized++; // increment init counter
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("   - ONLY INCREMENT init counter: "), DLT_INT(gPfcInitialized),
      		                             DLT_STRING(" app: "), DLT_STRING(appID) );
   }

#if 0
      printf("\nCache  Path: %s\n", gCachPathLocation);
      printf("Backup Path: %s\n", 	gBackupPathLocation);
      printf("Cache Limit: %d\n", 	statsGetMaxCacheSize());
      printf("Backup?    : %d\n\n", gCreateBackup);
#endif

   return ravl;
}


int pfcDeinitCache(void)
{
   int rval = 1;

	if(gPfcInitialized == PFCinitialized)
	{
		gPfcInitialized = PFCnotInitialized;

      DLT_UNREGISTER_CONTEXT(gDLTContext);
   }
   else if(gPfcInitialized > PFCinitialized)
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pfcDeinitCache -> D E I N I T  file cache - "),
                                         DLT_STRING("- ONLY DECREMENT init counter: "), DLT_INT(gPfcInitialized));
      gPfcInitialized--;   // decrement init counter
   }
   else
   {
      rval = -1;
   }

   return rval;
}



int pfcOpenFile(const char* path, PfcFileCreateMode mode)
{
   return pfcOpenFileAffinity(path, mode, CacheAffMedium);
}


/**
 * TODO:
 * - Check for consistency
 * - open file
 *
 *
 */
// PfcCachePriority_e cachePrio
int pfcOpenFileAffinity(const char* path, PfcFileCreateMode mode, PfcCacheAffinity affinity)
{
   int handle = -1;
   int flags = O_RDWR;     // open all files read/writable

   char cachePath[PathMaxLen]  = {0};
   char backupPath[PathMaxLen] = {0};
   char csumPath[PathMaxLen]   = {0};

   snprintf(cachePath,  PathMaxLen, "%s%s", gCachPathLocation, path);
   snprintf(backupPath, PathMaxLen, "%s%s", gBackupPathLocation, path);
   snprintf(csumPath,   PathMaxLen, "%s%s", backupPath, ".crc");

   if(gCreateBackup == 1)  // check if backup is needed
   {
      if((handle = helperVerifyConsistency(path, backupPath, csumPath, flags)) == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcPrioOpenFile: error => file inconsistent, recovery  N O T  possible!"));
         return -1;
      }
   }
   else
   {
      DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pfcPrioOpenFile: backup creation DISABLED"));
   }

   if(handle <= 0)   // check if open is needed or already done in verifyConsistency
   {
      handle = open(path, flags);

      if(handle == -1)
      {
      	if(errno == ENOENT && mode == CreateFile) // file does not exist, create file and folder
      	{
      		handle = helperCreateAndOpenFile(path);
      	}
      }
   }

   if(handle != -1)
   {
      // init file handle array
      handleSetMultiple(handle, -1, -1, 0, backupPath, csumPath, cachePath);
      statsSingleInitCacheStats(handle, affinity);
      statsSetCacheStatus(handle, CacheStatusNotInCache);
   }

#if 0
   printf("Handle: %d\n", handle);
   printf("* * * * * backupPath: %s \n    \"%s\"\n", backupPath, handleGetBackupPath(handle));
   printf("* * * * * csumPath  : %s \n    \"%s\"\n", csumPath, handleGetChecksumPath(handle));
   printf("* * * * * cachePath : %s \n    \"%s\"\n", cachePath, handleGetCachepath(handle));
#endif

   return handle;
}



/**
 */
int pfcCloseFile(int handle)
{
   int rval = 0;

   if(handle < MaxPersHandle && handle >= 0)
   {
      if(statsGetCacheStatus(handle)  == CacheStatusInCache)
      {
         if(gCreateBackup == 1)  // check if backup is configured
         {
            helperRemoveBackup(handle);            // remove backup
         }

         statsSingleClearCacheStats(handle);
         pfcHelperRemoveComplete(handle);
      }

      rval = close(handle);
      if(rval == -1)
      {
         DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("pfcCloseFile => Failed to close file ==> fd: %d\n"),
         		                             DLT_INT(handle), DLT_STRING(strerror(errno)) );
      }
   }
   else
   {
      //printf("pfcCloseFile => handle exceed limit: %d\n", handle);
      rval = EPRS_MAX_HANDLE;
   }

   return rval;
}



/**
 * TODO:
 * - first access: create a copy in ramdisk
 * - all further access will got the the RAM version
 * - keep origin file open
 */
int pfcWriteFile(int handle, const void *buf, size_t nbyte)
{
   int rval = -1, fd = handle, cacheHandle = -1;  // set fd to non cache handle by default

   if(handle < MaxPersHandle && handle >= 0)
   {
      if(gCreateBackup == 1)  // check if backup is configured
      {
         if(handleGetBackupStatus(handle) != BackupCreated)    // check if a backup file has to be created
         {
            pfcHelperDoBackup(handle);
         }
      }

      if(statsGetCacheStatus(handle)  == CacheStatusNotInCache)   // check if file is already in the cache
      {
         struct stat statBuf;
         memset(&statBuf, 0, sizeof(statBuf));

         if(fstat(handle, &statBuf) != -1)
         {
            unsigned int fileSizeInCache = statBuf.st_size + nbyte;  // the file size after write

            if(fileSizeInCache < statsGetMaxCacheSize())             // first check if file could be kept in cache at all
            {
   #if 0
               if(pfcHelperRemoveFromCache(handle, fileSizeInCache) != -1)
               {
   #endif
                  fd = cacheHandle = helperCreateAndOpenFile(handleGetCachepath(handle));    // create the file

                  if(fd != -1)
                  {
                     handleSetCacheHandle(handle, cacheHandle);                  // remember the cache handle

                     if(helperDoFileCopy(handle, cacheHandle) != -1)             // copy file into the cache
                     {
                        statsAddCacheUsage(fileSizeInCache);                     // update statistics
                        statsSetCacheSize(handle, fileSizeInCache );
                        statsSetCacheStatus(handle, CacheStatusInCache);
                        rval = 1;
                        // everthing OK ==> write to file
                     }
                     else
                     {
                        rval = EPERS_COPY_FILE_CACHE; //printf("pfcWriteFile => Failed to copy file to cache: %d | %s \n", errno, strerror(errno));
                     }
                  }
                  else
                  {
                     rval = EPERS_CREATE_FILE_CACHE; //printf("pfcWriteFile => Failed to create the file in the cache: %d | %s \n", errno, strerror(errno));
                  }
   #if 0
               }

               else
               {
                  rval = EPERS_CACHE_FULL; //printf("pfcWriteFile => No space in cache available\n");
               }
   #endif
            }
            else
            {
               rval = EPRS_FILE_TO_BIG; //printf("pfcWriteFile => file to big: file size: %d | cache size: %d \n", fileSizeInCache, statsGetMaxCacheSize() );
            }
         }
         else
         {
            rval = EPERS_CREATE_STAT_FILE;   printf("pfcWriteFile => Failed to stat file\n");
         }
      }
      else if(statsGetCacheStatus(handle)  == CacheStatusInCache)    // file is already in cache
      {
         unsigned int fileSizeInCache = nbyte + statsGetCacheSize(handle);
         //printf("File  A L R E A D Y  in cache: %d\n", handle);

         if(fileSizeInCache < statsGetMaxCacheSize())    // first check if file could be kept in cache at all
         {
            if(pfcHelperRemoveFromCache(handle, nbyte) != -1)
            {
               fd = handleGetCacheHandle(handle);     // get the cached fd
               statsAddCacheUsage(nbyte);             // correct the cache count
               rval = 1;                              // everthing OK ==> write to file
            }
            else
            {
               rval = EPERS_CACHE_FULL;   //printf("pfcWriteFile => No space in cache available\n");
            }
         }
         else
         {
            rval = EPRS_FILE_TO_BIG;   //printf("pfcWriteFile => file to big: file size: %d | cache size: %d \n", fileSizeInCache, statsGetMaxCacheSize() );
         }
      }
      else
      {
         rval = EPERS_CACHE_STATUS_UNDEF; //printf("    pfcWriteFile ==> undefined cache status!!!!!\n");
      }

      if(rval == 1 && fd != -1)                 // finally write data to file
      {
         rval = write(fd, buf, nbyte);
         statsUpdateCacheFactor(handle);        // update cache statistics
      }
   }
   else
   {
      //printf("pfcWriteFile => handle exceed limit: %d\n", handle);
      rval = EPRS_MAX_HANDLE;
   }


   return rval;
}



/**
 * TODO:
 * - read from the correct file
 *    * original
 *    * file in cache
 * - set the correct filedescriptor
 */
int pfcReadFile(int handle, void *buf, size_t count)
{
   int fd = handle;  // by default read form the file not in cache
   int rval = -1;

   if(handle < MaxPersHandle && handle >= 0)
   {
      if(statsGetCacheStatus(handle) == CacheStatusInCache)
      {
         fd = handleGetCacheHandle(handle);  // read from file in cache
      }

      if(fd != -1)
      {
         rval = read(fd, buf, count);
         statsUpdateCacheFactor(handle);        // update cache statistics
      }
   }
   else
   {
      rval = EPRS_MAX_HANDLE;
   }


   return rval;
}



int pfcFileSeek(int handle, long int offset, int whence)
{
   int fd = handle;  // by default seek the non cached file
   int rval = -1;

   if(handle < MaxPersHandle && handle >= 0)
   {
      if(statsGetCacheStatus(handle) == CacheStatusInCache)
      {
         fd = handleGetCacheHandle(handle);     // seek the cache file
      }

      if(fd != -1)
      {
         rval = lseek(fd, offset, whence);
      }
   }
   else
   {
      rval = EPRS_MAX_HANDLE;
   }

   return rval;
}



int pfcFileGetSize(int handle)
{
   int size = -1, fd = handle;
   struct stat buf;

   if(handle < MaxPersHandle && handle >= 0)
   {
      if(statsGetCacheStatus(handle) == CacheStatusInCache)
      {
         fd = handleGetCacheHandle(handle);     // get the cache handle
      }

      size = fstat(fd, &buf);

      if(size != -1)
      {
         size = buf.st_size;
      }
   }
   else
   {
      size = EPRS_MAX_HANDLE;
   }

   return size;
}


//-------------------------------------------------------------------
// be careful when using this function from outside the library
// ------------------------------------------------------------------
int pfcWriteBackAndSync(int handle)
{
	int rval = 0;
	DLT_LOG(gDLTContext, DLT_LOG_WARN, DLT_STRING("pfcWriteBackAndSync => writing back file from cache to memory device"),
			                             DLT_STRING("- Hope you know what you are doing!!!!"));

	rval = pfcHelperWriteBack(handleGetCacheHandle(handle), handle);

	if(rval != -1 )
	{
		rval = fsync(handle);
		if(rval == -1)
		{
			DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcWriteBackAndSync => Failed to fsync => fd: "), DLT_INT(handle));
		}
	}
	else
	{
		DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcWriteBackAndSync => Failed to write back and sync => fd: "), DLT_INT(handle));
	}

	return rval;
}


//-------------------------------------------------------------------
// internal helper functions
// ------------------------------------------------------------------

int pfcHelperDoBackup(int handle)
{
   int rval = 0;
   char csumBuf[ChecksumBufSize] = {0};

   helperCalcCrc32Csum(handle, csumBuf);     // calculate checksum

   // create checksum and backup file
   rval = helperCreateBackup(handleGetBackupPath(handle), handle, handleGetChecksumPath(handle), csumBuf);

   handleSetBackupStatus(handle, BackupCreated);

   return rval;
}




int pfcHelperWriteBack(int srcFd, int dstFd)
{
   int rval = -1;
   // remember the current position
   off_t curPosSrc = lseek(srcFd, 0, SEEK_CUR);
   off_t curPosDst = lseek(dstFd, 0, SEEK_CUR);

   // reset both fd to beginning of the file
   if(lseek(srcFd, 0, SEEK_SET) != -1)
   {
      if(lseek(dstFd, 0, SEEK_SET) != -1)
      {
         if(helperDoFileCopy(srcFd, dstFd) != -1)     // copy file back to memory device
         {
           rval = 0;
         }
         else
         {
         	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperWriteBack => failed to copy file "), DLT_STRING(strerror(errno)));
         }
      }
      else
      {
      	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperWriteBack => failed so seek to beginning of file => "), DLT_STRING(strerror(errno)));
      }
   }
   else
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperWriteBack => failed so seek to beginning of file =>"), DLT_STRING(strerror(errno)));
   }

   // set back to the position
   lseek(srcFd, curPosSrc, SEEK_SET);
   lseek(dstFd, curPosDst, SEEK_SET);


   return rval;
}



int pfcHelperDelFilesRecursive(const char* dir_name)
{
   int rval = 0;
   DIR * directory;

   directory = opendir(dir_name);
   if(NULL == directory)
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperDelFilesRecursive => can't open directory"), DLT_STRING(dir_name), DLT_STRING(strerror(errno)) );
      return -1;
   }

   while(1)
   {
      struct dirent * entry;
      const char * d_name;

      entry = readdir (directory);
      if(NULL == entry)
      {
         break;   // nothing
      }
      d_name = entry->d_name;

     if(entry->d_type & DT_DIR)
     {
         if(strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0) // don't go for the current or parent directory
         {
             int path_length;
             char path[PathMaxLen] = {0};

             path_length = snprintf(path, PathMaxLen, "%s/%s", dir_name, d_name);
             if(path_length >= PathMaxLen)
             {
                 return -1;
             }

             pfcHelperDelFilesRecursive(path);     // walk recoursively through the directory
         }
      }
     else if(entry->d_type & DT_REG)
     {
        char filename_path[PathMaxLen] = {0};
        snprintf(filename_path, PathMaxLen, "%s/%s", dir_name, d_name);
        if(remove(filename_path) == -1)
           DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperDelFilesRecursive => can't remove file"), DLT_STRING(filename_path), DLT_STRING(strerror(errno)) );
     }
   }

   if(closedir (directory))      // close opend directory
   {
      DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperDelFilesRecursive => Could not close "), DLT_STRING(dir_name), DLT_STRING(strerror(errno)) );
      return -1;
   }
   return rval;
}



int pfcHelperRemoveFromCache(int handle, unsigned int fileSize)
{
   int counter = 0, rval = 0;

   while((statsGetMaxCacheSize() - statsGetCacheUsage()) <  fileSize )  // not enough space in cache, remove something
   {
      int removeHandle = 0;

      statsGetHandleToRemove(-1, &removeHandle);      // find a file to remove

      if(removeHandle != handle)                      // make sure not to remove ourself
      {
         pfcHelperRemoveComplete(removeHandle);

         /*
         printf("     [%d] => pfcWriteFile: cache full, remove: %d: => %s ==> FreeSize: %d ToWrite: %d \n", counter, removeHandle, handleGetCachepath(removeHandle),
                                                                                                            statsGetMaxCacheSize() - statsGetCacheUsage(), fileSize);
         */
      }

      if(counter++ >= MaxPersHandle)
      {
         //printf("    pfcWriteFile: Nothing found to remove\n");
         rval = -1;     // we have searched the complete handles, nothing more to remove
         break;
      }
   } // while

   return rval;
}



void pfcHelperRemoveComplete(int handle)
{
	int cacheHandle = handleGetCacheHandle(handle);

   if(pfcHelperWriteBack(cacheHandle, handle) == -1)   // write back form cache to device
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperRemoveComplete => failed to write back handle: "), DLT_INT(handle));
   }

   statsSubCacheUsage(statsGetCacheSize(handle));              // subtract the cache size

   if(close(cacheHandle) == -1)
   {
   	DLT_LOG(gDLTContext, DLT_LOG_ERROR, DLT_STRING("pfcHelperRemoveComplete => failed to close cache handle: "), DLT_INT(cacheHandle));
   }

   remove(handleGetCachepath(handle));                         // remove file from cache

   statsSetCacheStatus(handle, CacheStatusNotInCache);         // reset cache status
   statsSetCacheSize(handle, 0);
   statsSetInvalidateEntry(handle);

   handleClearMultiple(handle);                                // remove from handle array
}
