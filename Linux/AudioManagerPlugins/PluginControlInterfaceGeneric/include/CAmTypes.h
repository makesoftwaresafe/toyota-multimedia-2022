/******************************************************************************
 * @file: CAmTypes.h
 *
 * This file contains the declaration of all the constants and enum shared
 * between framework and policy engine.
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Toshiaki Isogai <tisogai@jp.adit-jv.com>
 *          Kapildev Patel  <kpatel@jp.adit-jv.com>
 *          Prashant Jain   <pjain@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/

#ifndef GC_TYPES_H_
#define GC_TYPES_H_

#include "IAmControlCommon.h"
#include <set>

namespace am {
namespace gc {

#define DEFAULT_UNDO_TIMEOUT        (10000)
#define INFINITE_TIMEOUT            (0xFFFFFFFF)
#define DEFAULT_RAMP_TIME           (200)
#define DEFAULT_ASYNC_ACTION_TIME   (2000)
#define DEFAULT_RAMP_TYPE           (RAMP_GENIVI_DIRECT)
#define DEFAULT_OFFSET_VOLUME       (0)
#define DEFAULT_LIMIT_PATTERN       (0xFFFFFFFF)
#define E_WAIT_FOR_CHILD_COMPLETION    (-1)

//supported parameters of actions
#define ACTION_PARAM_CLASS_NAME             "className"
#define ACTION_PARAM_CONNECTION_NAME        "connectionName"
#define ACTION_PARAM_MAIN_CONNECTION_NAME   "mainConnectionName"
#define ACTION_PARAM_SOURCE_NAME            "sourceName"
#define ACTION_PARAM_SINK_NAME              "sinkName"
#define ACTION_PARAM_GATEWAY_NAME           "gatewayName"
#define ACTION_PARAM_RAMP_TYPE              "rampType"
#define ACTION_PARAM_RAMP_TIME              "rampTime"
#define ACTION_PARAM_MAIN_VOLUME_STEP       "mainVolumeStep"
#define ACTION_PARAM_MAIN_VOLUME            "mainVolume"
#define ACTION_PARAM_VOLUME_STEP            "volumeStep"
#define ACTION_PARAM_VOLUME                 "volume"
#define ACTION_PARAM_ORDER                  "order"
#define ACTION_PARAM_PROPERTY_TYPE          "propertyType"
#define ACTION_PARAM_PROPERTY_VALUE         "propertyValue"
#define ACTION_PARAM_TIMEOUT                "timeOut"
#define ACTION_PARAM_PATTERN                "pattern"
#define ACTION_PARAM_CONNECTION_STATE       "connectionState"
#define ACTION_PARAM_CONNECTION_FORMAT      "connectionFormat"
#define ACTION_PARAM_MUTE_STATE             "muteState"
#define ACTION_PARAM_EXCEPT_SOURCE_NAME     "exceptSource"
#define ACTION_PARAM_EXCEPT_SINK_NAME       "exceptSink"
#define ACTION_PARAM_EXCEPT_CLASS_NAME      "exceptClass"
#define ACTION_PARAM_SOURCE_INFO            "sourceInfo"
#define ACTION_PARAM_SINK_INFO              "sinkInfo"
#define ACTION_PARAM_GATEWAY_INFO           "gatewayInfo"
#define ACTION_PARAM_SOURCE_STATE           "sourceState"
#define ACTION_PARAM_ELEMENT_TYPE           "elementType"
#define ACTION_PARAM_LIMIT_STATE            "limitState"
#define ACTION_PARAM_DEBUG_TYPE             "debugType"
#define ACTION_PARAM_DEBUG_VALUE            "debugValue"
#define ACTION_PARAM_LIMIT_TYPE             "limitType"
#define ACTION_PARAM_LIMIT_VOLUME           "limitVolume"
#define ACTION_PARAM_LIMIT_MAP              "mapLimits"
#define ACTION_PARAM_SAVE_LAST_VOLUME       "saveLastVolume"
#define ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE    "notificationConfigurationType"
#define ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS  "notificationConfigurationStatus"
#define ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM    "notificationConfigurationParam"
#define ACTION_PARAM_SET_SOURCE_STATE_DIRECTION "setSourceStateDirection"

#define ACTION_NAME_UNKNOWN             "ACTION_UNKNOWN"
#define ACTION_NAME_CONNECT             "ACTION_CONNECT"
#define ACTION_NAME_DISCONNECT          "ACTION_DISCONNECT"
#define ACTION_NAME_INTERRUPT           "ACTION_INTERRUPT"
#define ACTION_NAME_SUSPEND             "ACTION_SUSPEND"
#define ACTION_NAME_LIMIT               "ACTION_LIMIT"
#define ACTION_NAME_MUTE                "ACTION_MUTE"
#define ACTION_NAME_SET_VOLUME          "ACTION_SET_VOLUME"
#define ACTION_NAME_SET_PROPERTY        "ACTION_SET_PROPERTY"
#define ACTION_NAME_REGISTER            "ACTION_REGISTER"
#define ACTION_DEBUG                    "ACTION_DEBUG"
#define ACTION_NAME_STORED             "ACTION_STORED"
#define ACTION_NAME_SET_NOTIFICATION_CONFIGURATION  "ACTION_SET_NOTIFICATION_CONFIGURATION"

#define SYSTEM_ELEMENT_NAME             "System"
#define DEFAULT_CLASS_NAME              "default"
#define AM_VOLUME_NO_LIMIT              0
#define CLASS_ID_BASE                   101
/*
 * These system properties are reserved for controllers use only. The numbers are allocated using
 * following rules
 * - system property occupies the last 4096 entries of the range i,e from 61440 to 65535
 * - 12 bit System property is made up of 5 bit use case ID and 7 bit system property
 *  In other words it can be calculated as
 *  61440 + 128*X + Y, where 61440 is the reserved system property base
 *                           X is the use case ID, and
 *                           Y is the system property.
 *
 * The use case ID are defined as below
 * 0 -> Global.
 * 1 -> Registration/Unregistration use case
 * 2 -> connect/disconnect use case.
 * 3 -> set Volume use case.
 * 4 -> set Sound property use case
 * 5 -> set system property use case.
 * 6 -> update use case.
 * 7 -> notification configuration use case.
 * 8 -> Related to action execution.
 */
