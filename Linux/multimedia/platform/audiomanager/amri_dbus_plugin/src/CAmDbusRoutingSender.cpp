/************************************************************************
 * @file: CRaDbusWrpSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CAmDbusRoutingSender class implementation of Routing Adapter.
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

#include <map>
#include "CAmDbusRoutingSender.h"
#include "CDBusSender.h"
#include "CAmDltWrapper.h"
#include "CAmDbusWrapper.h"

using namespace am;
using namespace std;

DLT_DECLARE_CONTEXT (routingDbus)

extern "C" IAmRoutingSend* amri_dbus_pluginFactory()
{
    CAmDltWrapper::instance()->registerContext(routingDbus, "DRS", "DBus Plugin");

    return (new CAmDbusRoutingSender());
}

extern "C" void destroyamri_dbus_plugin(IAmRoutingSend* routingSendInterface)
{
    delete routingSendInterface;
}

CAmDbusRoutingSender::CAmDbusRoutingSender() :
        mpCAmDBusWrapper(NULL), mpDBusConnection(NULL),
        mIAmDbusRoutingReceiverShadow(this),
        mpRoutingReceive(NULL),
        mSetRoutingReady(false)
{
    CAmDbusRoutingParser parser(mDatabase);
    parser.readConfig();
    establishIsThereRoutingAdapter(mDatabase.getDBusInterfacesList(), mDatabase.getDomains());
}

CAmDbusRoutingSender::~CAmDbusRoutingSender()
{
    log(&routingDbus, DLT_LOG_INFO, "RoutingSender destructed");
    CAmDltWrapper::instance()->unregisterContext(routingDbus);
}

void CAmDbusRoutingSender::establishIsThereRoutingAdapter(const std::vector<std::string> & dbusPaths, std::vector<dr_domain_s> & domains)
{
    for (auto && it : domains)
    {
        it.isThereRA = false;
        for (auto && iit : dbusPaths)
        {
            if (iit.compare(string(DBUS_SERVICE_PREFIX) + "." + string(ROUTING_DBUS_NAMESPACE) + "." + it.nodeName) == 0)
            {
                it.isThereRA = true;
            }
        }
    }
}

void CAmDbusRoutingSender::writeDomainsToMap(const std::vector<dr_domain_s> & domainArray)
{
    for (auto it : domainArray)
    {
        addDomainLookup(it.id, dbus_comm_amri_s(ROUTING_DBUS_NAMESPACE, it.nodeName));

        mMapDomains.at(it.id).domainOwner = it.owner;
        mMapDomains.at(it.id).communicationMode = it.comMod;
        mMapDomains.at(it.id).mIsThereRoutingAdapter = it.isThereRA;

        for (auto iit : it.lSrc)
        {
            mMapDomains.at(it.id).mMapSrc[iit.id] = iit.application;
        }
        for (auto iit : it.lSnk)
        {
            mMapDomains.at(it.id).mMapSink[iit.id] = iit.application;
        }
    }
}

am_domainID_t CAmDbusRoutingSender::queryDomainLookup(const std::string & nodeName)
{
    dbus_comm_amri_s dbusInterface(ROUTING_DBUS_NAMESPACE, nodeName);
    for (auto && it : mMapDomains)
    {
        if(dbusInterface.iface.compare(it.second.iface) == 0)
        {
            return it.first;
        }
    }

    return 0;
}

am_Error_e CAmDbusRoutingSender::startupInterface(IAmRoutingReceive* pIAmRoutingReceive)
{
    am_Error_e ret_val = E_UNKNOWN;

    log(&routingDbus, DLT_LOG_INFO, "CAmDbusRoutingSender::startupInterface gets called");
    if (pIAmRoutingReceive != NULL)
    {
        mpRoutingReceive = pIAmRoutingReceive;
        mIAmDbusRoutingReceiverShadow.setRoutingReceiver(pIAmRoutingReceive);
        pIAmRoutingReceive->getDBusConnectionWrapper(mpCAmDBusWrapper);
        if (mpCAmDBusWrapper != NULL)
        {
            mpCAmDBusWrapper->getDBusConnection(mpDBusConnection);
            if (NULL != mpDBusConnection)
            {
                mCDBusSender.setDBusConnection(mpDBusConnection);

                /*
                 * Register Domain if needed
                 */
                for (auto && it : mDatabase.getDomains())
                {
                    if (!it.name.empty() && !it.nodeName.empty() && it.id == 0)
                    {
                        am_Domain_s domain;
                        domain.nodename = it.nodeName;
                        domain.domainID = it.id;
                        domain.name = it.name;
                        domain.busname = RA_DBUS_BUSNAME;
                        domain.state = DS_CONTROLLED;
                        mpRoutingReceive->registerDomain(domain, it.id);
                    }
                }

                writeDomainsToMap(mDatabase.getDomains());
                ret_val = E_OK;
            }
            else
            {
                logError("CAmDbusRoutingSender::startupInterface DBus connection not exist");
                ret_val = E_ABORTED;
            }
        }
        else
        {
            logError("CAmDbusRoutingSender::startupInterface DBus wrapper connection not exist");
            ret_val = E_ABORTED;
        }
    }
    else
    {
        logError(" CAmDbusRoutingSender::startupInterface AM receiver pointer is not initialised");
        ret_val = E_ABORTED;
    }

    return ret_val;
}

