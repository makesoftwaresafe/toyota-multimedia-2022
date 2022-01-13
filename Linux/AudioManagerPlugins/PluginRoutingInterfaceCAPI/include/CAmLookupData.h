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

#ifndef CAMLOOKUPDATA_H_
#define CAMLOOKUPDATA_H_

#include <map>
#include <iostream>
#include <cassert>
#include <memory>
#include "audiomanagertypes.h"
#include "IAmRouting.h"
#include "CAmRoutingSenderCommon.h"
#include <v1/org/genivi/am/routinginterface/RoutingControlProxy.hpp>

#ifdef UNIT_TEST
#include "../test/IAmRoutingSenderBackdoor.h" //we need this for the unit test
#endif

namespace am {

/**
 * The class encapsulates the lookup mechanism and forwards the invocations to the appropriate lookup objects ( proxies ).
 */
class CAmLookupData {

	/** Class wrapping a routing control proxy object, which is connected to the routing control service of given domain.
	 */
	class RSLookupData
	{
		CAmLookupData *mpLookupDataOwner;
	    bool mIsConnected; //!< bool indicating whether the domain is reachable or not
	    bool mIsSubcribed;
	    std::shared_ptr<am_routing_interface::RoutingControlProxy<> > mSenderProxy; //!< a pointer to the proxy object, which implements the connection out from AudioManager
	    void onServiceStatusEvent(const CommonAPI::AvailabilityStatus& serviceStatus); //!< proxy status event callback
	    void subscribe();
	public:
	    RSLookupData(CAmLookupData *pLookupDataOwner, const std::shared_ptr<am_routing_interface::RoutingControlProxy<> > & aProxy);
	    ~RSLookupData();

	    /**
	     * Return the proxy object.
	     */
	    std::shared_ptr<am_routing_interface::RoutingControlProxy<>> & getProxy();
	    /**
	     * Return whether the proxy object is connected or not.
	     */
	    bool isConnected();
	    /**
	     * proxy wrapping methods.
	     */
	    am_Error_e doAbort(const am_Handle_s handle, am_routing_interface::RoutingControlProxyBase::AsyncAbortAsyncCallback);
	    am_Error_e doConnect(const am_Handle_s handle, const am_connectionID_t, const am_sourceID_t, const am_sinkID_t, const am_CustomConnectionFormat_t, am_routing_interface::RoutingControlProxyBase::AsyncConnectAsyncCallback);
	    am_Error_e doDisconnect(const am_Handle_s handle, const am_connectionID_t, am_routing_interface::RoutingControlProxyBase::AsyncDisconnectAsyncCallback);
	    am_Error_e doSetSinkVolume(const am_Handle_s handle, const am_sinkID_t, const am_volume_t, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkVolumeAsyncCallback);
	    am_Error_e doSetSourceVolume(const am_Handle_s handle, const am_sourceID_t , const am_volume_t, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceVolumeAsyncCallback);
	    am_Error_e doSetSourceState(const am_Handle_s handle, const am_sourceID_t, const am_SourceState_e, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceStateAsyncCallback);
	    am_Error_e doSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t, const std::vector<am_SoundProperty_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback);
	    am_Error_e doSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t, const am_SoundProperty_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertyAsyncCallback);
	    am_Error_e doSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t, const std::vector<am_SoundProperty_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertiesAsyncCallback);
	    am_Error_e doSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t, const am_SoundProperty_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertyAsyncCallback);
	    am_Error_e doCrossFade(const am_Handle_s handle, const am_crossfaderID_t, const am_HotSink_e, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncCrossFadeAsyncCallback);
	    am_Error_e doSetDomainState(const am_domainID_t, const am_DomainState_e, am_routing_interface::RoutingControlProxyBase::SetDomainStateAsyncCallback);
	    am_Error_e doSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetVolumesAsyncCallback);
	    am_Error_e doSetSinkNotificationConfiguration(const am_Handle_s handle, const am_sinkID_t, const am_NotificationConfiguration_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkNotificationConfigurationAsyncCallback);
	    am_Error_e doSetSourceNotificationConfiguration(const am_Handle_s handle, const am_sourceID_t, const am_NotificationConfiguration_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceNotificationConfigurationAsyncCallback);

	};

	typedef std::shared_ptr<RSLookupData> RSLookupDataPtr;

	IAmRoutingReceive *mpIAmRoutingReceive;

	/**
	 * Lookup maps.
	 */
	typedef std::map<am_domainID_t,RSLookupDataPtr> mapDomain_t;
	typedef std::map<am_sinkID_t,RSLookupDataPtr> mapSinks_t;
	typedef std::map<am_sourceID_t,RSLookupDataPtr> mapSources_t;
	typedef std::map<am_connectionID_t,RSLookupDataPtr> mapConnections_t;
	typedef std::map<uint16_t,RSLookupDataPtr> mapHandles_t;
	typedef std::map<am_crossfaderID_t,RSLookupDataPtr> mapCrossfaders_t;

	mapDomain_t mMapDomains; //!< mapDomain_t domains lookup map
	mapSinks_t mMapSinks; //!< mapSinks_t sinks lookup map
	mapSources_t mMapSources; //!< mapSources_t sources lookup map
	mapConnections_t mMapConnections; //!< mapConnections_t connections lookup map
	mapHandles_t mMapHandles; //!< mapHandles_t handles lookup map
	mapCrossfaders_t mMapCrossfaders; //!< mapCrossfaders_t crossfaders lookup map

	/** \brief returns the value for given key if exists.
	 *
	 * @param  key is a search key.
	 * @param  map is a either domain, sink, source, connection, crossfader or handle map.
	 */
	template <typename TKey> static const RSLookupDataPtr getValueForKey(const TKey & key, const std::map<TKey,RSLookupDataPtr> & map);

	/** \brief removes all entries which contains given value.
	 *
	 * @param  value is a search value.
	 * @param  map is a either domain, sink, source, connection, crossfader or handle map.
	 */
	template <typename TKey> static void removeEntriesForValue(const RSLookupDataPtr & value, std::map<TKey,RSLookupDataPtr> & map);

