/************************************************************************
* @file: CRaHotplugReceiver.cpp
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
#include "CRaHotplugReceiver.h"
#include <dlt/dlt.h>
#include <Log.h>

DLT_IMPORT_CONTEXT(raContext)

CRaHotplugReceiver::CRaHotplugReceiver(CRaAMRoutingClient *routingClient, CRaConfigManager *configMnanager, CRaRequestManager *requestManager, CContextManager *contextManager) :
	mRoutingClient(routingClient),
	mConfigManager(configMnanager),
	mRequestManager(requestManager),
	mContextManager(contextManager)
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

CRaHotplugReceiver::~CRaHotplugReceiver()
{
	LOG_FN_ENTRY(raContext);
	if (mRoutingClient != NULL)
	{
		mRoutingClient = NULL;
	}
	if (mConfigManager != NULL)
	{
		mConfigManager = NULL;
	}
	if (mRequestManager != NULL)
	{
		mRequestManager = NULL;
	}
	if (mContextManager != NULL)
	{
		mContextManager = NULL;
	}
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::getInterfaceVersion(std::string& version) const
{
	LOG_FN_ENTRY(raContext, DLT_STRING("version: "), DLT_STRING(version.c_str()));
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::confirmHotplugReady(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::asyncRegisterSink(const am::am_Sink_s& sinkData)
{
	LOG_FN_ENTRY(raContext);
	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return;
	}

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}


	request->handle.handle=0;
	request->handle.handleType = H_UNKNOWN;
	request->type = eREQUEST_TYPE_ADD_HOTPLUG_SINK;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = 0;
	request->as_sink_id = 0;
	request->data.sink = new am::am_Sink_s(sinkData);

	// sink belongs to routing adapter's domain
	request->data.sink->domainID = mContextManager->GetDomainID();

	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return;
}

void CRaHotplugReceiver::asyncDeregisterSink(const am::am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return;
	}

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}

	request->handle.handle=0;
	request->handle.handleType = H_UNKNOWN;
	request->type = eREQUEST_TYPE_REMOVE_HOTPLUG_SINK;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = 0;
	request->as_sink_id = 0;
	request->data.sinkID = sinkID;
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return;

}

void CRaHotplugReceiver::asyncRegisterSource(const am::am_Source_s& sourceData)
{
	LOG_FN_ENTRY(raContext);
	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return;
	}

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}

	request->handle.handle=0;
	request->handle.handleType = H_UNKNOWN;
	request->type = eREQUEST_TYPE_ADD_HOTPLUG_SOURCE;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = 0;
	request->as_sink_id = 0;
	request->data.source = new am::am_Source_s(sourceData);

	// source belongs to routing adapter's domain
	request->data.source->domainID = mContextManager->GetDomainID();

	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return;
}

void CRaHotplugReceiver::asyncDeregisterSource(const am::am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext);
	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		return;
	}

	if (NULL == mRequestManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mRequestManager == NULL"));
		return;
	}

	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		return;
	}

	request->handle.handle=0;
	request->handle.handleType = H_UNKNOWN;
	request->type = eREQUEST_TYPE_REMOVE_HOTPLUG_SOURCE;
	request->state = eREQUEST_STATE_NEW;
	request->am_sink_id = 0;
	request->as_sink_id = 0;
	request->data.sourceID = sourceID;
	mRequestManager->EnqueueRequest(request);

	LOG_FN_EXIT(raContext);
	return;

}

void CRaHotplugReceiver::ackSetSinkVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" volume: "), DLT_INT(volume), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSourceVolumeChange(const uint16_t handle, const am::am_volume_t volume, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" volume: "), DLT_INT(volume), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSourceState(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	Request         *request = NULL;
	request = mRequestManager->PeekRequestQueue();

	if (request->handle.handle == handle
		&& request->type == eREQUEST_TYPE_FORWARD_HOTPLUG_SOURCE_STATE)
	{
		mRoutingClient->ackSetSourceState(request->handle, error);
		mRequestManager->DequeueRequest();
		delete request;
	}
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSinkSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSinkSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSourceSoundProperties(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::ackSetSourceSoundProperty(const uint16_t handle, const am::am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_INT(handle), DLT_STRING(" error: "), DLT_INT(error));
	LOG_ERROR(raContext, DLT_STRING("This interface is not used yet !!!"));
	// This interface is not used.
	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::hookInterruptStatusChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" interruptState: "), DLT_INT(interruptState));
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
	}
	else
	{
		request->handle.handle=0;
		request->handle.handleType = H_UNKNOWN;
		request->type = eREQUEST_TYPE_HOTPLUG_SOURCE_INTERRUPT_STATUS_CHANGE;
		request->state = eREQUEST_STATE_NEW;
		request->data.sourceIntStateData.sourceID = sourceID;
		request->data.sourceIntStateData.sourceInterruptState = interruptState;
		mRequestManager->EnqueueRequest(request);

		LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SOURCE_INTERRUPT_STATUS_CHANGE"));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::hookSinkAvailablityStatusChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" availability: "), DLT_INT(availability.availability), DLT_STRING(" availability reason: "), DLT_INT(availability.availabilityReason));
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
	}
	else
	{
		request->handle.handle=0;
		request->handle.handleType = H_UNKNOWN;
		request->type = eREQUEST_TYPE_HOTPLUG_SINK_AVAILABILITY_CHANGE;
		request->state = eREQUEST_STATE_NEW;
		request->data.sinkAvailabilityData.sinkID = sinkID;
		request->data.sinkAvailabilityData.availability = availability;
		mRequestManager->EnqueueRequest(request);

		LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SINK_AVAILABILITY_CHANGE"));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::hookSourceAvailablityStatusChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" availability: "), DLT_INT(availability.availability), DLT_STRING(" availability reason: "), DLT_INT(availability.availabilityReason));
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
	}
	else
	{
		request->handle.handle=0;
		request->handle.handleType = H_UNKNOWN;
		request->type = eREQUEST_TYPE_HOTPLUG_SOURCE_AVAILABILITY_CHANGE;
		request->state = eREQUEST_STATE_NEW;
		request->data.sourceAvailabilityData.sourceID = sourceID;
		request->data.sourceAvailabilityData.availability = availability;
		mRequestManager->EnqueueRequest(request);

		LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SOURCE_AVAILABILITY_CHANGE"));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::hookSinkNotificationDataChange(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
	}
	else
	{
		request->handle.handle=0;
		request->handle.handleType = H_UNKNOWN;
		request->type = eREQUEST_TYPE_HOTPLUG_SINK_NOTIF_DATA_CHANGE;
		request->state = eREQUEST_STATE_NEW;
		request->data.sinkNotificationData.sinkID = sinkID;
		request->data.sinkNotificationData.notifPayload = payload;
		mRequestManager->EnqueueRequest(request);

		LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SINK_NOTIF_DATA_CHANGE"));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::hookSourceNotificationDataChange(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	Request	*request = new Request;

	if (NULL == request)
	{
		LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
	}
	else
	{
		request->handle.handle=0;
		request->handle.handleType = H_UNKNOWN;
		request->type = eREQUEST_TYPE_HOTPLUG_SOURCE_NOTIF_DATA_CHANGE;
		request->state = eREQUEST_STATE_NEW;
		request->data.sourceNotifationData.sourceID = sourceID;
		request->data.sourceNotifationData.notifPayload = payload;
		mRequestManager->EnqueueRequest(request);

		LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SOURCE_NOTIF_DATA_CHANGE"));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::asyncUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	am::am_Error_e status = am::E_OK;
	AlsaParams_s alsaParamsInContext, alsaParamsReceived;

	// obtain the ALSA parameters in the context
	mContextManager->GetSourceALSAParams(sourceID, alsaParamsInContext);

	// obtain the received ALSA parameters
	status = getAlsaParamsFromSoundProperties(listSoundProperties, alsaParamsReceived);
	if (status != am::E_OK)
	{
		LOG_ERROR(raContext, DLT_STRING(" GetSourceALSAParams failed - incomplete ALSA parameters !!! "));
	}

	// if there is a change in number of channels
	// or sample format
	// or sample rate then re-register the ALSA source
	// however we have considered only sample rate currently
	//if (alsaParamsReceived.nrChannels != alsaParamsInContext.nrChannels
	// || alsaParamsReceived.sampleFormat != alsaParamsInContext.sampleFormat
	// || alsaParamsReceived.sampleRate != alsaParamsInContext.sampleRate)
	if (alsaParamsReceived.sampleRate != alsaParamsInContext.sampleRate)
	{
		LOG_DEBUG(raContext, DLT_STRING("CRaHotplugReceiver::updateSource There is a change in sample rate"));
		Request	*request = new Request;

		if (NULL == request)
		{
			LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		}
		else
		{
			// update the ALSA parameter in the context data for latter use
			alsaParamsInContext.sampleRate = alsaParamsReceived.sampleRate;
			mContextManager->SetSourceALSAParams(sourceID, alsaParamsInContext);

			request->handle.handle=0;
			request->handle.handleType = H_UNKNOWN;
			request->type = eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS;
			request->state = eREQUEST_STATE_NEW;
			request->am_sink_id = 0;
			request->as_sink_id = 0;
			request->data.sourceUpdateData = new UpdateSourceData();
			request->data.sourceUpdateData->sourceID = sourceID;
			request->data.sourceUpdateData->sourceClassID = sourceClassID;
			request->data.sourceUpdateData->listSoundProperties.clear();
			request->data.sourceUpdateData->listSoundProperties = listSoundProperties;
			request->data.sourceUpdateData->listConnectionFormats.clear();
			request->data.sourceUpdateData->listConnectionFormats = listConnectionFormats;
			request->data.sourceUpdateData->listMainSoundProperties.clear();
			request->data.sourceUpdateData->listMainSoundProperties = listMainSoundProperties;

			mRequestManager->EnqueueRequest(request);

			LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS"));
		}

	}
	else
	{
		Request	*request = new Request;

		if (NULL == request)
		{
			LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		}
		else
		{
			request->handle.handle=0;
			request->handle.handleType = H_UNKNOWN;
			request->type = eREQUEST_TYPE_HOTPLUG_SOURCE_UPDATE;
			request->state = eREQUEST_STATE_NEW;
			request->am_sink_id = 0;
			request->as_sink_id = 0;
			request->data.sourceUpdateData = new UpdateSourceData();
			request->data.sourceUpdateData->sourceID = sourceID;
			request->data.sourceUpdateData->sourceClassID = sourceClassID;
			request->data.sourceUpdateData->listSoundProperties.clear();
			request->data.sourceUpdateData->listSoundProperties = listSoundProperties;
			request->data.sourceUpdateData->listConnectionFormats.clear();
			request->data.sourceUpdateData->listConnectionFormats = listConnectionFormats;
			request->data.sourceUpdateData->listMainSoundProperties.clear();
			request->data.sourceUpdateData->listMainSoundProperties = listMainSoundProperties;

			mRequestManager->EnqueueRequest(request);

			LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SOURCE_UPDATE"));
		}

		LOG_DEBUG(raContext, DLT_STRING(" There is no change in sample rate, hence the update is forwarded to AM "));
	}

	LOG_FN_EXIT(raContext);
}

void CRaHotplugReceiver::asyncUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID, const std::vector<am::am_SoundProperty_s>& listSoundProperties, const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats, const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	am::am_Error_e status = am::E_OK;
	AlsaParams_s alsaParamsInContext, alsaParamsReceived;

	// obtain the ALSA parameters in the context
	mContextManager->GetSinkALSAParams(sinkID, alsaParamsInContext);

	// obtain the received ALSA parameters
	status = getAlsaParamsFromSoundProperties(listSoundProperties, alsaParamsReceived);
	if (status != am::E_OK)
	{
		LOG_ERROR(raContext, DLT_STRING(" GetSourceALSAParams failed - incomplete ALSA parameters"));
	}

	// if there is a change in number of channels
	// or sample format
	// or sample rate then re-register the ALSA source
	// however we have considered only sample rate currently
	//if (alsaParamsReceived.nrChannels != alsaParamsInContext.nrChannels
	// || alsaParamsReceived.sampleFormat != alsaParamsInContext.sampleFormat
	// || alsaParamsReceived.sampleRate != alsaParamsInContext.sampleRate)
	if (alsaParamsReceived.sampleRate != alsaParamsInContext.sampleRate)
	{
		Request	*request = new Request;

		if (NULL == request)
		{
			LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		}
		else
		{
			// update the ALSA parameter in the context data for latter use
			alsaParamsInContext.sampleRate = alsaParamsReceived.sampleRate;
			mContextManager->SetSinkALSAParams(sinkID, alsaParamsInContext);

			request->handle.handle=0;
			request->handle.handleType = H_UNKNOWN;
			request->type = eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS;
			request->state = eREQUEST_STATE_NEW;
			request->am_sink_id = 0;
			request->as_sink_id = 0;
			request->data.sinkUpdateData = new UpdateSinkData();
			request->data.sinkUpdateData->sinkID = sinkID;
			request->data.sinkUpdateData->sinkClassID = sinkClassID;
			request->data.sinkUpdateData->listSoundProperties.clear();
			request->data.sinkUpdateData->listSoundProperties = listSoundProperties;
			request->data.sinkUpdateData->listConnectionFormats.clear();
			request->data.sinkUpdateData->listConnectionFormats = listConnectionFormats;
			request->data.sinkUpdateData->listMainSoundProperties.clear();
			request->data.sinkUpdateData->listMainSoundProperties = listMainSoundProperties;

			mRequestManager->EnqueueRequest(request);

			LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS"));
		}
	}
	else
	{
		Request	*request = new Request;

		if (NULL == request)
		{
			LOG_ERROR(raContext, DLT_STRING("failed to allocate request"));
		}
		else
		{
			request->handle.handle=0;
			request->handle.handleType = H_UNKNOWN;
			request->type = eREQUEST_TYPE_HOTPLUG_SINK_UPDATE;
			request->state = eREQUEST_STATE_NEW;
			request->am_sink_id = 0;
			request->as_sink_id = 0;
			request->data.sinkUpdateData = new UpdateSinkData();
			request->data.sinkUpdateData->sinkID = sinkID;
			request->data.sinkUpdateData->sinkClassID = sinkClassID;
			request->data.sinkUpdateData->listSoundProperties.clear();
			request->data.sinkUpdateData->listSoundProperties = listSoundProperties;
			request->data.sinkUpdateData->listConnectionFormats.clear();
			request->data.sinkUpdateData->listConnectionFormats = listConnectionFormats;
			request->data.sinkUpdateData->listMainSoundProperties.clear();
			request->data.sinkUpdateData->listMainSoundProperties = listMainSoundProperties;

			mRequestManager->EnqueueRequest(request);

			LOG_DEBUG(raContext, DLT_STRING(" EnqueueRequest : eREQUEST_TYPE_HOTPLUG_SINK_UPDATE"));
		}

	}

	LOG_FN_EXIT(raContext);
}
