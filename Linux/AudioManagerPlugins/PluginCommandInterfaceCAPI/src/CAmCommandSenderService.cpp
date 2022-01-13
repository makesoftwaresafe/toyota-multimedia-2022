/**
 *  Copyright (c) 2012 BMW
 *
 *  \author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013
 *
 *  \copyright
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 */

#include "CAmCommandSenderService.h"
#include <assert.h>
#include <algorithm>
#include "CAmCommandSenderCommon.h"


namespace am {

CAmCommandSenderService::CAmCommandSenderService():mpIAmCommandReceive(NULL) {
	// TODO Auto-generated constructor stub

}

CAmCommandSenderService::CAmCommandSenderService(IAmCommandReceive *aReceiver):mpIAmCommandReceive(aReceiver) {
	// TODO Auto-generated constructor stub

}

CAmCommandSenderService::~CAmCommandSenderService() {
	// TODO Auto-generated destructor stub
}

void CAmCommandSenderService::connect(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_sinkID_t _sinkID, connectReply_t _reply) {
	assert(mpIAmCommandReceive);
	am_types::am_mainConnectionID_t mainConnectionID;
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->connect(_sourceID, _sinkID, mainConnectionID));
	_reply(mainConnectionID, result);
}

void CAmCommandSenderService::disconnect(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_mainConnectionID_t _mainConnectionID, disconnectReply_t _reply){
	assert(mpIAmCommandReceive);
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->disconnect(_mainConnectionID));
	_reply(result);
}

void CAmCommandSenderService::setVolume(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_mainVolume_t _volume, setVolumeReply_t _reply) {

	assert(mpIAmCommandReceive);
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setVolume(_sinkID, _volume));
	_reply(result);
}

void CAmCommandSenderService::volumeStep(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, int16_t _step, volumeStepReply_t _reply) {
	assert(mpIAmCommandReceive);
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->volumeStep(_sinkID, _step));
	_reply(result);
}

void CAmCommandSenderService::setSinkMuteState(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_MuteState_e _muteState, setSinkMuteStateReply_t _reply){
	assert(mpIAmCommandReceive);
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setSinkMuteState(_sinkID, CAmConvertFromCAPIType(_muteState)));
	_reply(result);
}

void CAmCommandSenderService::setMainSinkSoundProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_MainSoundProperty_s _soundProperty, setMainSinkSoundPropertyReply_t _reply){
	assert(mpIAmCommandReceive);
	am_MainSoundProperty_s property = {static_cast<am_CustomMainSoundPropertyType_t>(_soundProperty.getType()), _soundProperty.getValue()};
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setMainSinkSoundProperty(property, _sinkID));
	_reply(result);
}

void CAmCommandSenderService::setMainSourceSoundProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_MainSoundProperty_s _soundProperty, setMainSourceSoundPropertyReply_t _reply){
    assert(mpIAmCommandReceive);
	am_MainSoundProperty_s property = {static_cast<am_CustomMainSoundPropertyType_t>(_soundProperty.getType()), _soundProperty.getValue()};
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setMainSourceSoundProperty(property, _sourceID));
    _reply(result);
}

void CAmCommandSenderService::setSystemProperty(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_SystemProperty_s _property, setSystemPropertyReply_t _reply) {
    assert(mpIAmCommandReceive);
    am_SystemProperty_s property = {static_cast<am_CustomSystemPropertyType_t>(_property.getType()), _property.getValue()};
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setSystemProperty(property));
    _reply(result);
}

void CAmCommandSenderService::getListMainConnections(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainConnectionsReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_MainConnectionType_s> list;
    am_types::am_MainConnection_L listConnections;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainConnections(list));
    std::sort(list.begin(), list.end(),
		[](const am_MainConnectionType_s& a, const am_MainConnectionType_s& b)
		{
			return (a.mainConnectionID < b.mainConnectionID);
		});
    if((int)result==E_OK)
    {
    	am_types::am_MainConnectionType_s item;
		for(std::vector<am_MainConnectionType_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setMainConnectionID(iter->mainConnectionID);
			item.setSourceID (iter->sourceID);
			item.setSinkID(iter->sinkID);
			item.setDelay(iter->delay);
			item.setConnectionState(CAmConvert2CAPIType(iter->connectionState));
			listConnections.push_back (item);
		}
    }
    _reply(listConnections, result);
}

