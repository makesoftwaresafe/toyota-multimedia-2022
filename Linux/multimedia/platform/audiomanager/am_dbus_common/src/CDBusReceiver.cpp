/************************************************************************
 * @file: CDBusReceiver.h
 *
 * @version: 1.1
 *
 * @description: A CDBusReceiver class implementation of Routing Adapter.
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

#include "CAmDltWrapper.h"
#include "CDBusReceiver.h"

using namespace std;

namespace am
{

CDBusReceiver::CDBusReceiver(DBusConnection* connection) :
    mDBusMessageIter(), mDBusError(), mErrorName(""), mErrorMsg(""), mpDBusMessage(NULL), mpDBusConnection(connection)
{
}

CDBusReceiver::~CDBusReceiver()
{
}

void CDBusReceiver::setDBusConnection(DBusConnection*& connection)
{
    if (connection != NULL)
    {
        mpDBusConnection = connection;
    }
}

void CDBusReceiver::initReceive(DBusMessage* msg)
{
    if (msg != NULL)
    {
        if (!dbus_message_iter_init(msg, &mDBusMessageIter))
        {
            logDebug("CDBusReceiver::initReceive DBus Message has no arguments.");
            mErrorName = string(DBUS_ERROR_INVALID_ARGS);
            mErrorMsg = "DBUS Message has no arguments!";
        }
    }
}

char* CDBusReceiver::getString(DBusMessageIter& iter, bool next)
{
    char* param = NULL;

    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&iter))
    {
        dbus_message_iter_get_basic(&iter, &param);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }
    else
    {
        logError("CDBusReceiver::getString DBUS handler argument is no String!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no string";
    }

    return (param);
}

char* CDBusReceiver::getString()
{
    return (getString(mDBusMessageIter, true));
}

dbus_bool_t CDBusReceiver::getBool(DBusMessageIter& iter, bool next)
{
    dbus_bool_t boolparam = false;
    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&iter))
    {
        dbus_message_iter_get_basic(&iter, &boolparam);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }
    else
    {
        logError("CDBusReceiver::getBool DBUS handler argument is no bool!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no bool";
    }

    return (boolparam);
}

dbus_bool_t CDBusReceiver::getBool()
{
    return (getBool(mDBusMessageIter, true));
}

char CDBusReceiver::getByte(DBusMessageIter& iter, bool next)
{
    char param(0);

    if (DBUS_TYPE_BYTE == dbus_message_iter_get_arg_type(&iter))
    {
        dbus_message_iter_get_basic(&iter, &param);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }
    else
    {
        logError("CDBusReceiver::getByte DBUS handler argument is no byte!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no byte";
    }

    return (param);
}

char CDBusReceiver::getByte()
{
    return (getByte(mDBusMessageIter, true));
}

dbus_uint16_t CDBusReceiver::getUInt(DBusMessageIter& iter, bool next)
{
    dbus_uint16_t param(0);
#ifdef GLIB_DBUS_TYPES_TOLERANT
    if (DBUS_TYPE_UINT16 != dbus_message_iter_get_arg_type(&iter) && DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&iter))
#else
    if (DBUS_TYPE_UINT16 != dbus_message_iter_get_arg_type(&iter))
#endif /* GLIB_DBUS_TYPES_TOLERANT */
    {
        logError("CDBusReceiver::getUInt DBUS handler argument is no uint16_t!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no uint16_t";
    }
    else
    {
        dbus_message_iter_get_basic(&iter, &param);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }

    return (param);
}

dbus_uint16_t CDBusReceiver::getUInt()
{
    return (getUInt(mDBusMessageIter, true));
}

dbus_int16_t CDBusReceiver::getInt(DBusMessageIter& iter, bool next)
{
    dbus_int16_t param(0);
#ifdef GLIB_DBUS_TYPES_TOLERANT
    if (DBUS_TYPE_INT16 != dbus_message_iter_get_arg_type(&iter) && DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&iter))
#else
    if (DBUS_TYPE_INT16 != dbus_message_iter_get_arg_type(&iter))