public:
	CAmLookupData(IAmRoutingReceive * pRoutingReceive = NULL);
	virtual ~CAmLookupData();

	IAmRoutingReceive *getIAmRoutingReceive(){ return mpIAmRoutingReceive; }
	void setIAmRoutingReceive(IAmRoutingReceive * pRoutingReceive){ mpIAmRoutingReceive=pRoutingReceive; }

	/** \brief adds a lookup for given domain.
	 *
	 * @param  domainID is a valid domain id.
	 * @param  aProxy is a proxy object constructed by registerDomain
	 */
	void addDomainLookup(am_domainID_t & domainID,
							std::shared_ptr<am_routing_interface::RoutingControlProxy<>> & aProxy);

	/** \brief removes given handle from the list.
	 *
	 */
	void removeHandle(am_Handle_s handle);

	/** \brief adds a lookup for given source in a given domain.
	 *
	 * @param  sourceID is a valid source id.
	 * @param  domainID is a valid domain id
	 */
    void addSourceLookup(am_sourceID_t sourceID, am_domainID_t domainID);

	/** \brief adds a lookup for given sink in a given domain.
	 *
	 * @param  sinkID is a valid sink id.
	 * @param  domainID is a valid domain id
	 */
    void addSinkLookup(am_sinkID_t sinkID, am_domainID_t domainID);

	/** \brief adds a lookup for given crossfader in the domain wherein the given source belongs to.
	 *
	 * @param  crossfaderID is a valid crossfader id.
	 * @param  soucreID is a valid source id
	 */
    void addCrossfaderLookup(am_crossfaderID_t crossfaderID, am_sourceID_t soucreID);

	/** \brief removes a given domain lookup.
	 */
    void removeDomainLookup(am_domainID_t domainID);

	/** \brief removes a given source lookup.
	 */
    void removeSourceLookup(am_sourceID_t sourceID);

	/** \brief removes a given sink lookup.
	 */
    void removeSinkLookup(am_sinkID_t sinkID);

	/** \brief removes a given crossfader lookup.
	 */
    void removeCrossfaderLookup(am_crossfaderID_t crossfaderID);

	/** \brief removes a given connection lookup.
	 */
    void removeConnectionLookup(am_connectionID_t connectionID);

    size_t numberOfDomains() { return mMapDomains.size(); }

    static const char * BUS_NAME;

    /**
     * Wrapping methods.
     */
    am_Error_e asyncAbort(const am_Handle_s, am_routing_interface::RoutingControlProxyBase::AsyncAbortAsyncCallback);
    am_Error_e asyncConnect(const am_Handle_s , const am_connectionID_t, const am_sourceID_t, const am_sinkID_t, const am_CustomConnectionFormat_t, am_routing_interface::RoutingControlProxyBase::AsyncConnectAsyncCallback);
    am_Error_e asyncDisconnect(const am_Handle_s , const am_connectionID_t, am_routing_interface::RoutingControlProxyBase::AsyncDisconnectAsyncCallback);
    am_Error_e asyncSetSinkVolume(const am_Handle_s , const am_sinkID_t, const am_volume_t, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkVolumeAsyncCallback);
    am_Error_e asyncSetSourceVolume(const am_Handle_s , const am_sourceID_t , const am_volume_t, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceVolumeAsyncCallback);
    am_Error_e asyncSetSourceState(const am_Handle_s , const am_sourceID_t, const am_SourceState_e, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceStateAsyncCallback);
    am_Error_e asyncSetSinkSoundProperties(const am_Handle_s , const am_sinkID_t, const std::vector<am_SoundProperty_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertiesAsyncCallback);
    am_Error_e asyncSetSinkSoundProperty(const am_Handle_s , const am_sinkID_t, const am_SoundProperty_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkSoundPropertyAsyncCallback);
    am_Error_e asyncSetSourceSoundProperties(const am_Handle_s , const am_sourceID_t, const std::vector<am_SoundProperty_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertiesAsyncCallback);
    am_Error_e asyncSetSourceSoundProperty(const am_Handle_s , const am_sourceID_t, const am_SoundProperty_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceSoundPropertyAsyncCallback);
    am_Error_e asyncCrossFade(const am_Handle_s , const am_crossfaderID_t, const am_HotSink_e, const am_CustomRampType_t, const am_time_t, am_routing_interface::RoutingControlProxyBase::AsyncCrossFadeAsyncCallback);
    am_Error_e setDomainState(const am_domainID_t, const am_DomainState_e, am_routing_interface::RoutingControlProxyBase::SetDomainStateAsyncCallback);
    am_Error_e asyncSetVolumes(const am_Handle_s , const std::vector<am_Volumes_s>&, am_routing_interface::RoutingControlProxyBase::AsyncSetVolumesAsyncCallback);
    am_Error_e asyncSetSinkNotificationConfiguration(const am_Handle_s , const am_sinkID_t, const am_NotificationConfiguration_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSinkNotificationConfigurationAsyncCallback);
    am_Error_e asyncSetSourceNotificationConfiguration(const am_Handle_s , const am_sourceID_t, const am_NotificationConfiguration_s&, am_routing_interface::RoutingControlProxyBase::AsyncSetSourceNotificationConfigurationAsyncCallback);

    /**
     * Event callbacks
     */
    void ackConnect(am_types::am_Handle_s handle, am_types::am_connectionID_t connectionID, am_types::am_Error_e error);
	void ackDisconnect(am_types::am_Handle_s handle, am_types::am_connectionID_t connectionID, am_types::am_Error_e error);
	void ackSetSinkVolumeChange(am_types::am_Handle_s handle, am_types::am_volume_t volume, am_types::am_Error_e error);
	void ackSetSourceVolumeChange(am_types::am_Handle_s handle, am_types::am_volume_t volume, am_types::am_Error_e error);
	void ackSetSourceState(am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackSetSinkSoundProperties(am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackSetSinkSoundProperty(am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackSetSourceSoundProperties(am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackSetSourceSoundProperty(am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackCrossFading(am_types::am_Handle_s handle, am_types::am_HotSink_e hotSink, am_types::am_Error_e error);
	void ackSourceVolumeTick(am_types::am_Handle_s handle, am_types::am_sourceID_t source, am_types::am_volume_t volume);
	void ackSinkVolumeTick(am_types::am_Handle_s handle, am_types::am_sinkID_t sink, am_types::am_volume_t volume);
	void ackSetVolumes(am_types::am_Handle_s handle , am_types::am_Volumes_L listVolumes, am_types::am_Error_e error);
	void ackSinkNotificationConfiguration (am_types::am_Handle_s handle, am_types::am_Error_e error);
	void ackSourceNotificationConfiguration(am_types::am_Handle_s handle, am_types::am_Error_e error);

#ifdef UNIT_TEST
    friend class IAmRoutingSenderBackdoor;
#endif
};

} /* namespace am */
#endif /* CAMLOOKUPDATA_H_ */
