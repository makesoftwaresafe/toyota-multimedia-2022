/************************************************************************
 * @file: CCpDbusWrpSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CCpDbusWrpSender class implementation of command plug-in.
 * A wrapper class for sender class. CCpDbusWrpSender class will forward the
 * call to Application.
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

#include <fstream> // for ifstream
#include <stdexcept> // for runtime_error
#include <vector>
#include "CAmDltWrapper.h"
#include "CCpDbusWrpSender.h"
#include "CDBusCommon.h"
#include "IAmCommandClient.h"

using namespace std;
using namespace am;

DLT_DECLARE_CONTEXT (CP_Dbus)

CCpDbusWrpSender::CCpDbusWrpSender(IAmCommandClient* const client, CAmDbusWrapper*& wrapper) :
        CDBusCommandSender(client, wrapper)
{
    CAmDltWrapper::instance()->registerContext(CP_Dbus, "CP_Dbus", "CP_Context");
    mFunctionMap = createMap();
}

CCpDbusWrpSender::~CCpDbusWrpSender()
{
    CAmDltWrapper::instance()->unregisterContext(CP_Dbus);
}

CDBusCommandSender::functionMap_t CCpDbusWrpSender::createMap()
{
    functionMap_t m;
    m["setCommandReady"] = &CCpDbusWrpSender::setCommandReady;
    m["setCommandRundown"] = &CCpDbusWrpSender::setCommandRundown;
    m["cbNewMainConnection"] = &CCpDbusWrpSender::cbNewMainConnection;
    m["cbRemovedMainConnection"] = &CCpDbusWrpSender::cbRemovedMainConnection;
    m["cbNewSink"] = &CCpDbusWrpSender::cbNewSink;
    m["cbRemovedSink"] = &CCpDbusWrpSender::cbRemovedSink;
    m["cbNewSource"] = &CCpDbusWrpSender::cbNewSource;
    m["cbRemovedSource"] = &CCpDbusWrpSender::cbRemovedSource;
    m["cbNumberOfSinkClassesChanged"] = &CCpDbusWrpSender::cbNumberOfSinkClassesChanged;
    m["cbNumberOfSourceClassesChanged"] = &CCpDbusWrpSender::cbNumberOfSourceClassesChanged;
    m["cbMainConnectionStateChanged"] = &CCpDbusWrpSender::cbMainConnectionStateChanged;
    m["cbMainSinkSoundPropertyChanged"] = &CCpDbusWrpSender::cbMainSinkSoundPropertyChanged;
    m["cbMainSourceSoundPropertyChanged"] = &CCpDbusWrpSender::cbMainSourceSoundPropertyChanged;
    m["cbSinkAvailabilityChanged"] = &CCpDbusWrpSender::cbSinkAvailabilityChanged;
    m["cbSourceAvailabilityChanged"] = &CCpDbusWrpSender::cbSourceAvailabilityChanged;
    m["cbVolumeChanged"] = &CCpDbusWrpSender::cbVolumeChanged;
    m["cbSinkMuteStateChanged"] = &CCpDbusWrpSender::cbSinkMuteStateChanged;
    m["cbSystemPropertyChanged"] = &CCpDbusWrpSender::cbSystemPropertyChanged;
    m["cbTimingInformationChanged"] = &CCpDbusWrpSender::cbTimingInformationChanged;
    m["cbSinkUpdated"] = &CCpDbusWrpSender::cbSinkUpdated;
    m["cbSourceUpdated"] = &CCpDbusWrpSender::cbSourceUpdated;
    m["cbSinkNotification"] = &CCpDbusWrpSender::cbSinkNotification;
    m["cbSourceNotification"] = &CCpDbusWrpSender::cbSourceNotification;
    m["cbMainSinkNotificationConfigurationChanged"] = &CCpDbusWrpSender::cbMainSinkNotificationConfigurationChanged;
    m["cbMainSourceNotificationConfigurationChanged"] = &CCpDbusWrpSender::cbMainSourceNotificationConfigurationChanged;

    return (m);
}

