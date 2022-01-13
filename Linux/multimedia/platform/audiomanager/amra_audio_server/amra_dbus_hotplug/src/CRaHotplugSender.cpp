/************************************************************************
* @file: CRaHotplugSender.cpp
*
* @version: 1.1
*
* CRaHotplugSender class implements the callback inerface IRaHotplugSend
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
#include <dbus/dbus.h>
#include "CRaHotplugSender.h"
#include "CDBusSender.h"
#include "CDBusReceiver.h"
#include "RaDBusConfig.h"
#include <dlt/dlt.h>
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext)

CRaHotplugSender::CRaHotplugSender() :
	mpDBusConnection(NULL),
	mBusName(DBUS_APP_HOTPLUG_SERVICE_PREFIX),
	mPath(DBUS_HOTPLUG_SEND_INTERFACE_OBJECT_PATH),
	mInterface(DBUS_HOTPLUG_SEND_INTERFACE),
	mHotplugReceiverShadow()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

CRaHotplugSender::~CRaHotplugSender()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

void CRaHotplugSender::getInterfaceVersion(std::string& version) const
{
	(void) (version);
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
	return;
}

am::am_Error_e CRaHotplugSender::init(CAmSocketHandler *mainLoopHandler, IRaHotplugReceive* hotplugReceiver)
{
	LOG_FN_ENTRY(raContext);
	am::am_Error_e status = mHotplugReceiverShadow.init(mainLoopHandler,
							hotplugReceiver,
							DBUS_RA_HOTPLUG_SERVICE_PREFIX,
							DBUS_RA_HOTPLUG_SERVICE_OBJECT_PATH,
							DBUS_HOTPLUG_RECEIVE_INTERFACE,
							DBUS_HOTPLUG_RECEIVE_INTERFACE_OBJECT_PATH,
							DBUS_BUS_SYSTEM);

	if (status == am::E_OK)
	{
		mHotplugReceiverShadow.getConnection(mpDBusConnection);
		if (NULL == mpDBusConnection)
		{
			LOG_ERROR(raContext, DLT_STRING("CRaHotplugSender::init -- NULL == mpDBusConnection"));
			status = am::E_UNKNOWN;
		}
	}

	LOG_FN_EXIT(raContext);
	return status;
}

void CRaHotplugSender::setHotplugReady(const uint16_t handle)
{
	(void) (handle);
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

am::am_Error_e CRaHotplugSender::asyncSetSinkVolume(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSinkVolume", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sinkID);
	send.append(volume);
	send.append(static_cast<int16_t>(ramp));
	send.append(time);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;
}

am::am_Error_e CRaHotplugSender::asyncSetSourceVolume(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSourceVolume", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sourceID);
	send.append(volume);
	send.append(static_cast<int16_t>(ramp));
	send.append(time);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;

}

am::am_Error_e CRaHotplugSender::asyncSetSourceState(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SourceState_e state)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSourceState", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sourceID);
	send.append(static_cast<int16_t>(state));
	am_Error_e ret = send.send_async();
    /*
     * TODO
     */
	LOG_FN_EXIT(raContext);
	return ret;
}

am::am_Error_e CRaHotplugSender::asyncSetSinkSoundProperties(const uint16_t handle, const am::am_sinkID_t sinkID, const std::vector<am::am_SoundProperty_s>& listSoundProperties)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSinkSoundProperties", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sinkID);
	send.append(listSoundProperties);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;
}

am::am_Error_e CRaHotplugSender::asyncSetSinkSoundProperty(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_SoundProperty_s& soundProperty)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSinkSoundProperty", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sinkID);
	send.append(soundProperty);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;
}

am::am_Error_e CRaHotplugSender::asyncSetSourceSoundProperties(const uint16_t handle, const am::am_sourceID_t sourceID, const std::vector<am::am_SoundProperty_s>& listSoundProperties)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSourceSoundProperties", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sourceID);
	send.append(listSoundProperties);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;
}

am::am_Error_e CRaHotplugSender::asyncSetSourceSoundProperty(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SoundProperty_s& soundProperty)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "asyncSetSourceSoundProperty", mBusName, mPath, mInterface);
	send.append(handle);
	send.append(sourceID);
	send.append(soundProperty);
    am_Error_e ret = send.send_async();
    /*
     * TODO
     */
    LOG_FN_EXIT(raContext);
    return ret;
}

void CRaHotplugSender::ackRegisterSink(const std::string sinkName, const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackRegisterSink", mBusName, mPath, mInterface);
	send.append(sinkName);
	send.append(sinkID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CRaHotplugSender::ackDeregisterSink(const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackDeregisterSink", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CRaHotplugSender::ackRegisterSource(const std::string sourceName, const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackRegisterSource", mBusName, mPath, mInterface);
	send.append(sourceName);
	send.append(sourceID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CRaHotplugSender::ackDeregisterSource(const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackDeregisterSource", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}
void CRaHotplugSender::ackUpdateSource(const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackUpdateSource", mBusName, mPath, mInterface);
	send.append(sourceID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}

void CRaHotplugSender::ackUpdateSink(const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	LOG_FN_ENTRY(raContext);
	am::CDBusSender send(mpDBusConnection, "ackUpdateSink", mBusName, mPath, mInterface);
	send.append(sinkID);
	send.append(status);
	send.send_async();
	LOG_FN_EXIT(raContext);
}
