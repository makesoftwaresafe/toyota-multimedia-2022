/**
 *  Copyright (c) copyright 2011-2012 AricentÂ® Group  and its licensors
 *  Copyright (c) 2012 BMW
 *
 *  \author Sampreeth Ramavana
 *  \author Christian Linke, christian.linke@bmw.de BMW 2011,2012
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

#ifndef _CAMSDBUSMESSAGEHANDLER_H_
#define _CAMSDBUSMESSAGEHANDLER_H_


#include <dbus/dbus.h>
#include <vector>
#include <sstream>
#include <string>
#include <list>
#include "audiomanagertypes.h"

namespace am {


/**
 * handles DBus Messages, is used to extract & append parameters and send messages
 */
class CAmRoutingDbusMessageHandler
{
public:
    CAmRoutingDbusMessageHandler();
    ~CAmRoutingDbusMessageHandler();

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
     *  is called to initiate the reply to a message
     * @param msg pointer to the message the reply is for
     */
    void initReply(DBusMessage* msg);

    /**
     * inits a signal to be sent via dbus
     * parameters can be added before sending the signal
     * @param path the path
     * @param signalName the signal name
     */
    void initSignal(std::string path, std::string signalName);

    /**
     * sends out the message
     */
    void sendMessage();

    /**
     * the get functions return a value from the received dbus message
     * @return
     */
    dbus_uint16_t getUInt();
    dbus_uint16_t getUInt(DBusMessageIter& iter, bool next);
    dbus_int16_t getInt();
    dbus_int16_t getInt(DBusMessageIter& iter, bool next);
    dbus_int32_t getInt32();
    dbus_int32_t getInt32(DBusMessageIter& iter, bool next);
    dbus_bool_t getBool();
    dbus_bool_t getBool(DBusMessageIter& iter, bool next);
    char getByte();
    char getByte(DBusMessageIter& iter, bool next);
    double getDouble();
    double getDouble(DBusMessageIter& iter, bool next);
    char* getString();
    char* getString(DBusMessageIter& iter, bool next);
    am_Availability_s getAvailability();
    std::vector<am::am_EarlyData_s> getEarlyData();
    am_Domain_s getDomainData();
    am_Source_s getSourceData();
    am_Sink_s getSinkData();
    am_Gateway_s getGatewayData();
    am_Converter_s getConverterData();
    am_Crossfader_s getCrossfaderData();
    am_SoundProperty_s getSoundProperty();
    am_MainSoundProperty_s getMainSoundProperty();
    std::vector<am_CustomAvailabilityReason_t> getListconnectionFormats();
    std::vector<bool> getListBool();
    std::vector<am_SoundProperty_s> getListSoundProperties();
    std::vector<am_MainSoundProperty_s> getListMainSoundProperties();
    am_NotificationPayload_s getNotificationPayload();



    /**
     * the overloaded append function appends different datatypes to the dbusmessage
     */
    void append(dbus_int16_t toAppend);
    void append(dbus_uint16_t toAppend);
    void append(dbus_uint32_t toAppend);
    void append(char toAppend);
    void append(bool toAppend);
    void append(double toAppend);
    void append(const am::am_Error_e error);
    void append(const am::am_SinkType_s& sinkType);
    void append(const am::am_SourceType_s& sourceType);
    void append(const am::am_MainSoundProperty_s mainSoundProperty);
    void append(const am::am_Availability_s & availability);
    void append(const am::am_SystemProperty_s & SystemProperty);
    void append(const std::vector<am::am_MainConnectionType_s>& listMainConnections);
    void append(const std::vector<am::am_SinkType_s>& listMainSinks);
    void append(const std::vector<am::am_SourceType_s>& listMainSources);
    void append(const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties);
    void append(const std::vector<am::am_SourceClass_s>& listSourceClasses);
    void append(const std::vector<am::am_SinkClass_s>& listSinkClasses);
    void append(const std::vector<am::am_SystemProperty_s>& listSystemProperties);

private:

    DBusMessageIter mDBusMessageIter;
    DBusError mDBusError;
    dbus_uint32_t mSerial;
    std::string mErrorName;
    std::string mErrorMsg;
    DBusMessage* mpDBusMessage;
    DBusMessage* mpReveiveMessage;
    DBusConnection* mpDBusConnection;
};

}

#endif // _CAMSDBUSMESSAGEHANDLER_H_
