/************************************************************************
* @file: CRaASClient.cpp
*
* @version: 1.1
*
* CRaASClient class implements the callbacks of interface classes
* IasAudioRouting, IasAudioProcessing, IasAudioSetup
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
#include "CRaASClient.h"
#include "CRaRequestManager.h"
#include "CRaConfigManager.h"
#include "CRaAMRoutingClient.h"
#include "Log.h"

#include <iostream>
#include <assert.h>
using namespace std;

#define NO_EVENT_WORK_AROUND 1

DLT_IMPORT_CONTEXT(raContext);

CRaASClient::CRaASClient(CRaConfigManager *configManager, CRaRequestManager *requestManager, CRaAMRoutingClient *amClient, CContextManager *contextManager) :
mAmClient(amClient), mConfigManager(configManager), mRequestManager(requestManager), mDelayedRequestCount(0), mContextManager(contextManager), mHotplugSender(0)
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

CRaASClient::~CRaASClient()
{
	LOG_FN_ENTRY(raContext);
	mAmClient = NULL;
	mConfigManager = NULL;
	mRequestManager = NULL;
	mContextManager = NULL;
	mHotplugSender = NULL;
	LOG_FN_EXIT(raContext);
}

void CRaASClient::SetRequestManager(CRaRequestManager *requestManager)
{
	LOG_FN_ENTRY(raContext);
	mRequestManager = requestManager;
	LOG_FN_EXIT(raContext);
}

void CRaASClient::SetAMClient(CRaAMRoutingClient *amClient)
{
	LOG_FN_ENTRY(raContext);
	mAmClient  = amClient;
	LOG_FN_EXIT(raContext);
}
void CRaASClient::SetHotplugSender(CRaHotplugSender *hotplugSender)
{
	LOG_FN_ENTRY(raContext);
	mHotplugSender = hotplugSender;
	LOG_FN_EXIT(raContext);
}
void CRaASClient::errorEventAudioRouting(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId)
{
	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING(" communicationResult: "), DLT_INT(communicationResult),
		DLT_STRING(", calledFunctionId: "), DLT_INT(calledFunctionId));

	SendAckToAM(E_UNKNOWN);
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseConnectionList(IasAudioRouting::IasConnectionVector const &connectionVector)
{
	(void) connectionVector;
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseStatusAudioRouting(IasAudioRouting::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" response code: "), DLT_INT(code));
	am_Error_e                      error = E_OK;

	if (IasAudioRouting::eIasNoError != code) {
		LOG_ERROR(raContext, DLT_STRING(" received error response !! code: "), DLT_INT(code));
	} else {
		LOG_INFO(raContext, DLT_STRING(" received response IasAudioRouting::eIasNoError "));
	}

	switch (code) {
		case IasAudioRouting::eIasNoError:
			error = E_OK;
			break;
		case IasAudioRouting::eIasInvalidId:
			error = E_NON_EXISTENT;
			break;
		case IasAudioRouting::eIasError:
			error = E_NO_CHANGE;
			break;
		case IasAudioRouting::eIasSourceSinkNotConnected:
			error = E_NOT_POSSIBLE;
			break;
		case IasAudioRouting::eIasSinkInUse:
			error = E_ALREADY_EXISTS;
			break;
		default:
			error = E_UNKNOWN;
			break;
	}

	Request         *request = NULL;
	request = mRequestManager->PeekRequestQueue();

	if (code == IasAudioRouting::eIasNoError) {
		mRequestManager->SetRequestState(eREQUEST_STATE_RESPONSE_OK);

		if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS)
		{
			if (mContextManager->IsConnectedSink(request->data.sinkUpdateData->sinkID))
			{
				// this is response of internal disconnection request
				if (!mContextManager->IsSinkInternallyDisconnected(request->data.sinkUpdateData->sinkID))
				{
					LOG_DEBUG(raContext, DLT_STRING("response for callReleaseConnection"));
					mContextManager->SetSinkInterallyDisconnected(request->data.sinkUpdateData->sinkID);
					// delete ALSA device
					Ias::UInt32 asSinkHandle = mContextManager->GetAsSinkHandleFromAmSinkID(request->data.sinkUpdateData->sinkID);

					LOG_DEBUG(raContext, DLT_STRING("sent request callDeleteAlsaDevice"));
					ias_ufipc_result ipcResult = callDeleteAlsaDevice(asSinkHandle);
					if (IAS_UFIPC_FAILED(ipcResult)) {
						LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sinkID: "), DLT_UINT16(request->data.sinkUpdateData->sinkID),
												DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
						// send -ve ack from RA -> Hot Plug App
						mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, am::E_UNKNOWN);
						LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App, sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: UFIPC failed"));
					}
					else
					{
						LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sinkID: "), DLT_UINT16(request->data.sinkUpdateData->sinkID),
														DLT_STRING(" deviceHandle: "), DLT_UINT32(asSinkHandle));
					}
				}
				// this is response of re-connection request
				else
				{
					mContextManager->SetSinkInterallyConnected(request->data.sinkUpdateData->sinkID);
					// send update to am
					am_Error_e status = mAmClient->updateSink(request->data.sinkUpdateData->sinkID,
							request->data.sinkUpdateData->sinkClassID,
							request->data.sinkUpdateData->listSoundProperties,
							request->data.sinkUpdateData->listConnectionFormats,
							request->data.sinkUpdateData->listMainSoundProperties);
					LOG_DEBUG(raContext, DLT_STRING("updateSink RA -> AM, sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));
					// we have finished internal handling of update sink ALSA parameters
					// send the ack to Hot Plug App
					mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, status);
					LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App, sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));
					// remove the request from queue and delete.
					mRequestManager->DequeueRequest();
					delete request->data.sinkUpdateData;
					delete request;

				}
				// return from here as we do not want to send ack to am
				return;
			}
		}
		else if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS)
		{
			if (mContextManager->IsConnectedSource(request->data.sourceUpdateData->sourceID))
			{
				// this is response of internal disconnection request
				if (!mContextManager->IsSourceInternallyDisconnected(request->data.sourceUpdateData->sourceID))
				{
					mContextManager->SetSourceInterallyDisconnected(request->data.sourceUpdateData->sourceID);
					LOG_DEBUG(raContext, DLT_STRING("response for callReleaseConnection"));
					// delete ALSA device
					Ias::UInt32 asSourceHandle = mContextManager->GetAsSourceHandleFromAmSourceID(request->data.sourceUpdateData->sourceID);
					LOG_DEBUG(raContext, DLT_STRING("sent request callDeleteAlsaDevice"));
					ias_ufipc_result ipcResult = callDeleteAlsaDevice(asSourceHandle);
					if (IAS_UFIPC_FAILED(ipcResult)) {
						LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callDeleteAlsaDevice sourceID: "), DLT_UINT16(request->data.sourceUpdateData->sourceID),
												DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
						mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, am::E_UNKNOWN);
						LOG_DEBUG(raContext, DLT_STRING("ackUpdateSource RA -> Hot Plug App, sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: UFIPC failed"));
					}
					else
					{
						LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callDeleteAlsaDevice sourceID: "), DLT_UINT16(request->data.sourceUpdateData->sourceID),
														DLT_STRING(" deviceHandle: "), DLT_UINT32(asSourceHandle));
					}
				}
				// this is response of re-connection request
				else
				{
					mContextManager->SetSourceInterallyConnected(request->data.sourceUpdateData->sourceID);
					// update am
					am_Error_e status = mAmClient->updateSource(request->data.sourceUpdateData->sourceID,
							request->data.sourceUpdateData->sourceClassID,
							request->data.sourceUpdateData->listSoundProperties,
							request->data.sourceUpdateData->listConnectionFormats,
							request->data.sourceUpdateData->listMainSoundProperties);

					LOG_DEBUG(raContext, DLT_STRING("updateSource RA -> AM, sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));
					// we have finished internal handling of update sink ALSA parameters
					// send the ack to Hot Plug App
					mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, status);
					LOG_DEBUG(raContext, DLT_STRING("ackUpdateSource RA -> Hot Plug App, sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));
					// remove the request from queue and delete.
					mRequestManager->DequeueRequest();
					delete request->data.sourceUpdateData;
					delete request;

				}

				// return from here as we do not want to send ack to am
				return;
			}
		}
#if NO_EVENT_WORK_AROUND
		SendAckToAM(error);
#endif
	}
	else
	{
		if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS
		 || request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS)
		{
			// TODO: send -ve ack for update source or sink
			// time being just return as we do not want send ack to am
			return;
		}
		SendAckToAM(error);
	}

	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventConnectionState(Ias::Int32 const &source,
		      Ias::Int32 const &sink,
		      IasAudioRouting::IasConnectionState const &state)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" source: "), DLT_INT(source),
		DLT_STRING(", sink: "), DLT_INT(sink),
		DLT_STRING(", state: "), DLT_INT(state));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::errorEventAudioProcessing(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId)
{
	LOG_FN_ENTRY(raContext, DLT_STRING("[CRaASClient::errorEventAudioProcessing] -- communicationResult: "), DLT_INT(communicationResult),
		DLT_STRING(", calledFunctionId: "), DLT_INT(calledFunctionId));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_UNKNOWN);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseStatusAudioProcessing(IasAudioProcessing::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" recived response code: "), DLT_INT(code));
	if (IasAudioProcessing::eIasNoError != code) {
		LOG_ERROR(raContext, DLT_STRING(" received error response !! code: "), DLT_INT(code));
	} else {
		LOG_INFO(raContext, DLT_STRING(" received response IasAudioProcessing::eIasNoError "));
	}

	am_Error_e			error = E_OK;

	// check if a delayed request issued
	// if yes n do not send ack
	if (IsDelayed())
	{
		DecrementDelayedRequestCount();
		return;
	}

	/*map AS side error codes with AM side error codes */
	switch (code) {
		case IasAudioProcessing::eIasNoError:
			error = E_OK;
			break;
		case IasAudioProcessing::eIasError:
			error = E_NO_CHANGE;
			break;
		case IasAudioProcessing::eIasInvalidId:
			error = E_NON_EXISTENT;
			break;
		case IasAudioProcessing::eIasNotConfigured:
			error = E_UNKNOWN;
			break;
		case IasAudioProcessing::eIasOutOfRange:
			error = E_OUT_OF_RANGE;
			break;
		default:
			error = E_UNKNOWN;
			break;
	}

	if (code == IasAudioProcessing::eIasNoError) {
		mRequestManager->SetRequestState(eREQUEST_STATE_RESPONSE_OK);
#if NO_EVENT_WORK_AROUND
		SendAckToAM(error);
#endif
	} else {
		SendAckToAM(error);
	}
	LOG_FN_EXIT(raContext);
}

