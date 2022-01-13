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

#include <cassert>
#include <map>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include "CAmDltWrapper.h"
#include "CAmRoutingSenderCAPI.h"
#include "CAmRoutingSenderCommon.h"




namespace am
{

extern "C" IAmRoutingSend* PluginRoutingInterfaceCAPIFactory()
{
    CAmDltWrapper::instance()->registerContext(GetDefaultRoutingDltContext(), "DRS", "Common-API Plugin");
    return (new CAmRoutingSenderCAPI());
}

extern "C" void destroyPluginRoutingInterfaceCAPI(IAmRoutingSend* routingSendInterface)
{
    delete routingSendInterface;
}


const char * CAmRoutingSenderCAPI::ROUTING_INSTANCE = DBUS_SERVICE_PREFIX;
const char * CAmRoutingSenderCAPI::DEFAULT_DOMAIN = "local";


CAmRoutingSenderCAPI::CAmRoutingSenderCAPI() :
                mIsServiceStarted(false),
                mLookupData(),
				mpCAmCAPIWrapper(NULL), //
				mpIAmRoutingReceive(NULL),
				mService()
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,"RoutingSender constructed");
}

CAmRoutingSenderCAPI::~CAmRoutingSenderCAPI()
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "RoutingSender deallocate");
    CAmDltWrapper::instance()->unregisterContext(GetDefaultRoutingDltContext());
    tearDownInterface(mpIAmRoutingReceive);
}

am_Error_e CAmRoutingSenderCAPI::startService(IAmRoutingReceive* pIAmRoutingReceive)
{
	if(!mpCAmCAPIWrapper)
		mpCAmCAPIWrapper=CAPI;
    assert(mpCAmCAPIWrapper!=NULL);
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__);
	if(!mIsServiceStarted)
	{
		assert(pIAmRoutingReceive);
		mLookupData.setIAmRoutingReceive(pIAmRoutingReceive);
		mService = std::make_shared<CAmRoutingService>(pIAmRoutingReceive, &mLookupData, mpCAmCAPIWrapper);
	    mService->setRoutingReadyAttribute(am_types::am_RoutingReady_e::RR_UNKNOWN);
		//Registers the service
		if( false == mpCAmCAPIWrapper->registerService(mService, CAmRoutingSenderCAPI::DEFAULT_DOMAIN, CAmRoutingSenderCAPI::ROUTING_INSTANCE) )//"AudioManager"
		{
			log(&GetDefaultRoutingDltContext(), DLT_LOG_ERROR, "Can't register stub ", CAmRoutingSenderCAPI::DEFAULT_DOMAIN, am_routing_interface::RoutingControlObserver::getInterface(), CAmRoutingSenderCAPI::ROUTING_INSTANCE);
			return (E_NOT_POSSIBLE);
		}
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "Stub has been successful registered!", CAmRoutingSenderCAPI::DEFAULT_DOMAIN, am_routing_interface::RoutingControlObserver::getInterface(), CAmRoutingSenderCAPI::ROUTING_INSTANCE);
		mIsServiceStarted = true;
	}
    return (E_OK);
}

am_Error_e CAmRoutingSenderCAPI::startupInterface(IAmRoutingReceive* pIAmRoutingReceive)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__);
    mpIAmRoutingReceive = pIAmRoutingReceive;
    return startService(mpIAmRoutingReceive);
}

am_Error_e CAmRoutingSenderCAPI::tearDownInterface(IAmRoutingReceive*)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__);
    if(mpCAmCAPIWrapper)
    {
    	if(mIsServiceStarted)
    	{
    		mIsServiceStarted = false;
			mpCAmCAPIWrapper->unregisterService(CAmRoutingSenderCAPI::DEFAULT_DOMAIN, am_routing_interface::RoutingControlObserver::getInterface(), CAmRoutingSenderCAPI::ROUTING_INSTANCE);
			mService.reset();
    	}
   		return (E_OK);
    }
    return (E_NOT_POSSIBLE);
}

void CAmRoutingSenderCAPI::getInterfaceVersion(std::string & version) const
{
    version = RoutingVersion;
}

void CAmRoutingSenderCAPI::setRoutingReady(const uint16_t handle)
{
	assert(mpIAmRoutingReceive);
	mService->setHandle(handle);

    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "sending routingReady signal");
    mService->setRoutingReadyAttribute(am_types::am_RoutingReady_e::RR_READY);
    mpIAmRoutingReceive->confirmRoutingReady(handle,am_Error_e::E_OK);
}

void CAmRoutingSenderCAPI::setRoutingRundown(const uint16_t handle)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__);
	assert(mpIAmRoutingReceive);
	mService->setRoutingReadyAttribute(am_types::am_RoutingReady_e::RR_RUNDOWN);
	mService->gotRundown(mLookupData.numberOfDomains(),handle);
	mpIAmRoutingReceive->confirmRoutingRundown(handle,am_Error_e::E_OK);
}

void CAmRoutingSenderCAPI::checkSourceLookup(const am_sourceID_t id)
{
	am_domainID_t domainID;
	am_Error_e result = mpIAmRoutingReceive->getDomainOfSource(id, domainID);
	if(E_OK==result)
		mLookupData.addSourceLookup(id, domainID);
	else
		log(&GetDefaultRoutingDltContext(), DLT_LOG_ERROR,"checkSinkLookup addSourceLookup error:", result);
}

