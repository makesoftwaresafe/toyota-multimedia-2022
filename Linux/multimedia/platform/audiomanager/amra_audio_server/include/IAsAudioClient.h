/************************************************************************
* @file: IAsAudioClient.h
*
* @version: 1.1
*
* IAsAudioClient class combines the interfaces of IAsAudioRouting, IAsAudioProcessing and IasAudioSetup
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
#ifndef __I_AS_AUDIO_CLIENT_H__
#define __I_AS_AUDIO_CLIENT_H__
#include "IAsAudioRoutingClient.h"
#include "IAsAudioProcessingClient.h"
#include "IasAudioSetupClient.h"

class IAsAudioClient
: public IAsAudioRoutingClient, public IAsAudioProcessingClient, public IasAudioSetupClient
{
public:
	IAsAudioClient();
	virtual ~IAsAudioClient();
	virtual void errorEventAudioRouting(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId) = 0;
	virtual void responseConnectionList(IasAudioRouting::IasConnectionVector const &connectionVector) = 0;
	virtual void responseStatusAudioRouting(IasAudioRouting::IasResponseCode const &code) = 0;
	virtual void eventConnectionState(Ias::Int32 const &source,
				      Ias::Int32 const &sink,
				      IasAudioRouting::IasConnectionState const &state) = 0;
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
	virtual void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::UInt32 const &alsaCard,
				       Ias::UInt32 const &alsaDevice,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id,
				       Ias::Bool const &isSynchronousToSystemClock,
				       IasAudioSetup::IasSampleFormat const &sampleFormat,
				       Ias::UInt32 const &sampleRate) = 0;

	virtual void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::String const &alsaName,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id) = 0;

	virtual void responseStatusAudioSetup(IasAudioSetup::IasResponseCode const &code) = 0;

	virtual void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
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
				       Ias::Bool const &addSid) = 0;

	virtual void responseGetDeviceHandle(IasAudioSetup::IasResponseCode const &code,
					 Ias::Int32 const &deviceHandle,
					 IasAudioSetup::IasAlsaDeviceType const &deviceType) = 0;

	virtual void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::String const &alsaName,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id,
				       Ias::Bool const &isSynchronousToSystemClock,
				       IasAudioSetup::IasSampleFormat const &sampleFormat,
				       Ias::UInt32 const &sampleRate,
				       Ias::Bool const &addDownmix,
				       Ias::Bool const &addSid) = 0;

	virtual void responseGetCurrentSidfromStream(IasAudioSetup::IasResponseCode const &code,
						 Ias::Int32 const &streamId,
						 Ias::UInt32 const &currentSid) = 0;
};

#endif // __I_AS_AUDIO_CLIENT_H__
