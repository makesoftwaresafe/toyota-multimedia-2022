/**
 *  Copyright (c) copyright 2011-2012 AricentÂ® Group  and its licensors
 *  Copyright (c) 2012 BMW
 *
 *  \author Sampreeth Ramavana
 *	\author Christian Linke, christian.linke@bmw.de BMW 2011,2012
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

#include "IAmRoutingReceiverShadow.h"

#include <string.h>
#include <fstream>
#include <stdexcept>
#include <cassert>

#include "configRoutingDbus.h"
#include "CAmRoutingSenderDbus.h"
#include "CAmDbusWrapper.h"
#include "CAmDltWrapper.h"

namespace am
{

DLT_IMPORT_CONTEXT(routingDbus)


/**
 * static ObjectPathTable is needed for DBus Callback handling
 */
static DBusObjectPathVTable gObjectPathVTable;

IAmRoutingReceiverShadowDbus::IAmRoutingReceiverShadowDbus(CAmRoutingSenderDbus* pRoutingSenderDbus) :
        mRoutingReceiveInterface(NULL), //
        mDBusWrapper(NULL), //
        mpRoutingSenderDbus(pRoutingSenderDbus), //
        mFunctionMap(createMap()), //
        mDBUSMessageHandler(), //
        mNumberDomains(0), //
        mHandle(0), //
        mRoutingReady(false)
{
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow constructed");
}

IAmRoutingReceiverShadowDbus::~IAmRoutingReceiverShadowDbus()
{
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow destructed");
}

