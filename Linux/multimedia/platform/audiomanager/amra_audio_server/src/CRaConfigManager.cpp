/************************************************************************
* @file: CRaConfigManager.cpp
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
#include "CRaConfigManager.h"
#include "dlt/dlt.h"

static inline float CalculateGain(SPRange_t user, SPRange_t domain)
{
	return ((float) (domain.max - domain.min) / (float) (user.max - user.min));
}

CRaConfigManager::CRaConfigManager(const char *configFileName)
{
	// default log threshold is INFO
	mLogThreshold = DLT_LOG_INFO;
	ParseDomain(configFileName);
}

CRaConfigManager::~CRaConfigManager()
{
}

std::string CRaConfigManager::GetInterface(void)
{
	return mInterface;
}

DltLogLevelType CRaConfigManager::GetLogThreshold(void)
{
	return mLogThreshold;
}

ListGatewayData_t CRaConfigManager::GetGateways(void)
{
	return mGateways;
}

ListSinkData_t CRaConfigManager::GetSinks(void)
{
	return mSinks;
}

am_Domain_s CRaConfigManager::GetDomain(void)
{
	return mDomain;
}

ListSourceData_t CRaConfigManager::GetSources(void)
{
	return mSources;
}

ListCrossFader_t CRaConfigManager::GetCrossfaders(void)
{
	return mCrossfaders;
}

void CRaConfigManager::ParseDomain(const char *configFileName)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *key;

	doc = xmlParseFile(configFileName);
	if (NULL == doc)
	{
		std::cout << "Parsing Domain Config file: " << configFileName << " failed !" << std::endl;
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (NULL == cur)
	{
		std::cout << "Domain Config file: " << configFileName << " is empty!" << std::endl;
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "domain"))
	{
		std::cout << "Root node is != domain in Domain Config file: " << configFileName << std::endl;
		return;
	}

	cur = cur->xmlChildrenNode;

	while (NULL != cur) {
		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "id")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mDomain.domainID = (am_domainID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "name")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mDomain.name = (char *) key;
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "bus_name")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mDomain.busname = (char *) key;
			xmlFree(key);
		}
	
		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "node_name")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mDomain.nodename = (char *) key;
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "early")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (0 == atoi((const char *) key)) {
				mDomain.early = false;
			}
			else {
				mDomain.early = true;
			}
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "complete")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (0 == atoi((const char *) key)) {
				mDomain.complete = false;
			}
			else {
				mDomain.complete = true;
			}
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "state")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mDomain.state = (am_DomainState_e) atoi((const char *) key);
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "interface")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mInterface = (char *) key;
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "dlt_log_threshold")) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (0 == strcmp((const char *) key, "DLT_LOG_FATAL"))
			{
				mLogThreshold = DLT_LOG_FATAL;
			}
			else if (0 == strcmp((const char *) key, "DLT_LOG_ERROR"))
			{
				mLogThreshold = DLT_LOG_ERROR;
			}
			else if (0 == strcmp((const char *) key, "DLT_LOG_WARN"))
			{
				mLogThreshold = DLT_LOG_WARN;
			}
			else if (0 == strcmp((const char *) key, "DLT_LOG_INFO"))
			{
				mLogThreshold = DLT_LOG_INFO;
			}
			else if (0 == strcmp((const char *) key, "DLT_LOG_DEBUG"))
			{
				mLogThreshold = DLT_LOG_DEBUG;
			}
			else if (0 == strcmp((const char *) key, "DLT_LOG_VERBOSE"))
			{
				mLogThreshold = DLT_LOG_VERBOSE;
			}
			else
			{
				mLogThreshold = DLT_LOG_INFO;
			}
			xmlFree(key);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "source")) {
			Source_t src;
			ParseSource(doc, cur, src);
			mSources.push_back(src);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "sink")) {
			Sink_t snk;
			ParseSink(doc, cur, snk);
			mSinks.push_back(snk);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "gateway")) {
			Gateway_t gw;
			ParseGateway(doc, cur, gw);
			mGateways.push_back(gw);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "crossfader")) {
			CrossFader_t cf;
			ParseCrossfader(doc, cur, cf);
			mCrossfaders.push_back(cf);
		}

		if(0 == xmlStrcmp(cur->name, (const xmlChar *) "sound_properties_map")) {
			ParseSPMap(doc, cur);
		}

		cur = cur->next;
	}
}

void CRaConfigManager::ParseSource(xmlDocPtr doc, xmlNodePtr cur, Source_t& s)
{
	xmlChar *key;
	// if configuration does not contain owner tag, then it is assumed that owner is routing
	s.owner = OWNER_ROUTING;

	cur = cur->xmlChildrenNode;

	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.sourceID = (am_sourceID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.name = (char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"class_id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.sourceClassID = (am_sourceClass_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"state"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.sourceState = (am_SourceState_e) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"volume"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.volume = (am_volume_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"visible"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (0 == atoi((const char *) key)) {
				s.am_source.visible = false;
			}
			else {
				s.am_source.visible = true;
			}
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"availability"))) {
			ParseAvailability(doc, cur, s.am_source.available);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"interrupt_state"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_source.interruptState = (am_InterruptState_e) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sound_property"))) {
			am_SoundProperty_s p;
			ParseSoundProperties(doc, cur, p);
			s.am_source.listSoundProperties.push_back(p);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"connection_format"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			am_CustomConnectionFormat_t format = (am_CustomConnectionFormat_t) atoi((const char *)key);
			s.am_source.listConnectionFormats.push_back(format);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"main_sound_property"))) {
			am_MainSoundProperty_s p;
			ParseMainSoundProperties(doc, cur, p);
			s.am_source.listMainSoundProperties.push_back(p);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"as_source_id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.as_source_id = (Ias::Int32) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"owner"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (strcmp((const char *) key, "CONTROLLER") == 0)
			{
				s.owner = OWNER_CONTROLLER;
			}
			else if (strcmp((const char *) key, "ROUTING") == 0)
			{
				s.owner = OWNER_ROUTING;
			}
			else
			{
				s.owner = OWNER_UNKNOWN;
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseSink(xmlDocPtr doc, xmlNodePtr cur, Sink_t& s)
{
	xmlChar *key;
	// if configuration does not contain owner tag, then it is assumed that owner is routing
	s.owner = OWNER_ROUTING;

	cur = cur->xmlChildrenNode;

	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.sinkID = (am_sinkID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.name = (char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"class_id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.sinkClassID = (am_sinkClass_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"volume"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.volume = (am_volume_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"visible"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (0 == atoi((const char *) key)) {
				s.am_sink.visible = false;
			}
			else {
				s.am_sink.visible = true;
			}
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"availability"))) {
			ParseAvailability(doc, cur, s.am_sink.available);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"mute_state"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.muteState = (am_MuteState_e) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"main_volume"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.am_sink.mainVolume = (am_mainVolume_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sound_property"))) {
			am_SoundProperty_s p;
			ParseSoundProperties(doc, cur, p);
			s.am_sink.listSoundProperties.push_back(p);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"connection_format"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			am_CustomConnectionFormat_t format = (am_CustomConnectionFormat_t) atoi((const char *) key);
			s.am_sink.listConnectionFormats.push_back(format);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"main_sound_property"))) {
			am_MainSoundProperty_s p;
			ParseMainSoundProperties(doc, cur, p);
			s.am_sink.listMainSoundProperties.push_back(p);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"as_sink_id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			s.as_sink_id = (am_sinkID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"owner"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (strcmp((const char *) key, "CONTROLLER") == 0)
			{
				s.owner = OWNER_CONTROLLER;
			}
			else if (strcmp((const char *) key, "ROUTING") == 0)
			{
				s.owner = OWNER_ROUTING;
			}
			else
			{
				s.owner = OWNER_UNKNOWN;
			}
			xmlFree(key);
		}

		cur = cur->next;
	}
}

void CRaConfigManager::ParseGateway(xmlDocPtr doc, xmlNodePtr cur, Gateway_t& gw)
{
	xmlChar *key;
	// if configuration does not contain owner tag, then it is assumed that owner is routing
	gw.owner = OWNER_ROUTING;

	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gatewayID = (am_gatewayID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.name = (char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sink"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gwSinkName = (const char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"source"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gwSourceName = (const char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sink_domain"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gwSinkDomainName = (const char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"source_domain"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gwSourceDomainName = (const char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"control_domain"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			gw.gwControlDomainName = (const char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"source_format"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			am_CustomConnectionFormat_t format = (am_CustomConnectionFormat_t) atoi((const char *) key);
			gw.listSourceFormats.push_back(format);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sink_format"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			am_CustomConnectionFormat_t format = (am_CustomConnectionFormat_t) atoi((const char *) key);
			gw.listSinkFormats.push_back(format);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"convert"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			bool convert;
			if (0 == atoi((const char *) key))
			{
				convert = false;
			}
			else
			{
				convert = true;
			}

			gw.convertionMatrix.push_back(convert);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"owner"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (strcmp((const char *) key, "CONTROLLER") == 0)
			{
				gw.owner = OWNER_CONTROLLER;
			}
			else if (strcmp((const char *) key, "ROUTING") == 0)
			{
				gw.owner = OWNER_ROUTING;
			}
			else
			{
				gw.owner = OWNER_UNKNOWN;
			}
			xmlFree(key);
		}

		cur = cur->next;
	}
}

void CRaConfigManager::ParseCrossfader(xmlDocPtr doc, xmlNodePtr cur, CrossFader_t& cf)
{
	xmlChar *key;
	// if configuration does not contain owner tag, then it is assumed that owner is routing
	cf.owner = OWNER_ROUTING;

	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.crossfaderID = (am_crossfaderID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.name = (char *) key;
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sink_id_a"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.sinkID_A = (am_sinkID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"sink_id_b"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.sinkID_B = (am_sinkID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"source_id"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.sourceID = (am_sourceID_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"hot_sink"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			cf.hotSink = (am_HotSink_e) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"owner"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (strcmp((const char *) key, "CONTROLLER") == 0)
			{
				cf.owner = OWNER_CONTROLLER;
			}
			else if (strcmp((const char *) key, "ROUTING") == 0)
			{
				cf.owner = OWNER_ROUTING;
			}
			else
			{
				cf.owner = OWNER_UNKNOWN;
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseAvailability(xmlDocPtr doc, xmlNodePtr cur, am_Availability_s& availability)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"available"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			availability.availability = (am_Availability_e) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"availability_reason"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			availability.availabilityReason = (am_CustomAvailabilityReason_t) atoi((const char *) key);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseSoundProperties(xmlDocPtr doc, xmlNodePtr cur, am_SoundProperty_s& soundProperty)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			soundProperty.type = (am_CustomSoundPropertyType_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			soundProperty.value = (int16_t) atoi((const char *) key);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseMainSoundProperties(xmlDocPtr doc, xmlNodePtr cur, am_MainSoundProperty_s& soundProperty)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			soundProperty.type = (am_CustomMainSoundPropertyType_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			soundProperty.value = (int16_t) atoi((const char *) key);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseSPMap(xmlDocPtr doc, xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"user"))) {
			ParseSPValues(doc, cur, mUserSPMap);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"domain"))) {
			ParseSPValues(doc, cur, mDomainSPMap);
		}
		cur = cur->next;
	}
}

void CRaConfigManager::ParseSPValues(xmlDocPtr doc, xmlNodePtr cur, SPMap_t& SPMap)
{
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"volume"))) {
			ParseMinMax(doc, cur, SPMap.volume);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"balance"))) {
			ParseMinMax(doc, cur, SPMap.balance);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"fader"))) {
			ParseMinMax(doc, cur, SPMap.fader);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"equalizer"))) {
			ParseMinMax(doc, cur, SPMap.equalizer);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"speedcontrolvolume"))) {
			ParseMinMax(doc, cur, SPMap.speedcontrolvolume);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"loudness"))) {
			ParseMinMax(doc, cur, SPMap.loudness);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"inputgainoffset"))) {
			ParseMinMax(doc, cur, SPMap.inputgainoffset);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"mutestate"))) {
			ParseMinMax(doc, cur, SPMap.mutestate);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"rampshape"))) {
			ParseMinMax(doc, cur, SPMap.rampshape);
		}

		cur = cur->next;
	}
}

void CRaConfigManager::ParseMinMax(xmlDocPtr doc, xmlNodePtr cur, SPRange_t& spRange)
{
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (NULL != cur) {
		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"min"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			spRange.min = (int16_t) atoi((const char *) key);
			xmlFree(key);
		}

		if ((0 == xmlStrcmp(cur->name, (const xmlChar *)"max"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			spRange.max = (int16_t) atoi((const char *) key);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

Ias::Int32 CRaConfigManager::GetAsVolumeFromAmVolume(const am_volume_t amVolume)
{
	Ias::Int32 asVolume;
	float gain = CalculateGain(mUserSPMap.volume, mDomainSPMap.volume);

	// saturate audio server volume to lowest if -3000 (for mute) is requested
	if ( amVolume == -3000 )
	{
		asVolume = mDomainSPMap.volume.min;
	}
	else
	{
		asVolume = (Ias::Int32) (mDomainSPMap.volume.min + (gain * (float) (amVolume) - mUserSPMap.volume.min));
	}

	return asVolume;
}

IasAudioProcessing::IasRampShape CRaConfigManager::GetAsRampShapeFromAmRampShape(const am_CustomRampType_t amRamp)
{
	/*am side rampshape data is an enum with several values,
	 * however as side supports only two values, i.e. linear & exponential 
	 * we have mapped min -> linear & max -> exponential
	 * and default = linear */
	IasAudioProcessing::IasRampShape asRamp = (IasAudioProcessing::IasRampShape) mDomainSPMap.rampshape.min;
	if (amRamp == mUserSPMap.rampshape.max)
	{
		asRamp = (IasAudioProcessing::IasRampShape) mDomainSPMap.rampshape.max;
	}

	return asRamp;
}

