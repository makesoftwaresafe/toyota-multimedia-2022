#ifndef __C_HOTPLUG_CLIENT_H__
#define __C_HOTPLUG_CLIENT_H__

#include "IRaHotplugClient.h"

class CHotplugClient : public IRaHotplugClient
{
public:
	CHotplugClient();
	virtual ~CHotplugClient();
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
};

#endif //__C_HOTPLUG_CLIENT_H__