bool CRaASClient::SendAckToAM(am_Error_e error)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" error: "), DLT_INT(error));

	Request         *request = NULL;
	request = mRequestManager->PeekRequestQueue();

	if (request == NULL) {
		LOG_ERROR(raContext, DLT_STRING("request == NULL"));
		LOG_FN_EXIT(raContext);
		return false;
	}

	if (request->state == eREQUEST_STATE_NEW) {
		/* should not hit here ... */
		LOG_ERROR(raContext, DLT_STRING("request->state == eREQUEST_STATE_NEW"));
		LOG_FN_EXIT(raContext);
		return false;
	}


	switch (request->type) {
		case eREQUEST_TYPE_CONNECT:
			if (error == E_OK) {
				// process delayed requests for the sink if any
				mRequestManager->ProcessDelayedRequests(request->am_sink_id);
				// delete processed delayed requests
				mRequestManager->RemoveDelayedRequests(request->am_sink_id);

				mContextManager->SetConnectionData(request->data.connection.am_connection_id,
						request->data.connection.am_source_id,
						request->am_sink_id);
			}
			mAmClient->ackConnect(request->handle,
					request->data.connection.am_connection_id,
					error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackConnect] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", connection id: "), DLT_INT(request->data.connection.am_connection_id),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_DISCONNECT:
			if (error == E_OK) {
				mContextManager->RemoveConnectionData(request->data.connection.am_connection_id);
			}
			mAmClient->ackDisconnect(request->handle,
					request->data.connection.am_connection_id,
					error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackDisconnect] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", connection id: "), DLT_INT(request->data.connection.am_connection_id),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_VOLUME:
			if (error == E_OK) {
				mContextManager->SetVolumeData(request->am_sink_id,
						request->data.volume.am_volume);
			}
			mAmClient->ackSetSinkVolumeChange(request->handle,
					request->data.volume.am_volume,
					error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkVolumeChange] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", volume: "), DLT_INT(request->data.volume.am_volume),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_MUTE_STATE:
			if (error == E_OK) {
				mContextManager->SetMuteStateData(request->am_sink_id,
						request->data.mutestate.am_mute_state);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (muteState)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_BALANCE:
			if (error == E_OK) {
				mContextManager->SetBalanceData(request->am_sink_id,
						request->data.balance.am_balance);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (balance)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_FADER:
			if (error == E_OK) {
				mContextManager->SetFaderData(request->am_sink_id,
						request->data.fader.am_fader);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (fader)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_EQUALIZATION:
			if (error == E_OK) {
				mContextManager->SetEqualizationData(request->am_sink_id,
						request->data.equalizer.am_band,
						request->data.equalizer.am_gain);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (equalizer)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_LOUDNESS:
			if (error == E_OK) {
				mContextManager->SetLoudnessData(request->am_sink_id,
						request->data.loudness.am_loudness);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (loudness)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_SCV:
			if (error == E_OK) {
				mContextManager->SetSCVData(request->am_sink_id,
						request->data.scv.am_scv);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (svc)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		case eREQUEST_TYPE_INPUTGAINOFFSET:
			if (error == E_OK) {
				mContextManager->SetInputGainOffset(request->am_sink_id,
						request->data.inputgain.am_input_gain_offset);
			}
			mAmClient->ackSetSinkSoundProperty(request->handle, error);
			LOG_DEBUG(raContext, DLT_STRING(" Sent [ackSetSinkSoundProperty (inputgain)] -- handle: "), DLT_UINT(request->handle.handle),
				DLT_STRING(", error: "), DLT_INT(error));
			break;
		default:
			break;
	}

	mRequestManager->DequeueRequest();
	delete request;

	LOG_FN_EXIT(raContext);
	return true;
}

void CRaASClient::eventInputGainOffset(Ias::Int32 const &sink,
		      Ias::Int32 const &gainOffset)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventInputGainOffset sink: "), DLT_INT(sink),
		DLT_STRING(", gainOffset: "), DLT_INT(gainOffset));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
	return;
}

void CRaASClient::eventVolume(Ias::Int32 const &sink,
	     Ias::Int32 const &volume)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventVolume -- sink: "), DLT_INT(sink),
		DLT_STRING(", volume: "), DLT_INT(volume));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
	return;
}

