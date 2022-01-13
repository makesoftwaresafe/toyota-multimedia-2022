/************************************************************************
 * @file: CDBusSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CDBusSender class implementation of Routing Adapter.
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

#include "CDBusSender.h"
#include "CAmDltWrapper.h"

using namespace std;

namespace am
{

CDBusSender::CDBusSender(DBusConnection* conn) :
        mpDBusMessage(NULL), mpDBusReceivedMessage(NULL), mpDBusConnection(conn), mSerial(), mDBusMessageIter(), mDBusError()
{
    dbus_error_init(&mDBusError);
    mPathPrefix = string(DBUS_SERVICE_OBJECT_PATH);
    mServicePrefix = string(DBUS_SERVICE_PREFIX);
}

CDBusSender::CDBusSender(DBusConnection* conn, string method, const string& bus_name, const string& path,
                         const string& interface) :
        mpDBusMessage(NULL), mpDBusReceivedMessage(NULL), mpDBusConnection(conn), mSerial(), mDBusMessageIter(), mDBusError()
{
    dbus_error_init(&mDBusError);
    mpDBusMessage = dbus_message_new_method_call(bus_name.c_str(), path.c_str(), interface.c_str(), method.c_str());
    if (NULL == mpDBusMessage)
    {
        logError("CDBusSender::call memory can't be allocated for the message");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
    dbus_message_iter_init_append(mpDBusMessage, &mDBusMessageIter);
}

CDBusSender::~CDBusSender()
{
}

void CDBusSender::setDBusConnection(DBusConnection* conn)
{
    mpDBusConnection = conn;
}

void CDBusSender::call(string method, const dbus_comm_s &dbus)
{
    mpDBusMessage = dbus_message_new_method_call(dbus.dest.c_str(), dbus.path.c_str(), dbus.iface.c_str(), method.c_str());
    if (NULL == mpDBusMessage)
    {
        logError("CDBusSender::call memory can't be allocated for the message");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
    dbus_message_iter_init_append(mpDBusMessage, &mDBusMessageIter);
}

DBusMessage* CDBusSender::getDbusMessage(void)
{
    return mpDBusMessage;
}

void CDBusSender::append(const string &str)
{
    const char* ptrchar = str.c_str();
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &ptrchar))
    {
        logError("CDBusSender::append(string) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const char &toAppend)
{
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BYTE, &toAppend))
    {
        logError("CDBusSender::append(char) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const dbus_int16_t &toAppend)
{
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_INT16, &toAppend))
    {
        logError("CDBusSender::append(int16) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const dbus_uint16_t &toAppend)
{
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &toAppend))
    {
        logError("CDBusSender::append(uint16) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Error_e &error)
{
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &error))
    {
        logError("CDBusSender::append(am_Error_e) Cannot allocate DBus message!");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const bool &toAppend)
{
    dbus_bool_t dbusBoolValue = convertToDBusType(toAppend);
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BOOLEAN, &dbusBoolValue))
    {
        logError("CDBusSender::append(bool) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const double &toAppend)
{
    if (!dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_DOUBLE, &toAppend))
    {
        logError("CDBusSender::append(double) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_SoundProperty_s &soundProperty)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &soundProperty.type);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &soundProperty.value);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_SoundProperty_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SoundProperty_s> &listSoundProperties)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter outerStructIter;
    int size = listSoundProperties.size();
    vector<am_SoundProperty_s>::const_iterator listIterator = listSoundProperties.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);
    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nn)", &arrayIter);
    for (; success && (listIterator < listSoundProperties.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->type);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->value);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_SoundProperty_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_MainSoundProperty_s &mainSoundProperty)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &mainSoundProperty.type);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &mainSoundProperty.value);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_MainSoundProperty_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_MainSoundProperty_s> &listMainSoundProperties)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter,outerStructIter;
    int size = listMainSoundProperties.size();
    vector<am_MainSoundProperty_s>::const_iterator listIterator = listMainSoundProperties.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nn)", &arrayIter);
    for (; success && (listIterator < listMainSoundProperties.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->type);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->value);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_MainSoundProperty_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_SystemProperty_s &SystemProperty)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &SystemProperty.type);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &SystemProperty.value);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_SystemProperty_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SystemProperty_s> &listSystemProperties)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter outerStructIter;
    int size = listSystemProperties.size();

    vector<am_SystemProperty_s>::const_iterator listIterator = listSystemProperties.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nn)", &arrayIter);
    for (; success && (listIterator < listSystemProperties.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->type);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->value);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_SystemProperty_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_SourceType_s &sourceType)
{
    DBusMessageIter structIter;
    DBusMessageIter structAvailIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &sourceType.sourceID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &sourceType.name);
    success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_STRUCT, NULL, &structAvailIter);
    success = success
            && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16, &sourceType.availability.availability);
    success = success
            && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16,
                                              &sourceType.availability.availabilityReason);
    success = success && dbus_message_iter_close_container(&structIter, &structAvailIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &sourceType.sourceClassID);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_SourceType_s) error: ", mDBusError.message);
    }
}

void CDBusSender::append(const am_SinkType_s &sinkType)
{
    DBusMessageIter structIter;
    DBusMessageIter structAvailIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &sinkType.sinkID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &sinkType.name);
    success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_STRUCT, NULL, &structAvailIter);
    success = success
            && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16, &sinkType.availability.availability);
    success = success
            && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16, &sinkType.availability.availabilityReason);
    success = success && dbus_message_iter_close_container(&structIter, &structAvailIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &sinkType.volume);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &sinkType.muteState);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &sinkType.sinkClassID);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_SinkType_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SourceType_s> &listMainSources)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter availIter;
    DBusMessageIter outerStructIter;
    int size = listMainSources.size();
    vector<am_SourceType_s>::const_iterator listIterator = listMainSources.begin();
    dbus_bool_t success = true;
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(qs(nn)q)", &arrayIter);
    for (; success && (listIterator < listMainSources.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sourceID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &listIterator->name);
        success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_STRUCT, NULL, &availIter);
        success = success
                && dbus_message_iter_append_basic(&availIter, DBUS_TYPE_INT16, &listIterator->availability.availability);
        success = success
                && dbus_message_iter_append_basic(&availIter, DBUS_TYPE_INT16,
                                                  &listIterator->availability.availabilityReason);
        success = success && dbus_message_iter_close_container(&structIter, &availIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sourceClassID);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_SourceType_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SinkType_s> &listMainSinks)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter availIter;
    DBusMessageIter outerStructIter;
    int size = listMainSinks.size();
    vector<am_SinkType_s>::const_iterator listIterator = listMainSinks.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(qs(nn)nnq)", &arrayIter);

    for (; success && (listIterator < listMainSinks.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sinkID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &listIterator->name);
        success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_STRUCT, NULL, &availIter);
        success = success
                && dbus_message_iter_append_basic(&availIter, DBUS_TYPE_INT16, &listIterator->availability.availability);
        success = success
                && dbus_message_iter_append_basic(&availIter, DBUS_TYPE_INT16,
                                                  &listIterator->availability.availabilityReason);
        success = success && dbus_message_iter_close_container(&structIter, &availIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->volume);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->muteState);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sinkClassID);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);

    if (!success)
    {
        logError("CDBusSender::append(vector<am_SinkType_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SourceClass_s> &listSourceClasses)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter propIter;
    DBusMessageIter innerIter;
    DBusMessageIter outerStructIter;
    int size = listSourceClasses.size();
    vector<am_SourceClass_s>::const_iterator listIterator = listSourceClasses.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(qsna(nn))", &arrayIter);
    for (; success && (listIterator < listSourceClasses.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sourceClassID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &listIterator->name);
        size = listIterator->listClassProperties.size();
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &size);
        success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_ARRAY, "(nn)", &innerIter);

        vector<am_ClassProperty_s>::const_iterator listInnerIterator = listIterator->listClassProperties.begin();
        for (; success && (listInnerIterator < listIterator->listClassProperties.end()); ++listInnerIterator)
        {
            success = success && dbus_message_iter_open_container(&innerIter, DBUS_TYPE_STRUCT, NULL, &propIter);
            success = success
                    && dbus_message_iter_append_basic(&propIter, DBUS_TYPE_INT16, &listInnerIterator->classProperty);
            success = success && dbus_message_iter_append_basic(&propIter, DBUS_TYPE_INT16, &listInnerIterator->value);
            success = success && dbus_message_iter_close_container(&innerIter, &propIter);
        }
        success = success && dbus_message_iter_close_container(&structIter, &innerIter);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }

    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_SourceClass_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_SinkClass_s> &listSinkClasses)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter propIter;
    DBusMessageIter innerIter;
    DBusMessageIter outerStructIter;
    int size = listSinkClasses.size();
    vector<am_SinkClass_s>::const_iterator listIterator = listSinkClasses.begin();
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(qsna(nn))", &arrayIter);
    for (; success && (listIterator < listSinkClasses.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sinkClassID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &listIterator->name);
        size = listIterator->listClassProperties.size();
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &size);
        success = success && dbus_message_iter_open_container(&structIter, DBUS_TYPE_ARRAY, "(nn)", &innerIter);

        vector<am_ClassProperty_s>::const_iterator listInnerIterator = listIterator->listClassProperties.begin();
        for (; success && (listInnerIterator < listIterator->listClassProperties.end()); ++listInnerIterator)
        {
            success = success && dbus_message_iter_open_container(&innerIter, DBUS_TYPE_STRUCT, NULL, &propIter);
            success = success
                    && dbus_message_iter_append_basic(&propIter, DBUS_TYPE_INT16, &listInnerIterator->classProperty);
            success = success && dbus_message_iter_append_basic(&propIter, DBUS_TYPE_INT16, &listInnerIterator->value);
            success = success && dbus_message_iter_close_container(&innerIter, &propIter);
        }
        success = success && dbus_message_iter_close_container(&structIter, &innerIter);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_SinkClass_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }

}

void CDBusSender::append(const vector<am_MainConnectionType_s> &listMainConnections)
{
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    DBusMessageIter outerStructIter;
    int size = listMainConnections.size();

    vector<am_MainConnectionType_s>::const_iterator listIterator = listMainConnections.begin();
    dbus_bool_t success = true;
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);
    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(qqqnn)", &arrayIter);
    for (; success && (listIterator < listMainConnections.end()); ++listIterator)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->mainConnectionID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sourceID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &listIterator->sinkID);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->delay);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listIterator->connectionState);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_MainConnectionType_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_MainConnectionType_s &MainConnections)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &MainConnections.mainConnectionID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &MainConnections.sourceID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &MainConnections.sinkID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &MainConnections.delay);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &MainConnections.connectionState);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter,&structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_MainConnectionType_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Availability_s &availability)
{
    DBusMessageIter structAvailIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structAvailIter);
    success = success && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16, &availability.availability);
    success = success && dbus_message_iter_append_basic(&structAvailIter, DBUS_TYPE_INT16, &availability.availabilityReason);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structAvailIter);
    if (!success)
    {
        logError("CDBusSender::append(am_Availability_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Domain_s &domainData)
{
    dbus_bool_t success = true;
    dbus_bool_t dbusBoolValue;

    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &domainData.domainID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &domainData.name);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &domainData.busname);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &domainData.nodename);
    dbusBoolValue = convertToDBusType(domainData.early);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BOOLEAN, &dbusBoolValue);
    dbusBoolValue = convertToDBusType(domainData.complete);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BOOLEAN, &dbusBoolValue);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &domainData.state);
    if (!success)
    {
        logError("CDBusSender::append(am_Domain_s) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Source_s &sourceData)
{
    dbus_bool_t success = true;
    dbus_bool_t sourceVisible;
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sourceData.sourceID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sourceData.domainID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &sourceData.name);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sourceData.sourceClassID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sourceData.sourceState);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_INT16, &sourceData.volume);
    sourceVisible = convertToDBusType(sourceData.visible);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BOOLEAN, &sourceVisible);
    // Availability
    append(sourceData.available);
    // InterruptState
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sourceData.interruptState);
    //sound property
    append(sourceData.listSoundProperties);
    // connection formats
    append(sourceData.listConnectionFormats);
    //listMainSoundProperties
    append(sourceData.listMainSoundProperties);
    //listMainNotificationConfigurations
    append(sourceData.listMainNotificationConfigurations);
    //listNotificationConfigurations
    append(sourceData.listNotificationConfigurations);
    if (!success)
    {
        logError("CDBusSender::append(am_Source_s) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Sink_s &sinkData)
{
    dbus_bool_t success = true;
    dbus_bool_t sinkVisible = FALSE;
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sinkData.sinkID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &sinkData.name);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sinkData.domainID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &sinkData.sinkClassID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_INT16, &sinkData.volume);
    sinkVisible = convertToDBusType(sinkData.visible);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_BOOLEAN, &sinkVisible);
    // Availability
    append(sinkData.available);
    // mute state
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_INT16, &sinkData.muteState);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_INT16, &sinkData.mainVolume);
    //sound property
    append(sinkData.listSoundProperties);
    // connection formats
    append(sinkData.listConnectionFormats);
    //listMainSoundProperties
    append(sinkData.listMainSoundProperties);
    //listMainNotificationConfigurations
    append(sinkData.listMainNotificationConfigurations);
    //listNotificationConfigurations
    append(sinkData.listNotificationConfigurations);
    if (!success)
    {
        logError("CDBusSender::append(am_Sink_s)  Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Gateway_s &gatewayData)
{
    dbus_bool_t success = true;

    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.gatewayID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &gatewayData.name);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.sinkID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.sourceID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.domainSinkID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.domainSourceID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &gatewayData.controlDomainID);
    // list of source connection format
    append(gatewayData.listSourceFormats);
    // List of sink connection format
    append(gatewayData.listSinkFormats);
    //List of conversion matrix
    append(gatewayData.convertionMatrix);
    if (!success)
    {
        logError("CDBusSender::append(am_Gateway_s) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Crossfader_s &crossfaderData)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &crossfaderData.crossfaderID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &crossfaderData.name);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &crossfaderData.sinkID_A);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &crossfaderData.sinkID_B);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_UINT16, &crossfaderData.sourceID);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &crossfaderData.hotSink);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_Crossfader_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_Converter_s& converterData)
{
    dbus_bool_t success = true;

    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &converterData.converterID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_STRING, &converterData.name);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &converterData.sinkID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &converterData.sourceID);
    success = success && dbus_message_iter_append_basic(&mDBusMessageIter, DBUS_TYPE_UINT16, &converterData.domainID);
    // list of source connection format
    append(converterData.listSourceFormats);
    // List of sink connection format
    append(converterData.listSinkFormats);
    //List of conversion matrix
    append(converterData.convertionMatrix);
    if (!success)
    {
        logError("CDBusSender::append(am_Converter_s) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}


void CDBusSender::append(const vector<am_EarlyData_s> &earlyData)
{
    dbus_bool_t success = true;

    DBusMessageIter volArrayIter;
    DBusMessageIter volStructIter;

    DBusMessageIter sndPtyArrayIter;
    DBusMessageIter sndPtyStructIter;
    DBusMessageIter innerStructIter;
    vector<am_EarlyData_s>::const_iterator earlyDataItr = earlyData.begin();
    DBusMessageIter outerStructIter;
    int size=0;
    int soundsize=0;
    //first the volume array
    /*
     * Get the size of volume array
     */
    for(;earlyDataItr < earlyData.end();++earlyDataItr)
    {
        if (earlyDataItr->type == ED_SINK_VOLUME || earlyDataItr->type == ED_SOURCE_VOLUME)
        {
            size++;
        }
        if (earlyDataItr->type == ED_SINK_PROPERTY || earlyDataItr->type == ED_SOURCE_PROPERTY)
        {
            soundsize++;
        }
    }
    earlyDataItr = earlyData.begin();
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nqq)", &volArrayIter);
    for (; success && (earlyDataItr < earlyData.end()); ++earlyDataItr)
    {
        if (earlyDataItr->type == ED_SINK_VOLUME || earlyDataItr->type == ED_SOURCE_VOLUME)
        {
            success = success && dbus_message_iter_open_container(&volArrayIter, DBUS_TYPE_STRUCT, NULL, &volStructIter);
            success = success && dbus_message_iter_append_basic(&volStructIter, DBUS_TYPE_INT16, &earlyDataItr->type);
            if (earlyDataItr->type == ED_SINK_VOLUME)
            {
                success = success
                        && dbus_message_iter_append_basic(&volStructIter, DBUS_TYPE_UINT16, &earlyDataItr->sinksource.sink);
            }
            else
            {
                success = success
                        && dbus_message_iter_append_basic(&volStructIter, DBUS_TYPE_UINT16,
                                                          &earlyDataItr->sinksource.source);
            }
            success = success
                    && dbus_message_iter_append_basic(&volStructIter, DBUS_TYPE_UINT16, &earlyDataItr->data.volume);
            success = success && dbus_message_iter_close_container(&volArrayIter, &volStructIter);
        }
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &volArrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    //dbus_message_iter_next(&mDBusMessageIter);
    earlyDataItr = earlyData.begin();
    //then the sound property array
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &soundsize);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nqnn)", &sndPtyArrayIter);
    for (; success && (earlyDataItr < earlyData.end()); ++earlyDataItr)
    {
        if (earlyDataItr->type == ED_SINK_PROPERTY || earlyDataItr->type == ED_SOURCE_PROPERTY)
        {
            success = success
                    && dbus_message_iter_open_container(&sndPtyArrayIter, DBUS_TYPE_STRUCT, NULL, &sndPtyStructIter);
            success = success && dbus_message_iter_append_basic(&sndPtyStructIter, DBUS_TYPE_INT16, &earlyDataItr->type);
            if (earlyDataItr->type == ED_SINK_PROPERTY)
            {
                success = success
                        && dbus_message_iter_append_basic(&sndPtyStructIter, DBUS_TYPE_UINT16,
                                                          &earlyDataItr->sinksource.sink);
            }
            else
            {
                success = success
                        && dbus_message_iter_append_basic(&sndPtyStructIter, DBUS_TYPE_UINT16,
                                                          &earlyDataItr->sinksource.source);
            }
            success = success
                    && dbus_message_iter_open_container(&sndPtyStructIter, DBUS_TYPE_STRUCT, NULL, &innerStructIter);
            success = success
                    && dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_INT16,
                                                      &earlyDataItr->data.soundProperty.type);
            success = success
                    && dbus_message_iter_append_basic(&innerStructIter, DBUS_TYPE_INT16,
                                                      &earlyDataItr->data.soundProperty.value);
            success = success && dbus_message_iter_close_container(&sndPtyStructIter, &innerStructIter);
            success = success && dbus_message_iter_close_container(&sndPtyArrayIter, &sndPtyStructIter);
        }
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &sndPtyArrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);

    if (!success)
    {
        logError("CDBusSender::append(vector<am_EarlyData_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_Volumes_s> &listVolume)
{
    DBusMessageIter volumeArrayIter;
    DBusMessageIter volumeStructIter;
    vector<am_Volumes_s>::const_iterator volumeItr = listVolume.begin();
    dbus_bool_t success = true;
    DBusMessageIter outerStructIter;
    int size = listVolume.size();
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nqqnq)", &volumeArrayIter);
    for (; success && (volumeItr < listVolume.end()); ++volumeItr)
    {
        success = success && dbus_message_iter_open_container(&volumeArrayIter, DBUS_TYPE_STRUCT, NULL, &volumeStructIter);
        success = success && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_INT16, &volumeItr->volumeType);
        if (volumeItr->volumeType == VT_SINK)
        {
            success = success
                    && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_UINT16, &volumeItr->volumeID.sink);
        }
        else if (volumeItr->volumeType == VT_SOURCE)
        {
            success = success
                    && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_UINT16, &volumeItr->volumeID.source);
        }
        else
        {
            /* Do nothing*/
        }
        success = success && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_UINT16, &volumeItr->volume);
        success = success && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_INT16, &volumeItr->ramp);
        success = success && dbus_message_iter_append_basic(&volumeStructIter, DBUS_TYPE_UINT16, &volumeItr->time);
        success = success && dbus_message_iter_close_container(&volumeArrayIter, &volumeStructIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &volumeArrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_Volumes_s>) error !");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const am_NotificationConfiguration_s &notifConfig)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &notifConfig.type);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &notifConfig.status);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &notifConfig.parameter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_NotificationConfiguration_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_NotificationConfiguration_s> &listnotifConfig)
{
    DBusMessageIter structIter;
    DBusMessageIter arrayIter;
    vector<am_NotificationConfiguration_s>::const_iterator listnotifConfigItr = listnotifConfig.begin();
    dbus_bool_t success = true;
    DBusMessageIter outerStructIter;
    int size = listnotifConfig.size();

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "(nnn)", &arrayIter);
    for (; success && (listnotifConfigItr < listnotifConfig.end()); ++listnotifConfigItr)
    {
        success = success && dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, NULL, &structIter);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listnotifConfigItr->type);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listnotifConfigItr->status);
        success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &listnotifConfigItr->parameter);
        success = success && dbus_message_iter_close_container(&arrayIter, &structIter);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    if (!success)
    {
        logError("CDBusSender::append(vector<am_NotificationConfiguration_s>) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<am_CustomConnectionFormat_t>& listConnFrmt)
{
    DBusMessageIter arrayIter;
    vector<am_CustomConnectionFormat_t>::const_iterator listConnItr;
    dbus_bool_t success = true;
    DBusMessageIter outerStructIter;
    int size = listConnFrmt.size();

    // list of connection format
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "n", &arrayIter);
    for (listConnItr = listConnFrmt.begin(); success && (listConnItr != listConnFrmt.end());
            ++listConnItr)
    {
        int16_t Value = *listConnItr;
        success = success && dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_INT16, &Value);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    // end of structure
    if (!success)
    {
        logError("CDBusSender::append(vector<am_CustomConnectionFormat_t>) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::append(const vector<bool>& convertionmatrix)
{
    DBusMessageIter arrayIter;
    DBusMessageIter outerStructIter;
    int size = convertionmatrix.size();

    vector<bool>::const_iterator ConvMatrixItr;
    uint16_t ItrCnt = 0;
    dbus_bool_t convMatrix;
    dbus_bool_t success = true;

    //List of conversion matrix
    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &outerStructIter);
    success = success && dbus_message_iter_append_basic(&outerStructIter, DBUS_TYPE_INT16, &size);

    success = success && dbus_message_iter_open_container(&outerStructIter, DBUS_TYPE_ARRAY, "b", &arrayIter);
    for (ItrCnt = 0, ConvMatrixItr = convertionmatrix.begin(); success && (ConvMatrixItr < convertionmatrix.end());
            ++ConvMatrixItr, ++ItrCnt)
    {
        convMatrix = convertToDBusType(*ConvMatrixItr);
        success = success && dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_BOOLEAN, &convMatrix);
    }
    success = success && dbus_message_iter_close_container(&outerStructIter, &arrayIter);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &outerStructIter);
    // end of structure
    if (!success)
    {
        logError("CDBusSender::append(vector<bool>) Not enough memory");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}
void CDBusSender::append(const am_NotificationPayload_s& payload)
{
    DBusMessageIter structIter;
    dbus_bool_t success = true;

    success = success && dbus_message_iter_open_container(&mDBusMessageIter, DBUS_TYPE_STRUCT, NULL, &structIter);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &payload.type);
    success = success && dbus_message_iter_append_basic(&structIter, DBUS_TYPE_INT16, &payload.value);
    success = success && dbus_message_iter_close_container(&mDBusMessageIter, &structIter);
    if (!success)
    {
        logError("CDBusSender::append(am_NotificationPayload_s) error: ", mDBusError.message);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
    }
}

void CDBusSender::initSignal(const string& path, const string& signalName)
{
    initSignal(path, signalName, path);
    // add sleep of 10 second here and check for the failure of DBus startup. Need to discuss with Jens
}

void CDBusSender::setPathPrefix(string pathPrefix, string servicePrefix)
{
    mPathPrefix = pathPrefix;
    mServicePrefix = servicePrefix;
}

void CDBusSender::initSignal(const string& path, const string& signalName, const string& interface)
{
    if (!signalName.empty() && !path.empty())
    {
        string completePath = mPathPrefix + "/" + path;
        string completeInterface = mServicePrefix + "." + interface;
        mpDBusMessage = dbus_message_new_signal(completePath.c_str(), completeInterface.c_str(), signalName.c_str());

        if (mpDBusMessage == NULL)
        {
            logError("CDBusSender::initSignal Cannot allocate DBus message!");
        }
        dbus_message_iter_init_append(mpDBusMessage, &mDBusMessageIter);
    }
    else
    {
        logError("CDBusSender::initSignal Cannot allocate DBus message!");
    }
}

void CDBusSender::initReply(DBusMessage* msg)
{
    if (msg != NULL)
    {
        mpDBusReceivedMessage = msg;
        mpDBusMessage = dbus_message_new_method_return(msg);
        if (mpDBusMessage == NULL)
        {
            logError("CDBusSender::initReply Cannot allocate DBus message!");
        }
        dbus_message_iter_init_append(mpDBusMessage, &mDBusMessageIter);
    }
}

am_Error_e CDBusSender::send(void)
{
    int16_t error;
    if( (mpDBusMessage == NULL) || (mpDBusConnection == NULL))
    {
        logError("CDBusSender::send failed either message or connection is NULL");
        return E_ABORTED;
    }

    dbus_error_init(&mDBusError);
    DBusMessage* reply(dbus_connection_send_with_reply_and_block(mpDBusConnection, mpDBusMessage, -1, &mDBusError));
    if (!reply)
    {
        logError("CDBusSender::send failed reply NULL, dbus error", mDBusError.message);
        dbus_error_free(&mDBusError);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
        error = static_cast<int16_t>(E_ABORTED);
    }
    else
    {
        if (!dbus_message_get_args(reply, &mDBusError, DBUS_TYPE_UINT16, &error, DBUS_TYPE_INVALID))
        {
            logError("CDBusSender::send failed , dbus error: ", mDBusError.message);
            dbus_error_free(&mDBusError);
            error = static_cast<int16_t>(E_ABORTED);
        }
        dbus_message_unref(reply);
    }
    dbus_message_unref(mpDBusMessage);
    return (static_cast<am_Error_e>(error));
}

am_Error_e CDBusSender::send_sync(void)
{
    DBusError mDBusError;
    dbus_error_init (&mDBusError);
    if( (mpDBusMessage == NULL) || (mpDBusConnection == NULL))
    {
        logError("CDBusSender::send failed either message or connection is NULL");
        return E_ABORTED;
    }
    mpDBusMessage = dbus_connection_send_with_reply_and_block(mpDBusConnection, mpDBusMessage, -1, &mDBusError);
    if (!mpDBusMessage)
    {
        logError("CDBusSender::send_sync failed, dbus error", mDBusError.message);
        dbus_error_free(&mDBusError);
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
        return E_ABORTED;
    }
    return E_OK;
}

am_Error_e CDBusSender::send_async(void)
{
    dbus_uint32_t mDbusSerial;
    am_Error_e error = E_OK;
    if (mpDBusMessage != 0)
    {
        mDbusSerial = dbus_message_get_serial(mpDBusMessage);
    }
    else
    {
        mDbusSerial = 1;
    }
    if (!dbus_connection_send(mpDBusConnection, mpDBusMessage, &mDbusSerial))
    {
        logError("CDBusSender::send_async cannot send message!");
        mErrorName = string(DBUS_ERROR_NO_MEMORY);
        mErrorMsg = "Cannot create reply!";
        error = E_ABORTED;
    }
    dbus_connection_flush(mpDBusConnection);
    dbus_message_unref(mpDBusMessage);
    return error;
}

void CDBusSender::sendMessage()
{
    mSerial = 1;
    if (mpDBusReceivedMessage != 0)
    {
        mSerial = dbus_message_get_serial(mpDBusReceivedMessage);
    }
    else
    {
        mSerial = 1;
    }
    if (!mErrorName.empty())
    {
        mpDBusMessage = dbus_message_new_error(mpDBusReceivedMessage, mErrorName.c_str(), mErrorMsg.c_str());
    }
    if (!dbus_connection_send(mpDBusConnection, mpDBusMessage, &mSerial))
    {
        logError("CDBusSender::sendMessage cannot send message!");
    }
    dbus_connection_flush(mpDBusConnection);
    dbus_message_unref(mpDBusMessage);
    mpDBusMessage = NULL;
}

}/* namespace am*/
