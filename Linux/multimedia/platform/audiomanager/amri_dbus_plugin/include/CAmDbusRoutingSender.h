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
#include "IAmDbusRoutingReceiverShadow.h"
#include "CDBusReceiver.h"
#include "CDBusSender.h"
#include "CDBusCommon.h"
#include "CAmDbusRoutingParser.h"

#define RA_DBUS_BUSNAME  "RoutingDbus"

#define MULTICAST_MARKER_MSB 0x8000U

#define IS_SINK_APP 0x1
#define IS_SRC_APP 0x2
#define IS_MCAST 0x4
#define IS_DBRA 0x8


namespace am
{

struct dbus_comm_amri_s : public dbus_comm_s
{
    dbus_comm_amri_s(const dbus_comm_amri_s &obj) : dbus_comm_s(obj)
    {
        domainOwner = obj.domainOwner;
        communicationMode = obj.communicationMode;
        mIsThereRoutingAdapter = obj.mIsThereRoutingAdapter;
        mConnection = obj.mConnection;
    }

    dbus_comm_amri_s(const std::string& objectpath) :
        dbus_comm_s(objectpath), mIsThereRoutingAdapter(false)
    {
    }

    dbus_comm_amri_s(const std::string& objectpath, const std::string& interface) :
        dbus_comm_s(objectpath, interface), mIsThereRoutingAdapter(false)
    {
    }

    uint16_t getBitMask(const am_sourceID_t srcID, const am_sinkID_t sinkID, dbus_comm_amri_s & dbus, bool isConnect)
    {
        std::map<am_sinkID_t, std::string>::const_iterator sinkElem = mMapSink.find(sinkID);
        std::map<am_sourceID_t, std::string>::const_iterator sourceElem = mMapSrc.find(srcID);

        uint8_t mask = (sinkElem != mMapSink.end()) ? IS_SINK_APP : 0x0;
        mask |= (sourceElem != mMapSrc.end()) ? IS_SRC_APP : 0x0;
        mask |= (communicationMode == CM_MULTICAST) ? IS_MCAST : 0x0;
        mask |= (mIsThereRoutingAdapter) ? IS_DBRA : 0x0;
        bool isSameApplicationSourceSink = false;
        if ((mask & (IS_SINK_APP | IS_SRC_APP)) == (IS_SINK_APP | IS_SRC_APP))
        {
            isSameApplicationSourceSink = (sourceElem->second == sinkElem->second) ? true : false;
        }

        /*
         * Implementing following decisionTable:
         * SinkApp  SourceApp   isMulticast     isThereRA   HexBitMask    Interface2Append    MulticastEnabled
         * 0        0           0               0           0x0           -                   0
         * 1        0           0               0           0x1           sink                0
         * 0        1           0               0           0x2           source              0
         * 1        1           0               0           0x3           sink                0
         * 0        0           1               0           0x4           -                   0
         * 1        0           1               0           0x5           sink                0
         * 0        1           1               0           0x6           source              0
         * 1        1           1               0           0x7           sink & source       0/1 (depends if same application)
         * 0        0           0               1           0x8           router              0
         * 1        0           0               1           0x9           router              0
         * 0        1           0               1           0xA           router              0
         * 1        1           0               1           0xB           sink                0
         * 0        0           1               1           0xC           router              0
         * 1        0           1               1           0xD           sink & router       1
         * 0        1           1               1           0xE           router & source     1
         * 1        1           1               1           0xF           sink & source       0/1 (depends if same application)
         */

        std::map<uint8_t, std::pair<std::string, uint16_t> > decisionTable =
        {
            {0x0, {"", 0}},
            {0x1, {((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "", 0}},
            {0x2, {((mask & IS_SRC_APP) == IS_SRC_APP) ? sourceElem->second : "", 0}},
            {0x3, {((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "", 0}},
            {0x4, {"", 0}},
            {0x5, {((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "", 0}},
            {0x6, {((mask & IS_SRC_APP) == IS_SRC_APP) ? sourceElem->second : "", 0}},
            {0x7, {isConnect ? (((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "") : (((mask & IS_SRC_APP) == IS_SRC_APP) ? sourceElem->second : ""), isSameApplicationSourceSink ? 0 : MULTICAST_MARKER_MSB}},
            {0x8, {"", 0}},
            {0x9, {"", 0}},
            {0xA, {"", 0}},
            {0xB, {((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "", 0}},
            {0xC, {"", 0}},
            {0xD, {((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "", MULTICAST_MARKER_MSB}},
            {0xE, {isConnect ? "" : (((mask & IS_SRC_APP) == IS_SRC_APP) ? sourceElem->second : ""), MULTICAST_MARKER_MSB}},
            {0xF, {isConnect ? (((mask & IS_SINK_APP) == IS_SINK_APP) ? sinkElem->second : "") : (((mask & IS_SRC_APP) == IS_SRC_APP) ? sourceElem->second : ""), isSameApplicationSourceSink ? 0 : MULTICAST_MARKER_MSB}},
        };

        dbus.append(decisionTable.at(mask).first);

        return decisionTable.at(mask).second;
    }

    std::string domainOwner;
    amri_CommunicationMode communicationMode;
    bool mIsThereRoutingAdapter;

    std::map<am_sourceID_t, std::string> mMapSrc;
    std::map<am_sinkID_t, std::string> mMapSink;

    am_Connection_s mConnection;
};

class CAmDbusRoutingSender:public IAmRoutingSend
{
public:
    CAmDbusRoutingSender();
    virtual ~CAmDbusRoutingSender();
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
    void removeHandle(const handle_uint16_s handle);
    void addDomainLookup(const am_domainID_t domainID, const dbus_comm_amri_s & lookupData);
    void removeDomainLookup(const am_domainID_t domainID);
    void removeConnectionLookup(const am_connectionID_t connectionID);
    void addSourceLookup(const am_Source_s & source, const std::string & application);
    void addSinkLookup(const am_Sink_s & sink, const std::string & application);
    am_domainID_t queryDomainLookup(const std::string &lookupNodename);
    void writeDomainsToMap(const std::vector<dr_domain_s> & domainArray);
    void establishIsThereRoutingAdapter(const std::vector<std::string> & dbusPaths, std::vector<dr_domain_s> & domains);
    am_Error_e asyncConnect(const handle_uint16_s handle);
    bool getRoutingReady(void);
private:
    template <typename TKey> static void  removeEntriesForValue(const dbus_comm_amri_s & value, std::map<TKey,dbus_comm_amri_s> & map);

private:
    CAmDbusWrapper* mpCAmDBusWrapper;
    DBusConnection* mpDBusConnection;
    IAmDbusRoutingReceiverShadow mIAmDbusRoutingReceiverShadow;
    IAmRoutingReceive* mpRoutingReceive;
    CDBusSender mCDBusSender;


    // To update Dubs communication details
    void updateRADbusCommDetails(void);
    // D-Bus parameter look up for multiple domains
    typedef std::map<am_domainID_t, dbus_comm_amri_s> mapDomain_t;
    typedef std::map<am_connectionID_t, dbus_comm_amri_s> mapConnections_t;
    typedef std::map< uint16_t, dbus_comm_amri_s> mapHandles_t;

    mapDomain_t mMapDomains;
    mapConnections_t mMapConnections;
    mapHandles_t mMapHandles;

    CAmDbusRoutingdb mDatabase;
    bool mSetRoutingReady;
};

}

#endif /* CAMDBUSROUTINGSENDER_H_ */
