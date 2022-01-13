#ifndef PERSISTENCE_FILE_CACHE_HANDLE_H
#define PERSISTENCE_FILE_CACHE_HANDLE_H

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
 * @file           persistence_file_cachehandle.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner
 * @brief          Header of the persistence file cache handle.
 * @see            
 */

#include "pfc_def.h"

#include <string.h>


void handleSetMultiple(int handle, int  backupCreated, int cacheHandle, int cursorPos,
                       char* backupPath, char* csumPath, char* cachePath);

void handleClearMultiple(int handle);


void handleSetBackupStatus(int handle, int  backupCreated);

int handleGetBackupStatus(int handle);

void handleSetCacheHandle(int handle, int cacheHandle);

int handleGetCacheHandle(int handle);

void handleSetCursorPos(int handle, int cursorPos);

int handleGetCursorPos(int handle);

void handleSetBackupPath(int handle, char* backupPath);

char* handleGetBackupPath(int handle);

void handleSetChecksumPath(int handle, char* csumPath);

char* handleGetChecksumPath(int handle);

void handleSetCachepath(int handle, char* cachePath);


char* handleGetCachepath(int handle);
#endif /* PERSISTENCY_FILE_CACHE_HANDLE_H */

