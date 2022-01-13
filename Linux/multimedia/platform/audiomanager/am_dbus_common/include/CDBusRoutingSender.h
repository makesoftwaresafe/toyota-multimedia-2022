/************************************************************************
 * @file: CDBusRoutingSender.h
 *
 * @version: 1.1
 *
 * @description: A CDBusRoutingSender class definition of Routing Adapter.
 * A wrapper class for sender class. CDBusRoutingSender class will call the
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

#ifndef _C_DBUS_ROUTING_SENDER_H_
#define _C_DBUS_ROUTING_SENDER_H_

#include <string>
#include <map>
#include "CAmDbusWrapper.h"
#include "CDBusSender.h"
#include "CDBusReceiver.h"
#include "CDBusCommon.h"
#include "CAmSocketHandler.h"

namespace am
{

class IDBusRoutingClient;

class CDBusRoutingSender
{
public:
    CDBusRoutingSender(IDBusRoutingClient* const client, CAmDbusWrapper*& wrapper);
    virtual ~CDBusRoutingSender();
    void setRoutingReady(DBusMessage *msg);
    void setRoutingRundown(DBusMessage *msg);
    void asyncAbort(DBusMessage *msg);
    void asyncConnect(DBusMessage *msg);
    void asyncDisconnect(DBusMessage *msg);
    void asyncSetSinkVolume(DBusMessage *msg);
    void asyncSetSourceVolume(DBusMessage *msg);
    void asyncSetSourceState(DBusMessage *msg);
    void asyncSetSinkSoundProperties(DBusMessage *msg);
    void asyncSetSinkSoundProperty(DBusMessage *msg);
    void asyncSetSourceSoundProperties(DBusMessage *msg);
    void asyncSetSourceSoundProperty(DBusMessage *msg);
    void asyncSetVolumes(DBusMessage *msg);
    void asyncSetSinkNotificationConfiguration(DBusMessage *msg);
    void asyncSetSourceNotificationConfiguration(DBusMessage *msg);
    void asyncCrossFade(DBusMessage *msg);
    void setDomainState(DBusMessage *msg);

protected:
    CAmDbusWrapper           *mpCAmDbusWrapper;
    IDBusRoutingClient       *mpIDBusRoutingClient;
    CDBusSender               mCDBusSender;
    CDBusReceiver             mCDBusReceiver;

    typedef void (CDBusRoutingSender::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t mFunctionMap;

    /**
     * This template tries to load a library and cast to a class
     * @param libname the full path to the library to be loaded
     * @param libraryHandle the handle to the library that gets returned
     * @return returns the pointer to the class to be loaded
     */

    static DBusHandlerResult receiveCallback(DBusConnection *conn, DBusMessage *msg, void *user_data);

    /**
     * dynamic delegate that handles the Callback of the static receiveCallback
     * @param conn DBus connection
     * @param msg DBus message
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
};

} /* namespace am */

#endif /* _C_DBUS_ROUTING_SENDER_H_ */