void CRaASClient::eventBalance(Ias::Int32 const &sink,
	      Ias::Int32 const &position)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventBalance -- sink: "), DLT_INT(sink),
		DLT_STRING(", position: "), DLT_INT(position));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventFader(Ias::Int32 const &sink,
	    Ias::Int32 const &position)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventFader -- sink: "), DLT_INT(sink),
		DLT_STRING(", position"), DLT_INT(position));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventMuteState(Ias::Int32 const &sink,
		IasAudioProcessing::IasMuteState const &muteState)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventMuteState -- sink: "), DLT_INT(sink),
		DLT_STRING(", muteState: "), DLT_INT(muteState));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventEqualizer(Ias::Int32 const &sink,
		Ias::UInt32 const &band,
		Ias::Int32 const &gain)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventEqualize -- sink: "), DLT_INT(sink),
		DLT_STRING(", band: "), DLT_INT(band),
		DLT_STRING(", gain: "), DLT_INT(gain));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventLoudness(Ias::Int32 const &sink,
	       Ias::Bool const &active)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventLoudness -- sink: "), DLT_INT(sink),
		DLT_STRING(", active: "), DLT_INT(active));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventSpeedControlledVolume(Ias::Int32 const &sink,
				    Ias::UInt32 const &mode)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventSpeedControlledVolume -- sink: "), DLT_INT(sink),
		DLT_STRING(", mode: "), DLT_INT(mode));

