/************************************************************************
* @file: IasAudioSetupClient.cpp
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
#include "IasAudioSetupClient.h"
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext);

IasAudioSetupClient::IasAudioSetupClient()
	: IasAudioSetup::IasAudioSetupStub("KPRA_SetupStub", "ias.audio.daemon")
{
	LOG_FN_ENTRY(raContext);
	Ias::String const channelName  = "KP-Bus";
	ias_ufipc_result ipcResult;

	ipcResult = mChannel.init(channelName);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING("[IasAudioSetupClient::IasAudioSetupClient] -- could not init IPC channel for Audo Setup"));
		return;
	}
	ipcResult = mCommunicator.init(mChannel);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING("[IasAudioSetupClient::IasAudioSetupClient] -- could not init IPC communicatior for channel for Audio Setup"));
		return;
	}

	mCommunicatorThread = new Ias::IasThread(&mCommunicator);

	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING("[IasAudioSetupClient::IasAudioSetupClient] -- failed to allocate communicator thread"));
		return;
	}

	if (Ias::IAS_FAILED(mCommunicatorThread->start(true))) {
		LOG_ERROR(raContext, DLT_STRING("[IasAudioSetupClient::IasAudioSetupClient] -- could not start UFIPC communication thread for Audio Setup"));
		return;
	}

	ipcResult = IasAudioSetup::IasAudioSetupStub::init(&mCommunicator);
	if (IAS_UFIPC_FAILED(ipcResult)) {
		LOG_ERROR(raContext, DLT_STRING(" could not init IasAudioSetup::IasAudioSetupStub"));
		return;
	}

	LOG_DEBUG(raContext, DLT_STRING(" UFIPC initialization complete !"));
	LOG_FN_EXIT(raContext);
}

IasAudioSetupClient::~IasAudioSetupClient()
{
	LOG_FN_ENTRY(raContext);
	IasAudioSetup::IasAudioSetupStub::cleanup();

	if (NULL == mCommunicatorThread)
	{
		LOG_ERROR(raContext, DLT_STRING("NULL == mCommunicatorThread"));
		return;
	}

	mCommunicatorThread->stop();
	delete mCommunicatorThread;
	LOG_FN_EXIT(raContext);
}

void IasAudioSetupClient::responseStatus(IasAudioSetup::IasResponseCode const &code)
{
	LOG_FN_ENTRY(raContext);
	responseStatusAudioSetup(code);
	LOG_FN_EXIT(raContext);
}
