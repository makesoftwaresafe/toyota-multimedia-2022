#include "audiomanagertypes.h"
#include "CHotplugClient.h"
#include <string.h>
#include <iostream>

CHotplugClient::CHotplugClient() : IRaHotplugClient()
{
}

CHotplugClient::~CHotplugClient()
{
}

void CHotplugClient::setHotplugReady(const uint16_t handle)
{
	std::cout << __func__ << " handle: " << handle << std::endl;
}

am::am_Error_e CHotplugClient::asyncSetSinkVolume(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time)
{
	std::cout << __func__ << " handle: " << handle << " sinkID: " << sinkID << " volume: " << volume << " ramp: " << ramp << " time: " << time << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSourceVolume(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_volume_t volume, const am::am_CustomRampType_t ramp, const am::am_time_t time)
{
	std::cout << __func__ << " handle: " << handle << " sourceID: " << sourceID << " volume: " << volume << " ramp: " << ramp << " time: " << time << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSourceState(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SourceState_e state)
{
	std::cout << __func__ << " handle: " << handle << " sourceID: " << sourceID << " state: " << state << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSinkSoundProperties(const uint16_t handle, const am::am_sinkID_t sinkID, const std::vector<am::am_SoundProperty_s>& listSoundProperties)
{
	(void) listSoundProperties;
	std::cout << __func__ << " handle: " << handle << " sinkID: " << sinkID << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSinkSoundProperty(const uint16_t handle, const am::am_sinkID_t sinkID, const am::am_SoundProperty_s& soundProperty)
{
	(void) soundProperty;
	std::cout << __func__ << " handle: " << handle << " sinkID: " << sinkID << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSourceSoundProperties(const uint16_t handle, const am::am_sourceID_t sourceID, const std::vector<am::am_SoundProperty_s>& listSoundProperties)
{
	(void) listSoundProperties;
	std::cout << __func__ << " handle: " << handle << " sourceID: " << sourceID << std::endl;
	return am::E_OK;
}

am::am_Error_e CHotplugClient::asyncSetSourceSoundProperty(const uint16_t handle, const am::am_sourceID_t sourceID, const am::am_SoundProperty_s& soundProperty)
{
	(void) soundProperty;
	std::cout << __func__ << " handle: " << handle << " sourceID: " << sourceID << std::endl;
	return am::E_OK;
}

void CHotplugClient::ackRegisterSink(const std::string sinkName, const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	std::cout << __func__ << " sinkName: " << sinkName << " sinkID: " << sinkID << " status: "<< status << std::endl;
}

void CHotplugClient::ackDeregisterSink(const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	std::cout << __func__ << " sinkID: " << sinkID << " status: "<< status << std::endl;
}

void CHotplugClient::ackRegisterSource(const std::string sourceName, const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	std::cout << __func__ << " sourceName: " << sourceName << " sourceID: " << sourceID << " status: "<< status << std::endl;
}

void CHotplugClient::ackDeregisterSource(const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	std::cout << __func__ << " sourceID: " << sourceID << " status: "<< status << std::endl;
}

void CHotplugClient::ackUpdateSource(const am::am_sourceID_t sourceID, const am::am_Error_e status)
{
	std::cout << __func__ << " sourceID: " << sourceID << " status: "<< status << std::endl;
}

void CHotplugClient::ackUpdateSink(const am::am_sinkID_t sinkID, const am::am_Error_e status)
{
	std::cout << __func__ << " sinkID: " << sinkID << " status: "<< status << std::endl;
}
