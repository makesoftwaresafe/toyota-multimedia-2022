/************************************************************************
* @file: CAppHotplugReceiver.h
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
#ifndef __I_RA_HOTPLUG_CLIENT__
#define __I_RA_HOTPLUG_CLIENT__
#include <pthread.h>
#include "audiomanagertypes.h"
#include "CAmSocketHandler.h"
#include "CAmSerializer.h"
#include "IRaHotplugSend.h"

#define RA_HOTPLUG_CLIENT_INTERFACE_VERSION "v.1.0"

class IRaHotplugReceive;
class CAppHoptplugReceiver;

class IRaHotplugClient : public IRaHotplugSend
{
public:
	IRaHotplugClient(am::CAmSocketHandler* socketHandler=NULL);
	virtual ~IRaHotplugClient();

	// messages from App -> RA
	void confirmHotplugReady(const uint16_t handle, const am::am_Error_e error);
	void asyncRegisterSink(const am::am_Sink_s& sinkData);
	void asyncDeregisterSink(const am::am_sinkID_t sinkID);
	void asyncRegisterSource(const am::am_Source_s& sourceData);
	void asyncDeregisterSource(const am::am_sourceID_t sourceID);
	void ackSetSinkVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error);
	void ackSetSourceVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error);
	void ackSetSourceState(const uint16_t handle, const am::am_Error_e error);
	void ackSetSinkSoundProperties(const uint16_t handle, const am::am_Error_e error);
	void ackSetSinkSoundProperty(const uint16_t handle, const am::am_Error_e error);
	void ackSetSourceSoundProperties(const uint16_t handle, const am::am_Error_e error);
	void ackSetSourceSoundProperty(const uint16_t handle, const am::am_Error_e error);
	void hookInterruptStatusChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState);
	void hookSinkAvailablityStatusChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability);
	void hookSourceAvailablityStatusChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability);
	void hookSinkNotificationDataChange(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload);
	void hookSourceNotificationDataChange(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload);
	void asyncUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties);
	void asyncUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties);

	// callbacks from RA -> App
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
private:
	void getInterfaceVersion(std::string& version) const;
	am::am_Error_e init(am::CAmSocketHandler *socketHandler, IRaHotplugReceive* hotplugreceiveinterface);

private:
	CAppHoptplugReceiver *mpHotplugReceiver;
	am::CAmSocketHandler* mpSocketHandler;
	pthread_t*  mpEventLoopThread;
	am::CAmSerializer *mpSerializer;
};

#endif //__I_RA_HOTPLUG_CLIENT__