void CAmCommandSenderService::getListMainSinks(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainSinksReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_SinkType_s> list;
    am_types::am_SinkType_L listMainSinks;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSinks(list));
    std::sort(list.begin(), list.end(),
		[](const am_SinkType_s& a, const am_SinkType_s& b)
		{
			return (a.sinkID < b.sinkID);
		});

    if((int)result==E_OK)
    {
    	am_types::am_SinkType_s item;
		for(std::vector<am_SinkType_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setSinkID(iter->sinkID);
			item.setName(iter->name);
			item.setSinkClassID(iter->sinkClassID);
			item.setVolume(iter->volume);
			item.setMuteState(CAmConvert2CAPIType(iter->muteState));
			am_types::am_Availability_s av;
		    CAmConvertAvailablility(iter->availability, av);
		    item.setAvailability(av);
			listMainSinks.push_back (item);
		}
    }
    _reply(listMainSinks, result);
}

void CAmCommandSenderService::getListMainSources(const std::shared_ptr<CommonAPI::ClientId> _client, getListMainSourcesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_SourceType_s> list;
    am_types::am_SourceType_L listMainSources;
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSources(list));
    std::sort(list.begin(), list.end(),
		[](const am_SourceType_s& a, const am_SourceType_s& b)
		{
			return (a.sourceID < b.sourceID);
		});
	if((int)result==E_OK)
	{
		am_types::am_SourceType_s item;
		for(std::vector<am_SourceType_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setSourceID(iter->sourceID);
			item.setName(iter->name);
			item.setSourceClassID(iter->sourceClassID);
			am_types::am_Availability_s av;
			CAmConvertAvailablility(iter->availability, av);
			item.setAvailability(av);
			listMainSources.push_back (item);
		}
	}
	_reply(listMainSources, result);
}

void CAmCommandSenderService::getListMainSinkSoundProperties(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, getListMainSinkSoundPropertiesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_MainSoundProperty_s> list;
    am_types::am_MainSoundProperty_L listSoundProperties;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSinkSoundProperties(_sinkID, list));
    if((int)result==E_OK)
	{
		am_types::am_MainSoundProperty_s item;
		for(std::vector<am_MainSoundProperty_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setType(iter->type);
			item.setValue(iter->value);
			listSoundProperties.push_back (item);
		}
	}
    _reply(listSoundProperties, result);
}

void CAmCommandSenderService::getListMainSourceSoundProperties(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, getListMainSourceSoundPropertiesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_MainSoundProperty_s> list;
    am_types::am_MainSoundProperty_L listSoundProperties;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSourceSoundProperties(_sourceID, list));
    if((int)result==E_OK)
	{
		am_types::am_MainSoundProperty_s item;
		for(std::vector<am_MainSoundProperty_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setType(iter->type);
			item.setValue(iter->value);
			listSoundProperties.push_back (item);
		}
	}
    _reply(listSoundProperties, result);
}

void CAmCommandSenderService::getListSourceClasses(const std::shared_ptr<CommonAPI::ClientId> _client, getListSourceClassesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_SourceClass_s> list;
    am_types::am_SourceClass_L listSourceClasses;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListSourceClasses(list));
    if((int)result==E_OK)
	{
		am_types::am_SourceClass_s item;
		for(std::vector<am_SourceClass_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setSourceClassID(iter->sourceClassID);
			item.setName(iter->name);
			am_types::am_ClassProperty_L listClassProperties;
			std::for_each(iter->listClassProperties.begin(), iter->listClassProperties.end(), [&](const am_ClassProperty_s & ref) {
				am_types::am_ClassProperty_s classProp(ref.classProperty, ref.value);
				listClassProperties.push_back(classProp);
			});
			item.setListClassProperties(listClassProperties);
			listSourceClasses.push_back (item);
		}
	}
    _reply(listSourceClasses, result);
}

void CAmCommandSenderService::getListSinkClasses(const std::shared_ptr<CommonAPI::ClientId> _client, getListSinkClassesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_SinkClass_s> list;
    am_types::am_SinkClass_L listSinkClasses;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListSinkClasses(list));
    if((int)result==E_OK)
	{
		am_types::am_SinkClass_s item;
		for(std::vector<am_SinkClass_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setSinkClassID(iter->sinkClassID);
			item.setName(iter->name);
			am_types::am_ClassProperty_L listClassProperties;
			std::for_each(iter->listClassProperties.begin(), iter->listClassProperties.end(), [&](const am_ClassProperty_s & ref) {
				am_types::am_ClassProperty_s classProp(ref.classProperty, ref.value);
				listClassProperties.push_back(classProp);
			});
			item.setListClassProperties(listClassProperties);
			listSinkClasses.push_back (item);
		}
	}
    _reply(listSinkClasses, result);
}

