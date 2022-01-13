/************************************************************************
* @file: CRaRequestManager.cpp
*
* @version: 1.1
*
* CRaRequestManager class serialize the requests from the Audio Manager
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
#include "CRaRequestManager.h"
#include "CRaAMRoutingClient.h"
#include "IasAudioSetupClient.h"
#include "audio/rpcontroller/idl/IasAudioSetup.hpp"
#include "projecttypes.h"
#include "Log.h"
#include <iostream>
#include <unistd.h>

static const uint32_t AlsaParamCardReceived = (1U << 0);
static const uint32_t AlsaParamDeviceReceived = (1U << 1);
static const uint32_t AlsaParamNrChannelsReceived = (1U << 2);
static const uint32_t AlsaAsIdReceived = (1U << 3);
static const uint32_t AlsaParamIsSyncToSysClkReceived = (1U << 4);
static const uint32_t AlsaParamSampleFormatReceived = (1U << 5);
static const uint32_t AlsaParamSampleRateReceived = (1U << 6);
static const uint32_t AlsaParamAllReceived = AlsaParamCardReceived
	| AlsaParamDeviceReceived
	| AlsaParamNrChannelsReceived
	| AlsaAsIdReceived
	| AlsaParamIsSyncToSysClkReceived
	| AlsaParamSampleFormatReceived
	| AlsaParamSampleRateReceived;

using namespace std;

DLT_IMPORT_CONTEXT(raContext);

am::am_Error_e getAlsaParamsFromSoundProperties(const std::vector<am_SoundProperty_s> &listSoundProperties, AlsaParams_s &alsaParams)
{
	LOG_FN_ENTRY(raContext);
	uint32_t paramsReceived = 0;
	for (std::vector<am_SoundProperty_s>::const_iterator it = listSoundProperties.begin();
			it != listSoundProperties.end();
			++it)
	{
		switch(it->type)
		{
			case SP_GENIVI_ALSA_CARD_NO:
				alsaParams.alsaCard = it->value;
				paramsReceived = paramsReceived | AlsaParamCardReceived;
				break;
			case SP_GENIVI_ALSA_DEVICE_NO:
				alsaParams.alsaDevice = it->value;
				paramsReceived = paramsReceived | AlsaParamDeviceReceived;
				break;
			case SP_GENIVI_ALSA_NUMBER_OF_CHANNELS:
				alsaParams.nrChannels = it->value;
				paramsReceived = paramsReceived | AlsaParamNrChannelsReceived;
				break;
			case SP_GENIVI_ALSA_AS_ID:
				alsaParams.asId = it->value;
				paramsReceived = paramsReceived | AlsaAsIdReceived;
				break;
			case SP_GENIVI_ALSA_SYNC_TO_SYS_CLK:
				if (it->value == 0)
					alsaParams.isSyncToSysClk = false;
				else
					alsaParams.isSyncToSysClk = true;
				paramsReceived = paramsReceived | AlsaParamIsSyncToSysClkReceived;
				break;
			case SP_GENIVI_ALSA_SAMPLE_FORMAT:
				switch (it->value)
				{
					case SF_S16_LE:
						alsaParams.sampleFormat = IasAudioSetup::eIasS16LE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_S16_BE:
						alsaParams.sampleFormat = IasAudioSetup::eIasS16BE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_S32_LE:
						alsaParams.sampleFormat = IasAudioSetup::eIasS32LE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_S32_BE:
						alsaParams.sampleFormat = IasAudioSetup::eIasS32BE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_FLOAT_LE:
						alsaParams.sampleFormat = IasAudioSetup::eIasFloat32LE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_FLOAT_BE:
						alsaParams.sampleFormat = IasAudioSetup::eIasFloat32BE;
						paramsReceived = paramsReceived | AlsaParamSampleFormatReceived;
						break;
					case SF_S8:
					case SF_U8:
					case SF_U16_LE:
					case SF_U16_BE:
					case SF_S24_LE:
					case SF_S24_BE:
					case SF_U24_LE:
					case SF_U24_BE:
					case SF_U32_LE:
					case SF_U32_BE:
					case SF_FLOAT64_LE:
					case SF_FLOAT64_BE:
					case SF_IEC958_SUBFRAME_LE:
					case SF_IEC958_SUBFRAME_BE:
					case SF_MU_LAW:
					case SF_A_LAW:
					case SF_IMA_ADPCM:
					case SF_MPEG:
					case SF_GSM:
					default:
						alsaParams.sampleFormat = IasAudioSetup::eIasUnknown;
						break;
				}
				break;
			case SP_GENIVI_ALSA_SAMPLE_RATE:
				switch (it->value)
				{
					case SR_5512:
						alsaParams.sampleRate = 5512;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_8000:
						alsaParams.sampleRate = 8000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_11025:
						alsaParams.sampleRate = 11025;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_16000:
						alsaParams.sampleRate = 16000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_22050:
						alsaParams.sampleRate = 22050;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_32000:
						alsaParams.sampleRate = 32000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_44100:
						alsaParams.sampleRate = 44100;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_48000:
						alsaParams.sampleRate = 48000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_64000:
						alsaParams.sampleRate = 64000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_88200:
						alsaParams.sampleRate = 88200;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_96000:
						alsaParams.sampleRate = 96000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_176400:
						alsaParams.sampleRate = 176400;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
					case SR_192000:
						alsaParams.sampleRate = 192000;
						paramsReceived = paramsReceived | AlsaParamSampleRateReceived;
						break;
				}
				break;
				default:
				break;
		}
	}

	if (paramsReceived != AlsaParamAllReceived)
	{
		LOG_ERROR(raContext, DLT_STRING(" incomplete alsa params"));
		LOG_FN_EXIT(raContext);
		return am::E_UNKNOWN;
	}
	LOG_FN_EXIT(raContext);
	return am::E_OK;
}

CRaRequestManager::CRaRequestManager(CRaASClient *asClient, CRaAMRoutingClient *amClient, CRaConfigManager *configManager, CContextManager *contextManager)
	: mAsClient(asClient), mAmClient(amClient), mConfigManager(configManager), mContextManager(contextManager)
{
	LOG_FN_ENTRY(raContext);

	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCond, NULL);
	int ret = 0;
	ret = pthread_create(&mRequestManagerThread, NULL, CRaRequestManager::RequestManagerThreadHandler, this);
	if(ret != 0)
	{
		LOG_ERROR(raContext, DLT_STRING(" request manager thread creation failed !"));
	}

	LOG_FN_EXIT(raContext);
}

CRaRequestManager::~CRaRequestManager()
{
	LOG_FN_ENTRY(raContext);
	pthread_join(mRequestManagerThread, NULL);
	pthread_cond_destroy(&mCond);
	pthread_mutex_destroy(&mLock);
	LOG_FN_EXIT(raContext);
	return;
}

void CRaRequestManager::EnqueueRequest(Request * request)
{
	LOG_FN_ENTRY(raContext);
	pthread_mutex_lock(&mLock);
	mRequestQueue.push_back(request);
	pthread_cond_signal(&mCond);
	pthread_mutex_unlock(&mLock);
	LOG_FN_EXIT(raContext);

	return;
}

void CRaRequestManager::RemoveRequest(const am_Handle_s handle)
{
    LOG_FN_ENTRY(raContext);
    std::list<Request *>::iterator  itrRequestQueue;
    pthread_mutex_lock(&mLock);
    for(itrRequestQueue = mRequestQueue.begin();\
        itrRequestQueue != mRequestQueue.end();\
        ++itrRequestQueue
       )
    {
        if((*itrRequestQueue)->handle.handle == handle.handle)
        {
            mRequestQueue.remove((*itrRequestQueue));
            delete (*itrRequestQueue);
            break;
        }
    }
    pthread_cond_signal(&mCond);
    pthread_mutex_unlock(&mLock);
    LOG_FN_EXIT(raContext);
    return;
}

void CRaRequestManager::SetRequestState(RequestState_e state)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" state: "), DLT_INT(state));

	Request *front = NULL;

	pthread_mutex_lock(&mLock);
	if (mRequestQueue.empty()) {
		pthread_mutex_unlock(&mLock);
		LOG_FN_EXIT(raContext);
		return;
	}

	front = mRequestQueue.front();

	front->state = state;
	pthread_mutex_unlock(&mLock);
	LOG_FN_EXIT(raContext);
	return;
}

void  CRaRequestManager::SetAmClient(CRaAMRoutingClient *amClient)
{
	LOG_FN_ENTRY(raContext);
	mAmClient = amClient;
	LOG_FN_EXIT(raContext);
}

void CRaRequestManager::SetHotplugSender(CRaHotplugSender *hotplugSender)
{
	LOG_FN_ENTRY(raContext);
	mHotplugSender = hotplugSender;
	LOG_FN_EXIT(raContext);
}

void CRaRequestManager::AddDelayedRequest(Request * request)
{
	LOG_FN_ENTRY(raContext);
	mDelayedRequest[request->am_sink_id][request->type] = *request;
	LOG_DEBUG(raContext, DLT_STRING("Added Delayed Request For Sink: "),
	DLT_INT(mDelayedRequest[request->am_sink_id][request->type].am_sink_id),
	DLT_STRING("Request Type: "),
	DLT_INT(mDelayedRequest[request->am_sink_id][request->type].type));
	LOG_FN_EXIT(raContext);
}

void CRaRequestManager::ProcessDelayedRequests(am_sinkID_t sink)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sink));
	std::map<am_sinkID_t, std::map<RequestType_e, Request> >::iterator sink_iter;
	std::map<RequestType_e, Request>::iterator type_iter;

	sink_iter = mDelayedRequest.find(sink);
	if (sink_iter != mDelayedRequest.end())
	{
		LOG_DEBUG(raContext, DLT_STRING("Process Delayed Request For Sink: "), DLT_INT(sink));
		for (type_iter = mDelayedRequest[sink].begin(); type_iter != mDelayedRequest[sink].end(); ++type_iter)
		{
			switch(type_iter->second.type)
			{	
				case eREQUEST_TYPE_VOLUME:
				mAsClient->callSetVolume(type_iter->second.as_sink_id,
							type_iter->second.data.volume.as_volume,
							type_iter->second.data.volume.as_time,
							type_iter->second.data.volume.as_ramp);
				LOG_DEBUG(raContext,
					DLT_STRING("Delayed [callSetVolume] -- as_sink_id: "),
					DLT_INT(type_iter->second.as_sink_id),
					DLT_STRING(", as_volume: "),
					DLT_INT(type_iter->second.data.volume.as_volume),
					DLT_STRING(", as_time: "),
					DLT_INT(type_iter->second.data.volume.as_time),
					DLT_STRING(", as_ramp: "),
					DLT_INT(type_iter->second.data.volume.as_ramp));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_MUTE_STATE:
					mAsClient->callSetMuteState(type_iter->second.as_sink_id,
							type_iter->second.data.mutestate.as_mute_state,
							type_iter->second.data.mutestate.as_time,
							type_iter->second.data.mutestate.as_ramp);
					LOG_DEBUG(raContext, 
						DLT_STRING("Delayed [callSetMuteState] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_mute_state: "),
						DLT_INT(type_iter->second.data.mutestate.as_mute_state),
						DLT_STRING(", as_time: "),
						DLT_INT(type_iter->second.data.mutestate.as_time),
						DLT_STRING(", as_ramp: "),
						DLT_INT(type_iter->second.data.mutestate.as_ramp));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_BALANCE:
					mAsClient->callSetBalance(type_iter->second.as_sink_id,
							type_iter->second.data.balance.as_balance);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetBalance] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_balance: "),
						DLT_INT(type_iter->second.data.balance.as_balance));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_FADER:
					mAsClient->callSetFader(type_iter->second.as_sink_id,
							type_iter->second.data.fader.as_fader);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetFader] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_fader: "),
						DLT_INT(type_iter->second.data.fader.as_fader));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_EQUALIZATION:
					mAsClient->callSetEqualizer(type_iter->second.as_sink_id,
							type_iter->second.data.equalizer.as_band,
							type_iter->second.data.equalizer.as_gain);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetEqualizer] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_band: "),
						DLT_INT(type_iter->second.data.equalizer.as_band),
						DLT_STRING(", as_gain: "), DLT_INT(type_iter->second.data.equalizer.as_gain));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_LOUDNESS:
					mAsClient->callSetLoudness(type_iter->second.as_sink_id,
							type_iter->second.data.loudness.as_loudness);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetLoudness] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_loudness: "),
						DLT_INT(type_iter->second.data.loudness.as_loudness));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_SCV:
					mAsClient->callSetSpeedControlledVolume(type_iter->second.as_sink_id,
							type_iter->second.data.scv.as_scv);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetSpeedControlledVolume] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_scv: "),
						DLT_INT(type_iter->second.data.scv.as_scv));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				case eREQUEST_TYPE_INPUTGAINOFFSET:
					mAsClient->callSetInputGainOffset(type_iter->second.as_sink_id,
							type_iter->second.data.inputgain.as_input_gain_offset);
					LOG_DEBUG(raContext,
						DLT_STRING("Delayed [callSetInputGainOffset] -- as_sink_id: "),
						DLT_INT(type_iter->second.as_sink_id),
						DLT_STRING(", as_input_gain_offset: "),
						DLT_INT(type_iter->second.data.inputgain.as_input_gain_offset));
					mAsClient->IncreamentDelayedRequestCount();
					break;
				default:
					LOG_DEBUG(raContext, DLT_STRING("Unhandled Delayed Request for Sink: "), DLT_INT(sink), DLT_STRING("Request Type: "), DLT_INT(type_iter->second.type));
					break;
			}
		}
	}
	LOG_FN_EXIT(raContext);
}

void CRaRequestManager::RemoveDelayedRequests(am_sinkID_t sink)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sink));
	std::map<am_sinkID_t, std::map<RequestType_e, Request> >::iterator sink_iter;
	std::map<RequestType_e, Request>::iterator type_iter;

	sink_iter = mDelayedRequest.find(sink);
	if (sink_iter != mDelayedRequest.end())
	{
		mDelayedRequest[sink].erase(mDelayedRequest[sink].begin(), mDelayedRequest[sink].end());
		mDelayedRequest.erase(sink_iter);
		LOG_DEBUG(raContext, DLT_STRING("Removed All Delayed Requests for the sink id: "), DLT_INT(sink));
	}
	LOG_FN_EXIT(raContext);
}

RequestState_e CRaRequestManager::GetRequestState()
{
	LOG_FN_ENTRY(raContext);
	Request *request = NULL;
	RequestState_e state;

	if (mRequestQueue.empty()) {
		return eREQUEST_STATE_UNKNOWN;
	}

	request = mRequestQueue.front();
	state = request->state;

	LOG_FN_EXIT(raContext, DLT_STRING(" state: "), DLT_INT(state));
	return state;
}
void CRaRequestManager::SendRequestToAS(void)
{
	LOG_FN_ENTRY(raContext);

	Request *request = NULL;
	am::am_Error_e status = am::E_UNKNOWN;
	
	if (mRequestQueue.empty())
	{
		LOG_DEBUG(raContext, DLT_STRING("Request Queue Empty !"));
		LOG_FN_EXIT(raContext);
		return;
	}

	request = mRequestQueue.front();

	// if the sink is not yet conected then remove the request from processing queue
	// put the request in delayed list, so that it can be processed latter
	// send a dummy ack to AudioManager

	if ((request->type == eREQUEST_TYPE_VOLUME)
		|| (request->type == eREQUEST_TYPE_MUTE_STATE)
		|| (request->type == eREQUEST_TYPE_BALANCE)
		|| (request->type == eREQUEST_TYPE_FADER)
		|| (request->type == eREQUEST_TYPE_EQUALIZATION)
		|| (request->type == eREQUEST_TYPE_LOUDNESS)
		|| (request->type == eREQUEST_TYPE_SCV)
		|| (request->type == eREQUEST_TYPE_INPUTGAINOFFSET))
	{
		if (!mContextManager->IsConnectedSink(request->am_sink_id))
		{
			AddDelayedRequest(request);
			SendDummyAckToAM(request);
			mRequestQueue.pop_front();
			delete request;
			LOG_FN_EXIT(raContext);
			return;
		}
	}
	#define DROP_REQUEST_AS_SOURCE_ID 999
	if ((request->type == eREQUEST_TYPE_CONNECT)||\
		 (request->type == eREQUEST_TYPE_DISCONNECT))
	{
		// just send dummy ack for SRC_AUDIO_OFF, the configuration contains value DROP_REQUEST_AS_SOURCE_ID = 999
		if (mContextManager->GetAsSourceIDFromAmSourceID(request->data.connection.am_source_id) == DROP_REQUEST_AS_SOURCE_ID)
		{
			SendDummyAckToAM(request);
			mRequestQueue.pop_front();
			delete request;
			LOG_FN_EXIT(raContext);
			return;
		}
	}

	switch(request->type) {
		case eREQUEST_TYPE_SET_ROUTING_READY:
			status = registerDomainData(request->handle.handle);
			if (status != am::E_OK)
			{
				LOG_ERROR(raContext, DLT_STRING("registerDomainData failed !!!,  status = "), DLT_INT(status));
			}
			else
			{
				LOG_DEBUG(raContext, DLT_STRING("registerDomainData success,  status = "), DLT_INT(status));
			}
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;	
		case eREQUEST_TYPE_SET_ROUTING_RUNDOWN:
			status = unregisterDomainData(request->handle.handle);
			if (status != am::E_OK)
			{
				LOG_ERROR(raContext, DLT_STRING("unregisterDomainData failed !!!,  status = "), DLT_INT(status));
			}
			else
			{
				LOG_DEBUG(raContext, DLT_STRING("unregisterDomainData success,  status = "), DLT_INT(status));
			}
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;	
		case eREQUEST_TYPE_CONNECT:
		    mAsClient->callRequestConnection(request->data.connection.as_source_id,
					request->as_sink_id);
			LOG_DEBUG(raContext, DLT_STRING("[callRequestConnection] -- as_source_id: "), DLT_INT(request->data.connection.as_source_id),
				DLT_STRING(", as_sink_id:"), DLT_INT(request->as_sink_id));
			break;
		case eREQUEST_TYPE_DISCONNECT:
			mAsClient->callReleaseConnection(request->data.connection.as_source_id,
					request->as_sink_id);
			LOG_DEBUG(raContext, DLT_STRING("[callReleaseConnection] -- as_source_id: "), DLT_INT(request->data.connection.as_source_id),
				DLT_STRING(", as_sink_id:"), DLT_INT(request->as_sink_id));
			break;
		case eREQUEST_TYPE_VOLUME:
			mAsClient->callSetVolume(request->as_sink_id, request->data.volume.as_volume,
						request->data.volume.as_time, request->data.volume.as_ramp);
			LOG_DEBUG(raContext, DLT_STRING("[callSetVolume] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_volume: "), DLT_INT(request->data.volume.as_volume),
				DLT_STRING(", as_time: "), DLT_INT(request->data.volume.as_time),
				DLT_STRING(", as_ramp: "), DLT_INT(request->data.volume.as_ramp));
			break;
		case eREQUEST_TYPE_MUTE_STATE:
			mAsClient->callSetMuteState(request->as_sink_id, request->data.mutestate.as_mute_state,
					request->data.mutestate.as_time, request->data.mutestate.as_ramp);
			LOG_DEBUG(raContext, DLT_STRING("[callSetMuteState] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_mute_state: "), DLT_INT(request->data.mutestate.as_mute_state),
				DLT_STRING(", as_time: "), DLT_INT(request->data.mutestate.as_time),
				DLT_STRING(", as_ramp: "), DLT_INT(request->data.mutestate.as_ramp));
			break;
		case eREQUEST_TYPE_BALANCE:
			mAsClient->callSetBalance(request->as_sink_id, request->data.balance.as_balance);
			LOG_DEBUG(raContext, DLT_STRING("[callSetBalance] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_balance: "), DLT_INT(request->data.balance.as_balance));
			break;
		case eREQUEST_TYPE_FADER:
			mAsClient->callSetFader(request->as_sink_id, request->data.fader.as_fader);
			LOG_DEBUG(raContext, DLT_STRING("[callSetFader] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_fader: "), DLT_INT(request->data.fader.as_fader));
			break;
		case eREQUEST_TYPE_EQUALIZATION:
			mAsClient->callSetEqualizer(request->as_sink_id,
					request->data.equalizer.as_band, request->data.equalizer.as_gain);
			LOG_DEBUG(raContext, DLT_STRING("[callSetEqualizer] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_band: "), DLT_INT(request->data.equalizer.as_band),
				DLT_STRING(", as_gain: "), DLT_INT(request->data.equalizer.as_gain));
			break;
		case eREQUEST_TYPE_LOUDNESS:
			mAsClient->callSetLoudness(request->as_sink_id, request->data.loudness.as_loudness);
			LOG_DEBUG(raContext, DLT_STRING("[callSetLoudness] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_loudness: "), DLT_INT(request->data.loudness.as_loudness));
			break;
		case eREQUEST_TYPE_SCV:
			mAsClient->callSetSpeedControlledVolume(request->as_sink_id, request->data.scv.as_scv);
			LOG_DEBUG(raContext, DLT_STRING("[callSetSpeedControlledVolume] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_scv: "), DLT_INT(request->data.scv.as_scv));
			break;
		case eREQUEST_TYPE_INPUTGAINOFFSET:
			mAsClient->callSetInputGainOffset(request->as_sink_id,
					request->data.inputgain.as_input_gain_offset);
			LOG_DEBUG(raContext, DLT_STRING("[callSetInputGainOffset] -- as_sink_id: "), DLT_INT(request->as_sink_id),
				DLT_STRING(", as_input_gain_offset: "), DLT_INT(request->data.inputgain.as_input_gain_offset));
			break;
		case eREQUEST_TYPE_SET_SOURCE_STATE:
			//just send an ack for this, no action at AS side
			LOG_DEBUG(raContext, DLT_STRING("B4 [ackSetSourceState] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(E_OK));
			mAmClient->ackSetSourceState(request->handle, E_OK);
			LOG_DEBUG(raContext, DLT_STRING("A4 [ackSetSourceState] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(E_OK));
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_ADD_HOTPLUG_SOURCE:
			status = addHotplugSource(request->data.source);
			if (status != am::E_OK)
			{
				mHotplugSender->ackRegisterSource(request->data.source->name, 0, status);
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request->data.source;
				delete request;
				request = NULL;
			}
			break;
		case eREQUEST_TYPE_ADD_HOTPLUG_SINK:
			status =  addHotplugSink(request->data.sink);
			if (status != am::E_OK)
			{
				mHotplugSender->ackRegisterSink(request->data.sink->name, 0, status);
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request->data.sink;
				delete request;
				request = NULL;
			}
			break;
		case eREQUEST_TYPE_REMOVE_HOTPLUG_SOURCE:
			status =  removeHotplugSource(request->data.sourceID);
			if (status != am::E_OK)
			{
				mHotplugSender->ackDeregisterSource(request->data.sourceID, status);
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request;
				request = NULL;
			}
			break;
		case eREQUEST_TYPE_REMOVE_HOTPLUG_SINK:
			status =  removeHotplugSink(request->data.sinkID);
			if (status != am::E_OK)
			{
				mHotplugSender->ackDeregisterSink(request->data.sinkID, status);
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request;
				request = NULL;
			}
			break;
		case eREQUEST_TYPE_FORWARD_HOTPLUG_SOURCE_STATE:
			status = mHotplugSender->asyncSetSourceState(request->handle.handle, request->data.sourcestate.sourceID, request->data.sourcestate.sourceState);
			break;
		case eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS:
			status = updateHotplugSinkAlsaParameters(request->data.sinkUpdateData->sinkID);
			if (status != am::E_OK)
			{
				// send -ve ack for updateSink
				mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, status);
				LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App, sink ID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request;
				request = NULL;
			}
			break;
		case eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS:
			status = updateHotplugSourceAlsaParameters(request->data.sourceUpdateData->sourceID);
			if (status != am::E_OK)
			{
				// send -ve ack for updateSource
				mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, status);
				LOG_DEBUG(raContext, DLT_STRING(" ackUpdateSource RA -> Hot Plug App, source ID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));
				if (!mRequestQueue.empty()) {
					mRequestQueue.pop_front();
				}
				delete request;
				request = NULL;
			}

			break;
		case eREQUEST_TYPE_HOTPLUG_SOURCE_INTERRUPT_STATUS_CHANGE:
			LOG_DEBUG(raContext, DLT_STRING("B4 hookInterruptStatusChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			mAmClient->hookInterruptStatusChange(request->data.sourceIntStateData.sourceID, request->data.sourceIntStateData.sourceInterruptState);
			LOG_DEBUG(raContext, DLT_STRING("A4 hookInterruptStatusChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			status = E_OK;
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SOURCE_AVAILABILITY_CHANGE:
			LOG_DEBUG(raContext, DLT_STRING("B4 hookSourceAvailablityStatusChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			mAmClient->hookSourceAvailablityStatusChange(request->data.sourceAvailabilityData.sourceID, request->data.sourceAvailabilityData.availability);
			LOG_DEBUG(raContext, DLT_STRING("A4 hookSourceAvailablityStatusChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			status = E_OK;
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SINK_AVAILABILITY_CHANGE:
			LOG_DEBUG(raContext, DLT_STRING("B4 hookSinkAvailablityStatusChange RA -> AM, sink ID: "), DLT_INT(request->am_sink_id));
			mAmClient->hookSinkAvailablityStatusChange(request->data.sinkAvailabilityData.sinkID, request->data.sinkAvailabilityData.availability);
			LOG_DEBUG(raContext, DLT_STRING("A4 hookSinkAvailablityStatusChange RA -> AM, sink ID: "), DLT_INT(request->am_sink_id));
			status = E_OK;
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SOURCE_NOTIF_DATA_CHANGE:
			LOG_DEBUG(raContext, DLT_STRING("B4 hookSourceNotificationDataChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			mAmClient->hookSourceNotificationDataChange(request->data.sourceNotifationData.sourceID, request->data.sourceNotifationData.notifPayload);
			LOG_DEBUG(raContext, DLT_STRING("A4 hookSourceNotificationDataChange RA -> AM, source ID: "), DLT_INT(request->data.sourceID));
			status = E_OK;
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SINK_NOTIF_DATA_CHANGE:
			LOG_DEBUG(raContext, DLT_STRING("B4 hookSinkNotificationDataChange RA -> AM, sink ID: "), DLT_INT(request->am_sink_id));
			mAmClient->hookSinkNotificationDataChange(request->data.sinkNotificationData.sinkID, request->data.sinkNotificationData.notifPayload);
			LOG_DEBUG(raContext, DLT_STRING("A4 hookSinkNotificationDataChange RA -> AM, sink ID: "), DLT_INT(request->am_sink_id));
			status = E_OK;
			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SOURCE_UPDATE:
			LOG_DEBUG(raContext, DLT_STRING("B4 updateSource RA -> AM, source ID: "), DLT_INT(request->data.sourceUpdateData->sourceID));
			status = mAmClient->updateSource(request->data.sourceUpdateData->sourceID,
					request->data.sourceUpdateData->sourceClassID,
					request->data.sourceUpdateData->listSoundProperties,
					request->data.sourceUpdateData->listConnectionFormats,
					request->data.sourceUpdateData->listMainSoundProperties);
			LOG_DEBUG(raContext, DLT_STRING("A4 updateSource RA -> AM, source ID: "), DLT_INT(request->data.sourceUpdateData->sourceID));
			// send ack to app
			mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, status);
			LOG_DEBUG(raContext, DLT_STRING(" ackUpdateSource RA -> Hot Plug App, source ID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));

			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request->data.sourceUpdateData;
			request->data.sourceUpdateData = NULL;
			delete request;
			request = NULL;
			break;
		case eREQUEST_TYPE_HOTPLUG_SINK_UPDATE:
			LOG_DEBUG(raContext, DLT_STRING("B4 updateSink RA -> AM, sink ID: "), DLT_INT(request->data.sinkUpdateData->sinkID));
			status = mAmClient->updateSink(request->data.sinkUpdateData->sinkID,
					request->data.sinkUpdateData->sinkClassID,
					request->data.sinkUpdateData->listSoundProperties,
					request->data.sinkUpdateData->listConnectionFormats,
					request->data.sinkUpdateData->listMainSoundProperties);
			LOG_DEBUG(raContext, DLT_STRING("A4 updateSink RA -> AM, sink ID: "), DLT_INT(request->data.sinkUpdateData->sinkID));
			// send ack to app
			mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, status);
			LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App, sink ID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));

			if (!mRequestQueue.empty()) {
				mRequestQueue.pop_front();
			}
			delete request->data.sinkUpdateData;
			request->data.sinkUpdateData = NULL;
			delete request;
			request = NULL;
			break;
		default:
			LOG_ERROR(raContext, DLT_STRING("Invalid Request Type: "), DLT_INT(request->type));
			break;
	}

	if (request != NULL)
	{
		request->state = eREQUEST_STATE_SENT_TO_AS;
	}

	LOG_FN_EXIT(raContext);
	return;
}

void CRaRequestManager::SendDummyAckToAM(Request *request)
{
	LOG_FN_ENTRY(raContext);
	am_Error_e error = E_OK;

	if (request == NULL) {
		LOG_FN_EXIT(raContext);
		return;
	}

	switch (request->type) {
		case eREQUEST_TYPE_CONNECT:
			mContextManager->SetConnectionData(request->data.connection.am_connection_id,
					request->data.connection.am_source_id,
					request->am_sink_id);
			mAmClient->ackConnect(request->handle,
					request->data.connection.am_connection_id,
					error);
			break;
		case eREQUEST_TYPE_DISCONNECT:
			mContextManager->RemoveConnectionData(request->data.connection.am_connection_id);
			mAmClient->ackDisconnect(request->handle,
					request->data.connection.am_connection_id,
					error);
			break;
		case eREQUEST_TYPE_VOLUME:
			mContextManager->SetVolumeData(request->am_sink_id,
					request->data.volume.am_volume);
			mAmClient->ackSetSinkVolumeChange(request->handle,
					request->data.volume.am_volume,
					error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkVolumeChange] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", volume: "), DLT_INT(request->data.volume.am_volume),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_MUTE_STATE:
			mContextManager->SetMuteStateData(request->am_sink_id,
					request->data.mutestate.am_mute_state);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (mute state)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_BALANCE:
			mContextManager->SetBalanceData(request->am_sink_id,
					request->data.balance.am_balance);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (balance)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_FADER:
			mContextManager->SetFaderData(request->am_sink_id,
					request->data.fader.am_fader);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (fader)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_EQUALIZATION:
			mContextManager->SetEqualizationData(request->am_sink_id,
					request->data.equalizer.am_band,
					request->data.equalizer.am_gain);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (equalizer)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_LOUDNESS:
			mContextManager->SetLoudnessData(request->am_sink_id,
					request->data.loudness.am_loudness);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (loudness)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_SCV:
			mContextManager->SetSCVData(request->am_sink_id,
					request->data.scv.am_scv);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (svc)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_INPUTGAINOFFSET:
			mContextManager->SetInputGainOffset(request->am_sink_id,
					request->data.inputgain.am_input_gain_offset);
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING("Dummy [ackSetSinkSoundProperty (inputgain)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		default:
			break;
	}

	LOG_FN_EXIT(raContext);
}

void CRaRequestManager::DequeueRequest(void)
{
	LOG_FN_ENTRY(raContext);
	pthread_mutex_lock(&mLock);
	if (!mRequestQueue.empty()) {
		mRequestQueue.pop_front();
	}
	pthread_cond_signal(&mCond);
	pthread_mutex_unlock(&mLock);

	LOG_FN_EXIT(raContext);
	return;
}

Request *CRaRequestManager::PeekRequestQueue(void)
{
	LOG_FN_ENTRY(raContext);
	Request *request = NULL;
	pthread_mutex_lock(&mLock);
	if (!mRequestQueue.empty()) {
		request = mRequestQueue.front();
	}
	pthread_mutex_unlock(&mLock);

	LOG_FN_EXIT(raContext);
	return request;
}

void CRaRequestManager::ProcessRequest(void)
{
	LOG_FN_ENTRY(raContext);

	RequestState_e state = eREQUEST_STATE_UNKNOWN;
	while (true)
	{
		pthread_mutex_lock(&mLock);
		state = GetRequestState();
		LOG_DEBUG(raContext, DLT_STRING(" B4 wait loop ...!"));
		while (mRequestQueue.empty() ||
			(state != eREQUEST_STATE_NEW)) {
			LOG_DEBUG(raContext, DLT_STRING(" B4 wait ...!"));
			pthread_cond_wait(&mCond, &mLock);
			state = GetRequestState();
			LOG_DEBUG(raContext, DLT_STRING(" A4 wait ...! queue state:"), DLT_INT(mRequestQueue.empty()),
				DLT_STRING(", request state: "), DLT_INT(state));
		}
		LOG_DEBUG(raContext, DLT_STRING(" A4 wait loop ...!"));

		SendRequestToAS();
		pthread_mutex_unlock(&mLock);
	}
	LOG_FN_EXIT(raContext);
}

am::am_Error_e CRaRequestManager::registerDomainData(const uint16_t handle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle));

	am_Error_e error;
	am_domainID_t domainID = 0;
	ListSourceData_t sources;
	ListSourceDataIter_t sourceIter;
	Source_t sourceData;
	ListSinkData_t sinks;
	ListSinkDataIter_t sinkIter;
	Sink_t sinkData;
	ListGatewayData_t gateways;
	ListGatewayDataIter_t gatewayIter;
	Gateway_t gatewayData;
	ListCrossFader_t crossfaders;
	ListCrossFaderIter_t crossfaderIter;
	CrossFader_t crossfaderData;


	/*
	 * First of all confirm the routing readu
	 *
	 */
	mAmClient->confirmRoutingReady(handle,E_OK);
	if (NULL == mConfigManager)
	{
		LOG_ERROR(raContext, DLT_STRING("mConfigManager == NULL"));
		LOG_FN_EXIT(raContext);
		return E_UNKNOWN;
	}

	error = mAmClient->registerDomain(mConfigManager->GetDomain(), domainID);

	if (E_OK != error) {
		LOG_ERROR(raContext, DLT_STRING("registerDomain() : domainID: "),
		DLT_INT(domainID), DLT_STRING(" error:"), DLT_INT(error));
	} else {
		LOG_INFO(raContext, DLT_STRING("registerDomain() : domainID: "),
		DLT_INT(domainID), DLT_STRING(" error:"), DLT_INT(error));
		mContextManager->SetDomainID(domainID);
	}

	sources = mConfigManager->GetSources();
	for (sourceIter = sources.begin(); sourceIter != sources.end(); ++sourceIter)
	{
		SourceInfo_t	sourceInfo;
		sourceData = *sourceIter;
		sourceData.am_source.domainID = domainID;
		if (sourceData.owner == OWNER_ROUTING)
		{
			error = mAmClient->registerSource(sourceData.am_source, sourceData.am_source.sourceID);
			if (E_OK != error) {
				LOG_ERROR(raContext, DLT_STRING("registerSource() : sourceID: "),
				DLT_INT(sourceData.am_source.sourceID), DLT_STRING(" error:"), DLT_INT(error), DLT_STRING(" : name : "), DLT_STRING(sourceData.am_source.name.c_str()));
			} else {
				LOG_INFO(raContext, DLT_STRING("registerSource() : sourceID: "),
				DLT_INT(sourceData.am_source.sourceID), DLT_STRING(" error:"), DLT_INT(error), DLT_STRING(" : name : "), DLT_STRING(sourceData.am_source.name.c_str()));
			}
			sourceInfo.isSourceOwnerRouter = true;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("source: "), DLT_STRING(sourceData.am_source.name.c_str()),
					DLT_STRING(" sourceID: "), DLT_INT(sourceData.am_source.sourceID), DLT_STRING(" owner != ROUTING"));
			sourceInfo.isSourceOwnerRouter = false;
			error = E_OK;
		}

		if (E_OK == error)
		{
			sourceInfo.as_source_id = sourceData.as_source_id;
			mContextManager->SetSourceMap(sourceData.am_source.sourceID, sourceInfo);
		}
	}

	sinks = mConfigManager->GetSinks();
	for (sinkIter = sinks.begin(); sinkIter != sinks.end(); ++sinkIter)
	{
		SinkInfo_t	sinkInfo;
		sinkData = *sinkIter;
		sinkData.am_sink.domainID = domainID;
		if (sinkData.owner == OWNER_ROUTING)
		{
			error = mAmClient->registerSink(sinkData.am_sink, sinkData.am_sink.sinkID);
			if (E_OK != error) {
				LOG_ERROR(raContext, DLT_STRING("registerSink() : sinkID: "),
				DLT_INT(sinkData.am_sink.sinkID), DLT_STRING(" error:"), DLT_INT(error), DLT_STRING(" : name : "), DLT_STRING(sinkData.am_sink.name.c_str()));
			} else {
				LOG_INFO(raContext, DLT_STRING("registerSink() : sinkID: "),
				DLT_INT(sinkData.am_sink.sinkID), DLT_STRING(" error:"), DLT_INT(error), DLT_STRING(" : name : "), DLT_STRING(sinkData.am_sink.name.c_str()));
			}
			sinkInfo.isSinkOwnerRouter = true;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("sink: "), DLT_STRING(sinkData.am_sink.name.c_str()),
					DLT_STRING(" sinkID: "), DLT_INT(sinkData.am_sink.sinkID), DLT_STRING(" owner != ROUTING"));
			sinkInfo.isSinkOwnerRouter = false;
			error = E_OK;
		}

		if (E_OK == error)
		{
			sinkInfo.as_sink_id = sinkData.as_sink_id;
			mContextManager->SetSinkMap(sinkData.am_sink.sinkID, sinkInfo);
		}

	}

	GatewayInfo_t gatewayInfo;
	gateways = mConfigManager->GetGateways();
	for (gatewayIter = gateways.begin(); gatewayIter != gateways.end(); ++gatewayIter)
	{
		error = mAmClient->peekSource(gatewayIter->gwSourceName, gatewayIter->sourceID);
		if (E_OK != error)
		{
			LOG_ERROR(raContext, DLT_STRING("peekSource() : sourceName: "),
			DLT_STRING(gatewayIter->gwSourceName.c_str()), DLT_STRING(" error:"), DLT_INT(error));
			gatewayIter->sourceID = 0;
			continue;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("peekSource() : sourceName: "),
			DLT_STRING(gatewayIter->gwSourceName.c_str()), DLT_STRING(" sourceID: "), DLT_INT(gatewayIter->sourceID));
		}

		error = mAmClient->peekSink(gatewayIter->gwSinkName, gatewayIter->sinkID);
		if (E_OK != error)
		{
			LOG_ERROR(raContext, DLT_STRING("peekSink() : sinkName: "),
			DLT_STRING(gatewayIter->gwSinkName.c_str()), DLT_STRING(" error:"), DLT_INT(error));
			gatewayIter->sinkID = 0;
			continue;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("peekSink() : sinkName: "),
			DLT_STRING(gatewayIter->gwSinkName.c_str()), DLT_STRING(" sinkID: "), DLT_INT(gatewayIter->sinkID));
		}

		error = mAmClient->peekDomain(gatewayIter->gwSourceDomainName, gatewayIter->domainSourceID);
		if (E_OK != error)
		{
			LOG_ERROR(raContext, DLT_STRING("peekDomain() : SourceDomainName: "),
			DLT_STRING(gatewayIter->gwSourceDomainName.c_str()), DLT_STRING(" error:"), DLT_INT(error));
			gatewayIter->domainSourceID = 0;
			continue;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("peekDomain() : SourceDomainName: "),
			DLT_STRING(gatewayIter->gwSourceDomainName.c_str()), DLT_STRING(" SourceDomainName: "), DLT_INT(gatewayIter->domainSourceID));
		}

		error = mAmClient->peekDomain(gatewayIter->gwSinkDomainName, gatewayIter->domainSinkID);
		if (E_OK != error)
		{
			LOG_ERROR(raContext, DLT_STRING("peekDomain() : SinkDomainName: "),
			DLT_STRING(gatewayIter->gwSinkDomainName.c_str()), DLT_STRING(" error:"), DLT_INT(error));
			gatewayIter->domainSinkID = 0;
			continue;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("peekDomain() : SinkDomainName: "),
			DLT_STRING(gatewayIter->gwSinkDomainName.c_str()), DLT_STRING(" SinkDomainName: "), DLT_INT(gatewayIter->domainSinkID));
		}

		gatewayData = *gatewayIter;

		if (gatewayData.owner == OWNER_ROUTING)
		{
			error = mAmClient->registerGateway(gatewayData, gatewayData.gatewayID);
			if (E_OK != error) {
				LOG_ERROR(raContext, DLT_STRING("registerGateway() : gatewayID: "),
				DLT_INT(gatewayData.gatewayID), DLT_STRING(" error:"), DLT_INT(error));
			} else {
				LOG_INFO(raContext, DLT_STRING("registerGateway() : gatewayID: "),
				DLT_INT(gatewayData.gatewayID), DLT_STRING(" error:"), DLT_INT(error));
			}
			gatewayInfo.isGatewayOwnerRouter = true;
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("gateway: "), DLT_STRING(gatewayData.name.c_str()),
					DLT_STRING(" gatewayID: "), DLT_INT(gatewayData.gatewayID), DLT_STRING(" owner != ROUTING"));
			gatewayInfo.isGatewayOwnerRouter = false;
			error = E_OK;
		}

		if (E_OK == error)
		{
			mContextManager->SetGatewayMap(gatewayData.gatewayID, gatewayInfo);
		}
	}

	crossfaders = mConfigManager->GetCrossfaders();
	for (crossfaderIter = crossfaders.begin(); crossfaderIter != crossfaders.end(); ++crossfaderIter)
	{
		crossfaderData = *crossfaderIter;
		if (crossfaderData.owner == OWNER_ROUTING)
		{
			error = mAmClient->registerCrossfader(crossfaderData, crossfaderData.crossfaderID);
			if (E_OK != error) {
				LOG_ERROR(raContext, DLT_STRING("registerCrossfader() : crossfaderID: "),
				DLT_INT(crossfaderData.crossfaderID), DLT_STRING(" error:"), DLT_INT(error));
			} else {
				LOG_INFO(raContext, DLT_STRING("registerCrossfader() : crossfaderID: "),
				DLT_INT(crossfaderData.crossfaderID), DLT_STRING(" error:"), DLT_INT(error));
			}
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("crossfader: "), DLT_STRING(crossfaderData.name.c_str()),
					DLT_STRING(" crossfaderID: "), DLT_INT(crossfaderData.crossfaderID), DLT_STRING(" owner != ROUTING"));
			error = E_OK;
		}
	}

	/*
	 * Finally send the domain registration complete
	 */
	mAmClient->hookDomainRegistrationComplete(domainID);
	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am::am_Error_e CRaRequestManager::unregisterDomainData(const uint16_t handle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" handle: "), DLT_UINT(handle));

	am_Error_e error = E_OK;
	std::vector<am_sourceID_t> sources;
	std::vector<am_sourceID_t>::iterator sourceIter;
	std::vector<am_sinkID_t> sinks;
	std::vector<am_sinkID_t>::iterator sinkIter;
	std::vector<am_gatewayID_t> gateways;
	std::vector<am_gatewayID_t>::iterator gatewayIter;

	sources = mContextManager->GetRegisteredSources();
	for (sourceIter = sources.begin(); sourceIter != sources.end(); ++sourceIter)
	{
		am_sourceID_t sourceID = *sourceIter;
		if (mContextManager->IsSourceOwnerRouter(sourceID))
		{
			error = mAmClient->deregisterSource(sourceID);
			if (E_OK != error)
			{
				LOG_ERROR(raContext, DLT_STRING("deregisterSource() : sourceID: "), DLT_INT(sourceID), DLT_STRING(" error:"), DLT_INT(error));
			}
			else
			{
				LOG_INFO(raContext, DLT_STRING("deregisterSource() : sourceID: "), DLT_INT(sourceID), DLT_STRING(" error:"), DLT_INT(error));
			}
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("sourceID: "),  DLT_INT(sourceID), DLT_STRING(" owner != ROUTING"));
		}
	}

	error = E_OK;
	sinks = mContextManager->GetRegisteredSinks();
	for (sinkIter = sinks.begin(); sinkIter != sinks.end(); ++sinkIter)
	{
		am_sinkID_t sinkID = *sinkIter;
		if (mContextManager->IsSinkOwnerRouter(sinkID))
		{
			error = mAmClient->deregisterSink(sinkID);
			if (E_OK != error)
			{
				LOG_ERROR(raContext, DLT_STRING("deregisterSink() : sinkID: "), DLT_INT(sinkID), DLT_STRING(" error:"), DLT_INT(error));
			}
			else
			{
				LOG_INFO(raContext, DLT_STRING("deregisterSink() : sinkID: "), DLT_INT(sinkID), DLT_STRING(" error:"), DLT_INT(error));
			}
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("sinkID: "),  DLT_INT(sinkID), DLT_STRING(" owner != ROUTING"));
		}
	}

	error = E_OK;
	gateways = mContextManager->GetRegisteredGateways();
	for (gatewayIter = gateways.begin(); gatewayIter != gateways.end(); ++gatewayIter)
	{
		am_gatewayID_t gatewayID = *gatewayIter;
		if (mContextManager->IsGatewayOwnerRouter(gatewayID))
		{
			error = mAmClient->deregisterGateway(gatewayID);
			if (E_OK != error)
			{
				LOG_ERROR(raContext, DLT_STRING("deregisterGateway() : gatewayID: "), DLT_INT(gatewayID), DLT_STRING(" error:"), DLT_INT(error));
			}
			else
			{
				LOG_INFO(raContext, DLT_STRING("deregisterGateway() : gatewayID: "), DLT_INT(gatewayID), DLT_STRING(" error:"), DLT_INT(error));
			}
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("gatewayID: "),  DLT_INT(gatewayID), DLT_STRING(" owner != ROUTING"));
		}
	}

	error = E_OK;
	am_domainID_t domainID = mContextManager->GetDomainID();
	if (0 != domainID)
	{
		error = mAmClient->deregisterDomain(domainID);
		if (E_OK != error)
		{
			LOG_ERROR(raContext, DLT_STRING("deregisterDomain() : domainID: "), DLT_INT(domainID), DLT_STRING(" error:"), DLT_INT(error));
		}
		else
		{
			LOG_INFO(raContext, DLT_STRING("deregisterDomain() : domainID: "), DLT_INT(domainID), DLT_STRING(" error:"), DLT_INT(error));
		}
	}
	else
	{
		LOG_ERROR(raContext, DLT_STRING("DomainID == 0 in the context manager; can not de-register"));
	}

	mContextManager->ClearContext();
	LOG_FN_EXIT(raContext, DLT_STRING(" error: "), DLT_INT(error));
	return error;
}