#if NO_EVENT_WORK_AROUND
	LOG_ERROR(raContext, DLT_STRING("should not come here ..! There is an  event. NO_EVENT_WORK_AROUND should be 0 then"));
#else
	SendAckToAM(E_OK);
#endif
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventEC_NR_Mode(Ias::Int32 const &mode)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventEC_NR_Mode -- mode:"), DLT_INT(mode));
	// TODO: handle the event
	LOG_FN_EXIT(raContext);
}

void CRaASClient::eventProperty(Ias::Int32 const &moduleId,
		       IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
		       IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
		       IasAudioProcessing::IasStringPropertyVector const &stringProperties,
		       IasAudioProcessing::IasBufferPropertyVector const &bufferProperties)
{
	(void) moduleId;
	(void) int32Properties;
	(void) float32Properties;
	(void) stringProperties;
	(void) bufferProperties;
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received eventProperty "));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseGetProperty(IasAudioProcessing::IasResponseCode const &code,
			     Ias::Int32 const &moduleId,
			     IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
			     IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
			     IasAudioProcessing::IasStringPropertyVector const &stringProperties,
			     IasAudioProcessing::IasBufferPropertyVector const &bufferProperties)
{
	(void) code;
	(void) moduleId;
	(void) int32Properties;
	(void) float32Properties;
	(void) stringProperties;
	(void) bufferProperties;
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" Received responseGetProperty "));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
			       Ias::Int32 const &deviceHandle,
			       Ias::UInt32 const &alsaCard,
			       Ias::UInt32 const &alsaDevice,
			       Ias::UInt32 const &numberChannels,
			       IasAudioSetup::IasAlsaDeviceType const &deviceType,
			       Ias::Int32 const &id,
			       Ias::Bool const &isSynchronousToSystemClock,
			       IasAudioSetup::IasSampleFormat const &sampleFormat,
			       Ias::UInt32 const &sampleRate)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING(" code: "), DLT_INT(code),
			DLT_STRING(" deviceHandle: "), DLT_UINT32(deviceHandle),
			DLT_STRING(" alsaCard: "), DLT_UINT32(alsaCard),
			DLT_STRING(" alsaDevice: "), DLT_UINT32(alsaDevice),
			DLT_STRING(" numberChannels: "), DLT_UINT32(numberChannels),
			DLT_STRING(" deviceType: "), DLT_INT32(deviceType),
			DLT_STRING(" id: "), DLT_INT32(id),
			DLT_STRING(" isSynchronousToSystemClock: "), DLT_BOOL(isSynchronousToSystemClock),
			DLT_STRING(" sampleFormat: "), DLT_INT32(sampleFormat),
			DLT_STRING(" sampleRate: "), DLT_UINT32(sampleRate));

	Request         *request = NULL;
	request = mRequestManager->PeekRequestQueue();
	am::am_Error_e status = am::E_UNKNOWN;
	AlsaParams_s alsaParams;
	alsaParams.alsaCard = alsaCard;
	alsaParams.alsaDevice = alsaDevice;
	alsaParams.asId = id;
	alsaParams.isSyncToSysClk = isSynchronousToSystemClock;
	alsaParams.nrChannels = numberChannels;
	alsaParams.sampleFormat = sampleFormat;
	alsaParams.sampleRate = sampleRate;
	
	assert(mHotplugSender != NULL);

	if (request->type == eREQUEST_TYPE_ADD_HOTPLUG_SOURCE)
	{
		am::am_sourceID_t sourceID = 0;

		if (code == IasAudioSetup::eIasNoError)
		{
			SourceInfo_t	sourceInfo;
			status = mAmClient->registerSource(*(request->data.source), sourceID);
			if (status == am::E_OK)
			{
				sourceInfo.as_source_id = id;
				sourceInfo.as_source_handle = deviceHandle;
				mContextManager->SetSourceMap(sourceID, sourceInfo);
				mContextManager->SetHotplugSource(sourceID);
				mContextManager->SetSourceALSAParams(sourceID, alsaParams);
			}
		}
		else
		{
			status = am::E_UNKNOWN;
		}

		request->data.source->sourceID = sourceID;
		mHotplugSender->ackRegisterSource(request->data.source->name, sourceID, status);
	}
	else if (request->type == eREQUEST_TYPE_ADD_HOTPLUG_SINK)
	{
		am::am_sinkID_t sinkID = 0;

		if (code == IasAudioSetup::eIasNoError)
		{
			SinkInfo_t	sinkInfo;
			status = mAmClient->registerSink(*(request->data.sink), sinkID);
			if (status == am::E_OK)
			{
				sinkInfo.as_sink_id = id;
				sinkInfo.as_sink_handle = deviceHandle;
				mContextManager->SetSinkMap(sinkID, sinkInfo);
				mContextManager->SetHotplugSink(sinkID);
				mContextManager->SetSinkALSAParams(sinkID, alsaParams);
			}
		}
		else
		{
			status = am::E_UNKNOWN;
		}
		request->data.sink->sinkID = sinkID;
		mHotplugSender->ackRegisterSink(request->data.sink->name, sinkID, status);
	}
	else if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS)
	{
		if (code == IasAudioSetup::eIasNoError)
		{

			LOG_DEBUG(raContext, DLT_STRING("ALSA sink re-created"));
			// update new device handle and as id for the sink in context
			mContextManager->SetSinkAsID(request->data.sinkUpdateData->sinkID, id);
			mContextManager->SetSinkAsHandle(request->data.sinkUpdateData->sinkID, deviceHandle);

			ias_ufipc_result ipcResult;
			if (mContextManager->IsConnectedSink(request->data.sinkUpdateData->sinkID))
			{
				am_sourceID_t sourceID = mContextManager->GetConnectedSourceIDFromSinkID(request->data.sinkUpdateData->sinkID);
				Ias::UInt32 asSourceID = mContextManager->GetAsSourceIDFromAmSourceID(sourceID);
				Ias::UInt32 asSinkID = mContextManager->GetAsSinkIDFromAmSinkID(request->data.sinkUpdateData->sinkID);

				LOG_DEBUG(raContext, DLT_STRING("send reconnection request callRequestConnection"));
				ipcResult = callRequestConnection(asSourceID, asSinkID);
				if (IAS_UFIPC_FAILED(ipcResult)) {
			 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callRequestConnection sourceID: "), DLT_UINT32(asSourceID),
			 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
					status = am::E_UNKNOWN;
					mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, status);
					LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" UFIPC failed"));
			 	}
			 	else
			 	{
			 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callRequestConnection sourceID: "), DLT_UINT32(asSourceID),
			 		 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
					status = am::E_OK;
			 	}
			}
			else
			{
				// update am
				 status = mAmClient->updateSink(request->data.sinkUpdateData->sinkID,
						request->data.sinkUpdateData->sinkClassID,
						request->data.sinkUpdateData->listSoundProperties,
						request->data.sinkUpdateData->listConnectionFormats,
						request->data.sinkUpdateData->listMainSoundProperties);

				LOG_DEBUG(raContext, DLT_STRING("updateSink RA -> AM sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));
				// the internal sequence for the ALSA parameter change is complete for unconnected sink
				// send ack to Hot Plug App
				mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, status);
				LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" status: "), DLT_INT(status));
				// the request should be removed and deleted
				mRequestManager->DequeueRequest();
				delete request->data.sinkUpdateData;
				delete request;
			}
		}
		else
		{
			status = am::E_UNKNOWN;
		}
	}
	else if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS)
	{
		if (code == IasAudioSetup::eIasNoError)
		{
			LOG_DEBUG(raContext, DLT_STRING("ALSA source re-created"));
			// update new device handle and as id for the sink in context
			mContextManager->SetSourceAsID(request->data.sourceUpdateData->sourceID, id);
			mContextManager->SetSourceAsHandle(request->data.sourceUpdateData->sourceID, deviceHandle);

			ias_ufipc_result ipcResult;
			if (mContextManager->IsConnectedSource(request->data.sourceUpdateData->sourceID))
			{
				am_sinkID_t sinkID = mContextManager->GetConnectedSinkIDFromSourceID(request->data.sourceUpdateData->sourceID);
				Ias::UInt32 asSourceID = mContextManager->GetAsSourceIDFromAmSourceID(request->data.sourceUpdateData->sourceID);
				Ias::UInt32 asSinkID = mContextManager->GetAsSinkIDFromAmSinkID(sinkID);

				LOG_DEBUG(raContext, DLT_STRING("send reconnection request callRequestConnection"));
				ipcResult = callRequestConnection(asSourceID, asSinkID);
				if (IAS_UFIPC_FAILED(ipcResult)) {
			 		LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callRequestConnection sourceID: "), DLT_UINT32(asSourceID),
			 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
					status = am::E_UNKNOWN;
					mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, status);
					LOG_DEBUG(raContext, DLT_STRING("ackUpdateSource RA -> Hot Plug App sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" UFIPC failed"));
			 	}
			 	else
			 	{
			 		LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callRequestConnection sourceID: "), DLT_UINT32(asSourceID),
			 		 		 		 				DLT_STRING(" sinkID: "), DLT_UINT32(asSinkID));
					status = am::E_OK;
			 	}
			}
			else
			{
				// update am
				status = mAmClient->updateSource(request->data.sourceUpdateData->sourceID,
						request->data.sourceUpdateData->sourceClassID,
						request->data.sourceUpdateData->listSoundProperties,
						request->data.sourceUpdateData->listConnectionFormats,
						request->data.sourceUpdateData->listMainSoundProperties);
				LOG_DEBUG(raContext, DLT_STRING("updateSource RA -> AM sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));
				// the internal sequence for the ALSA parameter change is complete for unconnected source
				// send ack to Hot Plug App
				mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, status);
				LOG_DEBUG(raContext, DLT_STRING("ackUpdateSource RA -> Hot Plug App sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" status: "), DLT_INT(status));
				// the request should be removed and deleted
				mRequestManager->DequeueRequest();
				delete request->data.sourceUpdateData;
				delete request;
			}
		}
		else
		{
			status = am::E_UNKNOWN;
		}
	}
	else
	{
		LOG_ERROR(raContext, DLT_STRING("CRaASClient::responseAddAlsaDevice should not come here ..!"));
	}

	if (request->type == eREQUEST_TYPE_ADD_HOTPLUG_SOURCE
		|| request->type == eREQUEST_TYPE_ADD_HOTPLUG_SINK)
	{
		mRequestManager->DequeueRequest();
		delete request;
	}

	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
			       Ias::Int32 const &deviceHandle,
			       Ias::String const &alsaName,
			       Ias::UInt32 const &numberChannels,
			       IasAudioSetup::IasAlsaDeviceType const &deviceType,
			       Ias::Int32 const &id)
{
	(void) code;
	(void) deviceHandle;
	(void) alsaName;
	(void) numberChannels;
	(void) deviceType;
	(void) id;
	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING("@1 CRaASClient::responseAddAlsaDevice should not come here ..!"));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseStatusAudioSetup(IasAudioSetup::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext);
	LOG_INFO(raContext, DLT_STRING("CRaASClient::responseStatusAudioSetup code: "),
			DLT_INT32(code));

	assert(mHotplugSender != NULL);

	Request         *request = NULL;
	request = mRequestManager->PeekRequestQueue();
	am::am_Error_e status = am::E_UNKNOWN;
	if (request->type == eREQUEST_TYPE_REMOVE_HOTPLUG_SOURCE)
	{

		if (code == IasAudioSetup::eIasNoError)
		{
			status = mAmClient->deregisterSource(request->data.sourceID);
		}
		else
		{
			status = am::E_UNKNOWN;
		}

		mHotplugSender->ackDeregisterSource(request->data.sourceID, status);
	}
	else if (request->type == eREQUEST_TYPE_REMOVE_HOTPLUG_SINK)
	{
		if (code == IasAudioSetup::eIasNoError)
		{
			status = mAmClient->deregisterSink(request->data.sinkID);
		}
		else
		{
			status = am::E_UNKNOWN;
		}
		mHotplugSender->ackDeregisterSink(request->data.sinkID, status);
	}
	else if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SINK_ALSA_PARAMS)
	{
		LOG_DEBUG(raContext, DLT_STRING("ALSA sink deleted"));
		AlsaParams_s alsaParams;
		mContextManager->GetSinkALSAParams(request->data.sinkUpdateData->sinkID, alsaParams);
		LOG_DEBUG(raContext, DLT_STRING("send request callAddAlsaDevice"));
		ias_ufipc_result ipcResult = callAddAlsaDevice(alsaParams.alsaCard, alsaParams.alsaDevice, alsaParams.nrChannels, IasAudioSetup::eIasSink, alsaParams.asId, alsaParams.isSyncToSysClk, alsaParams.sampleFormat, alsaParams.sampleRate);
		if (IAS_UFIPC_FAILED(ipcResult)) {
			LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callAddAlsaDevice SinkID: "), DLT_UINT16(request->data.sinkUpdateData->sinkID));
			mHotplugSender->ackUpdateSink(request->data.sinkUpdateData->sinkID, am::E_UNKNOWN);
			LOG_DEBUG(raContext, DLT_STRING("ackUpdateSink RA -> Hot Plug App sinkID: "), DLT_INT(request->data.sinkUpdateData->sinkID), DLT_STRING(" UFIPC failed"));
		}
		else
		{
			LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callAddAlsaDevice SinkID: "), DLT_UINT16(request->data.sinkUpdateData->sinkID));
		}
	}
	else if (request->type == eREQUEST_TYPE_UPDATE_HOTPLUG_SOURCE_ALSA_PARAMS)
	{
		if (code == IasAudioSetup::eIasNoError)
		{
			LOG_DEBUG(raContext, DLT_STRING("ALSA source deleted"));
			AlsaParams_s alsaParams;
			mContextManager->GetSourceALSAParams(request->data.sourceUpdateData->sourceID, alsaParams);
			LOG_DEBUG(raContext, DLT_STRING("send request callAddAlsaDevice"));
			ias_ufipc_result ipcResult = callAddAlsaDevice(alsaParams.alsaCard, alsaParams.alsaDevice, alsaParams.nrChannels, IasAudioSetup::eIasSource, alsaParams.asId, alsaParams.isSyncToSysClk, alsaParams.sampleFormat, alsaParams.sampleRate);
			if (IAS_UFIPC_FAILED(ipcResult)) {
				LOG_ERROR(raContext, DLT_STRING("UFIPC [failed] callAddAlsaDevice SourceID: "), DLT_UINT16(request->data.sourceUpdateData->sourceID));
				mHotplugSender->ackUpdateSource(request->data.sourceUpdateData->sourceID, am::E_UNKNOWN);
				LOG_DEBUG(raContext, DLT_STRING("ackUpdateSource RA -> Hot Plug App sourceID: "), DLT_INT(request->data.sourceUpdateData->sourceID), DLT_STRING(" UFIPC failed"));
			}
			else
			{
				LOG_DEBUG(raContext, DLT_STRING("UFIPC [success] callAddAlsaDevice SourceID: "), DLT_UINT16(request->data.sourceUpdateData->sourceID));
			}

		}
		else
		{
			status = am::E_UNKNOWN;
		}
	}
	else
	{
		LOG_ERROR(raContext, DLT_STRING("CRaASClient::responseAddAlsaDevice should not come here ..!"));
	}

	if (request->type == eREQUEST_TYPE_REMOVE_HOTPLUG_SOURCE
		|| request->type == eREQUEST_TYPE_REMOVE_HOTPLUG_SINK)
	{
		mRequestManager->DequeueRequest();
		delete request;
	}
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
			       Ias::Int32 const &deviceHandle,
			       Ias::UInt32 const &alsaCard,
			       Ias::UInt32 const &alsaDevice,
			       Ias::UInt32 const &numberChannels,
			       IasAudioSetup::IasAlsaDeviceType const &deviceType,
			       Ias::Int32 const &id,
			       Ias::Bool const &isSynchronousToSystemClock,
			       IasAudioSetup::IasSampleFormat const &sampleFormat,
			       Ias::UInt32 const &sampleRate,
			       Ias::Bool const &addDownmix,
			       Ias::Bool const &addSid)
{
	(void) code;
	(void) deviceHandle;
	(void) alsaCard;
	(void) alsaDevice;
	(void) numberChannels;
	(void) deviceType;
	(void) id;
	(void) isSynchronousToSystemClock;
	(void) sampleFormat;
	(void) sampleRate;
	(void) addDownmix;
	(void) addSid;

	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING("@2 CRaASClient::responseAddAlsaDevice should not come here ..!"));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseGetDeviceHandle(IasAudioSetup::IasResponseCode const &code,
				 Ias::Int32 const &deviceHandle,
				 IasAudioSetup::IasAlsaDeviceType const &deviceType)
{
	(void) code;
	(void) deviceHandle;
	(void) deviceType;
	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING("@3 CRaASClient::responseGetDeviceHandle should not come here ..!"));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
			       Ias::Int32 const &deviceHandle,
			       Ias::String const &alsaName,
			       Ias::UInt32 const &numberChannels,
			       IasAudioSetup::IasAlsaDeviceType const &deviceType,
			       Ias::Int32 const &id,
			       Ias::Bool const &isSynchronousToSystemClock,
			       IasAudioSetup::IasSampleFormat const &sampleFormat,
			       Ias::UInt32 const &sampleRate,
			       Ias::Bool const &addDownmix,
			       Ias::Bool const &addSid)
{
	(void) code;
	(void) deviceHandle;
	(void) alsaName;
	(void) numberChannels;
	(void) deviceType;
	(void) id;
	(void) isSynchronousToSystemClock;
	(void) sampleFormat;
	(void) sampleRate;
	(void) addDownmix;
	(void) addSid;
	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING("@4 CRaASClient::responseAddAlsaDevice should not come here ..!"));
	LOG_FN_EXIT(raContext);
}

