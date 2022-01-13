/************************************************************************
 * @file: IRaDbusWrpReceiverShadow.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow definition of Routing Adapter.
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

#ifndef IAMDBUSROUTINGRECEIVERSHADOW_H_
#define IAMDBUSROUTINGRECEIVERSHADOW_H_

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <map>
#include "CAmDbusWrapper.h"
#include "IAmRouting.h"
#include "CDBusReceiver.h"
#include "CDBusSender.h"

namespace am
{
class CAmDbusRoutingSender;
/**
 * receives the DBus Callbacks, marhsalls and demarshalls the parameters and calls CommandReceive
 */
class IAmDbusRoutingReceiverShadow
{
public:
    IAmDbusRoutingReceiverShadow(CAmDbusRoutingSender* mpCAmDbusRoutingSender);
    virtual ~IAmDbusRoutingReceiverShadow();
    void ackConnect(DBusMessage *msg);
    void ackDisconnect(DBusMessage *msg);
    void ackSetSinkVolumeChange(DBusMessage *msg);
    void ackSetSourceVolumeChange(DBusMessage *msg);
    void ackSetSourceState(DBusMessage *msg);
    void ackSinkVolumeTick(DBusMessage *msg);
    void ackSourceVolumeTick(DBusMessage *msg);
    void ackSetSinkSoundProperty(DBusMessage *msg);
    void ackSetSourceSoundProperty(DBusMessage *msg);
    void ackSetSinkSoundProperties(DBusMessage *msg);
    void ackSetSourceSoundProperties(DBusMessage *msg);
    void ackCrossFading(DBusMessage *msg);
    void registerDomain(DBusMessage *msg);
    void registerSource(DBusMessage *msg);
    void registerSink(DBusMessage *msg);
    void registerGateway(DBusMessage *msg);
    void peekDomain(DBusMessage *msg);
    void deregisterDomain(DBusMessage *msg);
    void deregisterGateway(DBusMessage *msg);
    void peekSink(DBusMessage *msg);
    void deregisterSink(DBusMessage *msg);
    void peekSource(DBusMessage *msg);
    void deregisterSource(DBusMessage *msg);
    void registerCrossfader(DBusMessage *msg);
    void deregisterCrossfader(DBusMessage *msg);
    void peekSourceClassID(DBusMessage *msg);
    void peekSinkClassID(DBusMessage *msg);
    void hookInterruptStatusChange(DBusMessage *msg);
    void hookDomainRegistrationComplete(DBusMessage *msg);
    void hookSinkAvailablityStatusChange(DBusMessage *msg);
    void hookSourceAvailablityStatusChange(DBusMessage *msg);
    void hookDomainStateChange(DBusMessage *msg);
    void hookTimingInformationChanged(DBusMessage *msg);
    void sendChangedData(DBusMessage *msg);
    void confirmRoutingReady(DBusMessage *msg);
    void confirmRoutingRundown(DBusMessage *msg);
    void updateGateway(DBusMessage *msg);
    void updateSink(DBusMessage *msg);
    void updateSource(DBusMessage *msg);
    void ackSetVolumes(DBusMessage *msg);
    void ackSinkNotificationConfiguration(DBusMessage *msg);
    void ackSourceNotificationConfiguration(DBusMessage *msg);
    void hookSinkNotificationDataChange(DBusMessage *msg);
    void hookSourceNotificationDataChange(DBusMessage *msg);
    void getInterfaceVersion(DBusMessage *msg);
    void getRoutingReady(DBusMessage *msg);


    /**
     * sets the pointer to the CommandReceiveInterface and registers Callback
     * @param receiver
     */
    void setRoutingReceiver(IAmRoutingReceive*& receiver);

private:
    IAmRoutingReceive* mpIAmRoutingReceive;
    CAmDbusWrapper* mpCAmDBusWrapper;
    typedef void (IAmDbusRoutingReceiverShadow::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t mFunctionMap;
    CDBusReceiver mCDBusReceiver;
    CDBusSender mCDBusSender;
    CAmDbusRoutingSender* mpCAmDbusRoutingSender;
    /**
     * receives a callback whenever the path of the plugin is called
     */
    static DBusHandlerResult receiveCallback(DBusConnection *conn, DBusMessage *msg, void *user_data);

    /**
     * dynamic delegate that handles the Callback of the static receiveCallback
     * @param conn DBus connection
     * @param msg DBus message
     * @param user_data pointer to instance of IAmDbusRoutingReceiverShadow
     * @return
     */
    DBusHandlerResult receiveCallbackDelegate(DBusConnection *conn, DBusMessage *msg);

    /**
     * sends out introspectiondata read from an xml file.
     * @param conn
     * @param msg
     */
    void sendIntrospection(DBusConnection* conn, DBusMessage* msg);

    /**
     * creates the function map needed to combine DBus messages and function adresses
     * @return the map
     */
    functionMap_t createMap();
};

} /* namespace am*/

#endif /* IAMDBUSROUTINGRECEIVERSHADOW_H_ */
