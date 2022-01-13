/******************************************************************************
 * @file: IAmPolicyReceive.h
 *
 * This file contains the declaration of abstract class used to provide
 * the interface for framework side to provide API to policy engine to get the
 * context
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

#ifndef GC_IPOLICYRECEIVE_H_
#define GC_IPOLICYRECEIVE_H_

#include "IAmControlCommon.h"
#include <map>
#include "CAmTypes.h"
namespace am {
namespace gc {
class IAmPolicyReceive
{

public:
    IAmPolicyReceive()
    {
    }
    virtual ~IAmPolicyReceive()
    {
    }
    virtual am_Error_e setListActions(std::vector<gc_Action_s >& listActions,
                                      gc_ActionList_e actionListType = AL_NORMAL) =0;
    virtual bool isDomainRegistrationComplete(const std::string& domainName) =0;
    virtual bool isRegistered(const gc_Element_e elementType, const std::string& elementName) =0;
    virtual am_Error_e getAvailability(const gc_Element_e elementType,
                                       const std::string& elementName,
                                       am_Availability_s& availability) =0;
    virtual am_Error_e getState(const gc_Element_e elementType, const std::string& elementName,
                                int& elementState) = 0;
    virtual am_Error_e getInterruptState(const gc_Element_e elementType,
                                         const std::string& elementName,
                                         am_InterruptState_e& interruptState) =0;
    virtual am_Error_e getMuteState(const gc_Element_e elementType, const std::string& elementName,
                                    am_MuteState_e& MuteState) =0;
    virtual am_Error_e getMainSoundProperty(const gc_Element_e elementType,
                                            const std::string& elementName,
                                            const am_CustomMainSoundPropertyType_t propertyType,
                                            int16_t& value) =0;
    virtual am_Error_e getSoundProperty(const gc_Element_e elementType,
                                        const std::string& elementName,
                                        const am_CustomSoundPropertyType_t propertyType,
                                        int16_t &Value) =0;
    virtual am_Error_e getSystemProperty(const am_CustomSystemPropertyType_t systempropertyType,
                                         int16_t& Value) =0;
    virtual am_Error_e getVolume(const gc_Element_e elementType, const std::string& elementName,
                                 am_volume_t& DeviceVolume) =0;
    virtual am_Error_e getMainVolume(const gc_Element_e elementType, const std::string& elementName,
                                     am_mainVolume_t& volume) =0;
    virtual am_Error_e getListMainConnections(const gc_Element_e elementType,
                                              const std::string& name,
                                              std::vector<gc_ConnectionInfo_s >& listConnectionInfos) =0;
    virtual am_Error_e getListNotificationConfigurations(const gc_Element_e elementType,
                                                         const std::string& name,
                                                         std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations) =0;
    virtual am_Error_e getListMainNotificationConfigurations(const gc_Element_e elementType,
                                                             const std::string& name,
                                                             std::vector<am_NotificationConfiguration_s >& listMainNotificationConfigurations) =0;
virtual am_Error_e getListMainConnections(const std::string& name,
                                                  std::vector<gc_ConnectionInfo_s >& listConnectionInfos,gc_Order_e order) =0;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_IPOLICYENGINERECEIVE_H_ */
