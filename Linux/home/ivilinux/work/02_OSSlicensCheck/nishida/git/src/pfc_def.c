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
 * @file           persistence_file_cache_data.c
 * @ingroup        Persistence file cache
 * @author         Ingo Huerner (XSe)
 * @brief          implementation of the persistence file cache data.
 *
 */


#include "pfc_def.h"


/// path of the cached location
char gCachPathLocation[PathMaxLen] = "/var/run/pers_ramfs"; // default path

/// path of the backup location
char gBackupPathLocation[PathMaxLen] = "/Data/mnt-backup"; // default path

/// default configuration file location
const char* gDefaultConfig = "/etc/persistence_pfc.conf";

unsigned int gPfcInitialized = PFCnotInitialized;

unsigned int gCreateBackup = 1;     // backup creation enabled by default


/// the DLT context
DltContext gDLTContext;
