/************************************************************************
* @file: CRaConfigManager.h
*
* @version: 1.1
*
* CRaConfigManager class provides interfaces to read configuration data
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
#ifndef __C_RA_CONFIG_MANAGER_H__
#define __C_RA_CONFIG_MANAGER_H__

#include <stdint.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <iostream>
#include <vector>
#include <audiomanagertypes.h>
#include <dlt/dlt.h>
#include "core_libraries/foundation/IasTypes.hpp"
#include "audio/rpcontroller/idl/IasAudioProcessingStub.hpp"

using namespace std;
using namespace am;

enum Owner_e {
	OWNER_UNKNOWN = 0,
	OWNER_CONTROLLER = 1,
	OWNER_ROUTING = 2
};

struct Source_t {
	am_Source_s	am_source;
	Ias::Int32	as_source_id;
	Owner_e		owner;
};

struct Sink_t {
	am_Sink_s		am_sink;
	Ias::Int32		as_sink_id;
	Owner_e		owner;
};

struct Gateway_t : public am_Gateway_s
{
	std::string	gwSinkName;
	std::string	gwSourceName;
	std::string	gwSinkDomainName;
	std::string	gwSourceDomainName;
	std::string	gwControlDomainName;
	Owner_e		owner;
};

struct CrossFader_t : public am_Crossfader_s
{
	Owner_e         owner;
};

typedef std::vector <Source_t> ListSourceData_t;
typedef std::vector <Sink_t> ListSinkData_t;
typedef std::vector <Gateway_t> ListGatewayData_t;
typedef std::vector <CrossFader_t> ListCrossFader_t;

typedef std::vector <Source_t>::iterator ListSourceDataIter_t;
typedef std::vector <Sink_t>::iterator ListSinkDataIter_t;
typedef std::vector <Gateway_t>::iterator ListGatewayDataIter_t;
typedef std::vector <CrossFader_t>::iterator ListCrossFaderIter_t;
struct SPRange_t
{
	int32_t min;
	int32_t max;
};

struct SPMap_t
{       
	SPRange_t volume;
	SPRange_t rampshape;
	SPRange_t balance;     
	SPRange_t fader;
	SPRange_t equalizer;   
	SPRange_t speedcontrolvolume;   
	SPRange_t loudness;    
	SPRange_t inputgainoffset;      
	SPRange_t mutestate;   
	SPRange_t scv;   
};

class CRaConfigManager
{
public:
	CRaConfigManager(const char *configFile);
	~CRaConfigManager();

	/* static configurations */
	std::string GetInterface(void);
	DltLogLevelType GetLogThreshold(void);
	am_Domain_s      GetDomain(void);
	ListSourceData_t  GetSources(void);
	ListSinkData_t    GetSinks(void);
	ListGatewayData_t GetGateways(void);
	ListCrossFader_t  GetCrossfaders(void);
	Ias::Int32 GetAsVolumeFromAmVolume(const am_volume_t amVolume);
	IasAudioProcessing::IasRampShape GetAsRampShapeFromAmRampShape(const am::am_CustomRampType_t amRamp);
	IasAudioProcessing::IasMuteState GetAsMuteStateFromAmMuteState(const bool amMuteState);
	Ias::Int32 GetAsBalanceFromAmBalance(const int16_t amBalance);
	Ias::Int32 GetAsFaderFromAmFader(const int16_t amFader);
	Ias::Int32 GetAsEqualizationFromAmEqualization(const int16_t amEqualization);
	Ias::Int32 GetAsLoudnessFromAmLoudness(const int16_t amLoudness);
	Ias::Int32 GetAsInputGainOffsetFromAmInputGainOffset(const int16_t amInputGainOffset);
	Ias::Int32 GetAsSCVFromAmSCV(const int16_t amSCV);
	Owner_e GetSourceOwner(const am_sourceID_t sourceID);
	Owner_e GetSinkOwner(const am_sinkID_t sinkID);
	Owner_e GetGatewayOwner(const am_gatewayID_t gatewayID);
	Owner_e GetCrossfaderOwner(const am_crossfaderID_t crossfaderID);

private:
	/* we do not want to instantiate the object with default constructor */
	CRaConfigManager();
	void ParseDomain(const char *configFileName);
	void ParseSource(xmlDocPtr doc, xmlNodePtr cur, Source_t& s);
	void ParseSink(xmlDocPtr doc, xmlNodePtr cur, Sink_t& s);
	void ParseGateway(xmlDocPtr doc, xmlNodePtr cur, Gateway_t& gw);
	void ParseCrossfader(xmlDocPtr doc, xmlNodePtr cur, CrossFader_t& cf);
	void ParseAvailability(xmlDocPtr doc, xmlNodePtr cur, am_Availability_s& a);
	void ParseSoundProperties(xmlDocPtr doc, xmlNodePtr cur, am_SoundProperty_s& SoundProperty);
	void ParseMainSoundProperties(xmlDocPtr doc, xmlNodePtr cur, am_MainSoundProperty_s& SoundProperty);
	void ParseSPMap(xmlDocPtr doc, xmlNodePtr cur);
	void ParseSPValues(xmlDocPtr doc, xmlNodePtr cur, SPMap_t& SPMap);
	void ParseMinMax(xmlDocPtr doc, xmlNodePtr cur, SPRange_t& spRange);

private:
	DltLogLevelType   mLogThreshold;
	std::string       mInterface;
	am_Domain_s       mDomain;
	ListSourceData_t  mSources;
	ListSinkData_t    mSinks;
	ListGatewayData_t mGateways;
	ListCrossFader_t  mCrossfaders;
        SPMap_t           mUserSPMap;
	SPMap_t           mDomainSPMap;
};
#endif //__C_RA_CONFIG_MANAGER_H__
