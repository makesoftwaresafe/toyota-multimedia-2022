/************************************************************************
* @file: CRaAMRoutingClient.cpp
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
#include "CRaAMRoutingClient.h"
#include "CContextManager.h"
#include "projecttypes.h"
#include <dlt/dlt.h>
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext);

CRaAMRoutingClient::CRaAMRoutingClient(CRaConfigManager *configManager, CRaRequestManager *requestManager, CContextManager *contextManager, std::string interface, CAmSocketHandler* socketHandler) :
        IAmRoutingClient(interface, DBUS_BUS_SYSTEM, socketHandler), mConfigManager(configManager), mRequestManager(requestManager), mContextManager(contextManager)



{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

CRaAMRoutingClient::~CRaAMRoutingClient()
{
	LOG_FN_ENTRY(raContext);
	mConfigManager = NULL;
	mRequestManager = NULL;
	mContextManager = NULL;
	LOG_FN_EXIT(raContext);
}

void CRaAMRoutingClient::setRoutingReady(const uint16_t handle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle));

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	am_Error_e error = E_OK;
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}

	request->handle.handleType = H_UNKNOWN;
	request->handle.handle = handle;
	request->type = eREQUEST_TYPE_SET_ROUTING_READY;
	request->state = eREQUEST_STATE_NEW;
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return;
}

void CRaAMRoutingClient::setRoutingRundown(const uint16_t handle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle));

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	am_Error_e error = E_OK;
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}

	request->handle.handleType = H_UNKNOWN;
	request->handle.handle = handle;
	request->type = eREQUEST_TYPE_SET_ROUTING_RUNDOWN;
	request->state = eREQUEST_STATE_NEW;
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return;
}

am_Error_e CRaAMRoutingClient::asyncAbort(const am_Handle_s handle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle "), DLT_INT(handle.handle));
	am_Error_e error = E_OK;
	mRequestManager->RemoveRequest(handle);
	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_sourceID_t sourceID, const am_sinkID_t sinkID, const am_CustomConnectionFormat_t connectionFormat)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", connectionID: "), DLT_INT(connectionID),
		DLT_STRING(", sourceID: "), DLT_INT(sourceID),
		DLT_STRING(", sinkID: " ), DLT_INT(sinkID),
		DLT_STRING(", connection format: "), DLT_INT(connectionFormat));

	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return E_UNKNOWN;
	}

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return E_UNKNOWN;
	}

	// check if the requested source sink pair is already connected
	if (mContextManager->connectionExists(sourceID, sinkID))
	{
		return E_ALREADY_EXISTS;
	}

	// check if the connection ID is already in use
	if (mContextManager->connectionExists(connectionID))
	{
		return E_ALREADY_EXISTS;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return E_UNKNOWN;
	}

	request->handle = handle;
	request->type = eREQUEST_TYPE_CONNECT;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = sinkID;
	request->as_sink_id = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);
	request->data.connection.am_connection_id = connectionID;
	request->data.connection.am_source_id = sourceID;
	request->data.connection.as_source_id = mContextManager->GetAsSourceIDFromAmSourceID(sourceID);
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return E_OK;
}

am_Error_e CRaAMRoutingClient::asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", connectionID: "), DLT_INT(connectionID));
	
	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return E_UNKNOWN;
	}

	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return E_UNKNOWN;
	}

	// check if the connection ID exists
	if (!mContextManager->connectionExists(connectionID))
	{
		return E_NON_EXISTENT;
	}

	am_sinkID_t sinkID;
	am_sourceID_t sourceID;

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return E_UNKNOWN;
	}

	mContextManager->GetConnectionData(connectionID, sourceID, sinkID);

	request->handle = handle;
	request->type = eREQUEST_TYPE_DISCONNECT;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = sinkID;
	request->as_sink_id = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);
	request->data.connection.am_connection_id = connectionID;
	request->data.connection.am_source_id = sourceID;
	request->data.connection.as_source_id = mContextManager->GetAsSourceIDFromAmSourceID(sourceID);
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return E_OK;
}

am_Error_e CRaAMRoutingClient::asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t rampTime)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sinkID: "), DLT_INT(sinkID),
		DLT_STRING(", volume: "), DLT_INT(volume),
		DLT_STRING(", ramp: "), DLT_INT(ramp),
		DLT_STRING(", time: "), DLT_INT(rampTime));

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return E_UNKNOWN;
	}

	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return E_UNKNOWN;
	}

	// check if sink exists
	if (!mContextManager->sinkExists(sinkID))
	{
		return E_NON_EXISTENT;
	}

	Request	*request = new Request;
	
	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return E_UNKNOWN;
	}

	request->handle = handle;
	request->type = eREQUEST_TYPE_VOLUME;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = sinkID;
	request->as_sink_id = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);
	request->data.volume.am_volume = volume;
	request->data.volume.am_ramp = ramp;
	request->data.volume.am_time = rampTime;
	request->data.volume.as_volume = mConfigManager->GetAsVolumeFromAmVolume(volume);
	request->data.volume.as_ramp = mConfigManager->GetAsRampShapeFromAmRampShape(ramp);
	request->data.volume.as_time = rampTime;

	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return E_OK;
}

am_Error_e CRaAMRoutingClient::asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t rampTime)
{
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sourceID: "), DLT_INT(sourceID),
		DLT_STRING(", volume: "), DLT_INT(volume),
		DLT_STRING(", ramp: "), DLT_INT(ramp),
		DLT_STRING(", time: "), DLT_INT(rampTime));

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SourceState_e state)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sourceID: "), DLT_INT(sourceID),
		DLT_STRING(", state: "), DLT_INT(state));

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return E_UNKNOWN;
	}

	// check if source exists
	if (!mContextManager->sourceExists(sourceID))
	{
		return E_NON_EXISTENT;
	}

	Request	*request = new Request;
	
	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return E_UNKNOWN;
	}

	request->handle = handle;
	if (mContextManager->IsHotplugSource(sourceID))
	{
		request->type = eREQUEST_TYPE_FORWARD_HOTPLUG_SOURCE_STATE;
	}
	else
	{
		request->type = eREQUEST_TYPE_SET_SOURCE_STATE;
	}
	request->state = eREQUEST_STATE_NEW;
	request->data.sourcestate.sourceID = sourceID;
	request->data.sourcestate.sourceState = state;
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return E_OK;
}

am_Error_e CRaAMRoutingClient::asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID, const std::vector<am_SoundProperty_s>& listSoundProperties)
{
	(void) listSoundProperties;
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sinkID:"), DLT_INT(sinkID));

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID, const am_SoundProperty_s& soundProperty)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sinkID: "), DLT_INT(sinkID),
		DLT_STRING(", soundProperty.type: "), DLT_INT(soundProperty.type),
		DLT_STRING(", soundProperty.value: "), DLT_INT(soundProperty.value));

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return E_UNKNOWN;
	}

	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return E_UNKNOWN;
	}

	// check if sink exists
	if (!mContextManager->sinkExists(sinkID))
	{
		return E_NON_EXISTENT;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return E_UNKNOWN;
	}

	request->handle = handle;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = sinkID;
	request->as_sink_id = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);

	switch(soundProperty.type) {
		case SP_GENIVI_MUTESTATE:
			request->type = eREQUEST_TYPE_MUTE_STATE;
			request->data.mutestate.am_mute_state = (am_MuteState_e) soundProperty.value;
			request->data.mutestate.am_ramp = RAMP_GENIVI_LINEAR; //assume linear ramp 
			request->data.mutestate.am_time = 500; //assume 500 ms ramp time
			request->data.mutestate.as_mute_state 
			= mConfigManager->GetAsMuteStateFromAmMuteState(soundProperty.value);
			request->data.mutestate.as_ramp = mConfigManager->GetAsRampShapeFromAmRampShape(RAMP_GENIVI_LINEAR);
			request->data.mutestate.as_time = 500;
			break;
		case SP_GENIVI_TREBLE:
			request->type = eREQUEST_TYPE_EQUALIZATION;
			// TODO: need to check how band is decided.
			request->data.equalizer.am_band = 0;
			request->data.equalizer.am_gain = soundProperty.value;
			request->data.equalizer.as_band = 0;
			request->data.equalizer.as_gain = mConfigManager->GetAsEqualizationFromAmEqualization(soundProperty.value);
			break;
		case SP_GENIVI_BASS:
			request->type = eREQUEST_TYPE_EQUALIZATION;
			// TODO: need to check how band is decided.
			request->data.equalizer.am_band = 0;
			request->data.equalizer.am_gain = soundProperty.value;
			request->data.equalizer.as_band = 0;
			request->data.equalizer.as_gain = mConfigManager->GetAsEqualizationFromAmEqualization(soundProperty.value);
			break;
		case SP_GENIVI_BALANCE:
			request->type = eREQUEST_TYPE_BALANCE;
			request->data.balance.am_balance = soundProperty.value;
			request->data.balance.as_balance = mConfigManager->GetAsBalanceFromAmBalance(soundProperty.value);
			break;
		case SP_GENIVI_FADE:
			request->type = eREQUEST_TYPE_FADER;
			request->data.fader.am_fader = soundProperty.value;
			request->data.fader.as_fader = mConfigManager->GetAsFaderFromAmFader(soundProperty.value);
			break;
		case SP_GENIVI_LOUDNESS:
			request->type = eREQUEST_TYPE_LOUDNESS;
			request->data.loudness.am_loudness = soundProperty.value;
			request->data.loudness.as_loudness = mConfigManager->GetAsLoudnessFromAmLoudness(soundProperty.value);
			break;
		case SP_GENIVI_SCV:
			request->type = eREQUEST_TYPE_SCV;
			request->data.scv.am_scv = soundProperty.value;
			request->data.scv.as_scv = mConfigManager->GetAsSCVFromAmSCV(soundProperty.value);
			break;
		case SP_GENIVI_INPUT_GAIN_OFFSET:
			request->type = eREQUEST_TYPE_INPUTGAINOFFSET;
			request->data.inputgain.am_input_gain_offset = soundProperty.value;
			request->data.inputgain.as_input_gain_offset = mConfigManager->GetAsInputGainOffsetFromAmInputGainOffset(soundProperty.value);
			break;
		default:
			break;
	}

	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return E_OK;
}

am_Error_e CRaAMRoutingClient::asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID, const std::vector<am_SoundProperty_s>& listSoundProperties)
{
	(void) listSoundProperties;
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sourceID: "), DLT_INT(sourceID));

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SoundProperty_s& soundProperty)
{
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING("[CRaAMRoutingClient::asyncSetSourceSoundProperty] -- handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", sourceID: "), DLT_INT(sourceID),
		DLT_STRING(", soundProperty.type: "), DLT_INT(soundProperty.type),
		DLT_STRING(", soundProperty.value: "), DLT_INT(soundProperty.value));
	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID, const am_HotSink_e hotSink, const am_CustomRampType_t rampType, const am_time_t rampTime)
{
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING("[CRaAMRoutingClient::asyncCrossFade] -- handle: "), DLT_UINT(handle.handle),
		DLT_STRING(", crossfaderID: "), DLT_INT(crossfaderID),
		DLT_STRING(", hotSink: "), DLT_INT(hotSink),
		DLT_STRING(", rampType: "), DLT_INT(rampType),
		DLT_STRING(", time: "), DLT_INT(rampTime));

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am_Error_e CRaAMRoutingClient::setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState)
{
	am_Error_e error = E_OK;
	LOG_FN_ENTRY(raContext, DLT_STRING(" domainID: "), DLT_INT(domainID),
		DLT_STRING(", domainState: "), DLT_INT(domainState));
	// The KP audio daemon is a controlled domain
	// setting anything else should return error
	if (domainState != DS_CONTROLLED)
	{
		error = E_NOT_POSSIBLE;
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}
am_Error_e CRaAMRoutingClient::asyncSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listVolumes)
{
	(void) listVolumes;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle.handle));
	LOG_FN_EXIT(raContext);
    return E_OK;
}
am_Error_e CRaAMRoutingClient::asyncSetSinkNotificationConfiguration(
        const am_Handle_s handle, const am_sinkID_t sinkID,
        const am_NotificationConfiguration_s& notificationConfiguration)
{
	(void) notificationConfiguration;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle.handle), DLT_STRING(" sinkID :"), DLT_INT(sinkID));
	LOG_FN_EXIT(raContext);
    return E_OK;
}
am_Error_e CRaAMRoutingClient::asyncSetSourceNotificationConfiguration(
        const am_Handle_s handle, const am_sourceID_t sourceID,
        const am_NotificationConfiguration_s& notificationConfiguration)
{
	(void) handle;
	(void) sourceID;
	(void) notificationConfiguration;
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle.handle), DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	LOG_FN_EXIT(raContext);
    return E_OK;
}