void CAmDbusRoutingSender::getInterfaceVersion(string & version) const
{
    version = RoutingVersion;
}

void CAmDbusRoutingSender::setRoutingReady(const uint16_t handle)
{
    (void)handle;
    mSetRoutingReady = true;
}

void CAmDbusRoutingSender::setRoutingRundown(const uint16_t handle)
{
    if (NULL != mpDBusConnection)
    {
        CDBusSender send(mpDBusConnection);
        send.initSignal(ROUTING_DBUS_NAMESPACE, "setRoutingRundown");
        send.append(handle);
        send.sendMessage();
    }
}

am_Error_e CAmDbusRoutingSender::asyncAbort(const am_Handle_s handle)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        mapHandles_t::iterator iter = mMapHandles.find(h.getUint16Handle());
        if (iter != mMapHandles.end())
        {
	        mCDBusSender.call("asyncAbort", iter->second);
	        mCDBusSender.append(h.getUint16Handle());
            ret = mCDBusSender.send_async();
            if(ret == E_OK)
            {
                removeHandle(h);
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncConnect(const handle_uint16_s handle)
{
    return asyncConnect(handle.getAmHandle(),
            mMapHandles.at(handle.getUint16Handle()).mConnection.connectionID,
            mMapHandles.at(handle.getUint16Handle()).mConnection.sourceID,
            mMapHandles.at(handle.getUint16Handle()).mConnection.sinkID,
            mMapHandles.at(handle.getUint16Handle()).mConnection.connectionFormat
    );
}

am_Error_e CAmDbusRoutingSender::asyncConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                              const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                              const am_CustomConnectionFormat_t connectionFormat)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                if ((h.getUint16Handle() & MULTICAST_MARKER_MSB) == 0)
                {
                    h.setHandle(h.getUint16Handle() | iter->second.getBitMask(sourceID, sinkID, dbus, true));
                }
                else
                {
                    h.setHandle(h.getUint16Handle() & (~MULTICAST_MARKER_MSB));
                    dbus.append(iter->second.mMapSrc[sourceID]);
                }
                logDebug("CAmDbusRoutingSender::asyncConnect - interface: ", dbus.iface);
                mCDBusSender.call("asyncConnect", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(connectionID);
                mCDBusSender.append(sourceID);
                mCDBusSender.append(sinkID);
                mCDBusSender.append(static_cast<int16_t>(connectionFormat));

                am_Connection_s & conn = iter->second.mConnection;
                conn.connectionID = connectionID;
                conn.sourceID = sourceID;
                conn.sinkID = sinkID;
                conn.connectionFormat = connectionFormat;

                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapConnections.insert(std::make_pair(connectionID, iter->second));
                    mMapConnections.at(connectionID).mMapSrc = iter->second.mMapSrc;
                    mMapConnections.at(connectionID).mMapSink = iter->second.mMapSink;
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), iter->second));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        mapConnections_t::iterator iter = mMapConnections.find(connectionID);
        if (iter != mMapConnections.end())
        {
            dbus_comm_amri_s dbus(iter->second);
            if ((h.getUint16Handle() & MULTICAST_MARKER_MSB) == 0)
            {
                h.setHandle(h.getUint16Handle() | iter->second.getBitMask(
                    iter->second.mConnection.sourceID,
                    iter->second.mConnection.sinkID,
                    dbus,
                    false));
            }
            else
            {
                h.setHandle(h.getUint16Handle() & (~MULTICAST_MARKER_MSB));
                dbus.append(iter->second.mMapSink[iter->second.mConnection.sinkID]);
            }
            logDebug("CAmDbusRoutingSender::asyncDisconnect - interface: ", dbus.iface);
            mCDBusSender.call("asyncDisconnect", dbus);
            mCDBusSender.append(h.getUint16Handle());
            mCDBusSender.append(connectionID);
            ret = mCDBusSender.send_async();
            if(ret == E_OK)
            {
                mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSinkVolume(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                    const am_volume_t volume, const am_CustomRampType_t ramp, const am_time_t time)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSink[sinkID]);
                mCDBusSender.call("asyncSetSinkVolume", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sinkID);
                mCDBusSender.append(volume);
                mCDBusSender.append(static_cast<int16_t>(ramp));
                mCDBusSender.append(time);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSourceVolume(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                      const am_volume_t volume, const am_CustomRampType_t ramp,
                                                      const am_time_t time)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSrc[sourceID]);
                mCDBusSender.call("asyncSetSourceVolume", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sourceID);
                mCDBusSender.append(volume);
                mCDBusSender.append(static_cast<int16_t>(ramp));
                mCDBusSender.append(time);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                   mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSourceState(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                     const am_SourceState_e state)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSrc[sourceID]);
                mCDBusSender.call("asyncSetSourceState", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sourceID);
                mCDBusSender.append(static_cast<int16_t>(state));
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSinkSoundProperties(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                             const vector<am_SoundProperty_s>& listSoundProperties)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSink[sinkID]);
                mCDBusSender.call("asyncSetSinkSoundProperties", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sinkID);
                mCDBusSender.append(listSoundProperties);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSinkSoundProperty(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                           const am_SoundProperty_s& soundProperty)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSink[sinkID]);
                mCDBusSender.call("asyncSetSinkSoundProperty", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sinkID);
                mCDBusSender.append(soundProperty);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSourceSoundProperties(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                               const vector<am_SoundProperty_s>& listSoundProperties)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSrc[sourceID]);
                mCDBusSender.call("asyncSetSourceSoundProperties", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sourceID);
                mCDBusSender.append(listSoundProperties);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSourceSoundProperty(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                             const am_SoundProperty_s& soundProperty)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSrc[sourceID]);
                mCDBusSender.call("asyncSetSourceSoundProperty", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sourceID);
                mCDBusSender.append(soundProperty);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncCrossFade(const am_Handle_s handle, const am_crossfaderID_t crossfaderID,
                                                const am_HotSink_e hotSink, const am_CustomRampType_t rampType,
                                                const am_time_t time)
{
#if 0
// TODO: mapping to target domain (am_Crossfader_s does not have a field for domainID ???).
    if (NULL != mpDBusConnection)
    {
        mCDBusSender.call("asyncCrossFade", mDBusData.dest, mDBusData.path, mDBusData.iface);
        mCDBusSender.append(handle.handle);
        mCDBusSender.append(crossfaderID);
        mCDBusSender.append(static_cast<int16_t>(hotSink));
        mCDBusSender.append(static_cast<int16_t>(rampType));
        mCDBusSender.append(time);
        mCDBusSender.send_async();
        return E_OK;
    }
#else
    (void) handle;
    (void) crossfaderID;
    (void) hotSink;
    (void) rampType;
    (void) time;
#endif
    return E_UNKNOWN;
}

