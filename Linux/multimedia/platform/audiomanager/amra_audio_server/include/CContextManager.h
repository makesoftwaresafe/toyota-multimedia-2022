/************************************************************************
* @file: CContextManager.h
*
* @version: 1.1
*
* CContextManager class maintains the internal state of the rouing manager.
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
#ifndef __C_CONTEXT_MANAGER_H__
#define __C_CONTEXT_MANAGER_H__

#include <audiomanagertypes.h>
#include "core_libraries/foundation/IasTypes.hpp"
#include "audio/rpcontroller/idl/IasAudioProcessingStub.hpp"
#include "audio/rpcontroller/idl/IasAudioSetupStub.hpp"

using namespace std;
using namespace am;

struct Connection_t {
	Connection_t()
	{
		sink = 0;
		source = 0;
		internallyDisconnected = false;
	}
	am_sinkID_t	sink;
	am_sourceID_t	source;
	bool			internallyDisconnected;
};


struct AlsaParams_s {
	Ias::UInt32 alsaCard;
	Ias::UInt32 alsaDevice;
	Ias::UInt32 nrChannels;
	Ias::UInt32 asId;
	Ias::Bool isSyncToSysClk;
	IasAudioSetup::IasSampleFormat sampleFormat;
	Ias::UInt32 sampleRate;
};

struct SinkInfo_t {
	SinkInfo_t()
	{
		as_sink_id = 0;
		as_sink_handle = 0;
		volume = 0;
		mutestate = 0;
		balance = 0;
		fader = 0;
		treble = 0;
		bass = 0;
		equalizer = 0;
		loudness = 0;
		scv = 0;
		inputgainoffset = 0;
		as_sink_alsa_params.alsaCard = 0;
		as_sink_alsa_params.alsaDevice = 0;
		as_sink_alsa_params.asId = 0;
		as_sink_alsa_params.isSyncToSysClk = false;
		as_sink_alsa_params.nrChannels = 0;
		as_sink_alsa_params.sampleFormat = IasAudioSetup::eIasUnknown;
		as_sink_alsa_params.sampleRate = 0;
		isHotplugSink = false;
		isSinkOwnerRouter = false;
	}

	Ias::Int32	as_sink_id;
	Ias::Int32	as_sink_handle;
	AlsaParams_s as_sink_alsa_params;
	am_volume_t	volume;
	bool		mutestate;
	int16_t		balance;
	int16_t		fader;
	int16_t		treble;
	int16_t		bass;
	int16_t		equalizer;
	bool		loudness;
	bool		scv;
	int16_t		inputgainoffset;
	bool 		isHotplugSink;
	bool		isSinkOwnerRouter;
};

struct SourceInfo_t {
	SourceInfo_t()
	{
		as_source_id = 0;
		as_source_handle = 0;
		as_source_alsa_params.alsaCard = 0;
		as_source_alsa_params.alsaDevice = 0;
		as_source_alsa_params.asId = 0;
		as_source_alsa_params.isSyncToSysClk = false;
		as_source_alsa_params.nrChannels = 0;
		as_source_alsa_params.sampleFormat = IasAudioSetup::eIasUnknown;
		as_source_alsa_params.sampleRate = 0;
		isHotplugSource = false;
		isSourceOwnerRouter = false;
	}

	Ias::Int32      as_source_id;
	Ias::Int32      as_source_handle;
	AlsaParams_s    as_source_alsa_params;
	bool			isHotplugSource;
	bool			isSourceOwnerRouter;
	//TODO: add if new source dynamic data identified.
};

struct GatewayInfo_t
{
	GatewayInfo_t()
	{
		isGatewayOwnerRouter = false;
	}

	bool		isGatewayOwnerRouter;
};

class CContextManager
{
public:
	CContextManager();
	~CContextManager();
	void SetDomainID(const am_domainID_t &domainID);
	am_domainID_t GetDomainID(void);
	Ias::Int32 GetAsSourceIDFromAmSourceID(const am_sourceID_t sourceID);
	Ias::Int32 GetAsSourceHandleFromAmSourceID(const am_sourceID_t sourceID);
	void SetSourceAsID(const am_sourceID_t sourceID, const Ias::Int32 asID);
	void SetSourceAsHandle(const am_sourceID_t sourceID, const Ias::Int32 asHandle);
	void SetSourceMap(const am_sourceID_t sourceID, const SourceInfo_t srcInfo);
	Ias::Int32 GetAsSinkIDFromAmSinkID(const am_sinkID_t sinkID);
	Ias::Int32 GetAsSinkHandleFromAmSinkID(const am_sinkID_t sinkID);
	void SetSinkAsID(const am_sinkID_t sinkID, const Ias::Int32 asID);
	void SetSinkAsHandle(const am_sinkID_t sinkID, const Ias::Int32 asHandle);
	void SetSinkMap(const am_sinkID_t sinkID, const SinkInfo_t sinkInfo);
	void GetConnectionData(const am_connectionID_t connectionID, am_sourceID_t &sourceID, am_sinkID_t &sinkID);
	void SetConnectionData(const am_connectionID_t connectionID, const am_sourceID_t sourceID, const am_sinkID_t sinkID);
	void RemoveConnectionData(const am_connectionID_t connectionID);
	bool IsConnectedSink(const am_sinkID_t sinkID);
	bool IsConnectedSource(const am_sourceID_t sourceID);
	am_sourceID_t GetConnectedSourceIDFromSinkID(const am_sinkID_t sinkID);
	am_sinkID_t GetConnectedSinkIDFromSourceID(const am_sourceID_t sourceID);
	void SetSourceInterallyDisconnected(const am_sourceID_t sourceID);
	void SetSourceInterallyConnected(const am_sourceID_t sourceID);
	bool IsSourceInternallyDisconnected(const am_sourceID_t sourceID);
	void SetSinkInterallyDisconnected(const am_sinkID_t sinkID);
	void SetSinkInterallyConnected(const am_sinkID_t sinkID);
	bool IsSinkInternallyDisconnected(const am_sinkID_t sinkID);

	void SetVolumeData(const am_sinkID_t sinkID, const am_volume_t volume);
	void SetMuteStateData(const am_sinkID_t sinkID, const bool muteState);
	void SetBalanceData(const am_sinkID_t sinkID, const int16_t balance);
	void SetFaderData(const am_sinkID_t sinkID, const int16_t fader);
	void SetEqualizationData(const am_sinkID_t sinkID, const uint16_t band, const int16_t gain);
	void SetLoudnessData(const am_sinkID_t sinkID, const bool loudness);
	void SetSCVData(const am_sinkID_t sinkID, const bool scv);
	void SetInputGainOffset(const am_sinkID_t sinkID, const int16_t inputgainoffset);
	void SetHotplugSource(const am_sourceID_t sourceID);
	void SetHotplugSink(const am_sinkID_t sinkID);
	void SetSourceALSAParams(const am_sourceID_t sourceID, const AlsaParams_s &alsaParams);
	void GetSourceALSAParams(const am_sourceID_t sourceID, AlsaParams_s &alsaParams);
	void SetSinkALSAParams(const am_sinkID_t sinkID, const AlsaParams_s &alsaParams);
	void GetSinkALSAParams(const am_sinkID_t sinkID, AlsaParams_s &alsaParams);
	bool IsHotplugSource(const am_sourceID_t sourceID);
	bool IsHotPlugSink(const am_sinkID_t sinkID);
	bool sourceExists(const am_sourceID_t sourceID);
	bool sinkExists(const am_sinkID_t sinkID);
	bool connectionExists(const am_connectionID_t connectionID);
	bool connectionExists(const am_sourceID_t sourceID, const am_sinkID_t sinkID);
	void SetGatewayMap(const am_gatewayID_t gatewayID, const GatewayInfo_t gwInfo);
	std::vector <am_sourceID_t> GetRegisteredSources(void);
	std::vector <am_sinkID_t> GetRegisteredSinks(void);
	std::vector <am_gatewayID_t> GetRegisteredGateways(void);
	std::vector <am_connectionID_t> GetEstablishedConnections(void);
	bool IsSourceOwnerRouter(const am_sourceID_t sourceID);
	bool IsSinkOwnerRouter(const am_sinkID_t sinkID);
	bool IsGatewayOwnerRouter(const am_gatewayID_t gatewayID);
	void ClearContext(void);

private:
	std::map <am_connectionID_t, Connection_t> mConnectionMap;
	std::map <am_sinkID_t, SinkInfo_t> mSinkMap;
	std::map <am_sourceID_t, SourceInfo_t> mSourceMap;
	std::map <am_gatewayID_t, GatewayInfo_t> mGatewayMap;
	am_domainID_t		mDomainID;
};

#endif // __C_CONTEXT_MANAGER_H__
