/************************************************************************
* @file: IAsAudioProcessingClient.cpp
*
* @version: 1.1
*
* IAsAudioProcessingClient class implements responseStatus and errorEvent interfaces
* of the IAsAudioProcessing interface
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
#include <iostream>
#include "IAsAudioProcessingClient.h"
#include "Log.h"

using namespace std;

DLT_IMPORT_CONTEXT(raContext);

IAsAudioProcessingClient::IAsAudioProcessingClient()
	: IasAudioProcessing::IasAudioProcessingStub("KPRA_ProcessingStub", "ias.audio.daemon")

{
	LOG_FN_ENTRY(raContext);
	Ias::String const channelName  = "KP-Bus";
	ias_ufipc_result ipcResult;

	ipcResult = mChannel.init(channelName);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IPC channel for Audio Processing"));
		return;
	}
	ipcResult = mCommunicator.init(mChannel);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IPC communicatior for channel for Audio Processing"));
		return;
	}

	mCommunicatorThread = new Ias::IasThread(&mCommunicator);

	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING(" failed to allocate communicator thread"));
		return;
	}

	if (Ias::IAS_FAILED(mCommunicatorThread->start(true))) {
		LOG_ERROR(raContext, DLT_STRING(" could not start UFIPC communication thread for Audio Processing"));
		return;
	}

	ipcResult = IasAudioProcessing::IasAudioProcessingStub::init(&mCommunicator);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IasAudioProcessing::IasAudioProcessingStub"));
		return;
	}
	
	LOG_DEBUG(raContext, DLT_STRING("[IAsAudioProcessingClient::IAsAudioProcessingClient] -- UFIPC initialization complete !"));
	LOG_FN_EXIT(raContext);
}

IAsAudioProcessingClient:: ~IAsAudioProcessingClient()
{
	LOG_FN_ENTRY(raContext);
	IasAudioProcessing::IasAudioProcessingStub::cleanup();
	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING("NULL == mCommunicatorThread"));
		return;
	}

	mCommunicatorThread->stop();
	delete mCommunicatorThread;
	mCommunicatorThread = NULL;
	LOG_FN_EXIT(raContext);
}

void IAsAudioProcessingClient::errorEvent(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId)
{
	LOG_FN_ENTRY(raContext);
	errorEventAudioProcessing(communicationResult, calledFunctionId);
	LOG_FN_EXIT(raContext);
}

void IAsAudioProcessingClient::responseStatus(IasAudioProcessing::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext);
	responseStatusAudioProcessing(code);
	LOG_FN_EXIT(raContext);
}
