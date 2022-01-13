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
 * @file           persistence_hm_dbus_message.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor dbus message dispatching.
 * @see
 */

#include "persistence_hm_dbus_message.h"

#include "persistence_hm_dbus_service.h"
#include "persistence_hm_definitions.h"
#include "persistence_hm_fs_tools.h"

#include <persistence_admin_service.h>		// use the PAS to setup new data on the partition
#include <persComDataOrg.h>					// use defines for persistence data folder
#include <string.h>

// TODO: only for testing => replace with the path on the target
static const char* gResourcePath = "/home/ihuerner/development/git_stash/persistence-client-library/test/data/PAS_data.tar.gz";


DBusHandlerResult checkPersPhmMsg(DBusConnection * connection, DBusMessage * message, void * user_data)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if((0==strcmp("org.genivi.persistence.health", dbus_message_get_interface(message))))
	{
		DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("checkPersClientMsg -> received message:"),
				                            DLT_STRING(dbus_message_get_member(message)));
		if((0==strcmp("fsCheck", dbus_message_get_member(message))))
		{
			result = msg_persFsCheck(connection, message);
		}
		else if((0==strcmp("fsCheckAndRecover", dbus_message_get_member(message))))
		{
			result = msg_persFsCheckAndRecover(connection, message);
		}
		else if((0==strcmp("mount", dbus_message_get_member(message))))
		{
			result = msg_persFsMount(connection, message);
		}
		else if((0==strcmp("umount", dbus_message_get_member(message))))
		{
			result = msg_persFsUnMount(connection, message);
		}
		else if((0==strcmp("createPartition", dbus_message_get_member(message))))
		{
			result = msg_persFsCreatePartition(connection, message);
		}
		else
		{
			 DLT_LOG(phmContext, DLT_LOG_WARN, DLT_STRING("   checkPersClientMsg -> unknown message"),
														  DLT_STRING(dbus_message_get_interface(message)));
		}
	}
	return result;
}



DBusHandlerResult msg_persFsCheckAndRecover(DBusConnection *connection, DBusMessage *message)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;

	FSType_e fsType = FSType_LastEntry;
   char* fsTypeString = NULL;
	char* deviceName = NULL;

   DBusMessage *reply;
   DBusError error;
   dbus_error_init (&error);

   if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING , &fsTypeString,
                                               DBUS_TYPE_STRING , &deviceName,
                                               DBUS_TYPE_INVALID))
   {
   	reply = dbus_message_new_error(message, error.name, error.message);

		if (reply == 0)
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		if (!dbus_connection_send(connection, reply, 0))
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   fsType = getFsType(fsTypeString);

   unmountFS(PERS_ORG_LOCAL_APP_CACHE_PATH, 0);		// unmount partition	==> TODO: check if the partition really needs to be unmounted
   unmountFS(PERS_ORG_LOCAL_APP_WT_PATH, 0);			// unmount partition	==> TODO: check if the partition really needs to be unmounted

   //if(-1 != checkFS(deviceName, fsType))	// just for testing
	if(4 == checkFS(deviceName, fsType))	// 4 - File system errors left uncorrected ==> create a new partition and populate with data
	{
		if(-1 != createNewPartition(deviceName, PERS_ORG_LOCAL_APP_CACHE_PATH, fsType, 0))
		{
			int ret = 0;
			// mount partition
			mountFS(deviceName, PERS_ORG_LOCAL_APP_CACHE_PATH, fsType, 0);
			mountFS(deviceName, PERS_ORG_LOCAL_APP_WT_PATH, fsType, 0);

			// populate partition with data
			if((ret = persAdminResourceConfigAdd(gResourcePath)) >= 0)
			{
				DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("Succeeded to setup persistence data"), DLT_INT(ret));
			}
			else
			{
				DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("Failed to setup persistence data!!"));
			}
		}
	}
	else
	{
		DLT_LOG(phmContext, DLT_LOG_INFO, DLT_STRING("FS OK; nothing to do!!"));
	}

   return result;
}



