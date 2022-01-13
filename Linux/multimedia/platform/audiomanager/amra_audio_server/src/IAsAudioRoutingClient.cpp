/************************************************************************
* @file: IAsAudioRoutingClient.cpp
*
* @version: 1.1
*
* IAsAudioRoutingClient class implements responseStatus and errorEvent callbacks
* of the IasAudioRouting interface
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
#include "IAsAudioRoutingClient.h"
#include "Log.h"

using namespace std;

DLT_IMPORT_CONTEXT(raContext);

IAsAudioRoutingClient::IAsAudioRoutingClient()
	: IasAudioRouting::IasAudioRoutingStub("KPRA_RoutingStub", "ias.audio.daemon")
{
	LOG_FN_ENTRY(raContext);
	Ias::String const channelName  = "KP-Bus";
	ias_ufipc_result ipcResult;

	ipcResult = mChannel.init(channelName);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IPC channel for Audo Routing"));
		return;
	}
	ipcResult = mCommunicator.init(mChannel);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IPC communicatior for channel for Audio Routing"));
		return;
	}

	mCommunicatorThread = new Ias::IasThread(&mCommunicator);

	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING(" failed to allocate communicator thread"));
		return;
	}

	if (Ias::IAS_FAILED(mCommunicatorThread->start(true))) {
		LOG_ERROR(raContext, DLT_STRING(" could not start UFIPC communication thread for Audio Routing"));
		return;
	}

	ipcResult = IasAudioRouting::IasAudioRoutingStub::init(&mCommunicator);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IasAudioRouting::IasAudioRoutingStub"));
		return;
	}

	LOG_DEBUG(raContext, DLT_STRING(" UFIPC initialization complete !"));
	LOG_FN_EXIT(raContext);
}

IAsAudioRoutingClient:: ~IAsAudioRoutingClient()
{
	LOG_FN_ENTRY(raContext);
	IasAudioRouting::IasAudioRoutingStub::cleanup();

	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING("NULL == mCommunicatorThread"));
		return;
	}

	mCommunicatorThread->stop();
	delete mCommunicatorThread;
	LOG_FN_EXIT(raContext);
}

void IAsAudioRoutingClient::errorEvent(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId)
{
	LOG_FN_ENTRY(raContext);
	errorEventAudioRouting(communicationResult, calledFunctionId);
	LOG_FN_EXIT(raContext);
}

void IAsAudioRoutingClient::responseStatus(IasAudioRouting::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext);
	responseStatusAudioRouting(code);
	LOG_FN_EXIT(raContext);
}
