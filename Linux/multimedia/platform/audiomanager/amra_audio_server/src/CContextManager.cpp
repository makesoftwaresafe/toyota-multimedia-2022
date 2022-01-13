/************************************************************************
* @file: CContextManager.cpp
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
#include "CContextManager.h"
#include <dlt/dlt.h>
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext);

CContextManager::CContextManager()
{
	LOG_FN_ENTRY(raContext);
	mDomainID = 0;
	LOG_FN_EXIT(raContext);
}

CContextManager::~CContextManager()
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetDomainID(const am_domainID_t &domainID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" domainID: "), DLT_INT(domainID));
	mDomainID = domainID;
	LOG_FN_EXIT(raContext);
}

am_domainID_t CContextManager::GetDomainID(void)
{
	LOG_FN_ENTRY(raContext);
	LOG_FN_EXIT(raContext, DLT_STRING(" mDomainID: "), DLT_INT(mDomainID));
	return mDomainID;
}

void CContextManager::SetSourceAsID(const am_sourceID_t sourceID, const Ias::Int32 asID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am source ID: "), DLT_INT(sourceID), DLT_STRING(" as source ID: "), DLT_INT(asID));
	mSourceMap[sourceID].as_source_id = asID;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSourceAsHandle(const am_sourceID_t sourceID, const Ias::Int32 asHandle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am sourceID: "), DLT_INT(sourceID), DLT_STRING(" as handle "), DLT_INT(asHandle));
	mSourceMap[sourceID].as_source_handle = asHandle;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSourceMap(const am_sourceID_t sourceID, const SourceInfo_t srcInfo)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	mSourceMap[sourceID] = srcInfo;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSinkAsID(const am_sinkID_t sinkID, const Ias::Int32 asID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am sinkID: "), DLT_INT(sinkID), DLT_STRING(" as sinkID: "), DLT_INT(asID));
	mSinkMap[sinkID].as_sink_id = asID;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSinkAsHandle(const am_sinkID_t sinkID, const Ias::Int32 asHandle)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am sinkID: "), DLT_INT(sinkID), DLT_STRING(" as handle: "), DLT_INT(asHandle));
	mSinkMap[sinkID].as_sink_handle = asHandle;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSinkMap(const am_sinkID_t sinkID, const SinkInfo_t sinkInfo)
{
	LOG_FN_ENTRY(raContext, DLT_STRING("am sinkID: "), DLT_INT(sinkID));
	mSinkMap[sinkID] = sinkInfo;
	LOG_FN_EXIT(raContext);
}

void CContextManager::GetConnectionData(const am_connectionID_t connectionID, am_sourceID_t &source, am_sinkID_t &sink)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" connectionID: "), DLT_INT(connectionID));
	Connection_t c = mConnectionMap[connectionID];
	
	source = c.source;
	sink = c.sink;

	LOG_FN_EXIT(raContext, DLT_STRING(" connectionID: "), DLT_INT(connectionID), DLT_STRING(" sourceID: "), DLT_INT(source), DLT_STRING(" sinkID: "), DLT_INT(sink));
	return;
}

void CContextManager::SetConnectionData(const am_connectionID_t connectionID, const am_sourceID_t source, const am_sinkID_t sink)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" connectionID: "), DLT_INT(connectionID), DLT_STRING(" sourceID: "), DLT_INT(source), DLT_STRING(" sinkID: "), DLT_INT(sink));
	Connection_t c;
	c.source = source;
	c.sink = sink;

	mConnectionMap[connectionID] = c;
	LOG_FN_EXIT(raContext);
	return;
}

void CContextManager::RemoveConnectionData(const am_connectionID_t connectionID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" connectionID: "), DLT_INT(connectionID));
	mConnectionMap.erase(connectionID);
	LOG_FN_EXIT(raContext);
}

bool CContextManager::IsConnectedSink(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.sink == sinkID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" true "));
			return true;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" false "));
	return false;
}

bool CContextManager::IsConnectedSource(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.source == sourceID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" true "));
			return true;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" true "));
	return false;
}

am_sourceID_t CContextManager::GetConnectedSourceIDFromSinkID(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.sink == sinkID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" is connected to sourceID: "), DLT_INT(it->second.source));
			return it->second.source;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" No source is connected to sinkID: "), DLT_INT(sinkID));
	return 0;
}

am_sinkID_t CContextManager::GetConnectedSinkIDFromSourceID(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.source == sourceID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" is connected to sinkID: "), DLT_INT(it->second.sink));
			return it->second.sink;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" No sink is connected to sourceID: "), DLT_INT(sourceID));
	return 0;
}

void CContextManager::SetSourceInterallyDisconnected(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.source == sourceID)
			it->second.internallyDisconnected = true;
	}
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSourceInterallyConnected(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.source == sourceID)
			it->second.internallyDisconnected = false;
	}
	LOG_FN_EXIT(raContext);
}

bool CContextManager::IsSourceInternallyDisconnected(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.source == sourceID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" is internaly disconnected: "), DLT_STRING(it->second.internallyDisconnected ? "yes":"no"));
			return it->second.internallyDisconnected;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" not found !"));
	return false;
}

void CContextManager::SetSinkInterallyDisconnected(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.sink == sinkID)
			it->second.internallyDisconnected = true;
	}
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSinkInterallyConnected(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.sink == sinkID)
			it->second.internallyDisconnected = false;
	}
	LOG_FN_EXIT(raContext);
}

bool CContextManager::IsSinkInternallyDisconnected(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if(it->second.sink == sinkID)
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" is internaly disconnected: "), DLT_STRING(it->second.internallyDisconnected ? "yes":"no"));
			return it->second.internallyDisconnected;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" not found !"));
	return false;
}

void CContextManager::SetVolumeData(const am_sinkID_t sinkID, const am_volume_t volume)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" volume: "), DLT_INT(volume));
	mSinkMap[sinkID].volume = volume;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetMuteStateData(const am_sinkID_t sinkID, const bool muteState)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" muteState: "), DLT_STRING(muteState ? "muted":"unmuted"));
	mSinkMap[sinkID].mutestate = muteState;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetBalanceData(const am_sinkID_t sinkID, const int16_t balance)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" balance: "), DLT_INT(balance));
	mSinkMap[sinkID].balance = balance;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetFaderData(const am_sinkID_t sinkID, const int16_t fader)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" fader: "), DLT_INT(fader));
	mSinkMap[sinkID].fader = fader;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetEqualizationData(const am_sinkID_t sinkID, const uint16_t band, const int16_t gain)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" band: "), DLT_INT(band), DLT_STRING(" gain: "), DLT_INT(gain));
	switch(band) {
		case 0:
			mSinkMap[sinkID].treble = gain;
			break;
		case 1:
			mSinkMap[sinkID].bass = gain;
			break;
		default:
			break;
	}
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetLoudnessData(const am_sinkID_t sinkID, const bool loudness)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" loudness: "), DLT_STRING(loudness ? "on":"off"));
	mSinkMap[sinkID].loudness = loudness;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSCVData(const am_sinkID_t sinkID, const bool scv)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" speed controlled volume: "), DLT_STRING(scv ? "on":"off"));
	mSinkMap[sinkID].scv = scv;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetInputGainOffset(const am_sinkID_t sinkID, const int16_t inputgainoffset)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" inputgainoffset: "), DLT_INT(inputgainoffset));
	mSinkMap[sinkID].inputgainoffset = inputgainoffset;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetHotplugSource(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	mSourceMap[sourceID].isHotplugSource = true;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetHotplugSink(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	mSinkMap[sinkID].isHotplugSink = true;
	LOG_FN_EXIT(raContext);
}

bool CContextManager::IsHotplugSource(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" is hotplug source: "), DLT_STRING(mSourceMap[sourceID].isHotplugSource ? "yes":"no"));
	return mSourceMap[sourceID].isHotplugSource;
}

bool CContextManager::IsHotPlugSink(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" is hotplug sink: "), DLT_STRING(mSinkMap[sinkID].isHotplugSink ? "yes":"no"));
	return mSinkMap[sinkID].isHotplugSink;
}

bool CContextManager::sourceExists(const am_sourceID_t sourceID)
{
	if (mSourceMap.find(sourceID) != mSourceMap.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CContextManager::sinkExists(const am_sinkID_t sinkID)
{
	if (mSinkMap.find(sinkID) != mSinkMap.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CContextManager::connectionExists(const am_connectionID_t connectionID)
{
	if (mConnectionMap.find(connectionID) != mConnectionMap.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CContextManager::connectionExists(const am_sourceID_t sourceID, const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	for (std::map <am_connectionID_t, Connection_t>::iterator it = mConnectionMap.begin();
		it != mConnectionMap.end(); ++it)
	{
		if((it->second.source == sourceID) && (it->second.sink == sinkID))
		{
			LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" isConnected == true "));
			return true;
		}
	}

	LOG_FN_EXIT(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID), DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" isConnected == false "));
	return false;
}

void CContextManager::SetGatewayMap(const am_gatewayID_t gatewayID, const GatewayInfo_t gwInfo)
{
	LOG_FN_ENTRY(raContext, DLT_STRING("am gatewayID: "), DLT_INT(gatewayID));
	mGatewayMap[gatewayID] = gwInfo;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSourceALSAParams(const am_sourceID_t sourceID, const AlsaParams_s &alsaParams)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	mSourceMap[sourceID].as_source_alsa_params = alsaParams;
	LOG_FN_EXIT(raContext);
}

void CContextManager::GetSourceALSAParams(const am_sourceID_t sourceID, AlsaParams_s &alsaParams)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sourceID: "), DLT_INT(sourceID));
	alsaParams = mSourceMap[sourceID].as_source_alsa_params;
	LOG_FN_EXIT(raContext);
}

void CContextManager::SetSinkALSAParams(const am_sinkID_t sinkID, const AlsaParams_s &alsaParams)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	mSinkMap[sinkID].as_sink_alsa_params = alsaParams;
	LOG_FN_EXIT(raContext);
}

void CContextManager::GetSinkALSAParams(const am_sinkID_t sinkID, AlsaParams_s &alsaParams)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	alsaParams = mSinkMap[sinkID].as_sink_alsa_params;
	LOG_FN_EXIT(raContext);
}

Ias::Int32 CContextManager::GetAsSourceIDFromAmSourceID(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am sourceID: "), DLT_INT(sourceID));
	LOG_FN_EXIT(raContext, DLT_STRING(" am sourceID: "), DLT_INT(sourceID), DLT_STRING(" as source ID: "), DLT_INT(mSourceMap[sourceID].as_source_id));
	return mSourceMap[sourceID].as_source_id;
}

Ias::Int32 CContextManager::GetAsSourceHandleFromAmSourceID(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" am sourceID: "), DLT_INT(sourceID));
	LOG_FN_EXIT(raContext, DLT_STRING(" am sourceID: "), DLT_INT(sourceID), DLT_STRING(" as source handle: "), DLT_INT(mSourceMap[sourceID].as_source_handle));
	return mSourceMap[sourceID].as_source_handle;
}

Ias::Int32 CContextManager::GetAsSinkIDFromAmSinkID(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" as sink ID: "), DLT_INT(mSinkMap[sinkID].as_sink_id));
	return mSinkMap[sinkID].as_sink_id;
}

Ias::Int32 CContextManager::GetAsSinkHandleFromAmSinkID(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID));
	LOG_FN_EXIT(raContext, DLT_STRING(" sinkID: "), DLT_INT(sinkID), DLT_STRING(" as sink handle: "), DLT_INT(mSinkMap[sinkID].as_sink_handle));
	return mSinkMap[sinkID].as_sink_handle;
}

std::vector <am_sourceID_t> CContextManager::GetRegisteredSources(void)
{
	LOG_FN_ENTRY(raContext);
	std::map <am_sourceID_t, SourceInfo_t>::iterator it;
	std::vector <am_sourceID_t> registeredSources;
	for (it = mSourceMap.begin(); it != mSourceMap.end(); ++it)
	{
		registeredSources.push_back(it->first);
	}

	LOG_FN_EXIT(raContext);
	return registeredSources;
}

std::vector <am_sinkID_t> CContextManager::GetRegisteredSinks(void)
{
	LOG_FN_ENTRY(raContext);	
	std::map <am_sinkID_t, SinkInfo_t>::iterator it;
	std::vector <am_sinkID_t> registeredSinks;
	for (it = mSinkMap.begin(); it != mSinkMap.end(); ++it)
	{
		registeredSinks.push_back(it->first);
	}

	LOG_FN_EXIT(raContext);
	return registeredSinks;
}

std::vector <am_gatewayID_t> CContextManager::GetRegisteredGateways(void)
{
	LOG_FN_ENTRY(raContext);
	std::map <am_gatewayID_t, GatewayInfo_t>::iterator it;
	std::vector <am_gatewayID_t> registeredGateways;
	for (it = mGatewayMap.begin(); it != mGatewayMap.end(); ++it)
	{
		registeredGateways.push_back(it->first);
	}

	LOG_FN_EXIT(raContext);
	return registeredGateways;
}

std::vector <am_connectionID_t> CContextManager::GetEstablishedConnections(void)
{
	LOG_FN_ENTRY(raContext);
	std::map <am_connectionID_t, Connection_t>::iterator it;
	std::vector <am_connectionID_t> connections;
	for (it = mConnectionMap.begin(); it != mConnectionMap.end(); ++it)
	{
		connections.push_back(it->first);
	}

	LOG_FN_EXIT(raContext);
	return connections;
}

bool CContextManager::IsSourceOwnerRouter(const am_sourceID_t sourceID)
{
	LOG_FN_ENTRY(raContext);
	bool status = false;

	std::map <am_sourceID_t, SourceInfo_t>::iterator it = mSourceMap.find(sourceID);
	if (it != mSourceMap.end())
	{
		status = it->second.isSourceOwnerRouter;
	}

	LOG_FN_EXIT(raContext);
	return status;
}

bool CContextManager::IsSinkOwnerRouter(const am_sinkID_t sinkID)
{
	LOG_FN_ENTRY(raContext);
	bool status = false;

	std::map <am_sinkID_t, SinkInfo_t>::iterator it = mSinkMap.find(sinkID);
	if (it != mSinkMap.end())
	{
		status = it->second.isSinkOwnerRouter;
	}

	LOG_FN_EXIT(raContext);
	return status;
}

bool CContextManager::IsGatewayOwnerRouter(const am_gatewayID_t gatewayID)
{
	LOG_FN_ENTRY(raContext);
	bool status = false;

	std::map <am_gatewayID_t, GatewayInfo_t>::iterator it = mGatewayMap.find(gatewayID);
	if (it != mGatewayMap.end())
	{
		status = it->second.isGatewayOwnerRouter;
	}

	LOG_FN_EXIT(raContext);
	return status;
}

void CContextManager::ClearContext(void)
{
	LOG_FN_ENTRY(raContext);
	mSourceMap.clear();
	mSinkMap.clear();
	mConnectionMap.clear();
	mGatewayMap.clear();
	mDomainID = 0;
	LOG_FN_EXIT(raContext);
}
