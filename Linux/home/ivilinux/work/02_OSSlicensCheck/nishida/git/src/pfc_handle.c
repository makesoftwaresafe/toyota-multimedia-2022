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
 * @file           persistence_file_cache_handle.c
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence file cache handle
 * @see
 */


#include "pfc_handle.h"

/// file handle structure definition
typedef struct _PersistenceFileHandle_s
{
   int  backupCreated;                    /// indicator if backup has been already created
   int  cacheHandle;                      /// the cache handle
   int  cursorPos;                        /// the cursor position in the file
   char backupPath[PathMaxLen];           /// the backup file
   char csumPath[PathMaxLen];             /// the checksum file
   char cachePath[PathMaxLen];            /// the cache path

} PersistenceFileHandle_s;

/// persistence file handle array
static PersistenceFileHandle_s gFileHandleArray[MaxPersHandle];


void handleSetMultiple(int handle, int  backupCreated, int cacheHandle, int cursorPos,
                       char* backupPath, char* csumPath, char* cachePath)
{
   gFileHandleArray[handle].backupCreated = backupCreated;
   gFileHandleArray[handle].cacheHandle   = cacheHandle;
   gFileHandleArray[handle].cursorPos     = cursorPos;
   strncpy(gFileHandleArray[handle].backupPath, backupPath, PathMaxLen);
   gFileHandleArray[handle].backupPath[PathMaxLen-1] = '\0';   // Ensures 0-Termination
   strncpy(gFileHandleArray[handle].csumPath,   csumPath,   PathMaxLen);
   gFileHandleArray[handle].csumPath[PathMaxLen-1] = '\0';     // Ensures 0-Termination
   strncpy(gFileHandleArray[handle].cachePath,  cachePath,  PathMaxLen);
   gFileHandleArray[handle].cachePath[PathMaxLen-1] = '\0';    // Ensures 0-Termination
}

void handleClearMultiple(int handle)
{
   //printf("       handleClearMultiple => handle[%d]\n", handle);
   gFileHandleArray[handle].cursorPos = 0;
   gFileHandleArray[handle].backupCreated =  -1;
   gFileHandleArray[handle].cacheHandle   = -1;
   memset(gFileHandleArray[handle].backupPath, 0, PathMaxLen);
   gFileHandleArray[handle].backupPath[PathMaxLen-1] = '\0';   // Ensures 0-Termination
   memset(gFileHandleArray[handle].csumPath,   0, PathMaxLen);
   gFileHandleArray[handle].csumPath[PathMaxLen-1] = '\0';     // Ensures 0-Termination
   memset(gFileHandleArray[handle].cachePath,  0, PathMaxLen);
   gFileHandleArray[handle].cachePath[PathMaxLen-1] = '\0';    // Ensures 0-Termination
}



void handleSetBackupStatus(int handle, int  backupCreated)
{
   gFileHandleArray[handle].backupCreated = backupCreated;
}

int handleGetBackupStatus(int handle)
{
   return gFileHandleArray[handle].backupCreated;
}



void handleSetCacheHandle(int handle, int cacheHandle)
{
   gFileHandleArray[handle].cacheHandle = cacheHandle;
}

int handleGetCacheHandle(int handle)
{
   return gFileHandleArray[handle].cacheHandle;
}



void handleSetCursorPos(int handle, int cursorPos)
{
   gFileHandleArray[handle].cursorPos = cursorPos;
}

int handleGetCursorPos(int handle)
{
   return gFileHandleArray[handle].cursorPos;
}



void handleSetBackupPath(int handle, char* backupPath)
{
   strncpy(gFileHandleArray[handle].backupPath, backupPath, PathMaxLen);
}

char* handleGetBackupPath(int handle)
{
   return gFileHandleArray[handle].backupPath;
}



void handleSetChecksumPath(int handle, char* csumPath)
{
   strncpy(gFileHandleArray[handle].csumPath,   csumPath,   PathMaxLen);
}

char* handleGetChecksumPath(int handle)
{
   return gFileHandleArray[handle].csumPath;
}



void handleSetCachepath(int handle, char* cachePath)
{
   strncpy(gFileHandleArray[handle].cachePath,  cachePath,  PathMaxLen);
}

char* handleGetCachepath(int handle)
{
   return gFileHandleArray[handle].cachePath;
}