#endif /* GLIB_DBUS_TYPES_TOLERANT */
    {
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no int16_t";
    }
    else
    {
        dbus_message_iter_get_basic(&iter, &param);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }

    return (param);
}

dbus_int16_t CDBusReceiver::getInt()
{
    return (getInt(mDBusMessageIter, true));
}

double CDBusReceiver::getDouble(DBusMessageIter& iter, bool next)
{
    double param(0);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&iter))
    {
        dbus_message_iter_get_basic(&iter, &param);
        if (next)
        {
            dbus_message_iter_next(&iter);
        }
    }
    else
    {
        logError("CDBusReceiver::getDouble DBUS handler argument is no double!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no double";
    }
    return (param);
}

double CDBusReceiver::getDouble()
{
    return (getDouble(mDBusMessageIter, true));
}

void CDBusReceiver::getSoundProperty(am_SoundProperty_s& soundProperty)
{
    DBusMessageIter structIter;
    dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
    soundProperty.type = static_cast<am_CustomSoundPropertyType_t> (getInt(structIter, true));
    soundProperty.value = static_cast<int16_t> (getInt(structIter, false));
    dbus_message_iter_next(&mDBusMessageIter);
}

void CDBusReceiver::getListSoundProperties(vector<am_SoundProperty_s>& listSoundProperties)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SoundProperty_s soundProperty;
        dbus_int16_t size;
        listSoundProperties.clear();
        DBusMessageIter structIter, arrayIter, outerstructIter;
        dbus_message_iter_recurse(&mDBusMessageIter, &outerstructIter);
        size = getInt(outerstructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerstructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                soundProperty.type = static_cast<am_CustomSoundPropertyType_t> (getInt(structIter, true));
                soundProperty.value = static_cast<int16_t> (getInt(structIter, false));
                listSoundProperties.push_back(soundProperty);
            }
            while (dbus_message_iter_next(&arrayIter));

        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSoundProperties DBUS handler argument is no struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getMainSoundProperty(am_MainSoundProperty_s& mainSoundProperty)
{
    DBusMessageIter structIter;
    dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
    mainSoundProperty.type = static_cast<am_CustomMainSoundPropertyType_t> (getInt(structIter, true));
    mainSoundProperty.value = static_cast<int16_t> (getInt(structIter, false));
    dbus_message_iter_next(&mDBusMessageIter);
}

void CDBusReceiver::getListMainSoundProperties(vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_MainSoundProperty_s mainSoundProperty;
        dbus_int16_t size;
        listMainSoundProperties.clear();
        DBusMessageIter structIter, arrayIter, outerstructIter;
        dbus_message_iter_recurse(&mDBusMessageIter, &outerstructIter);
        size = getInt(outerstructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerstructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                mainSoundProperty.type = static_cast<am_CustomMainSoundPropertyType_t> (getInt(structIter, true));
                mainSoundProperty.value = static_cast<int16_t> (getInt(structIter, false));
                listMainSoundProperties.push_back(mainSoundProperty);
            }
            while (dbus_message_iter_next(&arrayIter));
        }

        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListMainSoundProperties DBUS handler argument is no struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getListConnFrmt(vector<am_CustomConnectionFormat_t>& listConnFrmt)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_CustomConnectionFormat_t connectionFormat;
        DBusMessageIter arrayIter;
        DBusMessageIter outerStructIter;
        dbus_int16_t size;
        listConnFrmt.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter,true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter,&arrayIter);
            do
            {
                connectionFormat = static_cast<am_CustomConnectionFormat_t> (getInt(arrayIter, false));
                listConnFrmt.push_back(connectionFormat);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListConnFrmt DBUS handler argument is no struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getMainConnectionType(am_MainConnectionType_s &mainConnectionType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        DBusMessageIter structIter;
        dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
        mainConnectionType.mainConnectionID = static_cast<am_mainConnectionID_t> (getUInt(structIter, true));
        mainConnectionType.sourceID = static_cast<am_sourceID_t> (getUInt(structIter, true));
        mainConnectionType.sinkID = static_cast<am_sinkID_t> (getUInt(structIter, true));
        mainConnectionType.delay = static_cast<am_timeSync_t> (getInt(structIter, true));
        mainConnectionType.connectionState = static_cast<am_ConnectionState_e> (getInt(structIter, false));
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
//        logError("CDBusReceiver::getMainConnectionType DBUS handler argument is not a struct!"); // Need to be fixed
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is not a struct";
    }
}

