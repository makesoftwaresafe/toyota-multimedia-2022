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

#ifndef CAMROUTINGSENDERSERVICE_H_
#define CAMROUTINGSENDERSERVICE_H_

#include "audiomanagertypes.h"
#include "../include/CAmRoutingSenderCommon.h"
#include <v1/org/genivi/am/routinginterface/RoutingControlStubDefault.hpp>
#include <v1/org/genivi/am/routinginterface/RoutingControlObserverProxy.hpp>
#include <v1/org/genivi/am/routinginterface/RoutingControlProxy.hpp>


namespace am {


class CAmCommonAPIWrapper;

#define CAPI_SENDER_INSTANCE  	"RoutingControlInstance2"
#define CAPI_SENDER_PATH 		"/org/genivi/audiomanager/routingsender"

#define CAPI_ROUTING_INSTANCE   "org.genivi.audiomanager"
#define CAPI_ROUTING_PATH 		 "/org/genivi/audiomanager/routinginterface"


/** Test class implementing the routing sender service.
*
*	Please see cmake/CAmRoutingSenderService.h.in	
*/
class CAmTestRoutingSenderService : public am_routing_interface::RoutingControlStubDefault {

	am_types::am_Domain_s mDomainData;
	am_types::am_Handle_s mAbortedHandle;
	bool mIsServiceAvailable;
	bool mIsDomainRegistred;
	bool mIsReady;
	CAmCommonAPIWrapper *mpWrapper;
	std::shared_ptr<am_routing_interface::RoutingControlObserverProxy<> > mRoutingInterfaceProxy;

public:

	CAmTestRoutingSenderService();
	CAmTestRoutingSenderService(CAmCommonAPIWrapper * aWrapper);
	CAmTestRoutingSenderService(CAmCommonAPIWrapper * aWrapper, std::shared_ptr<am_routing_interface::RoutingControlObserverProxy<> > anObserverProxy);
	virtual ~CAmTestRoutingSenderService();

	am_types::am_Domain_s & getDomainData() { return mDomainData; };
	am_types::am_Handle_s & getAbortedHandle() { return mAbortedHandle; };
	bool getIsReady() const { return mIsReady; };
	bool requestIsReady();

	void onServiceStatusEvent(const CommonAPI::AvailabilityStatus& serviceStatus);
	void onRoutingReadyRundown();
	void onRoutingReadyEvent();
	bool registerDomain();
	bool deregisterDomain();
	void setAbortHandle(am_types::am_Handle_s handle);
	am_types::am_Error_e errorForHandle(const am_types::am_Handle_s & handle);

	void asyncAbort(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, asyncAbortReply_t _reply);
	void asyncConnect(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_connectionID_t _connectionID, am_types::am_sourceID_t _sourceID, am_types::am_sinkID_t _sinkID, am_types::am_ConnectionFormat_pe _connectionFormat, asyncConnectReply_t _reply);
	void asyncDisconnect(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_connectionID_t _connectionID, asyncDisconnectReply_t _reply);
	void asyncSetVolumes(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_Volumes_L _volumes, asyncSetVolumesReply_t _reply);
	void asyncCrossFade(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_crossfaderID_t _crossfaderID, am_types::am_HotSink_e _hotSink, am_types::am_RampType_pe _rampType, am_types::am_time_t _time, asyncCrossFadeReply_t _reply);
	void asyncSetSinkVolume(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_volume_t _volume, am_types::am_RampType_pe _ramp, am_types::am_time_t _time, asyncSetSinkVolumeReply_t _reply);
	void asyncSetSourceVolume(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_volume_t _volume, am_types::am_RampType_pe _ramp, am_types::am_time_t _time, asyncSetSourceVolumeReply_t _reply);
	void asyncSetSinkSoundProperty(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_SoundProperty_s _soundProperty, asyncSetSinkSoundPropertyReply_t _reply);
	void asyncSetSinkSoundProperties(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_SoundProperty_L _listSoundProperties, asyncSetSinkSoundPropertiesReply_t _reply);
	void asyncSetSourceSoundProperty(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SoundProperty_s _soundProperty, asyncSetSourceSoundPropertyReply_t _reply);
	void asyncSetSourceSoundProperties(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SoundProperty_L _listSoundProperties, asyncSetSourceSoundPropertiesReply_t _reply);
	void asyncSetSourceState(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SourceState_e _state, asyncSetSourceStateReply_t _reply);
	void asyncSetSinkNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_NotificationConfiguration_s _notificationConfiguration, asyncSetSinkNotificationConfigurationReply_t _reply);
	void asyncSetSourceNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_NotificationConfiguration_s _notificationConfiguration, asyncSetSourceNotificationConfigurationReply_t _reply);
	void setDomainState(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_domainID_t _domainID, am_types::am_DomainState_e _domainState, setDomainStateReply_t _reply);

};


} /* namespace am */
#endif /* CAMROUTINGSENDERSERVICE_H_ */
