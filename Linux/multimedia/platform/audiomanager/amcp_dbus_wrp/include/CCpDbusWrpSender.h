/************************************************************************
 * @file: CCpDbusWrpSender.h
 *
 * @version: 1.1
 *
 * @description: A CCpDbusWrpSender class definition of command plug-in.
 * A wrapper class for sender class. CCpDbusWrpSender class will call the
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

#ifndef CCPDBUSWRPSENDER_H_
#define CCPDBUSWRPSENDER_H_

#include "CDBusCommandSender.h"

namespace am
{

class IAmCommandClient;

class CCpDbusWrpSender : public CDBusCommandSender
{
public:
    CCpDbusWrpSender(IAmCommandClient* const client, CAmDbusWrapper*& wrapper);
    virtual ~CCpDbusWrpSender();

protected:
    typedef void (CDBusCommandSender::*CallBackMethod)(DBusMessage *message);
    typedef std::map<std::string, CallBackMethod> functionMap_t;
    functionMap_t createMap();
};

} /* namespace am */

#endif /* CCPDBUSWRPSENDER_H_ */