void CDBusReceiver::getListMainConnectionType(vector<am_MainConnectionType_s> &listMainConnectionType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_MainConnectionType_s mainConnectionType;
        DBusMessageIter structIter, arrayIter, outerStructIter;
        dbus_int16_t size;
        listMainConnectionType.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter,true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter,&arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                mainConnectionType.mainConnectionID = static_cast<am_mainConnectionID_t> (getUInt(structIter, true));
                mainConnectionType.sourceID = static_cast<am_sourceID_t> (getUInt(structIter, true));
                mainConnectionType.sinkID = static_cast<am_sinkID_t> (getUInt(structIter, true));
                mainConnectionType.delay = static_cast<am_timeSync_t> (getInt(structIter, true));
                mainConnectionType.connectionState = static_cast<am_ConnectionState_e> (getInt(structIter, false));
                listMainConnectionType.push_back(mainConnectionType);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListMainConnectionType DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getSinkType(am_SinkType_s &sinkType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        DBusMessageIter structIter;
        DBusMessageIter structIterAvail;
        dbus_message_iter_recurse(&mDBusMessageIter, &structIter);

        sinkType.sinkID = static_cast<am_sinkID_t> (getUInt(structIter, true));
        sinkType.name = getString(structIter, true);
        //availability
        dbus_message_iter_recurse(&structIter, &structIterAvail);
        sinkType.availability.availability = static_cast<am_Availability_e> (getInt(structIterAvail, true));
        sinkType.availability.availabilityReason = static_cast<am_CustomAvailabilityReason_t> (getInt(structIterAvail, false));
        dbus_message_iter_next(&structIter);
        sinkType.volume = static_cast<am_mainVolume_t> (getInt(structIter, true));
        sinkType.muteState = static_cast<am_MuteState_e> (getInt(structIter, true));
        sinkType.sinkClassID = static_cast<am_sinkClass_t> (getUInt(structIter, false));
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getSinkType DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is not a struct";
    }
}

void CDBusReceiver::getListSinkType(vector<am_SinkType_s> &listSinkType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SinkType_s sinkType;
        DBusMessageIter structIter, arrayIter;
        DBusMessageIter structIterAvail;
        DBusMessageIter outerStructIter;
        dbus_int16_t size;
        listSinkType.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter,true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                sinkType.sinkID = static_cast<am_sinkID_t> (getUInt(structIter, true));
                sinkType.name = getString(structIter, true);
                //availability
                dbus_message_iter_recurse(&structIter, &structIterAvail);
                sinkType.availability.availability = static_cast<am_Availability_e> (getInt(structIterAvail, true));
                sinkType.availability.availabilityReason = static_cast<am_CustomAvailabilityReason_t> (getInt(structIterAvail, false));
                dbus_message_iter_next(&structIter);
                sinkType.volume = static_cast<am_mainVolume_t> (getInt(structIter, true));
                sinkType.muteState = static_cast<am_MuteState_e> (getInt(structIter, true));
                sinkType.sinkClassID = static_cast<am_sinkClass_t> (getUInt(structIter, false));
                listSinkType.push_back(sinkType);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSinkType DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getSourceType(am_SourceType_s &sourceType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        DBusMessageIter structIter;
        DBusMessageIter structIterAvail;
        dbus_message_iter_recurse(&mDBusMessageIter, &structIter);

        sourceType.sourceID = static_cast<am_sourceID_t> (getUInt(structIter, true));
        sourceType.name = getString(structIter, true);
        //availability
        dbus_message_iter_recurse(&structIter, &structIterAvail);
        sourceType.availability.availability = static_cast<am_Availability_e> (getInt(structIterAvail, true));
        sourceType.availability.availabilityReason = static_cast<am_CustomAvailabilityReason_t> (getInt(structIterAvail, false));
        dbus_message_iter_next(&structIter);
        sourceType.sourceClassID = static_cast<am_sourceClass_t> (getUInt(structIter, false));
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getSourceType DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is not a struct";
    }
}

