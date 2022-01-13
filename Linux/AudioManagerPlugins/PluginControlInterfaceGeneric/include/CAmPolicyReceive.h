/******************************************************************************
 * @file: CAmPolicyReceive.h
 *
 * This file contains the declaration of policy engine receive class (member
 * functions and data members) used to provide the interface to policy engine to
 * get the context of controller as per current scenario
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

#ifndef GC_POLICYRECEIVE_H_
#define GC_POLICYRECEIVE_H_

#include <string>
#include <vector>

#include "IAmPolicyReceive.h"

namespace am {
namespace gc {

class CAmControlReceive;
class IAmPolicySend;
class CAmElement;
class CAmMainConnectionElement;
class CAmClassElement;

typedef CAmElement* (*getElementFptr)(const std::string);

class CAmPolicyReceive : public IAmPolicyReceive
{
public:
    /**
     * @brief It is the constructor of policy Engine receive class. Initialize the member
     * variables with default value.It will be invoked during startupController phase.
     * @param pRoutingManager: pointer to routing manager class instance
     * @return none
     */
    CAmPolicyReceive(CAmControlReceive* pControlReceive, IAmPolicySend* pPolicySend);
    /**
     * @brief It is the destructor of policy Engine receive class.
     * @param none
     * @return none
     */
    ~CAmPolicyReceive()
    {
    }
    /**
     * @brief It is the API providing the interface to policy engine to set the list of actions to
     * framework.
     * @param listActions: list of actions to be executed
     * @return E_NO_CHANGE if action list is empty
     *         E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e setListActions(std::vector<gc_Action_s >& listActions,
                              gc_ActionList_e actionListType);
    /**
     * @brief It is the API providing the interface to policy engine to check if domain
     * registration is completed or not
     * @param domainName: name of domain for which registration completion need to be checked
     * @return true if completed
     *         false if not completed
     */
    bool isDomainRegistrationComplete(const std::string& domainName);
    /**
     * @brief It is the API providing the interface to policy engine to check if element is
     * registered or not
     * @param elementType: element type
     *        elementName: name of element
     * @return true if completed
     *         false if not completed
     */
    bool isRegistered(const gc_Element_e elementType, const std::string& elementName);
    /**
     * @brief It is the API providing the interface to policy engine to get the availability of
     * element
     * @param elementType: element type
     *        elementName: name of element
     *        availabilityInstance: availability status
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getAvailability(const gc_Element_e elementType, const std::string& elementName,
                               am_Availability_s& availabilityInstance);
    /**
     * @brief It is the API providing the interface to policy engine to get the state of domain or source
     * @param elementType: element type. Possible values are ET_SOURCE or ET_DOMAIN
     * 		  elementName: name of the element whose state is needed
     *        elementState: state of the element
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getState(const gc_Element_e elementType, const std::string& elementName,
                        int& elementState);
    /**
     * @brief It is the API providing the interface to policy engine to get the interrupt state of source
     * @param elementType: element type
     *        elementName: name of element
     *        interruptState: variable in which interrupt state of the element will be returned
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getInterruptState(const gc_Element_e elementType, const std::string& elementName,
                                 am_InterruptState_e& interruptState);
    /**
     * @brief It is the API providing the interface to policy engine to get the mute state of element
     * @param elementType: element type
     *        elementName: name of element
     *        muteState: mute state
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getMuteState(const gc_Element_e elementType, const std::string& Name,
                            am_MuteState_e& muteState);
    /**
     * @brief It is the API providing the interface to policy engine to get the sound property value of element
     * @param elementType: element type
     *        elementName: name of element
     *        propertyType: type of property whose value is needed
     *        value: value of property
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getSoundProperty(const gc_Element_e elementType, const std::string& elementName,
                                const am_CustomSoundPropertyType_t propertyType, int16_t &value);
    /**
     * @brief It is the API providing the interface to policy engine to get the system property value
     * @param systempropertyType: type of property whose value is needed
     *        value: value of property
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getSystemProperty(const am_CustomSystemPropertyType_t systempropertyType,
                                 int16_t& value);
    /**
     * @brief It is the API providing the interface to policy engine to get the main sound property
     * value of element
     * @param elementType: element type
     *        elementName: name of element
     *        propertyType: type of property whose value is needed
     *        value: value of property
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getMainSoundProperty(const gc_Element_e elementType, const std::string& elementName,
                                    const am_CustomMainSoundPropertyType_t propertyType,
                                    int16_t& value);
    /**
     * @brief It is the API providing the interface to policy engine to get the volume value of
     * element
     * @param elementType: element type
     *        elementName: name of element
     *        deviceVolume: value of volume
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getVolume(const gc_Element_e elementType, const std::string& elementName,
                         am_volume_t& deviceVolume);
    /**
     * @brief It is the API providing the interface to policy engine to get the main volume value of
     * element
     * @param elementType: element type
     *        elementName: name of element
     *        volume: value of main/user volume
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getMainVolume(const gc_Element_e elementType, const std::string& elementName,
                             am_mainVolume_t& volume);
    /**
     * @brief It is the API providing the interface to policy engine to get the list of main connection
     * involving the given element
     * @param elementType: element type
     *        elementName: name of element
     *        listConnectionInfo: list of connection information
     * @return E_OK on success
     *         E_NOT_POSSIBLE on error
     */
    am_Error_e getListMainConnections(const gc_Element_e elementType,
                                      const std::string& elementName,
                                      std::vector<gc_ConnectionInfo_s >& listConnectionInfos);
    am_Error_e getListMainConnections(const std::string& name,
                                      std::vector<gc_ConnectionInfo_s >& listConnectionInfos,gc_Order_e order);
    am_Error_e getListNotificationConfigurations(
                    const gc_Element_e elementType, const std::string& name,
                    std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations);
    am_Error_e getListMainNotificationConfigurations(
                    const gc_Element_e elementType,
                    const std::string& name,
                    std::vector<am_NotificationConfiguration_s >& listMainNotificationConfigurations);
private:
    /**
     * @brief It is the internal function used to store the list of connection belonging to class
     * @param pClass: name of the class
     *        listConnectionInfo: list of connection information
     * @return none
     */
    void _getConnectionInfo(CAmClassElement* pClass,
                            std::vector<gc_ConnectionInfo_s >& listConnectionInfo);
    /**
     * @brief It is the template function used to find whether element is registered or not
     * @param listElements: list of elements
     *        name: name of the element
     * @return none
     */
    template <class T>
    bool _isElementRegistered(const std::vector<T >& listElements, const std::string& name);
    CAmElement* _getElement(const gc_Element_e type, const std::string& name);
    static bool _sortingLowest(gc_ConnectionInfo_s i, gc_ConnectionInfo_s j);
    static bool _sortingHighest(gc_ConnectionInfo_s i, gc_ConnectionInfo_s j);
    CAmControlReceive* mpControlReceive;
    IAmPolicySend* mpPolicySend;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_POLICYRECEIVE_H_ */
