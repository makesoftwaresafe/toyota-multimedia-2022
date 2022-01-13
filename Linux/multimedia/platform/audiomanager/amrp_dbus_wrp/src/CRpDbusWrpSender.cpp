/************************************************************************
 * @file: CRpDbusWrpSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CRaDlinkWrpSend class implementation of Routing Adapter.
 * A wrapper class for sender class. CRaDlinkWrpSend class will call the
 * sender class API which has the actual sender API definition.
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

#include <fstream> //for file operations
#include <stdexcept> // for runtime_error
#include <vector>
#include "audiomanagertypes.h"
#include "CAmDltWrapper.h"
#include "IAmRoutingClient.h"
#include "CRpDbusWrpSender.h"
#include <assert.h>
#include "CAmDbusWrapper.h"

using namespace std;
using namespace am;

DLT_DECLARE_CONTEXT (RP_Dbus)

CRpDbusWrpSender::CRpDbusWrpSender(IAmRoutingClient* const client, CAmDbusWrapper*& wrapper) :
    CDBusRoutingSender(client, wrapper)
{
    CAmDltWrapper::instance()->registerContext(RP_Dbus, "RP_Dbus", "RP_Context");
    mFunctionMap = createMap();
}

CRpDbusWrpSender::~CRpDbusWrpSender()
{
    CAmDltWrapper::instance()->unregisterContext(RP_Dbus);
}

CDBusRoutingSender::functionMap_t CRpDbusWrpSender::createMap()
{
    functionMap_t m;

    m["setRoutingReady"] = &CRpDbusWrpSender::setRoutingReady;
    m["setRoutingRundown"] = &CRpDbusWrpSender::setRoutingRundown;
    m["asyncAbort"] = &CRpDbusWrpSender::asyncAbort;
    m["asyncConnect"] = &CRpDbusWrpSender::asyncConnect;
    m["asyncDisconnect"] = &CRpDbusWrpSender::asyncDisconnect;
    m["asyncSetSinkVolume"] = &CRpDbusWrpSender::asyncSetSinkVolume;
    m["asyncSetSourceVolume"] = &CRpDbusWrpSender::asyncSetSourceVolume;
    m["asyncSetSourceState"] = &CRpDbusWrpSender::asyncSetSourceState;
    m["asyncSetSinkSoundProperties"] = &CRpDbusWrpSender::asyncSetSinkSoundProperties;
    m["asyncSetSinkSoundProperty"] = &CRpDbusWrpSender::asyncSetSinkSoundProperty;
    m["asyncSetSourceSoundProperties"] = &CRpDbusWrpSender::asyncSetSourceSoundProperties;
    m["asyncSetSourceSoundProperty"] = &CRpDbusWrpSender::asyncSetSourceSoundProperty;
    m["asyncCrossFade"] = &CRpDbusWrpSender::asyncCrossFade;
    m["setDomainState"] = &CRpDbusWrpSender::setDomainState;
    m["asyncSetVolumes"] = &CRpDbusWrpSender::asyncSetVolumes;
    m["asyncSetSinkNotificationConfiguration"] = &CRpDbusWrpSender::asyncSetSinkNotificationConfiguration;
    m["asyncSetSourceNotificationConfiguration"] = &CRpDbusWrpSender::asyncSetSourceNotificationConfiguration;

    return (m);
}