#define RESERVED_PROPERTIES_BASE    ((1<<16) - (1<<12))
#define PROPERTY_USE_CASE_ID_SHIFT  7

#define GLOBAL_PROPERTY_BASE        RESERVED_PROPERTIES_BASE + (0<<PROPERTY_USE_CASE_ID_SHIFT)
#define REGISTRATION_PROPERTY_BASE  RESERVED_PROPERTIES_BASE + (1<<PROPERTY_USE_CASE_ID_SHIFT)
#define CONNECTION_PROPERTY_BASE    RESERVED_PROPERTIES_BASE + (2<<PROPERTY_USE_CASE_ID_SHIFT)
#define VOLUME_PROPERTY_BASE        RESERVED_PROPERTIES_BASE + (3<<PROPERTY_USE_CASE_ID_SHIFT)
#define SOUND_PROP_PROPERTY_BASE    RESERVED_PROPERTIES_BASE + (4<<PROPERTY_USE_CASE_ID_SHIFT)
#define SYS_PROPERTY_PROPERTY_BASE  RESERVED_PROPERTIES_BASE + (5<<PROPERTY_USE_CASE_ID_SHIFT)
#define UPDATE_PROPERTY_BASE        RESERVED_PROPERTIES_BASE + (6<<PROPERTY_USE_CASE_ID_SHIFT)
#define NOTIFICATION_CONFIGURARION_PROPERTY_BASE \
                                    RESERVED_PROPERTIES_BASE + (7<<PROPERTY_USE_CASE_ID_SHIFT)