void CDBusReceiver::getListSourceType(vector<am_SourceType_s> &listSourceType)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SourceType_s sourceType;
        DBusMessageIter structIter, arrayIter, outerStructIter;
        DBusMessageIter structIterAvail;
        dbus_int16_t size;
        listSourceType.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                sourceType.sourceID = static_cast<am_sourceID_t> (getUInt(structIter, true));
                sourceType.name = getString(structIter, true);
                dbus_message_iter_recurse(&structIter, &structIterAvail);
                sourceType.availability.availability = static_cast<am_Availability_e> (getInt(structIterAvail, true));
                sourceType.availability.availabilityReason = static_cast<am_CustomAvailabilityReason_t> (getInt(structIterAvail, false));
                dbus_message_iter_next(&structIter);
                sourceType.sourceClassID = static_cast<am_sourceClass_t> (getUInt(structIter, false));
                listSourceType.push_back(sourceType);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSourceType DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getAvailability(am_Availability_s& availability)
{
    DBusMessageIter structIter;
    dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
    availability.availability = static_cast<am_Availability_e> (getInt(structIter, true));
    availability.availabilityReason = static_cast<am_CustomAvailabilityReason_t> (getInt(structIter, false));
    dbus_message_iter_next(&mDBusMessageIter);
}

void CDBusReceiver::getListSourceClass(vector<am_SourceClass_s> &listSourceClasses)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SourceClass_s sourceClass;
        am_ClassProperty_s ClassProperties;
        vector < am_ClassProperty_s > listClassProperties;
        DBusMessageIter structIter, arrayIter, structIter2, arrayIter2, outerStructIter;
        dbus_int16_t size;
        listSourceClasses.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                sourceClass.sourceClassID = static_cast<am_sourceID_t> (getUInt(structIter, true));
                sourceClass.name = getString(structIter, true);
                size = getInt(structIter, true);
                listClassProperties.clear();

                if(size > 0)
                {
                    dbus_message_iter_recurse(&structIter, &arrayIter2);
                    do
                    {
                        dbus_message_iter_recurse(&arrayIter2, &structIter2);
                        ClassProperties.classProperty = static_cast<am_CustomClassProperty_t> (getUInt(structIter, true));
                        ClassProperties.value = getInt(structIter, false);
                        listClassProperties.push_back(ClassProperties);
                    }
                    while (dbus_message_iter_next(&arrayIter));
                }
                sourceClass.listClassProperties = listClassProperties;
                listSourceClasses.push_back(sourceClass);
            }
            while (dbus_message_iter_next(&arrayIter));
        }

        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSourceClass DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getListSinkClass(vector<am_SinkClass_s> &listSinkClasses)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SinkClass_s sinkClass;
        am_ClassProperty_s ClassProperties;
        vector < am_ClassProperty_s > listClassProperties;
        DBusMessageIter structIter, arrayIter, structIter2, arrayIter2, outerStructIter;
        dbus_int16_t size;
        listSinkClasses.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                sinkClass.sinkClassID = static_cast<am_sinkClass_t> (getUInt(structIter, true));
                sinkClass.name = getString(structIter, true);
                size = getInt(structIter, true);
                listClassProperties.clear();

                if(size > 0)
                {
                    dbus_message_iter_recurse(&structIter, &arrayIter2);
                    do
                    {
                        dbus_message_iter_recurse(&arrayIter2, &structIter2);
                        ClassProperties.classProperty = static_cast<am_CustomClassProperty_t> (getUInt(structIter, true));
                        ClassProperties.value = getInt(structIter, false);
                        listClassProperties.push_back(ClassProperties);
                    }
                    while (dbus_message_iter_next(&arrayIter2));
                }
                sinkClass.listClassProperties = listClassProperties;
                listSinkClasses.push_back(sinkClass);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSinkClass DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getSystemProperty(am_SystemProperty_s &systemProperty)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        DBusMessageIter structIter;
        dbus_message_iter_recurse(&mDBusMessageIter, &structIter);

        systemProperty.type = static_cast<am_CustomSystemPropertyType_t> (getInt(structIter, true));
        systemProperty.value = getInt(structIter, false);
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getSystemProperty DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is not a struct";
    }
}

