/************************************************************************
* @file: CRaHotplugSender.h
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
#ifndef _C_RA_HOTPLUG_SENDER_H_
#define _C_RA_HOTPLUG_SENDER_H_
#include <string>
#include <dbus/dbus.h>
#include "audiomanagertypes.h"
#include "IRaHotplugReceive.h"
#include "IRaHotplugSend.h"
#include "IRaHotplugReceiverShadow.h"

class CRaHotplugSender : public IRaHotplugSend
{
public:
	CRaHotplugSender();
	virtual ~CRaHotplugSender();
	void getInterfaceVersion(std::string& version) const;
	am::am_Error_e init(am::CAmSocketHandler *socketHandler, IRaHotplugReceive* hotplugreceiveinterface);
	void setHotplugReady(const uint16_t handle);
	am::am_Error_e asyncSetSinkVolume(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time);
	am::am_Error_e asyncSetSourceVolume(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time);
	am::am_Error_e asyncSetSourceState(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SourceState_e state);
	am::am_Error_e asyncSetSinkSoundProperties(const uint16_t handle, const am::am_sinkID_t sinkID, const std::vector<am::am_SoundProperty_s>& listSoundProperties);
	am::am_Error_e asyncSetSinkSoundProperty(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_SoundProperty_s& soundProperty);
	am::am_Error_e asyncSetSourceSoundProperties(const uint16_t handle, const am::am_sourceID_t sourceID, const std::vector<am::am_SoundProperty_s>& listSoundProperties);
	am::am_Error_e asyncSetSourceSoundProperty(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SoundProperty_s& soundProperty);
	void ackRegisterSink(const std::string sinkName, const am::am_sinkID_t sinkID, const am::am_Error_e status);
	void ackDeregisterSink(const am::am_sinkID_t sinkID, const am::am_Error_e status);
	void ackRegisterSource(const std::string sourceName, const am::am_sourceID_t sourceID, const am::am_Error_e status);
	void ackDeregisterSource(const am::am_sourceID_t sourceID, const am::am_Error_e status);
	void ackUpdateSource(const am::am_sourceID_t sourceID, const am::am_Error_e status);
	void ackUpdateSink(const am::am_sinkID_t sinkID, const am::am_Error_e status);
private:
	DBusConnection* mpDBusConnection;
	std::string mBusName;
	std::string mPath;
	std::string mInterface;
	IRaHotplugReceiverShadow mHotplugReceiverShadow;
};

#endif //_C_RA_HOTPLUG_SENDER_H_
