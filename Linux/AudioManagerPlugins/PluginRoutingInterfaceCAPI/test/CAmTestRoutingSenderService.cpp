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

#include  <stdint.h>
#include  <limits.h>
#include "CAmCommonAPIWrapper.h"
#include "CAmDltWrapper.h"
#include "CAmTestRoutingSenderService.h"


namespace am {


CAmTestRoutingSenderService::CAmTestRoutingSenderService(CAmCommonAPIWrapper * aWrapper):
		mDomainData(), mAbortedHandle(), mIsDomainRegistred(false), mIsServiceAvailable(0), mIsReady(0), mpWrapper(aWrapper), mRoutingInterfaceProxy(NULL)
{
	mAbortedHandle.setHandle(0xFFFF);
}

CAmTestRoutingSenderService::CAmTestRoutingSenderService():
		mDomainData(), mAbortedHandle(), mIsDomainRegistred(false), mIsServiceAvailable(0), mIsReady(0), mpWrapper(NULL), mRoutingInterfaceProxy(NULL)
{

}

CAmTestRoutingSenderService::CAmTestRoutingSenderService(CAmCommonAPIWrapper * aWrapper,
						std::shared_ptr<am_routing_interface::RoutingControlObserverProxy<> > anObserverProxy):
							mDomainData(),
							mAbortedHandle(),
							mIsDomainRegistred(false),
							mIsServiceAvailable(0),
							mIsReady(0),
							mpWrapper(aWrapper),
							mRoutingInterfaceProxy(anObserverProxy)
{
    mDomainData.setName("TestDomain");
    mDomainData.setBusname("TestDomain");
    mDomainData.setComplete(true);
    mDomainData.setDomainID(0);
    mDomainData.setEarly(false);
    mDomainData.setNodename("TestDomain");
    mDomainData.setState(am_types::am_DomainState_e(am_types::am_DomainState_e::DS_CONTROLLED));

	mAbortedHandle.setHandle(0xFFFF);
	mRoutingInterfaceProxy->getProxyStatusEvent().subscribe(std::bind(&CAmTestRoutingSenderService::onServiceStatusEvent,this,std::placeholders::_1));
}

CAmTestRoutingSenderService::~CAmTestRoutingSenderService() {
	mpWrapper = NULL;
	if(mRoutingInterfaceProxy)
		mRoutingInterfaceProxy.reset();
}

void CAmTestRoutingSenderService::onServiceStatusEvent(const CommonAPI::AvailabilityStatus& serviceStatus)
{
	mIsDomainRegistred = false;
	mIsServiceAvailable = false;
	mIsReady = false;
    std::stringstream  avail;
    avail  << "(" << static_cast<int>(serviceStatus) << ")";
    logInfo("CAmTestRoutingSenderService::onServiceStatusEvent -> status changed to ", avail.str());
    std::cout << std::endl << "CAmTestRoutingSenderService::onServiceStatusEvent -> service status changed to " << avail.str() << std::endl;
    if(serviceStatus==CommonAPI::AvailabilityStatus::AVAILABLE)
    {
    	mIsServiceAvailable = true;
    }
}

void CAmTestRoutingSenderService::onRoutingReadyEvent()
{
	logInfo(__PRETTY_FUNCTION__);
	mIsReady = true;
}

void CAmTestRoutingSenderService::onRoutingReadyRundown()
{
    logInfo(__PRETTY_FUNCTION__);
    mIsReady = true;
    mIsDomainRegistred = false;
}

bool CAmTestRoutingSenderService::requestIsReady()
{
	CommonAPI::CallStatus callStatus;
	am_types::am_RoutingReady_e readyAttr;
	mRoutingInterfaceProxy->getRoutingReadyAttribute().getValue(callStatus,readyAttr);
	if (callStatus!=CommonAPI::CallStatus::SUCCESS)
	{
		logError(__PRETTY_FUNCTION__,"Could not get RoutingReady");
		return false;
	}
	else
	{
		logInfo(__PRETTY_FUNCTION__,"RoutingReady has been retrieved");
		mIsReady = (int)readyAttr == am_types::am_RoutingReady_e::RR_READY ? true : false;
		return true;
	}
}

void CAmTestRoutingSenderService::setAbortHandle(am_types::am_Handle_s handle)
{
	mAbortedHandle=handle;
}

bool CAmTestRoutingSenderService::registerDomain()
{
	requestIsReady();
	logInfo(__PRETTY_FUNCTION__,"start registering domain ", mDomainData.getName(), mIsDomainRegistred, mIsServiceAvailable, mIsReady);
	if( mIsDomainRegistred || !mIsServiceAvailable || !mIsReady )
		return false;
    am_types::am_Error_e error;
    am_types::am_domainID_t dID(0);
    CommonAPI::CallStatus callStatus;
    mRoutingInterfaceProxy->registerDomain(mDomainData,
    									   CAPI_SENDER_INSTANCE,
    									   "",
    									   callStatus,
										   dID,
    									   error);
    mDomainData.setDomainID(dID);
    logInfo("Domain: got domainID",  (int)dID, " return:", (int)error);
    mIsDomainRegistred = (callStatus==CommonAPI::CallStatus::SUCCESS && (int)error==am_Error_e::E_OK) ;
    logInfo(__PRETTY_FUNCTION__, "Done ", mIsDomainRegistred);
    return mIsDomainRegistred;
}

bool CAmTestRoutingSenderService::deregisterDomain()
{
	logInfo(__PRETTY_FUNCTION__,"deregistering domain ", mDomainData.getName(), mIsDomainRegistred, mIsServiceAvailable, mIsReady);
	if( !mIsDomainRegistred || !mIsServiceAvailable || !mIsReady )
		return false;
    am_types::am_Error_e error;
    am_types::am_domainID_t dID(0);
    CommonAPI::CallStatus callStatus;
    mRoutingInterfaceProxy->deregisterDomain(mDomainData.getDomainID(), callStatus, error);
    mIsDomainRegistred = false;
    bool result = (callStatus==CommonAPI::CallStatus::SUCCESS && (int)error==am_Error_e::E_OK);
    logInfo(__PRETTY_FUNCTION__, "Done. ", result);
    return result;
}

am_types::am_Error_e CAmTestRoutingSenderService::errorForHandle(const am_types::am_Handle_s & handle)
{
	am_types::am_Error_e error(am_types::am_Error_e::E_OK);
	if(handle==mAbortedHandle && (int)mAbortedHandle.getHandle()!=0xFFFF)
	{
		error = am_types::am_Error_e::E_ABORTED;
		mAbortedHandle.setHandle(0xFFFF);
	}
	return error;
}

void CAmTestRoutingSenderService::asyncAbort(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, asyncAbortReply_t _reply)
{
   logInfo(__FUNCTION__, " called");
   mAbortedHandle = _handle;
   _reply();
}

void CAmTestRoutingSenderService::asyncConnect(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_connectionID_t _connectionID, am_types::am_sourceID_t _sourceID, am_types::am_sinkID_t _sinkID, am_types::am_ConnectionFormat_pe _connectionFormat, asyncConnectReply_t _reply)
{
	logInfo(__FUNCTION__, " called");
	fireAckConnectSelective(_handle, _connectionID, errorForHandle(_handle));
   _reply();
}

void CAmTestRoutingSenderService::asyncDisconnect(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_connectionID_t _connectionID, asyncDisconnectReply_t _reply)
{
	logInfo(__FUNCTION__, " called");
	fireAckDisconnectSelective(_handle, _connectionID, errorForHandle(_handle));
   _reply();
}

void CAmTestRoutingSenderService::asyncSetVolumes(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_Volumes_L _volumes, asyncSetVolumesReply_t _reply){
	logInfo(__FUNCTION__, " called");
	am_types::am_Error_e error(am_types::am_Error_e::E_OK);
	fireAckSetVolumesSelective(_handle, _volumes, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncCrossFade(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_crossfaderID_t _crossfaderID, am_types::am_HotSink_e _hotSink, am_types::am_RampType_pe _rampType, am_types::am_time_t _time, asyncCrossFadeReply_t _reply)
{
    logInfo(__FUNCTION__, " called");
	fireAckCrossFadingSelective(_handle, _hotSink, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSinkVolume(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_volume_t _volume, am_types::am_RampType_pe _ramp, am_types::am_time_t _time, asyncSetSinkVolumeReply_t _reply)
{
  	logInfo(__FUNCTION__, " called");
	am_types::am_Error_e error(am_types::am_Error_e::E_OK);
	fireAckSetSinkVolumeChangeSelective(_handle, _volume, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSourceVolume(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_volume_t _volume, am_types::am_RampType_pe _ramp, am_types::am_time_t _time, asyncSetSourceVolumeReply_t _reply)
{
  	logInfo(__FUNCTION__, " called");
	am_types::am_Error_e error(am_types::am_Error_e::E_OK);
	fireAckSetSourceVolumeChangeSelective(_handle, _volume, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSinkSoundProperty(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_SoundProperty_s _soundProperty, asyncSetSinkSoundPropertyReply_t _reply)
{
	logInfo(__FUNCTION__, " called");
	fireAckSetSinkSoundPropertySelective(_handle, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSinkSoundProperties(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_SoundProperty_L _listSoundProperties, asyncSetSinkSoundPropertiesReply_t _reply) {
    logInfo(__FUNCTION__, " called");
	fireAckSetSinkSoundPropertiesSelective(_handle, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSourceSoundProperties(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SoundProperty_L _listSoundProperties, asyncSetSourceSoundPropertiesReply_t _reply) {
	logInfo(__FUNCTION__, " called");
	fireAckSetSourceSoundPropertiesSelective(_handle, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSourceSoundProperty(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SoundProperty_s _soundProperty, asyncSetSourceSoundPropertyReply_t _reply) {
    logInfo(__FUNCTION__, " called");
	fireAckSetSourceSoundPropertySelective(_handle, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSourceState(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_SourceState_e _state, asyncSetSourceStateReply_t _reply) {
	logInfo(__FUNCTION__, " called");
	fireAckSetSourceStateSelective(_handle, errorForHandle(_handle));
	_reply();
}

void CAmTestRoutingSenderService::asyncSetSinkNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sinkID_t _sinkID, am_types::am_NotificationConfiguration_s _notificationConfiguration, asyncSetSinkNotificationConfigurationReply_t _reply) {
	logInfo(__FUNCTION__, " called");
	fireAckSinkNotificationConfigurationSelective(_handle, errorForHandle(_handle));
}

void CAmTestRoutingSenderService::asyncSetSourceNotificationConfiguration(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_Handle_s _handle, am_types::am_sourceID_t _sourceID, am_types::am_NotificationConfiguration_s _notificationConfiguration, asyncSetSourceNotificationConfigurationReply_t _reply) {
   logInfo(__FUNCTION__, " called");
   fireAckSourceNotificationConfigurationSelective(_handle, errorForHandle(_handle));
   _reply();
}

void CAmTestRoutingSenderService::setDomainState(const std::shared_ptr<CommonAPI::ClientId> , am_types::am_domainID_t _domainID, am_types::am_DomainState_e _domainState, setDomainStateReply_t _reply) {
	logInfo(__FUNCTION__, " called");
	CommonAPI::CallStatus callStatus;
	am_types::am_Error_e error (am_types::am_Error_e::E_OK);
	mRoutingInterfaceProxy->hookDomainStateChange(_domainID, _domainState, callStatus);
	_reply(error);
}

} /* namespace org */