IasAudioProcessing::IasMuteState CRaConfigManager::GetAsMuteStateFromAmMuteState(const bool amMuteState)
{
	/* mute state is a boolean type, min -> off, max -> on, default = off */
	IasAudioProcessing::IasMuteState asMuteState = (IasAudioProcessing::IasMuteState) mDomainSPMap.mutestate.min;
	if (amMuteState == mUserSPMap.mutestate.max)
	{
		asMuteState = (IasAudioProcessing::IasMuteState) mDomainSPMap.mutestate.max;
	}

	return asMuteState;
}

Ias::Int32 CRaConfigManager::GetAsBalanceFromAmBalance(const int16_t amBalance)
{
	Ias::Int32 asBalance;
	float gain = CalculateGain(mUserSPMap.balance, mDomainSPMap.balance);
	asBalance = (Ias::Int32) (mDomainSPMap.balance.min + (gain * (float) (amBalance - mUserSPMap.balance.min)));

	return asBalance;
}

Ias::Int32 CRaConfigManager::GetAsFaderFromAmFader(const int16_t amFader)
{
	Ias::Int32 asFader;
	float gain = CalculateGain(mUserSPMap.fader, mDomainSPMap.fader);
	asFader = (Ias::Int32) (mDomainSPMap.fader.min + (gain * (float) (amFader - mUserSPMap.fader.min)));

	return asFader;
}

