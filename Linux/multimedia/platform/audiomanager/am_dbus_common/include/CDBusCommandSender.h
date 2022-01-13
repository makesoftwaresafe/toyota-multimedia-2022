/************************************************************************
 * @file: CDBusCommandSender.h
 *
 * @version: 1.1
 *
 * @description: A CDBusCommandSender class definition of command plug-in.
 * A wrapper class for sender class. CDBusCommandSender class will call the
 * sender class API which has the actual sender API definition.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *          Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 * @copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#ifndef _C_DBUS_COMMAND_SENDER_H_
#define _C_DBUS_COMMAND_SENDER_H_

#include <map>
#include <string>
#include "IAmCommand.h"
#include "CAmDbusWrapper.h"
#include "IDBusCommandReceiver.h"
#include "CDBusSender.h"
#include "CDBusReceiver.h"

namespace am
{

class IDBusCommandClient;

class CDBusCommandSender
{
public:
    CDBusCommandSender(IDBusCommandClient* const client, CAmDbusWrapper*& wrapper);
    virtual ~CDBusCommandSender();

    void setCommandReady(DBusMessage *msg);
    void setCommandRundown(DBusMessage *msg);
    void cbNewMainConnection(DBusMessage *msg);
    void cbRemovedMainConnection(DBusMessage *msg);
    void cbNewSink(DBusMessage *msg);
    void cbRemovedSink(DBusMessage *msg);
    void cbNewSource(DBusMessage *msg);
    void cbRemovedSource(DBusMessage *msg);
    void cbNumberOfSinkClassesChanged(DBusMessage *msg);
    void cbNumberOfSourceClassesChanged(DBusMessage *msg);
    void cbMainConnectionStateChanged(DBusMessage *msg);
    void cbMainSinkSoundPropertyChanged(DBusMessage *msg);
    void cbMainSourceSoundPropertyChanged(DBusMessage *msg);
    void cbSinkAvailabilityChanged(DBusMessage *msg);
    void cbSourceAvailabilityChanged(DBusMessage *msg);
    void cbVolumeChanged(DBusMessage *msg);
    void cbSinkMuteStateChanged(DBusMessage *msg);
    void cbSystemPropertyChanged(DBusMessage *msg);
    void cbTimingInformationChanged(DBusMessage *msg);
    void cbSinkUpdated(DBusMessage *msg);
    void cbSourceUpdated(DBusMessage *msg);
    void cbSinkNotification(DBusMessage *msg);
    void cbSourceNotification(DBusMessage *msg);
    void cbMainSinkNotificationConfigurationChanged(DBusMessage *msg);
    void cbMainSourceNotificationConfigurationChanged(DBusMessage *msg);

protected:
    CAmDbusWrapper           *mpCAmDbusWrapper;
    IDBusCommandClient       *mpIDBusCommandClient;
    CDBusReceiver             mCDBusReceiver;

    typedef void (CDBusCommandSender::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t mFunctionMap;

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
    void sendIntrospection(DBusConnection *conn, DBusMessage* msg);

    /**
     * creates the function map needed to combine DBus messages and function adresses
     * @return the map
     */
    functionMap_t createMap();

    /**
     * To receive the message from daemon continuously
     * @return the
     * E_OK on success
     * E_NOT_USED on error in connection
     */

};

} /* namespace am */

#endif /* _C_DBUS_COMMAND_SENDER_H_ */
