/******************************************************************************
 * @file: CAmPolicyEngine.h
 *
 * This file contains the declaration of policy engine class (member functions
 * and data members) used to provide the interface to get the actions related
 * to the trigger/hook from framework side
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

#ifndef GC_POLICYENGINE_H_
#define GC_POLICYENGINE_H_

#include <string>
#include <vector>
#include <map>
#include "CAmConfigurationReader.h"
#include "CAmTypes.h"

namespace am {
namespace gc {

#define SEARCH_STATIC_SOURCE    0x01
#define SEARCH_STATIC_SINK      0x02
#define SEARCH_STATIC_GATEWAY   0x04
#define SIZE_OF_REQUESTING_MACRO   10
enum gc_Operator_e
{
    EQ,
    GT,
    GEQ,
    LT,
    LEQ,
    NE,
    INC,
    EXC
};

struct gc_triggerParams_s
{
    gc_triggerParams_s()
    {
        triggerType = TRIGGER_UNKNOWN;
        sinkName.clear();
        sourceName.clear();
        domainName.clear();
        gatewayName.clear();
        className.clear();
        connectionName.clear();
        mainVolume = 0;
        status = E_OK;
        connectionState = CS_UNKNOWN;
        muteState = MS_UNKNOWN;
        interruptState = IS_UNKNOWN;
        mainSoundProperty.type = MSP_UNKNOWN;
        mainSoundProperty.value = 0;
        systemProperty.type = SYP_UNKNOWN;
        systemProperty.value = 0;
        availability.availability = A_UNKNOWN;
        availability.availabilityReason = AR_UNKNOWN;
        notificatonPayload.type = NT_UNKNOWN;
        notificatonPayload.value = 0;
        notificatonConfiguration.type = NT_UNKNOWN;
        notificatonConfiguration.status = NS_UNKNOWN;
        notificatonConfiguration.parameter = 0;
    }
    gc_Trigger_e triggerType;
    std::string sinkName;
    std::string sourceName;
    std::string domainName;
    std::string gatewayName;
    std::string className;
    std::string connectionName;
    am_ConnectionState_e connectionState;
    am_Error_e status;
    am_mainVolume_t mainVolume;
    am_MainSoundProperty_s mainSoundProperty;
    am_SystemProperty_s systemProperty;
    am_Availability_s availability;
    am_MuteState_e muteState;
    am_InterruptState_e interruptState;
    am_NotificationPayload_s notificatonPayload;
    am_NotificationConfiguration_s notificatonConfiguration;
};
class IAmPolicyReceive;
class CAmPolicyEngine
{
public:
    /**
     * @brief It is the constructor of policy Engine class. Initialize the member
     * variables with default value.
     * @param none
     * @return none
     */
    CAmPolicyEngine();
    /**
     * @brief It is the destructor of policy Engine class.
     * @param none
     * @return none
     */
    ~CAmPolicyEngine()
    {
    }
    /**
     * @brief It is API to start the policy engine. It gets the list of system properties from
     * configuration and give request to framework to register the properties with AM.
     * It also give request to register the classes to AM
     * @param pPolicyReceive: pointer to policy receive class instance
     * @return E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    am_Error_e startPolicyEngine(IAmPolicyReceive* pPolicyReceive);

    /**
     * @brief It is the template function used to find whether element registration
     * is allowed or not as per configuration file
     * @param elementName: name of element to be checked
     *        elementType: type of element
     *        elementInstance: information of element
     * @return E_OK on success
     *         E_UNKNOWN on error
     */
    template <typename Telement>
    am_Error_e getElement(const std::string& elementName, Telement& elementInstance)
    {
        return mpConfigReader->getElementByName(elementName, elementInstance);
    }
    am_Error_e processTrigger(gc_triggerParams_s& triggerParams);

    am_Error_e getListSystemProperties(std::vector<am_SystemProperty_s >& listSystemProperties);
    am_Error_e getListClasses(std::vector<gc_Class_s >& listClasses);
    am_Error_e getListDomains(std::vector<gc_Domain_s >& listDomains);