Ias::Int32 CRaConfigManager::GetAsEqualizationFromAmEqualization(const int16_t amEqualization)
{
	Ias::Int32 asEqualization;
	float gain = CalculateGain(mUserSPMap.equalizer, mDomainSPMap.equalizer);
	asEqualization = (Ias::Int32) (mDomainSPMap.equalizer.min + (gain * (float) (amEqualization - mUserSPMap.equalizer.min)));

	return asEqualization;
}

Ias::Int32 CRaConfigManager::GetAsLoudnessFromAmLoudness(const int16_t amLoudness)
{
	/* loudness is a boolean type, min -> off, max -> on, default = off */
	Ias::Int32 asLoudness = (Ias::Int32) mDomainSPMap.loudness.min;
	if (amLoudness == mUserSPMap.loudness.max)
	{
		asLoudness = (Ias::Int32) mDomainSPMap.loudness.max;
	}

	return asLoudness;
}

Ias::Int32 CRaConfigManager::GetAsInputGainOffsetFromAmInputGainOffset(const int16_t amInputGainOffset)
{
	Ias::Int32 asInputGainOffset = 0;
	float gain = CalculateGain(mUserSPMap.inputgainoffset, mDomainSPMap.inputgainoffset);
	asInputGainOffset = (Ias::Int32) (mDomainSPMap.inputgainoffset.min + (gain * (float) (amInputGainOffset - mUserSPMap.inputgainoffset.min)));

	return asInputGainOffset;
}

