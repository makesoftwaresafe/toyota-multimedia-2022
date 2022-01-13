/************************************************************************
* @file: CAppHoptplugReceiver.cpp
*
* @version: 1.1
*
* CAppHoptplugReceiver class implements the interface IRaHotplugReceive
* for communication from Application (Hotplug) to Routing Adapter using over D-Bus
* 
* @component: platform/audiomanager
*
* @author: Nrusingh Dash <ndash@jp.adit-jv.com>
*
* @copyright (c) 2010, 2011 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* @see <related items>
*
* @history
*
***********************************************************************/

#include <CAppHotplugReceiver.h>
#include <CDBusSender.h>
#include <CDBusReceiver.h>
#include "RaDBusConfig.h"
#include <dlt/dlt.h>
#include <Log.h>

DLT_IMPORT_CONTEXT(raContext)

CAppHoptplugReceiver::CAppHoptplugReceiver() :
mpDBusConnection(0),
mBusName(DBUS_RA_HOTPLUG_SERVICE_PREFIX),
mPath(DBUS_HOTPLUG_RECEIVE_INTERFACE_OBJECT_PATH),
mInterface(DBUS_HOTPLUG_RECEIVE_INTERFACE),
mAppHotplugSenderShadow()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

CAppHoptplugReceiver::~CAppHoptplugReceiver()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

am::am_Error_e CAppHoptplugReceiver::init(am::CAmSocketHandler *socketHandler, IRaHotplugSend* hotplugsendinterface)
{
	LOG_FN_ENTRY(raContext);
	am::am_Error_e status = mAppHotplugSenderShadow.init(socketHandler,
														hotplugsendinterface,
														DBUS_APP_HOTPLUG_SERVICE_PREFIX,
														DBUS_APP_HOTPLUG_SERVICE_OBJECT_PATH,
														DBUS_HOTPLUG_SEND_INTERFACE,
														DBUS_HOTPLUG_SEND_INTERFACE_OBJECT_PATH,
														DBUS_BUS_SYSTEM);

	if (status == am::E_OK)
	{
		mAppHotplugSenderShadow.getConnection(mpDBusConnection);
		if (NULL == mpDBusConnection)
		{
			LOG_ERROR(raContext, DLT_STRING(" NULL == mpDBusConnection"));
			status = am::E_UNKNOWN;
		}
	}

	LOG_FN_EXIT(raContext);
	return status;
}

void CAppHoptplugReceiver::getInterfaceVersion(std::string& version) const
{
	(void) (version);
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::confirmHotplugReady(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "confirmHotplugReady", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncRegisterSink(const am::am_Sink_s& sinkData)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncRegisterSink", mBusName, mPath, mInterface);
	send.append(sinkData);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncDeregisterSink(const am::am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncDeregisterSink", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncRegisterSource(const am::am_Source_s& sourceData)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncRegisterSource", mBusName, mPath, mInterface);
	send.append(sourceData);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncDeregisterSource(const am::am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncDeregisterSource", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSinkVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSinkVolumeChange", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(volume);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSourceVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSourceVolumeChange", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(volume);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSourceState(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSourceState", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSinkSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSinkSoundProperties", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSinkSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSinkSoundProperty", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSourceSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSourceSoundProperties", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::ackSetSourceSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "ackSetSourceSoundProperty", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(error);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::hookInterruptStatusChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "hookInterruptStatusChange", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append((uint16_t)interruptState);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::hookSinkAvailablityStatusChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "hookSinkAvailablityStatusChange", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.append(availability);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::hookSourceAvailablityStatusChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "hookSourceAvailablityStatusChange", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append(availability);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::hookSinkNotificationDataChange(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "hookSinkNotificationDataChange", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.append(payload);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::hookSourceNotificationDataChange(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "hookSourceNotificationDataChange", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append(payload);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncUpdateSource", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append(sourceClassID);
	send.append(listSoundProperties);
	send.append(listConnectionFormats);
	send.append(listMainSoundProperties);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CAppHoptplugReceiver::asyncUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
	LOG_FN_ENTRY(raContext);
	CDBusSender send(mpDBusConnection, "asyncUpdateSink", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.append(sinkClassID);
	send.append(listSoundProperties);
	send.append(listConnectionFormats);
	send.append(listMainSoundProperties);
	send.send_async();
	LOG_FN_EXIT(raContext);
}