void CRaASClient::responseGetCurrentSidfromStream(IasAudioSetup::IasResponseCode const &code,
					 Ias::Int32 const &streamId,
					 Ias::UInt32 const &currentSid)
{
	(void) code;
	(void) streamId;
	(void) currentSid;
	LOG_FN_ENTRY(raContext);
	LOG_ERROR(raContext, DLT_STRING("@5 CRaASClient::responseGetCurrentSidfromStream should not come here ..!"));
	LOG_FN_EXIT(raContext);
}

bool CRaASClient::IsDelayed(void)
{
	LOG_FN_ENTRY(raContext);
	if (mDelayedRequestCount > 0)
	{
		LOG_FN_EXIT(raContext, DLT_STRING("delayed=yes"));
		return true;
	}
	else
	{
		LOG_FN_EXIT(raContext, DLT_STRING("delayed=no"));
		return false;
	}
}
void CRaASClient::IncreamentDelayedRequestCount()
{
	LOG_FN_ENTRY(raContext);
	mDelayedRequestCount++;
	LOG_FN_EXIT(raContext, DLT_STRING(" delayed request count: "), DLT_INT32(mDelayedRequestCount));
}

void CRaASClient::DecrementDelayedRequestCount()
{
	LOG_FN_ENTRY(raContext);
	if(mDelayedRequestCount > 0)
	{
		mDelayedRequestCount--;
	}
	else
	{
		mDelayedRequestCount = 0;
	}
	LOG_FN_EXIT(raContext, DLT_STRING(" delayed request count: "), DLT_INT32(mDelayedRequestCount));
}
