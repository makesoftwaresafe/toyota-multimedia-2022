/************************************************************************
 * @file: IRaDbusWrpReceiverShadow.cpp
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of Routing Adapter.
 * IAmDbusRoutingReceiverShadow class will be running with the AM process.
 * IAmDbusRoutingReceiverShadow class methods will be called via DBus to
 * this methods will intern call the Actual call in AM.
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

#include <fstream> // for ifstream
#include <stdexcept> // for runtime_error
#include "IAmDbusRoutingReceiverShadow.h"
#include "CAmDltWrapper.h"
#include "CDBusCommon.h"
#include "CAmDbusRoutingSender.h"

DLT_IMPORT_CONTEXT (routingDbus)

using namespace std;
using namespace am;

/**
 * static ObjectPathTable is needed for DBus call back handling
 */
static DBusObjectPathVTable gObjectPathVTable;

IAmDbusRoutingReceiverShadow::IAmDbusRoutingReceiverShadow(CAmDbusRoutingSender* pCAmDbusRoutingSender)
        : mpIAmRoutingReceive(NULL), mpCAmDBusWrapper(NULL), mFunctionMap(createMap()),
          mpCAmDbusRoutingSender(pCAmDbusRoutingSender)
{
}

IAmDbusRoutingReceiverShadow::~IAmDbusRoutingReceiverShadow()
{
}

void IAmDbusRoutingReceiverShadow::registerDomain(DBusMessage *msg)
{
    am_Domain_s domainData;
    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getDomainData(domainData);
    /*
     * update the domain.busname with the "RoutingDbus". This is used
     * as mapping the different plugins inside audiomanager daemon.
     */
    domainData.busname = RA_DBUS_BUSNAME;
    dbus_comm_amri_s dbusInterface(ROUTING_DBUS_NAMESPACE, domainData.nodename);

    am_Error_e returnCode = mpIAmRoutingReceive->registerDomain(domainData, domainData.domainID);
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_ERROR, "error registering domain");
    }
    mCDBusSender.initReply(msg);
    mCDBusSender.append(domainData.domainID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
    mpCAmDbusRoutingSender->addDomainLookup(domainData.domainID, dbusInterface);
}

void IAmDbusRoutingReceiverShadow::registerSource(DBusMessage* msg)
{
    am_Source_s sourceData;
    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getSourceData(sourceData);
    std::string nodeName = mCDBusReceiver.getString();
    std::string application = mCDBusReceiver.getString();

    /*
     * When registering Resources from Application, the domainID is not known.
     * We need to fetch the proper domainID based on the nodename the Resource
     * is declaring to be part of.
     */
    sourceData.domainID = mpCAmDbusRoutingSender->queryDomainLookup(nodeName);
    am_Error_e returnCode;
    if (sourceData.domainID == 0)
    {
        returnCode = E_NOT_POSSIBLE;
    }
    else
    {
        returnCode = mpIAmRoutingReceive->registerSource(sourceData, sourceData.sourceID);
    }
    if (returnCode == E_OK || returnCode == E_ALREADY_EXISTS)
    {
        mpCAmDbusRoutingSender->addSourceLookup(sourceData, application);
    }
    else
    {
        log(&routingDbus, DLT_LOG_ERROR, "error registering source");
    }

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sourceData.sourceID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();

}

