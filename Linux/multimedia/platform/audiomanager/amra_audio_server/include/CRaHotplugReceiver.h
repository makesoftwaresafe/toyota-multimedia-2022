/************************************************************************
* @file: CRaHotplugReceiver.h
*
* @version: 1.1
*
* CRaHotplugReceiver class implements the callbacks of IRaHotplugReceive interface
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
#ifndef _C_RA_HOTPLUG_RECEIVER_H_
#define _C_RA_HOTPLUG_RECEIVER_H_
#include "IRaHotplugReceive.h"
#include "CRaConfigManager.h"
#include "CRaRequestManager.h"
#include "CContextManager.h"
#include "CRaAMRoutingClient.h"

class CRaHotplugReceiver : public IRaHotplugReceive
{
public:
	CRaHotplugReceiver(CRaAMRoutingClient *routingClient, CRaConfigManager *configMnanager, CRaRequestManager *requestManager, CContextManager *contextManager);
	virtual ~CRaHotplugReceiver();
	void getInterfaceVersion(std::string& version) const;
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
private:
	CRaAMRoutingClient	*mRoutingClient;
	CRaConfigManager 	*mConfigManager;
	CRaRequestManager	*mRequestManager;
	CContextManager		*mContextManager;
};

#endif // _C_RA_HOTPLUG_RECEIVER_H_