am::am_Error_e CRaRequestManager::addHotplugSource(am::am_Source_s *source)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" SourceName: "), DLT_STRING(source->name.c_str()));
	AlsaParams_s alsaParams;
	am::am_Error_e status = am::E_UNKNOWN;
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;
	status = getAlsaParamsFromSoundProperties(source->listSoundProperties, alsaParams);
	if (status != am::E_OK)
	{
		LOG_ERROR(raContext, DLT_STRING(" incomplete alsa params"));
		LOG_FN_EXIT(raContext);
		return am::E_UNKNOWN;
	}

	ipcResult = mAsClient->callAddAlsaDevice(alsaParams.alsaCard, alsaParams.alsaDevice, alsaParams.nrChannels, IasAudioSetup::eIasSource, alsaParams.asId, alsaParams.isSyncToSysClk, alsaParams.sampleFormat, alsaParams.sampleRate);

	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callAddAlsaDevice SourceName: "), DLT_STRING(source->name.c_str()));
		LOG_FN_EXIT(raContext);
		return am::E_UNKNOWN;
	}
	else
	{
		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callAddAlsaDevice SourceName: "), DLT_STRING(source->name.c_str()));
	}

	LOG_FN_EXIT(raContext);
	return am::E_OK;
}

am::am_Error_e CRaRequestManager::addHotplugSink(am::am_Sink_s *sink)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sink name: "), DLT_STRING(sink->name.c_str()));
	AlsaParams_s alsaParams;
	am::am_Error_e status = am::E_UNKNOWN;
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;
	status = getAlsaParamsFromSoundProperties(sink->listSoundProperties, alsaParams);
	if (status != am::E_OK)
	{
		LOG_ERROR(raContext, DLT_STRING("CRaRequestManager::addHotplugSink failed - incomplete alsa params"));
		LOG_FN_EXIT(raContext);
		return am::E_UNKNOWN;
	}

	ipcResult = mAsClient->callAddAlsaDevice(alsaParams.alsaCard, alsaParams.alsaDevice, alsaParams.nrChannels, IasAudioSetup::eIasSink, alsaParams.asId, alsaParams.isSyncToSysClk, alsaParams.sampleFormat, alsaParams.sampleRate);

	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callAddAlsaDevice SinkName: "), DLT_STRING(sink->name.c_str()));
		LOG_FN_EXIT(raContext);
		return am::E_UNKNOWN;
	}
	else
	{
		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callAddAlsaDevice SinkName: "), DLT_STRING(sink->name.c_str()));
	}

	LOG_FN_EXIT(raContext);
	return am::E_OK;
}