private:
    bool _isConditionTrue(const gc_ConditionStruct_s& Condition,
                          const gc_triggerParams_s& parameters);
    am_Error_e _evaluateConditionSet(const std::vector<gc_ConditionStruct_s >& listConditionSets,
                                     const gc_triggerParams_s& parameters);
    bool _getActionsfromPolicy(const gc_Process_s& process, std::vector<gc_Action_s >& listActions,
                               const gc_triggerParams_s& parameters);
    template <typename Toperand>
    bool _conditionResult(Toperand LHSData, gc_Operator_e opType, Toperand RHSData);
    void _convertActionParamToValue(std::vector<gc_Action_s >& listActions,
                                    const gc_triggerParams_s& parameters);

    //function pointer for function used in policy
    am_Error_e _findElementPeek(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const std::string clasName,
                                                const bool isLHS,const bool isSinkRequired);
    am_Error_e _findSourcePeek(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSinkPeek(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sink name found
     *         E_UNKNOWN if sink name not found
     */
    am_Error_e _findSinkName(const gc_ConditionStruct_s &conditionInstance,
                             std::vector<std::string > &listOutputs,
                             const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source name found
     *         E_UNKNOWN if source name not found
     */
    am_Error_e _findSourceName(const gc_ConditionStruct_s &conditionInstance,
                               std::vector<std::string > &listOutputs,
                               const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if domain name found
     *         E_UNKNOWN if domain name not found
     */
    am_Error_e _findDomainName(const gc_ConditionStruct_s &conditionInstance,
                               std::vector<std::string > &listOutputs,
                               const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the class name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if class name found
     *         E_UNKNOWN if class name not found
     */
    am_Error_e _findClassName(const gc_ConditionStruct_s &conditionInstance,
                              std::vector<std::string > &listOutputs,
                              const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection name found
     *         E_UNKNOWN if connection name not found
     */
    am_Error_e _findConnectionName(const gc_ConditionStruct_s &conditionInstance,
                                   std::vector<std::string > &listOutputs,
                                   const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain name of source as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if domain name of source found
     *         E_UNKNOWN if domain name of source not found
     */
    am_Error_e _findDomainOfSourceName(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain name of sink as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if domain name of sink found
     *         E_UNKNOWN if domain name of sink not found
     */
    am_Error_e _findDomainOfSinkName(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the class name of sink as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if class name of sink found
     *         E_UNKNOWN if class name of sink not found
     */
    am_Error_e _findClassOfSinkName(const gc_ConditionStruct_s &conditionInstance,
                                    std::vector<std::string > &listOutputs,
                                    const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the class name of source as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if class name of source found
     *         E_UNKNOWN if class name of source not found
     */
    am_Error_e _findClassOfSourceName(const gc_ConditionStruct_s &conditionInstance,
                                      std::vector<std::string > &listOutputs,
                                      const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sinks of given class name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sinks of given class name found
     *         E_UNKNOWN if sinks of given class name not found
     */
    am_Error_e _findSinkOfClassName(const gc_ConditionStruct_s &conditionInstance,
                                    std::vector<std::string > &listOutputs,
                                    const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sources of given class name as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sources of given class name found
     *         E_UNKNOWN if sources of given class name not found
     */
    am_Error_e _findSourceOfClassName(const gc_ConditionStruct_s &conditionInstance,
                                      std::vector<std::string > &listOutputs,
                                      const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink priority as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sink priority found
     *         E_UNKNOWN if sink priority not found
     */
    am_Error_e _findSinkPriority(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source priority as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source priority found
     *         E_UNKNOWN if source priority not found
     */
    am_Error_e _findSourcePriority(const gc_ConditionStruct_s &conditionInstance,
                                   std::vector<std::string > &listOutputs,
                                   const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the class priority as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if class priority found
     *         E_UNKNOWN if class priority not found
     */
    am_Error_e _findClassPriority(const gc_ConditionStruct_s &conditionInstance,
                                  std::vector<std::string > &listOutputs,
                                  const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection priority as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection priority found
     *         E_UNKNOWN if connection priority not found
     */
    am_Error_e _findConnectionPriority(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the priority of all connections of given class
     * as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if priority found
     *         E_UNKNOWN if priority not found
     */
    am_Error_e _findConnectionOfClassPriority(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,
                                              const gc_triggerParams_s &parameters,
                                              const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection state of all connection in the
     * class as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection state found
     *         E_UNKNOWN if connection state not found
     */
    am_Error_e _findConnectionOfClassState(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection state of all connection which
     * involves given source.
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection found
     *         E_UNKNOWN if connection not found
     */
    am_Error_e _findSourceConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection state of all connection which
     * involves given sink.
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection found
     *         E_UNKNOWN if connection not found
     */
    am_Error_e _findSinkConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                        std::vector<std::string > &listOutputs,
                                        const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findUserConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                        std::vector<std::string > &listOutputs,
                                        const gc_triggerParams_s &parameters,const bool isLHS);
    am_Error_e _findConnectionConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters,const bool isLHS);

    /**
     * @brief It is the internal function used to find the volume of sink as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findSinkDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the volume of source as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findSourceDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the volume of connection as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findConnectionDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the volume of all the connections of given class as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findConnectionOfClassDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS);
    /**
     * @brief It is elementTypethe internal function used to find the sink user/main volume as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findSinkMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                   std::vector<std::string > &listOutputs,
                                   const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source user/main volume as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume can be retrieved
     *         E_UNKNOWN if volume cannot be retrieved
     */
    am_Error_e _findSourceMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
      * @brief It is the internal function used to find the main volume received in trigger
      * @param conditionInstance: condition which need to be evaluated
      *        listOutputs: list in which output will be returned
      *        parameters: parameters as received in hook on which this condition is getting executed
      *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
      * @return E_OK
      */
    am_Error_e _findUserMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                                      std::vector<std::string > &listOutputs,
                                                      const gc_triggerParams_s &parameters,
                                                      const bool isLHS);
    am_Error_e _findUserErrorValue(const gc_ConditionStruct_s &conditionInstance,
                                                          std::vector<std::string > &listOutputs,
                                                          const gc_triggerParams_s &parameters,
                                                          const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink device property value as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if device property value is found
     *         E_UNKNOWN if device property value is not found
     */
    am_Error_e _findSinkDevicePropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source device property value as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if device property value is found
     *         E_UNKNOWN if device property value is not found
     */
    am_Error_e _findSourceDevicePropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,
                                              const gc_triggerParams_s &parameters,
                                              const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink user property value as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if user property value is found
     *         E_UNKNOWN if user property value is not found
     */
    am_Error_e _findSinkUserPropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source user property value as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if user property value is found
     *         E_UNKNOWN if user property value is not found
     */
    am_Error_e _findSourceUserPropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the system property value as received in trigger
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK
     */
    am_Error_e _findUserSystemPropertyValue(
                    const gc_ConditionStruct_s &conditionInstance,
                    std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                    const bool isLHS);
    /**
     * @brief It is the internal function used to find the system property type as received in trigger
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK
     */
    am_Error_e _findUserSystemPropertyType(
                    const gc_ConditionStruct_s &conditionInstance,
                    std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                    const bool isLHS);
    /**
     * @brief It is the internal function used to find the main sound property value as received in trigger
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK
     */
    am_Error_e _findUserMainSoundPropertyValue(
                    const gc_ConditionStruct_s &conditionInstance,
                    std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                    const bool isLHS);
    /**
     * @brief It is the internal function used to find the main sound property type as received in trigger
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK
     */
    am_Error_e _findUserMainSoundPropertyType(
                    const gc_ConditionStruct_s &conditionInstance,
                    std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                    const bool isLHS);
    /**
     * @brief It is the internal function used to find the system property value as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if system property value is found
     *         E_UNKNOWN if system property value is not found
     */
    am_Error_e _findSystemPropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                        std::vector<std::string > &listOutputs,
                                        const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink mute state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if mute state found
     *         E_UNKNOWN if mute state not found
     */
    am_Error_e _findSinkMuteState(const gc_ConditionStruct_s &conditionInstance,
                                  std::vector<std::string > &listOutputs,
                                  const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the class mute state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if mute state found
     *         E_UNKNOWN if mute state not found
     */
    am_Error_e _findClassMuteState(const gc_ConditionStruct_s &conditionInstance,
                                   std::vector<std::string > &listOutputs,
                                   const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection mute state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if mute state found
     *         E_UNKNOWN if mute state not found
     */
    am_Error_e _findConnectionMuteState(const gc_ConditionStruct_s &conditionInstance,
                                        std::vector<std::string > &listOutputs,
                                        const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink availability as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sink availability found
     *         E_UNKNOWN if sink availability not found
     */
    am_Error_e _findSinkAvailability(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source availability as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source availability found
     *         E_UNKNOWN if source availability not found
     */
    am_Error_e _findSourceAvailability(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findUserAvailability(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink availability reason as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if sink availability reason found
     *         E_UNKNOWN if sink availability reason not found
     */
    am_Error_e _findSinkAvailabilityReason(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source availability reason as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source availability reason found
     *         E_UNKNOWN if source availability reason not found
     */
    am_Error_e _findSourceAvailabilityReason(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const gc_triggerParams_s &parameters,
                                             const bool isLHS);
    am_Error_e _findUserAvailabilityReason(const gc_ConditionStruct_s &conditionInstance,
                                                 std::vector<std::string > &listOutputs,
                                                 const gc_triggerParams_s &parameters,
                                                 const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection format as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection format found
     *         E_UNKNOWN if connection format not found
     */
    am_Error_e _findConnectionFormat(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the list of connection format of all the
     * connection of class as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if connection format found
     *         E_UNKNOWN if connection format not found
     */
    am_Error_e _findConnectionOfClassFormat(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source interrupt state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source interrupt state found
     *         E_UNKNOWN if source interrupt state not found
     */
    am_Error_e _findSourceInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                         std::vector<std::string > &listOutputs,
                                         const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the connection interrupt state as per current scenario and condition. Main source of connection is checked for interrupt state.
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source interrupt state found
     *         E_UNKNOWN if source interrupt state not found
     */
    am_Error_e _findConnectionInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const gc_triggerParams_s &parameters,
                                             const bool isLHS);
    am_Error_e _findUserInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                                 std::vector<std::string > &listOutputs,
                                                 const gc_triggerParams_s &parameters,
                                                 const bool isLHS);
    /**
     * @brief It is the internal function used to find the sink is registered or not as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if registered
     *         E_UNKNOWN if not registered
     */
    am_Error_e _findSinkIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the source is registered or not as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if registered
     *         E_UNKNOWN if not registered
     */
    am_Error_e _findSourceIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain is registered or not as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if registered
     *         E_UNKNOWN if not registered
     */
    am_Error_e _findDomainIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain registration complete status as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if registration complete
     *         E_UNKNOWN if registration not complete
     */
    am_Error_e _findIsDomainRegistrationComplete(const gc_ConditionStruct_s &conditionInstance,
                                                 std::vector<std::string > &listOutputs,
                                                 const gc_triggerParams_s &parameters,
                                                 const bool isLHS);
    /**
     * @brief It is the internal function used to find the source state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if source state found
     *         E_UNKNOWN if source state not found
     */
    am_Error_e _findSourceState(const gc_ConditionStruct_s &conditionInstance,
                                std::vector<std::string > &listOutputs,
                                const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the domain state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if domain state found
     *         E_UNKNOWN if domain state not found
     */
    am_Error_e _findDomainState(const gc_ConditionStruct_s &conditionInstance,
                                std::vector<std::string > &listOutputs,
                                const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the internal function used to find the element name
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element whose name is needed
     * @return E_OK if availability is found
     *         E_UNKNOWN if availability is not found
     */
    am_Error_e _findElementName(const gc_ConditionStruct_s &conditionInstance,
                                std::vector<std::string > &listOutputs, const std::string& name,
                                const bool isLHS);
    /**
     * @brief It is the internal function used to find the element availability and its reason as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: element name
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element whose availability information is needed
     *        isReasonRequired: to indicate that expected output is availability reason.
     * @return E_OK if availability is found
     *         E_UNKNOWN if availability is not found
     */
    am_Error_e _findAvailability(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 std::string& mandatoryParameter, const bool isLHS,
                                 gc_Element_e elementType, bool isReasonRequired);
    /**
     * @brief It is the internal function used to find the list of connections as per optional
     * parameter set in function of condition in policy section of configuration file
     * @param elementType: element type
     *        name: name of the element
     *        optional: optional parameter base on which connections will be filtered
     *        listConnectionInformation: list in which connection will be returned
     *        parameters: parameters as received in hook on which this condition is getting executed
     * @return E_OK if connections found
     *         E_UNKNOWN if connections not found
     */
    am_Error_e _getConnectionList(const gc_Element_e elementType, const std::string& elementName,
                                  const std::string& optionalParameter,
                                  std::vector<gc_ConnectionInfo_s > &listConnectionInfo,
                                  const gc_triggerParams_s &parameters) const;
    /**
     * @brief It is the supporting internal function used to find the value of mandatory and optional parameter of condition
     * @param conditionInstance: condition which need to be evaluated
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        mandatoryParam: variable in which mandatory parameter will be returned
     *        optionalParam: variable in which optional parameter will be returned
     * @return none
     */
    void _getValueOfParameter(const gc_ConditionStruct_s &conditionInstance, const bool isLHS,
                              std::string& mandatoryParam, std::string& optionalParam) const;
    /**
     * @brief It is the supporting internal function used to find the value of mandatory parameter of condition
     * @param conditionInstance: condition which need to be evaluated
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        mandatoryParam: variable in which mandatory parameter will be returned
     * @return none
     */
    void _getValueOfParameter(const gc_ConditionStruct_s &conditionInstance, const bool isLHS,
                              std::string& mandatoryParam) const;
    /**
     * @brief It is the supporting internal function used to find the user volume of element as per current scenario and condition
     * @param elementType: type of element
     *        conditionInstance: condition which need to be evaluated
     *        elementName: name of the element whose user volume is needed
     *        listOutputs: list in which output will be returned
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if user volume is retrieved
     *         E_UNKNOWN if user volume is not retrieved
     */
    am_Error_e _findMainVolume(const gc_Element_e elementType,
                               const gc_ConditionStruct_s &conditionInstance,
                               std::string& elementName, std::vector<std::string > &listOutputs,
                               const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find the volume of element as per current scenario and condition
     * @param elementType: type of element
     *        conditionInstance: condition which need to be evaluated
     *        elementName: name of the element whose user volume is needed
     *        listOutputs: list in which output will be returned
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if volume is retrieved
     *         E_UNKNOWN if volume is not retrieved
     */
    am_Error_e _findDeviceVolume(const gc_Element_e elementType,
                                 const gc_ConditionStruct_s &conditionInstance,
                                 std::string& elementName, std::vector<std::string > &listOutputs,
                                 const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find the device property value of element as per current scenario and condition
     * @param elementType: type of element
     *        conditionInstance: condition which need to be evaluated
     *        elementName: name of the element whose user volume is needed
     *        listOutputs: list in which output will be returned
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if device property value is found
     *         E_UNKNOWN if device property value is not found
     */
    am_Error_e _findDevicePropertyValue(const gc_Element_e elementType,
                                        const gc_ConditionStruct_s &conditionInstance,
                                        std::string& elementName,
                                        std::vector<std::string > &listOutputs, const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find the property value of element as per current scenario and condition
     * @param elementType: type of element
     *        conditionInstance: condition which need to be evaluated
     *        elementName: name of the element whose user volume is needed
     *        listOutputs: list in which output will be returned
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return E_OK if property value is found
     *         E_UNKNOWN if property value is not found
     */
    am_Error_e _findUserPropertyValue(const gc_Element_e elementType,
                                      const gc_ConditionStruct_s &conditionInstance,
                                      std::string& elementName,
                                      std::vector<std::string > &listOutputs, const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find whether the condition is for evaluating single domain or for multiple domain evaluation
     * @param conditionInstance: condition which need to be evaluated
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        mandatoryParameter: mandatory parameter of condition structure
     * @return true if request is for single domain
     *         false if request is for multiple domain
     */
    bool _isSingleDomainRequest(const gc_ConditionStruct_s &condition, const bool isLHS,
                                std::string& mandatoryParameter) const;
    /**
     * @brief It is the supporting internal function used to find the connection state of all
     * connection which involves given element.
     * @param condition: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element
     * @return E_OK if connection found
     *         E_UNKNOWN if connection not found
     */
    am_Error_e _findElementConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           std::string& mandatoryParameter, const bool isLHS,
                                           const gc_Element_e elementType);
    /**
     * @brief It is the supporting internal function used to find the mute state of element as received in trigger
     * @param condition: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element
     * @return E_OK
     */
    am_Error_e _findUserMuteState(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find the mute state of element as per current scenario and condition
     * @param condition: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: parameters as received in hook on which this condition is getting executed
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element
     * @return E_OK if mute state found
     *         E_UNKNOWN if mute state not found
     */
    am_Error_e _findMuteState(const gc_ConditionStruct_s &condition,
                              std::vector<std::string > &listOutputs,
                              std::string& mandatoryParameter, const bool isLHS,
                              const gc_Element_e elementType);


    am_Error_e _findSinkNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSourceNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findUserNotificationStatus(const gc_ConditionStruct_s &conditionInstance,
                                     std::vector<std::string > &listOutputs,
                                     const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSinkNTParam(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSourceNTParam(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e __findUserNotificationParam(const gc_ConditionStruct_s &conditionInstance,
                                    std::vector<std::string > &listOutputs,
                                    const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findUserNotificationValue(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findUserNotificationType(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSinkMainNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSourceMainNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSinkMainNTParam(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    am_Error_e _findSourceMainNTParam(const gc_ConditionStruct_s &conditionInstance,
                                 std::vector<std::string > &listOutputs,
                                 const gc_triggerParams_s &parameters, const bool isLHS);
    /**
     * @brief It is the supporting internal function used to find the name of element from connection list
     * @param mandatoryParameter: mandatory parameter of condition
     *        optionalParameter: optional parameter of condition
     *        listOutputs: list in which output will be returned
     *        parameters: parameters as passed in hook
     *        isSinkRequired: to indicate whether sink or source name is required
     * @return E_OK if element name found
     *         E_UNKNOWN if element name not found
     */
    am_Error_e _getElementNameList(const std::string& mandatoryParameter,
                                   const std::string& optionalParameter,
                                   std::vector<std::string > &listOutputs,
                                   const gc_triggerParams_s &parameters, const bool isSinkRequired);
    /**
     * @brief It is the supporting internal template function used to find the domain or class name of element
     * @param elementInstance: type of the element instance
     *        conditionInstance: condition to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: value equivalent of mandatory parameter of condition as received in hook
     *        isLHS:to indicate whether RHS or LHS side function of condition need to be evaluated
     *        isClassRequired: to indicate whether class or domain name is required
     * @return E_OK if class/domain name found
     *         E_UNKNOWN if class/domain name not found
     */
    template <typename Telement>
    am_Error_e _findDomainOrClassOfElementName(Telement& elementInstance,
                                               const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               std::string& mandatoryParameter, const bool isLHS,
                                               const bool isClassRequired);
    /**
     * @brief It is the internal function used to find the element interrupt state as per current scenario and condition
     * @param conditionInstance: condition which need to be evaluated
     *        listOutputs: list in which output will be returned
     *        mandatoryParameter: element name
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     *        elementType: type of element whose interrupt state information is needed
     * @return E_OK if availability is found
     *         E_UNKNOWN if availability is not found
     */
    am_Error_e _findInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                   std::vector<std::string > &listOutputs,
                                   std::string& mandatoryParameter, const bool isLHS,
                                   gc_Element_e elementType);
    /**
     * @brief It is the supporting internal template function used to find the priority of element
     * @param elementInstance: type of the element instance
     *        conditionInstance: condition to be evaluated
     *        mandatoryParameter: value equivalent of mandatory parameter of condition as received in hook
     *        isLHS:to indicate whether RHS or LHS side function of condition need to be evaluated
     *        listOutputs: list in which output will be returned
     * @return E_OK if priority is found
     *         E_UNKNOWN if priority is not found
     */
    template <typename T>
    am_Error_e _findElementPriority(T& elementInstance,
                                    const gc_ConditionStruct_s &conditionInstance,
                                    std::string& mandatoryParameter, const bool isLHS,
                                    std::vector<std::string > &listOutputs);

    /**
     * @brief It is API providing the interface to execute the function as defined in conditions as
     * given in policy set of configuration file
     * @param functionName: function name
     *        category: function category
     *        conditionInstance: structure in which condition is stored
     *        listOutputs: list in which output of function execution will be stored
     *        parameters: parameters received in hook from framework side during which this function need to be evaluated
     *        isLHS: to indicate whether RHS or LHS side function of condition need to be evaluated
     * @return false on error
     *         true on success
     */
    bool _executeFunction(const std::string& functionName, const std::string& category,
                          const gc_ConditionStruct_s &conditionInstance,
                          std::vector<std::string > &listOutputs,
                          const gc_triggerParams_s &parameters, const bool isLHS);

    /**
     * @brief It is API providing the interface to get the actions related to trigger from configuration
     * @param trigger: trigger type for which actions are needed
     *        listActions: variable in which list of actions will be returned
     *        parameters: parameters received in hook
     * @return E_NOT_POSSIBLE on error
     *         E_OK on success
     */
    am_Error_e _getActions(const gc_Trigger_e triggerType, std::vector<gc_Action_s > &listActions,
                           const gc_triggerParams_s& parameters);
    am_Error_e _findNTStatusParam(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,const bool isLHS,
                                              std::string& mandatoryParameter, gc_Element_e elementType,
                                              am_CustomNotificationType_t ntType,const bool isStatusReq);
    am_Error_e _findMainNTStatusParam(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,const bool isLHS,
                                              std::string& mandatoryParameter, gc_Element_e elementType,
                                              am_CustomNotificationType_t ntType,const bool isStatusReq);
    void _removeDoubleQuotes(std::string& inputString,const std::string& replaceString);
    void _getImplicitActions(gc_Trigger_e trigger, std::vector<gc_Action_s >& listActions,
                             const gc_triggerParams_s& parameters);
    void _getListStaticSinks(const std::string& domainName,
                             std::vector<gc_Sink_s >& listStaticSinks);
    void _getListStaticSources(const std::string& domainName,
                               std::vector<gc_Source_s >& listStaticSources);
    void _getListStaticGateways(std::vector<std::string >& listGateways,
                                std::string& listSources,std::string& listSinks);
    am_Error_e _updateActionParameters(std::vector<gc_Action_s >& listActions,
                                       gc_triggerParams_s& triggerParams);
    typedef am_Error_e (CAmPolicyEngine::*functionPtr)(const gc_ConditionStruct_s &condition,
                                                       std::vector<std::string > &listOutputs,
                                                       const gc_triggerParams_s &parameters,
                                                       const bool isLHS);

    //map to store function pointers of name category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapNameFunctions;
    //map to store function pointers of priority category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapPriorityFunctions;
    //map to store function pointers of connection state category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapConnectionStateFunctions;
    //map to store function pointers of volume category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapVolumeFunctions;
    //map to store function pointers of main volume category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainVolumeFunctions;
    //map to store function pointers of sound property category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapSoundPropertyValueFunctions;
    //map to store function pointers of main sound property value category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainSoundPropertyValueFunctions;
    //map to store function pointers of system property value category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapSystemPropertyValueFunctions;
    //map to store function pointers of main sound property type category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainSoundPropertyTypeFunctions;
    //map to store function pointers of system property type category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapSystemPropertyTypeFunctions;
    //map to store function pointers of mute state category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMuteStateFunctions;
    //map to store function pointers of registration complete category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapIsRegistrationCompleteFunctions;
    //map to store function pointers of availability category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapAvailabilityFunctions;
    //map to store function pointers of availability reason category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapAvailabilityReasonFunctions;
    //map to store function pointers of connection format category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapConnectionFormatFunctions;
    //map to store function pointers of interrupt state category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapInterruptStateFunctions;
    //map to store function pointers of finding element registration category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapIsRegisteredFunctions;
    //map to store function pointers of element state category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapStateFunctions;
    //map to store function pointers of error category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapErrorFunctions;
    //map to store function pointers of notification configuration status category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapNotificationStatusFunctions;
    //map to store function pointers of notification configuration param category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapNotificationParamFunctions;
    //map to store function pointers of notification configuration value category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapNotificationValueFunctions;
    //map to store function pointers of main notification configuration type category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainNotificationTypeFunctions;
    //map to store function pointers of main notification configuration status category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainNotificationStatusFunctions;
    //map to store function pointers of main notification configuration param category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapMainNotificationParamFunctions;
    //map to store function pointers of peek category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapPeekFunctions;
    //map to store function pointers of all domain registration error category function of condition set as defined in policy
    std::map<std::string, functionPtr > mMapDomainRegistrationErrorFunctions;
    //map to store map of function pointers
    std::map<std::string, std::map<std::string, functionPtr > > mMapFunctionNameToFunctionMaps;
    std::map<std::string, std::string > mMapActions;
    // instance of config reader class
    CAmConfigurationReader* mpConfigReader;
    //map to store function return value type. This are the functions supported in condition of policy
    std::map<std::string, bool > mMapFunctionReturnValue;

    //pointer to store policy receive class instance
    IAmPolicyReceive* mpPolicyReceive;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_POLICYENGINE_H_ */
