/************************************************************************
 * @file: ICpDbusWrpReceiverShadow.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow definition of command plug-in.
 * IAmDbusCommandReceiverShadow class will be running with the AM process.
 * IAmDbusCommandReceiverShadow class methods will be called via DBus to
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

#ifndef IAMDBUSCOMMANDRECEIVERSHADOW_H_
#define IAMDBUSCOMMANDRECEIVERSHADOW_H_

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <map>
#include "CAmDbusWrapper.h"
#include "IAmCommand.h"
#include "CDBusSender.h"
#include "CDBusReceiver.h"

namespace am
{
/**
 * receives the DBus Callbacks, marhsalls and demarshalls the parameters and calls CommandReceive
 */
class IAmDbusCommandReceiverShadow
{
public:
    IAmDbusCommandReceiverShadow();
    virtual ~IAmDbusCommandReceiverShadow();
    void getInterfaceVersion(DBusMessage *msg);
    void connect(DBusMessage *msg);
    void disconnect(DBusMessage *msg);
    void setVolume(DBusMessage *msg);
    void volumeStep(DBusMessage *msg);
    void setSinkMuteState(DBusMessage *msg);
    void setMainSinkSoundProperty(DBusMessage *msg);
    void setMainSourceSoundProperty(DBusMessage *msg);
    void setSystemProperty(DBusMessage *msg);
    void getListMainConnections(DBusMessage *msg);
    void getListMainSinks(DBusMessage *msg);
    void getListMainSources(DBusMessage *msg);
    void getListMainSinkSoundProperties(DBusMessage *msg);
    void getListMainSourceSoundProperties(DBusMessage *msg);
    void getListSourceClasses(DBusMessage *msg);
    void getListSinkClasses(DBusMessage *msg);
    void getListSystemProperties(DBusMessage *msg);
    void getTimingInformation(DBusMessage *msg);
    void getDBusConnectionWrapper(DBusMessage *msg);
    void getSocketHandler(DBusMessage *msg);
    void confirmCommandReady(DBusMessage *msg);
    void confirmCommandRundown(DBusMessage *msg);
    void getListMainSinkNotificationConfigurations(DBusMessage *msg);
    void getListMainSourceNotificationConfigurations(DBusMessage *msg);
    void setMainSinkNotificationConfiguration(DBusMessage *msg);
    void setMainSourceNotificationConfiguration(DBusMessage *msg);
    void getVolume(DBusMessage *msg);

    void setCommandReceiver(IAmCommandReceive*& receiver);
private:
    IAmCommandReceive* mpIAmCommandReceive;
    CAmDbusWrapper* mpCAmDBusWrapper;
    typedef void (IAmDbusCommandReceiverShadow::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t mFunctionMap;
    CDBusSender mCDBusSender;
    CDBusReceiver mCDBusReceiver;

    /**
     * receives a callback whenever the path of the plugin is called
     */
    static DBusHandlerResult receiveCallback(DBusConnection *conn, DBusMessage *msg, void *user_data);

    /**
     * dynamic delegate that handles the Callback of the static receiveCallback
     * @param conn DBus connection
     * @param msg DBus message
     * @param user_data pointer to instance of IAmDbusCommandReceiverShadow
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

#endif /* IAMDBUSCOMMANDRECEIVERSHADOW_H_ */
