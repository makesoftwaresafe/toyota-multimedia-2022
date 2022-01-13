/************************************************************************
 * @file: CDBusReceiver.h
 *
 * @version: 1.1
 *
 * @description: A CDBusReceiver class definition of Routing Adapter.
 * CDBusReceiver is used to initialise and send the reply to the DBus call.
 * This class will also receive the message over DBus connection.
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

#ifndef CDBUSRECEIVER_H_
#define CDBUSRECEIVER_H_

#include <dbus/dbus.h>
#include <iostream>
#include <vector>
#include <string>
#include "CDBusCommon.h"


namespace am
{

/**
 * handles DBus Messages, is used to extract & append parameters and send messages
 */
class CDBusReceiver
{
public:
    CDBusReceiver(DBusConnection* connection = NULL);
    ~CDBusReceiver();

    /**
     * sets the DBus Connection
     * @param connection pointer to the DBus Connection
     */
    void setDBusConnection(DBusConnection*& connection);

    /**
     * is called to initiate the receiving of a message
     * @param msg pointer to the message to be received
     */
    void initReceive(DBusMessage* msg);

    /**
     * the get functions return a value from the received dbus message
     * @return
     */
    dbus_uint16_t getUInt(void);
    dbus_uint16_t getUInt(DBusMessageIter& iter, bool next);
    dbus_int16_t getInt(void);
    dbus_int16_t getInt(DBusMessageIter& iter, bool next);
    dbus_bool_t getBool(void);
    dbus_bool_t getBool(DBusMessageIter& iter, bool next);
    char getByte(void);
    char getByte(DBusMessageIter& iter, bool next);
    double getDouble(void);
    double getDouble(DBusMessageIter& iter, bool next);
    char* getString(void);
    char* getString(DBusMessageIter& iter, bool next);
    void getSoundProperty(am_SoundProperty_s& soundProperty);
    void getListSoundProperties(std::vector<am_SoundProperty_s>& listSoundProperties);
    void getMainSoundProperty(am_MainSoundProperty_s& mainSoundProperty);
    void getListMainSoundProperties(std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    void getListConnFrmt(std::vector<am_CustomConnectionFormat_t>& listConnFrmt);
    void getMainConnectionType(am_MainConnectionType_s &mainConnectionType);
    void getListMainConnectionType(std::vector<am_MainConnectionType_s > &listMainConnectionType);
    void getSinkType(am_SinkType_s &sinkType);
    void getListSinkType(std::vector<am_SinkType_s > &listSinkType);
    void getSourceType(am_SourceType_s &sourceType);
    void getListSourceType(std::vector<am_SourceType_s> &listSourceType);
    void getAvailability(am_Availability_s& availability);
    void getListSourceClass(std::vector<am_SourceClass_s> &listSourceClasses);
    void getListSinkClass(std::vector<am_SinkClass_s> &listSinkClasses);
    void getSystemProperty(am_SystemProperty_s &systemProperty);
    void getListSystemProperty(std::vector<am_SystemProperty_s> &listSystemProperty);
    void getDomainData(am_Domain_s& domainData);
    void getSourceData(am_Source_s& sourceData);
    void getSinkData(am_Sink_s& sinkData);
    void getGatewayData(am_Gateway_s& gatewayData);
    void getEarlyData(std::vector<am_EarlyData_s>& listEarlyData);
    void getListVolumes(std::vector<am_Volumes_s>& listVolumes);
    void getNotificationConfiguration(am_NotificationConfiguration_s& notificationConfiguration);
    void getListNotificationConfiguration(std::vector<am_NotificationConfiguration_s> &listNotificationConfiguration);
    void getNotificationPayload(am_NotificationPayload_s& payload);
    void getConvertionMatrix(std::vector<bool>& convertionMatrix);

private:
    DBusMessageIter mDBusMessageIter;
    DBusError mDBusError;
    std::string mErrorName;
    std::string mErrorMsg;
    DBusMessage* mpDBusMessage;
    DBusConnection* mpDBusConnection;
};

} /* namespace am*/

#endif /* CDBUSRECEIVER_H_ */