#define ACTION_PROPERTY_BASE        RESERVED_PROPERTIES_BASE + (8<<PROPERTY_USE_CASE_ID_SHIFT)
static const am_CustomSystemPropertyType_t SYP_GLOBAL_LOG_THRESHOLD = GLOBAL_PROPERTY_BASE + 0;
static const am_CustomSystemPropertyType_t SYP_REGISTRATION_ALLOW_UNKNOWN_ELEMENT = \
                                                                  REGISTRATION_PROPERTY_BASE + 0;
static const am_CustomSystemPropertyType_t SYP_REGISTRATION_DOMAIN_TIMEOUT = \
                                                        REGISTRATION_PROPERTY_BASE + 1;
static const am_CustomSystemPropertyType_t SYP_CONNECTION_ALLOW_ONLY_TOPOLOGY_ROUTES = \
                                                                  CONNECTION_PROPERTY_BASE + 0;

static const am_CustomClassProperty_t      CP_PER_SINK_CLASS_VOLUME_SUPPORT = \
                                                GLOBAL_PROPERTY_BASE + 0;
#define MSP_SINK_PER_CLASS_VOLUME_TYPE(CLASS_ID) VOLUME_PROPERTY_BASE - CLASS_ID_BASE + CLASS_ID
enum gc_SetSourceStateDirection_e
{
    SD_MAINSOURCE_TO_MAINSINK,
    SD_MAINSINK_TO_MAINSOURCE
};

enum gc_ActionList_e
{
    AL_NONE,
    // append at the end of Queue
    AL_NORMAL,
    // append after the currently executing action
    AL_SYSTEM

};
// For class Topology
enum gc_ClassTopologyCodeID_e
{
    MC_GENERAL_ELEMENT = 0,
    MC_SINK_ELEMENT,
    MC_SOURCE_ELEMENT,
    MC_GATEWAY_ELEMENT,
    MC_EQUAL_CODE,
    MC_EXCLUSIVE_CODE,
    MC_SHARED_CODE,
    MC_LBRACKET_CODE,
    MC_RBRACKET_CODE,
    MC_NULL_CODE
};

enum gc_Class_e
{
    C_UNKNOWN = 0,
    C_PLAYBACK,
    C_CAPTURE,
    C_MAX
};

enum gc_Trigger_e
{
    TRIGGER_UNKNOWN,
    USER_CONNECTION_REQUEST,
    USER_DISCONNECTION_REQUEST,
    USER_SET_SINK_MUTE_STATE,
    USER_SET_VOLUME,
    USER_SET_SINK_MAIN_SOUND_PROPERTY,
    USER_SET_SOURCE_MAIN_SOUND_PROPERTY,
    USER_SET_SYSTEM_PROPERTY,
    USER_SET_SINK_MAIN_NOTIFICATION_CONFIGURATION,
    USER_SET_SOURCE_MAIN_NOTIFICATION_CONFIGURATION,
    SYSTEM_SOURCE_AVAILABILITY_CHANGED,
    SYSTEM_SINK_AVAILABILITY_CHANGED,
    SYSTEM_INTERRUPT_STATE_CHANGED,
    SYSTEM_SINK_MUTE_STATE_CHANGED,
    SYSTEM_SINK_MAIN_SOUND_PROPERTY_CHANGED,
    SYSTEM_SOURCE_MAIN_SOUND_PROPERTY_CHANGED,
    SYSTEM_VOLUME_CHANGED,
    SYSTEM_SINK_NOTIFICATION_DATA_CHANGED,
    SYSTEM_SOURCE_NOTIFICATION_DATA_CHANGED,
    SYSTEM_REGISTER_DOMAIN,
    SYSTEM_REGISTER_SOURCE,
    SYSTEM_REGISTER_SINK,
    SYSTEM_REGISTER_GATEWAY,
    SYSTEM_DEREGISTER_DOMAIN,
    SYSTEM_DEREGISTER_SOURCE,
    SYSTEM_DEREGISTER_SINK,
    SYSTEM_DEREGISTER_GATEWAY,
    SYSTEM_DOMAIN_REGISTRATION_COMPLETE,
    SYSTEM_CONNECTION_STATE_CHANGE,
    SYSTEM_STORED_SINK_VOLUME,
    SYSTEM_ALL_DOMAIN_REGISTRATION_COMPLETE,
    USER_ALL_TRIGGER,
    TRIGGER_MAX
};