void CDBusReceiver::getListSystemProperty(vector<am_SystemProperty_s> &listSystemProperty)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_SystemProperty_s systemProperty;
        DBusMessageIter structIter, arrayIter, outerStructIter;
        dbus_int16_t size;
        listSystemProperty.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter, true);

        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                systemProperty.type = static_cast<am_CustomSystemPropertyType_t> (getInt(structIter, true));
                systemProperty.value = getInt(structIter, false);
                listSystemProperty.push_back(systemProperty);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListSystemProperty DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}
void CDBusReceiver::getDomainData(am_Domain_s& domainData)
{
    domainData.domainID = static_cast<am_domainID_t> (getUInt());
    domainData.name = getString();
    domainData.busname = getString();
    domainData.nodename = getString();
    domainData.early = getBool();
    domainData.complete = getBool();
    domainData.state = static_cast<am_DomainState_e> (getUInt());
}

void CDBusReceiver::getSourceData(am_Source_s& sourceData)
{
    sourceData.sourceID = static_cast<am_sourceID_t> (getUInt());
    sourceData.domainID = static_cast<am_domainID_t> (getUInt());
    sourceData.name = getString();
    sourceData.sourceClassID = static_cast<am_sourceClass_t> (getUInt());
    sourceData.sourceState = static_cast<am_SourceState_e> (getUInt());
    sourceData.volume = static_cast<am_volume_t> (getInt());
    sourceData.visible = getBool();
    //availability
    getAvailability(sourceData.available);
    sourceData.interruptState = static_cast<am_InterruptState_e> (getUInt());
    // listSoundProperties
    getListSoundProperties(sourceData.listSoundProperties);
    // listConnectionFormats
    getListConnFrmt(sourceData.listConnectionFormats);
    //listMainSoundProperties
    getListMainSoundProperties(sourceData.listMainSoundProperties);

#if 0 // Need to be uncommented after fixing all attributes initialisation
    DBusMessageIter arrayIter;
    DBusMessageIter structIter;
    am_NotificationConfiguration_s amNotifConfig;
    dbus_message_iter_next(&mDBusMessageIter);
    dbus_message_iter_recurse(&mDBusMessageIter, &arrayIter);
    do
    {
        dbus_message_iter_recurse(&arrayIter, &structIter);
        amNotifConfig.type = static_cast<am_CustomNotificationType_t>(getInt(
                        structIter, true));
        amNotifConfig.status = static_cast<am_NotificationStatus_e>(getInt(
                        structIter, true));
        amNotifConfig.parameter = getInt(structIter, false);
        sourceData.listMainNotificationConfigurations.push_back(amNotifConfig);
    }while (dbus_message_iter_next(&arrayIter));

    dbus_message_iter_next(&mDBusMessageIter);
    dbus_message_iter_recurse(&mDBusMessageIter, &arrayIter);
    do
    {
        dbus_message_iter_recurse(&arrayIter, &structIter);
        amNotifConfig.type = static_cast<am_CustomNotificationType_t>(getInt(
                        structIter, true));
        amNotifConfig.status = static_cast<am_NotificationStatus_e>(getInt(
                        structIter, true));
        amNotifConfig.parameter = getInt(structIter, false);
        sourceData.listNotificationConfigurations.push_back(amNotifConfig);
    }while (dbus_message_iter_next(&arrayIter));
#endif /* if 0*/
    getListNotificationConfiguration(sourceData.listMainNotificationConfigurations);
    getListNotificationConfiguration(sourceData.listNotificationConfigurations);

}

