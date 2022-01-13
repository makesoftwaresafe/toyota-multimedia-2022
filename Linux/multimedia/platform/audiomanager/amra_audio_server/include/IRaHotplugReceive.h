/************************************************************************
* @file: IRaHotplugReceive.h
*
* @version: 1.1
*
* IRaHotplugReceive class is an interface for the communication from
* Application (Hotplug) to Routing Adapter
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
#ifndef __I_RA_HOTPLUG_RECEIVE__
#define __I_RA_HOTPLUG_RECEIVE__
#include "audiomanagertypes.h"

class IRaHotplugReceive
{
public:
	virtual ~IRaHotplugReceive(){}
	virtual void getInterfaceVersion(std::string& version) const =0;
	virtual void confirmHotplugReady(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void asyncRegisterSink(const am::am_Sink_s& sinkData) =0;
	virtual void asyncDeregisterSink(const am::am_sinkID_t sinkID) =0;
	virtual void asyncRegisterSource(const am::am_Source_s& sourceData) =0;
	virtual void asyncDeregisterSource(const am::am_sourceID_t sourceID) =0;
	virtual void ackSetSinkVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error) =0;
	virtual void ackSetSourceVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error) =0;
	virtual void ackSetSourceState(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void ackSetSinkSoundProperties(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void ackSetSinkSoundProperty(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void ackSetSourceSoundProperties(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void ackSetSourceSoundProperty(const uint16_t handle, const am::am_Error_e error) =0;
	virtual void hookInterruptStatusChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState) =0;
	virtual void hookSinkAvailablityStatusChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability) =0;
	virtual void hookSourceAvailablityStatusChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability) =0;
	virtual void hookSinkNotificationDataChange(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload) =0;
	virtual void hookSourceNotificationDataChange(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload) =0;
	virtual void asyncUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties) =0;
	virtual void asyncUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties) =0;
};
#endif //__I_RA_HOTPLUG_RECEIVE__
