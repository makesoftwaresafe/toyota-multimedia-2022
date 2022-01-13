/******************************************************************************
 * @file: IAmPolicySend.h
 *
 * This file contains the declaration of abstract class used to provide
 * the interface for policy engine side to provide API to framework to pass the
 * trigger and get actions related to trigger from policy engine
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

#ifndef GC_IPOLICYSEND_H_
#define GC_IPOLICYSEND_H_

#include "CAmTypes.h"
namespace am {
namespace gc {
class IAmPolicyReceive;
class IAmPolicySend
{
public:
    IAmPolicySend()
    {
    }
    virtual ~IAmPolicySend()
    {
    }
    virtual am_Error_e startupInterface(IAmPolicyReceive* pPolicyReceive) =0;
    virtual am_Error_e hookRegisterDomain(const std::string& domainName,
                                          const am_Error_e status) =0;
    virtual am_Error_e hookRegisterSource(const std::string& sourceName,
                                          const am_Error_e status) =0;
    virtual am_Error_e hookRegisterSink(const std::string& sinkName,
                                        const am_Error_e status) =0;
    virtual am_Error_e hookRegisterGateway(const std::string& gatewayName,
                                           const am_Error_e status) =0;
    virtual am_Error_e hookDeregisterDomain(const std::string& domainName,
                                            const am_Error_e status) =0;
    virtual am_Error_e hookDeregisterSource(const std::string& sourceName,
                                            const am_Error_e status) =0;
    virtual am_Error_e hookDeregisterSink(const std::string& sinkName,
                                          const am_Error_e status) =0;
    virtual am_Error_e hookDeregisterGateway(const std::string& gatewayName,
                                             const am_Error_e status) =0;
    virtual am_Error_e hookDomainRegistrationComplete(const std::string& domainName) =0;
    virtual am_Error_e hookAllDomainRegistrationComplete(const am_Error_e& error) =0;
    virtual am_Error_e hookConnectionRequest(const std::string& className,
                                             const std::string& sourceName,
                                             const std::string& sinkName) =0;
    virtual am_Error_e hookDisconnectionRequest(const std::string& className,
                                                const std::string& sourceName,
                                                const std::string& sinkname) =0;
    virtual am_Error_e hookSourceAvailabilityChange(const std::string& sourceName,
                                                    const am_Availability_s& Availability) =0;
    virtual am_Error_e hookSinkAvailabilityChange(const std::string& sinkName,
                                                  const am_Availability_s& Availability) =0;
    virtual am_Error_e hookSourceInterruptStateChange(const std::string& sourceName,
                                                      const am_InterruptState_e interruptState) =0;
    virtual am_Error_e hookSetMainSourceSoundProperty(const std::string& sourceName,
                                                      const am_MainSoundProperty_s& soundProperty) =0;
    virtual am_Error_e hookSetMainSinkSoundProperty(const std::string& sinkName,
                                                    const am_MainSoundProperty_s& soundProperty) =0;
    virtual am_Error_e hookSetSystemProperty(const am_SystemProperty_s& systemProperty) =0;

    virtual am_Error_e hookSetMainSinkNotificationConfiguration(
                    const std::string& sinkName,
                    const am_NotificationConfiguration_s& notificationConfiguration) =0;
    virtual am_Error_e hookSetMainSourceNotificationConfiguration(
                    const std::string& sourceName,
                    const am_NotificationConfiguration_s& notificationConfiguration) =0;
    virtual am_Error_e hookSinkNotificationDataChanged(const std::string& sinkName,
                                                       const am_NotificationPayload_s& payload)=0;
    virtual am_Error_e hookSourceNotificationDataChanged(const std::string& sourceName,
                                                         const am_NotificationPayload_s& payload)=0;

    virtual am_Error_e hookVolumeChange(const std::string& sinkName,
                                        const am_mainVolume_t mainVolume) =0;
    virtual am_Error_e hookSetSinkMuteState(const std::string& sinkName,
                                            const am_MuteState_e muteState) =0;
    virtual am_Error_e hookConnectionStateChange(const std::string& connectionName,
                                                   const am_ConnectionState_e& connectionState,
                                                   am_Error_e& status) =0;
    virtual am_Error_e getListElements(const std::vector<std::string >& listNames,
                                       std::vector<gc_Sink_s >& listSinks)=0;
    virtual am_Error_e getListElements(const std::vector<std::string >& listNames,
                                       std::vector<gc_Source_s >& listSources)=0;
    virtual am_Error_e getListElements(const std::vector<std::string >& listNames,
                                       std::vector<gc_Gateway_s >& listGateways)=0;
    virtual am_Error_e getListElements(const std::vector<std::string >& listNames,
                                       std::vector<gc_Domain_s >& listDomain)=0;
    virtual am_Error_e getListSystemProperties(std::vector<am_SystemProperty_s>& listSystemProperties)=0;
    virtual am_Error_e getListClasses(std::vector<gc_Class_s >& listClasses)=0;
    virtual am_Error_e getListDomains(std::vector<gc_Domain_s >& listDomains)=0;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_IPOLICYENGINESEND_H_ */
