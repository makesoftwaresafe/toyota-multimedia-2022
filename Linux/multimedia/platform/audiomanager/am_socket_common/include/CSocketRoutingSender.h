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
#include "CAmSocketWrapper.h"
#include "CSocketSender.h"
#include "CAmSocketHandler.h"
#include "CSocketCommon.h"

namespace am
{

class IDBusRoutingClient;

class CSocketRoutingSender
{
public:
    CSocketRoutingSender(ISocketRoutingClient* const client, CAmSocketWrapper*& wrapper);
    virtual ~CSocketRoutingSender();
    void setRoutingReady(void *msg);
    void setRoutingRundown(void *msg);
    void asyncAbort(void *msg);
    void asyncConnect(void *msg);
    void asyncDisconnect(void *msg);
    void asyncSetSinkVolume(void *msg);
    void asyncSetSourceVolume(void *msg);
    void asyncSetSourceState(void *msg);
    void asyncSetSinkSoundProperties(void *msg);
    void asyncSetSinkSoundProperty(void *msg);
    void asyncSetSourceSoundProperties(void *msg);
    void asyncSetSourceSoundProperty(void *msg);
    void asyncSetVolumes(void *msg);
    void asyncSetSinkNotificationConfiguration(void *msg);
    void asyncSetSourceNotificationConfiguration(void *msg);
    void asyncCrossFade(void *msg);
    void setDomainState(void *msg);

    void receiveCallback(void *pmsg);
	TRecieveCallBack<CSocketRoutingSender> mpRecieveCallBack;

protected:
    CAmSocketWrapper         *mpCAmSocketWrapper;
    ISocketRoutingClient     *mpISocketRoutingClient;
    CSocketSender            mCSocketSender;

    typedef void (CSocketRoutingSender::*CallBackMethod)(void *pmsg);
    typedef std::map<unsigned short, CallBackMethod> functionMap_t;
    functionMap_t mFunctionMap;

    /**
     * creates the function map needed to combine DBus messages and function adresses
     * @return the map
     */
    functionMap_t createMap();
};

} /* namespace am */

#endif /* _C_DBUS_ROUTING_SENDER_H_ */