void CAmCommandSenderService::getListSystemProperties(const std::shared_ptr<CommonAPI::ClientId> _client, getListSystemPropertiesReply_t _reply) {
    assert(mpIAmCommandReceive);
    std::vector<am_SystemProperty_s> list;
    am_types::am_SystemProperty_L listSystemProperties;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListSystemProperties(list));
    if((int)result==E_OK)
	{
		am_types::am_SystemProperty_s item;
		for(std::vector<am_SystemProperty_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setType(iter->type);
			item.setValue(iter->value);
			listSystemProperties.push_back (item);
		}
	}
    _reply(listSystemProperties, result);
}

void CAmCommandSenderService::getTimingInformation(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_mainConnectionID_t _mainConnectionID, getTimingInformationReply_t _reply){
    assert(mpIAmCommandReceive);
    am_timeSync_t delay;
    am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getTimingInformation(_mainConnectionID, delay));
    _reply(delay, result);
}

void CAmCommandSenderService::getListMainSinkNotificationConfigurations(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, getListMainSinkNotificationConfigurationsReply_t _reply)
{
	assert(mpIAmCommandReceive);
	std::vector<am_NotificationConfiguration_s> list;
	am_types::am_NotificationConfiguration_L listNotificationConfigurations;
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSinkNotificationConfigurations(_sinkID, list));
    if((int)result==E_OK)
	{
		am_types::am_NotificationConfiguration_s item;
		for(std::vector<am_NotificationConfiguration_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setType(iter->type);
			item.setStatus((am_types::am_NotificationStatus_e::Literal)iter->status);
			item.setParameter(iter->parameter);
			listNotificationConfigurations.push_back (item);
		}
	}
    _reply(listNotificationConfigurations, result);
}

void CAmCommandSenderService::getListMainSourceNotificationConfigurations(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, getListMainSourceNotificationConfigurationsReply_t _reply)
{
	assert(mpIAmCommandReceive);
	std::vector<am_NotificationConfiguration_s> list;
	am_types::am_NotificationConfiguration_L listNotificationConfigurations;
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->getListMainSourceNotificationConfigurations(_sourceID, list));
    if((int)result==E_OK)
	{
		am_types::am_NotificationConfiguration_s item;
		for(std::vector<am_NotificationConfiguration_s>::const_iterator iter = list.begin(); iter!=list.end(); iter++)
		{
			item.setType(iter->type);
			item.setStatus((am_types::am_NotificationStatus_e::Literal)iter->status);
			item.setParameter(iter->parameter);
			listNotificationConfigurations.push_back (item);
		}
	}
    _reply(listNotificationConfigurations, result);
}

void CAmCommandSenderService::setMainSinkNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sinkID_t _sinkID, am_types::am_NotificationConfiguration_s _mainNotificationConfiguration, setMainSinkNotificationConfigurationReply_t _reply)
{
	assert(mpIAmCommandReceive);
	am_NotificationConfiguration_s config = {(int)_mainNotificationConfiguration.getType(),(am_NotificationStatus_e)((int)_mainNotificationConfiguration.getStatus()), _mainNotificationConfiguration.getParameter()};
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setMainSinkNotificationConfiguration(_sinkID, config));
	_reply(result);
}

void CAmCommandSenderService::setMainSourceNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> _client, am_types::am_sourceID_t _sourceID, am_types::am_NotificationConfiguration_s _mainNotificationConfiguration, setMainSourceNotificationConfigurationReply_t _reply)
{
	assert(mpIAmCommandReceive);
	am_NotificationConfiguration_s config = {(int)_mainNotificationConfiguration.getType(),(am_NotificationStatus_e)((int)_mainNotificationConfiguration.getStatus()), _mainNotificationConfiguration.getParameter()};
	am_types::am_Error_e result = CAmConvert2CAPIType(mpIAmCommandReceive->setMainSourceNotificationConfiguration(_sourceID, config));
	_reply(result);
}

} /* namespace am */
