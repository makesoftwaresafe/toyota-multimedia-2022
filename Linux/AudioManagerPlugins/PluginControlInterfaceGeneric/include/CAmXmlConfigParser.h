/******************************************************************************
 * @file: CAmXmlConfigParser.h
 *
 * This file contains the declaration of xml configuration parser.
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

#ifndef GC_XMLCONFIGPARSER_H_
#define GC_XMLCONFIGPARSER_H_

#include <libxml/parser.h>
#include "CAmTypes.h"

namespace am {
namespace gc {

#ifndef GENERIC_CONTROLLER_DEFAULT_CONF_ROOT
#define GENERIC_CONTROLLER_DEFAULT_CONF_ROOT "/etc/audiomanager/controller"
#endif

#define DEFAULT_USER_CONFIGURATION_PATH   GENERIC_CONTROLLER_DEFAULT_CONF_ROOT"/generic.xml"
#define DEFAULT_SCHEMA_PATH               GENERIC_CONTROLLER_DEFAULT_CONF_ROOT"/audiomanagertypes.xsd"
#define DEFAULT_CUSTOM_SCHEMA_PATH        GENERIC_CONTROLLER_DEFAULT_CONF_ROOT"/customtypes.xsd"
#define CONFIGURATION_FILE_ENV_VARNAME    "GENERIC_CONTROLLER_CONFIGURATION"

// sysmbols used in topology to seperate elements
#define TOPOLOGY_SYMBOL_CARET                       "^"
#define TOPOLOGY_SYMBOL_EQUAL                       "="
#define TOPOLOGY_SYMBOL_RIGHT_BRACKET               ")"
#define TOPOLOGY_SYMBOL_LEFT_BRACKET                "("
#define TOPOLOGY_SYMBOL_WHITESPACE                  " "
#define TOPOLOGY_SYMBOL_ASTERISK                    "*"

// sets of elements of configuration
#define CONFIG_ELEMENT_SET_DOMAIN                   "listDomains"
#define CONFIG_ELEMENT_SET_SOURCE                   "listSources"
#define CONFIG_ELEMENT_SET_SINK                     "listSinks"
#define CONFIG_ELEMENT_SET_GATEWAY                  "listGateways"
#define CONFIG_ELEMENT_SET_CLASS                    "listClasses"
#define CONFIG_ELEMENT_SET_POLICY                   "listPolicies"
#define CONFIG_ELEMENT_SET_SYSTEM_PROPERTY          "listSystemProperties"

//tags used by policyset in configuration
#define POLICY_TAG_COMMENT                          "name"
#define POLICY_TAG_TRIGGER                          "trigger"
#define POLICY_TAG_STOP_EVALUATION                  "break"
#define POLICY_TAG_CONDITION                        "condition"
#define POLICY_TAG_ACTION                           "action"
#define POLICY_TAG_PRIORITY                         "priority"
#define POLICY_TAG_LIST_PROCESS                     "process"

// tags used by classset in configuration
#define CLASS_TAG_CLASS_NAME                        "name"
#define CLASS_TAG_TYPE                              "type"
#define CLASS_TAG_TOPOLOGY_TAG                      "topology"
#define CLASS_TAG_PROPERTIES_TAG                    "classProperty"
#define CLASS_TAG_DEFAULT_VOLUME                    "defaultVolume"

// tags used by domainset in configuration
#define DOMAIN_TAG_BUS_NAME                         "busName"
#define DOMAIN_TAG_NODE_NAME                        "nodeName"
#define DOMAIN_TAG_EARLY                            "early"
#define DOMAIN_TAG_COMPLETE                         "complete"
#define DOMAIN_TAG_STATE                            "state"

// tags used by gatewayset in configuration
#define GATEWAY_TAG_SINK_NAME                       "sinkName"
#define GATEWAY_TAG_SOURCE_NAME                     "sourceName"
#define GATEWAY_TAG_CONTROL_DOMAIN_NAME             "controlDomainName"
#define GATEWAY_TAG_CONVERSION_MATRIX               "conversionMatrix"

// tags used by sinkset in configuration
#define SINK_TAG_MAIN_VOLUME                        "mainVolume"
#define SINK_TAG_MUTE_STATE                         "muteState"

// tags used by sourceset in configuration
#define SOURCE_TAG_SOURCE_STATE                     "sourceState"
#define SOURCE_TAG_INTERRUPT_STATE                  "interruptState"

#define SOURCE_TAG_VOLUME_MAP                       "listSourceVolumeMap"
//tags used by MSP SP maps
#define CONFIG_ELEMENT_SET_MSP_SP_MAP               "listMainSoundPropertiesToSoundProperties"
#define MSPSP_MAP_TAG_TYPE                          "mapEntry"
#define MSPSP_MAP_ATTRIBUTE_TYPE                    "mappingDirection"

// tags used by main sound and sound propertyset in configuration
#define SOUND_PROPERTY_TAG_TYPE                     "type"
#define SOUND_PROPERTY_TAG_VALUE                    "value"
#define SOUND_PROPERTY_TAG_MIN_VALUE                "minValue"
#define SOUND_PROPERTY_TAG_MAX_VALUE                "maxValue"

// tags used by availability set in configuration
#define AVAILABILITY_TAG_AVAILABLE                  "availability"
#define AVAILABILITY_TAG_REASON                     "availabilityReason"

//tags used by source/sink/class element set
#define COMMON_TAG_VOLUME_USER_TO_NORM              "listMainVolumesToNormalizedVolumes"
#define COMMON_TAG_VOLUME_NORM_TO_USER              "listNormalizedVolumesToDecibelVolumes"
#define COMMON_TAG_TABLE_ENTRY                      "tableEntry"
#define COMMON_TAG_PRIORITY                         "priority"

// tags used by source/sink element set
#define ELEMENT_COMMON_TAG_DOMAIN_NAME              "domainName"
#define ELEMENT_COMMON_TAG_CLASS_NAME               "className"
#define ELEMENT_COMMON_TAG_AVAILABLE                "availability"
#define ELEMENT_COMMON_TAG_IS_VOLUME_SUPPORTED      "isVolumeSupported"
#define ELEMENT_COMMON_TAG_IS_PERSISTENCY_SUPPORTED "isPersistencySupported"
#define ELEMENT_COMMON_TAG_IS_VOLUME_PERSISTENCY_SUPPORTED "isVolumePersistencySupported"
#define ELEMENT_COMMON_TAG_VOLUME                   "volume"
#define ELEMENT_COMMON_TAG_MIN_VOLUME               "minVolume"
#define ELEMENT_COMMON_TAG_MAX_VOLUME               "maxVolume"
#define ELEMENT_COMMON_TAG_VISIBLE                  "visible"

// tags used by source/sink/domain/gateway/class element set
#define ELEMENT_COMMON_TAG_REGISTRATION_TYPE        "registrationType"
#define ELEMENT_COMMON_TAG_ID                       "id"
#define ELEMENT_COMMON_TAG_NAME                     "name"
#define ELEMENT_ATTRIBUTE_BASENAME                  "baseName"

//tags used by action set in configuration
#define ACTION_SET_TAG_TYPE                         "type"
#define ACTION_SET_TAG_PARAM                        "param"

//tags used by system property set in configuration
#define SYSTEM_PROPERTY_TAG_TYPE                    "type"
#define SYSTEM_PROPERTY_TAG_VALUE                   "value"

//tags used by notification set in configuration
#define NOTIFICATION_TAG_TYPE                       "type"
#define NOTIFICATION_TAG_STATUS                     "status"
#define NOTIFICATION_TAG_PARAM                      "parameter"

//tags used by enumeration set in configuration
#define ENUM_TAG_ENUMERATION                        "enumeration"
#define ENUM_TAG_VALUE                              "value"

//main tags whose child parameters will be stored
#define XML_ELEMENT_TAG_CONNECTION_FORMAT           "connectionFormat"
#define XML_ELEMENT_TAG_SIMPLE_TYPE                 "simpleType"
#define XML_ELEMENT_TAG_ANNOTATION                  "annotation"
#define XML_ELEMENT_TAG_RESTRICTION                 "restriction"
#define XML_ELEMENT_TAG_NOTIFICATION_CONFIG         "notificationConfiguration"
#define XML_ELEMENT_TAG_MAIN_NOTIFICATION_CONFIG    "mainNotificationConfiguration"
#define XML_ELEMENT_TAG_SOUND_PROPERTY              "soundProperty"
#define XML_ELEMENT_TAG_MAIN_SOUND_PROPERTY         "mainSoundProperty"
#define XML_ELEMENT_TAG_SYSTEM_PROPERTY             "systemProperty"
#define XML_ELEMENT_TAG_CONFIG                      "configuration"
#define XML_ELEMENT_TAG_SOURCE                      "source"
#define XML_ELEMENT_TAG_SINK                        "sink"
#define XML_ELEMENT_TAG_GATEWAY                     "gateway"
#define XML_ELEMENT_TAG_DOMAIN                      "domain"
#define XML_ELEMENT_TAG_CLASS                       "class"
#define XML_ELEMENT_TAG_POLICY                      "policy"
#define XML_ELEMENT_TAG_SCHEMA                      "schema"
#define XML_ELEMENT_TAG_SIMPLE_TYPE_NAME            "name"

//extra symbol returned by xmllibrary during parsing
#define XML_ILLEGAL_SYMBOL                          "text"

#define DEFAULT_CONFIG_PARSED_POLICY_PRIORITY 49
#define DEFAULT_CONFIG_ELEMENT_PRIORITY 50
#define DEFAULT_CONFIG_CLASS_PRIORITY       99

// functions supported by configuration conditions set
#define FUNCTION_NAME 						"name"
#define FUNCTION_PRIORITY					"priority"
#define FUNCTION_CONNECTION_STATE 			"connectionState"
#define FUNCTION_VOLUME 					"volume"
#define FUNCTION_MAIN_VOLUME 			    "mainVolume"
#define FUNCTION_SOUND_PROPERTY 			"soundProperty"
#define FUNCTION_MAIN_SOUND_PROPERTY_TYPE 	"mainSoundPropertyType"
#define FUNCTION_MAIN_SOUND_PROPERTY_VALUE  "mainSoundPropertyValue"
#define FUNCTION_SYSTEM_PROPERTY_VALUE      "systemPropertyValue"
#define FUNCTION_SYSTEM_PROPERTY_TYPE		"systemPropertyType"
#define FUNCTION_MUTE_STATE					"muteState"
#define FUNCTION_IS_REGISTRATION_COMPLETE	"isRegistrationComplete"
#define FUNCTION_AVAILABILITY				"availability"
#define FUNCTION_AVAILABILITY_REASON		"availabilityReason"
#define FUNCTION_INTERRUPT_STATE			"interruptState"
#define FUNCTION_IS_REGISTERED				"isRegistered"
#define FUNCTION_STATE						"state"
#define FUNCTION_DOMAIN_STATE				"domainState"
#define FUNCTION_SOURCE_STATE				"sourceState"
#define FUNCTION_CONNECTION_FORMAT			"connectionFormat"
#define FUNCTION_ERROR                      "error"
#define FUNCTION_NOTIFICATION_CONFIGURATION_STATUS  "notificationConfigurationStatus"
#define FUNCTION_NOTIFICATION_CONFIGURATION_PARAM   "notificationConfigurationParam"
#define FUNCTION_NOTIFICATION_DATA_VALUE            "notificationDataValue"
#define FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_TYPE    "mainNotificationConfigurationType"
#define FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_STATUS  "mainNotificationConfigurationStatus"
#define FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_PARAM   "mainNotificationConfigurationParam"
#define FUNCTION_PEEK                       "peek"

// macros supported in functions
#define FUNCTION_MACRO_SUPPORTED_REQUESTING	"REQUESTING"
#define FUNCTION_MACRO_SUPPORTED_ALL		"ALL"
#define FUNCTION_MACRO_SUPPORTED_OTHERS		"OTHERS"

//category by which functions can be separated and evaluated
#define CATEGORY_SINK					"SINK"
#define CATEGORY_SOURCE					"SOURCE"
#define CATEGORY_CLASS					"CLASS"
#define CATEGORY_CONNECTION				"CONNECTION"
#define CATEGORY_DOMAIN					"DOMAIN"
#define CATEGORY_DOMAIN_OF_SOURCE		"DOMAINOFSOURCE"
#define CATEGORY_DOMAIN_OF_SINK			"DOMAINOFSINK"
#define CATEGORY_CLASS_OF_SOURCE		"CLASSOFSOURCE"
#define CATEGORY_CLASS_OF_SINK			"CLASSOFSINK"
#define CATEGORY_SOURCE_OF_CLASS		"SOURCEOFCLASS"
#define CATEGORY_SINK_OF_CLASS			"SINKOFCLASS"
#define CATEGORY_CONNECTION_OF_CLASS	"CONNECTIONOFCLASS"
#define CATEGORY_CONNECTION_OF_SOURCE	"CONNECTIONOFSOURCE"
#define CATEGORY_CONNECTION_OF_SINK		"CONNECTIONOFSINK"
#define CATEGORY_SYSTEM					"SYSTEM"
#define CATEGORY_USER                   "USER"

#define CONFIG_ACTION_NAME_CONNECT             "ACTION_CONNECT"
#define CONFIG_ACTION_NAME_DISCONNECT          "ACTION_DISCONNECT"
#define CONFIG_ACTION_NAME_PUSH                "ACTION_PUSH"
#define CONFIG_ACTION_NAME_POP                 "ACTION_POP"
#define CONFIG_ACTION_NAME_SUSPEND             "ACTION_SUSPEND"
#define CONFIG_ACTION_NAME_RESUME              "ACTION_RESUME"
#define CONFIG_ACTION_NAME_LIMIT               "ACTION_LIMIT"
#define CONFIG_ACTION_NAME_UNLIMIT             "ACTION_UNLIMIT"
#define CONFIG_ACTION_NAME_MUTE                "ACTION_MUTE"
#define CONFIG_ACTION_NAME_UNMUTE              "ACTION_UNMUTE"
#define CONFIG_ACTION_NAME_SET_VOLUME          "ACTION_SET_VOLUME"
#define CONFIG_ACTION_NAME_SET_PROPERTY        "ACTION_SET_PROPERTY"
#define CONFIG_ACTION_NAME_REGISTER            "ACTION_REGISTER"
#define CONFIG_ACTION_NAME_DEBUG               "ACTION_DEBUG"
#define CONFIG_ACTION_NAME_NOTIFICATION_CONFIGURATION    "ACTION_SET_NOTIFICATION_CONFIGURATION"

// store the function and its parameter as given in policy
struct gc_FunctionElement_s
{
    std::string functionName;
    std::string category;
    std::string mandatoryParameter;
    std::string optionalParameter;
    std::string optionalParameter2;
    bool isValueMacro;
};

// store the RHS side of condition as given in policy
struct gc_RHSElement_s
{
    bool isValue;
    bool isMacro;
    gc_FunctionElement_s functionObject;
    std::string directValue;
};

// store the condition as given in policy
struct gc_ConditionStruct_s
{
    gc_FunctionElement_s leftObject;
    std::string operation;
    gc_RHSElement_s rightObject;
};

struct gc_Process_s
{
    std::string comment;
    int32_t priority;
    bool stopEvaluation;
    std::vector<gc_ConditionStruct_s > listConditions;
    std::vector<gc_Action_s > listActions;
};

struct gc_Policy_s
{
    std::vector<int > listEvents;
    std::vector<gc_Process_s > listProcesses;
};

struct gc_Configuration_s
{
    std::vector<gc_Source_s > listSources;
    std::vector<gc_Sink_s > listSinks;
    std::vector<gc_Gateway_s > listGateways;
    std::vector<gc_Domain_s > listDomains;
    std::vector<gc_Class_s > listClasses;
    std::vector<gc_Policy_s > listPolicies;
    std::vector<am_SystemProperty_s > listSystemProperties;
};

class IAmXmlNode
{
public:
    static std::map<std::string, int > *mpMapEnumerations;
    static gc_Configuration_s* mpConfiguration;
    virtual ~IAmXmlNode()
    {
    }
    virtual int parse(xmlDocPtr pDocument, xmlNodePtr* pCurrent)=0;
};

class CAmXmlConfigParser
{
public:
    /**
     * @brief It is the constructor of config reader class. It validates the user configuration
     * against the schema and the parse the default and user configuration.
     * @param pParser: pointer to condition parser class instance
     * @return none
     */
    CAmXmlConfigParser(void);
    ~CAmXmlConfigParser(void);
    am_Error_e parse(gc_Configuration_s* pConfiguration);
