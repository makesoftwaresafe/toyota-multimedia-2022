/************************************************************************
* @file: IasAudioSetupClient.h
*
* @version: 1.1
*
* IAsAudioRoutingClient class implements responseStatus callback
* of the IasAudioSetup interface
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
#ifndef __IAS_AUDIO_SETUP_CLIENT_H__
#define __IAS_AUDIO_SETUP_CLIENT_H__

#include "core_libraries/foundation/IasTypes.hpp"
#include "audio/rpcontroller/idl/IasAudioSetupStub.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicationChannel.hpp"
#include "systembus/ufipc_helper/IasUfipcCommunicator.hpp"
#include "core_libraries/foundation/IasSignalHandler.hpp"
#include "core_libraries/foundation/IasThread.hpp"

class IasAudioSetupClient : public IasAudioSetup::IasAudioSetupStub
{
public:
    IasAudioSetupClient();
    virtual ~IasAudioSetupClient();
    void responseStatus(IasAudioSetup::IasResponseCode const &code);

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
private:
	Ias::IasUfipcCommunicationChannel 	mChannel;
	Ias::IasUfipcCommunicator		mCommunicator;
	Ias::IasThread				*mCommunicatorThread;
};

#endif //__IAS_AUDIO_SETUP_CLIENT_H__