enum gc_Element_e
{
    ET_UNKNOWN,
    ET_SOURCE,
    ET_SINK,
    ET_ROUTE,
    ET_DOMAIN,
    ET_CLASS,
    ET_CONNECTION,
    ET_GATEWAY,
    ET_SYSTEM,
    ET_MAX
};

enum gc_Registration_e
{
    REG_NONE,
    REG_CONTROLLER,
    REG_ROUTER,
    REG_TEMPLATE
};

enum gc_LimitState_e
{
    LS_UNKNWON,
    LS_LIMITED,
    LS_UNLIMITED,
    LS_MAX
};

enum gc_Order_e
{
    O_HIGH_PRIORITY,
    O_LOW_PRIORITY,
    O_NEWEST,
    O_OLDEST
};

enum gc_LimitType_e
{
    LT_UNKNOWN,
    LT_RELATIVE,
    LT_ABSOLUTE,
    LT_MAX
};

struct gc_LimitVolume_s
{
    gc_LimitType_e limitType;
    am_volume_t limitVolume;
};

struct gc_volume_s
{
    bool           isvolumeSet;
    am_volume_t    volume;
    bool           isOffsetSet;
    am_volume_t    offsetVolume;
};
enum gc_MSPMappingDirection_e
{
    MD_MSP_TO_SP,
    MD_SP_TO_MSP,
    MD_BOTH
};

enum gc_Element_Status_e
{
    UNKNOWN,
    ROUTE_DISCONNECT,
    MAINCONNECTION_DISCONNECT,
    MAX
};

struct gc_System_s
{
    bool readOnly;
    std::string name;
    std::vector<am_SystemProperty_s > listSystemProperties;
};

struct gc_Route_s : public am_Route_s
{
    std::string name;
};

struct gc_RoutingElement_s : public am_RoutingElement_s
{
    std::string name;
};

struct gc_TopologyElement_s
{
    std::string name;
    gc_ClassTopologyCodeID_e codeID;
    gc_TopologyElement_s(gc_ClassTopologyCodeID_e elementCode, std::string elementName = std::string(""));
    gc_TopologyElement_s( gc_ClassTopologyCodeID_e elementCode,
                          std::string elementName, size_t pos,
                          size_t length = std::string::npos);
    gc_TopologyElement_s(std::string elementName);
};

struct gc_SoundProperty_s : public am_SoundProperty_s
{
    int16_t minValue;
    int16_t maxValue;
};

struct gc_MainSoundProperty_s : public am_MainSoundProperty_s
{
    int16_t minValue;
    int16_t maxValue;
};

struct gc_Source_s : public am_Source_s
{
    std::string domainName;
    std::string className;
    gc_Registration_e registrationType;
    bool isVolumeChangeSupported;
    bool isPersistencySupported;
    bool isVolumePersistencySupported;
    int32_t priority;
    std::vector<gc_MainSoundProperty_s > listGCMainSoundProperties;
    std::vector<gc_SoundProperty_s > listGCSoundProperties;
    std::map<gc_MSPMappingDirection_e, std::map<uint16_t, uint16_t > > mapMSPTOSP;
    am_volume_t minVolume;
    am_volume_t maxVolume;
    std::map<float, float > mapSourceVolume;
};