void CDBusReceiver::getSinkData(am_Sink_s& sinkData)
{
    sinkData.sinkID = static_cast<am_sinkID_t> (getUInt());
    sinkData.name = getString();
    sinkData.domainID = static_cast<am_domainID_t> (getUInt());
    sinkData.sinkClassID = static_cast<am_sinkClass_t> (getUInt());
    sinkData.volume = static_cast<am_volume_t> (getInt());
    sinkData.visible = getBool();
    //availability
    getAvailability(sinkData.available);
    sinkData.muteState = static_cast<am_MuteState_e> (getInt());
    sinkData.mainVolume = static_cast<am_mainVolume_t> (getInt());
    // listSoundProperties
    getListSoundProperties(sinkData.listSoundProperties);
    // listConnectionFormats
    getListConnFrmt(sinkData.listConnectionFormats);
    //listMainSoundProperties
    getListMainSoundProperties(sinkData.listMainSoundProperties);
    getListNotificationConfiguration(sinkData.listMainNotificationConfigurations);
    getListNotificationConfiguration(sinkData.listNotificationConfigurations);
}

void CDBusReceiver::getGatewayData(am_Gateway_s& gatewayData)
{
    DBusMessageIter arrayIter, outerStructIter;
    dbus_int16_t size;
    bool convertion;

    gatewayData.gatewayID = static_cast<am_gatewayID_t> (getUInt());
    gatewayData.name = getString();
    gatewayData.sinkID = static_cast<am_sinkID_t> (getUInt());
    gatewayData.sourceID = static_cast<am_sourceID_t> (getUInt());
    gatewayData.domainSinkID = static_cast<am_domainID_t> (getUInt());
    gatewayData.domainSourceID = static_cast<am_domainID_t> (getUInt());
    gatewayData.controlDomainID = static_cast<am_domainID_t> (getUInt());
    // listSourceFormats
    getListConnFrmt(gatewayData.listSourceFormats);
    // listSinkFormats
    getListConnFrmt(gatewayData.listSinkFormats);
    dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
    size = getInt(outerStructIter, true);
    // convertionMatrix
    gatewayData.convertionMatrix.clear();
    if(size > 0)
    {
        dbus_message_iter_recurse(&outerStructIter, &arrayIter);
        do
        {
            convertion = getBool(arrayIter, false);
            gatewayData.convertionMatrix.push_back(convertion);
        }
        while (dbus_message_iter_next(&arrayIter));
    }
    dbus_message_iter_next(&mDBusMessageIter);
}

void CDBusReceiver::getEarlyData(vector<am_EarlyData_s>& listEarlyData)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {

        DBusMessageIter arrayIter, structIter, soundpropIter, outerStructIter;
        dbus_int16_t size;
        am_EarlyData_s earlyData;
        //volume array
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter, true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter,&arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                earlyData.type = static_cast<am_EarlyDataType_e> (getInt(structIter, true));
                if (earlyData.type == ED_SINK_VOLUME)
                {
                    earlyData.sinksource.sink = static_cast<am_sinkID_t> (getUInt(structIter, true));
                }
                else
                {
                    earlyData.sinksource.source = static_cast<am_sourceID_t> (getUInt(structIter, true));
                }
                earlyData.data.volume = static_cast<am_volume_t> (getInt(structIter, false));
                listEarlyData.push_back(earlyData);
            }while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&outerStructIter);
        size = getInt(outerStructIter,true);
        //soundproperty array
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter, &arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                earlyData.type = static_cast<am_EarlyDataType_e> (getInt(structIter, true));
                if (earlyData.type == ED_SINK_PROPERTY)
                {
                    earlyData.sinksource.sink = static_cast<am_sinkID_t> (getUInt(structIter, true));
                }
                else
                {
                    earlyData.sinksource.source = static_cast<am_sourceID_t> (getUInt(structIter, true));
                }
                dbus_message_iter_recurse(&structIter, &soundpropIter);
                earlyData.data.soundProperty.type = static_cast<am_CustomSoundPropertyType_t> (getInt(soundpropIter, true));
                earlyData.data.soundProperty.value = (getInt(soundpropIter, false));
                listEarlyData.push_back(earlyData);
            }
            while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getEarlyData DBUS handler argument is no array!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getListVolumes(vector<am_Volumes_s>& listVolumes)
{
    am_Volumes_s VolumeInfo;
    DBusMessageIter structIter, arrayIter, unionIter;

    dbus_message_iter_recurse(&mDBusMessageIter, &arrayIter);
    do
    {
        dbus_message_iter_recurse(&arrayIter, &structIter);
        VolumeInfo.volumeType = static_cast<am_VolumeType_e> (getInt(structIter, true));
        if (VolumeInfo.volumeType == VT_SINK)
        {
            VolumeInfo.volumeID.sink = static_cast<am_sinkID_t> (getUInt(unionIter, true));
        }
        else
        {
            VolumeInfo.volumeID.source = static_cast<am_sourceID_t> (getUInt(unionIter, false));
        }
        VolumeInfo.volume = static_cast<am_volume_t> (getUInt(structIter, true));
        VolumeInfo.ramp = static_cast<am_CustomRampType_t> (getInt(structIter, true));
        VolumeInfo.time = static_cast<am_time_t> (getUInt(structIter, false));
        listVolumes.push_back(VolumeInfo);
    }
    while (dbus_message_iter_next(&arrayIter));
}

