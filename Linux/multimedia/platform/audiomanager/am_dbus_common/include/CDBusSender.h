/************************************************************************
 * @file: CDBusSender.h
 *
 * @version: 1.1
 *
 * @description: A CDBusSender class definition of Routing Adapter.
 * CDBusSender is used to send the data over DBus connection.
 * This class also used to append the data to DBus message.
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

#ifndef CDBUSSENDER_H_
#define CDBUSSENDER_H_

#include <dbus/dbus.h>
#include <iostream>
#include <vector>
#include <string>
#include "CDBusCommon.h"

namespace am
{

class CDBusSender
{
public:
    CDBusSender(DBusConnection* conn = NULL);
    CDBusSender(DBusConnection* conn, std::string method, const std::string& bus_name, const std::string& path, const std::string& interface);
    virtual ~CDBusSender();
    // set's the DBus connection pointer
    void setDBusConnection(DBusConnection* conn);
    // Construct the DBus method call
    void call(std::string method, const dbus_comm_s &dbus);
    // Returns DBus message pointer
    DBusMessage* getDbusMessage(void);

    void append(const std::string &str);
    void append(const char &toAppend);
    void append(const dbus_int16_t &toAppend);
    void append(const dbus_uint16_t &toAppend);
    void append(const am_Error_e &error);
    void append(const bool &toAppend);
    void append(const double &toAppend);
    void append(const am_SoundProperty_s &soundProperty);
    void append(const std::vector<am_SoundProperty_s> &listSoundProperties);
    void append(const am_MainSoundProperty_s &mainSoundProperty);
    void append(const std::vector<am_MainSoundProperty_s> &listMainSoundProperties);
    void append(const am_SystemProperty_s &systemProperty);
    void append(const std::vector<am_SystemProperty_s> &listSystemProperties);
    void append(const am_SourceType_s &sourceType);
    void append(const am_SinkType_s &sinkType);
    void append(const std::vector<am_SourceType_s> &listMainSources);
    void append(const std::vector<am_SinkType_s> &listMainSinks);
    void append(const std::vector<am_SourceClass_s> &listSourceClasses);
    void append(const std::vector<am_SinkClass_s> &listSinkClasses);
    void append(const std::vector<am_MainConnectionType_s> &listMainConnections);
    void append(const am_MainConnectionType_s &MainConnections);
    void append(const am_Availability_s &availability);
    void append(const am_Domain_s &domainData);
    void append(const am_Source_s &sourceData);
    void append(const am_Sink_s &sinkData);
    void append(const am_Gateway_s &gatewayData);
    void append(const am_Crossfader_s &crossfaderData);
    void append(const am_Converter_s& converterData);
    void append(const std::vector<am_EarlyData_s> &earlyData);
    void append(const std::vector<am_Volumes_s> &listVolume);
    void append(const am_NotificationConfiguration_s &notifConfig);
    void append(const std::vector<am_NotificationConfiguration_s> &listnotifConfig);
    void append(const std::vector<am_CustomConnectionFormat_t>& listConnFrmt);
    void append(const std::vector<bool>& convertionMatrix);
    void append(const am_NotificationPayload_s& payload);
    void setPathPrefix(std::string pathPrefix, std::string servicePrefix);
    /**
     * Initialise a reply call to the received call via DBus
     * @param msg:  DBus message received via DBus
     */

    void initReply(DBusMessage* msg);
    /**
     * Initialise a signal to be sent via dbus
     * parameters can be added before sending the signal
     * @param signalName: the signal name
     */
    void initSignal(const std::string& path, const std::string& signalName);
    void initSignal(const std::string& path, const std::string& signalName, const std::string& interface);
    // To make synchronous(blocking) method call. Returns error code after getting the Reply.
    am_Error_e send(void);
    // To make synchronous(blocking) method call. Received DBus message need to process, to get the reply information.
    am_Error_e send_sync(void);
    // To make asynchronous(non-blocking) call.
    am_Error_e send_async(void);
    // Send's the reply back to DBus blocking call.
    void sendMessage();

private:
    DBusMessage* mpDBusMessage;
    DBusMessage* mpDBusReceivedMessage;
    DBusConnection* mpDBusConnection;
    dbus_uint32_t mSerial;
    DBusMessageIter mDBusMessageIter;
    DBusError mDBusError;
    std::string mErrorName;
    std::string mErrorMsg;
    std::string mPathPrefix;
    std::string mServicePrefix;
};

} /* namespace am */

#endif /* CDBUSSENDER_H_ */
