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

#include <algorithm>
#include "CAmLookupData.h"
#include "CAmRoutingSenderCommon.h"
#include "CAmDltWrapper.h"


namespace am {

const char * CAmLookupData::BUS_NAME = "CAPIRoutingPlugin";

/**
 * RSLookupData
 */

CAmLookupData::RSLookupData::RSLookupData(CAmLookupData *pLookupDataOwner, const std::shared_ptr<am_routing_interface::RoutingControlProxy<> > & aProxy)
{
	mSenderProxy = aProxy;
	mpLookupDataOwner = pLookupDataOwner;
	mIsConnected = mSenderProxy->isAvailable();
	mIsSubcribed = false;
	mSenderProxy->getProxyStatusEvent().subscribe(std::bind(&CAmLookupData::RSLookupData::onServiceStatusEvent,this,std::placeholders::_1));
	if(mIsConnected)
		subscribe();
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO, __PRETTY_FUNCTION__, "mIsConnected:", mIsConnected);
}

CAmLookupData::RSLookupData::~RSLookupData()
{
	mSenderProxy.reset();
}

void CAmLookupData::RSLookupData::subscribe()
{
	if(!mIsSubcribed)
	{
		mSenderProxy->getAckConnectSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackConnect,mpLookupDataOwner,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckDisconnectSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackDisconnect,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSetVolumesSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetVolumes,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckCrossFadingSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackCrossFading,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSinkVolumeTickSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSinkVolumeTick,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSourceVolumeTickSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSourceVolumeTick,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSetSinkVolumeChangeSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSinkVolumeChange,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSetSourceVolumeChangeSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSourceVolumeChange,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		mSenderProxy->getAckSetSinkSoundPropertySelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSinkSoundProperty,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSetSinkSoundPropertiesSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSinkSoundProperties,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSetSourceSoundPropertySelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSourceSoundProperty,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSetSourceSoundPropertiesSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSourceSoundProperties,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSetSourceStateSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSetSourceState,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSinkNotificationConfigurationSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSinkNotificationConfiguration,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mSenderProxy->getAckSourceNotificationConfigurationSelectiveEvent().subscribe(std::bind(&CAmLookupData::ackSourceNotificationConfiguration,mpLookupDataOwner, std::placeholders::_1, std::placeholders::_2));
		mIsSubcribed = true;
	}
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__);
}

void CAmLookupData::RSLookupData::onServiceStatusEvent(const CommonAPI::AvailabilityStatus& serviceStatus)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " status : ", (int)serviceStatus );
	mIsConnected = (serviceStatus==CommonAPI::AvailabilityStatus::AVAILABLE);

	if(mIsConnected)
	{
		/* Subscribe should be done every time that connecting the proxy!
		 *
		 * An infinite loop in subscribe causes the method to hang when the status is UNAVAILABLE!
		 */
		subscribe();
		log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,"...event subscriptions done");
	}
	else
		mIsSubcribed = false;
}

std::shared_ptr<am_routing_interface::RoutingControlProxy<>> & CAmLookupData::RSLookupData::getProxy()
{
	return mSenderProxy;
}

bool CAmLookupData::RSLookupData::isConnected()
{
	return mIsConnected;
}

