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

#ifndef CAMROUTINGSERVICE_H_
#define CAMROUTINGSERVICE_H_


#include <v1/org/genivi/am/routinginterface/RoutingControlObserverStubDefault.hpp>
#include "IAmRouting.h"
#include "CAmCommonAPIWrapper.h"
#include "CAmLookupData.h"
#include "CAmRoutingSenderCommon.h"

namespace am {


/** Routing interface stub implementation.
 * This class is the routing interface service for the Audio Manager.
 */
class CAmRoutingService: public am_routing_interface::RoutingControlObserverStubDefault {
	CAmCommonAPIWrapper *mpCAmCAPIWrapper; ///< pointer to common-api wrapper
	IAmRoutingReceive* mpIAmRoutingReceive; ///< pointer to the routing receive interface
	CAmLookupData*   mpLookpData;			///< pointer to the plugin's lookup mechanism implementation
    int16_t mNumberDomains;	///< int number of registred domains
    uint16_t mHandle;		///< unsigned current handle
    bool mReady;			///< bool whether the service is in ready state or not

    static void parseConfigFile(std::istream & cfgfile, std::map<std::string, std::string> & options);

    CAmRoutingService();
public:
	CAmRoutingService(IAmRoutingReceive *aReceiver, CAmLookupData*   aLookpData, CAmCommonAPIWrapper *aCAPIWrapper);
	virtual ~CAmRoutingService();

    static const char * DOMAIN_INSTANCES_FILENAME;
    static bool getDomainInstance(const std::string & domainName, std::string & instanceID);

	void ackConnect(am_Handle_s handle, am_connectionID_t connectionID, am_Error_e error);
	void ackDisconnect(am_Handle_s handle, am_connectionID_t connectionID, am_Error_e error);
	void ackSetSinkVolumeChange(am_Handle_s handle, am_volume_t volume, am_Error_e error);
	void ackSetSourceVolumeChange(am_Handle_s handle, am_volume_t volume, am_Error_e error);
	void ackSetSourceState(am_Handle_s handle, am_Error_e error);
	void ackSetSinkSoundProperties(am_Handle_s handle, am_Error_e error);
	void ackSetSinkSoundProperty(am_Handle_s handle, am_Error_e error);
	void ackSetSourceSoundProperties(am_Handle_s handle, am_Error_e error);
	void ackSetSourceSoundProperty(am_Handle_s handle, am_Error_e error);
	void ackCrossFading(am_Handle_s handle, am_HotSink_e hotSink, am_Error_e error);
	void ackSourceVolumeTick(am_Handle_s handle, am_sourceID_t source, am_volume_t volume);
	void ackSinkVolumeTick(am_Handle_s handle, am_sinkID_t sink, am_volume_t volume);
	void ackSetVolumes(am_Handle_s handle , const std::vector<am_Volumes_s> & listVolumes, am_Error_e error);
	void ackSinkNotificationConfiguration (am_Handle_s handle, am_Error_e error);
	void ackSourceNotificationConfiguration(am_Handle_s handle, am_Error_e error);

	/** Stub overwritten methods.
	 *
	 */
	void peekDomain(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekDomainReply_t _reply);

	void registerDomain(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Domain_s _domainData, std::string _returnBusname, std::string _returnInterface, registerDomainReply_t _reply);

	void deregisterDomain(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, deregisterDomainReply_t _reply);

	void registerGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Gateway_s _gatewayData, registerGatewayReply_t _reply);

	void deregisterGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_gatewayID_t _gatewayID, deregisterGatewayReply_t _reply);

	void registerConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Converter_s _converterData, registerConverterReply_t _reply);

	void deregisterConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_converterID_t _converterID, deregisterConverterReply_t _reply);

	void peekSink(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSinkReply_t _reply);

	void registerSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Sink_s _sinkData, registerSinkReply_t _reply);

	void deregisterSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, deregisterSinkReply_t _reply);

	void peekSource(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSourceReply_t _reply);

	void registerSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Source_s _sourceData, registerSourceReply_t _reply);

	void deregisterSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, deregisterSourceReply_t _reply);

	void registerCrossfader(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_Crossfader_s _crossfaderData, registerCrossfaderReply_t _reply);

	void deregisterCrossfader(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_crossfaderID_t _crossfaderID, deregisterCrossfaderReply_t _reply);

	void peekSourceClassID(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSourceClassIDReply_t _reply);

	void peekSinkClassID(const std::shared_ptr<CommonAPI::ClientId>, std::string _name, peekSinkClassIDReply_t _reply);

	void hookInterruptStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_InterruptState_e _interruptState, hookInterruptStatusChangeReply_t _reply);

	void hookDomainRegistrationComplete(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, hookDomainRegistrationCompleteReply_t _reply);

	void hookSinkAvailablityStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_Availability_s _availability, hookSinkAvailablityStatusChangeReply_t _reply);

	void hookSourceAvailablityStatusChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_Availability_s _availability, hookSourceAvailablityStatusChangeReply_t _reply);

	void hookDomainStateChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_domainID_t _domainID, am_types::am_DomainState_e _domainState, hookDomainStateChangeReply_t _reply);

	void hookTimingInformationChanged(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_connectionID_t _connectionID, am_types::am_timeSync_t _delay, hookTimingInformationChangedReply_t _reply);

	void sendChangedData(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_EarlyData_L _earlyData, sendChangedDataReply_t _reply);

	void updateGateway(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_gatewayID_t _gatewayID, am_types::am_ConnectionFormat_L _listSourceFormats, am_types::am_ConnectionFormat_L _listSinkFormats, am_types::am_Convertion_L _convertionMatrix, updateGatewayReply_t _reply);

	void updateConverter(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_converterID_t _converterID, am_types::am_ConnectionFormat_L _listSourceFormats, am_types::am_ConnectionFormat_L _listSinkFormats, am_types::am_Convertion_L _convertionMatrix, updateConverterReply_t _reply);

	void updateSink(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_sinkClass_t _sinkClassID, am_types::am_SoundProperty_L _listSoundProperties, am_types::am_ConnectionFormat_L _listConnectionFormats, am_types::am_MainSoundProperty_L _listMainSoundProperties, updateSinkReply_t _reply);

	void updateSource(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_sourceClass_t _sourceClassID, am_types::am_SoundProperty_L _listSoundProperties, am_types::am_ConnectionFormat_L _listConnectionFormats, am_types::am_MainSoundProperty_L _listMainSoundProperties, updateSourceReply_t _reply);

	void hookSinkNotificationDataChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sinkID_t _sinkID, am_types::am_NotificationPayload_s _payload, hookSinkNotificationDataChangeReply_t _reply);

	void hookSourceNotificationDataChange(const std::shared_ptr<CommonAPI::ClientId>, am_types::am_sourceID_t _sourceID, am_types::am_NotificationPayload_s _payload, hookSourceNotificationDataChangeReply_t _reply);

	void confirmRoutingRundown(const std::shared_ptr<CommonAPI::ClientId>, std::string _domainName, confirmRoutingRundownReply_t _reply);

	void gotRundown(int16_t numberDomains, uint16_t handle);

	void setHandle(uint16_t handle) {mHandle=handle;}

};

} /* namespace am */
#endif /* CAMROUTINGSERVICE_H_ */
