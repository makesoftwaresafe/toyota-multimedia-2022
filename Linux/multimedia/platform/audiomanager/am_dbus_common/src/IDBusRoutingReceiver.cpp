/************************************************************************
 * @file: IDBusRoutingReceiver.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of Routing Adapter.
 * Receiver class will make call to AM via DBus connection.
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

#include <string.h>
#include <fstream>
#include <stdexcept>
#include "IDBusRoutingReceiver.h"
#include "CDBusSender.h"
#include "CDBusCommon.h"

using namespace std;
using namespace am;

//For SA fix
//DLT_IMPORT_CONTEXT (RA_DBus_wrapper)

IDBusRoutingReceiver::IDBusRoutingReceiver(DBusConnection* DbusConnection, const string& interface, const string application) :
        mpDBusConnection(DbusConnection), mDBusData(ROUTING_DBUS_NAMESPACE), mNodename(interface), mApplicationName(application)
{
}

IDBusRoutingReceiver::~IDBusRoutingReceiver()
{
}

void IDBusRoutingReceiver::ackConnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                          const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackConnect", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(connectionID);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackDisconnect(const am_Handle_s handle, const am_connectionID_t connectionID,
                                             const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackDisconnect", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(connectionID);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                      const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSinkVolumeChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(volume);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                                        const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSourceVolumeChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(volume);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSourceState(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSourceState", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSinkSoundProperties", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSinkSoundProperty", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSourceSoundProperties", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetSourceSoundProperty", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackCrossFading(const am_Handle_s handle, const am_HotSink_e hotSink,
                                              const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackCrossFading", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<int16_t>(hotSink));
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::ackSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID,
                                                   const am_volume_t volume)
{
    CDBusSender sender(mpDBusConnection, "ackSourceVolumeTick", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(sourceID);
    sender.append(volume);
    sender.send_async();
}

void IDBusRoutingReceiver::ackSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID,
                                                 const am_volume_t volume)
{
    CDBusSender sender(mpDBusConnection, "ackSinkVolumeTick", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(sinkID);
    sender.append(volume);
    sender.send_async();  // make non-blocking Dbus message call
}

am_Error_e IDBusRoutingReceiver::peekDomain(const string &name, am_domainID_t &domainID)
{
    CDBusSender sender(mpDBusConnection, "peekDomain", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(name);
    am_Error_e ret = sender.send_sync();  // make blocking Dbus message call
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        domainID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

//todo kapildev
//extern std::string gobjectpath;
//extern std::string gservicename;

am_Error_e IDBusRoutingReceiver::registerDomain(const am_Domain_s &domainData, am_domainID_t &domainID)
{
    CDBusSender sender(mpDBusConnection, "registerDomain", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Domain_s domain = domainData;
    domain.nodename = mNodename;
    sender.append(domain);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        domainID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterDomain(const am_domainID_t domainID)
{
    CDBusSender sender(mpDBusConnection, "deregisterDomain", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(domainID);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::registerGateway(const am_Gateway_s &gatewayData, am_gatewayID_t &gatewayID)
{
    CDBusSender sender(mpDBusConnection, "registerGateway", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(gatewayData);
    sender.append(mNodename);
    sender.append(mApplicationName);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        gatewayID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterGateway(const am_gatewayID_t gatewayID)
{
    CDBusSender sender(mpDBusConnection, "deregisterGateway", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(gatewayID);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::registerConverter(const am_Converter_s& converterData, am_converterID_t& converterID)
{
    CDBusSender send(mpDBusConnection, "registerConverter", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(converterData);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        converterID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterConverter(am_converterID_t converterID)
{
    CDBusSender send(mpDBusConnection, "deregisterConverter", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(converterID);

    return (send.send());
}
am_Error_e IDBusRoutingReceiver::peekSink(const string &name, am_sinkID_t &sinkID)
{
    CDBusSender sender(mpDBusConnection, "peekSink", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(name);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sinkID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::registerSink(const am_Sink_s &sinkData, am_sinkID_t &sinkID)
{
    CDBusSender sender(mpDBusConnection, "registerSink", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sinkData);
    sender.append(mNodename);
    sender.append(mApplicationName);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sinkID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterSink(const am_sinkID_t sinkID)
{
    CDBusSender sender(mpDBusConnection, "deregisterSink", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sinkID);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::peekSource(const string &name, am_sourceID_t &sourceID)
{
    CDBusSender sender(mpDBusConnection, "peekSource", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(name);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sourceID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::registerSource(const am_Source_s &sourceData, am_sourceID_t &sourceID)
{
    CDBusSender sender(mpDBusConnection, "registerSource", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceData);
    sender.append(mNodename);
    sender.append(mApplicationName);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sourceID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterSource(const am_sourceID_t sourceID)
{
    CDBusSender sender(mpDBusConnection, "deregisterSource", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceID);
    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::registerCrossfader(const am_Crossfader_s &crossfaderData,
                                                        am_crossfaderID_t &crossfaderID)
{
    CDBusSender sender(mpDBusConnection, "registerCrossfader", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(crossfaderData);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        crossfaderID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::deregisterCrossfader(const am_crossfaderID_t crossfaderID)
{
    CDBusSender sender(mpDBusConnection, "deregisterCrossfader", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(crossfaderID);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::peekSourceClassID(const string &name, am_sourceClass_t &sourceClassID)
{
    CDBusSender sender(mpDBusConnection, "peekSourceClassID", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(name);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sourceClassID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusRoutingReceiver::peekSinkClassID(const string &name, am_sinkClass_t &sinkClassID)
{
    CDBusSender sender(mpDBusConnection, "peekSinkClassID", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(name);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        sinkClassID = receive.getUInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

void IDBusRoutingReceiver::hookInterruptStatusChange(const am_sourceID_t sourceID,
                                                         const am_InterruptState_e interruptState)
{
    CDBusSender sender(mpDBusConnection, "hookInterruptStatusChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceID);
    sender.append(static_cast<int16_t>(interruptState));
    sender.send_async();
}

void IDBusRoutingReceiver::hookDomainRegistrationComplete(const am_domainID_t domainID)
{
    CDBusSender sender(mpDBusConnection, "hookDomainRegistrationComplete", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(domainID);
    sender.send_async();
}

void IDBusRoutingReceiver::hookSinkAvailablityStatusChange(const am_sinkID_t sinkID,
                                                               const am_Availability_s &availability)
{
    CDBusSender sender(mpDBusConnection, "hookSinkAvailablityStatusChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sinkID);
    sender.append(availability);
    sender.send_async();
}

void IDBusRoutingReceiver::hookSourceAvailablityStatusChange(const am_sourceID_t sourceID,
                                                                 const am_Availability_s &availability)
{
    CDBusSender sender(mpDBusConnection, "hookSourceAvailablityStatusChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceID);
    sender.append(availability);
    sender.send_async();
}

void IDBusRoutingReceiver::hookDomainStateChange(const am_domainID_t domainID, const am_DomainState_e domainState)
{
    CDBusSender sender(mpDBusConnection, "hookDomainStateChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(domainID);
    sender.append(static_cast<int16_t>(domainState));
    sender.send_async();
}

void IDBusRoutingReceiver::hookTimingInformationChanged(const am_connectionID_t connectionID,
                                                            const am_timeSync_t delay)
{
    CDBusSender sender(mpDBusConnection, "hookTimingInformationChanged", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(connectionID);
    sender.append(delay);
    sender.send_async();
}

void IDBusRoutingReceiver::sendChangedData(const vector<am_EarlyData_s> &earlyData)
{
    CDBusSender sender(mpDBusConnection, "sendChangedData", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(earlyData);
    sender.send_async();
}

am_Error_e IDBusRoutingReceiver::getDBusConnectionWrapper(CAmDbusWrapper *&dbusConnectionWrapper) const
{
    (void) ((dbusConnectionWrapper));
    return E_NON_EXISTENT;
}

am_Error_e IDBusRoutingReceiver::getSocketHandler(CAmSocketHandler *&socketHandler) const
{
    (void) ((socketHandler));
    return E_NON_EXISTENT;
}

void IDBusRoutingReceiver::getInterfaceVersion(string &version) const
{
    CDBusSender sender(mpDBusConnection, "getInterfaceVersion", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        version = receive.getString();
    }
}

void IDBusRoutingReceiver::confirmRoutingReady(const uint16_t handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "confirmRoutingReady", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(handle);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

void IDBusRoutingReceiver::confirmRoutingRundown(const uint16_t handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "confirmRoutingRundown", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(handle);
    sender.append(static_cast<uint16_t>(error));
    sender.send_async();
}

bool  IDBusRoutingReceiver::getRoutingReady(void)
{
    bool routingReady = false;
    CDBusSender sender(mpDBusConnection, "getRoutingReady", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = sender.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(sender.getDbusMessage());
        routingReady = receive.getBool();
    }
    return routingReady;
}

am_Error_e IDBusRoutingReceiver::updateGateway(const am_gatewayID_t gatewayid,
                                                   const vector<am_CustomConnectionFormat_t>& listsourceformats,
                                                   const vector<am_CustomConnectionFormat_t>& listsinkformats,
                                                   const vector<bool>& convertionmatrix)
{
    CDBusSender sender(mpDBusConnection, "updateGateway", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(gatewayid);
    sender.append(listsourceformats);
    sender.append(listsinkformats);
    sender.append(convertionmatrix);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::updateSink(const am_sinkID_t sinkid, const am_sinkClass_t sinkclassid,
                                                const vector<am_SoundProperty_s>& listsoundproperties,
                                                const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    CDBusSender sender(mpDBusConnection, "updateSink", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sinkid);
    sender.append(sinkclassid);
    sender.append(listsoundproperties);
    sender.append(listconnectionformats);
    sender.append(listmainsoundproperties);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::updateSource(const am_sourceID_t sourceid, const am_sourceClass_t sourceclassid,
                                                  const vector<am_SoundProperty_s>& listsoundproperties,
                                                  const vector<am_CustomConnectionFormat_t>& listconnectionformats,
                                                  const vector<am_MainSoundProperty_s>& listmainsoundproperties)
{
    CDBusSender sender(mpDBusConnection, "updateSource", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceid);
    sender.append(sourceclassid);
    sender.append(listsoundproperties);
    sender.append(listconnectionformats);
    sender.append(listmainsoundproperties);

    return (sender.send());
}

am_Error_e IDBusRoutingReceiver::updateConverter(const am_converterID_t converterID,
		const std::vector<am_CustomConnectionFormat_t>& listSourceFormats,
		const std::vector<am_CustomConnectionFormat_t>& listSinkFormats,
		const std::vector<bool>& convertionMatrix)
{
    CDBusSender send(mpDBusConnection, "updateConverter", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(converterID);
    send.append(listSourceFormats);
    send.append(listSinkFormats);
    send.append(convertionMatrix);

    return (send.send());
}
void IDBusRoutingReceiver::ackSetVolumes(const am_Handle_s handle, const vector<am_Volumes_s>& listvolumes,
                                             const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSetVolumes", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(listvolumes);
    sender.append(static_cast<uint16_t>(error));

    sender.send_async();
}

void IDBusRoutingReceiver::ackSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSinkNotificationConfiguration", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));

    sender.send_async();
}

void IDBusRoutingReceiver::ackSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error)
{
    CDBusSender sender(mpDBusConnection, "ackSourceNotificationConfiguration", mDBusData.dest, mDBusData.path, mDBusData.iface);
    handle_uint16_s h(handle);
    sender.append(h.getUint16Handle());
    sender.append(static_cast<uint16_t>(error));

    sender.send_async();
}

void IDBusRoutingReceiver::hookSinkNotificationDataChange(const am_sinkID_t sinkid,
                                                              const am_NotificationPayload_s& payload)
{
    CDBusSender sender(mpDBusConnection, "hookSinkNotificationDataChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sinkid);
    sender.append(payload);

    sender.send_async();
}

void IDBusRoutingReceiver::hookSourceNotificationDataChange(const am_sourceID_t sourceid,
                                                                const am_NotificationPayload_s& payload)
{
    CDBusSender sender(mpDBusConnection, "hookSourceNotificationDataChange", mDBusData.dest, mDBusData.path, mDBusData.iface);
    sender.append(sourceid);
    sender.append(payload);

    sender.send_async();
}

/*
 * Functions has to be implemented by routing plugin only
 */
am_Error_e IDBusRoutingReceiver::getDomainOfSink(const am_sinkID_t sinkID, am_domainID_t& domainID) const
{
	(void)sinkID;
	(void)domainID;
    return E_NOT_POSSIBLE;
}

am_Error_e IDBusRoutingReceiver::getDomainOfSource(const am_sourceID_t sourceID, am_domainID_t& domainID) const
{
	(void)sourceID;
	(void)domainID;
    return E_NOT_POSSIBLE;
}

am_Error_e IDBusRoutingReceiver::getDomainOfCrossfader(const am_crossfaderID_t crossfaderID, am_domainID_t& domainID) const
{
	(void)crossfaderID;
	(void)domainID;
    return E_NOT_POSSIBLE;
}
