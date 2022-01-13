/************************************************************************
* @file: CRaAMRoutingClient.h
*
* @version: 1.1
*
* CRaAMRoutingClient class implements the callbacks of IAmRoutingClient interface.
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
#ifndef __C_RA_AM_ROUTING_CLIENT_H__
#define __C_RA_AM_ROUTING_CLIENT_H__

#include "IAmRoutingClient.h"
#include "CRaConfigManager.h"
#include "CRaRequestManager.h"
#include "CContextManager.h"

class CRaAMRoutingClient : public IAmRoutingClient
{
public:
	CRaAMRoutingClient(CRaConfigManager *configManager, CRaRequestManager *requestManager, CContextManager *contextManager, std::string interface, CAmSocketHandler* socketHandler);
	virtual ~CRaAMRoutingClient();
	void setRoutingReady(const uint16_t handle);
	void setRoutingRundown(const uint16_t handle);
	am_Error_e asyncAbort(const am_Handle_s handle);
	am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_sourceID_t sourceID, const am_sinkID_t sinkID, const am_CustomConnectionFormat_t connectionFormat);
	am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID);
	am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time);
	am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time);
	am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SourceState_e state);
	am_Error_e asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID, const std::vector<am_SoundProperty_s>& listSoundProperties);
	am_Error_e asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID, const am_SoundProperty_s& soundProperty);
	am_Error_e asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID, const std::vector<am_SoundProperty_s>& listSoundProperties);
	am_Error_e asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SoundProperty_s& soundProperty);
	am_Error_e asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID, const am_HotSink_e hotSink, const am_CustomRampType_t rampType, const am_time_t time);
	am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState);
    am_Error_e asyncSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listVolumes);
    am_Error_e asyncSetSinkNotificationConfiguration(
            const am_Handle_s handle, const am_sinkID_t sinkID,
            const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e asyncSetSourceNotificationConfiguration(
            const am_Handle_s handle, const am_sourceID_t sourceID,
            const am_NotificationConfiguration_s& notificationConfiguration);
private:
	/* we do not want to instantiate object with default constructor */
	CRaAMRoutingClient();
private:
	CRaConfigManager 	*mConfigManager;
	CRaRequestManager	*mRequestManager;
	CContextManager		*mContextManager;
};

#endif //__C_RA_AM_ROUTING_CLIENT_H__