private:
    /**
     * @brief It is the internal function use to validate the user configuration against the schema
     * @param XMLFilename: user configuration file name
     *        XSDFilename: schema name
     * @return E_UNKNOWN on error
     *         E_OK on success
     */
    am_Error_e _validateConfiguration(const std::string& XMLFilename,
                                      const std::string& XSDFilename);
    /**
     * @brief It is the internal function use to parse the schema.
     * @param XSDFilename: schema file name
     * @return E_UNKNOWN on error
     *         E_OK on success
     */
    am_Error_e _parseXSDFile(const std::string& XSDFilename);
    /**
     * @brief It is the internal function use to parse the simple types as defined in schema.
     * @param pDocument: document pointer
     *        pCurrent: current node to be parsed
     * @return none
     */
    void _parseSimpleType(const xmlDocPtr pDocument, xmlNodePtr pCurrent);
    /**
     * @brief It is the internal function use to parse the enum initializer values as defined in schema.
     * @param pDocument: document pointer
     *        pCurrent: current node to be parsed
     *        value: value of enum
     * @return E_UNKNOWN on error
     *         E_OK on success
     */
    am_Error_e _parseEnumInitialiser(const xmlDocPtr pDocument, xmlNodePtr pCurrent, int& value);
    /**
     * @brief It is the internal function use to parse the enum as defined in schema.
     * @param pDocument: document pointer
     *        pCurrent: current node to be parsed
     *        Name: name of enum
     * @return none
     */
    void _parseEnumeration(const xmlDocPtr pDocument, xmlNodePtr pCurrent);

    /**
     * @brief It is the internal function use to parse the default and user configuration common elements
     * @param XMLFilename: file to be parsed
     * @return E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    am_Error_e _parseConfiguration(const std::string& XMLFilename,
                                   gc_Configuration_s* pConfiguration);

    // map to store the value of enumeration as given in schema
    std::map<std::string, int > mMapEnumerations;

#if AMCO_DEBUGGING
    void printAllEnums(void);
    void printListGateways(gc_Configuration_s* pConfiguration);
    void printListDomains(gc_Configuration_s* pConfiguration);
    void printListClasses(gc_Configuration_s* pConfiguration);
    void printListPolicies(gc_Configuration_s* pConfiguration);
    void printListSinks(gc_Configuration_s* pConfiguration);
    void printListSources(gc_Configuration_s* pConfiguration);
    void printListSystemProperties(gc_Configuration_s* pConfiguration);
#endif
};

}/* namespace gc */
} /* namespace am */
#endif /* GC_XMLCONFIGPARSER_H_ */
