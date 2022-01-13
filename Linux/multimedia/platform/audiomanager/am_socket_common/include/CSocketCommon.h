/************************************************************************
 * @file: CDBusCommon.h
 *
 * @version: 1.1
 *
 * @description: This file include all common define needed for dbus communication
 *
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

#ifndef CDBUSCOMMON_H_
#define CDBUSCOMMON_H_

#include <string>
#include <map>
#include "audiomanagertypes.h"

namespace am
{

#define ROUTING_SOCKET_NAMESPACE               "routing"
#define	D_AM_DOMAIN_ADDRESS_INVALID				(0xFFFF)						/* invalid value for logical address */

struct socket_comm_s
{
public:
	socket_comm_s(const std::string& objectpath, const std::string& interface)
	{
		iface = std::string(".") + objectpath + std::string(".") + interface;
		domainID = 0;
		destaddr = D_AM_DOMAIN_ADDRESS_INVALID;
		
	};
	std::string iface;
	am_domainID_t domainID;
	unsigned short destaddr;
};

class handle_uint16_s
{
public:
    explicit handle_uint16_s(const uint16_t h)
    {
        handle = h;
    }

    explicit handle_uint16_s(const am_Handle_s& h)
    {
        handle = ((h.handleType & 0x3F)<<10) | (h.handle & 0x3FF);
    }

    am_Handle_s getAmHandle(void) const
    {
        am_Handle_s h;
        h.handle = handle & 0x3FF;
        h.handleType = (am_Handle_e)((handle >> 10)&0x3F);
        return h;
    }
    uint16_t getUint16Handle(void) const
    {
        return handle;
    }

private:
    uint16_t handle;
};

class CSocketCommon
{
public:
    CSocketCommon();
    ~CSocketCommon();
};

} /* namespace am */
#endif /* CDBUSCOMMON_H_ */