Ias::Int32 CRaConfigManager::GetAsSCVFromAmSCV(const int16_t amSCV)
{
	/* scv is a boolean type, min -> off, max -> on, default = off*/
	Ias::Int32 asSCV = (Ias::Int32) mDomainSPMap.scv.min;
	if (amSCV == mUserSPMap.scv.max)
	{
		asSCV = (Ias::Int32) mDomainSPMap.scv.max;
	}

	return asSCV;
}

Owner_e CRaConfigManager::GetSourceOwner(const am_sourceID_t sourceID)
{
	for(ListSourceDataIter_t it = mSources.begin(); it != mSources.end(); ++it)
	{
		if (it->am_source.sourceID == sourceID)
		{
			return it->owner;
		}
	}

	return OWNER_UNKNOWN;
}

Owner_e CRaConfigManager::GetSinkOwner(const am_sinkID_t sinkID)
{
	for (ListSinkDataIter_t it = mSinks.begin(); it != mSinks.end(); ++it)
	{
		if (it->am_sink.sinkID == sinkID)
		{
			return it->owner;
		}
	}

	return OWNER_UNKNOWN;
}

Owner_e CRaConfigManager::GetGatewayOwner(const am_gatewayID_t gatewayID)
{
	for (ListGatewayDataIter_t it = mGateways.begin(); it !=  mGateways.end(); ++it)
	{
		if (it->gatewayID == gatewayID)
		{
			return it->owner;
		}
	}

	return OWNER_UNKNOWN;
}

Owner_e CRaConfigManager::GetCrossfaderOwner(const am_crossfaderID_t crossfaderID)
{
	for (ListCrossFaderIter_t it = mCrossfaders.begin(); it !=  mCrossfaders.end(); ++it)
	{
		if (it->crossfaderID == crossfaderID)
		{
			return it->owner;
		}
	}

	return OWNER_UNKNOWN;
}