void CAmRoutingSenderCAPI::checkSinkLookup(const am_sinkID_t id)
{
	am_domainID_t domainID;
	am_Error_e result = mpIAmRoutingReceive->getDomainOfSink(id, domainID);
	if(E_OK==result)
		mLookupData.addSinkLookup(id, domainID);
	else
		log(&GetDefaultRoutingDltContext(), DLT_LOG_ERROR,"checkSinkLookup addSinkLookup error:", result);
}

am_Error_e CAmRoutingSenderCAPI::asyncAbort(const am_Handle_s handle)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncAbort called");

   	return mLookupData.asyncAbort(handle,[](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID, const am_sourceID_t sourceID, const am_sinkID_t sinkID, const am_CustomConnectionFormat_t connectionFormat)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncConnect called");
    checkSourceLookup(sourceID);
    checkSinkLookup(sinkID);
   	return mLookupData.asyncConnect(handle,connectionID, sourceID, sinkID, connectionFormat, [&,handle,connectionID](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackConnect(handle, connectionID, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncDisconnect called");
	return mLookupData.asyncDisconnect(handle,connectionID, [&, handle, connectionID](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackDisconnect(handle, connectionID, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSinkVolume called");
	checkSinkLookup(sinkID);
	return mLookupData.asyncSetSinkVolume(handle,sinkID, volume, ramp, time, [&, handle, volume](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSinkVolumeChange(handle, volume, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSourceVolume called");
	checkSourceLookup(sourceID);
	return mLookupData.asyncSetSourceVolume(handle,sourceID, volume, ramp, time, [&, handle, volume](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSourceVolumeChange(handle, volume, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SourceState_e state)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSourceState called");
	checkSourceLookup(sourceID);
	return mLookupData.asyncSetSourceState(handle,sourceID, state,[&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSourceState(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID, const std::vector<am_SoundProperty_s>& listSoundProperties)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSinkSoundProperties called");
	checkSinkLookup(sinkID);
	return mLookupData.asyncSetSinkSoundProperties(handle,sinkID, listSoundProperties, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSinkSoundProperties(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID, const am_SoundProperty_s& soundProperty)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSinkSoundProperty called");
	checkSinkLookup(sinkID);
	return mLookupData.asyncSetSinkSoundProperty(handle, sinkID, soundProperty, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSinkSoundProperty(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID, const std::vector<am_SoundProperty_s>& listSoundProperties)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSourceSoundProperties called");
	checkSourceLookup(sourceID);
	return mLookupData.asyncSetSourceSoundProperties(handle, sourceID, listSoundProperties, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSourceSoundProperties(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID, const am_SoundProperty_s& soundProperty)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSourceSoundProperty called");
	checkSourceLookup(sourceID);
	return mLookupData.asyncSetSourceSoundProperty(handle, sourceID, soundProperty, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetSourceSoundProperty(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID, const am_HotSink_e hotSink, const am_CustomRampType_t rampType, const am_time_t time)
{
	return mLookupData.asyncCrossFade(handle, crossfaderID, hotSink, rampType, time, [&, handle, hotSink](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackCrossFading(handle, hotSink, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::setDomainState called");
	return mLookupData.setDomainState(domainID, domainState, [](const CommonAPI::CallStatus& callStatus, am_types::am_Error_e error){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus,"Error",static_cast<am_Error_e>((int)error));
	});
}

am_Error_e CAmRoutingSenderCAPI::returnBusName(std::string& BusName) const
{
    BusName = CAmLookupData::BUS_NAME;
    return (E_OK);
}

am_Error_e CAmRoutingSenderCAPI::asyncSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& volumes)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetVolumes called");
	return mLookupData.asyncSetVolumes(handle, volumes, [&, handle, volumes](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSetVolumes(handle, volumes, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSinkNotificationConfiguration(const am_Handle_s handle, const am_sinkID_t sinkID, const am_NotificationConfiguration_s& nc)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSinkNotificationConfiguration called");
	checkSinkLookup(sinkID);
	return mLookupData.asyncSetSinkNotificationConfiguration(handle, sinkID, nc, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSinkNotificationConfiguration(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::asyncSetSourceNotificationConfiguration(const am_Handle_s handle, const am_sourceID_t sourceID, const am_NotificationConfiguration_s& nc)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, "CAmRoutingSenderCAPI::asyncSetSourceNotificationConfiguration called");
	checkSourceLookup(sourceID);
	return mLookupData.asyncSetSourceNotificationConfiguration(handle, sourceID, nc, [&, handle](const CommonAPI::CallStatus& callStatus){
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "Response with call status:", (int16_t)callStatus);
		if (callStatus != CommonAPI::CallStatus::SUCCESS)
		{
			mService->ackSourceNotificationConfiguration(handle, am_Error_e::E_NON_EXISTENT);
		}
	});
}

am_Error_e CAmRoutingSenderCAPI::resyncConnectionState(const am_domainID_t domainID, std::vector<am_Connection_s>& listOfExistingConnections)
{
	return am_Error_e::E_NOT_USED;
}

}

