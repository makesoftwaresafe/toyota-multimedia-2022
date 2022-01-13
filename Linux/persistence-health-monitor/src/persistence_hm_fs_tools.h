#ifndef PERSISTENCE_HM_FS_TOOLS_H_
#define PERSISTENCE_HM_FS_TOOLS_H_

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
 * @file           persistence_hm_fs_tools.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor file system tools.
 * @see
 */

#include <dbus/dbus.h>


/// storages to manage the data
typedef enum FSType_e_
{
	/// file system type ext2
	FSType_Ext2 = 0,
	/// file system type ext3
   FSType_Ext3,
   /// file system type ext4
   FSType_Ext4,
   /// file system type btrfs
   FSType_Btrfs,

   // insert new entries here ...

   /// last entry
   FSType_LastEntry

} FSType_e;


FSType_e getFsType(const char* fsId);


int checkFS(const char* deviceName, FSType_e fsType);


int mountFS(const char* deviceName,const char* mountPointPath, FSType_e fsType, unsigned long mountflags);


int unmountFS(const char* mountPointPath, int mountflags);


int createNewPartition(const char* deviceName, const char* mountpoint, FSType_e fsType, unsigned long mountflags);


#endif /* PERSISTENCE_HM_FS_TOOLS_H_ */
