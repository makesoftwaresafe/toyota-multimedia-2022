/************************************************************************
* @file: IRaHotplugSend.h
*
* @version: 1.1
*
* IRaHotplugSend class is an interface for the communication from
* Routing Adapter to Application (Hotplug). 
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
#ifndef __I_RA_HOTPLUG_SEND_H__
#define __I_RA_HOTPLUG_SEND_H__
#include "audiomanagertypes.h"
#include <IRaHotplugReceive.h>
#include <CAmSocketHandler.h>

class IRaHotplugSend
{
public:
	virtual ~IRaHotplugSend() {}
	virtual void getInterfaceVersion(std::string& version) const =0;
	virtual am::am_Error_e init(am::CAmSocketHandler *socketHandler, IRaHotplugReceive* hotplugreceiveinterface) =0;
	virtual void setHotplugReady(const uint16_t handle) =0;
	virtual am::am_Error_e asyncSetSinkVolume(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time) =0;
	virtual am::am_Error_e asyncSetSourceVolume(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time) =0;
	virtual am::am_Error_e asyncSetSourceState(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SourceState_e state) =0;
	virtual am::am_Error_e asyncSetSinkSoundProperties(const uint16_t handle, const am::am_sinkID_t sinkID, const std::vector<am::am_SoundProperty_s>& listSoundProperties) =0;
	virtual am::am_Error_e asyncSetSinkSoundProperty(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_SoundProperty_s& soundProperty) =0;
	virtual am::am_Error_e asyncSetSourceSoundProperties(const uint16_t handle, const am::am_sourceID_t sourceID, const std::vector<am::am_SoundProperty_s>& listSoundProperties) =0;
	virtual am::am_Error_e asyncSetSourceSoundProperty(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SoundProperty_s& soundProperty) =0;
	virtual void ackRegisterSink(const std::string sinkName, const am::am_sinkID_t sinkID, const am::am_Error_e status) =0;
	virtual void ackDeregisterSink(const am::am_sinkID_t sinkID, const am::am_Error_e status) =0;
	virtual void ackRegisterSource(const std::string sourceName, const am::am_sourceID_t sourceID, const am::am_Error_e status) =0;
	virtual void ackDeregisterSource(const am::am_sourceID_t sourceID, const am::am_Error_e status) =0;
	virtual void ackUpdateSource(const am::am_sourceID_t sourceID, const am::am_Error_e status) =0;
	virtual void ackUpdateSink(const am::am_sinkID_t sinkID, const am::am_Error_e status) =0;
};
#endif //__I_RA_HOTPLUG_SEND_H__
