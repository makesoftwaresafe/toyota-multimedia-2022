#ifndef PERSISTENCY_FILE_CACHE_DATA_H
#define PERSISTENCY_FILE_CACHE_DATA_H

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
 * @file           persistence_file_cache_data.h
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner (XSe)
 * @brief          Header of the persistence file cache data.
 *
 */


#include "persistence_file_cache_error_def.h"
#include "persistence_file_cache.h"

#include <dlt.h>
#include <dlt_common.h>


/// constant definitions
enum _PfcConstantDef
{
   FileNotInRam         =  0,       /// file status indicator: file has never been in RAM
   FileInRam            =  1,       /// file status indicator: file is currently in RAM
   FileSupsended        =  2,       /// file status indicator: file has been removed from cache

   BackupCreated        = 1,        /// indicator flag if a backup has been created

   PFCnotInitialized    = 0,        /// indication if PFC is not initialized
   PFCinitialized       = 1,        /// indication if PFC is initialized

   FileClosed           = 0,        /// flag to identify if file will be closed
   FileOpen             = 1,        /// flag to identify if file has been opend

   PathMaxLen           = 256,      /// max database path length
   MaxPersHandle        = 256,      /// max number of parallel open persistence handles

   ChecksumBufSize      = 64,       /// max checksum buffer size

   FilePathMaxLen       = 256,   	/// the max size of the filename including the path to the filename

   DefaultCacheLimit    = 102400		/// default cache limit 100kB

};



/// path of the cached location
extern char gCachPathLocation[PathMaxLen];

/// path of the backup location
extern char gBackupPathLocation[PathMaxLen];

/// default configuration file location
extern const char* gDefaultConfig;

/// indicator if file cache has been initialized.
extern unsigned int gPfcInitialized;

/// indicator if a backup should be created
extern unsigned int gCreateBackup;

/// the DLT context
extern DltContext gDLTContext;



#endif /* PERSISTENCY_FILE_CACHE_DATA_H */