am::am_Error_e CRaRequestManager::removeHotplugSource(am::am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_UINT16(sourceID));
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;
	Ias::UInt32 asSourceHandle = mContextManager->GetAsSourceHandleFromAmSourceID(sourceID);
	ipcResult = mAsClient->callDeleteAlsaDevice(asSourceHandle);
 	if (IAS_UFIPC_FAILED(ipcResult)) {
 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sourceID: "), DLT_UINT16(sourceID),
 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
		LOG_FN_EXIT(raContext);
 		return am::E_UNKNOWN;
 	}
 	else
 	{
 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sourceID: "), DLT_UINT16(sourceID),
 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
 	}

	LOG_FN_EXIT(raContext);
 	return am::E_OK;
}

am::am_Error_e CRaRequestManager::removeHotplugSink(am::am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_UINT16(sinkID));
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;
	Ias::UInt32 asSinkHandle = mContextManager->GetAsSinkHandleFromAmSinkID(sinkID);
	ipcResult = mAsClient->callDeleteAlsaDevice(asSinkHandle);
 	if (IAS_UFIPC_FAILED(ipcResult)) {
 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sinkID: "), DLT_UINT16(sinkID),
 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
		LOG_FN_EXIT(raContext);
 		return am::E_UNKNOWN;
 	}
 	else
 	{
 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sinkID: "), DLT_UINT16(sinkID),
 		 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
 	}

	LOG_FN_EXIT(raContext);
 	return am::E_OK;
}

