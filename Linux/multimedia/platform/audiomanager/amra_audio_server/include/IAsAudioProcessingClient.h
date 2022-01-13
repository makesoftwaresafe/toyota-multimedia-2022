/************************************************************************
* @file: IAsAudioProcessingClient.h
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
#ifndef __IAS_AUDIO_PROCESSING_CLIENT_H__
#define __IAS_AUDIO_PROCESSING_CLIENT_H__
#include "core_libraries/foundation/IasTypes.hpp"
#include "audio/rpcontroller/idl/IasAudioProcessingStub.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicationChannel.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicator.hpp"
#include "core_libraries/foundation/IasSignalHandler.hpp"
#include "core_libraries/foundation/IasThread.hpp"

class IAsAudioProcessingClient : public IasAudioProcessing::IasAudioProcessingStub
{
public:
	IAsAudioProcessingClient();
	virtual ~IAsAudioProcessingClient();
	void errorEvent(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId);
	void responseStatus(IasAudioProcessing::IasResponseCode const &code);
	virtual void errorEventAudioProcessing(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId) = 0;
	virtual void responseStatusAudioProcessing(IasAudioProcessing::IasResponseCode const &code) = 0;
	virtual void eventInputGainOffset(Ias::Int32 const &sink,
				      Ias::Int32 const &gainOffset) = 0;   
	virtual void eventVolume(Ias::Int32 const &sink,
			     Ias::Int32 const &volume) = 0;   
	virtual void eventBalance(Ias::Int32 const &sink,
			      Ias::Int32 const &position) = 0;   
	virtual void eventFader(Ias::Int32 const &sink,
			    Ias::Int32 const &position) = 0;   
	virtual void eventMuteState(Ias::Int32 const &sink,
				IasAudioProcessing::IasMuteState const &muteState) = 0;   
	virtual void eventEqualizer(Ias::Int32 const &sink,
				Ias::UInt32 const &band,
				Ias::Int32 const &gain) = 0;   
	virtual void eventLoudness(Ias::Int32 const &sink,
			       Ias::Bool const &active) = 0;   
	virtual void eventSpeedControlledVolume(Ias::Int32 const &sink,
					    Ias::UInt32 const &mode) = 0;
	virtual void eventEC_NR_Mode(Ias::Int32 const &mode) = 0;
	virtual void eventProperty(Ias::Int32 const &moduleId,
			       IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
			       IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
			       IasAudioProcessing::IasStringPropertyVector const &stringProperties,
			       IasAudioProcessing::IasBufferPropertyVector const &bufferProperties) = 0;   
	virtual void responseGetProperty(IasAudioProcessing::IasResponseCode const &code,
				     Ias::Int32 const &moduleId,
				     IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
				     IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
				     IasAudioProcessing::IasStringPropertyVector const &stringProperties,
				     IasAudioProcessing::IasBufferPropertyVector const &bufferProperties) = 0;  
private:
	Ias::IasUfipcCommunicationChannel 	mChannel;
	Ias::IasUfipcCommunicator		mCommunicator;
	Ias::IasThread				*mCommunicatorThread;
};
#endif //__IAS_AUDIO_PROCESSING_CLIENT_H__
