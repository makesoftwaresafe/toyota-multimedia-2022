/************************************************************************
* @file: CRaASClient.h
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
#ifndef __C_RA_AS_CLIENT_H__
#define __C_RA_AS_CLIENT_H__
#include "IAsAudioClient.h"
#include "audiomanagertypes.h"
#include "CContextManager.h"
#include "CRaHotplugSender.h"

using namespace am;

class CRaAMRoutingClient;
class CRaRequestManager;
class CRaConfigManager;

class CRaASClient : public IAsAudioClient
{
public:
	CRaASClient(CRaConfigManager *configManager, CRaRequestManager *requestManager, CRaAMRoutingClient *routingClient, CContextManager *contextManager);
	virtual ~CRaASClient();
	void SetRequestManager(CRaRequestManager *requestManager);
	void SetAMClient(CRaAMRoutingClient *amClient);
	void SetHotplugSender(CRaHotplugSender *hotplugSender);
	void errorEventAudioRouting(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId);
	void responseConnectionList(IasAudioRouting::IasConnectionVector const &connectionVector);
	void responseStatusAudioRouting(IasAudioRouting::IasResponseCode const &code);
	void eventConnectionState(Ias::Int32 const &source,
			      Ias::Int32 const &sink,
			      IasAudioRouting::IasConnectionState const &state);
	void errorEventAudioProcessing(ias_ufipc_result const communicationResult, ias_ufipc_function_id const calledFunctionId);
	void responseStatusAudioProcessing(IasAudioProcessing::IasResponseCode const &code);
	void eventInputGainOffset(Ias::Int32 const &sink,
			      Ias::Int32 const &gainOffset);
	void eventVolume(Ias::Int32 const &sink,
		     Ias::Int32 const &volume);
	void eventBalance(Ias::Int32 const &sink,
		      Ias::Int32 const &position);
	void eventFader(Ias::Int32 const &sink,
		    Ias::Int32 const &position);
	void eventMuteState(Ias::Int32 const &sink,
			IasAudioProcessing::IasMuteState const &muteState);
	void eventEqualizer(Ias::Int32 const &sink,
			Ias::UInt32 const &band,
			Ias::Int32 const &gain);
	void eventLoudness(Ias::Int32 const &sink,
		       Ias::Bool const &active);
	void eventSpeedControlledVolume(Ias::Int32 const &sink,
					    Ias::UInt32 const &mode);
	void eventEC_NR_Mode(Ias::Int32 const &mode);
	void eventProperty(Ias::Int32 const &moduleId,
			       IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
			       IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
			       IasAudioProcessing::IasStringPropertyVector const &stringProperties,
			       IasAudioProcessing::IasBufferPropertyVector const &bufferProperties);
	void responseGetProperty(IasAudioProcessing::IasResponseCode const &code,
				     Ias::Int32 const &moduleId,
				     IasAudioProcessing::IasInt32PropertyVector const &int32Properties,
				     IasAudioProcessing::IasFloat32PropertyVector const &float32Properties,
				     IasAudioProcessing::IasStringPropertyVector const &stringProperties,
				     IasAudioProcessing::IasBufferPropertyVector const &bufferProperties);
	void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::UInt32 const &alsaCard,
				       Ias::UInt32 const &alsaDevice,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id,
				       Ias::Bool const &isSynchronousToSystemClock,
				       IasAudioSetup::IasSampleFormat const &sampleFormat,
				       Ias::UInt32 const &sampleRate);

	void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::String const &alsaName,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id);

	void responseStatusAudioSetup(IasAudioSetup::IasResponseCode const &code);

	void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
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
				       Ias::Bool const &addSid);

	void responseGetDeviceHandle(IasAudioSetup::IasResponseCode const &code,
					 Ias::Int32 const &deviceHandle,
					 IasAudioSetup::IasAlsaDeviceType const &deviceType);

	void responseAddAlsaDevice(IasAudioSetup::IasResponseCode const &code,
				       Ias::Int32 const &deviceHandle,
				       Ias::String const &alsaName,
				       Ias::UInt32 const &numberChannels,
				       IasAudioSetup::IasAlsaDeviceType const &deviceType,
				       Ias::Int32 const &id,
				       Ias::Bool const &isSynchronousToSystemClock,
				       IasAudioSetup::IasSampleFormat const &sampleFormat,
				       Ias::UInt32 const &sampleRate,
				       Ias::Bool const &addDownmix,
				       Ias::Bool const &addSid);

	void responseGetCurrentSidfromStream(IasAudioSetup::IasResponseCode const &code,
						 Ias::Int32 const &streamId,
						 Ias::UInt32 const &currentSid);
	bool IsDelayed(void);
	void IncreamentDelayedRequestCount();
	void DecrementDelayedRequestCount();
private:
	/* we do not want to instanciate the object with default constructor */
	CRaASClient();
	bool SendAckToAM(am_Error_e error);
private:
	CRaAMRoutingClient 		*mAmClient;
	CRaConfigManager 		*mConfigManager;
	CRaRequestManager 		*mRequestManager;
	unsigned int			mDelayedRequestCount;
	CContextManager			*mContextManager;
	CRaHotplugSender		*mHotplugSender;
};
#endif //__C_RA_AS_CLIENT_H__