am::am_Error_e CRaRequestManager::updateHotplugSinkAlsaParameters(am::am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_UINT16(sinkID));
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;

	if (mContextManager->IsConnectedSink(sinkID))
	{
		am_sourceID_t sourceID = mContextManager->GetConnectedSourceIDFromSinkID(sinkID);
		Ias::UInt32 asSourceID = mContextManager->GetAsSourceIDFromAmSourceID(sourceID);
		Ias::UInt32 asSinkID = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);

		LOG_DEBUG(raContext, DLT_STRING("sinkID: "),
				DLT_UINT16(sinkID), DLT_STRING(" is a connected sink. send callReleaseConnection to audio server"));

		ipcResult = mAsClient->callReleaseConnection(asSourceID, asSinkID);
		if (IAS_UFIPC_FAILED(ipcResult)) {
	 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callReleaseConnection sourceID: "), DLT_UINT32(asSourceID),
	 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
			LOG_FN_EXIT(raContext);
	 		return am::E_UNKNOWN;
	 	}
	 	else
	 	{
	 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callReleaseConnection sourceID: "), DLT_UINT32(asSourceID),
	 		 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
	 	}
	}
	else
	{
		Ias::UInt32 asSinkHandle = mContextManager->GetAsSinkHandleFromAmSinkID(sinkID);

		LOG_DEBUG(raContext, DLT_STRING("sinkID: "),
						DLT_UINT16(sinkID), DLT_STRING(" is a not connected sink. send callDeleteAlsaDevice to audio server"));

		ipcResult = mAsClient->callDeleteAlsaDevice(asSinkHandle);
	 	if (IAS_UFIPC_FAILED(ipcResult)) {
	 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sinkID: "), DLT_UINT16(sinkID),
	 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
	 		return am::E_UNKNOWN;
	 	}
	 	else
	 	{
	 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sinkID: "), DLT_UINT16(sinkID),
	 		 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
	 	}
	}

	LOG_FN_EXIT(raContext);
	return am::E_OK;
}