void IAmDbusRoutingReceiverShadow::registerSink(DBusMessage* msg)
{
    am_Sink_s sinkData;
    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getSinkData(sinkData);
    std::string nodeName = mCDBusReceiver.getString();
    std::string application = mCDBusReceiver.getString();

    /*
     * When registering Resources from Application, the domainID is not known.
     * We need to fetch the proper domainID based on the nodename the Resource
     * is declaring to be part of.
     */
    sinkData.domainID = mpCAmDbusRoutingSender->queryDomainLookup(nodeName);
    am_Error_e returnCode;
    if (sinkData.domainID == 0)
    {
        returnCode = E_NOT_POSSIBLE;
    }
    else
    {
        returnCode = mpIAmRoutingReceive->registerSink(sinkData, sinkData.sinkID);
    }
    if (returnCode == E_OK || returnCode == E_ALREADY_EXISTS)
    {
        mpCAmDbusRoutingSender->addSinkLookup(sinkData, application);
    }
    else
    {
        log(&routingDbus, DLT_LOG_ERROR, "error registering registerSink");
    }

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sinkData.sinkID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::registerGateway(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_Gateway_s gatewayData;
    mCDBusReceiver.getGatewayData(gatewayData);
    std::string nodeName = mCDBusReceiver.getString();
    std::string application = mCDBusReceiver.getString();

    /*
     * When registering Resources from Application, the domainID is not known.
     * We need to fetch the proper domainID based on the nodename the Resource
     * is declaring to be part of.
     */
    gatewayData.controlDomainID = mpCAmDbusRoutingSender->queryDomainLookup(nodeName);
    am_Error_e returnCode;
    if (gatewayData.controlDomainID == 0)
    {
        returnCode = E_NOT_POSSIBLE;
    }
    else
    {
        returnCode = mpIAmRoutingReceive->registerGateway(gatewayData, gatewayData.gatewayID);
    }
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_ERROR, "error registering gateway");
    }

    mCDBusSender.initReply(msg);
    mCDBusSender.append(gatewayData.gatewayID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookDomainRegistrationComplete(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_domainID_t domainID(mCDBusReceiver.getUInt());
    mpIAmRoutingReceive->hookDomainRegistrationComplete((am_domainID_t)((domainID)));
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackConnect(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_connectionID_t connectionID(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));

    /*
     * Here we implement the MULTICAST: if we receive an ackConnect whose
     * handle is "marked" as a MULTICAST, we then need to contact the next
     * node. We have stored the connection data in dbus, here we call an
     * overloaded asyncConnect in the sender, which will then retrieve all
     * the data and re-enter the asyncConnect.
     *
     * Whilst, if we already have an error, we should proceed with the removal
     *
     */
    if ((error == E_OK) && ((handle.getUint16Handle() & MULTICAST_MARKER_MSB) == MULTICAST_MARKER_MSB))
    {
        mpCAmDbusRoutingSender->asyncConnect(handle);
        return;
    }
    /*
     * This is needed to send back the proper handle to AM
     */
    handle_uint16_s unmasked(handle.getUint16Handle() & (uint16_t)(~MULTICAST_MARKER_MSB));

    mpIAmRoutingReceive->ackConnect(unmasked.getAmHandle(), connectionID, error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackDisconnect(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_connectionID_t connectionID(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));

    if ((error == E_OK) && ((handle.getUint16Handle() & MULTICAST_MARKER_MSB) == MULTICAST_MARKER_MSB))
    {
        mpCAmDbusRoutingSender->asyncDisconnect(handle.getAmHandle(), connectionID);
        return;
    }
    /*
     * This is needed to send back the proper handle to AM
     */
    handle_uint16_s unmasked(handle.getUint16Handle() & (uint16_t)(~MULTICAST_MARKER_MSB));

    mpIAmRoutingReceive->ackDisconnect(unmasked.getAmHandle(), connectionID, error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    /*
     * The map entry mapping a connection ID to the domain dbus information should be removed
     * only if the connection is successfully disconnected. If the lookup entry is removed
     * without this check any further disconnect request from the Audiomanager controller
     * would not be sent to the routing side.
     */
    if(error == E_OK)
    {
        mpCAmDbusRoutingSender->removeConnectionLookup(connectionID);
    }
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSinkVolumeChange(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSetSinkVolumeChange(handle.getAmHandle(), volume, error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSourceState(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSetSourceState(handle.getAmHandle(), error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSinkVolumeTick(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    mpIAmRoutingReceive->ackSinkVolumeTick(handle.getAmHandle(), sinkID, volume);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSourceVolumeTick(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    mpIAmRoutingReceive->ackSourceVolumeTick(handle.getAmHandle(), sourceID, volume);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSourceVolumeChange(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSetSourceVolumeChange(handle.getAmHandle(), volume, error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSinkSoundProperty(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSetSinkSoundProperty(handle.getAmHandle(), error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSourceSoundProperty(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSetSourceSoundProperty(handle.getAmHandle(), error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSinkSoundProperties(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error = (am_Error_e)((mCDBusReceiver.getUInt()));
    mpIAmRoutingReceive->ackSetSinkSoundProperties(handle.getAmHandle(), error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetSourceSoundProperties(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error = (am_Error_e)((mCDBusReceiver.getUInt()));
    mpIAmRoutingReceive->ackSetSourceSoundProperties(handle.getAmHandle(), error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackCrossFading(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_HotSink_e hotsink = (am_HotSink_e)((mCDBusReceiver.getInt()));
    am_Error_e error = (am_Error_e)((mCDBusReceiver.getUInt()));
    mpIAmRoutingReceive->ackCrossFading(handle.getAmHandle(), hotsink, error);
    mpCAmDbusRoutingSender->removeHandle(handle);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::peekDomain(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    string name = string(mCDBusReceiver.getString());
    am_domainID_t domainID;
    am_Error_e returnCode = mpIAmRoutingReceive->peekDomain(name, domainID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(domainID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::deregisterDomain(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_domainID_t domainID = mCDBusReceiver.getUInt();
    am_Error_e returnCode = mpIAmRoutingReceive->deregisterDomain(domainID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
    mpCAmDbusRoutingSender->removeDomainLookup(domainID);
}

void IAmDbusRoutingReceiverShadow::deregisterGateway(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_gatewayID_t gatewayID = mCDBusReceiver.getUInt();
    am_Error_e returnCode = mpIAmRoutingReceive->deregisterGateway(gatewayID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::peekSink(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    string name = string(mCDBusReceiver.getString());
    am_sinkID_t sinkID;
    am_Error_e returnCode = mpIAmRoutingReceive->peekSink(name, sinkID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sinkID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::deregisterSink(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = mCDBusReceiver.getUInt();
    am_Error_e returnCode = mpIAmRoutingReceive->deregisterSink(sinkID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::peekSource(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    string name = string(mCDBusReceiver.getString());
    am_sourceID_t sourceID;
    am_Error_e returnCode = mpIAmRoutingReceive->peekSource(name, sourceID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sourceID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::deregisterSource(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = mCDBusReceiver.getUInt();
    am_Error_e returnCode = mpIAmRoutingReceive->deregisterSource(sourceID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::registerCrossfader(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_Crossfader_s crossfader;
    crossfader.crossfaderID = mCDBusReceiver.getUInt();
    crossfader.name = string(mCDBusReceiver.getString());
    crossfader.sinkID_A = mCDBusReceiver.getUInt();
    crossfader.sinkID_B = mCDBusReceiver.getUInt();
    crossfader.sourceID = mCDBusReceiver.getUInt();
    crossfader.hotSink = (am_HotSink_e)((mCDBusReceiver.getInt()));
    am_Error_e returnCode = mpIAmRoutingReceive->registerCrossfader(crossfader, crossfader.crossfaderID);
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering crossfader");
    }

    mCDBusSender.initReply(msg);
    mCDBusSender.append(crossfader.crossfaderID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::deregisterCrossfader(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_crossfaderID_t crossfaderID = mCDBusReceiver.getInt();
    am_Error_e returnCode = mpIAmRoutingReceive->deregisterCrossfader(crossfaderID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::peekSourceClassID(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    string name = string(mCDBusReceiver.getString());
    am_sourceClass_t sourceClassID;
    am_Error_e returnCode = mpIAmRoutingReceive->peekSourceClassID(name, sourceClassID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sourceClassID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::peekSinkClassID(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    string name = string(mCDBusReceiver.getString());
    am_sinkClass_t sinkClassID;
    am_Error_e returnCode = mpIAmRoutingReceive->peekSinkClassID(name, sinkClassID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(sinkClassID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookInterruptStatusChange(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = mCDBusReceiver.getUInt();
    am_InterruptState_e interruptState = (am_InterruptState_e)((mCDBusReceiver.getInt()));
    mpIAmRoutingReceive->hookInterruptStatusChange(sourceID, interruptState);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookSinkAvailablityStatusChange(DBusMessage* msg)
{
    am_Availability_s avialabilty;
    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = mCDBusReceiver.getUInt();
    mCDBusReceiver.getAvailability(avialabilty);
    mpIAmRoutingReceive->hookSinkAvailablityStatusChange(sinkID, avialabilty);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookSourceAvailablityStatusChange(DBusMessage* msg)
{
    am_Availability_s avialabilty;
    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = mCDBusReceiver.getUInt();
    mCDBusReceiver.getAvailability(avialabilty);
    mpIAmRoutingReceive->hookSourceAvailablityStatusChange(sourceID, avialabilty);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookDomainStateChange(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_domainID_t domainID = mCDBusReceiver.getInt();
    am_DomainState_e domainState = (am_DomainState_e)((mCDBusReceiver.getInt()));
    mpIAmRoutingReceive->hookDomainStateChange(domainID, domainState);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookTimingInformationChanged(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    am_connectionID_t connectionID(mCDBusReceiver.getUInt());
    am_timeSync_t time(mCDBusReceiver.getInt());
    mpIAmRoutingReceive->hookTimingInformationChanged(connectionID, time);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::sendChangedData(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    vector<am_EarlyData_s> listEarlyData;
    mCDBusReceiver.getEarlyData(listEarlyData);
    mpIAmRoutingReceive->sendChangedData(listEarlyData);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::confirmRoutingReady(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    uint16_t handle = mCDBusReceiver.getUInt();
    am_Error_e error = static_cast<am_Error_e>(mCDBusReceiver.getUInt());
    mpIAmRoutingReceive->confirmRoutingReady(handle, error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::confirmRoutingRundown(DBusMessage* msg)
{
    mCDBusReceiver.initReceive(msg);
    uint16_t handle = mCDBusReceiver.getUInt();
    am_Error_e error = static_cast<am_Error_e>(mCDBusReceiver.getUInt());
    mpIAmRoutingReceive->confirmRoutingRundown(handle, error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::updateGateway(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    vector<am_CustomConnectionFormat_t> listsourceformats;
    vector<am_CustomConnectionFormat_t> listsinkformats;
    vector<bool> convertionmatrix;
    am_gatewayID_t gatewayid = static_cast<am_gatewayID_t>(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListConnFrmt(listsourceformats);
    mCDBusReceiver.getListConnFrmt(listsinkformats);
    mCDBusReceiver.getConvertionMatrix(convertionmatrix);
    am_Error_e returnCode = mpIAmRoutingReceive->updateGateway(gatewayid, listsourceformats, listsinkformats,
                                                                    convertionmatrix);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::updateSink(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    vector<am_SoundProperty_s> listsoundproperties;
    vector<am_CustomConnectionFormat_t> listconnectionformats;
    vector<am_MainSoundProperty_s> listmainsoundproperty;
    am_sinkID_t sinkid = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_sinkClass_t sinkclassid = static_cast<am_sinkClass_t>(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListSoundProperties(listsoundproperties);
    mCDBusReceiver.getListConnFrmt(listconnectionformats);
    mCDBusReceiver.getListMainSoundProperties(listmainsoundproperty);
    am_Error_e returnCode = mpIAmRoutingReceive->updateSink(sinkid, sinkclassid, listsoundproperties,
                                                                 listconnectionformats, listmainsoundproperty);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::updateSource(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    vector<am_SoundProperty_s> listsoundproperties;
    vector<am_CustomConnectionFormat_t> listconnectionformats;
    vector<am_MainSoundProperty_s> listmainsoundproperty;
    am_sourceID_t sourceid = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_sourceClass_t sourceclassid = static_cast<am_sourceClass_t>(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListSoundProperties(listsoundproperties);
    mCDBusReceiver.getListConnFrmt(listconnectionformats);
    mCDBusReceiver.getListMainSoundProperties(listmainsoundproperty);
    am_Error_e returnCode = mpIAmRoutingReceive->updateSource(sourceid, sourceclassid, listsoundproperties,
                                                                   listconnectionformats, listmainsoundproperty);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSetVolumes(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    vector<am_Volumes_s> listvolumes;
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListVolumes(listvolumes);
    am_Error_e error((am_Error_e)(mCDBusReceiver.getUInt()));
    mpIAmRoutingReceive->ackSetVolumes(handle.getAmHandle(), listvolumes, error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::ackSinkNotificationConfiguration(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSinkNotificationConfiguration(handle.getAmHandle(), error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();

}

void IAmDbusRoutingReceiverShadow::ackSourceNotificationConfiguration(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e error((am_Error_e)((mCDBusReceiver.getUInt())));
    mpIAmRoutingReceive->ackSourceNotificationConfiguration(handle.getAmHandle(), error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookSinkNotificationDataChange(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    am_NotificationPayload_s payload;
    am_sinkID_t sinkid = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    mCDBusReceiver.getNotificationPayload(payload);

    mpIAmRoutingReceive->hookSinkNotificationDataChange(sinkid, payload);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::hookSourceNotificationDataChange(DBusMessage *msg)
{
    mCDBusReceiver.initReceive(msg);
    am_NotificationPayload_s payload;
    am_sourceID_t sourceid = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    mCDBusReceiver.getNotificationPayload(payload);

    mpIAmRoutingReceive->hookSourceNotificationDataChange(sourceid, payload);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::getInterfaceVersion(DBusMessage *msg)
{
    logInfo("IAmDbusRoutingReceiverShadow::getInterfaceVersion gets called");
    string version;

    mCDBusReceiver.initReceive(msg);
    mpIAmRoutingReceive->getInterfaceVersion(version);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(version);
    mCDBusSender.sendMessage();
}

void IAmDbusRoutingReceiverShadow::getRoutingReady(DBusMessage *msg)
{
    logInfo("IAmDbusRoutingReceiverShadow::getRoutingReady gets called");

    mCDBusReceiver.initReceive(msg);
    bool routingReady = mpCAmDbusRoutingSender->getRoutingReady();

    mCDBusSender.initReply(msg);
    mCDBusSender.append(routingReady);
    mCDBusSender.sendMessage();

}

DBusHandlerResult IAmDbusRoutingReceiverShadow::receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    DBusHandlerResult ret_val = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if ((conn != NULL) && (msg != NULL) && (user_data != NULL))
    {
        IAmDbusRoutingReceiverShadow* reference = (IAmDbusRoutingReceiverShadow*) ((user_data));
        ret_val = reference->receiveCallbackDelegate(conn, msg);
    }
    else
    {
        logError("IAmDbusRoutingReceiverShadow::receiveCallback DBus pointer or DBus message or user data not initialised");
    }
    return ret_val;
}

void IAmDbusRoutingReceiverShadow::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
    if ((conn != NULL) && (msg != NULL))
    {
        DBusMessage* reply;
        DBusMessageIter args;
        dbus_uint32_t serial = 0;

        // create a reply from the message
        reply = dbus_message_new_method_return(msg);
        string fullpath(ROUTING_RECV_DBUS_INTROSPECTION_FILE);
        ifstream in(fullpath.c_str(), ifstream::in);
        if (!in)
        {
            logError("IAmCommandReceiverShadow::sendIntrospection could not load xml file ", fullpath);
            throw runtime_error("IAmCommandReceiverShadow::sendIntrospection Could not load introspecton XML");
        }
        string introspect((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        const char* string = introspect.c_str();

        // add the arguments to the reply
        dbus_message_iter_init_append(reply, &args);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &string))
        {
            log(&routingDbus, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
        }

        // send the reply && flush the connection
        if (!dbus_connection_send(conn, reply, &serial))
        {
            log(&routingDbus, DLT_LOG_ERROR, "DBUS handler Out Of Memory!");
        }
        dbus_connection_flush(conn);

        // free the reply
        dbus_message_unref(reply);
    }
    else
    {
        logError("IAmDbusRoutingReceiverShadow::sendIntrospection DBus and/or Message pointer not initialised");
    }
}

DBusHandlerResult IAmDbusRoutingReceiverShadow::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "CallBack"))
    {
        sendIntrospection(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    char* messagename = (char*)dbus_message_get_member(msg);
    if(messagename == NULL)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    functionMap_t::iterator iter = mFunctionMap.begin();
    const char *methodName = dbus_message_get_member(msg);
    if (NULL != methodName) {
        string k(methodName);
        iter = mFunctionMap.find(k);
        if (iter != mFunctionMap.end())
        {
            string p(iter->first);
            CallBackMethod cb = iter->second;
            (this->*cb)(msg);
            return (DBUS_HANDLER_RESULT_HANDLED);
        }
    }

    return (DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

void IAmDbusRoutingReceiverShadow::setRoutingReceiver(IAmRoutingReceive*& receiver)
{
    mpIAmRoutingReceive = receiver;
    gObjectPathVTable.message_function = IAmDbusRoutingReceiverShadow::receiveCallback;
    DBusConnection* connection;
    mpIAmRoutingReceive->getDBusConnectionWrapper(mpCAmDBusWrapper);
    if (mpCAmDBusWrapper != NULL)
    {
        mpCAmDBusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mCDBusReceiver.setDBusConnection(connection);
            mCDBusSender.setDBusConnection(connection);
            string path(ROUTING_DBUS_NAMESPACE);
            mpCAmDBusWrapper->registerCallback(&gObjectPathVTable, path, this);
        }
        else
        {
            logError("IAmDbusRoutingReceiverShadow::setRoutingReceiver Failed to get DBus connection pointer");
        }
    }
    else
    {
        logError("IAmDbusRoutingReceiverShadow::setRoutingReceiver Failed to get DBus wrapper pointer");
    }
}

IAmDbusRoutingReceiverShadow::functionMap_t IAmDbusRoutingReceiverShadow::createMap()
{
    functionMap_t m;
    m["ackConnect"] = &IAmDbusRoutingReceiverShadow::ackConnect;
    m["ackDisconnect"] = &IAmDbusRoutingReceiverShadow::ackDisconnect;
    m["ackSetSinkVolumeChange"] = &IAmDbusRoutingReceiverShadow::ackSetSinkVolumeChange;
    m["ackSetSourceVolumeChange"] = &IAmDbusRoutingReceiverShadow::ackSetSourceVolumeChange;
    m["ackSetSourceState"] = &IAmDbusRoutingReceiverShadow::ackSetSourceState;
    m["ackSinkVolumeTick"] = &IAmDbusRoutingReceiverShadow::ackSinkVolumeTick;
    m["ackSourceVolumeTick"] = &IAmDbusRoutingReceiverShadow::ackSourceVolumeTick;
    m["ackSetSinkSoundProperty"] = &IAmDbusRoutingReceiverShadow::ackSetSinkSoundProperty;
    m["ackSetSourceSoundProperty"] = &IAmDbusRoutingReceiverShadow::ackSetSourceSoundProperty;
    m["ackSetSinkSoundProperties"] = &IAmDbusRoutingReceiverShadow::ackSetSinkSoundProperties;
    m["ackSetSourceSoundProperties"] = &IAmDbusRoutingReceiverShadow::ackSetSourceSoundProperties;
    m["ackCrossFading"] = &IAmDbusRoutingReceiverShadow::ackCrossFading;
    m["registerDomain"] = &IAmDbusRoutingReceiverShadow::registerDomain;
    m["registerSource"] = &IAmDbusRoutingReceiverShadow::registerSource;
    m["registerSink"] = &IAmDbusRoutingReceiverShadow::registerSink;
    m["registerGateway"] = &IAmDbusRoutingReceiverShadow::registerGateway;
    m["peekDomain"] = &IAmDbusRoutingReceiverShadow::peekDomain;
    m["deregisterDomain"] = &IAmDbusRoutingReceiverShadow::deregisterDomain;
    m["deregisterGateway"] = &IAmDbusRoutingReceiverShadow::deregisterGateway;
    m["peekSink"] = &IAmDbusRoutingReceiverShadow::peekSink;
    m["deregisterSink"] = &IAmDbusRoutingReceiverShadow::deregisterSink;
    m["peekSource"] = &IAmDbusRoutingReceiverShadow::peekSource;
    m["deregisterSource"] = &IAmDbusRoutingReceiverShadow::deregisterSource;
    m["registerCrossfader"] = &IAmDbusRoutingReceiverShadow::registerCrossfader;
    m["deregisterCrossfader"] = &IAmDbusRoutingReceiverShadow::deregisterCrossfader;
    m["peekSourceClassID"] = &IAmDbusRoutingReceiverShadow::peekSourceClassID;
    m["peekSinkClassID"] = &IAmDbusRoutingReceiverShadow::peekSinkClassID;
    m["hookInterruptStatusChange"] = &IAmDbusRoutingReceiverShadow::hookInterruptStatusChange;
    m["hookDomainRegistrationComplete"] = &IAmDbusRoutingReceiverShadow::hookDomainRegistrationComplete;
    m["hookSinkAvailablityStatusChange"] = &IAmDbusRoutingReceiverShadow::hookSinkAvailablityStatusChange;
    m["hookSourceAvailablityStatusChange"] = &IAmDbusRoutingReceiverShadow::hookSourceAvailablityStatusChange;
    m["hookDomainStateChange"] = &IAmDbusRoutingReceiverShadow::hookDomainStateChange;
    m["hookTimingInformationChanged"] = &IAmDbusRoutingReceiverShadow::hookTimingInformationChanged;
    m["sendChangedData"] = &IAmDbusRoutingReceiverShadow::sendChangedData;
    m["confirmRoutingReady"] = &IAmDbusRoutingReceiverShadow::confirmRoutingReady;
    m["confirmRoutingRundown"] = &IAmDbusRoutingReceiverShadow::confirmRoutingRundown;
    m["updateGateway"] = &IAmDbusRoutingReceiverShadow::updateGateway;
    m["updateSink"] = &IAmDbusRoutingReceiverShadow::updateSink;
    m["updateSource"] = &IAmDbusRoutingReceiverShadow::updateSource;
    m["ackSetVolumes"] = &IAmDbusRoutingReceiverShadow::ackSetVolumes;
    m["ackSinkNotificationConfiguration"] = &IAmDbusRoutingReceiverShadow::ackSinkNotificationConfiguration;
    m["ackSourceNotificationConfiguration"] = &IAmDbusRoutingReceiverShadow::ackSourceNotificationConfiguration;
    m["hookSinkNotificationDataChange"] = &IAmDbusRoutingReceiverShadow::hookSinkNotificationDataChange;
    m["hookSourceNotificationDataChange"] = &IAmDbusRoutingReceiverShadow::hookSourceNotificationDataChange;
    m["getInterfaceVersion"] = &IAmDbusRoutingReceiverShadow::getInterfaceVersion;
    m["getRoutingReady"] = &IAmDbusRoutingReceiverShadow::getRoutingReady;
    return (m);
}
