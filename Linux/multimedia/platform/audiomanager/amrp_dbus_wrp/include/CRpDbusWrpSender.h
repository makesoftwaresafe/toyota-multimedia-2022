/************************************************************************
 * @file: CRaDbusWrpSender.h
 *
 * @version: 1.1
 *
 * @description: A CRaDbusWrpSender class definition of Routing Adapter.
 * A wrapper class for sender class. CRaDbusWrpSender class will call the
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

#ifndef CRPDBUSWRPSENDER_H_
#define CRPDBUSWRPSENDER_H_

#include "CDBusRoutingSender.h"

namespace am
{

class IAmRoutingClient;

class CRpDbusWrpSender : public CDBusRoutingSender
{
public:
    CRpDbusWrpSender(IAmRoutingClient* const client, CAmDbusWrapper*& wrapper);
    virtual ~CRpDbusWrpSender();

protected:
    typedef void (CDBusRoutingSender::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t createMap();
};

} /* namespace am */

#endif /* CRADBUSWRPSENDER_H_ */
