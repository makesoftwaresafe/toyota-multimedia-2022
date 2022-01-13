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

#include <dbus/dbus.h>
#include <libxml/parser.h> // for xmlParsing library calls
#include <string>
#include <map>
#include "audiomanagertypes.h"

namespace am
{

/* common dbus related information */
#include <audiomanagerconfig.h>
#define DBUS_BUS_NAME                        DBUS_SERVICE_PREFIX
#define DBUS_OBJECT_PATH                     DBUS_SERVICE_OBJECT_PATH
#define DBUS_INTROSPECTION_DIR               "/usr/share/audiomanager"
#define DBUS_SET_INTERFACE_RULE(ADDR)        "type='signal',interface='" DBUS_SERVICE_PREFIX "." ADDR "'"

// command send (DBus signals) and receive (D-Bus methods) use same node
#define COMMAND_DBUS_NAMESAPACE              "command"
#define COMMAND_SEND_DBUS_INTROSPECTION_FILE DBUS_INTROSPECTION_DIR "/CpCommandSender.xml"
#define COMMAND_RECV_DBUS_INTROSPECTION_FILE DBUS_INTROSPECTION_DIR "/CpCommandReceiver.xml"
#define COMMAND_PLUGIN_BUSNAME               DBUS_SERVICE_PREFIX "." COMMAND_DBUS_NAMESAPACE
#define COMMAND_DBUS_OBJECT_PATH             DBUS_SERVICE_OBJECT_PATH "/" COMMAND_DBUS_NAMESAPACE

#define ROUTING_DBUS_NAMESPACE               "routing"
#define ROUTING_SEND_DBUS_INTROSPECTION_FILE DBUS_INTROSPECTION_DIR "/RaRoutingSender.xml"
#define ROUTING_RECV_DBUS_INTROSPECTION_FILE DBUS_INTROSPECTION_DIR "/RaRoutingReceiver.xml"
#define ROUTING_PLUGIN_BUSNAME               DBUS_SERVICE_PREFIX "." ROUTING_DBUS_NAMESPACE
#define ROUTING_DBUS_OBJECT_PATH             DBUS_SERVICE_OBJECT_PATH "/" ROUTING_DBUS_NAMESPACE

struct dbus_comm_s
{
public:
    dbus_comm_s(const dbus_comm_s &obj)
    {
        dest = obj.dest;
        path = obj.path;
        iface = obj.iface;
    }
    dbus_comm_s(const std::string& objectpath) :
        dest(DBUS_BUS_NAME), path(DBUS_OBJECT_PATH), iface(DBUS_BUS_NAME)
    {
        path  += std::string("/") + objectpath;
        iface += std::string(".") + objectpath;
    };
    dbus_comm_s(const std::string& objectpath, const std::string& interface) :
        dest(DBUS_BUS_NAME), path(DBUS_OBJECT_PATH), iface(DBUS_BUS_NAME)
    {
        dest  += std::string(".") + objectpath + std::string(".") + interface;
        path  += std::string("/") + objectpath + std::string("/") + interface;
        iface += std::string(".") + objectpath + std::string(".") + interface;
    };

    void append(const std::string &interface)
    {
        if (interface.empty())
            return;

        dest  += std::string(".") + interface;
        path  += std::string("/") + interface;
        iface += std::string(".") + interface;
    }

    std::string dest;
    std::string path;
    std::string iface;

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
    void setHandle(uint16_t value)
    {
        handle = value;
    }
private:
    uint16_t handle;
};

static inline dbus_bool_t convertToDBusType(const bool value)
{
    dbus_bool_t dbusBoolValue = FALSE;
    if(value == true)
    {
        dbusBoolValue = TRUE;
    }
    return dbusBoolValue;
}


class CDBusCommon
{
public:
    CDBusCommon();
    ~CDBusCommon();
    // To remove space and tab in a string
    inline void removeWhiteSpaces(std::string& str) const;
    // get the key value pair
    void getKeyValPairs(xmlNode *devNode, std::map< std::string, std::string > & keyValPairs);
};

} /* namespace am */
#endif /* CDBUSCOMMON_H_ */