struct gc_Sink_s : public am_Sink_s
{
    std::string domainName;
    std::string className;
    gc_Registration_e registrationType;
    bool isVolumeChangeSupported;
    bool isPersistencySupported;
    bool isVolumePersistencySupported;
    int32_t priority;
    std::vector<gc_MainSoundProperty_s > listGCMainSoundProperties;
    std::vector<gc_SoundProperty_s > listGCSoundProperties;
    std::map<int16_t, float > mapUserVolumeToNormalizedVolume;
    std::map<float, float > mapNormalizedVolumeToDecibelVolume;
    std::map<gc_MSPMappingDirection_e, std::map<uint16_t, uint16_t > > mapMSPTOSP;
};

struct gc_Gateway_s : public am_Gateway_s
{
    std::string sinkName;
    std::string sourceName;
    std::string controlDomainName;
    gc_Registration_e registrationType;
    std::vector<std::pair<am_CustomConnectionFormat_t, am_CustomConnectionFormat_t > > listConvertionmatrix;
};

struct gc_Domain_s : public am_Domain_s
{
    gc_Registration_e registrationType;
};

struct gc_Class_s
{
    uint16_t    classID;
    std::string name;
    gc_Class_e type;
    int32_t priority;
    std::vector<std::vector<gc_TopologyElement_s > > listTopologies;
    gc_Registration_e registrationType;
    std::vector<am_ClassProperty_s> listClassProperties;
    bool isVolumePersistencySupported;
    am_volume_t defaultVolume;
};

struct gc_Action_s
{
    std::string actionName;
    std::map<std::string, std::string > mapParameters;
};

struct gc_ConnectionInfo_s
{
    std::string sourceName;
    std::string sinkName;
    std::string connectionName;
    int32_t priority;
    am_ConnectionState_e connectionState;
    am_volume_t volume;
    gc_Element_e elementType;
    gc_ConnectionInfo_s() {}
    gc_ConnectionInfo_s(const std::string &source, const std::string &sink)
    {
        sourceName = source;
        sinkName = sink;
        connectionName = source + ":" + sink;
    }
    gc_ConnectionInfo_s(const gc_Element_e &element, const std::string &name)
    {
        elementType = element;
        switch(elementType)
        {
            case ET_SOURCE:
                sourceName = name;
                break;
            case ET_SINK:
                sinkName = name;
                break;
            case ET_CONNECTION:
                connectionName = name;
                break;
            default:
                break;
        }
    }
    bool operator()(const gc_ConnectionInfo_s & elem) const
    {
        switch(elementType)
        {
            case ET_SOURCE:
                return (sourceName == elem.sourceName);
                break;
            case ET_SINK:
                return (sinkName == elem.sinkName);
                break;
            case ET_CONNECTION:
                return (connectionName == elem.sourceName + ":" + elem.sinkName);
                break;
            default:
                return false;
                break;
        }
    }

};
struct gc_ElementTypeName_s
{

public:
    /**
     * the element type
     */
    gc_Element_e elementType;

    /**
     * the element name
     */
    std::string elementName;

};

struct gc_ElementTypeID_s
{

public:
    /**
     * the element type
     */
    gc_Element_e elementType;
    /**
     * the element name
     */
    uint16_t elementID;

};

struct gc_LastMainConnections_s
{
    public:
    std::string className;
    std::set<std::string> listMainConnections;
};

struct gc_SinkVolume_s
{
    public:
    std::string sinkName;
    am_mainVolume_t mainVolume;
};

struct gc_LastClassVolume_s
{
    public:
    std::string className;
    std::vector<gc_SinkVolume_s> listSinkVolume;
};

struct gc_LastMainConVolInfo_s
{
    public:
    std::string mainConnectionName;
    am_mainVolume_t mainVolume;
};

struct gc_LastMainConnectionsVolume_s
{
    public:
    std::string className;
    std::vector<gc_LastMainConVolInfo_s> listLastMainConVolInfo;
};

} /* namespace gc */
} /* namespace am */

#endif /* GC_TYPES_H_ */