am::am_Error_e CRaRequestManager::updateHotplugSourceAlsaParameters(am::am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_UINT16(sourceID));
	ias_ufipc_result ipcResult = eIAS_UFIPC_OK;
	if (mContextManager->IsConnectedSource(sourceID))
	{
		am_sinkID_t sinkID = mContextManager->GetConnectedSinkIDFromSourceID(sourceID);
		Ias::UInt32 asSourceID = mContextManager->GetAsSourceIDFromAmSourceID(sourceID);
		Ias::UInt32 asSinkID = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);

		ipcResult = mAsClient->callReleaseConnection(asSourceID, asSinkID);
		if (IAS_UFIPC_FAILED(ipcResult)) {
	 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callReleaseConnection sourceID: "), DLT_UINT32(asSourceID),
	 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
			LOG_FN_EXIT(raContext);
	 		return am::E_UNKNOWN;
	 	}
	 	else
	 	{
	 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callReleaseConnection sourceID: "), DLT_UINT32(asSourceID),
	 		 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
	 	}
	}
	else
	{
		Ias::UInt32 asSourceHandle = mContextManager->GetAsSourceHandleFromAmSourceID(sourceID);
		ipcResult = mAsClient->callDeleteAlsaDevice(asSourceHandle);
	 	if (IAS_UFIPC_FAILED(ipcResult)) {
	 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sourceID: "), DLT_UINT16(sourceID),
	 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
			LOG_FN_EXIT(raContext);
	 		return am::E_UNKNOWN;
	 	}
	 	else
	 	{
	 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sourceID: "), DLT_UINT16(sourceID),
	 		 		 		 				DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
	 	}
	}

	LOG_FN_EXIT(raContext);
	return am::E_OK;
}
