/******************************************************************************
 * @file: CAmPolicySend.h
 *
 * This file contains the declaration of policy engine send class (member
 * functions and data members) used to provide the interface to framework to
 * pass the hook/trigger to policy engine to get the actions
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

#ifndef GC_POLICYSEND_H_
#define GC_POLICYSEND_H_

#include "IAmPolicySend.h"
#include "CAmPolicyEngine.h"

namespace am {
namespace gc {
class CAmPolicySend : public IAmPolicySend
{
public:
    /**
     * @brief It is the constructor of policy Engine send class. Initialize the member
     * variables with default value.It will be invoked during startupController phase.
     * @param none
     * @return none
     */
    CAmPolicySend(void);
    /**
     * @brief It is the destructor of policy Engine send class.
     * @param none
     * @return none
     */
    ~CAmPolicySend(void);
    /**
     * @brief It is the API providing the interface to start the policy engine.
     * It instantiate the policy engine class and config reader class
     * @param pPolicyReceive: pointer to policy engine receive class instance
     * @return E_UNKNOWN on internal error
     *         E_OK on success
     */
    am_Error_e startupInterface(IAmPolicyReceive* pPolicyReceive);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * register domain request from routing adaptor to policy engine.
     * @param domainName: name of domain for which registration request received
     *        status: status of domain registration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookRegisterDomain(const std::string& domainName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * register source request to policy engine.
     * @param sourceName: name of source for which registration request received
     *        status: status of source registration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookRegisterSource(const std::string& sourceName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * register sink request to policy engine.
     * @param sinkName: name of sink for which registration request received
     *        status: status of sink registration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookRegisterSink(const std::string& sinkName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * register gateway request to policy engine.
     * @param gatewayName: name of gateway for which registration request received
     *        status: status of gateway registration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookRegisterGateway(const std::string& gatewayName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * unregister domain request from routing adaptor to policy engine.
     * @param domainName: name of domain for which unregistration request received
     *        status: status of domain unregistration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDeregisterDomain(const std::string& domainName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * unregister source request to policy engine.
     * @param sourceName: name of source for which unregistration request received
     *        status: status of source unregistration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDeregisterSource(const std::string& sourceName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * unregister sink request to policy engine.
     * @param sinkName: name of sink for which unregistration request received
     *        status: status of sink unregistration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDeregisterSink(const std::string& sinkName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * unregister gateway request to policy engine.
     * @param gatewayName: name of gateway for which unregistration request received
     *        status: status of gateway unregistration request
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDeregisterGateway(const std::string& gatewayName, const am_Error_e status);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * domain registration complete request from routing adaptor to policy engine.
     * @param domainName: name of domain whose registration is completed
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDomainRegistrationComplete(const std::string& domainName);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * domain registration complete request from routing adaptor to policy engine.
     * @param error: The result of the all domain registration, success or timeout
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookAllDomainRegistrationComplete(const am_Error_e& error);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * connection request from application to policy engine.
     * @param className: name of class in which connection need to be created
     *        sourceName: name of main source
     *        sinkName: name of main sink
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookConnectionRequest(const std::string& className, const std::string& sourceName,
                                     const std::string& sinkName);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * disconnection request from application to policy engine.
     * @param className: name of class in which connection exist and need to be disconnected
     *        sourceName: name of main source
     *        sinkName: name of main sink
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookDisconnectionRequest(const std::string& className, const std::string& sourceName,
                                        const std::string& sinkname);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * source availability change request from routing adaptor to policy engine.
     * @param sourceName: name of source whose availability is changed
     *        availabilityInstance: new availability status
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSourceAvailabilityChange(const std::string& sourceName,
                                            const am_Availability_s& availabilityInstance);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * sink availability change request from routing adaptor to policy engine.
     * @param sinkName: name of sink whose availability is changed
     *        availabilityInstance: new availability status
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSinkAvailabilityChange(const std::string& sinkName,
                                          const am_Availability_s& availabilityInstance);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * source interrupt change request from routing adaptor to policy engine.
     * @param sourceName: name of source whose availability is changed
     *        interruptState: new interrupt state
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSourceInterruptStateChange(const std::string& sourceName,
                                              const am_InterruptState_e interruptState);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * setting main source sound property request from application to policy engine.
     * @param sourceName: name of main source
     *        soundProperty: new sound property
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetMainSourceSoundProperty(const std::string& sourceName,
                                              const am_MainSoundProperty_s& soundProperty);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * setting main sink sound property request from application to policy engine.
     * @param sinkName: name of main sink
     *        soundProperty: new sound property
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetMainSinkSoundProperty(const std::string& sinkName,
                                            const am_MainSoundProperty_s& soundProperty);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * setting system property request from application to policy engine.
     * @param systemProperty: new system property
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetSystemProperty(const am_SystemProperty_s& systemProperty);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * sink notification configuration from application to policy engine.
     * @param sinkName: name of Sink
     * @param notificationConfiguration: Notification configuration
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetMainSinkNotificationConfiguration(
                    const std::string& sinkName,
                    const am_NotificationConfiguration_s& notificationConfiguration);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * source notification configuration from application to policy engine.
     * @param sourceName: name of Source
     * @param notificationConfiguration: Notification configuration
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetMainSourceNotificationConfiguration(
                    const std::string& sourceName,
                    const am_NotificationConfiguration_s& notificationConfiguration);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * sink notification data change from application to policy engine.
     * @param sinkName: name of Sink
     * @param payload: Notification Data
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSinkNotificationDataChanged(const std::string& sinkName,
                                               const am_NotificationPayload_s& payload);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * source notification data change from application to policy engine.
     * @param sourceName: name of Sink
     * @param payload: Notification Data
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSourceNotificationDataChanged(const std::string& sourceName,
                                                 const am_NotificationPayload_s& payload);

    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * change volume request from application to policy engine.
     * @param sinkName: name of sink
     *        mainVolume: new volume requested
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookVolumeChange(const std::string& sinkName, const am_mainVolume_t mainVolume);
    /**
     * @brief It is the API providing the interface to framework to pass the hook of
     * change mute state request from application to policy engine.
     * @param sinkName: name of sink
     *        muteState: new mute state requested
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e hookSetSinkMuteState(const std::string& sinkName, const am_MuteState_e muteState);
    am_Error_e hookConnectionStateChange(const std::string& connectionName,
                                                   const am_ConnectionState_e& connectionState,
                                                   am_Error_e& status);
    am_Error_e hookStoredMainConnectionVolume(const std::string& connectionName,
                                                    const am_mainVolume_t mainVolume);
    /**
     * @brief This API returns the list of configuration data of a sink by list of names.
     * @param listNames The list of the sink names
     * @param listSinks The list of sink configuration data.
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e getListElements(const std::vector<std::string >& listNames,
                               std::vector<gc_Sink_s >& listSinks);
    /**
     * @brief This API returns the list of configuration data of a source by list of names.
     * @param listNames The list of the source names
     * @param listSources The list of source configuration data.
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e getListElements(const std::vector<std::string >& listNames,
                               std::vector<gc_Source_s >& listSources);
    /**
     * @brief This API returns the list of configuration data of a gateway by list of names.
     * @param listNames The list of the sink names
     * @param listGateways The list of gateway configuration data.
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e getListElements(const std::vector<std::string >& listNames,
                               std::vector<gc_Gateway_s >& listGateways);
    /**
     * @brief This API returns the list of configuration data of a domain by list of names.
     * @param listNames The list of the domains names
     * @param listDomains The list of domain configuration data.
     * @return E_NOT_POSSIBLE on internal error
     *         E_OK on success
     */
    am_Error_e getListElements(const std::vector<std::string >& listNames,
                               std::vector<gc_Domain_s >& listDomains);
    am_Error_e getListSystemProperties(std::vector<am_SystemProperty_s >& listSystemProperties);
    am_Error_e getListClasses(std::vector<gc_Class_s >& listClasses);
    am_Error_e getListDomains(std::vector<gc_Domain_s >& listDomains);

private:
    template <typename TinElement, typename ToutElement>
    void _copyElementData(const TinElement& inputData, ToutElement& outputData);
    // pointer to policy engine class to get the actions as per configuration file
    CAmPolicyEngine* mpPolicyEngine;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_POLICYSEND_H_ */
