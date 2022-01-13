/************************************************************************
 * @file: CRaDbusWrpSender.h
 *
 * @version: 1.1
 *
 * @description: A CAmDbusRoutingSender class definition of Routing Adapter.
 * CAmDbusRoutingSender class will run in the context of AM process.
 * This is DBus wrapper class for sender class in AM side. CAmDbusRoutingSender
 * class will call the CRaDbusWrpSender class methods via DBus connection
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
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

#ifndef CAMDBUSROUTINGSENDER_H_
#define CAMDBUSROUTINGSENDER_H_

#include "IAmRouting.h"
#include "IAmSocketRoutingReceiverShadow.h"
#include "CSocketSender.h"
#include "CAmSocketRoutingParser.h"
#include "CSocketCommon.h"

#define RA_SOCKET_BUSNAME  "RoutingSocket"

namespace am
{

class CAmSocketRoutingSender:public IAmRoutingSend
{
public:
    CAmSocketRoutingSender();
    virtual ~CAmSocketRoutingSender();
    void getInterfaceVersion(std::string& version) const;
    am_Error_e returnBusName(std::string& BusName) const;
    am_Error_e startupInterface(IAmRoutingReceive* pIAmRoutingReceive);
    void setRoutingReady(const uint16_t handle);
    void setRoutingRundown(const uint16_t handle);
    am_Error_e asyncAbort(const am_Handle_s handle);
    am_Error_e asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                            const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                            const am_CustomConnectionFormat_t connectionFormat);
    am_Error_e asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID);
    am_Error_e asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID, const am_volume_t volume,
                                  const am_CustomRampType_t ramp, const am_time_t time);
    am_Error_e asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID, const am_volume_t volume,
                                    const am_CustomRampType_t ramp, const am_time_t time);
    am_Error_e asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                   const am_SourceState_e state);
    am_Error_e asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                           const std::vector<am_SoundProperty_s>& listSoundProperties);
    am_Error_e asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                         const am_SoundProperty_s& soundProperty);
    am_Error_e asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                             const std::vector<am_SoundProperty_s>& listSoundProperties);
    am_Error_e asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                           const am_SoundProperty_s& soundProperty);
    am_Error_e asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                              const am_HotSink_e hotSink, const am_CustomRampType_t rampType, const am_time_t time);
    am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState);
    am_Error_e asyncSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s>& listVolumes);
    am_Error_e asyncSetSinkNotificationConfiguration(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                     const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e asyncSetSourceNotificationConfiguration(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                       const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e resyncConnectionState(const am_domainID_t domainID, std::vector<am_Connection_s>& listOfExistingConnections);

    // helpers to manage D-Bus parameter look up
    void removeHandle( am_Handle_s handle );
    void addDomainLookup(const am_domainID_t domainID, socket_comm_s lookupData);
    void removeDomainLookup(const am_domainID_t domainID);
    void removeConnectionLookup(const am_connectionID_t connectionID);
    void addSourceLookup(const am_Source_s & source, const std::string & application);
    void addSinkLookup(const am_Sink_s & sink, const std::string & application);
    am_domainID_t queryDomainLookup(const std::string &lookupNodename);
    void writeDomainsToMap(const std::vector<dr_domain_s> & domainArray);
    void establishIsThereRoutingAdapter(const std::vector<std::string> & dbusPaths, std::vector<dr_domain_s> & domains);
	am_Error_e asyncConnect( const am_Handle_s handle );
    bool getRoutingReady(void);

	void registRoutingReceiver( IRecieveCallBack *recieveCb );
	CSocketSender* getCSocketSenderPt( void );
	void notificationRecvDest( unsigned short destaddr );

private:
    template <typename TKey> static void  removeEntriesForValue( void );

private:
	CAmSocketWrapper*				mpCAmSocketWrapper;
    IAmSocketRoutingReceiverShadow	mIAmSocketRoutingReceiverShadow;
    IAmRoutingReceive*				mpRoutingReceive;
    CSocketSender					mCSocketSender;
	CAmSocketHandler*				mpSocketHandler;

    // To update Dubs communication details
    void updateRADbusCommDetails(void);

    typedef std::map<am_domainID_t, socket_comm_s> mapDomain_t;
    typedef std::map<am_connectionID_t, socket_comm_s> mapConnections_t;
    typedef std::map< uint16_t, socket_comm_s> mapHandles_t;


    mapDomain_t mMapDomains;
    mapConnections_t mMapConnections;
    mapHandles_t mMapHandles;

    CAmDbusRoutingdb mDatabase;
    bool mSetRoutingReady;

};

}

#endif /* CAMDBUSROUTINGSENDER_H_ */