void IAmRoutingReceiverShadowDbus::registerDomain(DBusConnection *conn, DBusMessage *msg)
{
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::registerDomain called");

    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Domain_s domain(mDBUSMessageHandler.getDomainData());
    CAmRoutingSenderDbus::rs_lookupData_s lookupData;
    lookupData.busname = mDBUSMessageHandler.getString();
    lookupData.path = mDBUSMessageHandler.getString();
    lookupData.interface = mDBUSMessageHandler.getString();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::registerDomain called, name ", domain.name, "nodename ", domain.nodename);
    domain.busname = "DbusRoutingPlugin";
    am_Error_e returnCode = mRoutingReceiveInterface->registerDomain(domain, domain.domainID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(domain.domainID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering domain");
        return;
    }
    mpRoutingSenderDbus->addDomainLookup(domain.domainID, lookupData);
}

void IAmRoutingReceiverShadowDbus::registerSource(DBusConnection* conn, DBusMessage* msg)
{
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::RegisterSource called");
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Source_s sourceData(mDBUSMessageHandler.getSourceData());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::registerSource called, name", sourceData.name, "mSource.sourceClassID", sourceData.sourceClassID, "mSource.domainID", sourceData.domainID);
    am_Error_e returnCode = mRoutingReceiveInterface->registerSource(sourceData, sourceData.sourceID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sourceData.sourceID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering source");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::registerSink(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Sink_s sinkData(mDBUSMessageHandler.getSinkData());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::registerSink called, name", sinkData.name, "mSink.sinkClassID", sinkData.sinkClassID, "mSink.domainID", sinkData.domainID);
    am_Error_e returnCode = mRoutingReceiveInterface->registerSink(sinkData, sinkData.sinkID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sinkData.sinkID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering sink");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::registerGateway(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Gateway_s gatewayData(mDBUSMessageHandler.getGatewayData());
    am_Error_e returnCode = mRoutingReceiveInterface->registerGateway(gatewayData, gatewayData.gatewayID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(gatewayData.gatewayID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering gateway");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::registerConverter(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Converter_s gatewayData(mDBUSMessageHandler.getConverterData());
    am_Error_e returnCode = mRoutingReceiveInterface->registerConverter(gatewayData, gatewayData.converterID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(gatewayData.converterID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering gateway");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::hookDomainRegistrationComplete(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_domainID_t domainID(mDBUSMessageHandler.getUInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookDomainRegistrationComplete called, domainID", domainID);
    mRoutingReceiveInterface->hookDomainRegistrationComplete((am_domainID_t)((domainID)));
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackConnect(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_connectionID_t connectionID(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackConnect called, handle", handle, "connectionID", connectionID, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_CONNECT;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackConnect(myhandle, connectionID, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackDisconnect(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_connectionID_t connectionID(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackDisconnect called, handle", handle, "connectionID", connectionID, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_DISCONNECT;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackDisconnect(myhandle, connectionID, error);
    mpRoutingSenderDbus->removeHandle(handle);
    //todo: Connection removal ???
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSinkVolume(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_volume_t volume(mDBUSMessageHandler.getInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSinkVolume called, handle", handle, "error", error, "volume", volume);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSINKVOLUME;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSinkVolumeChange(myhandle, volume, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSourceState(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSourceState called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSOURCESTATE;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSourceState(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSinkVolumeTick(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_sinkID_t sinkID(mDBUSMessageHandler.getUInt());
    am_volume_t volume(mDBUSMessageHandler.getInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSinkVolumeTick called, handle", handle, "sinkID", sinkID, "volume", volume);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSINKVOLUME;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSinkVolumeTick(myhandle, sinkID, volume);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSourceVolumeTick(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_sourceID_t sourceID(mDBUSMessageHandler.getUInt());
    am_volume_t volume(mDBUSMessageHandler.getInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSourceVolumeTick called, handle", handle, "sourceID", sourceID, "volume", volume);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSOURCEVOLUME;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSourceVolumeTick(myhandle, sourceID, volume);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSourceVolume(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_volume_t volume(mDBUSMessageHandler.getInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSourceVolume called, handle", handle, "volume", volume, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSOURCEVOLUME;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSourceVolumeChange(myhandle, volume, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSinkSoundProperty(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSinkSoundProperty called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSINKSOUNDPROPERTY;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSinkSoundProperty(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSourceSoundProperty(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSinkSoundProperty called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSOURCESOUNDPROPERTY;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSourceSoundProperty(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSinkSoundProperties(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle = mDBUSMessageHandler.getUInt();
    am_Error_e error = (am_Error_e)((mDBUSMessageHandler.getUInt()));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSinkSoundProperties called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSINKSOUNDPROPERTIES;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSinkSoundProperties(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSetSourceSoundProperties(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle = mDBUSMessageHandler.getUInt();
    am_Error_e error = (am_Error_e)((mDBUSMessageHandler.getUInt()));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSetSourceSoundProperties called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_SETSOURCESOUNDPROPERTIES;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSetSourceSoundProperties(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackCrossFading(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle = mDBUSMessageHandler.getUInt();
    am_HotSink_e hotsink = (am_HotSink_e)((mDBUSMessageHandler.getInt()));
    am_Error_e error = (am_Error_e)((mDBUSMessageHandler.getUInt()));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackCrossFading called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_CROSSFADE;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackCrossFading(myhandle, hotsink, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::peekDomain(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::string name = std::string(mDBUSMessageHandler.getString());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::peekDomain called, name", name);
    am_domainID_t domainID;
    am_Error_e returnCode = mRoutingReceiveInterface->peekDomain(name, domainID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(domainID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::deregisterDomain(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_domainID_t domainID = mDBUSMessageHandler.getUInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterDomain called, id", domainID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterDomain(domainID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    mpRoutingSenderDbus->removeDomainLookup(domainID);
}

void IAmRoutingReceiverShadowDbus::deregisterGateway(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_gatewayID_t gatewayID = mDBUSMessageHandler.getUInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterGateway called, id", gatewayID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterGateway(gatewayID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::deregisterConverter(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_converterID_t converterID = mDBUSMessageHandler.getUInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterGateway called, id", converterID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterConverter(converterID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::peekSink(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::string name = std::string(mDBUSMessageHandler.getString());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::peekSink called, name", name);
    am_sinkID_t sinkID;
    am_Error_e returnCode = mRoutingReceiveInterface->peekSink(name, sinkID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sinkID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::deregisterSink(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sinkID_t sinkID = mDBUSMessageHandler.getInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterSink called, id", sinkID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterSink(sinkID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::peekSource(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::string name = std::string(mDBUSMessageHandler.getString());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::peekSource called, name", name);
    am_sourceID_t sourceID;
    am_Error_e returnCode = mRoutingReceiveInterface->peekSource(name, sourceID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sourceID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::deregisterSource(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID = mDBUSMessageHandler.getInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterSource called, id", sourceID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterSource(sourceID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::registerCrossfader(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_Crossfader_s crossfader (mDBUSMessageHandler.getCrossfaderData());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::registerCrossfader called, name", crossfader.name);
    am_Error_e returnCode = mRoutingReceiveInterface->registerCrossfader(crossfader, crossfader.crossfaderID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(crossfader.crossfaderID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error registering crossfader");
        return;
    }
    //todo: add Crossfader lookup
}

void IAmRoutingReceiverShadowDbus::deregisterCrossfader(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_crossfaderID_t crossfaderID = mDBUSMessageHandler.getInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::deregisterCrossfader called, id", crossfaderID);
    am_Error_e returnCode = mRoutingReceiveInterface->deregisterCrossfader(crossfaderID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    //todo: remove Crossfader lookup
}

void IAmRoutingReceiverShadowDbus::peekSourceClassID(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::string name = std::string(mDBUSMessageHandler.getString());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::peekSourceClassID called, name", name);
    am_sourceClass_t sourceClassID;
    am_Error_e returnCode = mRoutingReceiveInterface->peekSourceClassID(name, sourceClassID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sourceClassID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::getDomainOfSource(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    am_domainID_t domainID=0;
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID = mDBUSMessageHandler.getUInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::getDomainOfSource called, ID", sourceID);
    am_Error_e returnCode = mRoutingReceiveInterface->getDomainOfSource(sourceID, domainID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(domainID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::getDomainOfSink(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    am_domainID_t domainID=0;

    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sinkID_t sinkID = mDBUSMessageHandler.getUInt();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::getDomainOfSource called, ID", sinkID);
    am_Error_e returnCode = mRoutingReceiveInterface->getDomainOfSource(sinkID, domainID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(domainID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();

}

void IAmRoutingReceiverShadowDbus::peekSinkClassID(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::string name = std::string(mDBUSMessageHandler.getString());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::peekSinkClassID called, name", name);
    am_sinkClass_t sinkClassID;
    am_Error_e returnCode = mRoutingReceiveInterface->peekSinkClassID(name, sinkClassID);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(sinkClassID);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::hookInterruptStatusChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID = mDBUSMessageHandler.getUInt();
    am_InterruptState_e interruptState = (am_InterruptState_e)((mDBUSMessageHandler.getUInt()));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookInterruptStatusChange called, sourceID", sourceID);
    mRoutingReceiveInterface->hookInterruptStatusChange(sourceID, interruptState);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::hookSinkAvailablityStatusChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sinkID_t sinkID = mDBUSMessageHandler.getInt();
    am_Availability_s avialabilty = mDBUSMessageHandler.getAvailability();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookSinkAvailablityStatusChange called, sinkID", sinkID);
    mRoutingReceiveInterface->hookSinkAvailablityStatusChange(sinkID, avialabilty);
}

void IAmRoutingReceiverShadowDbus::hookSourceAvailablityStatusChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID = mDBUSMessageHandler.getInt();
    am_Availability_s avialabilty = mDBUSMessageHandler.getAvailability();
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookSourceAvailablityStatusChange called, sourceID", sourceID);
    mRoutingReceiveInterface->hookSourceAvailablityStatusChange(sourceID, avialabilty);
}

void IAmRoutingReceiverShadowDbus::hookDomainStateChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_domainID_t domainID = mDBUSMessageHandler.getUInt();
    am_DomainState_e domainState = (am_DomainState_e)((mDBUSMessageHandler.getInt()));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookDomainStateChange called, hookDomainStateChange", domainID);
    mRoutingReceiveInterface->hookDomainStateChange(domainID, domainState);
}

void IAmRoutingReceiverShadowDbus::hookTimingInformationChanged(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_connectionID_t connectionID(mDBUSMessageHandler.getInt());
    am_timeSync_t time(mDBUSMessageHandler.getInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookTimingInformationChanged called, connectionID", connectionID);
    mRoutingReceiveInterface->hookTimingInformationChanged(connectionID, time);
}

void IAmRoutingReceiverShadowDbus::sendChangedData(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    std::vector < am_EarlyData_s > listEarlyData(mDBUSMessageHandler.getEarlyData());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookTimingInformationChanged called, sendChangedData");
    mRoutingReceiveInterface->sendChangedData(listEarlyData);
}

DBusHandlerResult IAmRoutingReceiverShadowDbus::receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    assert(conn != NULL);
    assert(msg != NULL);
    assert(user_data != NULL);
    IAmRoutingReceiverShadowDbus* reference = (IAmRoutingReceiverShadowDbus*) ((user_data));
    return (reference->receiveCallbackDelegate(conn, msg));
}

void IAmRoutingReceiverShadowDbus::getRoutingReadyStatus(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(mRoutingReady);
    mDBUSMessageHandler.sendMessage();
}


void IAmRoutingReceiverShadowDbus::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
    assert(conn != NULL);
    assert(msg != NULL);
    DBusMessage* reply;
    DBusMessageIter args;
    dbus_uint32_t serial = 0;

    // create a reply from the message
    reply = dbus_message_new_method_return(msg);
    std::string fullpath(ROUTING_DBUS_INTROSPECTION_FILE);
    std::ifstream in(fullpath.c_str(), std::ifstream::in);
    if (!in)
    {
        logError("IAmCommandReceiverShadow::sendIntrospection could not load xml file ",fullpath);
        throw std::runtime_error("IAmCommandReceiverShadow::sendIntrospection Could not load introspecton XML");
    }
    std::string introspect((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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
        log(&routingDbus, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
    }
    dbus_connection_flush(conn);

    // free the reply
    dbus_message_unref(reply);
}

DBusHandlerResult IAmRoutingReceiverShadowDbus::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
    {
        sendIntrospection(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    functionMap_t::iterator iter = mFunctionMap.begin();
    std::string k(dbus_message_get_member(msg));
    log(&routingDbus, DLT_LOG_INFO, k.c_str());
    iter = mFunctionMap.find(k);
    if (iter != mFunctionMap.end())
    {
        std::string p(iter->first);
        CallBackMethod cb = iter->second;
        (this->*cb)(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    return (DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

void IAmRoutingReceiverShadowDbus::setRoutingReceiver(IAmRoutingReceive*& receiver)
{
    assert(receiver != NULL);
    mRoutingReceiveInterface = receiver;
    gObjectPathVTable.message_function = IAmRoutingReceiverShadowDbus::receiveCallback;
    DBusConnection* connection;
    mRoutingReceiveInterface->getDBusConnectionWrapper(mDBusWrapper);
    assert(mDBusWrapper != NULL);
    mDBusWrapper->getDBusConnection(connection);
    assert(connection != NULL);
    mDBUSMessageHandler.setDBusConnection(connection);
    std::string path(ROUTING_NODE);
    {
        assert(receiver != NULL);
    }
    mDBusWrapper->registerCallback(&gObjectPathVTable, path, this);
}

void IAmRoutingReceiverShadowDbus::confirmRoutingReady(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_domainID_t domainID(mDBUSMessageHandler.getUInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadowDbus::confirmRoutingReady called, domainID", domainID);

    mRoutingReceiveInterface->confirmRoutingReady(mHandle, E_OK);
    mNumberDomains++;

    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::confirmRoutingRundown(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_domainID_t domainID(mDBUSMessageHandler.getUInt());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadowDbus::confirmRoutingRundown called, domainID", domainID);

    mNumberDomains--;
    if(mNumberDomains==0)
    {
        logInfo("sending out");
        mRoutingReceiveInterface->confirmRoutingRundown(mHandle,E_OK);
    }
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::gotReady(int16_t numberDomains, uint16_t handle)
{
    mRoutingReady=true;
    mNumberDomains=numberDomains;
    mHandle=handle;
}
void IAmRoutingReceiverShadowDbus::gotRundown(int16_t numberDomains, uint16_t handle)
{
    mRoutingReady=false;
    mNumberDomains=numberDomains;
    mHandle=handle;
}

void IAmRoutingReceiverShadowDbus::updateGateway(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_gatewayID_t gatewayID(mDBUSMessageHandler.getInt());
    std::vector<am_CustomAvailabilityReason_t> listSourceConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<am_CustomAvailabilityReason_t> listSinkConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<bool> convertionMatrix(mDBUSMessageHandler.getListBool());

    am_Error_e returnCode = mRoutingReceiveInterface->updateGateway(gatewayID,listSourceConnectionFormats,listSinkConnectionFormats,convertionMatrix);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error updateGateway");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::updateConverter(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_converterID_t converterID(mDBUSMessageHandler.getInt());
    std::vector<am_CustomAvailabilityReason_t> listSourceConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<am_CustomAvailabilityReason_t> listSinkConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<bool> convertionMatrix(mDBUSMessageHandler.getListBool());

    am_Error_e returnCode = mRoutingReceiveInterface->updateConverter(converterID,listSourceConnectionFormats,listSinkConnectionFormats,convertionMatrix);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error updateGateway");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::updateSink(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sinkID_t sinkID(mDBUSMessageHandler.getInt());
    am_sinkClass_t sinkClassID(mDBUSMessageHandler.getInt());
    std::vector<am_SoundProperty_s> listSoundProperties(mDBUSMessageHandler.getListSoundProperties());
    std::vector<am_CustomAvailabilityReason_t> listSinkConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<am_MainSoundProperty_s> listMainSoundProperties(mDBUSMessageHandler.getListMainSoundProperties());

    am_Error_e returnCode = mRoutingReceiveInterface->updateSink(sinkID,sinkClassID,listSoundProperties,listSinkConnectionFormats,listMainSoundProperties);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error updateSink");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::updateSource(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID(mDBUSMessageHandler.getInt());
    am_sourceClass_t sourceClassID(mDBUSMessageHandler.getInt());
    std::vector<am_SoundProperty_s> listSoundProperties(mDBUSMessageHandler.getListSoundProperties());
    std::vector<am_CustomAvailabilityReason_t> listSinkConnectionFormats(mDBUSMessageHandler.getListconnectionFormats());
    std::vector<am_MainSoundProperty_s> listMainSoundProperties(mDBUSMessageHandler.getListMainSoundProperties());

    am_Error_e returnCode = mRoutingReceiveInterface->updateSource(sourceID,sourceClassID,listSoundProperties,listSinkConnectionFormats,listMainSoundProperties);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    if (returnCode != E_OK)
    {
        log(&routingDbus, DLT_LOG_INFO, "error updateSink");
        return;
    }
}

void IAmRoutingReceiverShadowDbus::ackSetVolumes(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    am_Error_e returnCode(am_Error_e::E_NOT_USED);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.append(returnCode);
    mDBUSMessageHandler.sendMessage();
    log(&routingDbus, DLT_LOG_INFO, "error ackSetVolumes was called - not implemented yet");
    return;
}

void IAmRoutingReceiverShadowDbus::ackSinkNotificationConfiguration(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSinkNotificationConfiguration called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_CONNECT;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSinkNotificationConfiguration(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::ackSourceNotificationConfiguration(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    uint16_t handle(mDBUSMessageHandler.getUInt());
    am_Error_e error((am_Error_e)((mDBUSMessageHandler.getUInt())));
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::ackSourceNotificationConfiguration called, handle", handle, "error", error);
    am_Handle_s myhandle;
    myhandle.handleType = H_CONNECT;
    myhandle.handle = handle;
    mRoutingReceiveInterface->ackSourceNotificationConfiguration(myhandle, error);
    mpRoutingSenderDbus->removeHandle(handle);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::hookSinkNotificationDataChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sinkID_t sinkID(mDBUSMessageHandler.getUInt());
    am_NotificationPayload_s payload(mDBUSMessageHandler.getNotificationPayload());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookSinkNotificationDataChange called, sinkID", sinkID);
    mRoutingReceiveInterface->hookSinkNotificationDataChange(sinkID, payload);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

void IAmRoutingReceiverShadowDbus::hookSourceNotificationDataChange(DBusConnection* conn, DBusMessage* msg)
{
    (void) ((conn));
    assert(mRoutingReceiveInterface != NULL);
    mDBUSMessageHandler.initReceive(msg);
    am_sourceID_t sourceID(mDBUSMessageHandler.getUInt());
    am_NotificationPayload_s payload(mDBUSMessageHandler.getNotificationPayload());
    log(&routingDbus, DLT_LOG_INFO, "IAmRoutingReceiverShadow::hookSourceNotificationDataChange called, sourceID", sourceID);
    mRoutingReceiveInterface->hookSourceNotificationDataChange(sourceID, payload);
    mDBUSMessageHandler.initReply(msg);
    mDBUSMessageHandler.sendMessage();
}

IAmRoutingReceiverShadowDbus::functionMap_t IAmRoutingReceiverShadowDbus::createMap()
{
    functionMap_t m;
    m["ackConnect"] = &IAmRoutingReceiverShadowDbus::ackConnect;
    m["ackDisconnect"] = &IAmRoutingReceiverShadowDbus::ackDisconnect;
    m["ackSetSinkVolume"] = &IAmRoutingReceiverShadowDbus::ackSetSinkVolume;
    m["ackSetSourceVolume"] = &IAmRoutingReceiverShadowDbus::ackSetSourceVolume;
    m["ackSetSourceState"] = &IAmRoutingReceiverShadowDbus::ackSetSourceState;
    m["ackSinkVolumeTick"] = &IAmRoutingReceiverShadowDbus::ackSinkVolumeTick;
    m["ackSourceVolumeTick"] = &IAmRoutingReceiverShadowDbus::ackSourceVolumeTick;
    m["ackSetSinkSoundProperty"] = &IAmRoutingReceiverShadowDbus::ackSetSinkSoundProperty;
    m["ackSetSourceSoundProperty"] = &IAmRoutingReceiverShadowDbus::ackSetSourceSoundProperty;
    m["ackSetSinkSoundProperties"] = &IAmRoutingReceiverShadowDbus::ackSetSinkSoundProperties;
    m["ackSetSourceSoundProperties"] = &IAmRoutingReceiverShadowDbus::ackSetSourceSoundProperties;
    m["ackCrossFading"] = &IAmRoutingReceiverShadowDbus::ackCrossFading;
    m["registerDomain"] = &IAmRoutingReceiverShadowDbus::registerDomain;
    m["registerSource"] = &IAmRoutingReceiverShadowDbus::registerSource;
    m["registerSink"] = &IAmRoutingReceiverShadowDbus::registerSink;
    m["registerGateway"] = &IAmRoutingReceiverShadowDbus::registerGateway;
    m["peekDomain"] = &IAmRoutingReceiverShadowDbus::peekDomain;
    m["deregisterDomain"] = &IAmRoutingReceiverShadowDbus::deregisterDomain;
    m["deregisterGateway"] = &IAmRoutingReceiverShadowDbus::deregisterGateway;
    m["peekSink"] = &IAmRoutingReceiverShadowDbus::peekSink;
    m["deregisterSink"] = &IAmRoutingReceiverShadowDbus::deregisterSink;
    m["peekSource"] = &IAmRoutingReceiverShadowDbus::peekSource;
    m["deregisterSource"] = &IAmRoutingReceiverShadowDbus::deregisterSource;
    m["registerCrossfader"] = &IAmRoutingReceiverShadowDbus::registerCrossfader;
    m["deregisterCrossfader"] = &IAmRoutingReceiverShadowDbus::deregisterCrossfader;
    m["peekSourceClassID"] = &IAmRoutingReceiverShadowDbus::peekSourceClassID;
    m["peekSinkClassID"] = &IAmRoutingReceiverShadowDbus::peekSinkClassID;
    m["hookInterruptStatusChange"] = &IAmRoutingReceiverShadowDbus::hookInterruptStatusChange;
    m["hookDomainRegistrationComplete"] = &IAmRoutingReceiverShadowDbus::hookDomainRegistrationComplete;
    m["hookSinkAvailablityStatusChange"] = &IAmRoutingReceiverShadowDbus::hookSinkAvailablityStatusChange;
    m["hookSourceAvailablityStatusChange"] = &IAmRoutingReceiverShadowDbus::hookSourceAvailablityStatusChange;
    m["hookDomainStateChange"] = &IAmRoutingReceiverShadowDbus::hookDomainStateChange;
    m["hookTimingInformationChanged"] = &IAmRoutingReceiverShadowDbus::hookTimingInformationChanged;
    m["sendChangedData"] = &IAmRoutingReceiverShadowDbus::sendChangedData;
    m["confirmRoutingReady"] =  &IAmRoutingReceiverShadowDbus::confirmRoutingReady;
    m["confirmRoutingRundown"] =  &IAmRoutingReceiverShadowDbus::confirmRoutingRundown;
    m["ackSetVolumes"] = &IAmRoutingReceiverShadowDbus::ackSetVolumes;
    m["ackSinkNotificationConfiguration"] = &IAmRoutingReceiverShadowDbus::ackSinkNotificationConfiguration;
    m["ackSourceNotificationConfiguration"] = &IAmRoutingReceiverShadowDbus::ackSourceNotificationConfiguration;
    m["hookSinkNotificationDataChange"] = &IAmRoutingReceiverShadowDbus::hookSinkNotificationDataChange;
    m["hookSourceNotificationDataChange"] = &IAmRoutingReceiverShadowDbus::hookSourceNotificationDataChange;
    m["getRoutingReadyState"] = &IAmRoutingReceiverShadowDbus::getRoutingReadyStatus;
    m["getDomainOfSource"] = &IAmRoutingReceiverShadowDbus::getDomainOfSource;
    m["getDomainOfSink"] = &IAmRoutingReceiverShadowDbus::getDomainOfSink;

    return (m);
}
}
