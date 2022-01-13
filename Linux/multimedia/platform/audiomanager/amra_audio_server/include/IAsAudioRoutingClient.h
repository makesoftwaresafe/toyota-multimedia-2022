/************************************************************************
* @file: IAsAudioRoutingClient.h
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
#ifndef __IAS_AUDIO_ROUTING_CLIENT_H__
#define __IAS_AUDIO_ROUTING_CLIENT_H__
#include "core_libraries/foundation/IasTypes.hpp"
#include "audio/rpcontroller/idl/IasAudioRoutingStub.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicationChannel.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicator.hpp"
#include "core_libraries/foundation/IasSignalHandler.hpp"
#include "core_libraries/foundation/IasThread.hpp"

class IAsAudioRoutingClient : public IasAudioRouting::IasAudioRoutingStub
{
public:
	IAsAudioRoutingClient();
	virtual ~IAsAudioRoutingClient();
	void errorEvent(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId);
	void responseStatus(IasAudioRouting::IasResponseCode const &code);
	virtual void errorEventAudioRouting(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId) = 0;
	virtual void responseConnectionList(IasAudioRouting::IasConnectionVector const &connectionVector) = 0;
	virtual void responseStatusAudioRouting(IasAudioRouting::IasResponseCode const &code) = 0;
	virtual void eventConnectionState(Ias::Int32 const &source,
				      Ias::Int32 const &sink,
				      IasAudioRouting::IasConnectionState const &state) = 0;
private:
	Ias::IasUfipcCommunicationChannel 	mChannel;
	Ias::IasUfipcCommunicator		mCommunicator;
	Ias::IasThread				*mCommunicatorThread;
};
#endif //__IAS_AUDIO_ROUTING_CLIENT_H__