DBusHandlerResult msg_persFsCheck(DBusConnection *connection, DBusMessage *message)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;
	FSType_e fsType = FSType_LastEntry;
	char* fsTypeString = NULL;
	char* deviceName = NULL;

	DBusMessage *reply;
	DBusError error;
	dbus_error_init (&error);

	if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING , &fsTypeString,
															  DBUS_TYPE_STRING , &deviceName,
															  DBUS_TYPE_INVALID))
	{
		reply = dbus_message_new_error(message, error.name, error.message);

		if (reply == 0)
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		if (!dbus_connection_send(connection, reply, 0))
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	fsType = getFsType(fsTypeString);

	// unmount
   unmountFS(PERS_ORG_LOCAL_APP_CACHE_PATH, 0);		// unmount partition	==> TODO: check if the partition really needs to be unmounted
   unmountFS(PERS_ORG_LOCAL_APP_WT_PATH, 0);			// unmount partition	==> TODO: check if the partition really needs to be unmounted

	// check the fs
	(void)checkFS(deviceName, getFsType(fsTypeString));

   mountFS(deviceName, PERS_ORG_LOCAL_APP_CACHE_PATH, fsType, 0);	// unmount partition	==> TODO: check if the partition really needs to be unmounted
   mountFS(deviceName, PERS_ORG_LOCAL_APP_WT_PATH, fsType, 0);		// unmount partition	==> TODO: check if the partition really needs to be unmounted

	return result;
}



DBusHandlerResult msg_persFsMount(DBusConnection *connection, DBusMessage *message)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;
	FSType_e fsType = FSType_LastEntry;
	char* fsTypeString = NULL;
	char* deviceName = NULL;

	DBusMessage *reply;
	DBusError error;
	dbus_error_init (&error);

	if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING , &fsTypeString,
															  DBUS_TYPE_STRING , &deviceName,
															  DBUS_TYPE_INVALID))
	{
		reply = dbus_message_new_error(message, error.name, error.message);

		if (reply == 0)
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		if (!dbus_connection_send(connection, reply, 0))
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	fsType = getFsType(fsTypeString);

   mountFS(deviceName, PERS_ORG_LOCAL_APP_CACHE_PATH, fsType, 0);
   mountFS(deviceName, PERS_ORG_LOCAL_APP_WT_PATH, fsType, 0);

	return result;
}



DBusHandlerResult msg_persFsUnMount(DBusConnection *connection, DBusMessage *message)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;

	char* fsTypeString = NULL;
	char* deviceName = NULL;

	DBusMessage *reply;
	DBusError error;
	dbus_error_init (&error);

	if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING , &fsTypeString,
															  DBUS_TYPE_STRING , &deviceName,
															  DBUS_TYPE_INVALID))
	{
		reply = dbus_message_new_error(message, error.name, error.message);

		if (reply == 0)
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		if (!dbus_connection_send(connection, reply, 0))
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

   unmountFS(PERS_ORG_LOCAL_APP_CACHE_PATH, 0);		// unmount partition	==> TODO: check if the partition really needs to be unmounted
   unmountFS(PERS_ORG_LOCAL_APP_WT_PATH, 0);			// unmount partition	==> TODO: check if the partition really needs to be unmounted

	return result;
}



DBusHandlerResult msg_persFsCreatePartition(DBusConnection *connection, DBusMessage *message)
{
	DBusHandlerResult result = DBUS_HANDLER_RESULT_HANDLED;

	char* fsTypeString = NULL;
	char* deviceName = NULL;

	DBusMessage *reply;
	DBusError error;
	dbus_error_init (&error);

	if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING , &fsTypeString,
															  DBUS_TYPE_STRING , &deviceName,
															  DBUS_TYPE_INVALID))
	{
		reply = dbus_message_new_error(message, error.name, error.message);

		if (reply == 0)
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		if (!dbus_connection_send(connection, reply, 0))
		{
			DLT_LOG(phmContext, DLT_LOG_ERROR, DLT_STRING("DBus No memory"));
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

   unmountFS(PERS_ORG_LOCAL_APP_CACHE_PATH, 0);		// unmount partition	==> TODO: check if the partition really needs to be unmounted
   unmountFS(PERS_ORG_LOCAL_APP_WT_PATH, 0);			// unmount partition	==> TODO: check if the partition really needs to be unmounted

	createNewPartition(deviceName, PERS_ORG_LOCAL_APP_CACHE_PATH, getFsType(fsTypeString), 0);

	return result;
}