am_Error_e CAmDbusRoutingSender::setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    am_Error_e ret_val = E_UNKNOWN;
    if (NULL != mpDBusConnection)
    {
        mapDomain_t::iterator iter = mMapDomains.find(domainID);
        if (iter != mMapDomains.end())
        {
            mCDBusSender.call("setDomainState", iter->second);
            mCDBusSender.append(domainID);
            mCDBusSender.append(static_cast<int16_t>(domainState));
            ret_val = mCDBusSender.send();
        }

    }
    return ret_val;
}

am_Error_e CAmDbusRoutingSender::returnBusName(string& BusName) const
{
    BusName = RA_DBUS_BUSNAME;
    return E_OK;
}

am_Error_e CAmDbusRoutingSender::asyncSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listVolumes)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        if (!listVolumes.empty())
        {
            // Assumption: the target for volume list is only one domain
            // finding out the domain of one element is sufficient
            am_Volumes_s volInfo = listVolumes[0];
            am_domainID_t domainID = 0;

            if (volInfo.volumeType == VT_SINK)
            {
                ret = mpRoutingReceive->getDomainOfSink(volInfo.volumeID.sink, domainID);
            }
            else if (volInfo.volumeType == VT_SOURCE)
            {
                ret = mpRoutingReceive->getDomainOfSource(volInfo.volumeID.source, domainID);
            }
            else
            {
                ret = E_UNKNOWN;
            }

            if (ret == E_OK)
            {
                mapDomain_t::iterator iter = mMapDomains.find(domainID);
                if (iter != mMapDomains.end())
                {
                    mCDBusSender.call("asyncSetVolumes", iter->second);
                    mCDBusSender.append(h.getUint16Handle());
                    mCDBusSender.append(listVolumes);
                    ret = mCDBusSender.send_async();
                    if(ret == E_OK)
                    {
                        mMapHandles.insert(std::make_pair(h.getUint16Handle(), iter->second));
                    }
                }
                else
                {
                    ret = E_UNKNOWN;
                }
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSinkNotificationConfiguration(
        const am_Handle_s handle, const am_sinkID_t sinkID, const am_NotificationConfiguration_s& notificationConfiguration)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSink(sinkID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSink[sinkID]);
                mCDBusSender.call("asyncSetSinkNotificationConfiguration", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sinkID);
                mCDBusSender.append(notificationConfiguration);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

am_Error_e CAmDbusRoutingSender::asyncSetSourceNotificationConfiguration(
        const am_Handle_s handle, const am_sourceID_t sourceID,
        const am_NotificationConfiguration_s& notificationConfiguration)
{
    am_Error_e ret = E_UNKNOWN;
    handle_uint16_s h(handle);
    if (NULL != mpDBusConnection)
    {
        am_domainID_t domainID = 0;
        ret = mpRoutingReceive->getDomainOfSource(sourceID, domainID);
        if (ret == E_OK)
        {
            mapDomain_t::iterator iter = mMapDomains.find(domainID);
            if (iter != mMapDomains.end())
            {
                dbus_comm_amri_s dbus(iter->second);
                dbus.append(iter->second.mMapSrc[sourceID]);
                mCDBusSender.call("asyncSetSourceNotificationConfiguration", dbus);
                mCDBusSender.append(h.getUint16Handle());
                mCDBusSender.append(sourceID);
                mCDBusSender.append(notificationConfiguration);
                ret = mCDBusSender.send_async();
                if(ret == E_OK)
                {
                    mMapHandles.insert(std::make_pair(h.getUint16Handle(), dbus));
                }
            }
            else
            {
                ret = E_UNKNOWN;
            }
        }
    }
    return ret;
}

bool CAmDbusRoutingSender::getRoutingReady()
{
    return mSetRoutingReady;
}

am_Error_e CAmDbusRoutingSender::resyncConnectionState(const am_domainID_t domainID, std::vector<am_Connection_s>& listOfExistingConnections)
{
    (void)domainID;
    (void)listOfExistingConnections;
    return E_OK;
}

void CAmDbusRoutingSender::removeHandle(const handle_uint16_s h)
{
    mMapHandles.erase(h.getUint16Handle());
    mMapHandles.erase(h.getUint16Handle() | MULTICAST_MARKER_MSB);
}

void CAmDbusRoutingSender::addDomainLookup(const am_domainID_t domainID, const dbus_comm_amri_s & lookupData)
{
    mMapDomains.insert(std::make_pair(domainID, lookupData));
    mMapDomains.at(domainID).domainOwner = lookupData.domainOwner;
}

void CAmDbusRoutingSender::addSourceLookup(const am_Source_s & source, const std::string & application)
{
    auto iter = mMapDomains.find(source.domainID);
    if (iter != mMapDomains.end())
    {
        iter->second.mMapSrc[source.sourceID] = application;
    }
}
void CAmDbusRoutingSender::addSinkLookup(const am_Sink_s & sink, const std::string & application)
{
    auto iter = mMapDomains.find(sink.domainID);
    if (iter != mMapDomains.end())
    {
        iter->second.mMapSink[sink.sinkID] = application;
    }
}

void CAmDbusRoutingSender::removeDomainLookup(const am_domainID_t domainID)
{
    mapDomain_t::iterator iter(mMapDomains.begin());
    iter = mMapDomains.find(domainID);
    if (iter != mMapDomains.end())
    {
        CAmDbusRoutingSender::removeEntriesForValue(iter->second, mMapHandles);
        CAmDbusRoutingSender::removeEntriesForValue(iter->second, mMapConnections);
        mMapDomains.erase(domainID);
    }
}

void CAmDbusRoutingSender::removeConnectionLookup(const am_connectionID_t connectionID)
{
    mMapConnections.erase(connectionID);
}

template <typename TKey> void  CAmDbusRoutingSender::removeEntriesForValue(const dbus_comm_amri_s & value, std::map<TKey,dbus_comm_amri_s> & map)
{
        typename std::map<TKey,dbus_comm_amri_s>::iterator it = map.begin();
        while ( it != map.end() )
        {
                if (it->second.dest == value.dest &&
                        it->second.iface == value.iface &&
                        it->second.path == value.path)
                {
                        typename std::map<TKey,dbus_comm_amri_s>::iterator it_tmp = it;
                        it++;
                        map.erase(it_tmp);
                }
                else
                        ++it;
        }
}