am_Error_e CAmLookupData::RSLookupData::doAbort(const am_Handle_s handle, am_routing_interface::RoutingControlProxyBase::AsyncAbortAsyncCallback callback)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncAbortAsync(myHandle, callback);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doConnect(const am_Handle_s handle,
											const am_connectionID_t connectionID,
											const am_sourceID_t sourceID,
											const am_sinkID_t sinkID,
											const am_CustomConnectionFormat_t connectionFormat,
											am_routing_interface::RoutingControlProxyBase::AsyncConnectAsyncCallback callback)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncConnectAsync(myHandle,
										static_cast<am_types::am_connectionID_t>(connectionID),
										static_cast<am_types::am_sourceID_t>(sourceID),
										static_cast<am_types::am_sinkID_t>(sinkID),
										static_cast<am_types::am_ConnectionFormat_pe>(connectionFormat),
										callback);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doDisconnect(const am_Handle_s handle,
												const am_connectionID_t connectionID,
												am_routing_interface::RoutingControlProxyBase::AsyncDisconnectAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncDisconnectAsync(myHandle,
										   static_cast<am_types::am_connectionID_t>(connectionID),
										   cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSinkVolume(	const am_Handle_s handle,
													const am_sinkID_t sinkID,
													const am_volume_t volume,
													const am_CustomRampType_t ramp,
													const am_time_t time,
													am_routing_interface::RoutingControlProxyBase::AsyncSetSinkVolumeAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSinkVolumeAsync(myHandle,
										      static_cast<am_types::am_sinkID_t>(sinkID),
										      static_cast<am_types::am_volume_t>(volume),
										      static_cast<am_types::am_RampType_pe>(ramp),
										      static_cast<am_types::am_time_t>(time),
										      cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSourceVolume(const am_Handle_s handle,
														const am_sourceID_t sourceID,
														const am_volume_t volume,
														const am_CustomRampType_t ramp,
														const am_time_t time,
														am_routing_interface::RoutingControlProxyBase::AsyncSetSourceVolumeAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSourceVolumeAsync(myHandle,
												static_cast<am_types::am_sourceID_t>(sourceID),
												static_cast<am_types::am_volume_t>(volume),
												static_cast<am_types::am_RampType_pe>(ramp),
												static_cast<am_types::am_time_t>(time),
												cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSourceState(	const am_Handle_s handle,
															const am_sourceID_t sourceID,
															const am_SourceState_e state,
															am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSourceStateAsync(myHandle,
												static_cast<am_types::am_sourceID_t>(sourceID),
												static_cast<am_types::am_SourceState_e::Literal>(state),
												cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSinkSoundProperties(	const am_Handle_s handle,
																const am_sinkID_t sinkID,
																const std::vector<am_SoundProperty_s>& listSoundProperties,
																am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_SoundProperty_L lsp;
		convert_am_types(listSoundProperties, lsp);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSinkSoundPropertiesAsync(myHandle,
														static_cast<am_types::am_sinkID_t>(sinkID),
														lsp,
														cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSinkSoundProperty(	const am_Handle_s handle,
															const am_sinkID_t sinkID,
															const am_SoundProperty_s& soundProperty,
															am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertyAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_SoundProperty_s converted;
		convert_am_types(soundProperty, converted);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSinkSoundPropertyAsync(myHandle,
													 static_cast<am_types::am_sinkID_t>(sinkID),
													 converted,
													 cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSourceSoundProperties(const am_Handle_s handle,
																const am_sourceID_t sourceID,
																const std::vector<am_SoundProperty_s>& listSoundProperties,
																am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertiesAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_SoundProperty_L lsp;
		convert_am_types(listSoundProperties, lsp);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSourceSoundPropertiesAsync(myHandle,
														static_cast<am_types::am_sourceID_t>(sourceID),
														lsp,
														cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSourceSoundProperty(const am_Handle_s handle,
																const am_sourceID_t sourceID,
																const am_SoundProperty_s& soundProperty,
																am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertyAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_SoundProperty_s converted;
		convert_am_types(soundProperty, converted);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSourceSoundPropertyAsync(	myHandle,
														static_cast<am_types::am_sourceID_t>(sourceID),
														converted,
														cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}


am_Error_e CAmLookupData::RSLookupData::doCrossFade(const am_Handle_s handle,
												const am_crossfaderID_t crossfaderID,
												const am_HotSink_e hotSink,
												const am_CustomRampType_t rampType,
												const am_time_t time,
												am_routing_interface::RoutingControlProxyBase::AsyncCrossFadeAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncCrossFadeAsync(myHandle,
										  static_cast<am_types::am_crossfaderID_t>(crossfaderID),
										  static_cast<am_types::am_HotSink_e::Literal>(hotSink),
										  static_cast<am_types::am_RampType_pe>(rampType),
										  static_cast<am_types::am_time_t>(time),
										  cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetDomainState(const am_domainID_t domainID, const am_DomainState_e domainState, am_routing_interface::RoutingControlProxyBase::SetDomainStateAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		mSenderProxy->setDomainStateAsync(static_cast<am_types::am_domainID_t>(domainID),
										  static_cast<am_types::am_DomainState_e::Literal>(domainState),
										  cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetVolumes(const am_Handle_s handle,
												const std::vector<am_Volumes_s>& volumes ,
												am_routing_interface::RoutingControlProxyBase::AsyncSetVolumesAsyncCallback cb )
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_Volumes_L list;
		convert_am_types(volumes, list);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetVolumesAsync(myHandle,
								      list,
									   cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSinkNotificationConfiguration(const am_Handle_s handle,
																		 const am_sinkID_t sinkID,
																		 const am_NotificationConfiguration_s& notificationConfiguration,
																		 am_routing_interface::RoutingControlProxyBase::AsyncSetSinkNotificationConfigurationAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_NotificationConfiguration_s converted;
		convert_am_types(notificationConfiguration, converted);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSinkNotificationConfigurationAsync(myHandle,
										  	  	  	  	    static_cast<am_types::am_sinkID_t>(sinkID),
										  	  	  	  	    converted,
										  	  	  	  	   	cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

am_Error_e CAmLookupData::RSLookupData::doSetSourceNotificationConfiguration(const am_Handle_s handle,
																			const am_sourceID_t sourceID,
																			const am_NotificationConfiguration_s& notificationConfiguration,
																			am_routing_interface::RoutingControlProxyBase::AsyncSetSourceNotificationConfigurationAsyncCallback cb)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ isConnected : ", mIsConnected, " ]");
	if(mIsConnected)
	{
		am_types::am_NotificationConfiguration_s converted;
		convert_am_types(notificationConfiguration, converted);
		am_types::am_Handle_s myHandle;
		convert_am_types(handle,myHandle);
		mSenderProxy->asyncSetSourceNotificationConfigurationAsync( myHandle,
																static_cast<am_types::am_sourceID_t>(sourceID),
																converted,
																cb);
		return (E_OK);
	}
	return (E_UNKNOWN);
}

/**
 * CAmLookupData
 */

CAmLookupData::CAmLookupData(IAmRoutingReceive * pRoutingReceive):
		mpIAmRoutingReceive(pRoutingReceive),
		mMapDomains(),
		mMapSinks(),
		mMapSources(),
		mMapConnections(),
		mMapHandles(),
		mMapCrossfaders()
{
	// TODO Auto-generated constructor stub

}

CAmLookupData::~CAmLookupData() {
	// TODO Auto-generated destructor stub
}

void CAmLookupData::addDomainLookup(am_domainID_t & domainID,
										std::shared_ptr<am_routing_interface::RoutingControlProxy<>> & aProxy)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ domainID : ", domainID, " ]");
	RSLookupDataPtr lookupData = std::make_shared<RSLookupData>(this, aProxy);
	mMapDomains.insert(std::make_pair(domainID, lookupData));
}

void CAmLookupData::removeHandle(am_Handle_s handle)
{
	mMapHandles.erase(handle.handle);
}

void CAmLookupData::addSourceLookup(am_sourceID_t sourceID, am_domainID_t domainID)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ domainID : ", domainID, " ]", " [ sourceID : ", sourceID, " ]");
    mapDomain_t::iterator iter(mMapDomains.begin());
    iter = mMapDomains.find(domainID);
    if (iter != mMapDomains.end())
    {
        mMapSources.insert(std::make_pair(sourceID, iter->second));
    }
}

void CAmLookupData::addSinkLookup(am_sinkID_t sinkID, am_domainID_t domainID)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ domainID : ", domainID, " ]", " [ sinkID : ", sinkID, " ]");
    mapDomain_t::iterator iter(mMapDomains.begin());
    iter = mMapDomains.find(domainID);
    if (iter != mMapDomains.end())
    {
        mMapSinks.insert(std::make_pair(sinkID, iter->second));
    }
}

void CAmLookupData::addCrossfaderLookup(am_crossfaderID_t crossfaderID, am_sourceID_t soucreID)
{
	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [ crossfaderID : ", crossfaderID, " ]", " [ soucreID : ", soucreID, " ]");
    mapSources_t::iterator iter(mMapSources.begin());
    iter = mMapSources.find(soucreID);
    if (iter != mMapSources.end())
    {
    	mMapCrossfaders.insert(std::make_pair(crossfaderID, iter->second));
    }
}

void CAmLookupData::removeDomainLookup(am_domainID_t domainID)
{
    mapDomain_t::iterator iter(mMapDomains.begin());
    iter = mMapDomains.find(domainID);
    if (iter != mMapDomains.end())
    {
    	CAmLookupData::removeEntriesForValue(iter->second, mMapSources);
    	CAmLookupData::removeEntriesForValue(iter->second, mMapSinks);
    	CAmLookupData::removeEntriesForValue(iter->second, mMapCrossfaders);
    	CAmLookupData::removeEntriesForValue(iter->second, mMapHandles);
    	CAmLookupData::removeEntriesForValue(iter->second, mMapConnections);
		mMapDomains.erase(domainID);
    }
}

void CAmLookupData::removeSourceLookup(am_sourceID_t sourceID)
{
    mMapSources.erase(sourceID);
}

void CAmLookupData::removeSinkLookup(am_sinkID_t sinkID)
{
    mMapSinks.erase(sinkID);
}

void CAmLookupData::removeCrossfaderLookup(am_crossfaderID_t crossfaderID)
{
	mMapCrossfaders.erase(crossfaderID);
}

void CAmLookupData::removeConnectionLookup(am_connectionID_t connectionID)
{
	mMapConnections.erase(connectionID);
}

template <typename TKey> void  CAmLookupData::removeEntriesForValue(const RSLookupDataPtr & value, std::map<TKey,RSLookupDataPtr> & map)
{
	typename std::map<TKey,RSLookupDataPtr>::iterator it = map.begin();
	while ( it != map.end() )
	{
		if (it->second == value)
		{
			typename std::map<TKey,RSLookupDataPtr>::iterator it_tmp = it;
			it++;
			map.erase(it_tmp);
		}
		else
			++it;
	}
}

template <typename TKey> const CAmLookupData::RSLookupDataPtr  CAmLookupData::getValueForKey(const TKey & key, const std::map<TKey,CAmLookupData::RSLookupDataPtr> & map)
{
	mapHandles_t::const_iterator iter = map.find(key);
	if (iter != map.end() )
	{
		return iter->second;
	}
	return NULL;
}

am_Error_e CAmLookupData::asyncAbort(const am_Handle_s handle, am_routing_interface::RoutingControlProxyBase::AsyncAbortAsyncCallback callback)
{
	RSLookupDataPtr result = getValueForKey(handle.handle, mMapHandles);
    if(result)
   		return result->doAbort(handle, callback);
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncConnect(const am_Handle_s handle,
											const am_connectionID_t connectionID,
											const am_sourceID_t sourceID,
											const am_sinkID_t sinkID,
											const am_CustomConnectionFormat_t connectionFormat,
											am_routing_interface::RoutingControlProxyBase::AsyncConnectAsyncCallback callback)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
    log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,__PRETTY_FUNCTION__, " [sourceID:", sourceID, " sinkID:", sinkID, "]");
    if(result)
    {
    	log(&GetDefaultRoutingDltContext(), DLT_LOG_INFO,"	[address:", result->getProxy()->getAddress().getAddress(), " connected:", result->isConnected(), "]" );

        mMapConnections.insert(std::make_pair(connectionID, result));
        mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doConnect(handle, connectionID, sourceID, sinkID, connectionFormat, callback);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncDisconnect(const am_Handle_s handle,
												const am_connectionID_t connectionID,
												am_routing_interface::RoutingControlProxyBase::AsyncDisconnectAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(connectionID, mMapConnections);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doDisconnect(handle, connectionID, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSinkVolume(	const am_Handle_s handle,
													const am_sinkID_t sinkID,
													const am_volume_t volume,
													const am_CustomRampType_t ramp,
													const am_time_t time,
													am_routing_interface::RoutingControlProxyBase::AsyncSetSinkVolumeAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sinkID, mMapSinks);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSinkVolume(handle, sinkID, volume, ramp, time, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSourceVolume(const am_Handle_s handle,
													const am_sourceID_t sourceID,
													const am_volume_t volume,
													const am_CustomRampType_t ramp,
													const am_time_t time,
													am_routing_interface::RoutingControlProxyBase::AsyncSetSourceVolumeAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSourceVolume(handle, sourceID, volume, ramp, time, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSourceState(const am_Handle_s handle,
													const am_sourceID_t sourceID,
													const am_SourceState_e state,
													am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSourceState(handle, sourceID, state, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSinkSoundProperties(	const am_Handle_s handle,
																const am_sinkID_t sinkID,
																const std::vector<am_SoundProperty_s>& listSoundProperties,
																am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sinkID, mMapSinks);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSinkSoundProperties(handle, sinkID, listSoundProperties, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSinkSoundProperty(	const am_Handle_s handle,
															const am_sinkID_t sinkID,
															const am_SoundProperty_s& soundProperty,
															am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertyAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sinkID, mMapSinks);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSinkSoundProperty(handle, sinkID, soundProperty, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSourceSoundProperties(const am_Handle_s handle,
																const am_sourceID_t sourceID,
																const std::vector<am_SoundProperty_s>& listSoundProperties,
																am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertiesAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSourceSoundProperties(handle, sourceID, listSoundProperties, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSourceSoundProperty(const am_Handle_s handle,
															const am_sourceID_t sourceID,
															const am_SoundProperty_s& soundProperty,
															am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertyAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSourceSoundProperty(handle, sourceID, soundProperty, cb);
    }
    return (E_UNKNOWN);
}


am_Error_e CAmLookupData::asyncCrossFade(const am_Handle_s handle,
												const am_crossfaderID_t crossfaderID,
												const am_HotSink_e hotSink,
												const am_CustomRampType_t rampType,
												const am_time_t time,
												am_routing_interface::RoutingControlProxyBase::AsyncCrossFadeAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(crossfaderID, mMapCrossfaders);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doCrossFade(handle, crossfaderID, hotSink, rampType, time, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState, am_routing_interface::RoutingControlProxyBase::SetDomainStateAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(domainID, mMapDomains);
    if(result)
   		return result->doSetDomainState(domainID, domainState, cb);
    return (E_UNKNOWN);
}


am_Error_e CAmLookupData::asyncSetVolumes(const am_Handle_s handle,
												const std::vector<am_Volumes_s>& volumes ,
												am_routing_interface::RoutingControlProxyBase::AsyncSetVolumesAsyncCallback cb )
{

	if(volumes.size())
	{
		am_Volumes_s volumeItem = volumes.at(0);
		RSLookupDataPtr result = NULL;
		if(volumeItem.volumeType == VT_SINK)
			result = CAmLookupData::getValueForKey(volumeItem.volumeID.sink, mMapSinks);
		else if(volumeItem.volumeType == VT_SOURCE)
			result = CAmLookupData::getValueForKey(volumeItem.volumeID.source, mMapSources);
	    if(result)
	    {
	    	mMapHandles.insert(std::make_pair(+handle.handle, result));
	   		return result->doSetVolumes(handle, volumes, cb);
	    }
	}
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSinkNotificationConfiguration(const am_Handle_s handle,
																	 const am_sinkID_t sinkID,
																	 const am_NotificationConfiguration_s& notificationConfiguration,
																	 am_routing_interface::RoutingControlProxyBase::AsyncSetSinkNotificationConfigurationAsyncCallback cb)
{
    RSLookupDataPtr result = CAmLookupData::getValueForKey(sinkID, mMapSinks);
    if(result)
    {
    	mMapHandles.insert(std::make_pair(+handle.handle, result));
   		return result->doSetSinkNotificationConfiguration(handle, sinkID, notificationConfiguration, cb);
    }
    return (E_UNKNOWN);
}

am_Error_e CAmLookupData::asyncSetSourceNotificationConfiguration(const am_Handle_s handle,
																		const am_sourceID_t sourceID,
																		const am_NotificationConfiguration_s& notificationConfiguration,
																		am_routing_interface::RoutingControlProxyBase::AsyncSetSourceNotificationConfigurationAsyncCallback cb)
{
	RSLookupDataPtr result = CAmLookupData::getValueForKey(sourceID, mMapSources);
	if(result)
	{
		mMapHandles.insert(std::make_pair(+handle.handle, result));
		return result->doSetSourceNotificationConfiguration(handle, sourceID, notificationConfiguration, cb);
	}
	return (E_UNKNOWN);
}

void CAmLookupData::ackConnect(am_types::am_Handle_s handle, am_types::am_connectionID_t connectionID, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackConnect(handle_s, (int)connectionID, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackDisconnect(am_types::am_Handle_s handle, am_types::am_connectionID_t connectionID, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackDisconnect(handle_s, (int)connectionID, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
	removeConnectionLookup(connectionID);
}

void CAmLookupData::ackSetSinkVolumeChange(am_types::am_Handle_s handle, am_types::am_volume_t volume, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSinkVolumeChange(handle_s, volume, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSourceVolumeChange(am_types::am_Handle_s handle, am_types::am_volume_t volume, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSourceVolumeChange(handle_s, volume, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSourceState(am_types::am_Handle_s handle, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSourceState(handle_s, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSinkSoundProperties(am_types::am_Handle_s handle, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSinkSoundProperties(handle_s, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSinkSoundProperty(am_types::am_Handle_s handle, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSinkSoundProperty(handle_s, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSourceSoundProperties(am_types::am_Handle_s handle, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSourceSoundProperties(handle_s, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSetSourceSoundProperty(am_types::am_Handle_s handle, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSetSourceSoundProperty(handle_s, static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackCrossFading(am_types::am_Handle_s handle, am_types::am_HotSink_e hotSink, am_types::am_Error_e error)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackCrossFading(handle_s, static_cast<am_HotSink_e>((int)hotSink), static_cast<am_Error_e>((int)error));
	removeHandle(handle_s);
}

void CAmLookupData::ackSourceVolumeTick(am_types::am_Handle_s handle, am_types::am_sourceID_t source, am_types::am_volume_t volume)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSourceVolumeTick(handle_s, source, volume);
}

void CAmLookupData::ackSinkVolumeTick(am_types::am_Handle_s handle, am_types::am_sinkID_t sink, am_types::am_volume_t volume)
{
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	mpIAmRoutingReceive->ackSinkVolumeTick(handle_s, sink, volume);
}

void CAmLookupData::ackSetVolumes(am_types::am_Handle_s handle , am_types::am_Volumes_L listVolumes, am_types::am_Error_e error) {
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
	std::vector<am_Volumes_s> list;
	convert_am_types(listVolumes, list);
	am_Error_e amError = static_cast<am_Error_e>((int)error);
	mpIAmRoutingReceive->ackSetVolumes(handle_s, list, amError);
	removeHandle(handle_s);
}

void CAmLookupData::ackSinkNotificationConfiguration (am_types::am_Handle_s handle, am_types::am_Error_e error) {
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
    am_Error_e amError = static_cast<am_Error_e>((int)error);
    mpIAmRoutingReceive->ackSinkNotificationConfiguration(handle_s, amError);
    removeHandle(handle_s);
}

void CAmLookupData::ackSourceNotificationConfiguration(am_types::am_Handle_s handle, am_types::am_Error_e error) {
	assert(mpIAmRoutingReceive);
	am_Handle_s handle_s;
	convert_am_types(handle,handle_s);
    am_Error_e amError = static_cast<am_Error_e>((int)error);
    mpIAmRoutingReceive->ackSourceNotificationConfiguration(handle_s, amError);
    removeHandle(handle_s);
}

} /* namespace am */
