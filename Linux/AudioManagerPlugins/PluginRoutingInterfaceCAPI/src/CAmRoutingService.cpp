/**
 *  Copyright (c) 2012 BMW
 *
 *  \author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013-2015
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
#include <memory>
#include <assert.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "CAmRoutingSenderCommon.h"
#include "CAmCommonAPIWrapper.h"
#include "CAmDltWrapper.h"
#include "CAmRoutingService.h"


namespace am {

const char * CAmRoutingService::DOMAIN_INSTANCES_FILENAME = "domain_instances.cfg";

timespec timespecFromMiliseconds(const unsigned long milisec)
{
	struct timespec req;
	long int nsec = (milisec*1000000);
	req.tv_sec = 0;
	req.tv_nsec = nsec;
	return req;
}

CAmRoutingService::CAmRoutingService():
		mpCAmCAPIWrapper(NULL),
		mpIAmRoutingReceive(NULL),
		mpLookpData(NULL),
		mNumberDomains(0),
		mHandle(0),
		mReady(false)
{
	// TODO Auto-generated constructor stub
}

CAmRoutingService::CAmRoutingService(IAmRoutingReceive *aReceiver, CAmLookupData*   aLookpData, CAmCommonAPIWrapper *aCAPIWrapper):
		mpCAmCAPIWrapper(aCAPIWrapper),
		mpIAmRoutingReceive(aReceiver),
		mpLookpData(aLookpData),
		mNumberDomains(0),
		mHandle(0),
		mReady(false)
{
	// TODO Auto-generated constructor stub
}

CAmRoutingService::~CAmRoutingService() {
	// TODO Auto-generated destructor stub
}

void CAmRoutingService::ackConnect(am_Handle_s handle, am_connectionID_t connectionID, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackConnect(dst, (int)connectionID, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackDisconnect(am_Handle_s handle , am_connectionID_t connectionID, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackDisconnect(dst, (int)connectionID, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSinkVolumeChange(am_Handle_s handle , am_volume_t volume, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSinkVolumeChange(dst, volume, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSourceVolumeChange(am_Handle_s handle, am_volume_t volume, am_Error_e error){
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSourceVolumeChange(dst, volume, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSourceState(am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSourceState(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSinkSoundProperties(am_Handle_s handle, am_Error_e error){
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSinkSoundProperties(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSinkSoundProperty(am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSinkSoundProperty(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSourceSoundProperties(am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSourceSoundProperties(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSetSourceSoundProperty(am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSetSourceSoundProperty(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackCrossFading(am_Handle_s handle, am_HotSink_e hotSink, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackCrossFading(dst, static_cast<am_types::am_HotSink_e::Literal>(hotSink), static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSourceVolumeTick(am_Handle_s handle, am_sourceID_t source, am_volume_t volume) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSourceVolumeTick(dst, source, volume);
}

void CAmRoutingService::ackSinkVolumeTick(am_Handle_s handle, am_sinkID_t sink, am_volume_t volume) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	mpLookpData->ackSinkVolumeTick(dst, sink, volume);
}

void CAmRoutingService::ackSetVolumes(am_Handle_s handle , const std::vector<am_Volumes_s> & listVolumes, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
	am_types::am_Volumes_L list;
	convert_am_types(listVolumes, list);
	mpLookpData->ackSetVolumes(dst, list, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSinkNotificationConfiguration (am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
    mpLookpData->ackSinkNotificationConfiguration(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::ackSourceNotificationConfiguration(am_Handle_s handle, am_Error_e error) {
	assert(mpLookpData);
	am_types::am_Handle_s dst;
	convert_am_types(handle, dst);
    mpLookpData->ackSourceNotificationConfiguration(dst, static_cast<am_types::am_Error_e::Literal>(error));
}

void CAmRoutingService::peekDomain(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekDomainReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_domainID_t domainID;
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->peekDomain(_name, domainID));
	_reply(domainID, error);
}

void CAmRoutingService::registerDomain(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Domain_s _domainData, std::string _returnBusname, std::string _returnInterface, registerDomainReply_t _reply){

	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	assert(mpCAmCAPIWrapper);

	am_types::am_Error_e resultCode;
	am_domainID_t domainID;
	if(_returnBusname.empty())
		CAmRoutingService::getDomainInstance(_domainData.getName(), _returnBusname);

	if(!_returnBusname.empty())
	{
		am_Domain_s converted;
		convert_am_types(_domainData, converted);
		converted.busname = CAmLookupData::BUS_NAME;

		resultCode = ((am_types::am_Error_e::Literal)mpIAmRoutingReceive->registerDomain(converted, domainID));
		if(E_OK==resultCode)
		{
			std::shared_ptr<am_routing_interface::RoutingControlProxy<>> shpSenderProxy = mpCAmCAPIWrapper->buildProxy<am_routing_interface::RoutingControlProxy>("local", _returnBusname);
			mpLookpData->addDomainLookup(domainID, shpSenderProxy);
			if (domainID==2)
				mpIAmRoutingReceive->confirmRoutingReady(mHandle,E_OK);
		}
	}
	else
	{
		resultCode = am_types::am_Error_e::E_NOT_POSSIBLE;
		domainID = 0;
	}
	_reply(domainID, resultCode);
}

void CAmRoutingService::deregisterDomain(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, deregisterDomainReply_t _reply) {
	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	am_types::am_Error_e resultCode((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterDomain(_domainID));
	if(am_types::am_Error_e::E_OK==resultCode)
		mpLookpData->removeDomainLookup(_domainID);
	_reply(resultCode);
}

void CAmRoutingService::registerGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Gateway_s _gatewayData, registerGatewayReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_Gateway_s converted;
	am_gatewayID_t gatewayID;
	convert_am_types(_gatewayData, converted);
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->registerGateway(converted, gatewayID));
	_reply(gatewayID, error);
}

void CAmRoutingService::deregisterGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_gatewayID_t _gatewayID, deregisterGatewayReply_t _reply) {
	am_types::am_Error_e returnError ((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterGateway(_gatewayID));
	_reply(returnError);
}

void CAmRoutingService::registerConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Converter_s _converterData, registerConverterReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_Converter_s converted;
	am_converterID_t converterID;
	convert_am_types(_converterData, converted);
	am_types::am_Error_e error = static_cast<am_types::am_Error_e::Literal>(mpIAmRoutingReceive->registerConverter(converted, converterID));
	_reply(converterID, error);
}

void CAmRoutingService::deregisterConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_converterID_t _converterID, deregisterConverterReply_t _reply) {
	am_types::am_Error_e returnError ((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterConverter(_converterID));
	_reply(returnError);
}

void CAmRoutingService::peekSink(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSinkReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_sinkID_t sinkID;
	am_types::am_Error_e error ((am_types::am_Error_e::Literal)mpIAmRoutingReceive->peekSink(_name, sinkID));
	_reply(sinkID, error);
}

void CAmRoutingService::registerSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Sink_s _sinkData, registerSinkReply_t _reply) {
	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	am_Sink_s converted;
	am_sinkID_t sinkID;
	convert_am_types(_sinkData, converted);
	am_Error_e result = mpIAmRoutingReceive->registerSink(converted, sinkID);
	am_types::am_Error_e error((am_types::am_Error_e::Literal)result);
	if(E_OK==result)
		mpLookpData->addSinkLookup(sinkID, converted.domainID);
	_reply(sinkID, error);
}

void CAmRoutingService::deregisterSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, deregisterSinkReply_t _reply) {
	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	am_types::am_Error_e returnError((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterSink(_sinkID));
	if((int)returnError==E_OK)
		mpLookpData->removeSinkLookup(_sinkID);
	_reply(returnError);
}

void CAmRoutingService::peekSource(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSourceReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_sourceID_t sourceID;
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->peekSource(_name, sourceID));
	_reply(sourceID, error);
}

void CAmRoutingService::registerSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Source_s _sourceData, registerSourceReply_t _reply) {
	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	am_Source_s converted;
	am_sourceID_t sourceID;
	convert_am_types(_sourceData, converted);
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->registerSource(converted, sourceID));
	if((int)error==E_OK)
		mpLookpData->addSourceLookup(sourceID, _sourceData.getDomainID());
	_reply(sourceID, error);
}

void CAmRoutingService::deregisterSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, deregisterSourceReply_t _reply) {
	assert(mpIAmRoutingReceive);
	assert(mpLookpData);
	am_types::am_Error_e returnError((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterSource(_sourceID));
	if((int)returnError==E_OK)
		mpLookpData->removeSourceLookup(_sourceID);
	_reply(returnError);
}

void CAmRoutingService::registerCrossfader(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Crossfader_s _crossfaderData, registerCrossfaderReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_Crossfader_s converted;
	am_crossfaderID_t crossfaderID;
	convert_am_types(_crossfaderData, converted);
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->registerCrossfader(converted, crossfaderID));
	if((int)error==E_OK)
		mpLookpData->addCrossfaderLookup(crossfaderID, _crossfaderData.getSourceID());
	_reply(crossfaderID, error);
}

void CAmRoutingService::deregisterCrossfader(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_crossfaderID_t _crossfaderID, deregisterCrossfaderReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_types::am_Error_e returnError((am_types::am_Error_e::Literal)mpIAmRoutingReceive->deregisterCrossfader(_crossfaderID));
	if((int)returnError==E_OK)
		mpLookpData->removeCrossfaderLookup(_crossfaderID);
	_reply(returnError);
}

void CAmRoutingService::peekSourceClassID(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSourceClassIDReply_t _reply) {
	am_sourceClass_t sourceClassID;
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->peekSourceClassID(_name, sourceClassID));
	_reply(sourceClassID, error);
}

void CAmRoutingService::peekSinkClassID(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSinkClassIDReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_sinkClass_t sinkClassID;
	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->peekSinkClassID(_name, sinkClassID));
	_reply(sinkClassID, error);
}

void CAmRoutingService::hookInterruptStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_InterruptState_e _interruptState, hookInterruptStatusChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	mpIAmRoutingReceive->hookInterruptStatusChange(_sourceID, static_cast<am_InterruptState_e>((int)_interruptState));
	_reply();
}

void CAmRoutingService::hookDomainRegistrationComplete(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, hookDomainRegistrationCompleteReply_t _reply) {
	assert(mpIAmRoutingReceive != NULL);
	mpIAmRoutingReceive->hookDomainRegistrationComplete(_domainID);
	_reply();
}

void CAmRoutingService::hookSinkAvailablityStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_Availability_s _availability, hookSinkAvailablityStatusChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_Availability_s am_avialabilty;
	convert_am_types(_availability, am_avialabilty);
	mpIAmRoutingReceive->hookSinkAvailablityStatusChange(_sinkID, am_avialabilty);
	_reply();
}

void CAmRoutingService::hookSourceAvailablityStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_Availability_s _availability, hookSourceAvailablityStatusChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_Availability_s am_availabilty;
	convert_am_types(_availability, am_availabilty);
	mpIAmRoutingReceive->hookSourceAvailablityStatusChange(_sourceID, am_availabilty);
	_reply();
}

void CAmRoutingService::hookDomainStateChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, am_types::am_DomainState_e _domainState, hookDomainStateChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_DomainState_e am_domainState = static_cast<am_DomainState_e>((int)_domainState);
	mpIAmRoutingReceive->hookDomainStateChange(_domainID, am_domainState);
	_reply();
}

void CAmRoutingService::hookTimingInformationChanged(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_connectionID_t _connectionID, am_types::am_timeSync_t _delay, hookTimingInformationChangedReply_t _reply) {
	assert(mpIAmRoutingReceive);
	mpIAmRoutingReceive->hookTimingInformationChanged(_connectionID, _delay);
	_reply();
}

void CAmRoutingService::sendChangedData(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_EarlyData_L _earlyData, sendChangedDataReply_t _reply) {

	assert(mpIAmRoutingReceive);
	std::vector<am_EarlyData_s> dest;
	convert_am_types(_earlyData,dest);
	mpIAmRoutingReceive->sendChangedData(dest);
	_reply();
}

void CAmRoutingService::updateGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_gatewayID_t _gatewayID, am_types::am_ConnectionFormat_L _listSourceFormats, am_types::am_ConnectionFormat_L _listSinkFormats, am_types::am_Convertion_L _convertionMatrix, updateGatewayReply_t _reply) {

	assert(mpIAmRoutingReceive);
	std::vector<am_CustomConnectionFormat_t> destinationSourceConnectionFormats;
	convert_am_types(_listSourceFormats, destinationSourceConnectionFormats);

	std::vector<am_CustomConnectionFormat_t> destinationSinkConnectionFormats;
	convert_am_types(_listSinkFormats, destinationSinkConnectionFormats);

	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->updateGateway(_gatewayID, destinationSourceConnectionFormats, destinationSinkConnectionFormats, _convertionMatrix));
	_reply(error);
}

void CAmRoutingService::updateConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_converterID_t _converterID, am_types::am_ConnectionFormat_L _listSourceFormats, am_types::am_ConnectionFormat_L _listSinkFormats, am_types::am_Convertion_L _convertionMatrix, updateConverterReply_t _reply) {

	assert(mpIAmRoutingReceive);
	std::vector<am_CustomConnectionFormat_t> destinationSourceConnectionFormats;
	convert_am_types(_listSourceFormats, destinationSourceConnectionFormats);

	std::vector<am_CustomConnectionFormat_t> destinationSinkConnectionFormats;
	convert_am_types(_listSinkFormats, destinationSinkConnectionFormats);

	am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->updateConverter(_converterID, destinationSourceConnectionFormats, destinationSinkConnectionFormats, _convertionMatrix));
	_reply(error);
}

void CAmRoutingService::updateSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_sinkClass_t _sinkClassID, am_types::am_SoundProperty_L _listSoundProperties, am_types::am_ConnectionFormat_L _listConnectionFormats, am_types::am_MainSoundProperty_L _listMainSoundProperties, updateSinkReply_t _reply) {
	assert(mpIAmRoutingReceive);
	std::vector<am_SoundProperty_s> dstListSoundProperties;
    convert_am_types(_listSoundProperties, dstListSoundProperties);
    std::vector<am_CustomConnectionFormat_t> dstListSinkConnectionFormats;
    convert_am_types(_listConnectionFormats, dstListSinkConnectionFormats);
    std::vector<am_MainSoundProperty_s> dstListMainSoundProperties;
    convert_am_types(_listMainSoundProperties, dstListMainSoundProperties);
    am_types::am_Error_e error((am_types::am_Error_e::Literal)mpIAmRoutingReceive->updateSink( _sinkID, _sinkClassID, dstListSoundProperties,dstListSinkConnectionFormats,dstListMainSoundProperties));
	_reply(error);
}

void CAmRoutingService::updateSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_sourceClass_t _sourceClassID, am_types::am_SoundProperty_L _listSoundProperties, am_types::am_ConnectionFormat_L _listConnectionFormats, am_types::am_MainSoundProperty_L _listMainSoundProperties, updateSourceReply_t _reply) {
	assert(mpIAmRoutingReceive);
	std::vector<am_SoundProperty_s> dstListSoundProperties;
    convert_am_types(_listSoundProperties, dstListSoundProperties);
    std::vector<am_CustomConnectionFormat_t> dstListSinkConnectionFormats;
    convert_am_types(_listConnectionFormats, dstListSinkConnectionFormats);
    std::vector<am_MainSoundProperty_s> dstListMainSoundProperties;
    convert_am_types(_listMainSoundProperties, dstListMainSoundProperties);
    am_types::am_Error_e error = (am_types::am_Error_e::Literal)mpIAmRoutingReceive->updateSource( _sourceID, _sourceClassID, dstListSoundProperties,dstListSinkConnectionFormats,dstListMainSoundProperties);
	_reply(error);
}

void CAmRoutingService::hookSinkNotificationDataChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_NotificationPayload_s _payload, hookSinkNotificationDataChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_NotificationPayload_s converted;
	convert_am_types(_payload, converted);
	mpIAmRoutingReceive->hookSinkNotificationDataChange(_sinkID, converted);
	_reply();
}

void CAmRoutingService::hookSourceNotificationDataChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_NotificationPayload_s _payload, hookSourceNotificationDataChangeReply_t _reply) {
	assert(mpIAmRoutingReceive);
	am_NotificationPayload_s converted;
	convert_am_types(_payload, converted);
	mpIAmRoutingReceive->hookSourceNotificationDataChange(_sourceID, converted);
	_reply();
}

void CAmRoutingService::confirmRoutingRundown(const std::shared_ptr<CommonAPI::ClientId>, std::string _domainName, confirmRoutingRundownReply_t _reply)
{
	mNumberDomains--;
	if (mNumberDomains==0)
		mpIAmRoutingReceive->confirmRoutingRundown(mHandle,E_OK);
	_reply();
}

void CAmRoutingService::gotRundown(int16_t numberDomains, uint16_t handle)
{
	mReady=false;
    mNumberDomains=numberDomains;
    mHandle=handle;
}


void CAmRoutingService::parseConfigFile(std::istream & cfgfile, std::map<std::string, std::string> & options)
{
    for (std::string line; std::getline(cfgfile, line); )
    {
        std::istringstream iss(line);
        std::string id, eq, val;

        bool error = false;

        if (!(iss >> id))
            error = true;
        else if (id[0] == '#')
            continue;
        else if (!(iss >> eq >> val >> std::ws) || eq != "=" || iss.get() != EOF)
            error = true;

        if (error)
        	log(&GetDefaultRoutingDltContext(), DLT_LOG_ERROR, "config file is bad formatted!");
        else
            options[id] = val;
    }
}

bool CAmRoutingService::getDomainInstance(const std::string & domainName, std::string & instanceID)
{
	static std::map<std::string, std::string> glDomainInstances;
	static bool glDomainInstancesLoaded = false;
	if(!glDomainInstancesLoaded)
	{
		std::string path(std::string(AM_SHARE_FOLDER)+std::string("/")+std::string(CAmRoutingService::DOMAIN_INSTANCES_FILENAME));
		std::fstream fs(path, std::ios_base::in);
		if(fs.good())
		{
			parseConfigFile(fs, glDomainInstances);
		}
		else
		{
			log(&GetDefaultRoutingDltContext(), DLT_LOG_ERROR, path, " doesn't exist");
		}
		glDomainInstancesLoaded = true;
	}
	auto it = glDomainInstances.find(domainName);
	if(it!=glDomainInstances.end())
	{
		instanceID = it->second;
		return true;
	}
	return false;
}

} /* namespace am */