void CDBusReceiver::getNotificationConfiguration(am_NotificationConfiguration_s& notificationConfiguration)
{
    DBusMessageIter structIter;

    dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
    notificationConfiguration.type = static_cast<am_CustomNotificationType_t> (getInt(structIter, true));
    notificationConfiguration.status = static_cast<am_NotificationStatus_e> (getInt(structIter, true));
    notificationConfiguration.parameter = static_cast<int16_t> (getInt(structIter, false));
}

void CDBusReceiver::getListNotificationConfiguration(
                                                     vector<am_NotificationConfiguration_s> &listNotificationConfiguration)
{
    if (DBUS_TYPE_STRUCT == dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        am_NotificationConfiguration_s notificationConfiguration;
        DBusMessageIter structIter, arrayIter, outerStructIter;
        dbus_int16_t size;
        listNotificationConfiguration.clear();
        dbus_message_iter_recurse(&mDBusMessageIter, &outerStructIter);
        size = getInt(outerStructIter,true);
        if(size > 0)
        {
            dbus_message_iter_recurse(&outerStructIter,&arrayIter);
            do
            {
                dbus_message_iter_recurse(&arrayIter, &structIter);
                notificationConfiguration.type = static_cast<am_CustomNotificationType_t> (getInt(structIter, true));
                notificationConfiguration.status = static_cast<am_NotificationStatus_e> (getInt(structIter, true));
                notificationConfiguration.parameter = static_cast<int16_t> (getInt(structIter, false));
                listNotificationConfiguration.push_back(notificationConfiguration);
            }while (dbus_message_iter_next(&arrayIter));
        }
        dbus_message_iter_next(&mDBusMessageIter);
    }
    else
    {
        logError("CDBusReceiver::getListNotificationConfiguration DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
}

void CDBusReceiver::getNotificationPayload(am_NotificationPayload_s& payload)
{
    DBusMessageIter structIter;

    dbus_message_iter_recurse(&mDBusMessageIter, &structIter);
    payload.type = static_cast<am_CustomNotificationType_t> (getInt(structIter, true));
    payload.value = static_cast<int16_t> (getInt(structIter, false));
}

void CDBusReceiver::getConvertionMatrix(vector<bool>& convertionMatrix)
{
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&mDBusMessageIter))
    {
        logError("CDBusReceiver::getConvertionMatrix DBUS handler argument is not a struct!");
        mErrorName = string(DBUS_ERROR_INVALID_ARGS);
        mErrorMsg = "DBus argument is no array";
    }
    else
    {
        bool convertion;
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&mDBusMessageIter, &arrayIter);
        do
        {
            convertion = getBool(arrayIter, false);
            convertionMatrix.push_back(convertion);
        }
        while (dbus_message_iter_next(&arrayIter));
        dbus_message_iter_next(&mDBusMessageIter);
    }

}
} /* namespace am*/
